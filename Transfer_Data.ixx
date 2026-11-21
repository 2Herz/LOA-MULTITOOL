export module LostArkMultiTool.Models.TransferData;  // 2부 이관 표 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기

// 2부(겨울 업데이트) 기존 성장 이관 표 (2026-11-20 로스트아크 On Air 슬라이드 기준)
// 11/25 이관 상세 공지가 나오면 이 모듈의 표만 고치면 되도록 계산 규칙과 분리함
export namespace LostArkMultiTool::Models::TransferData  // 2부 이관 표 네임스페이스 내보내기 시작
{
    // ===== 공통 상수 =====
    inline constexpr int GEAR_MIN_EXCLUSIVE_LEVEL{ 1600 };     // 장비 이관 표의 시작 (이 레벨 "초과"부터 이관 장비를 받음)
    inline constexpr int ESTHER_FULL_LEVEL{ 1800 };            // 이 레벨 이상 에스더 무기 → 무기 기준 부위는 25강 무기(1800) 기준으로 받음
    inline constexpr int ESTHER_FULL_SLOT_REFINE{ 25 };        // 9 · 10단계 엘라3 에스더 무기의 무기 슬롯 재련 단계
    inline constexpr int NEW_SLOT_REFINE{ 0 };                 // 새로 생기는 허리띠 슬롯 재련 시작 단계
    inline constexpr int ACCESSORY_MIN_TIER{ 4 };              // 장신구 · 팔찌 주문서 이관 대상 티어
    inline constexpr int ACCESSORY_MIN_QUALITY{ 67 };          // 장신구 주문서 이관 대상 최소 품질
    inline constexpr std::size_t SLOT_COUNT_PER_SIDE{ 8 };     // 2부 장비 8칸 + 장신구 8칸
    inline constexpr std::size_t TRANSFER_SLOT_TOTAL{ SLOT_COUNT_PER_SIDE * 2 };   // 아이템 레벨 평균에 들어가는 16부위
    inline constexpr int STAR_FIRST_COST{ 3 };                 // 10레벨 광휘 3개 → 1레벨 성휘 1개
    inline constexpr int STAR_NEXT_COST{ 2 };                  // 성휘 2개 → 1레벨 상승 (이번 겨울은 2레벨까지)
    inline constexpr int MAX_GEM_LEVEL{ 10 };                  // 광휘 최고 레벨 (성휘 재료)

    // ===== 장비 (부위별 아이템 레벨 → 이관 장비) =====
    struct GearTransferRow
    {
        int m_maxLevel{};                 // 기존 장비 레벨 상한 (이전 행 상한 "초과" ~ 이 값 "이하")
        std::string_view m_grade;         // 이관 장비 등급 ("고급" · "희귀" · "영웅" · "전설" · "유물" · "고대")
        int m_polish{};                   // 이관 장비 연마 단계 (0 ~ 5, 1회 +5레벨)
        int m_level{};                    // 이관 장비 아이템 레벨
    };
    [[nodiscard]] std::span<const GearTransferRow> GetGearTable() noexcept;   // 1600 초과 ~ 1800 이하, 레벨 오름차순

    // ===== 에스더 무기 (진화 · 엘라 단계 → 엘라 1 ~ 4단계) =====
    struct EstherTransferRow
    {
        int m_maxLevel{};                 // 기존 에스더 무기 레벨 상한 (이하)
        std::string_view m_source;        // 기존 단계 설명 ("9단계 엘라3" 등)
        int m_elaStage{};                 // 이관 엘라 단계 (1 ~ 4)
        int m_polish{};                   // 이관 연마 단계 (4단계부터, 0 ~ 5)
        int m_level{};                    // 이관 아이템 레벨
        std::string_view m_refund;        // 재료 환급 안내 (없으면 빈 글자)
    };
    [[nodiscard]] std::span<const EstherTransferRow> GetEstherTable() noexcept;   // 레벨 오름차순 (마지막 행 = 그 위 전부)

    // ===== 장신구 · 무기 기준 부위 짝 =====
    struct SlotPair
    {
        std::string_view m_target;        // 2부 슬롯 이름 ("목걸이" · "허리띠" ...)
        std::string_view m_source;        // 기준이 되는 지금 장비 부위 이름 ("투구" · "무기")
        bool m_isNewSlot{};               // 2부에 새로 생기는 슬롯 (허리띠 · 성물 · 인장)
    };
    [[nodiscard]] std::span<const SlotPair> GetArmorSidePairs() noexcept;       // 장비 쪽 8칸 (무기 · 투구 · 견갑 · 상의 · 하의 · 장갑 · 완갑 · 허리띠)
    [[nodiscard]] std::span<const SlotPair> GetAccessorySidePairs() noexcept;   // 장신구 쪽 8칸 (목걸이 · 귀걸이 2 · 반지 2 · 성물 · 인장 · 팔찌)

    // ===== 장신구 연마 효과 상 · 중 · 하 =====
    inline constexpr std::size_t POLISH_RANK_COUNT{ 3 };       // 상 · 중 · 하
    enum class PolishRank
    {
        RANK_HIGH,                        // 상
        RANK_MIDDLE,                      // 중
        RANK_LOW,                         // 하
        RANK_NONE                         // 옵션 없음 · 표에 없는 옵션
    };
    struct PolishTier
    {
        std::string_view m_key;           // 띄어쓰기를 뺀 옵션 이름 (공격력 · 무기 공격력은 끝에 "+" · "%"로 구분)
        std::array<double, POLISH_RANK_COUNT> m_amounts{};   // 상 · 중 · 하 수치 (퍼센트 옵션은 % 단위)
    };
    [[nodiscard]] std::span<const PolishTier> GetPolishTiers() noexcept;       // 장신구 연마 효과 전체 (딜러 · 서포터 · 자잘한 옵션)
    [[nodiscard]] std::string_view GetPolishRankName(PolishRank rank) noexcept;   // "상" · "중" · "하" · "×" (옵션 없음)

    // 부위별 이관 판정에 쓰는 옵션 묶음 (유효 옵션 2개 + 세 번째 칸으로 인정하는 옵션)
    struct AccessoryRule
    {
        std::string_view m_partType;      // API 장비 종류 ("목걸이" · "귀걸이" · "반지")
        std::array<std::string_view, 2> m_valid;     // 유효 옵션 두 개 (상상의 앞 두 글자)
        std::span<const std::string_view> m_fillers; // 세 번째 칸으로 인정하는 옵션 (상상상의 마지막 글자)
    };
    [[nodiscard]] std::span<const AccessoryRule> GetDealerAccessoryRules() noexcept;      // 딜러 목걸이 · 귀걸이 · 반지
    [[nodiscard]] std::span<const AccessoryRule> GetSupporterAccessoryRules() noexcept;   // 서포터 목걸이 · 귀걸이 · 반지

    // 서포터 귀걸이 세 번째 칸 (최대 생명력 = 2등급, 보호막 · 회복 = 3등급)
    inline constexpr std::string_view SUPPORT_EARRING_HEALTH_KEY{ "최대생명력" };   // 최대 생명력 옵션 키
    inline constexpr std::string_view SUPPORTER_EARRING_TYPE{ "귀걸이" };            // 서포터 귀걸이 예외 표가 적용되는 부위

    // 등급 번호 (고대 기준, 숫자가 작을수록 좋음)
    inline constexpr int SCROLL_GRADE_BEST{ 1 };                // 상상상
    inline constexpr int SCROLL_GRADE_HIGH_HIGH_MIDDLE{ 2 };    // 상상중
    inline constexpr int SCROLL_GRADE_HIGH_HIGH{ 3 };           // 상상
    inline constexpr int SCROLL_GRADE_HIGH_MIDDLE{ 4 };         // 상중 · 중상
    inline constexpr int SCROLL_GRADE_HIGH_LOW{ 5 };            // 상하 · 하상
    inline constexpr int SCROLL_GRADE_WORST{ 6 };               // 중중 이하
    inline constexpr int SUPPORT_EARRING_HEALTH_GRADE{ 2 };     // 고대 서포터 귀걸이 상상상(최생)
    inline constexpr int SUPPORT_EARRING_SHIELD_GRADE{ 3 };     // 고대 서포터 귀걸이 상상상(보호막 · 회복)
    inline constexpr int SUPPORT_EARRING_HIGH_HIGH_MIDDLE{ 4 }; // 고대 서포터 귀걸이 상상중
    inline constexpr int SUPPORT_EARRING_HIGH_HIGH{ 5 };        // 고대 서포터 귀걸이 상상
    inline constexpr int RELIC_SUPPORTER_HIGH_HIGH{ 5 };        // 유물 서포터 목걸이 · 반지 상상
    inline constexpr std::string_view ANCIENT_GRADE{ "고대" };  // 고대 등급 이름
    inline constexpr std::string_view RELIC_GRADE{ "유물" };    // 유물 등급 이름
    inline constexpr std::string_view ESTHER_GRADE{ "에스더" }; // 에스더 등급 이름

    // ===== 팔찌 → 인장 주문서 (옵션 티어, 등급 계산식은 11/25 공지 대기) =====
    struct BraceletRule
    {
        std::array<std::string_view, 2> m_keywords;   // 띄어쓰기를 뺀 옵션 글자에 모두 들어 있어야 하는 낱말 (두 번째가 비면 하나만)
        std::string_view m_label;         // 표시 이름 ("치적 & 치명 시 피해" 등)
        std::string_view m_dealerTier;    // 딜러 기준 티어 ("1티어" · "2티어+" ..., 점수 없으면 빈 글자)
        std::string_view m_supporterTier; // 서포터 기준 티어 (점수 없으면 빈 글자)
    };
    [[nodiscard]] std::span<const BraceletRule> GetBraceletRules() noexcept;   // 앞에서부터 처음 맞는 규칙을 씀

    struct BraceletStatRule
    {
        std::string_view m_stat;          // 스탯 이름 ("치명" · "특화" · "신속" · "힘" · "민첩" · "지능")
        std::string_view m_dealerTier;    // 딜러 기준 티어
        std::string_view m_supporterTier; // 서포터 기준 티어 (점수 없으면 빈 글자)
    };
    [[nodiscard]] std::span<const BraceletStatRule> GetBraceletStatRules() noexcept;   // 팔찌 스탯 줄 티어

    // ===== 어빌리티 스톤 → 각인 업그레이드 =====
    struct StoneTransferRow
    {
        int m_high{};                     // 높은 쪽 활성화 포인트 (유효 각인)
        int m_low{};                      // 낮은 쪽 활성화 포인트 (유효 각인)
        std::string_view m_result;        // 이관 결과 ("슬롯 5개 7단계" 등)
    };
    [[nodiscard]] std::span<const StoneTransferRow> GetStoneTable() noexcept;   // 표에 없는 조합은 "표 밖 (공지 대기)"

    // ===== 안내 =====
    [[nodiscard]] std::string_view GetSourceText() noexcept;   // 표 출처 ("2026-11-20 On Air 슬라이드 · 상세 공지 11/25 예정")
}
