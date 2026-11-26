module LostArkMultiTool.Vision.BossHealthDetector;  // 보스 체력바 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr double AREA_LEFT_RATIO{ 0.27 };       // 체력바 영역 왼쪽 (화면 가로 비율)
        constexpr double AREA_RIGHT_RATIO{ 0.69 };      // 체력바 영역 오른쪽
        constexpr double AREA_TOP_RATIO{ 0.03 };        // 체력바 영역 위쪽 (창 제목줄이 있어도 · 없어도 들어오게 넉넉히)
        constexpr double AREA_BOTTOM_RATIO{ 0.15 };     // 체력바 영역 아래쪽
        constexpr int MIN_TEXT_CHANNEL{ 150 };          // 세 채널이 모두 이 이상이면 흰 글자 (막대 색은 파랑 · 초록이 낮음)
        constexpr int MASK_PADDING{ 20 };               // 흑백 그림 둘레 여백 (OCR이 가장자리 글자를 잘 읽게)
        constexpr std::uint8_t TEXT_VALUE{ 0 };         // 글자 = 검정
        constexpr std::uint8_t BACK_VALUE{ 255 };       // 바탕 = 흰색
        constexpr int ALPHA_CHANNEL{ 3 };               // BGRA 알파 위치
        constexpr int COLOR_CHANNELS{ 3 };              // B · G · R
        constexpr int MAX_DIGITS{ 18 };                 // 체력 숫자 최대 자릿수 (넘으면 오인식)
        constexpr int DECIMAL_BASE{ 10 };               // 십진수
        constexpr int MIN_HEALTH_DIGITS{ 4 };           // 최대 체력 최소 자릿수 (짧으면 다른 숫자)
        constexpr int MAX_LINES{ 999 };                 // 줄 수 상한
        constexpr std::array<std::wstring_view, 4> LINE_MARKS{ L")(", L"X", L"x", L"×" };   // 줄 수 앞 표시 (OCR이 X를 ")("로 읽기도 함)
        constexpr std::wstring_view ZERO_LOOKALIKES{ L"이OoD" };   // OCR이 이 숫자 글꼴의 0을 이렇게 읽기도 함
        constexpr double LINES_LEFT_RATIO{ 0.585 };     // 줄 수 영역 왼쪽 (화면 가로 비율, 체력바 오른쪽 끝)
        constexpr double LINES_RIGHT_RATIO{ 0.645 };    // 줄 수 영역 오른쪽
        constexpr double LINES_HEIGHT_FACTOR{ 1.0 };    // 체력 글자 줄 위아래로 더 볼 높이 (줄 높이 배수)
        constexpr std::array<int, 3> LINES_OCR_SCALES{ 2, 3, 4 };   // 줄 수는 작아서 키워 읽음 (작은 그림은 배율에 따라 읽히다 말다 해서 차례로 시도)
    }

    PixelRect BossHealthDetector::GetBarArea(int frameWidth, int frameHeight) noexcept
    {
        const int left = static_cast<int>(frameWidth * AREA_LEFT_RATIO);       // 왼쪽
        const int top = static_cast<int>(frameHeight * AREA_TOP_RATIO);        // 위쪽
        return ClampRect(PixelRect{ left, top, static_cast<int>(frameWidth * AREA_RIGHT_RATIO) - left, static_cast<int>(frameHeight * AREA_BOTTOM_RATIO) - top },
            frameWidth, frameHeight);                                          // 프레임 안으로
    }

    FrameImage BossHealthDetector::MakeTextMask(const FrameImage& image)
    {
        const int width = image.GetWidth() + MASK_PADDING * 2;                  // 여백 포함 가로
        const int height = image.GetHeight() + MASK_PADDING * 2;                // 여백 포함 세로
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * FrameImage::BYTES_PER_PIXEL, BACK_VALUE);   // 흰 바탕 (알파도 255)
        const auto source = image.GetPixels();                                  // 원본 픽셀
        for (int y = 0; y < image.GetHeight(); ++y)                             // 줄마다
        {
            for (int x = 0; x < image.GetWidth(); ++x)                          // 칸마다
            {
                const std::size_t from = (static_cast<std::size_t>(y) * image.GetWidth() + x) * FrameImage::BYTES_PER_PIXEL;   // 원본 위치
                bool isText = true;                                             // 세 채널이 모두 밝은지
                for (int channel = 0; channel < COLOR_CHANNELS; ++channel) isText = isText && source[from + channel] >= MIN_TEXT_CHANNEL;
                if (!isText) continue;                                          // 바탕
                const std::size_t to = (static_cast<std::size_t>(y + MASK_PADDING) * width + x + MASK_PADDING) * FrameImage::BYTES_PER_PIXEL;   // 새 위치
                for (int channel = 0; channel < COLOR_CHANNELS; ++channel) pixels[to + channel] = TEXT_VALUE;   // 검은 글자
                pixels[to + ALPHA_CHANNEL] = BACK_VALUE;                        // 불투명
            }
        }
        return FrameImage{ width, height, std::move(pixels) };                  // 흑백 그림
    }

    std::optional<std::int64_t> BossHealthDetector::ParseDigits(std::wstring_view text) noexcept
    {
        std::int64_t value = 0;                                                 // 결과
        int digits = 0;                                                         // 자릿수
        for (wchar_t character : text)                                          // 글자마다
        {
            int digit = -1;                                                     // 이 글자의 숫자 (아니면 -1)
            if (character >= L'0' && character <= L'9') digit = character - L'0';   // 숫자
            else if (ZERO_LOOKALIKES.find(character) != std::wstring_view::npos) digit = 0;   // 0을 잘못 읽은 글자
            if (digit < 0) continue;                                            // 숫자만 (쉼표 · 마침표 · 띄어쓰기 무시)
            if (++digits > MAX_DIGITS) return std::nullopt;                     // 너무 긺
            value = value * DECIMAL_BASE + digit;                               // 자릿수 추가
        }
        if (digits == 0) return std::nullopt;                                   // 숫자 없음
        return value;                                                           // 숫자
    }

    std::optional<BossHealthReading> BossHealthDetector::Parse(const TextResult& text)
    {
        std::optional<BossHealthReading> best;                                  // 결과
        std::optional<int> lines;                                               // 줄 수 (다른 줄에 있을 수 있음)
        for (const TextLine& line : text.m_lines)                               // 줄마다
        {
            std::wstring_view content = line.m_text;                            // 줄 글자
            for (std::wstring_view mark : LINE_MARKS)                           // "X 100"
            {
                const std::size_t markAt = content.rfind(mark);                 // 마지막 X
                if (markAt == std::wstring_view::npos) continue;                // 없음
                const auto count = ParseDigits(content.substr(markAt + mark.size()));   // 뒤 숫자
                if (count && *count > 0 && *count <= MAX_LINES) { lines = static_cast<int>(*count); content = content.substr(0, markAt); }   // 줄 수 · 앞부분만 남김
                break;                                                          // 표시 하나만
            }
            const std::size_t slash = content.find(L'/');                       // 현재 / 최대
            if (slash == std::wstring_view::npos) continue;                     // 체력 줄이 아님
            const auto current = ParseDigits(content.substr(0, slash));         // 현재
            const auto maximum = ParseDigits(content.substr(slash + 1));        // 최대
            if (!current || !maximum || *maximum <= 0 || *current > *maximum) continue;   // 앞뒤가 안 맞음 (오인식)
            if (std::to_string(*maximum).size() < static_cast<std::size_t>(MIN_HEALTH_DIGITS)) continue;   // 너무 작은 숫자 (다른 글자)
            best = BossHealthReading{ *current, *maximum, std::nullopt, line.m_bounds };   // 체력 (글자 줄 위치는 OCR 그림 기준)
        }
        if (best) best->m_lines = lines;                                        // 줄 수 붙이기
        return best;                                                            // 결과 (없으면 nullopt)
    }

    std::optional<BossHealthReading> BossHealthDetector::Detect(const FrameImage& frame, const TextRecognizer& text)
    {
        const PixelRect area = GetBarArea(frame.GetWidth(), frame.GetHeight()); // 위쪽 가운데
        if (area.m_width <= 0 || area.m_height <= 0) return std::nullopt;       // 너무 작은 프레임
        const auto result = text.Recognize(MakeTextMask(frame.Crop(area)));     // 흰 글자만 OCR (원래 크기가 가장 잘 읽힘)
        if (!result) return std::nullopt;                                       // OCR 실패
        auto reading = Parse(*result);                                          // 해석
        if (!reading) return std::nullopt;                                      // 체력 줄 없음
        if (reading->m_textBounds)                                              // 글자 줄 위치 → 프레임 기준으로
        {
            reading->m_textBounds->m_x += area.m_x - MASK_PADDING;
            reading->m_textBounds->m_y += area.m_y - MASK_PADDING;
        }
        if (!reading->m_lines && reading->m_textBounds)                         // 줄 수를 못 읽음 → 오른쪽만 3배로 다시
        {
            const PixelRect& textLine = *reading->m_textBounds;                 // 체력 글자 줄
            const int extra = static_cast<int>(textLine.m_height * LINES_HEIGHT_FACTOR);   // 위아래 여유
            const int left = static_cast<int>(frame.GetWidth() * LINES_LEFT_RATIO);       // 왼쪽
            const PixelRect linesArea = ClampRect(PixelRect{ left, textLine.m_y - extra,
                static_cast<int>(frame.GetWidth() * LINES_RIGHT_RATIO) - left, textLine.m_height + extra * 2 }, frame.GetWidth(), frame.GetHeight());   // 줄 수 영역
            const FrameImage linesMask = linesArea.m_width > 0 && linesArea.m_height > 0 ? MakeTextMask(frame.Crop(linesArea)) : FrameImage{};   // 흰 글자만
            for (int scale : LINES_OCR_SCALES)                                  // 배율마다 (읽히면 멈춤)
            {
                if (linesMask.IsEmpty() || reading->m_lines) break;             // 영역 없음 · 이미 읽음
                if (const auto linesText = text.Recognize(linesMask, scale))     // 키워서 OCR
                {
                    for (const TextLine& line : linesText->m_lines)             // 줄마다
                    {
                        for (std::wstring_view mark : LINE_MARKS)               // X 표시
                        {
                            const std::size_t markAt = line.m_text.find(mark);  // 표시 위치
                            if (markAt == std::wstring::npos) continue;          // 없음
                            const auto count = ParseDigits(std::wstring_view{ line.m_text }.substr(markAt + mark.size()));   // 뒤 숫자
                            if (count && *count > 0 && *count <= MAX_LINES) reading->m_lines = static_cast<int>(*count);    // 줄 수
                            break;                                              // 표시 하나만
                        }
                        if (reading->m_lines) break;                            // 찾음
                    }
                }
            }
        }
        return reading;                                                         // 체력 (+ 줄 수)
    }

    std::optional<BossHealthReading> BossHealthTracker::Feed(BossHealthReading reading)
    {
        m_maxima.push_back(reading.m_maximum);                                  // 최대 체력 기록
        while (m_maxima.size() > VOTE_WINDOW) m_maxima.pop_front();             // 최근 것만
        std::optional<std::int64_t> best;                                       // 표를 가장 많이 받은 값
        int bestVotes = 0;                                                      // 그 표 수
        for (auto candidate = m_maxima.rbegin(); candidate != m_maxima.rend(); ++candidate)   // 최근 값부터
        {
            const int votes = static_cast<int>(std::ranges::count(m_maxima, *candidate));      // 표 수
            if (votes > bestVotes) { bestVotes = votes; best = *candidate; }                    // 더 많음
        }
        const int confirmedVotes = m_confirmed ? static_cast<int>(std::ranges::count(m_maxima, *m_confirmed)) : 0;   // 지금 정한 값의 표 수
        if (best && bestVotes >= MIN_VOTES && bestVotes > confirmedVotes) m_confirmed = best;   // 더 많이 받은 값이 있을 때만 바꿈 (같으면 그대로 → 오인식이 두 번 나와도 안 흔들림)
        if (!m_confirmed) return std::nullopt;                                  // 아직 정한 값 없음 (두 번 같게 읽어야 함)
        reading.m_maximum = *m_confirmed;                                       // 최대 체력 고치기 (한 장 오인식)
        if (reading.m_current > reading.m_maximum) return std::nullopt;         // 현재 체력 자릿수 오인식
        if (reading.m_lines) m_lastLines = reading.m_lines;                     // 줄 수 기록
        else reading.m_lines = m_lastLines;                                     // 못 읽었으면 지난 줄 수
        return reading;                                                         // 믿을 만한 값
    }

    void BossHealthTracker::Reset() noexcept
    {
        m_maxima.clear();        // 투표 초기화
        m_confirmed.reset();     // 정한 값 초기화
        m_lastLines.reset();     // 줄 수 초기화
    }
}
