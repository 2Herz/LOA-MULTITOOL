export module LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모델 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기

export namespace LostArkMultiTool::Models  // 모델 네임스페이스 내보내기 시작
{
    // ===== 개수 =====
    inline constexpr std::size_t ACCESSORY_SLOT_COUNT{ 5 };       // 목걸이 1 · 귀걸이 2 · 반지 2
    inline constexpr std::size_t ACCESSORY_OPTION_COUNT{ 3 };     // 악세서리 연마 효과 줄 수
    inline constexpr std::size_t BRACELET_STAT_COUNT{ 4 };        // 팔찌 스탯 줄 수
    inline constexpr std::size_t BRACELET_OPTION_COUNT{ 4 };      // 팔찌 옵션 줄 수 (고정 옵션 포함)
    inline constexpr std::size_t STONE_LINE_COUNT{ 2 };           // 스톤 각인 줄 수 (감소 각인 제외)
    inline constexpr std::size_t BOOK_COUNT{ 5 };                 // 각인서(유물 각인) 수
    inline constexpr std::size_t AVATAR_SLOT_COUNT{ 4 };          // 무기 · 머리 · 상의 · 하의
    inline constexpr std::size_t ARK_CORE_COUNT{ 6 };             // 질서 해 · 달 · 별, 혼돈 해 · 달 · 별
    inline constexpr std::size_t ARK_GEM_EFFECT_COUNT{ 3 };       // 공격력 · 추가 피해 · 보스 피해
    inline constexpr std::size_t PASSIVE_TREE_COUNT{ 3 };         // 진화 · 깨달음 · 도약

    // ===== 범위 =====
    inline constexpr int MIN_VALUE{ 0 };  // 값의 최소 허용 범위
    inline constexpr int MAX_BRACELET_STAT{ 999 };                // 팔찌 스탯 수치 상한
    inline constexpr int MAX_ARK_CORE_POINTS{ 20 };               // 코어 의지력 포인트 합 상한
    inline constexpr int MAX_ARK_GEM_LEVEL_SUM{ 200 };            // 젬 효과 레벨 합 상한
    inline constexpr int MAX_PASSIVE_LEVEL{ 30 };                 // 아크 패시브 트리 레벨 상한
    inline constexpr int MAX_PASSIVE_RANK{ 6 };                   // 아크 패시브 랭크 상한
    inline constexpr int MAX_VAMBRACE_LEVEL{ 25 };                // 완갑 재련 상한
    inline constexpr double MAX_ACCESSORY_MAIN_STAT{ 99999.0 };   // 악세서리 주스탯 상한

    // ===== 문구 =====
    inline constexpr std::string_view NONE_TEXT{ "없음" };        // 옵션이 비어 있음
    inline constexpr std::string_view FULL_ENLIGHTENMENT{ "상급 + 일반3" };   // 깨달음 포인트 최대 구성 (= 배율 1)
    inline constexpr std::array<std::string_view, ARK_GEM_EFFECT_COUNT> ARK_GEM_EFFECT_NAMES{ "공격력", "추가 피해", "보스 피해" };  // 아크 젬 효과 이름 표 (배열 번호 = 효과 번호)

    // 아바타 등급
    enum class AvatarGrade
    {
        AVATAR_NONE,         // 없음
        AVATAR_EPIC,         // 영웅
        AVATAR_LEGENDARY     // 전설
    };

    // 아크 그리드 코어 등급
    enum class ArkCoreGrade
    {
        CORE_EPIC,           // 영웅
        CORE_LEGENDARY,      // 전설
        CORE_RELIC,          // 유물
        CORE_ANCIENT         // 고대
    };

    // 완갑 등급 (한계 해방으로 오름, 번호 = 계수 표 GetVambraceGradeBonuses() 순서)
    enum class VambraceGrade
    {
        VAMBRACE_EPIC,       // 영웅 (재련 전 ~ 10단계)
        VAMBRACE_LEGENDARY,  // 전설 (10단계 해방 ~ 15단계)
        VAMBRACE_RELIC,      // 유물 (15단계 해방 ~ 20단계)
        VAMBRACE_ANCIENT     // 고대 (20단계 해방 ~ 25단계)
    };
    inline constexpr std::size_t VAMBRACE_GRADE_COUNT{ 4 };  // 완갑 등급 개수
    inline constexpr std::array<int, VAMBRACE_GRADE_COUNT> VAMBRACE_GRADE_MIN_LEVELS{ 0, 10, 15, 20 };   // 등급별 재련 가능 최저 단계 (해방 직후 단계)
    inline constexpr std::array<int, VAMBRACE_GRADE_COUNT> VAMBRACE_GRADE_MAX_LEVELS{ 10, 15, 20, 25 };  // 등급별 재련 가능 최고 단계 (해방 전 단계)

    // 악세서리 한 개 (연마 효과 문구 3개 + 기본 효과 주스탯)
    struct AccessoryState
    {
        std::array<std::string, ACCESSORY_OPTION_COUNT> m_options;   // 정확한 문구 ("적에게 주는 피해 +2.00%", 비었으면 없음)
        double m_mainStat{};                                          // 힘 · 민첩 · 지능 (같은 값)
        [[nodiscard]] bool operator==(const AccessoryState& other) const = default;  // 모든 멤버가 같으면 같은 상태 (기본 비교 연산)
    };

    // 팔찌 스탯 한 줄
    struct BraceletStat
    {
        std::string m_type;                  // "치명" · "특화" · "신속" · "힘" ... (비었으면 없음)
        int m_value{};  // 팔찌 스탯 수치
        [[nodiscard]] bool operator==(const BraceletStat& other) const = default;  // 모든 멤버가 같으면 같은 상태 (기본 비교 연산)
    };

    // 스톤 각인 한 줄
    struct StoneLine
    {
        std::string m_name;                  // 각인 이름 (비었으면 없음)
        int m_level{};                       // 0 ~ 4
        [[nodiscard]] bool operator==(const StoneLine& other) const = default;  // 모든 멤버가 같으면 같은 상태 (기본 비교 연산)
    };

    // 각인서(유물 각인) 하나
    struct BookState
    {
        std::string m_name;                  // 각인 이름 (비었으면 없음)
        bool m_isLegendary{};                // 전설 등급 각인서 (유물보다 약간 낮음)
        int m_level{};                       // 각인서 레벨 0 ~ 4
        [[nodiscard]] bool operator==(const BookState& other) const = default;  // 모든 멤버가 같으면 같은 상태 (기본 비교 연산)
    };

    // 아크 그리드 코어 하나
    struct ArkCoreState
    {
        ArkCoreGrade m_grade{ ArkCoreGrade::CORE_RELIC };  // 코어 등급 (기본 유물)
        int m_points{};                      // 의지력 포인트 합
        std::string m_name;                  // 코어 이름 ("샷건 오버로드" · "현란한 공격" ..., API에서 읽음, 모르면 빈 글자)
        [[nodiscard]] bool operator==(const ArkCoreState& other) const = default;  // 모든 멤버가 같으면 같은 상태 (기본 비교 연산)
    };

    // 아크 패시브
    struct ArkPassiveState
    {
        int m_rank{ MAX_PASSIVE_RANK };                                       // 랭크 0 ~ 6 (세 트리 중 가장 낮은 값, 편집기 표시용)
        std::array<int, PASSIVE_TREE_COUNT> m_treeRanks{ MAX_PASSIVE_RANK, MAX_PASSIVE_RANK, MAX_PASSIVE_RANK };  // 진화 · 깨달음 · 도약 트리별 랭크 (랭크를 고치면 셋 다 같은 값)
        std::string m_enlightenment{ FULL_ENLIGHTENMENT };                    // 깨달음 포인트 구성
        std::array<int, PASSIVE_TREE_COUNT> m_levels{};                       // 진화 · 깨달음 · 도약 레벨
        [[nodiscard]] bool operator==(const ArkPassiveState& other) const = default;  // 모든 멤버가 같으면 같은 상태 (기본 비교 연산)
    };

    // 장비 재련 · 보석 · 무기 품질 말고 전투력에 영향을 주는 모든 시스템의 상태
    // (현재 = API에서 읽은 값, 목표 = 사용자가 고친 값, 같은 모양)
    struct ExtraState
    {
        std::array<AccessoryState, ACCESSORY_SLOT_COUNT> m_accessories;     // 목걸이, 귀걸이 1 · 2, 반지 1 · 2
        std::array<BraceletStat, BRACELET_STAT_COUNT> m_braceletStats;  // 팔찌 스탯 줄들
        std::array<std::string, BRACELET_OPTION_COUNT> m_braceletOptions;   // 공백을 모두 지운 문구
        std::array<StoneLine, STONE_LINE_COUNT> m_stone;  // 스톤 각인 줄들
        std::array<BookState, BOOK_COUNT> m_books;  // 각인서들
        std::array<AvatarGrade, AVATAR_SLOT_COUNT> m_avatars{};             // 무기 · 머리 · 상의 · 하의
        std::array<ArkCoreState, ARK_CORE_COUNT> m_cores;  // 아크 그리드 코어들
        std::array<int, ARK_GEM_EFFECT_COUNT> m_arkGemLevels{};             // 효과별 젬 레벨 합 (공격력 · 추가 피해 · 보스 피해)
        ArkPassiveState m_passive;  // 아크 패시브 상태
        std::optional<int> m_vambraceLevel;                                  // 완갑 재련 (없으면 nullopt)
        std::optional<double> m_vambraceGradeRate;                           // 완갑 한계 해방 추가 효과 기본 공격력 (0.02 = 2%, API에서 읽음, 모르면 등급 표로 추정)
        std::optional<VambraceGrade> m_vambraceGrade;                        // 완갑 등급 (현재 = API 등급, 목표 = 사용자가 고름, 모르면 단계로 추정)
        bool m_isEstherWeapon{};                                             // 에스더 무기 (완갑 · 무기 공격력 옵션 계수 선택용)
        [[nodiscard]] bool operator==(const ExtraState& other) const = default;  // 모든 멤버가 같으면 같은 상태 (기본 비교 연산)
    };

    // ===== 글자 도우미 (툴팁 · 표시 · 표 키 공용) =====
    // 연속 공백을 하나로 줄이고 양 끝 공백을 지운 글자
    [[nodiscard]] std::string NormalizeSpaces(std::string_view text);  // 글자 정리 함수 선언

    // 모든 공백을 지운 글자 (팔찌 옵션 표의 키)
    [[nodiscard]] std::string RemoveSpaces(std::string_view text);  // 공백 제거 함수 선언

    // 옵션이 비어 있는지 ("" 또는 "없음")
    [[nodiscard]] bool IsNone(std::string_view text) noexcept;  // 빈 옵션 판별 함수 선언

    // ===== 규칙 =====
    namespace CombatExtra  // 추가 시스템 규칙 네임스페이스
    {
        // 목표가 올려 주는 주스탯 합 (악세서리 주스탯 + 팔찌 힘/민첩/지능), 장비 재련의 S에 더해짐
        [[nodiscard]] double GetMainStatGain(const ExtraState& current, const ExtraState& target);  // 주스탯 증가량 계산 함수 선언

        // 완갑 재련이 바꾸는 값 (현재 → 목표 단계의 차이)
        struct VambraceGain  // 완갑 재련 변화량 구조체
        {
            double m_weaponAttack{};         // 무기 공격력 차이 → 장비 재련의 W에 더해짐
            double m_mainStat{};             // 주스탯 차이 → 장비 재련의 S에 더해짐
            double m_baseAttack{};           // 기본 공격력 고정값 차이 → 완갑 배율 (A + ΔB) / A
            double m_gradeRateFrom{};        // 현재 완갑 한계 해방 추가 효과 기본 공격력 비율 (보석 기본 공격력 %와 합산)
            double m_gradeRateTo{};          // 목표 완갑 한계 해방 추가 효과 기본 공격력 비율
        };

        // 완갑 재련 단계의 기본 등급 (해방 경계 10 · 15 · 20단계는 해방 전 등급)
        [[nodiscard]] VambraceGrade GetVambraceGradeForLevel(int level) noexcept;  // 단계 → 기본 등급 함수 선언

        // 그 등급에서 가질 수 있는 재련 단계인지 (영웅 0~10 · 전설 10~15 · 유물 15~20 · 고대 20~25)
        [[nodiscard]] bool IsVambraceGradeValid(VambraceGrade grade, int level) noexcept;  // 등급 · 단계 조합 검사 함수 선언

        // 완갑 등급 이름 ("영웅" · "전설" · "유물" · "고대") → 등급 (모르면 nullopt)
        [[nodiscard]] std::optional<VambraceGrade> ParseVambraceGrade(std::string_view name) noexcept;  // 등급 이름 해석 함수 선언

        // 완갑 재련 단계 차이 (현재나 목표 완갑이 없으면 모두 0 · 배율 1)
        [[nodiscard]] VambraceGain GetVambraceGain(const ExtraState& current, const ExtraState& target);  // 완갑 재련 변화량 계산 함수 선언

        // 입력 범위 안으로 맞춤 (스톤 · 각인서 · 코어 · 패시브 · 완갑 수치)
        [[nodiscard]] ExtraState Clamp(const ExtraState& state);  // 범위 보정 함수 선언
    }
}
