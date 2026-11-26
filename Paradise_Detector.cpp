module LostArkMultiTool.Vision.ParadiseDetector;  // 낙원력 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.OCRTextParser;  // OCR 글자 해석 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::wstring_view SCORE_KEYWORD{ L"현재 낙원력" };   // 점수 줄 글자
        constexpr std::size_t SCORE_MAX_DISTANCE{ 1 };                 // 오인식 허용 (1글자 → "권장 낙원력"은 2글자 차이라 제외)
        constexpr std::int64_t MIN_SCORE{ 1 };                         // 이보다 작으면 오인식으로 봄
    }

    std::string_view ParadiseDetector::GetScreenName() const noexcept
    {
        return SCREEN_NAME;   // "paradise"
    }

    std::optional<ScanResult> ParadiseDetector::Detect(const FrameImage&, const DetectorContext& context)
    {
        const auto score = OCRTextParser::FindNumberAfter(context.m_hoverText, SCORE_KEYWORD, SCORE_MAX_DISTANCE, context.m_hoverArea.m_width);   // 현재 낙원력
        if (!score || *score < MIN_SCORE) return std::nullopt;                                   // 낙원 화면이 아님

        ScanResult result;                                                                       // 결과
        result.m_values.push_back(ScanValue{ std::string{ VALUE_KEY }, *score });                // 낙원력
        return result;                                                                           // 낙원 결과
    }
}
