export module LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기

// 전투력 시뮬레이터 계수 (lopec 측정 데이터를 분석한 data/CombatCoefficients.json)
// 이 파일은 손으로 쓴 선언만 있고, 값은 tools/Export-CombatData.py가 만든 Combat_Coefficient_Data.cpp에만 있음
// → JSON을 고치면 도구를 다시 실행해서 .cpp를 새로 만들 것 (실행할 때 파일을 읽지 않음, 값은 실행 파일에 들어감)
export namespace LostArkMultiTool::Models::CombatCoefficientData  // 계수 데이터 네임스페이스 내보내기 시작
{
    // 측정하지 않은 칸 (JSON null) → 이 값이면 그 단계 능력치를 모름
    inline constexpr double MISSING_STAT{ -1.0 };

    // 장비 표가 올려 주는 능력치 종류
    enum class StatKind
    {
        STAT_MAIN,            // 주스탯 (힘 · 민첩 · 지능) → 방어구
        STAT_WEAPON_ATTACK    // 무기 공격력 → 무기
    };

    // 장비 표 하나 (분류 키 = "결단_머리"처럼 세트_부위)
    struct EquipmentTable
    {
        std::string_view m_key;                 // 분류 키 (UTF-8)
        StatKind m_stat{};  // 능력치 종류 (주스탯 또는 무기 공격력)
        std::span<const double> m_normal;       // 일반 재련 N단계 → 능력치 (상급 재련 0일 때)
        int m_firstLevel{};                     // m_byLevel[0]의 아이템 레벨
        std::span<const double> m_byLevel;      // 아이템 레벨별 능력치 (비어 있으면 상급 재련이 없는 분류)
        int m_minAdvancedEnhancement{};         // 상급 재련을 넣을 수 있는 최소 일반 단계 (없으면 음수)
        std::span<const int> m_itemLevels;      // 일반 단계별 아이템 레벨 (에스더처럼 불규칙한 분류만, 비어 있으면 기본 + 단계 × 5)
    };

    // 상급 재련 보너스 구간 (상급 단계가 m_minStep 이상이면 능력치 × m_multiplier)
    struct AdvancedBonus
    {
        int m_minStep{};  // 상급 재련 구간 시작 단계
        double m_multiplier{};  // 구간 배율
    };

    // 부위 아이템 레벨 = m_baseItemLevel + 일반 단계 × m_normalStepLevel + 상급 단계 × m_advancedStepLevel
    struct ItemLevelRule
    {
        int m_baseItemLevel{};  // 일반 재련 한 단계당 오르는 아이템 레벨
        int m_normalStepLevel{};  // 상급 재련 한 단계당 오르는 아이템 레벨
        int m_advancedStepLevel{};
        int m_maxAdvancedEnhancement{};         // 상급 재련 최대 단계
    };

    // 무기 품질 곡선 F(q) = m_base + m_scale × (q / m_maxQuality)²  (F(최대) = 1)
    struct QualityCurve
    {
        double m_base{};  // 곡선 기본값 A
        double m_scale{};  // 곡선 배율 B
        double m_maxQuality{};  // 곡선 최대 품질
    };

    // ===== 장비 =====
    [[nodiscard]] std::span<const EquipmentTable> GetEquipmentTables() noexcept;  // 장비 능력치 표 전체 조회
    [[nodiscard]] const EquipmentTable* FindEquipmentTable(std::string_view key) noexcept;   // 없으면 nullptr
    [[nodiscard]] std::span<const AdvancedBonus> GetAdvancedBonuses() noexcept;            // 기준 단계 오름차순
    [[nodiscard]] const ItemLevelRule& GetItemLevelRule() noexcept;

    // ===== 보석 =====
    [[nodiscard]] std::span<const double> GetGemFactors() noexcept;   // [0] = 첫 레벨(6) 계수 ... (10레벨 = 1)
    [[nodiscard]] int GetGemFirstLevel() noexcept;                    // GetGemFactors()[0]의 보석 레벨
    [[nodiscard]] double GetNoGemValue() noexcept;                    // 보석이 없을 때의 계수 (GetGemFactors와 같은 척도)
    [[nodiscard]] double GetGemTypeFactor(std::string_view kind) noexcept;   // 보석 종류(겁화 · 작열 · 멸화 · 홍염) 계수, 모르는 종류는 1

    // ===== 무기 품질 · 공격력 =====
    [[nodiscard]] const QualityCurve& GetWeaponQualityCurve() noexcept;
    [[nodiscard]] double GetWeaponAttackExtra() noexcept;             // W0 = 무기 툴팁 공격력 + 이 값

    // ===== 문구 키 표 (악세서리 · 팔찌 · 스톤 · 아바타 등 공통) =====
    struct KeyedValue
    {
        std::string_view m_key;                 // 문구 · 등급 이름 · 조합 키 (UTF-8)
        double m_value{};                       // ln 배율 또는 전투력 배율 (표마다 설명 참조)
    };

    // 표에서 키로 값 찾기 (없으면 nullptr)
    [[nodiscard]] const KeyedValue* FindKeyed(std::span<const KeyedValue> table, std::string_view key) noexcept;

    // ===== 악세서리 연마 옵션 =====
    enum class AccessoryKind
    {
        KIND_NECKLACE,  // 악세서리 부위 종류: 목걸이
        KIND_EARRING,  // 악세서리 부위 종류: 귀걸이
        KIND_RING  // 악세서리 부위 종류: 반지
    };

    // 악세서리 옵션 식의 상수 (추가 피해 · 공격력 · 치명 · 무기 공격력)
    struct AccessoryFormula
    {
        double m_addDamageBase{};               // 추가 피해: ln((기준 + x) / 기준)
        double m_attackFlatReference{};         // 공격력 +N: ln(1 + N / 기준)
        double m_critRateBase{};                // 치명타 적중률 기준 (cr0)
        double m_critDamageBase{};              // 치명타 피해 기준 배수 (m0)
        double m_weaponRatioEsther{};           // 무기 공격력 +x%: 0.5 × ln(1 + x × 비율) — 에스더 무기 캐릭터
        double m_weaponRatioReforge{};          // 같은 식 — 일반 재련 무기 캐릭터
    };

    [[nodiscard]] std::span<const KeyedValue> GetAccessoryOptions() noexcept;            // 정확한 문구 → ln (무기 공격력 옵션은 식 사용)
    [[nodiscard]] const AccessoryFormula& GetAccessoryFormula() noexcept;
    [[nodiscard]] std::span<const std::string_view> GetAccessoryCandidates(AccessoryKind kind) noexcept;   // 종류별 선택지 ("없음" 먼저)

    // ===== 팔찌 =====
    struct BraceletWeaponLine
    {
        std::string_view m_key;                 // 공백 없는 문구
        double m_weaponFlat{};                  // 무기 공격력 증가량
        double m_logAdd{};                      // 무기 공격력과 별개로 더하는 ln
    };

    [[nodiscard]] double GetBraceletStatRatio() noexcept;                                // 스탯 1포인트당 배율 증가
    [[nodiscard]] std::span<const std::string_view> GetBraceletStatTypes() noexcept;     // 스탯 종류 선택지 ("없음" 먼저)
    [[nodiscard]] std::span<const std::string_view> GetBraceletEffectiveTypes() noexcept;   // 전투력에 반영되는 스탯 (치명 · 특화 · 신속)
    [[nodiscard]] std::span<const std::string_view> GetBraceletMainStatTypes() noexcept;    // 주스탯 (힘 · 민첩 · 지능)
    [[nodiscard]] std::span<const KeyedValue> GetBraceletOptions() noexcept;             // 공백 없는 문구 → ln
    [[nodiscard]] std::span<const BraceletWeaponLine> GetBraceletWeaponLines() noexcept;  // 팔찌 무기 공격력 포함 옵션 줄 목록 조회
    [[nodiscard]] std::span<const std::string_view> GetBraceletOptionCandidates() noexcept;  // 팔찌 옵션 후보 목록 조회

    // ===== 스톤 · 각인서 =====
    struct EngravingBook
    {
        std::string_view m_name;  // 각인서 이름
        double m_base{};                        // 각인서 레벨 0의 ln
        double m_top{};                         // 각인서 레벨 4의 ln (효과 없는 각인은 둘 다 0)
    };

    struct LevelRange
    {
        double m_atMin{};                       // 각인서 레벨 0
        double m_atMax{};                       // 각인서 레벨 4
    };

    inline constexpr int ENGRAVING_BOOK_MAX_LEVEL{ 4 };                                  // 각인서 레벨 0 ~ 4
    inline constexpr int STONE_MAX_LEVEL{ 4 };                                           // 스톤 각인 레벨 0 ~ 4

    [[nodiscard]] std::span<const KeyedValue> GetStoneCells() noexcept;                  // "D12" = 다른 각인 · 1번 줄 1 · 2번 줄 2
    [[nodiscard]] std::span<const KeyedValue> GetStoneNameLn() noexcept;                 // 스톤 각인 이름 → 보정 ln (레벨 1 이상인 줄마다 더함, 없는 이름은 0)
    [[nodiscard]] std::span<const KeyedValue> GetStoneUnstable() noexcept;               // 1번 줄 없음: 2번 줄 레벨("1" ~ "4") → 평균 ln
    [[nodiscard]] std::span<const std::string_view> GetStoneCandidates() noexcept;       // 스톤 각인 이름 선택지 ("없음" 먼저)
    [[nodiscard]] std::span<const EngravingBook> GetEngravingBooks() noexcept;  // 각인서 효과 표 조회
    [[nodiscard]] std::span<const std::string_view> GetBookCandidates() noexcept;        // 각인서 이름 선택지 ("없음" 먼저)
    [[nodiscard]] const LevelRange& GetLegendPenalty() noexcept;                         // 전설 각인서 감점 ln

    // ===== 아바타 =====
    [[nodiscard]] std::span<const KeyedValue> GetAvatarLn() noexcept;                    // 등급 이름("전설" · "영웅" · "없음") → 한 부위 ln

    // ===== 아크 그리드 =====
    struct ArkCorePoint
    {
        int m_core{};                           // 코어 번호 0 ~ 5
        std::string_view m_name;                // 코어 이름 (비어 있으면 이름과 무관한 질서 코어)
        int m_points{};                         // 의지력 포인트 합 (정확히 이 값일 때만 효과가 켜짐: 10 / 14 / 17 / 18 / 19 / 20)
        double m_ln{};                          // 영웅 · 전설 · 유물 코어의 ln (평탄 구간 대비)
        double m_ancientLn{};                   // 고대 코어의 ln
    };

    [[nodiscard]] std::span<const double> GetArkCoreAncientGains() noexcept;             // 코어 번호별 고대 등급 추가 ln
    [[nodiscard]] std::span<const ArkCorePoint> GetArkCorePoints() noexcept;  // 아크 코어 포인트 표 조회
    [[nodiscard]] std::span<const KeyedValue> GetArkGemPerLevel() noexcept;              // "공격력" · "추가 피해" · "보스 피해" → 레벨 1당 ln

    // ===== 아크 패시브 =====
    struct PassiveSlopes
    {
        double m_level1{};                      // 깨달음 레벨 1 내릴 때마다 줄어드는 비율
        double m_level2{};                      // 도약
    };

    [[nodiscard]] std::span<const KeyedValue> GetEnlightenmentRatios() noexcept;         // 깨달음 포인트 구성 → 배율
    [[nodiscard]] std::span<const double> GetPassiveRankRatios() noexcept;               // [랭크 0 ~ 6] → 배율
    [[nodiscard]] const PassiveSlopes& GetPassiveSlopes() noexcept;  // 패시브 기울기 조회

    // ===== 완갑 =====
    // 재련 단계 하나의 기본 효과 (전율 완갑 4등급 표, 등급마다 단계별로 다름)
    struct VambraceStep
    {
        double m_weaponAttack{};                // 무기 공격력 → 장비 재련 제곱근의 W
        double m_mainStat{};                    // 힘 · 민첩 · 지능 → 장비 재련 제곱근의 S
        double m_baseAttack{};                  // 기본 공격력 (고정값) → 기본 공격력 A에 더함
    };

    // 완갑 등급 하나 (영웅 · 전설 · 유물 · 고대): 첫 단계부터 이어지는 단계 표
    struct VambraceGrade
    {
        std::string_view m_name;  // 완갑 등급 이름
        int m_firstStep{};                      // m_steps[0]의 재련 단계
        std::span<const VambraceStep> m_steps;
    };

    [[nodiscard]] std::span<const VambraceGrade> GetVambraceGrades() noexcept;  // 완갑 등급 표 조회
    [[nodiscard]] const VambraceStep* FindVambraceStep(int step) noexcept;               // 재련 단계(0 ~ 25) → 능력치, 범위 밖이면 nullptr

    // 완갑 한계 해방 등급 하나의 추가 효과 (기본 공격력 %)
    struct VambraceGradeBonus
    {
        std::string_view m_name;  // 완갑 등급 이름
        int m_firstStep{};        // 이 등급이 확정되는 첫 재련 단계 (해방 경계 단계는 해방 전 등급)
        double m_rate{};          // 추가 효과 기본 공격력 비율 (0.02 = 2%)
    };

    [[nodiscard]] std::span<const VambraceGradeBonus> GetVambraceGradeBonuses() noexcept;   // 완갑 등급별 한계 해방 추가 효과 표 (낮은 등급부터)
    [[nodiscard]] double GetBaseAttackCalibration() noexcept;                            // 기본 공격력 A = sqrt(S0 × W0 / 6) × (1 + 보석 %) × 이 값 (한 측정점)

    // ===== S0 · W0 보정 (로아랩 실측 28행 기준, 핢머니 2026-11-08) =====
    [[nodiscard]] double GetMainStatCalibration() noexcept;                              // 파싱한 주스탯 합 S0에 곱함
    [[nodiscard]] double GetWeaponBase(double weaponAttack) noexcept;                    // 보정한 W0 ((툴팁 공격력 + 보정) × W0 보정, 모르면 0)
}
