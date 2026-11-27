module LostArkMultiTool.Vision.CombatAnalyzerDetector;  // 전투 분석기 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.BossHealthDetector;  // 흰 글자 흑백 그림 만들기 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.OCRTextParser;  // 띄어쓰기 지우기 · 편집 거리 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr double AREA_LEFT_RATIO{ 0.60 };       // 영역 왼쪽 (화면 가로 비율, 전투 분석기를 조금 옮겨도 들어오게)
        constexpr double AREA_TOP_RATIO{ 0.75 };        // 영역 위쪽
        constexpr int OCR_SCALE{ 2 };                   // 작은 글자라 2배
        constexpr std::wstring_view DAMAGE_HEADER{ L"피해량" };   // 피해량 머리글 (서포터는 "조력 피해량"도 같은 말이 들어감)
        constexpr std::wstring_view DPS_HEADER{ L"DPS" };         // DPS 머리글
        constexpr std::size_t HEADER_MAX_DISTANCE{ 1 }; // 머리글 한 글자 오인식 허용 (피하랑 · 피해랑)
        constexpr double VALUE_MAX_GAP_RATIO{ 3.0 };    // 값은 머리글 아래 이 높이 배수 안
        constexpr std::wstring_view HUNDRED_MILLION_MARKS{ L"억익역엌" };   // 억 (OCR이 비슷하게 읽는 글자 포함)
        constexpr std::wstring_view TEN_THOUSAND_MARKS{ L"만민맨" };        // 만
        constexpr double HUNDRED_MILLION{ 100'000'000.0 };   // 억
        constexpr double TEN_THOUSAND{ 10'000.0 };           // 만
        constexpr std::size_t DECIMAL_DIGITS{ 2 };      // 전투 분석기 소수 자리
        constexpr double DECIMAL_SCALE{ 100.0 };        // 소수 두 자리
        constexpr int DECIMAL_BASE{ 10 };               // 십진수
        constexpr int MAX_DIGITS{ 15 };                 // 숫자 자릿수 상한
        constexpr double HALF{ 0.5 };                   // 절반

        [[nodiscard]] double CenterX(const PixelRect& rect) noexcept { return rect.m_x + rect.m_width * HALF; }   // 가로 가운데
    }

    PixelRect CombatAnalyzerDetector::GetArea(int frameWidth, int frameHeight) noexcept
    {
        const int left = static_cast<int>(frameWidth * AREA_LEFT_RATIO);       // 왼쪽
        const int top = static_cast<int>(frameHeight * AREA_TOP_RATIO);        // 위쪽
        return ClampRect(PixelRect{ left, top, frameWidth - left, frameHeight - top }, frameWidth, frameHeight);   // 오른쪽 아래
    }

    std::optional<std::int64_t> CombatAnalyzerDetector::ParseAmount(std::wstring_view text)
    {
        double unit = 1.0;                                                      // 단위
        std::size_t end = text.size();                                          // 숫자 끝
        for (std::size_t index = 0; index < text.size(); ++index)               // 단위 글자 찾기
        {
            if (HUNDRED_MILLION_MARKS.find(text[index]) != std::wstring_view::npos) { unit = HUNDRED_MILLION; end = index; break; }   // 억
            if (TEN_THOUSAND_MARKS.find(text[index]) != std::wstring_view::npos) { unit = TEN_THOUSAND; end = index; break; }       // 만
        }
        std::wstring digits;                                                    // 숫자 · 구분자만
        std::optional<std::size_t> lastSeparator;                               // 마지막 구분자 위치 (digits 기준)
        for (std::size_t index = 0; index < end; ++index)                       // 숫자 부분
        {
            const wchar_t character = text[index];                              // 글자
            if (character >= L'0' && character <= L'9') digits += character;   // 숫자
            else if ((character == L',' || character == L'.') && !digits.empty()) lastSeparator = digits.size();   // 구분자 자리
        }
        if (digits.empty() || digits.size() > static_cast<std::size_t>(MAX_DIGITS)) return std::nullopt;   // 숫자 없음 · 너무 긺
        std::size_t decimals = 0;                                               // 소수 자릿수
        if (lastSeparator && digits.size() - *lastSeparator == DECIMAL_DIGITS && unit > 1.0) decimals = DECIMAL_DIGITS;   // "460,99억" → 소수
        double value = 0.0;                                                     // 값
        for (wchar_t character : digits) value = value * DECIMAL_BASE + (character - L'0');   // 숫자
        if (decimals > 0) value /= DECIMAL_SCALE;                               // 소수 두 자리
        return static_cast<std::int64_t>(std::llround(value * unit));           // 단위 곱하기
    }

    std::optional<CombatReading> CombatAnalyzerDetector::Parse(const TextResult& text)
    {
        CombatReading reading;                                                  // 결과
        const auto findValue = [&text](const TextLine& header) -> std::optional<std::int64_t>   // 머리글 바로 아래 값
            {
                const double headerX = CenterX(header.m_bounds);                // 머리글 가운데
                std::optional<std::int64_t> best;                               // 가장 가까운 값
                int bestGap = std::numeric_limits<int>::max();                  // 그 거리
                for (const TextLine& line : text.m_lines)                       // 줄마다
                {
                    const int gap = line.m_bounds.m_y - header.m_bounds.GetBottom();   // 아래로 거리
                    if (gap < 0 || gap > header.m_bounds.m_height * VALUE_MAX_GAP_RATIO || gap >= bestGap) continue;   // 바로 아래가 아님
                    if (std::abs(CenterX(line.m_bounds) - headerX) > std::max(header.m_bounds.m_width, line.m_bounds.m_width)) continue;   // 다른 칸
                    if (const auto value = ParseAmount(line.m_text)) { best = value; bestGap = gap; }   // 값
                }
                return best;
            };
        for (const TextLine& line : text.m_lines)                               // 머리글 찾기
        {
            const std::wstring compact = OCRTextParser::Compact(line.m_text);   // 띄어쓰기 없이
            if (!reading.m_dps && compact == DPS_HEADER) reading.m_dps = findValue(line);   // DPS
            else if (!reading.m_damage && compact.size() <= DAMAGE_HEADER.size() + HEADER_MAX_DISTANCE
                && OCRTextParser::FindDistance(DAMAGE_HEADER, compact) <= HEADER_MAX_DISTANCE) reading.m_damage = findValue(line);   // 피해량
        }
        if (!reading.m_damage && !reading.m_dps) return std::nullopt;           // 전투 분석기가 없거나 값이 "-"
        return reading;                                                         // 읽은 값
    }

    std::optional<CombatReading> CombatAnalyzerDetector::Detect(const FrameImage& frame, const TextRecognizer& text)
    {
        const PixelRect area = GetArea(frame.GetWidth(), frame.GetHeight());    // 오른쪽 아래
        if (area.m_width <= 0 || area.m_height <= 0) return std::nullopt;       // 너무 작은 프레임
        const auto result = text.Recognize(BossHealthDetector::MakeTextMask(frame.Crop(area)), OCR_SCALE);   // 흰 글자만 2배 OCR
        if (!result) return std::nullopt;                                       // OCR 실패
        return Parse(*result);                                                  // 해석
    }
}
