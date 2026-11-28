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
        constexpr std::size_t DECIMAL_DIGITS{ 2 };      // 전투 분석기 소수 자리 (구분자 뒤 1 ~ 2자리면 소수, 3자리는 천 단위 구분자)
        constexpr int DECIMAL_BASE{ 10 };               // 십진수
        constexpr int MAX_DIGITS{ 15 };                 // 숫자 자릿수 상한
        constexpr double HALF{ 0.5 };                   // 절반
        constexpr int FIND_SCALE{ 1 };                  // 머리글 찾기는 1배 (빠름)
        constexpr int STRIP_ABOVE_RATIO{ 1 };           // 띠 위쪽 = 머리글 위 - 머리글 높이 × 1
        constexpr int STRIP_BELOW_RATIO{ 4 };           // 띠 아래쪽 = 머리글 아래 + 머리글 높이 × 4 (값 줄까지)

        // 머리글 줄인지 ("DPS" · "피해량")
        [[nodiscard]] bool IsHeader(const TextLine& line)
        {
            const std::wstring compact = OCRTextParser::Compact(line.m_text);   // 띄어쓰기 없이
            if (compact == DPS_HEADER) return true;                             // DPS
            return compact.size() <= DAMAGE_HEADER.size() + HEADER_MAX_DISTANCE && OCRTextParser::FindDistance(DAMAGE_HEADER, compact) <= HEADER_MAX_DISTANCE;   // 피해량
        }

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
            else if ((character == L',' || character == L'.' || character == L' ') && !digits.empty()) lastSeparator = digits.size();   // 구분자 자리 (작은 점이 띄어쓰기로 읽힌 "731 05억" 포함)
        }
        if (digits.empty() || digits.size() > static_cast<std::size_t>(MAX_DIGITS)) return std::nullopt;   // 숫자 없음 · 너무 긺
        std::size_t decimals = 0;                                               // 소수 자릿수
        if (unit > 1.0)                                                         // 억 · 만이 붙은 값 (전투 분석기는 항상 소수 둘째 자리까지 보여 줌)
        {
            const std::size_t tail = lastSeparator ? digits.size() - *lastSeparator : 0;   // 마지막 구분자 뒤 자릿수
            if (tail >= 1 && tail <= DECIMAL_DIGITS) decimals = tail;           // "460,99억" · OCR이 한 자리를 빠뜨린 "5,943.9만" → 소수 (10배로 읽히지 않게)
            else if (digits.size() > DECIMAL_DIGITS) decimals = DECIMAL_DIGITS; // 소수점을 놓친 "73105억" → 731.05억 (100배인 7.31조로 읽히지 않게)
        }
        double value = 0.0;                                                     // 값
        for (wchar_t character : digits) value = value * DECIMAL_BASE + (character - L'0');   // 숫자
        for (std::size_t digit = 0; digit < decimals; ++digit) value /= DECIMAL_BASE;   // 소수 자릿수만큼 나눔
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

    std::optional<CombatReading> CombatAnalyzerDetector::Detect(const FrameImage& frame, const TextRecognizer& text, std::optional<PixelRect>* stripCache)
    {
        const PixelRect area = GetArea(frame.GetWidth(), frame.GetHeight());    // 오른쪽 아래
        if (area.m_width <= 0 || area.m_height <= 0) return std::nullopt;       // 너무 작은 프레임
        struct StripRead { bool m_hasHeader{}; std::optional<CombatReading> m_reading; };   // 띠 읽은 결과 (머리글이 보였는지 · 값)
        const auto readStrip = [&](const PixelRect& strip) -> StripRead       // 띠만 2배로 읽기
            {
                const PixelRect clamped = ClampRect(strip, frame.GetWidth(), frame.GetHeight());   // 프레임 안
                if (clamped.IsEmpty()) return {};                               // 빈 띠
                const auto result = text.Recognize(BossHealthDetector::MakeTextMask(frame.Crop(clamped)), OCR_SCALE);   // 흰 글자만 2배
                if (!result) return {};                                         // OCR 실패
                const bool hasHeader = std::ranges::any_of(result->m_lines, [](const TextLine& line) { return IsHeader(line); });   // 전투 분석기가 그 자리에 있음
                return StripRead{ hasHeader, Parse(*result) };                  // 해석
            };

        // 1) 지난번에 찾은 띠 (전투 분석기는 잘 안 움직임)
        if (stripCache && *stripCache)
        {
            StripRead cached = readStrip(**stripCache);                         // 그 띠
            if (cached.m_reading || cached.m_hasHeader) return cached.m_reading; // 값 · 또는 머리글만 (값이 "-" = 전투 전)
            stripCache->reset();                                                // 옮겼거나 닫음 → 다시 찾기
        }

        // 2) 1배로 머리글 줄 찾기 → 그 띠만 2배
        const FrameImage mask = BossHealthDetector::MakeTextMask(frame.Crop(area));   // 흰 글자만
        if (const auto found = text.Recognize(mask, FIND_SCALE))                // 1배 OCR (빠름)
        {
            for (const TextLine& line : found->m_lines)                         // 줄마다
            {
                if (!IsHeader(line)) continue;                                  // 머리글 아님
                const int height = std::max(line.m_bounds.m_height, 1);         // 머리글 높이
                const int top = area.m_y + line.m_bounds.m_y - BossHealthDetector::MASK_PADDING - height * STRIP_ABOVE_RATIO;   // 띠 위 (프레임 기준)
                const int bottom = area.m_y + line.m_bounds.GetBottom() - BossHealthDetector::MASK_PADDING + height * STRIP_BELOW_RATIO;   // 띠 아래
                const PixelRect strip{ area.m_x, top, area.m_width, bottom - top };   // 영역 가로 전체 (다른 칸 머리글도 들어오게)
                StripRead found_strip = readStrip(strip);                       // 띠 읽기
                if (found_strip.m_reading || found_strip.m_hasHeader)           // 값 · 또는 머리글 (값이 "-")
                {
                    if (stripCache) *stripCache = strip;                        // 기억
                    return found_strip.m_reading;
                }
                break;                                                          // 한 번만 (아래 넓은 영역으로)
            }
        }

        // 3) 못 찾음 → 넓은 영역 2배 (예전 방식)
        const auto result = text.Recognize(mask, OCR_SCALE);                    // 흰 글자만 2배 OCR
        if (!result) return std::nullopt;                                       // OCR 실패
        return Parse(*result);                                                  // 해석
    }
}
