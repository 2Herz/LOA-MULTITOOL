export module LostArkMultiTool.Vision.ShardBarDetector;  // 상단바 운명의 파편 인식기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기

// 게임 화면 맨 위 재화 줄에서 운명의 파편 수량 읽기
// - 재화 줄에는 실링 · 골드 · 파편 등 숫자가 여러 개 → 숫자 오른쪽 아이콘이 보라색인 것을 파편으로 봄
// - 커서 주변 OCR 영역에 맨 위 줄이 들어와야 읽힘 (화면 위쪽에 마우스를 올려 두면 됨)
// 결과 값 이름: OCRTextParser::MakeMaterialValueKey("Shard", TIER_BY_LEVEL) ("Shard/level")
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 상단바 파편 인식기 (전략 구현)
    class ShardBarDetector final : public ScreenDetector
    {
    public:
        static constexpr std::string_view SCREEN_NAME{ "shard_bar" };   // 화면 이름

        [[nodiscard]] std::string_view GetScreenName() const noexcept override;   // 담당 화면 이름
        [[nodiscard]] std::optional<ScanResult> Detect(const FrameImage& frame, const DetectorContext& context) override;   // 파편 읽기

        // 숫자 오른쪽 아이콘 영역이 보라색인지 (자체 검사에서도 씀)
        [[nodiscard]] static bool IsPurpleIcon(const FrameImage& frame, const PixelRect& numberBounds) noexcept;
    };
}
