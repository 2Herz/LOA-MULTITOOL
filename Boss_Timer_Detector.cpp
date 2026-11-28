module LostArkMultiTool.Vision.BossTimerDetector;  // 보스 남은 시간 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.OCRTextParser;  // 글자 비교 도우미 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr double ENRAGE_LEFT_RATIO{ 0.24 };       // 광폭화 영역 왼쪽 (프레임 가로 비율)
        constexpr double ENRAGE_RIGHT_RATIO{ 0.35 };      // 오른쪽
        constexpr double ENRAGE_TOP_RATIO{ 0.07 };        // 위쪽 (세로 비율)
        constexpr double ENRAGE_BOTTOM_RATIO{ 0.17 };     // 아래쪽
        constexpr double GUARDIAN_LEFT_RATIO{ 0.0 };      // 가디언 토벌 영역 왼쪽
        constexpr double GUARDIAN_RIGHT_RATIO{ 0.20 };    // 오른쪽
        constexpr double GUARDIAN_TOP_RATIO{ 0.05 };      // 위쪽
        constexpr double GUARDIAN_BOTTOM_RATIO{ 0.12 };   // 아래쪽
        constexpr int OCR_SCALE{ 2 };                     // 시계 글자 OCR 배율 (1배는 자주 못 읽음)
        constexpr int MASK_PADDING{ 20 };                 // 흑백 그림 둘레 여백
        constexpr int WHITE_MIN{ 150 };                   // 흰 글자: 세 채널 모두 이 이상
        constexpr int YELLOW_RED_MIN{ 170 };              // 노란 · 주황 글자: R 하한
        constexpr int YELLOW_GREEN_MIN{ 120 };            // G 하한
        constexpr std::uint8_t INK{ 0 };                  // 글자 = 검정
        constexpr std::uint8_t PAPER{ 255 };              // 바탕 = 흰색
        constexpr int CHANNEL_BLUE{ 0 };                  // BGRA 파랑
        constexpr int CHANNEL_GREEN{ 1 };                 // BGRA 초록
        constexpr int CHANNEL_RED{ 2 };                   // BGRA 빨강
        constexpr int CHANNEL_ALPHA{ 3 };                 // BGRA 알파
        constexpr int SECONDS_PER_MINUTE{ 60 };           // 분 → 초
        constexpr int SECONDS_PER_HOUR{ 3600 };           // 시 → 초
        constexpr int MAX_FIELD{ 59 };                    // 분 · 초 상한
        constexpr int MAX_HOURS{ 9 };                     // 시 상한 (광폭화는 1시간 안쪽)
        constexpr std::size_t FIELD_DIGITS{ 2 };          // 한 칸 숫자 수
        constexpr std::size_t MAX_NAME_DISTANCE{ 2 };     // 보스 이름 오인식 허용 (편집 거리)
        constexpr double NAME_TOP_FACTOR{ 1.8 };          // 이름 줄 위쪽 = 체력 숫자 줄 위 - 높이 × 1.8
        constexpr double NAME_BOTTOM_FACTOR{ 0.2 };       // 이름 줄 아래쪽 = 체력 숫자 줄 위 - 높이 × 0.2
        constexpr std::wstring_view ENRAGE_KEYWORD{ L"광폭" };   // 레이드 타이머 머리말
        constexpr std::wstring_view CLOCK_SEPARATORS{ L":;" };   // 시계 구분자 (OCR이 ;로 읽기도 함)

        // 관문별 광폭화 시간 (나무위키: 지평의 성당 · 그림자 레이드/세르카 · 그림자 레이드/벨가르딘, 띄어쓰기 · 쉼표 없는 이름)
        struct EnrageEntry { std::wstring_view m_name; int m_minutes{}; };
        constexpr std::array<EnrageEntry, 6> ENRAGE_TABLE{ {
            { L"대주교아르세노스", 10 },          // 지평의 성당 1관문
            { L"광신의인도자아르세노스", 10 },    // 지평의 성당 2관문
            { L"고통의마녀세르카", 10 },          // 세르카 1관문
            { L"코르부스툴라크", 10 },            // 세르카 2관문
            { L"죽음의계율자벨가르딘", 10 },      // 벨가르딘 1관문
            { L"페투스안크라그마", 13 },          // 벨가르딘 2관문
        } };

        // 글자만 남긴 흑백 그림 (includeYellow: 노란 · 주황 글자도)
        [[nodiscard]] FrameImage MakeMask(const FrameImage& image, bool includeYellow)
        {
            const int width = image.GetWidth() + MASK_PADDING * 2;                   // 여백 포함
            const int height = image.GetHeight() + MASK_PADDING * 2;
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * FrameImage::BYTES_PER_PIXEL, PAPER);   // 흰 바탕
            const auto source = image.GetPixels();                                    // 원래 픽셀
            for (int y = 0; y < image.GetHeight(); ++y)
                for (int x = 0; x < image.GetWidth(); ++x)
                {
                    const std::size_t from = (static_cast<std::size_t>(y) * image.GetWidth() + x) * FrameImage::BYTES_PER_PIXEL;
                    const int red = source[from + CHANNEL_RED];                       // 빨강
                    const int green = source[from + CHANNEL_GREEN];                   // 초록
                    const int blue = source[from + CHANNEL_BLUE];                     // 파랑
                    const bool isWhite = std::min({ red, green, blue }) >= WHITE_MIN; // 흰 글자
                    const bool isYellow = includeYellow && red >= YELLOW_RED_MIN && green >= YELLOW_GREEN_MIN;   // 노란 글자
                    if (!isWhite && !isYellow) continue;                              // 바탕
                    const std::size_t to = (static_cast<std::size_t>(y + MASK_PADDING) * width + x + MASK_PADDING) * FrameImage::BYTES_PER_PIXEL;
                    pixels[to + CHANNEL_BLUE] = INK;                                  // 검정
                    pixels[to + CHANNEL_GREEN] = INK;
                    pixels[to + CHANNEL_RED] = INK;
                }
            for (std::size_t index = CHANNEL_ALPHA; index < pixels.size(); index += FrameImage::BYTES_PER_PIXEL) pixels[index] = PAPER;   // 불투명
            return FrameImage::FromRows(width, height, pixels, width * FrameImage::BYTES_PER_PIXEL);
        }

        // 영역을 OCR해서 시계 글자 찾기 (keyword가 있으면 그 글자가 보인 결과만)
        [[nodiscard]] std::optional<int> ReadClock(const FrameImage& frame, const TextRecognizer& text, const PixelRect& area, bool includeYellow, std::wstring_view keyword)
        {
            const PixelRect clamped = ClampRect(area, frame.GetWidth(), frame.GetHeight());   // 프레임 안
            if (clamped.IsEmpty()) return std::nullopt;                               // 빈 영역
            const auto result = text.Recognize(MakeMask(frame.Crop(clamped), includeYellow), OCR_SCALE);   // 2배 OCR
            if (!result) return std::nullopt;                                         // OCR 실패
            if (!keyword.empty())                                                     // 머리말이 보여야 함
            {
                const bool hasKeyword = std::ranges::any_of(result->m_lines, [keyword](const TextLine& line) { return OCRTextParser::Compact(line.m_text).find(keyword) != std::wstring::npos; });
                if (!hasKeyword) return std::nullopt;                                 // 다른 글자
            }
            for (const TextLine& line : result->m_lines)                              // 줄마다
                if (const auto seconds = BossTimerDetector::ParseClock(line.m_text)) return seconds;   // 시계
            return std::nullopt;                                                      // 시계 없음
        }
    }

    PixelRect BossTimerDetector::GetEnrageArea(int frameWidth, int frameHeight) noexcept
    {
        const int left = static_cast<int>(frameWidth * ENRAGE_LEFT_RATIO);            // 왼쪽
        const int top = static_cast<int>(frameHeight * ENRAGE_TOP_RATIO);             // 위
        return PixelRect{ left, top, static_cast<int>(frameWidth * ENRAGE_RIGHT_RATIO) - left, static_cast<int>(frameHeight * ENRAGE_BOTTOM_RATIO) - top };
    }

    PixelRect BossTimerDetector::GetGuardianArea(int frameWidth, int frameHeight) noexcept
    {
        const int left = static_cast<int>(frameWidth * GUARDIAN_LEFT_RATIO);          // 왼쪽
        const int top = static_cast<int>(frameHeight * GUARDIAN_TOP_RATIO);           // 위
        return PixelRect{ left, top, static_cast<int>(frameWidth * GUARDIAN_RIGHT_RATIO) - left, static_cast<int>(frameHeight * GUARDIAN_BOTTOM_RATIO) - top };
    }

    std::optional<int> BossTimerDetector::ParseClock(std::wstring_view text)
    {
        // 구분자(: ;)로 나눈 칸마다 숫자만 모음 ("19 : 4-7" → 19 · 47)
        std::vector<std::wstring> fields(1);                                          // 칸들
        for (const wchar_t character : text)
        {
            if (CLOCK_SEPARATORS.find(character) != std::wstring_view::npos) fields.emplace_back();   // 다음 칸
            else if (character >= L'0' && character <= L'9') fields.back() += character;   // 숫자
        }
        if (fields.size() < 2 || fields.size() > 3) return std::nullopt;              // 시계 아님 (분:초 · 시:분:초)
        // 첫 칸은 앞쪽 잡음 숫자가 붙을 수 있어 뒤 두 자리만, 마지막 칸은 앞 두 자리만
        std::wstring& first = fields.front();
        if (first.size() > FIELD_DIGITS) first = first.substr(first.size() - FIELD_DIGITS);
        std::wstring& last = fields.back();
        if (last.size() > FIELD_DIGITS) last = last.substr(0, FIELD_DIGITS);
        std::array<int, 3> values{};                                                  // 시 · 분 · 초 (오른쪽 정렬)
        const std::size_t offset = values.size() - fields.size();                     // 칸 수 차이
        for (std::size_t index = 0; index < fields.size(); ++index)
        {
            const std::wstring& field = fields[index];                                // 칸
            if (field.empty() || field.size() > FIELD_DIGITS) return std::nullopt;     // 숫자 없음 · 너무 긺
            values[offset + index] = std::stoi(field);                                // 값
        }
        if (values[1] > MAX_FIELD || values[2] > MAX_FIELD || values[0] > MAX_HOURS) return std::nullopt;   // 시계 범위 밖
        if (fields.size() == 2 && fields[1].size() != FIELD_DIGITS) return std::nullopt;   // 초는 두 자리
        return values[0] * SECONDS_PER_HOUR + values[1] * SECONDS_PER_MINUTE + values[2];   // 초
    }

    std::optional<int> BossTimerDetector::FindEnrageSeconds(std::wstring_view bossName)
    {
        std::wstring compact;                                                         // 띄어쓰기 · 쉼표 · 마침표 뺀 이름
        for (const wchar_t character : bossName)
            if (!std::iswspace(character) && character != L',' && character != L'.') compact += character;
        if (compact.empty()) return std::nullopt;                                     // 이름 없음
        std::optional<int> best;                                                      // 가장 잘 맞는 관문
        std::size_t bestDistance = MAX_NAME_DISTANCE + 1;                             // 그 거리
        for (const EnrageEntry& entry : ENRAGE_TABLE)                                 // 표마다
        {
            const std::size_t distance = OCRTextParser::FindDistance(entry.m_name, compact);   // 편집 거리
            if (distance < bestDistance) { bestDistance = distance; best = entry.m_minutes * SECONDS_PER_MINUTE; }
        }
        return best;                                                                  // 광폭화 시간 (초)
    }

    std::optional<BossTimerReading> BossTimerDetector::Detect(const FrameImage& frame, const TextRecognizer& text)
    {
        if (const auto enrage = ReadClock(frame, text, GetEnrageArea(frame.GetWidth(), frame.GetHeight()), true, ENRAGE_KEYWORD))   // 레이드 광폭화
            return BossTimerReading{ *enrage, true };
        if (const auto guardian = ReadClock(frame, text, GetGuardianArea(frame.GetWidth(), frame.GetHeight()), false, {}))        // 가디언 토벌
            return BossTimerReading{ *guardian, false };
        return std::nullopt;                                                          // 타이머 없음
    }

    std::optional<int> BossTimerDetector::ReadEnrageFromName(const FrameImage& frame, const TextRecognizer& text, const PixelRect& textBounds)
    {
        const int top = textBounds.m_y - static_cast<int>(textBounds.m_height * NAME_TOP_FACTOR);         // 이름 줄 위
        const int bottom = textBounds.m_y - static_cast<int>(textBounds.m_height * NAME_BOTTOM_FACTOR);   // 이름 줄 아래
        const PixelRect area = ClampRect(PixelRect{ textBounds.m_x, top, textBounds.m_width, bottom - top }, frame.GetWidth(), frame.GetHeight());   // 체력 숫자 위 띠
        if (area.IsEmpty()) return std::nullopt;                                      // 화면 밖
        const auto result = text.Recognize(frame.Crop(area), OCR_SCALE);              // 2배 OCR (빨간 · 흰 이름 그대로)
        if (!result) return std::nullopt;                                             // OCR 실패
        for (const TextLine& line : result->m_lines)                                  // 줄마다
            if (const auto seconds = FindEnrageSeconds(line.m_text)) return seconds;  // 표에 있는 보스
        return std::nullopt;                                                          // 모르는 보스
    }
}
