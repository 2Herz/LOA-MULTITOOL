module LostArkMultiTool.Vision.ShardBarDetector;  // 상단바 운명의 파편 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.OCRTextParser;  // OCR 글자 해석 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 인식기 인터페이스 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // OCR 결과 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr double TOP_BAR_HEIGHT_RATIO{ 0.06 };     // 화면 위에서 이 비율 안쪽만 재화 줄로 봄
        constexpr double MERGE_GAP_RATIO{ 1.2 };           // 숫자 단어 사이가 글자 높이 * 이 값 이하면 한 숫자 (쉼표를 띄어쓰기로 읽은 경우)
        constexpr int STRIP_OCR_SCALE{ 2 };                // 재화 줄 OCR 확대 배율 (작은 숫자)
        constexpr int MAX_DIGITS{ 12 };                    // 재화 숫자 최대 자릿수 (이보다 길면 오인식)
        constexpr int DECIMAL_BASE{ 10 };                  // 십진수
        constexpr int ICON_GAP{ 2 };                       // 숫자 오른쪽 끝과 아이콘 사이 여백
        constexpr double ICON_WIDTH_RATIO{ 1.5 };          // 아이콘 영역 가로 = 글자 높이 * 이 값
        constexpr int MIN_ICON_BRIGHTNESS{ 80 };           // 이보다 어두운 픽셀(바탕)은 색 평균에서 뺌
        constexpr int PURPLE_MARGIN{ 30 };                 // 빨강 · 파랑이 초록보다 이만큼 커야 보라색
        constexpr int MIN_ICON_PIXELS{ 8 };                // 색을 판단할 최소 픽셀 수
        constexpr int CHANNEL_BLUE{ 0 };                   // BGRA 파랑 위치
        constexpr int CHANNEL_GREEN{ 1 };                  // BGRA 초록 위치
        constexpr int CHANNEL_RED{ 2 };                    // BGRA 빨강 위치
        constexpr int BRIGHTNESS_CHANNELS{ 3 };            // 밝기 평균에 쓰는 채널 수
        constexpr std::string_view SHARD_KEY{ "Shard" };   // 계산기 운명의 파편 재료 키 (APPConstants::MAT_KEYS)

        // 숫자 단어인지 (숫자 · 쉼표 · 마침표만, 숫자 하나 이상)
        [[nodiscard]] bool IsNumberWord(std::wstring_view text) noexcept
        {
            bool hasDigit = false;                                            // 숫자가 있는지
            for (wchar_t value : text)                                        // 글자마다
            {
                if (value >= L'0' && value <= L'9') { hasDigit = true; continue; }   // 숫자
                if (value != L',' && value != L'.') return false;            // 다른 글자가 섞임
            }
            return hasDigit;                                                  // 숫자가 하나라도 있어야 함
        }

        // 덩어리 안 숫자만 이어 붙이기 ("24 615414" · "24,615 414" → 24615414, 재화 사이에는 아이콘이 있어 다른 재화와 붙지 않음)
        [[nodiscard]] std::optional<std::int64_t> ParseTokenDigits(std::wstring_view text) noexcept
        {
            std::int64_t value = 0;                                           // 결과
            int digitCount = 0;                                               // 자릿수
            for (wchar_t character : text)                                    // 글자마다
            {
                if (character < L'0' || character > L'9') continue;          // 숫자만
                if (++digitCount > MAX_DIGITS) return std::nullopt;          // 너무 긺 → 오인식
                value = value * DECIMAL_BASE + (character - L'0');           // 자릿수 추가
            }
            if (digitCount == 0) return std::nullopt;                         // 숫자가 없음
            return value;                                                     // 숫자
        }

        // 숫자 덩어리 (가까운 숫자 단어를 이은 것)
        struct NumberToken
        {
            std::wstring m_text;   // 이은 글자 (단어 사이 띄어쓰기)
            PixelRect m_bounds;    // 덩어리 위치 (프레임 기준)
        };

        // 두 사각형을 감싸는 사각형
        [[nodiscard]] PixelRect Unite(const PixelRect& a, const PixelRect& b) noexcept
        {
            const int left = std::min(a.m_x, b.m_x);                 // 왼쪽
            const int top = std::min(a.m_y, b.m_y);                  // 위쪽
            const int right = std::max(a.GetRight(), b.GetRight());  // 오른쪽
            const int bottom = std::max(a.GetBottom(), b.GetBottom());   // 아래쪽
            return PixelRect{ left, top, right - left, bottom - top };   // 합친 사각형
        }
    }

    std::string_view ShardBarDetector::GetScreenName() const noexcept
    {
        return SCREEN_NAME;   // "shard_bar"
    }

    PixelRect ShardBarDetector::GetBarArea(int frameWidth, int frameHeight) noexcept
    {
        return ClampRect(PixelRect{ 0, 0, frameWidth, static_cast<int>(frameHeight * TOP_BAR_HEIGHT_RATIO) }, frameWidth, frameHeight);   // 맨 위 재화 줄
    }

    bool ShardBarDetector::IsPurpleIcon(const FrameImage& frame, const PixelRect& numberBounds) noexcept
    {
        const int iconWidth = static_cast<int>(numberBounds.m_height * ICON_WIDTH_RATIO);             // 아이콘 가로
        const PixelRect icon = ClampRect(PixelRect{ numberBounds.GetRight() + ICON_GAP, numberBounds.m_y,
            iconWidth, numberBounds.m_height }, frame.GetWidth(), frame.GetHeight());                   // 숫자 오른쪽 아이콘 영역
        const auto pixels = frame.GetPixels();                                                         // 프레임 픽셀
        long long red = 0;                                                                             // 빨강 합
        long long green = 0;                                                                           // 초록 합
        long long blue = 0;                                                                            // 파랑 합
        int count = 0;                                                                                 // 센 픽셀 수
        for (int y = icon.m_y; y < icon.GetBottom(); ++y)                                              // 줄마다
        {
            for (int x = icon.m_x; x < icon.GetRight(); ++x)                                           // 칸마다
            {
                const std::size_t offset = (static_cast<std::size_t>(y) * frame.GetWidth() + x) * FrameImage::BYTES_PER_PIXEL;   // 픽셀 위치 (영역은 프레임 안으로 잘림)
                const int b = pixels[offset + CHANNEL_BLUE];                                           // 파랑
                const int g = pixels[offset + CHANNEL_GREEN];                                          // 초록
                const int r = pixels[offset + CHANNEL_RED];                                            // 빨강
                if ((r + g + b) / BRIGHTNESS_CHANNELS < MIN_ICON_BRIGHTNESS) continue;                 // 어두운 바탕은 뺌
                red += r;                                                                              // 합 누적
                green += g;
                blue += b;
                ++count;                                                                               // 수 증가
            }
        }
        if (count < MIN_ICON_PIXELS) return false;                                                     // 아이콘이 없음
        return red - green > static_cast<long long>(PURPLE_MARGIN) * count                             // 평균 빨강이 초록보다 크고
            && blue - green > static_cast<long long>(PURPLE_MARGIN) * count;                           // 평균 파랑도 초록보다 큼 → 보라색
    }

    std::optional<ScanResult> ShardBarDetector::Detect(const FrameImage& frame, const DetectorContext& context)
    {
        std::vector<NumberToken> words;                                                                // 재화 줄의 숫자 단어들
        const PixelRect strip = GetBarArea(frame.GetWidth(), frame.GetHeight());                       // 맨 위 재화 줄 (커서 위치와 상관없이)
        const auto stripText = context.m_text.Recognize(frame.Crop(strip), STRIP_OCR_SCALE);           // 작은 글자라 키워서 OCR
        if (!stripText) return std::nullopt;                                                           // OCR 실패
        for (const TextLine& line : stripText->m_lines)                                                // 줄마다
        {
            for (const TextWord& word : line.m_words)                                                  // 단어마다
            {
                if (!IsNumberWord(word.m_text)) continue;                                              // 숫자만
                words.push_back(NumberToken{ word.m_text, word.m_bounds });                            // 모음 (재화 줄은 프레임 왼쪽 위에서 시작 → 위치 그대로 프레임 기준)
            }
        }
        std::ranges::sort(words, {}, [](const NumberToken& token) { return token.m_bounds.m_x; });     // 왼쪽부터

        std::vector<NumberToken> tokens;                                                               // 이은 숫자 덩어리
        for (NumberToken& word : words)                                                                // 단어마다
        {
            const bool isNear = !tokens.empty()
                && word.m_bounds.m_x - tokens.back().m_bounds.GetRight() <= static_cast<int>(word.m_bounds.m_height * MERGE_GAP_RATIO)
                && std::abs(word.m_bounds.m_y - tokens.back().m_bounds.m_y) < word.m_bounds.m_height;  // 바로 옆 같은 높이
            if (isNear)                                                                                // 이어 붙임
            {
                tokens.back().m_text += L' ';                                                          // 띄어쓰기 (묶음 구분자)
                tokens.back().m_text += word.m_text;                                                   // 글자
                tokens.back().m_bounds = Unite(tokens.back().m_bounds, word.m_bounds);                 // 위치
            }
            else tokens.push_back(std::move(word));                                                    // 새 덩어리
        }

        for (const NumberToken& token : tokens)                                                        // 덩어리마다
        {
            if (!IsPurpleIcon(frame, token.m_bounds)) continue;                                        // 파편 아이콘이 아님
            const auto value = ParseTokenDigits(token.m_text);                                        // 숫자 (OCR이 쉼표 자리를 엉뚱하게 끊어도 숫자만 이음)
            if (!value) continue;                                                                      // 숫자가 없거나 너무 긺
            ScanResult result;                                                                         // 결과
            result.m_values.push_back(ScanValue{
                OCRTextParser::MakeMaterialValueKey(SHARD_KEY, OCRTextParser::MaterialTierHint::TIER_BY_LEVEL), *value });   // "Shard/level"
            return result;                                                                             // 파편 결과
        }
        return std::nullopt;   // 재화 줄이 없거나 파편 숫자가 없음
    }
}
