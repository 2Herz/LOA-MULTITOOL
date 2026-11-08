export module LostArkMultiTool.Models.CombatState;  // 전투력 상태 모델 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모듈 가져오기

export namespace LostArkMultiTool::Models  // 모델 네임스페이스 내보내기 시작
{
    // 부위 하나의 재련 상태 (분류 키가 비어 있으면 계수 표가 없는 장비 → 계산 · 편집 대상에서 빠짐)
    struct GearState
    {
        std::string m_tableKey;              // "결단_머리" · "에스더_테라" ... (UTF-8)
        int m_enhancement{};                 // 일반 재련 단계
        int m_advancedEnhancement{};         // 상급 재련 단계

        [[nodiscard]] bool IsKnown() const noexcept;  // 분류 키가 있어 계수 표가 존재하는지 확인
        [[nodiscard]] bool operator==(const GearState& other) const = default;  // 모든 멤버가 같으면 같은 상태로 비교 (기본 비교 연산)
    };

    // 전투력 계산의 입력 한 벌 (현재 상태 · 목표 상태가 같은 모양)
    // m_totalMainStat · m_weaponAttack · m_itemLevel은 현재 상태에서만 의미 있음 (API에서 받은 기준값)
    struct CombatState
    {
        std::array<GearState, GEAR_PART_COUNT> m_gears;  // 부위별 재련 상태 (GearPart 순서)
        std::optional<int> m_weaponQuality;  // 무기 품질 (에스더 무기 · 모름 = nullopt)
        std::vector<int> m_gemLevels;        // 보석 레벨들 (0 = 보석 없음)
        std::vector<std::string> m_gemKinds; // 보석 종류들 ("겁화" · "작열" · "멸화" · "홍염", m_gemLevels와 같은 순서, 모르면 빈 글자)
        double m_totalMainStat{};            // S0: 모든 장비 주스탯 합
        double m_weaponAttack{};             // 무기 툴팁 공격력 (W0 = 이 값 + 보정)
        double m_itemLevel{};                // API 평균 아이템 레벨
        double m_gemBaseAttackRate{};        // 보석 기본 공격력 총합 (0.066 = 6.6%, 완갑 기본 공격력 배율의 A 추정에 사용)
        ExtraState m_extra;                  // 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 그리드 · 아크 패시브 · 완갑
    };

    // 계수 표를 이용한 장비 규칙 (능력치 조회 · 입력 범위 · 세트 판별)
    namespace CombatGear
    {
        // (일반 N, 상급 M) 능력치 (M = 0 → 일반 표, M > 0 → 레벨별 표 × 상급 보너스 후 버림, 모르면 nullopt)
        [[nodiscard]] std::optional<double> GetStat(const GearState& gear);

        // 일반 재련 최대 단계 (표가 없으면 0)
        [[nodiscard]] int GetMaxEnhancement(std::string_view tableKey);

        // 이 일반 단계에서 상급 재련을 넣을 수 있는지 (결단 · 업화이고 표가 지원하는 단계 이상)
        [[nodiscard]] bool CanAdvance(const GearState& gear);

        // 입력 범위 안으로 맞춤 (일반 0 ~ 표 끝, 상급은 넣을 수 있을 때만 0 ~ 40 · 표 끝)
        [[nodiscard]] GearState Clamp(const GearState& gear);

        // 기본 아이템 레벨에서 올라간 양 (일반 × 5 + 상급 × 1, 에스더는 단계별 아이템 레벨 표 사용)
        [[nodiscard]] int GetItemLevelGain(const GearState& gear);

        // 에스더 무기 (품질 없음 · 상급 재련 없음)
        [[nodiscard]] bool IsEsther(std::string_view tableKey) noexcept;

        // 장비 이름 · 등급 → 분류 키 (계수 표에 없으면 빈 문자열)
        [[nodiscard]] std::string DetectTableKey(GearPart part, std::string_view itemName, std::string_view grade);
    }
}
