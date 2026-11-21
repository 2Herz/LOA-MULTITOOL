export module LostArkMultiTool.Models.CombatSupporterData;  // 서포터 전투력 계수 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 딜러 계수 데이터 모듈 가져오기 (같은 모양의 구조체 재사용)

// 서포터 전투력 계수 (lopec 시뮬레이터 서포터 9명 측정, 2026-11-21, Verify/live/서포터_계수_분석.md)
// 딜러 계수(Combat_Coefficient_Data.cpp)는 도구가 만드는 파일이라 서포터 값은 이 모듈에 손으로 따로 둠
export namespace LostArkMultiTool::Models::CombatSupporterData  // 서포터 계수 네임스페이스 내보내기 시작
{
    // 서포터 각인서 한 권 (유물 레벨 0 · 4 ln, 전설 레벨 4 ln, 스톤 효과는 뺀 값)
    struct SupporterBook
    {
        std::string_view m_name;  // 각인서 이름
        double m_base{};          // 유물 레벨 0의 ln
        double m_top{};           // 유물 레벨 4의 ln
        double m_legendTop{};     // 전설 레벨 4의 ln (서포터는 딜러의 전설 규칙과 다름)
    };

    // 서포터 아크 패시브 트리 레벨 기울기 (1 - 기울기 × (최대 - 레벨))
    struct SupporterPassiveSlopes
    {
        double m_evolution{};      // 진화 트리 레벨 기울기 (서포터는 효과 있음)
        double m_enlightenment{};  // 깨달음 트리 레벨 기울기
        double m_leap{};           // 도약 트리 레벨 기울기 (서포터는 효과 없음 = 0)
    };

    // ===== 판별 =====
    [[nodiscard]] std::span<const std::string_view> GetSupporterTitles() noexcept;   // 서포터 아크 패시브 칭호 (축복의 오라 · 만개 · 절실한 구원 · 해방자)
    [[nodiscard]] bool IsSupporterTitle(std::string_view title) noexcept;            // 칭호가 서포터 칭호인지 검사

    // ===== 장비 재련 (딜러와 같은 √(S × W) 식, 기준값 보정만 다름) =====
    [[nodiscard]] double GetMainStatExtra() noexcept;                                // S0에 더하는 서포터 보정 주스탯
    [[nodiscard]] double GetWeaponBase(double weaponAttack) noexcept;                // 서포터 W0 (툴팁 무기 공격력 + 보정, 모르면 0)

    // ===== 방어구 품질 (부위마다 F(q) = 1 - a + a × (q / 100)², 무기 품질은 영향 없음) =====
    [[nodiscard]] double GetArmorQualityAmplitude() noexcept;                        // a = 품질 0일 때 부위 하나의 감소 비율
    [[nodiscard]] double GetMaxQuality() noexcept;                                   // 최대 품질 (100)

    // ===== 완갑 재련 (재련 단계 → +25 대비 ln, 등급 변화 포함) =====
    [[nodiscard]] std::span<const double> GetVambraceLevelLn() noexcept;             // [0 ~ 25] 단계 → ln

    // ===== 보석 (레벨 계수 F(level)/F(10), 종류는 영향 없음) =====
    [[nodiscard]] std::span<const double> GetGemFactors() noexcept;                  // [0] = 1레벨 ... [9] = 10레벨 (= 1)
    [[nodiscard]] int GetGemFirstLevel() noexcept;                                   // GetGemFactors()[0]의 보석 레벨
    [[nodiscard]] double GetNoGemValue() noexcept;                                   // 보석이 없을 때의 계수

    // ===== 악세서리 · 팔찌 =====
    [[nodiscard]] std::span<const CombatCoefficientData::KeyedValue> GetAccessoryOptions() noexcept;   // 정확한 문구 → ln (없는 문구는 0)
    [[nodiscard]] std::span<const CombatCoefficientData::KeyedValue> GetBraceletOptions() noexcept;    // 공백 없는 문구 → ln (없는 문구는 0)
    [[nodiscard]] double GetBraceletStatRatio() noexcept;                                               // 유효 스탯 1포인트당 배율 증가
    [[nodiscard]] std::span<const std::string_view> GetBraceletEffectiveTypes() noexcept;               // 전투력에 반영되는 스탯 (특화 · 신속)

    // ===== 각인서 · 스톤 · 아바타 =====
    [[nodiscard]] std::span<const SupporterBook> GetBooks() noexcept;                                   // 효과 있는 각인서 표 (없는 이름은 0)
    [[nodiscard]] std::span<const CombatCoefficientData::KeyedValue> GetStoneLevelOne() noexcept;       // 스톤 각인 이름 → 레벨 1 효과 (각인서 배율에 더함)
    [[nodiscard]] const CombatCoefficientData::StoneSumBonus& GetStoneSumBonus() noexcept;              // 스톤 두 줄 레벨 합 보너스
    [[nodiscard]] double GetAvatarMainStatScale() noexcept;                                             // 아바타 배율 = sqrt((1 + 이 값 × 목표 % 합) / (1 + 이 값 × 현재 % 합))

    // ===== 아크 그리드 · 아크 패시브 =====
    [[nodiscard]] std::span<const CombatCoefficientData::ArkCorePoint> GetArkCorePoints() noexcept;     // 코어 번호 · 포인트 → ln (이름 무관, 없는 칸은 0)
    [[nodiscard]] std::span<const CombatCoefficientData::KeyedValue> GetArkGemPerLevel() noexcept;      // 젬 효과 이름 → 레벨 1당 ln
    [[nodiscard]] std::span<const double> GetPassiveRankRatios() noexcept;                              // [랭크 0 ~ 6] → 배율
    [[nodiscard]] std::span<const CombatCoefficientData::KeyedValue> GetEnlightenmentRatios() noexcept; // 깨달음 포인트 구성 → 배율
    [[nodiscard]] const SupporterPassiveSlopes& GetPassiveSlopes() noexcept;                            // 트리 레벨 기울기
    [[nodiscard]] const CombatCoefficientData::PassiveInteraction& GetPassiveInteraction() noexcept;    // 랭크 · 깨달음 상호작용 (조합 측정)

    // ===== 전투력 수준별 아크 패시브 (전투력을 모르면 0을 넘김 → 전투력 1만 기준 표 값) =====
    [[nodiscard]] double GetRankScale(double combatPower) noexcept;            // 랭크 · 깨달음 구성 · 상호작용 ln에 곱하는 배수 (낮은 전투력일수록 작음)
    [[nodiscard]] double GetEvolutionSlope(double combatPower) noexcept;       // 진화 레벨 1당 감소 비율 (전투력과 무관한 고정 전투력 → 전투력에 반비례)
    [[nodiscard]] double GetEnlightenmentSlope(double combatPower) noexcept;   // 깨달음 레벨 1당 감소 비율
}
