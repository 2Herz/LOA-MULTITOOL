export module LostArkMultiTool.Models.RefineData;

import std;

// 재련 시뮬레이터의 게임 데이터 (재련 확률 · 재료 소모량 · 상급 재련 기대 시도 수 · 아이템 레벨 공식 · 계승 규칙)
// 숫자 표는 Refine_Data.cpp에 있고, 여기에는 표를 읽는 형식과 함수만 둠
// 출처: 로아업 통합 재련 계산기가 쓰는 표 (게임 안 재련 정보와 같은 값)
export namespace LostArkMultiTool::Models::RefineData
{
    // 장비 단계 (에기르 = 운명의 업화 · 결단, 세르카 = 운명의 전율)
    enum class EquipmentTier
    {
        TIER_AEGIR,
        TIER_SERCA
    };

    // 장비 종류 (재료 · 확률표가 다름)
    enum class EquipmentKind
    {
        KIND_ARMOR,    // 방어구 (투구 · 어깨 · 상의 · 하의 · 장갑)
        KIND_WEAPON    // 무기
    };

    // 재련 재료 (재료 가격 표 순서 = 로아업 재료 목록 순서)
    enum class RefineMaterial : std::size_t
    {
        MATERIAL_GOLD,
        MATERIAL_ABIDOS_FUSION,
        MATERIAL_SUPERIOR_ABIDOS_FUSION,
        MATERIAL_DESTINY_SHARD,
        MATERIAL_DESTINY_LEAPSTONE,
        MATERIAL_GREAT_DESTINY_LEAPSTONE,
        MATERIAL_DESTINY_GUARDIAN_STONE,
        MATERIAL_DESTINY_DESTRUCTION_STONE,
        MATERIAL_GUARDIAN_CRYSTAL,
        MATERIAL_DESTRUCTION_CRYSTAL,
        MATERIAL_GLACIER_BREATH,
        MATERIAL_LAVA_BREATH,
        MATERIAL_TAILORING_HELLFIRE_11_14,
        MATERIAL_TAILORING_HELLFIRE_15_18,
        MATERIAL_TAILORING_HELLFIRE_19_20,
        MATERIAL_METALLURGY_HELLFIRE_11_14,
        MATERIAL_METALLURGY_HELLFIRE_15_18,
        MATERIAL_METALLURGY_HELLFIRE_19_20,
        MATERIAL_TAILORING_TREMOR_12_15,
        MATERIAL_TAILORING_TREMOR_16_19,
        MATERIAL_METALLURGY_TREMOR_12_15,
        MATERIAL_METALLURGY_TREMOR_16_19,
        MATERIAL_MASTER_TAILORING_1,
        MATERIAL_MASTER_TAILORING_2,
        MATERIAL_MASTER_TAILORING_3,
        MATERIAL_MASTER_TAILORING_4,
        MATERIAL_MASTER_METALLURGY_1,
        MATERIAL_MASTER_METALLURGY_2,
        MATERIAL_MASTER_METALLURGY_3,
        MATERIAL_MASTER_METALLURGY_4,
        MATERIAL_COUNT
    };

    inline constexpr std::size_t MATERIAL_COUNT{ static_cast<std::size_t>(RefineMaterial::MATERIAL_COUNT) };

    // 재료 → 배열 번호
    [[nodiscard]] constexpr std::size_t ToIndex(RefineMaterial material) noexcept
    {
        return static_cast<std::size_t>(material);
    }

    // 재료 하나의 이름 · 거래소 조회 방법
    struct MaterialInfo
    {
        std::string_view m_name;        // 화면 · 저장 이름 (UTF-8)
        std::string_view m_marketName;  // 거래소 검색 이름 (빈 문자열 = 조회하지 않음, 골드)
        bool m_isShard{};               // 파편 주머니 시세 (소 1000 · 중 2000 · 대 3000개 중 파편 1개가 가장 싼 값)
    };

    // ===== 강화 단계 · 상급 재련 범위 =====
    inline constexpr int MAX_ENHANCEMENT{ 25 };              // 일반 재련 최대 단계
    inline constexpr int MIN_AEGIR_ENHANCEMENT{ 10 };        // 에기르 재련표 시작 (+10 → +11부터 있음)
    inline constexpr int MIN_SERCA_ENHANCEMENT{ 11 };        // 세르카 재련표 시작 (+11 → +12부터 있음, 계승하면 +11 이상)
    inline constexpr int MAX_ADVANCED_ENHANCEMENT{ 40 };     // 상급 재련 최대 단계 (에기르만)
    inline constexpr int ADVANCED_BLOCK_SIZE{ 10 };          // 상급 재련은 10단계씩 같은 표 (1~10, 11~20, 21~30, 31~40)
    inline constexpr int ADVANCED_BLOCK_COUNT{ MAX_ADVANCED_ENHANCEMENT / ADVANCED_BLOCK_SIZE };
    inline constexpr int INHERIT_ENHANCEMENT{ 20 };          // 계승할 수 있는 에기르 강화 단계 (+20 이상)
    inline constexpr int INHERIT_ADVANCED_ENHANCEMENT{ 40 }; // 계승할 수 있는 에기르 상급 재련 단계

    // 장비 한 부위의 상태
    struct EquipmentState
    {
        EquipmentTier m_tier{ EquipmentTier::TIER_AEGIR };
        int m_enhancement{};          // 일반 재련 단계 (+n)
        int m_advancedEnhancement{};  // 상급 재련 단계 (세르카는 항상 0)

        [[nodiscard]] bool operator==(const EquipmentState& other) const noexcept = default;
    };

    // 재료와 수량
    struct MaterialAmount
    {
        RefineMaterial m_material{ RefineMaterial::MATERIAL_GOLD };
        double m_amount{};
    };

    // 성공 확률을 올려 주는 보조 재료 (숨결 = 최대 m_maxCount개까지 1개당 확률, 책 = 1개)
    struct SupportMaterial
    {
        RefineMaterial m_material{ RefineMaterial::MATERIAL_GOLD };
        int m_maxCount{};
        double m_probability{};   // 1개당 추가 확률
    };

    // 일반 재련 한 단계 (m_targetLevel - 1 → m_targetLevel)
    struct NormalStep
    {
        int m_targetLevel{};
        double m_baseProbability{};                // 기본 성공 확률
        std::vector<MaterialAmount> m_materials;   // 1회 시도에 드는 재료 (골드 포함)
        std::optional<SupportMaterial> m_breath;   // 숨결
        std::optional<SupportMaterial> m_book;     // 강화책 (없는 단계는 nullopt)
    };

    // 상급 재련 기대 시도 수 표 한 줄 (K = 숨결 사용 + 2 × 책 사용, 턴 종류별)
    struct AdvancedTryRow
    {
        int m_normalK{};
        int m_bonusK{};
        int m_enhancedBonusK{};
        double m_freeNormalTry{};       // 무료 일반 턴 (재료 없이 보조 재료만)
        double m_paidNormalTry{};       // 유료 일반 턴
        double m_bonusTry{};            // 선조 턴
        double m_enhancedBonusTry{};    // 강화 선조 턴
    };

    // 상급 재련 10단계 묶음 하나
    struct AdvancedBlock
    {
        std::vector<MaterialAmount> m_materials;    // 1회 시도에 드는 재료 (골드 포함)
        MaterialAmount m_breath;                    // 한 턴에 넣는 숨결 (넣으면 이 수량 전부)
        RefineMaterial m_book{};                    // 장인의 야금술 · 재봉술
        bool m_hasEnhancedBonus{};                  // 강화 선조 턴이 있는 묶음 (21~40)
        std::span<const AdvancedTryRow> m_tryRows;  // 기대 시도 수 표 (정적 데이터)
    };

    // 재료 정보 (범위를 벗어나면 std::out_of_range)
    [[nodiscard]] const MaterialInfo& GetMaterialInfo(RefineMaterial material);

    // 일반 재련 fromLevel → fromLevel + 1 (표에 없는 단계면 nullopt)
    [[nodiscard]] std::optional<NormalStep> FindNormalStep(EquipmentTier tier, EquipmentKind kind, int fromLevel);

    // 상급 재련 묶음 (blockIndex 0 = 1~10단계 ... 3 = 31~40단계, 범위를 벗어나면 nullopt)
    [[nodiscard]] std::optional<AdvancedBlock> FindAdvancedBlock(EquipmentKind kind, int blockIndex);

    // 부위 하나의 아이템 레벨 (에기르: 1730 + (강화 − 20) × 5 + (상급 − 40), 세르카: 1725 + (강화 − 10) × 5)
    [[nodiscard]] int CalculateItemLevel(const EquipmentState& state) noexcept;

    // 계승 후 세르카 시작 단계 (에기르 +20 → +11, +21 → +12, +22 → +13, +23 → +14, +24 → +16, +25 → +18)
    [[nodiscard]] int GetInheritStartLevel(int aegirEnhancement) noexcept;

    // 계승 준비가 끝난 부위인지 (세르카이거나 에기르 +20 · 상급 40 이상)
    [[nodiscard]] bool IsInheritPrepared(const EquipmentState& state) noexcept;

    // 상태가 표 범위 안인지 (에기르 +10~+25 · 상급 0~40, 세르카 +11~+25)
    [[nodiscard]] bool IsValidState(const EquipmentState& state) noexcept;
}
