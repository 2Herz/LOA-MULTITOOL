module LostArkMultiTool.Models.CombatState;  // 전투력 상태 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기

namespace LostArkMultiTool::Models  // 모델 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        namespace Data = CombatCoefficientData;  // 계수 데이터 네임스페이스 별칭

        // ===== 세트 판별 (이름에 들어 있는 글자) =====
        constexpr std::string_view KEY_SEPARATOR{ "_" };  // 분류 키에서 접두사와 부위를 잇는 구분 글자
        constexpr std::string_view ESTHER_PREFIX{ "에스더" };  // 에스더 분류 키 접두사
        constexpr std::string_view ESTHER_GRADE_NAME{ "에스더" };  // API 등급 이름 중 에스더 글자
        // 에스더 무기: 이름이 겹치는 단계가 있어서("테라 에크혼") 높은 단계부터 확인
        constexpr std::array<std::string_view, 4> ESTHER_KEYWORDS{ "테라", "기가", "네오", "에크혼" };
        // 일반 세트 (이름 → 분류 키 앞부분이 같은 글자)
        constexpr std::array<std::string_view, 3> SET_KEYWORDS{ "결단", "업화", "전율" };  // 일반 세트 키워드 목록

        constexpr int MIN_ENHANCEMENT{ 0 };  // 일반 재련 최소 단계
        constexpr int MIN_ADVANCED_ENHANCEMENT{ 0 };  // 상급 재련 최소 단계

        // 표의 한 칸 (범위 밖 · 측정 안 함이면 nullopt)
        [[nodiscard]] std::optional<double> ReadCell(std::span<const double> values, std::ptrdiff_t index) noexcept
        {
            if (index < 0 || static_cast<std::size_t>(index) >= values.size()) return std::nullopt;  // 인덱스가 표 범위를 벗어나는지 검사
            const double value{ values[static_cast<std::size_t>(index)] };  // 표 값 읽기 (경계 검사를 거친 번호)
            if (value == Data::MISSING_STAT) return std::nullopt;  // 측정하지 않은 칸이면 값 없음
            return value;  // 유효한 값 반환
        }

        // (N, M)의 레벨별 표 번호 = 부위 아이템 레벨 - 표 첫 칸 레벨
        [[nodiscard]] std::ptrdiff_t GetLevelIndex(const Data::EquipmentTable& table, int enhancement, int advancedEnhancement) noexcept
        {
            const Data::ItemLevelRule& rule{ Data::GetItemLevelRule() };  // 아이템 레벨 규칙 조회
            const int itemLevel{ rule.m_baseItemLevel + enhancement * rule.m_normalStepLevel + advancedEnhancement * rule.m_advancedStepLevel };  // 현재 단계의 부위 아이템 레벨 계산 (기본 + 일반 단계 + 상급 단계)
            return static_cast<std::ptrdiff_t>(itemLevel - table.m_firstLevel);  // 표 첫 칸 레벨을 빼서 표 번호로 변환
        }

        // 상급 단계에 맞는 보너스 배율 (구간에 들지 않으면 1)
        [[nodiscard]] double GetAdvancedMultiplier(int advancedEnhancement) noexcept  // 상급 보너스 배율 초기값 (구간 밖이면 1)
        {
            double multiplier{ 1.0 };  // 상급 보너스 구간 표 순회
            for (const Data::AdvancedBonus& bonus : Data::GetAdvancedBonuses())  // 단계가 구간 시작 이상이면 배율 갱신
            {
                if (advancedEnhancement >= bonus.m_minStep) multiplier = bonus.m_multiplier;   // 오름차순 → 마지막으로 맞은 구간
            }
            return multiplier;  // 맞은 구간의 배율 반환
        }

        // 이 일반 단계에서 넣을 수 있는 상급 재련 최대 단계 (표 끝 · 최대 단계 중 작은 값)
        [[nodiscard]] int GetMaxAdvanced(const Data::EquipmentTable& table, int enhancement) noexcept  // 아이템 레벨 규칙 조회
        {
            const Data::ItemLevelRule& rule{ Data::GetItemLevelRule() };  // 레벨별 표의 마지막 번호
            const std::ptrdiff_t lastIndex{ static_cast<std::ptrdiff_t>(table.m_byLevel.size()) - 1 };  // 상급 0일 때의 표 번호에서 남은 칸 수
            const std::ptrdiff_t room{ lastIndex - GetLevelIndex(table, enhancement, MIN_ADVANCED_ENHANCEMENT) };  // 남은 칸이 없거나 상급 단계 레벨이 잘못되면 최소값 반환
            if (room <= 0 || rule.m_advancedStepLevel <= 0) return MIN_ADVANCED_ENHANCEMENT;  // 표 끝과 최대 상급 단계 중 작은 값 반환
            return std::min(rule.m_maxAdvancedEnhancement, static_cast<int>(room / rule.m_advancedStepLevel));
        }

        [[nodiscard]] std::string MakeKey(std::string_view prefix, std::string_view suffix)  // 표 키 만들기 (접두사 + 구분 글자 + 접미사)
        {
            return std::string{ prefix } + std::string{ KEY_SEPARATOR } + std::string{ suffix };  // 세 조각을 이어서 키 문자열 생성
        }
    }

    bool GearState::IsKnown() const noexcept  // 표 키가 있으면 계수 표가 있는 부위
    {
        return !m_tableKey.empty();  // 키가 비어 있지 않으면 true
    }

    namespace CombatGear  // 장비 규칙 네임스페이스 시작
    {
        std::optional<double> GetStat(const GearState& gear)  // (일반 N, 상급 M) 능력치 조회
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };  // 분류 키로 장비 표 찾기
            if (table == nullptr) return std::nullopt;  // 표가 없으면 값 없음

            // 상급 0 → 일반 표 그대로
            if (gear.m_advancedEnhancement <= MIN_ADVANCED_ENHANCEMENT) return ReadCell(table->m_normal, gear.m_enhancement);

            // 상급 > 0 → 레벨별 표 × 보너스, 소수점 버림 (lopec 표와 같은 규칙)
            const std::optional<double> value{ ReadCell(table->m_byLevel, GetLevelIndex(*table, gear.m_enhancement, gear.m_advancedEnhancement)) };  // 레벨별 표에서 해당 칸의 값을 읽음
            if (!value.has_value()) return std::nullopt;  // 칸이 비어 있으면 값 없음
            return std::floor(*value * GetAdvancedMultiplier(gear.m_advancedEnhancement));  // 상급 보너스를 곱하고 소수점 버림
        }

        int GetMaxEnhancement(std::string_view tableKey)  // 일반 재련 최대 단계 조회
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(tableKey) };  // 분류 키로 장비 표 찾기
            if (table == nullptr || table->m_normal.empty()) return MIN_ENHANCEMENT;  // 표가 없거나 비어 있으면 최소 단계 반환
            return static_cast<int>(table->m_normal.size()) - 1;  // 표 길이 - 1 이 최대 단계
        }

        bool CanAdvance(const GearState& gear)  // 상급 재련 가능 여부 조회
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };  // 분류 키로 장비 표 찾기
            if (table == nullptr || table->m_byLevel.empty() || table->m_minAdvancedEnhancement < MIN_ENHANCEMENT) return false;  // 표가 없거나 상급을 지원하지 않으면 불가
            return gear.m_enhancement >= table->m_minAdvancedEnhancement && GetMaxAdvanced(*table, gear.m_enhancement) > MIN_ADVANCED_ENHANCEMENT;  // 일반 단계가 상급 시작 단계 이상이고 남은 칸이 있으면 가능
        }

        GearState Clamp(const GearState& gear)  // 입력 범위로 맞추기
        {
            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };  // 분류 키로 장비 표 찾기
            if (table == nullptr) return gear;  // 표가 없으면 그대로 반환

            GearState clamped{ gear };  // 복사본을 만들어 값을 보정
            clamped.m_enhancement = std::clamp(gear.m_enhancement, MIN_ENHANCEMENT, GetMaxEnhancement(gear.m_tableKey));  // 일반 단계를 0 ~ 표 끝으로 제한
            clamped.m_advancedEnhancement = CanAdvance(clamped)  // 상급 가능하면 범위 안으로 제한
                ? std::clamp(gear.m_advancedEnhancement, MIN_ADVANCED_ENHANCEMENT, GetMaxAdvanced(*table, clamped.m_enhancement))  // 상급 허용 범위 내 값 사용
                : MIN_ADVANCED_ENHANCEMENT;  // 불가하면 최소값으로 되돌림
            return clamped;  // 보정한 상태 반환
        }

        // 기본 아이템 레벨 기준 올라간 양 (에스더 = 단계별 아이템 레벨 표 - 기본 레벨 → 두 상태의 차이 = 표[목표] - 표[현재])
        int GetItemLevelGain(const GearState& gear)  // 기본 아이템 레벨 대비 증가량 조회
        {
            const Data::ItemLevelRule& rule{ Data::GetItemLevelRule() };  // 아이템 레벨 규칙 조회
            const int advancedGain{ gear.m_advancedEnhancement * rule.m_advancedStepLevel };  // 상급 재련에서 오른 레벨

            const Data::EquipmentTable* table{ Data::FindEquipmentTable(gear.m_tableKey) };  // 분류 키로 장비 표 찾기
            if (table != nullptr && !table->m_itemLevels.empty())  // 단계별 아이템 레벨 표가 있으면 (에스더)
            {
                const std::size_t step{ static_cast<std::size_t>(std::clamp(gear.m_enhancement, MIN_ENHANCEMENT, static_cast<int>(table->m_itemLevels.size()) - 1)) };  // 표 범위 안으로 단계 제한
                return table->m_itemLevels[step] - rule.m_baseItemLevel + advancedGain;  // 표의 레벨에서 기본 레벨을 빼고 상급 증가량을 더함
            }
            return gear.m_enhancement * rule.m_normalStepLevel + advancedGain;  // 표가 없으면 일반 단계 × 단계당 레벨 + 상급 증가량
        }

        bool IsEsther(std::string_view tableKey) noexcept  // 에스더 무기 여부 조회
        {
            return tableKey.starts_with(ESTHER_PREFIX);  // 분류 키가 에스더 접두사로 시작하면 에스더
        }

        // 무기: 에스더 단계 이름 먼저 → 결단 · 업화 · 전율 + 부위 (표에 없는 조합은 빈 문자열)
        std::string DetectTableKey(GearPart part, std::string_view itemName, std::string_view grade)  // 장비 이름과 등급으로 분류 키 찾기
        {
            std::string key;  // 결과 키 (찾지 못하면 빈 문자열)
            if (part == GearPart::PART_WEAPON)  // 무기만 에스더 판별
            {
                for (const std::string_view keyword : ESTHER_KEYWORDS)  // 에스더 단계 키워드 순회 (높은 단계부터)
                {
                    if (itemName.contains(keyword))  // 이름에 키워드가 들어 있으면
                    {
                        key = MakeKey(ESTHER_PREFIX, keyword);  // 에스더 접두사 + 키워드로 키 생성
                        break;  // 첫 일치로 끝냄
                    }
                }
                if (key.empty() && grade == ESTHER_GRADE_NAME) return {};   // 모르는 에스더 무기
            }

            if (key.empty())  // 아직 키가 없으면 일반 세트 판별
            {
                for (const std::string_view keyword : SET_KEYWORDS)  // 세트 키워드 순회
                {
                    if (itemName.contains(keyword))  // 이름에 키워드가 들어 있으면
                    {
                        key = MakeKey(keyword, GetGearTableSuffix(part));  // 세트 이름 + 부위 이름으로 키 생성
                        break;  // 첫 일치로 끝냄
                    }
                }
            }

            return Data::FindEquipmentTable(key) != nullptr ? key : std::string{};  // 표에 있는 키만 반환, 없으면 빈 문자열
        }
    }
}
