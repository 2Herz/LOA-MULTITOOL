module LostArkMultiTool.Models.CombatExtraState;

import std;
import LostArkMultiTool.Models.CombatCoefficientData;

namespace LostArkMultiTool::Models
{
    namespace
    {
        namespace Data = CombatCoefficientData;

        constexpr char SPACE_CHARACTER{ ' ' };
        constexpr double NO_GRADE_FACTOR{ 1.0 };   // 등급이 바뀌지 않음
        constexpr std::string_view WHITESPACE_CHARACTERS{ " \t\r\n" };

        [[nodiscard]] bool IsSpace(char character) noexcept
        {
            return WHITESPACE_CHARACTERS.contains(character);
        }

        // 팔찌 스탯 줄의 종류가 목록에 있는지 ("힘" 등)
        [[nodiscard]] bool IsIn(std::span<const std::string_view> list, std::string_view value) noexcept
        {
            return std::ranges::find(list, value) != list.end();
        }

        // 완갑 재련 단계의 능력치 (범위를 벗어나면 가장 가까운 끝 단계, 표가 없으면 0)
        [[nodiscard]] const Data::VambraceStep& GetVambraceStats(int level) noexcept
        {
            static constexpr Data::VambraceStep EMPTY_STEP{};
            const int clamped{ std::clamp(level, MIN_VALUE, MAX_VAMBRACE_LEVEL) };
            const Data::VambraceStep* step{ Data::FindVambraceStep(clamped) };
            return step != nullptr ? *step : EMPTY_STEP;
        }
    }

    std::string NormalizeSpaces(std::string_view text)
    {
        std::string result;
        result.reserve(text.size());
        bool isPendingSpace{};
        for (const char character : text)
        {
            if (IsSpace(character))
            {
                isPendingSpace = !result.empty();   // 맨 앞 공백은 버림, 연속 공백은 하나로
                continue;
            }
            if (isPendingSpace) result.push_back(SPACE_CHARACTER);
            isPendingSpace = false;
            result.push_back(character);
        }
        return result;
    }

    std::string RemoveSpaces(std::string_view text)
    {
        std::string result;
        result.reserve(text.size());
        for (const char character : text)
        {
            if (!IsSpace(character)) result.push_back(character);
        }
        return result;
    }

    bool IsNone(std::string_view text) noexcept
    {
        return text.empty() || text == NONE_TEXT;
    }

    namespace CombatExtra
    {
        double GetMainStatGain(const ExtraState& current, const ExtraState& target)
        {
            double gain{};

            // 악세서리 기본 효과 주스탯
            for (std::size_t index{}; index < ACCESSORY_SLOT_COUNT; ++index)
            {
                gain += target.m_accessories[index].m_mainStat - current.m_accessories[index].m_mainStat;
            }

            // 팔찌 스탯 줄 중 힘 · 민첩 · 지능
            const auto sumMainStat{ [](const ExtraState& state)
            {
                double sum{};
                for (const BraceletStat& line : state.m_braceletStats)
                {
                    if (IsIn(Data::GetBraceletMainStatTypes(), line.m_type)) sum += line.m_value;
                }
                return sum;
            } };
            gain += sumMainStat(target) - sumMainStat(current);

            return gain;
        }

        VambraceGain GetVambraceGain(const ExtraState& current, const ExtraState& target)
        {
            VambraceGain gain;
            if (!current.m_vambraceLevel.has_value() || !target.m_vambraceLevel.has_value()) return gain;

            const Data::VambraceStep& from{ GetVambraceStats(*current.m_vambraceLevel) };
            const Data::VambraceStep& to{ GetVambraceStats(*target.m_vambraceLevel) };
            gain.m_weaponAttack = to.m_weaponAttack - from.m_weaponAttack;
            gain.m_mainStat = to.m_mainStat - from.m_mainStat;
            gain.m_baseAttack = to.m_baseAttack - from.m_baseAttack;

            // 유물 → 고대 등급이 필요해지면 한 번 곱함 (고대에서 내려오면 역수)
            const int ancientStep{ Data::GetVambraceAncientFirstStep() };
            const bool isFromAncient{ *current.m_vambraceLevel >= ancientStep };
            const bool isToAncient{ *target.m_vambraceLevel >= ancientStep };
            if (isToAncient && !isFromAncient) gain.m_gradeFactor = Data::GetVambraceGradeUpFactor();
            else if (!isToAncient && isFromAncient) gain.m_gradeFactor = NO_GRADE_FACTOR / Data::GetVambraceGradeUpFactor();
            return gain;
        }

        ExtraState Clamp(const ExtraState& state)
        {
            ExtraState clamped{ state };
            for (AccessoryState& accessory : clamped.m_accessories) accessory.m_mainStat = std::clamp(accessory.m_mainStat, 0.0, MAX_ACCESSORY_MAIN_STAT);
            for (BraceletStat& line : clamped.m_braceletStats) line.m_value = std::clamp(line.m_value, MIN_VALUE, MAX_BRACELET_STAT);
            for (StoneLine& line : clamped.m_stone) line.m_level = std::clamp(line.m_level, MIN_VALUE, Data::STONE_MAX_LEVEL);
            for (BookState& book : clamped.m_books) book.m_level = std::clamp(book.m_level, MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL);
            for (ArkCoreState& core : clamped.m_cores) core.m_points = std::clamp(core.m_points, MIN_VALUE, MAX_ARK_CORE_POINTS);
            for (int& level : clamped.m_arkGemLevels) level = std::clamp(level, MIN_VALUE, MAX_ARK_GEM_LEVEL_SUM);
            clamped.m_passive.m_rank = std::clamp(clamped.m_passive.m_rank, MIN_VALUE, MAX_PASSIVE_RANK);
            for (int& level : clamped.m_passive.m_levels) level = std::clamp(level, MIN_VALUE, MAX_PASSIVE_LEVEL);
            if (clamped.m_vambraceLevel.has_value()) clamped.m_vambraceLevel = std::clamp(*clamped.m_vambraceLevel, MIN_VALUE, MAX_VAMBRACE_LEVEL);
            return clamped;
        }
    }
}
