export module LostArkMultiTool.Vision.TooltipDetector;  // 재료 툴팁 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기

// 창고 · 인벤토리 재료에 마우스를 올렸을 때 뜨는 툴팁 읽기
// - 제목 줄: "운명의 파괴석 결정 (귀속) [X 163439]" → 재료 이름 (귀속 표기가 같은 줄에 있어야 함 → 거래 가능 재료는 제외)
// - "전체 보유 수량: 163,439개" → 수량 (창고 칸에 "9999+"로만 보이는 재료도 정확한 수량)
// 결과 값 이름: OCRTextParser::MakeMaterialValueKey ("Dest/top" · "Yong/level")
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 재료 툴팁 인식기 (전략 구현)
    class TooltipDetector final : public ScreenDetector
    {
    public:
        static constexpr std::string_view SCREEN_NAME{ "tooltip" };   // 화면 이름

        [[nodiscard]] std::string_view GetScreenName() const noexcept override;   // 담당 화면 이름
        [[nodiscard]] std::optional<ScanResult> Detect(const FrameImage& frame, const DetectorContext& context) override;   // 툴팁 읽기
    };
}
