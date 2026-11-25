export module LostArkMultiTool.Services.TransferCalculator;  // 2부 이관 계산기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모델 가져오기 (스톤 줄)
import LostArkMultiTool.Models.TransferData;  // 2부 이관 표 가져오기

export namespace LostArkMultiTool::Services  // 서비스 네임스페이스 내보내기 시작
{
    // 2부 슬롯 하나의 이관 결과 (장비 쪽 8칸 · 장신구 쪽 8칸 공용)
    struct TransferSlotResult
    {
        std::string_view m_slotName;                  // 2부 슬롯 이름 ("무기" · "목걸이" · "허리띠" ...)
        std::string_view m_sourceName;                // 기준 부위 이름 ("투구" · "무기", API 부위 이름)
        bool m_isNewSlot{};                           // 2부에 새로 생기는 슬롯
        int m_sourceLevel{};                          // 기준 장비 아이템 레벨 (없으면 0)
        std::string m_sourceGrade;                    // 기준 장비 등급 (UTF-8, 없으면 빈 글자)
        std::optional<Models::TransferData::GearTransferRow> m_target;            // 이관 장비 (표 밖이면 nullopt)
        std::optional<Models::TransferData::EstherTransferRow> m_esther;          // 에스더 무기 슬롯이면 엘라 단계 이관
        std::optional<int> m_slotRefine;              // 슬롯 재련 단계 (장비 쪽만, 모르면 nullopt)
        int m_advancedRefine{};                       // 지금 상급 재련 단계 (2부에서 시스템 삭제, 표시용)
        [[nodiscard]] int GetTargetLevel() const noexcept;   // 이관 아이템 레벨 (에스더 우선, 표 밖이면 0)
    };

    // 장신구 하나 → 장신구 주문서
    struct ScrollResult
    {
        std::string_view m_slotName;                  // "목걸이" · "귀걸이1" ...
        std::string m_itemName;                       // 장신구 이름 (UTF-8, 없으면 빈 글자)
        std::string m_grade;                          // 등급 ("고대" · "유물" ...)
        std::optional<int> m_quality;                 // 품질
        int m_tier{};                                 // 아이템 티어
        std::string m_polishText;                     // 연마 판정 ("상상중", 딜러 · 서포터 중 이관 등급이 좋은 쪽)
        std::optional<int> m_scrollGrade;             // 이관 주문서 등급 (1 ~ 6, 대상이 아니면 nullopt)
        bool m_isSupporterBasis{};                    // 서포터 기준으로 판정한 등급인지
        std::string_view m_reason;                    // 대상이 아닌 이유 (대상이면 빈 글자)
    };

    // 팔찌 옵션 한 줄 → 인장 주문서 티어
    struct BraceletLineResult
    {
        std::string m_text;                           // 옵션 글자 (띄어쓰기를 뺀 원문 또는 "특화 +93")
        std::string_view m_label;                     // 분류 이름 (모르면 빈 글자)
        std::string_view m_dealerTier;                // 딜러 기준 티어 (점수 없으면 빈 글자)
        std::string_view m_supporterTier;             // 서포터 기준 티어
    };

    // 보석 → 광휘 · 성휘
    struct GemResult
    {
        std::array<int, Models::TransferData::MAX_GEM_LEVEL + 1> m_levelCounts{};           // 지금 레벨별 보석 개수 (번호 = 레벨)
        std::array<int, Models::TransferData::MAX_GEM_LEVEL + 1> m_transferLevelCounts{};   // 이관 뒤 광휘 레벨별 개수 (티어 3은 2레벨 낮게)
        int m_totalCount{};                           // 지금 보석 수
        int m_luminousCount{};                        // 이관 뒤 광휘 수 (티어 3 1 · 2레벨은 제거돼서 빠짐)
        int m_loweredCount{};                         // 2레벨 낮아지는 티어 3 보석 수 (멸화 · 홍염 3레벨 이상)
        int m_removedCount{};                         // 제거되는 티어 3 보석 수 (멸화 · 홍염 1 · 2레벨)
        int m_starTwoCount{};                         // 10레벨로 만들 수 있는 성휘 2레벨 수 (10레벨 6개당 1개)
        int m_starOneCount{};                         // 남은 10레벨로 만들 수 있는 성휘 1레벨 수 (3개당 1개)
        int m_leftTenCount{};                         // 성휘로 못 바꾸고 남는 10레벨 광휘 수
    };

    // 어빌리티 스톤 → 각인 업그레이드
    struct StoneResult
    {
        std::array<Models::StoneLine, Models::STONE_LINE_COUNT> m_lines{};   // 스톤 각인 두 줄
        std::array<bool, Models::STONE_LINE_COUNT> m_isValid{};              // 유효 각인 조합인지 (두 줄 모두 딜러 목록, 서포터는 두 줄 모두 서포터 목록도 인정)
        int m_high{};                                 // 높은 쪽 유효 활성화 포인트
        int m_low{};                                  // 낮은 쪽 유효 활성화 포인트
        std::string_view m_result;                    // 이관 결과 (표 밖이면 빈 글자)
        bool m_hasStone{};                            // 스톤을 끼고 있는지
    };

    // 캐릭터 하나의 2부 이관 결과 전체
    struct TransferResult
    {
        std::vector<TransferSlotResult> m_armorSlots;      // 장비 쪽 8칸
        std::vector<TransferSlotResult> m_accessorySlots;  // 장신구 쪽 8칸
        std::vector<ScrollResult> m_scrolls;               // 장신구 5개 → 주문서
        std::vector<BraceletLineResult> m_braceletLines;   // 팔찌 스탯 · 옵션 줄
        std::string m_braceletGrade;                       // 팔찌 등급 ("고대" ..., 없으면 빈 글자)
        std::string_view m_braceletReason;                 // 팔찌가 대상이 아닌 이유 (대상이면 빈 글자)
        GemResult m_gems;                                  // 보석
        StoneResult m_stone;                               // 스톤
        double m_currentAverageLevel{};                    // 지금 평균 아이템 레벨 (API)
        double m_transferAverageLevel{};                   // 이관 뒤 16부위 평균 아이템 레벨 (추정, 표 밖 부위는 0으로 셈)
        int m_weaponBasisLevel{};                          // 무기 기준 부위에 쓴 레벨 (에스더 1800 이상이면 1800)
        bool m_isEsther{};                                 // 에스더 무기를 끼고 있는지
        bool m_isSupporter{};                              // 서포터 (아크 패시브 칭호)
        std::optional<int> m_bestScrollGrade;              // 가장 좋은 장신구 주문서 등급
    };

    // 2부 이관 계산기 (상태 없음, 표는 Models::TransferData)
    class TransferCalculator
    {
    public:
        [[nodiscard]] TransferResult Calculate(const Models::CharacterArmory& armory) const;   // 무기고 → 이관 결과

        // ===== 낱개 규칙 (검사 · 다른 화면에서도 씀) =====
        [[nodiscard]] static std::optional<Models::TransferData::GearTransferRow> FindGearRow(int level) noexcept;      // 장비 레벨 → 이관 장비
        [[nodiscard]] static std::optional<Models::TransferData::EstherTransferRow> FindEstherRow(int level) noexcept; // 에스더 레벨 → 이관 (같은 레벨 행, 없으면 바로 아래 레벨 행)
        [[nodiscard]] static std::optional<Models::TransferData::EstherTransferRow> FindEstherRow(int evolution, int ela, int level) noexcept;   // 진화 · 엘라 단계로 먼저 찾고, 없으면 레벨로
        [[nodiscard]] static Models::TransferData::PolishRank RankPolishLine(std::string_view line, std::string& key);  // 연마 효과 한 줄 → 상 · 중 · 하 (key = 표 키)
        [[nodiscard]] static std::string_view FindStoneResult(int high, int low) noexcept;    // 활성화 포인트 → 각인 업그레이드
    };
}
