export module LostArkMultiTool.Models.RefinePlan;

import std;
import LostArkMultiTool.Models.RefineData;

// 재련 시뮬레이터의 계산 입력 · 결과 형식 (통합 재련: 일반 재련 + 상급 재련 + 계승)
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Models
{
    // 장비 부위 (표 순서 = 로아업 통합 재련 순서)
    enum class RefineSlot : std::size_t
    {
        SLOT_WEAPON,
        SLOT_HELMET,
        SLOT_SHOULDER,
        SLOT_CHEST,
        SLOT_PANTS,
        SLOT_GLOVES,
        SLOT_COUNT
    };

    inline constexpr std::size_t REFINE_SLOT_COUNT{ static_cast<std::size_t>(RefineSlot::SLOT_COUNT) };

    // 부위 → 장비 종류 (무기만 야금술 · 파괴석 계열)
    [[nodiscard]] constexpr RefineData::EquipmentKind GetSlotKind(RefineSlot slot) noexcept
    {
        return slot == RefineSlot::SLOT_WEAPON ? RefineData::EquipmentKind::KIND_WEAPON : RefineData::EquipmentKind::KIND_ARMOR;
    }

    using RefineStates = std::array<RefineData::EquipmentState, REFINE_SLOT_COUNT>;
    using MaterialAmounts = std::array<double, RefineData::MATERIAL_COUNT>;   // 재료별 수량 (RefineMaterial 순서)

    // 재료 가격 · 귀속 설정 (귀속 재료는 비용 0으로 계산)
    struct RefinePrices
    {
        MaterialAmounts m_prices{};                                 // 1개당 골드
        std::array<bool, RefineData::MATERIAL_COUNT> m_isBound{};   // 귀속 (거래하지 않음)
    };

    // 목표 기준
    enum class RefineTargetMode
    {
        TARGET_ITEM_LEVEL,   // 평균 아이템 레벨 (비용 최소 경로를 찾음)
        TARGET_STAGE         // 부위별 단계 (부위마다 목표 상태를 직접 정함)
    };

    // 계산 요청
    struct RefineRequest
    {
        RefineStates m_starts{};
        RefineStates m_targets{};             // 부위별 단계 기준일 때만 사용
        RefineTargetMode m_targetMode{ RefineTargetMode::TARGET_ITEM_LEVEL };
        int m_targetItemLevel{};              // 아이템 레벨 기준일 때만 사용
    };

    // 한 번(일반 1단계) · 한 묶음(상급 10단계)의 기대 비용 · 재료
    struct RefineStepCost
    {
        double m_expectedCost{};      // 기대 골드 (귀속 재료 제외)
        double m_expectedTries{};     // 기대 시도 횟수
        MaterialAmounts m_materials{};   // 기대 소모 재료 (귀속 재료 포함)
    };

    // 일반 재련 · 상급 재련 비용표 (가격이 바뀔 때만 다시 계산 → 경로 계산은 이 표만 봄)
    struct RefineCostCatalog
    {
        static constexpr std::size_t TIER_COUNT{ 2 };
        static constexpr std::size_t KIND_COUNT{ 2 };
        static constexpr std::size_t LEVEL_COUNT{ RefineData::MAX_ENHANCEMENT + 1 };

        // [단계][종류][시작 강화] (에기르 · 세르카 / 방어구 · 무기 / +n → +n+1, 표에 없으면 nullopt)
        std::array<std::array<std::array<std::optional<RefineStepCost>, LEVEL_COUNT>, KIND_COUNT>, TIER_COUNT> m_regular{};
        // [종류][묶음] (상급 재련 10단계 묶음 전체의 기대 비용)
        std::array<std::array<RefineStepCost, RefineData::ADVANCED_BLOCK_COUNT>, KIND_COUNT> m_advanced{};
    };

    // 재련 동작
    enum class RefineAction
    {
        ACTION_REGULAR,    // 일반 재련 1단계
        ACTION_ADVANCED,   // 상급 재련 1단계 (묶음 비용의 1/10)
        ACTION_INHERIT     // 에기르 → 세르카 계승 (비용 0)
    };

    // 경로의 한 걸음
    struct RefineStep
    {
        RefineSlot m_slot{};
        RefineAction m_action{};
        RefineData::EquipmentState m_from{};
        RefineData::EquipmentState m_to{};
        double m_expectedCost{};
    };

    // 계산 실패 이유
    enum class RefineFailure
    {
        FAILURE_NONE,
        FAILURE_INVALID_STATE,    // 표 범위를 벗어난 시작 · 목표 상태
        FAILURE_REGRESSION,       // 목표가 시작보다 낮음 (세르카 → 에기르 등)
        FAILURE_NOT_PREPARED,     // 세르카 재련 · 계승은 모든 부위가 계승 준비(에기르 +20 · 상급 40)를 마쳐야 함
        FAILURE_UNREACHABLE       // 목표 아이템 레벨에 닿을 수 없음
    };

    // 부위 하나의 결과
    struct RefineSlotPlan
    {
        RefineData::EquipmentState m_terminal{};   // 목표 상태
        double m_expectedCost{};
        int m_stepCount{};                         // 걸음 수 (일반 1단계 · 상급 1단계 · 계승 각 1)
    };

    // 계산 결과
    struct RefineRouteResult
    {
        RefineFailure m_failure{ RefineFailure::FAILURE_NONE };
        std::array<RefineSlotPlan, REFINE_SLOT_COUNT> m_slots{};
        std::vector<RefineStep> m_steps;      // 진행 순서 (계승 우선 → 아이템 레벨 1당 비용이 싼 순)
        double m_totalCost{};
        double m_startItemLevel{};            // 평균 아이템 레벨
        double m_achievedItemLevel{};
        MaterialAmounts m_materials{};        // 기대 소모 재료 합계
    };
}
