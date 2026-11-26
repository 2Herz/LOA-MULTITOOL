export module LostArkMultiTool.Vision.ParadiseDetector;  // 낙원력 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기

// 낙원 콘텐츠 UI의 "현재 낙원력 29,492,422" 읽기 ("권장 낙원력"은 제외)
// 결과 값 이름: VALUE_KEY ("paradise")
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 낙원력 인식기 (전략 구현)
    class ParadiseDetector final : public ScreenDetector
    {
    public:
        static constexpr std::string_view SCREEN_NAME{ "paradise" };   // 화면 이름
        static constexpr std::string_view VALUE_KEY{ "paradise" };     // 결과 값 이름

        [[nodiscard]] std::string_view GetScreenName() const noexcept override;   // 담당 화면 이름
        [[nodiscard]] std::optional<ScanResult> Detect(const FrameImage& frame, const DetectorContext& context) override;   // 낙원력 읽기
    };
}
