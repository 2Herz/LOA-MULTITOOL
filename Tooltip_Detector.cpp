module LostArkMultiTool.Vision.TooltipDetector;  // 재료 툴팁 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.OCRTextParser;  // OCR 글자 해석 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // OCR 결과 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::wstring_view QUANTITY_KEYWORD{ L"전체 보유 수량" };   // 수량 줄 글자
        constexpr std::size_t QUANTITY_MAX_DISTANCE{ 2 };                    // 수량 줄 글자 오인식 허용 (6글자 중 2글자)
        constexpr std::wstring_view QUANTITY_WORD{ L"수량" };                // 제목 후보에서 뺄 줄 표시 ("겹침 수량" 등)
        constexpr std::wstring_view BOUND_MARK{ L"귀속" };                   // 귀속 재료 표시
    }

    std::string_view TooltipDetector::GetScreenName() const noexcept
    {
        return SCREEN_NAME;   // "tooltip"
    }

    std::optional<ScanResult> TooltipDetector::Detect(const FrameImage&, const DetectorContext& context)
    {
        const auto quantity = OCRTextParser::FindQuantityAfter(context.m_hoverText, QUANTITY_KEYWORD, QUANTITY_MAX_DISTANCE, context.m_hoverArea.m_width);   // 전체 보유 수량 ("개"를 숫자로 읽어도 바로잡음)
        if (!quantity) return std::nullopt;                                                      // 재료 툴팁이 아님

        std::optional<OCRTextParser::MaterialMatch> best;                                        // 가장 잘 맞는 제목 줄의 재료
        for (const TextLine& line : context.m_hoverText.m_lines)                                 // 줄마다
        {
            if (line.m_text.contains(QUANTITY_WORD)) continue;                                   // 수량 줄은 제목이 아님
            if (!line.m_text.contains(BOUND_MARK)) continue;                                     // 같은 줄에 귀속 표시가 있어야 함
            const auto match = OCRTextParser::MatchMaterial(line.m_text);                        // 재료 이름 맞추기
            if (match && (!best || match->m_distance < best->m_distance)) best = match;          // 더 잘 맞으면 교체
        }
        if (!best) return std::nullopt;                                                          // 알아볼 귀속 재료가 아님

        ScanResult result;                                                                       // 결과
        result.m_values.push_back(ScanValue{
            OCRTextParser::MakeMaterialValueKey(best->m_material->m_key, best->m_material->m_tier), *quantity });   // "Dest/top" = 수량
        return result;                                                                           // 툴팁 결과
    }
}
