export module LostArkMultiTool.Vision.GemTooltipDetector;  // 귀속 젬 툴팁 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.BoundGemData;  // 젬 수량 종류 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // OCR 결과 가져오기

// 창고 · 소지품의 젬에 마우스를 올렸을 때 뜨는 툴팁 읽기 (귀속재료 툴팁과 같은 방식)
// - 제목: "질서의 젬 : 안정 [X 2]" → 젬 6종 중 하나 / 등급 줄: "고급 · 희귀 · 영웅 아크 그리드 젬"
// - 묶음 젬: "전체 보유 수량: 2개" → "귀속"이 있으면 귀속 묶음, 없으면 거래 가능 묶음
// - 가공 중인 젬: 수량 줄이 없고 가공 정보가 있음 → 한 개, 툴팁 숫자(포인트 · 횟수 · 의지력 · 효과)로 서로 구분
// 결과 값 이름: MakeValueKey(젬, 등급, 종류[, 구분]) ("Gem/0/1/B" · "Gem/0/1/T" · "Gem/0/1/P/4.99…")
//   → 귀속 젬 계산기의 지금 접속한 캐릭터 칸 = 귀속 + 거래 가능 + 가공 젬 개수
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 젬 값 이름을 푼 결과
    struct GemValueKey
    {
        std::size_t m_gemIndex{};                                                // 젬 번호
        std::size_t m_gradeIndex{};                                              // 등급 번호
        Models::GemStockKind m_kind{ Models::GemStockKind::STOCK_BOUND };        // 종류
        std::string m_processedKey;                                              // 가공 젬 구분 글자 (가공 젬만)
        bool operator==(const GemValueKey&) const = default;                     // 비교 (자체 검사용)
    };

    // 귀속 젬 툴팁 인식기 (전략 구현)
    class GemTooltipDetector final : public ScreenDetector
    {
    public:
        static constexpr std::string_view SCREEN_NAME{ "gem_tooltip" };   // 화면 이름
        static constexpr std::string_view VALUE_PREFIX{ "Gem/" };         // 결과 값 이름 머리

        [[nodiscard]] std::string_view GetScreenName() const noexcept override;   // 담당 화면 이름
        [[nodiscard]] std::optional<ScanResult> Detect(const FrameImage& frame, const DetectorContext& context) override;   // 젬 툴팁 읽기

        [[nodiscard]] static std::string MakeValueKey(std::size_t gemIndex, std::size_t gradeIndex,
            Models::GemStockKind kind = Models::GemStockKind::STOCK_BOUND, std::string_view processedKey = {});   // "Gem/젬/등급/종류[/구분]"
        [[nodiscard]] static std::optional<GemValueKey> ParseValueKey(std::string_view valueKey);   // 되돌리기 (젬 값이 아니면 nullopt)
        [[nodiscard]] static std::string MakeProcessedKey(const TextResult& text);                 // 가공 젬 구분 글자 (정보 줄 숫자, 없으면 빈 글자)
        [[nodiscard]] static std::optional<std::size_t> MatchGemName(std::wstring_view line);    // 제목 줄 → 젬 번호 (BoundGemData::GEM_NAMES 순서)
        [[nodiscard]] static std::optional<std::size_t> MatchGrade(std::wstring_view line);      // 등급 줄 → 등급 번호 (고급 · 희귀 · 영웅)
    };
}
