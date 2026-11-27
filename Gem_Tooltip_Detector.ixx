export module LostArkMultiTool.Vision.GemTooltipDetector;  // 귀속 젬 툴팁 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기

// 창고 · 소지품의 귀속 젬에 마우스를 올렸을 때 뜨는 툴팁 읽기 (귀속재료 툴팁과 같은 방식)
// - 제목: "질서의 젬 : 안정 [X 2]" → 젬 6종 중 하나 / 등급 줄: "고급 · 희귀 · 영웅 아크 그리드 젬"
// - "캐릭터 귀속됨"이 있어야 하고, "전체 보유 수량: 2개"가 있어야 함 (가공한 영웅 젬처럼 수량이 없는 젬은 건너뜀)
// 결과 값 이름: MakeValueKey(젬 번호, 등급 번호) ("Gem/0/1") → 귀속 젬 계산기의 지금 접속한 캐릭터 칸
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 귀속 젬 툴팁 인식기 (전략 구현)
    class GemTooltipDetector final : public ScreenDetector
    {
    public:
        static constexpr std::string_view SCREEN_NAME{ "gem_tooltip" };   // 화면 이름
        static constexpr std::string_view VALUE_PREFIX{ "Gem/" };         // 결과 값 이름 머리

        [[nodiscard]] std::string_view GetScreenName() const noexcept override;   // 담당 화면 이름
        [[nodiscard]] std::optional<ScanResult> Detect(const FrameImage& frame, const DetectorContext& context) override;   // 젬 툴팁 읽기

        [[nodiscard]] static std::string MakeValueKey(std::size_t gemIndex, std::size_t gradeIndex);   // "Gem/젬/등급"
        [[nodiscard]] static std::optional<std::pair<std::size_t, std::size_t>> ParseValueKey(std::string_view valueKey);   // 되돌리기 (젬 값이 아니면 nullopt)
        [[nodiscard]] static std::optional<std::size_t> MatchGemName(std::wstring_view line);    // 제목 줄 → 젬 번호 (BoundGemData::GEM_NAMES 순서)
        [[nodiscard]] static std::optional<std::size_t> MatchGrade(std::wstring_view line);      // 등급 줄 → 등급 번호 (고급 · 희귀 · 영웅)
    };
}
