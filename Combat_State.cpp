module LostArkMultiTool.Models.CombatState;

import std;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatCoefficientData;

namespace LostArkMultiTool::Models
{
    namespace
    {
        namespace Data = CombatCoefficientData;

        // ===== 세트 판별 (이름에 들어 있는 글자) =====
        constexpr std::string_view KEY_SEPARATOR{ "_" };
        constexpr std::string_view ESTHER_PREFIX{ "에스더" };
        constexpr std::string_view ESTHER_GRADE_NAME{ "에스더" };
        // 에스더 무기: 이름이 겹치는 단계가 있어서("테라 에크혼") 높은 단계부터 확인
        constexpr std::array<std::string_view, 4> ESTHER_KEYWORDS{ "테라", "기가", "네오", "에크혼" };
        // 일반 세트 (이름 → 분류 키 앞부분이 같은 글자)
        constexpr std::array<std::string_view, 3> SET_KEYWORDS{ "결단", "업화", "전율" };

        constexpr int MIN_ENHANCEMENT{ 0 };
        constexpr int MIN_ADVANCED_ENHANCEMENT{ 0 };

        // 표의 한 칸 (범위 밖 · 측정 안 함이면 nullopt)
        [[nodiscard]] std::optional<double> ReadCell(std::span<const double> values, std::ptrdiff_t index) noexcept
        {
            if (index < 0 || static_cast<std::size_t>(index) >= values.size()) return std::nullopt;
            const double value{ values[static_cast<std::size_t>(index)] };
            if (value == Data::MISSING_STAT) return std::nullopt;
            return value;
        }

        // (N, M)의 레벨별 표 번호 = 부위 아이템 레벨 - 표 첫 칸 레벨
        [[nodiscard]] std::ptrdiff_t GetLevelIndex(const Data::EquipmentTable& table, int enhancement, int advancedEnhancement) noexcept
        {
            const Data::ItemLevelRule& rule{ Data::GetItemLevelRule() };
            const int itemLevel{ rule.m_baseItemLevel + enhancement * rule.m_normalStepLevel + advancedEnhancement * rule.m_advancedStepLevel };
            return static_cast<std::ptrdiff_t>(itemLevel - table.m_firstLevel);
        }

        // 상급 단계에 맞는 보너스 배율 (구간에 들지 않으면 1)
        [[nodiscard]] double GetAdvancedMultiplier(int advancedEnhancement) noexcept
        {
            double multiplier{ 1.0 };
            for (const Data::AdvancedBonus& bonus : Data::GetAdvancedBonuses())
            {
                if (advancedEnhancement >= bonus.m_minStep) multiplier = bonus.m_multiplier;   // 오름차순 → 마지막으로 맞은 구간
            }
            return multiplier;
        }

        // 이 일반 단계에서 넣을 수 있는 상급 재련 최대 단계 (표 끝 · 최대 단계 중 작은 값)
        [[nodiscard]] int GetMaxAdvanced(const Data::EquipmentTable& table, int enhancement) noexcept
        {
            const Data::ItemLevelRule& rule{ Data::GetItemLevelRule() };
            const std::ptrdiff_t lastIndex{ static_cast<std::ptrdiff_t>(table.m_byLevel.size()) - 1 };
            const std::ptrdiff_t room{ lastIndex - GetLevelIndex(table, enhancement, MIN_ADVANCED_ENHANCEMENT) };
            if (room <= 0 || rule.m_advancedStepLevel <= 0) return MIN_ADVANCED_ENHANCEMENT;
            return std::min(rule.m_maxAdvancedEnhancement, static_cast<int>(room / rule.m_advancedStepLevel));
        }

        [[nodiscard]] std::string MakeKey(std::string_view prefix, std::string_view suffix)
        {
            return std::string{ prefix } + std::string{ KEY_SEPARATOR } + std::string{ suffix };
        }
    }

    bool GearState::IsKnown() const noexcept
    {
        return !m_tableKey.empty();
    }

    namespace CombatGear
    {
        std::optional<double> GetStat(const GearState& gear)
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };
            if (table == nullptr) return std::nullopt;

            // 상급 0 → 일반 표 그대로
            if (gear.m_advancedEnhancement <= MIN_ADVANCED_ENHANCEMENT) return ReadCell(table->m_normal, gear.m_enhancement);

            // 상급 > 0 → 레벨별 표 × 보너스, 소수점 버림 (lopec 표와 같은 규칙)
            const std::optional<double> value{ ReadCell(table->m_byLevel, GetLevelIndex(*table, gear.m_enhancement, gear.m_advancedEnhancement)) };
            if (!value.has_value()) return std::nullopt;
            return std::floor(*value * GetAdvancedMultiplier(gear.m_advancedEnhancement));
        }

        int GetMaxEnhancement(std::string_view tableKey)
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(tableKey) };
            if (table == nullptr || table->m_normal.empty()) return MIN_ENHANCEMENT;
            return static_cast<int>(table->m_normal.size()) - 1;
        }

        bool CanAdvance(const GearState& gear)
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };
            if (table == nullptr || table->m_byLevel.empty() || table->m_minAdvancedEnhancement < MIN_ENHANCEMENT) return false;
            return gear.m_enhancement >= table->m_minAdvancedEnhancement && GetMaxAdvanced(*table, gear.m_enhancement) > MIN_ADVANCED_ENHANCEMENT;
        }

        GearState Clamp(const GearState& gear)
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };
            if (table == nullptr) return gear;

            GearState clamped{ gear };
            clamped.m_enhancement = std::clamp(gear.m_enhancement, MIN_ENHANCEMENT, GetMaxEnhancement(gear.m_tableKey));
            clamped.m_advancedEnhancement = CanAdvance(clamped)
                ? std::clamp(gear.m_advancedEnhancement, MIN_ADVANCED_ENHANCEMENT, GetMaxAdvanced(*table, clamped.m_enhancement))
                : MIN_ADVANCED_ENHANCEMENT;
            return clamped;
        }

        // 기본 아이템 레벨 기준 올라간 양 (에스더 = 단계별 아이템 레벨 표 - 기본 레벨 → 두 상태의 차이 = 표[목표] - 표[현재])
        int GetItemLevelGain(const GearState& gear)
        {
            const Data::ItemLevelRule& rule{ Data::GetItemLevelRule() };
            const int advancedGain{ gear.m_advancedEnhancement * rule.m_advancedStepLevel };

            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };
            if (table != nullptr && !table->m_itemLevels.empty())
            {
                const std::size_t step{ static_cast<std::size_t>(std::clamp(gear.m_enhancement, MIN_ENHANCEMENT, static_cast<int>(table->m_itemLevels.size()) - 1)) };
                return table->m_itemLevels[step] - rule.m_baseItemLevel + advancedGain;
            }
            return gear.m_enhancement * rule.m_normalStepLevel + advancedGain;
        }

        bool IsEsther(std::string_view tableKey) noexcept
        {
            return tableKey.starts_with(ESTHER_PREFIX);
        }

        // 무기: 에스더 단계 이름 먼저 → 결단 · 업화 · 전율 + 부위 (표에 없는 조합은 빈 문자열)
        std::string DetectTableKey(GearPart part, std::string_view itemName, std::string_view grade)
        {
            std::string key;
            if (part == GearPart::PART_WEAPON)
            {
                for (const std::string_view keyword : ESTHER_KEYWORDS)
                {
                    if (itemName.contains(keyword))
                    {
                        key = MakeKey(ESTHER_PREFIX, keyword);
                        break;
                    }
                }
                if (key.empty() && grade == ESTHER_GRADE_NAME) return {};   // 모르는 에스더 무기
            }

            if (key.empty())
            {
                for (const std::string_view keyword : SET_KEYWORDS)
                {
                    if (itemName.contains(keyword))
                    {
                        key = MakeKey(keyword, GetGearTableSuffix(part));
                        break;
                    }
                }
            }

            return Data::FindEquipmentTable(key) != nullptr ? key : std::string{};
        }
    }
}
