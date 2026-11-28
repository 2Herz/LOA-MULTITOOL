module LostArkMultiTool.Vision.BossTimerDetector;  // 보스 남은 시간 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.BossHealthDetector;  // 보스 체력바 영역 가져오기
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
        constexpr int NAME_RED_MIN{ 150 };                // 보스 이름 빨간 글자: R 하한
        constexpr int NAME_RED_OVER_GREEN{ 50 };          // R - G 하한
        constexpr int NAME_RED_OVER_BLUE{ 40 };           // R - B 하한
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
        constexpr double NAME_TOP_FACTOR{ 3.5 };          // 이름 줄 위쪽 = 체력 숫자 줄 위 - 높이 × 3.5 (빨간 보스 이름은 막대 위)
        constexpr double NAME_BOTTOM_FACTOR{ 0.3 };       // 이름 줄 아래쪽 = 체력 숫자 줄 위 - 높이 × 0.3
        constexpr double NAME_SIDE_FACTOR{ 0.4 };         // 이름 줄 좌우 여유 (체력 숫자 줄 너비 배수, 긴 이름)
        constexpr double STAGE_LEFT_RATIO{ 0.0 };         // 가디언 토벌 퀘스트 칸 왼쪽 (프레임 가로 비율)
        constexpr double STAGE_RIGHT_RATIO{ 0.20 };       // 오른쪽
        constexpr double STAGE_TOP_RATIO{ 0.13 };         // 위쪽 (창 제목줄이 있어도 · 없어도)
        constexpr double STAGE_BOTTOM_RATIO{ 0.26 };      // 아래쪽
        constexpr int RAID_SIZE_FOUR{ 4 };                // 4인
        constexpr int RAID_SIZE_EIGHT{ 8 };               // 8인
        constexpr std::size_t MIN_NAME_LENGTH{ 4 };       // 보스 이름으로 볼 글자 수 하한
        constexpr std::size_t NAME_DISTANCE_DIVISOR{ 3 }; // 인원용 보스 이름 허용 거리 = (이름 길이 + 1) / 3 (최소 2, 8 ~ 10글자는 3)
        constexpr double NAME_LEFT_RATIO{ 0.38 };         // 보스 이름(체력바 위 빨간 글씨) 영역 왼쪽 (프레임 가로 비율, 레이드마다 같은 자리 · 가운데 정렬)
        constexpr double NAME_RIGHT_RATIO{ 0.58 };        // 오른쪽 (긴 이름까지)
        constexpr double NAME_AREA_TOP_RATIO{ 0.015 };    // 위쪽 (창 제목줄이 있어도 · 없어도)
        constexpr double NAME_AREA_BOTTOM_RATIO{ 0.085 }; // 아래쪽
        constexpr std::wstring_view STAGE_KEYWORD{ L"단계" };   // 가디언 토벌 "n단계"
        constexpr std::wstring_view SHADOW_KEYWORD{ L"잔영" };  // 가디언 토벌 로테이션 "…의 잔영 n단계"
        // 2부 엔드 콘텐츠 보스 이름 (띄어쓰기 · 쉼표 없이) → 인원 (정재가 준 범위, 목록에 없는 콘텐츠는 인원을 모름)
        struct RaidBoss { std::wstring_view m_name; int m_size{}; };
        constexpr std::array<RaidBoss, 12> RAID_BOSSES{ {
            { L"욕망의주인에키드나", 8 }, { L"잿더미의불씨아브렐슈드", 8 }, { L"심연의파수꾼아르모체", 8 },   // 카제로스 레이드 4막 : 파멸의 성채
            { L"심연의군주카제로스", 8 }, { L"대악마카제로스", 8 }, { L"죽음의현신카제로스", 8 },                 // 종막 : 최후의 날
            { L"고통의마녀세르카", 4 }, { L"코르부스툴라크", 4 },                                                   // 그림자 레이드 세르카
            { L"죽음의계율자벨가르딘", 8 }, { L"페투스안크라그마", 8 },                                             // 그림자 레이드 벨가르딘
            { L"대주교아르세노스", 4 }, { L"광신의인도자아르세노스", 4 },                                           // 지평의 성당
        } };
        constexpr std::array<std::wstring_view, 4> GUARDIANS{ L"아게오로스", L"스콜라키아", L"드렉탈라스", L"크라티오스" };   // 가디언 토벌 (4인, "…의 잔영 n단계")
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

        // 빨간 글자(보스 이름)만 검게 남긴 흑백 그림 (여백 포함)
        [[nodiscard]] FrameImage MakeRedMask(const FrameImage& image)
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
                    if (red < NAME_RED_MIN || red - green < NAME_RED_OVER_GREEN || red - blue < NAME_RED_OVER_BLUE) continue;   // 빨간 글자 아님
                    const std::size_t to = (static_cast<std::size_t>(y + MASK_PADDING) * width + x + MASK_PADDING) * FrameImage::BYTES_PER_PIXEL;
                    pixels[to + CHANNEL_BLUE] = INK;                                  // 검정
                    pixels[to + CHANNEL_GREEN] = INK;
                    pixels[to + CHANNEL_RED] = INK;
                }
            for (std::size_t index = CHANNEL_ALPHA; index < pixels.size(); index += FrameImage::BYTES_PER_PIXEL) pixels[index] = PAPER;   // 불투명
            return FrameImage::FromRows(width, height, pixels, width * FrameImage::BYTES_PER_PIXEL);
        }

        // 영역을 OCR해서 시계 글자 찾기 (keyword가 있으면 그 글자가 보인 결과만)
        [[nodiscard]] std::optional<int> ReadClock(const FrameImage& frame, const TextRecognizer& text, const PixelRect& area, bool includeYellow, std::wstring_view keyword, bool isMasked = true)
        {
            const PixelRect clamped = ClampRect(area, frame.GetWidth(), frame.GetHeight());   // 프레임 안
            if (clamped.IsEmpty()) return std::nullopt;                               // 빈 영역
            const FrameImage crop = frame.Crop(clamped);                              // 영역
            const auto result = text.Recognize(isMasked ? MakeMask(crop, includeYellow) : crop, OCR_SCALE);   // 2배 OCR (흑백 · 원본)
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
        const PixelRect enrageArea = GetEnrageArea(frame.GetWidth(), frame.GetHeight());   // 레이드 광폭화 영역
        if (const auto enrage = ReadClock(frame, text, enrageArea, true, ENRAGE_KEYWORD))   // 흰 · 노란 글자만 남겨 읽기
            return BossTimerReading{ *enrage, true };
        if (const auto enrage = ReadClock(frame, text, enrageArea, true, ENRAGE_KEYWORD, false))   // 못 읽으면 원본 그대로 (흑백으로 만들면 글자가 뭉개지는 화면)
            return BossTimerReading{ *enrage, true };
        if (const auto guardian = ReadClock(frame, text, GetGuardianArea(frame.GetWidth(), frame.GetHeight()), false, {}))        // 가디언 토벌
            return BossTimerReading{ *guardian, false };
        return std::nullopt;                                                          // 타이머 없음
    }

    std::optional<int> BossTimerDetector::ReadEnrageFromName(const FrameImage& frame, const TextRecognizer& text, const PixelRect& textBounds)
    {
        const int top = textBounds.m_y - static_cast<int>(textBounds.m_height * NAME_TOP_FACTOR);         // 이름 줄 위
        const int bottom = textBounds.m_y - static_cast<int>(textBounds.m_height * NAME_BOTTOM_FACTOR);   // 이름 줄 아래
        const int side = static_cast<int>(textBounds.m_width * NAME_SIDE_FACTOR);                         // 좌우 여유
        const PixelRect area = ClampRect(PixelRect{ textBounds.m_x - side, top, textBounds.m_width + side * 2, bottom - top }, frame.GetWidth(), frame.GetHeight());   // 체력 숫자 위 띠
        if (area.IsEmpty()) return std::nullopt;                                      // 화면 밖
        const auto result = text.Recognize(frame.Crop(area), OCR_SCALE);              // 2배 OCR (빨간 · 흰 이름 그대로)
        if (!result) return std::nullopt;                                             // OCR 실패
        for (const TextLine& line : result->m_lines)                                  // 줄마다
            if (const auto seconds = FindEnrageSeconds(line.m_text)) return seconds;  // 표에 있는 보스
        return std::nullopt;                                                          // 모르는 보스
    }

    std::optional<int> BossTimerDetector::FindRaidSize(std::wstring_view bossName)
    {
        std::wstring compact;                                                         // 띄어쓰기 · 쉼표 · 마침표 뺀 이름
        for (const wchar_t character : bossName)
        {
            if (character >= L'0' && character <= L'9') return std::nullopt;          // 숫자가 있으면 이름이 아님 (체력 숫자 줄)
            if (!std::iswspace(character) && character != L',' && character != L'.') compact += character;
        }
        if (compact.size() < MIN_NAME_LENGTH) return std::nullopt;                    // 너무 짧음
        std::optional<int> best;                                                      // 가장 잘 맞는 보스의 인원
        std::size_t bestDistance = std::numeric_limits<std::size_t>::max();           // 그 거리
        for (const RaidBoss& boss : RAID_BOSSES)                                      // 목록마다
        {
            const std::size_t distance = OCRTextParser::FindDistance(boss.m_name, compact);   // 편집 거리
            const std::size_t allowed = std::max(MAX_NAME_DISTANCE, (boss.m_name.size() + 1) / NAME_DISTANCE_DIVISOR);   // 허용 (이름 길이의 1/3 반올림, 빨간 글씨는 OCR이 자주 틀림)
            if (distance <= allowed && distance < bestDistance) { bestDistance = distance; best = boss.m_size; }
        }
        return best;                                                                  // 목록에 없으면 nullopt (2부에서 쓰지 않는 콘텐츠)
    }

    bool BossTimerDetector::IsGuardianStage(std::wstring_view line)
    {
        const std::wstring compact = OCRTextParser::Compact(line);                    // 띄어쓰기 없이
        const std::size_t stage = compact.find(STAGE_KEYWORD);                        // "단계"
        if (stage == std::wstring::npos || stage == 0) return false;                  // 단계 없음
        const wchar_t before = compact[stage - 1];                                    // 단계 앞 글자
        if (before < L'0' || before > L'9') return false;                             // "3단계" 아님
        if (OCRTextParser::FindDistance(SHADOW_KEYWORD, compact) <= 1) return true;   // "…의 잔영 n단계" (로테이션 가디언, OCR이 진영 · 한영으로 읽어도)
        return std::ranges::any_of(GUARDIANS, [&compact](std::wstring_view name)     // 고정 가디언 (한 글자 오인식 허용)
            { return OCRTextParser::FindDistance(name, compact) <= 1; });
    }

    std::optional<int> BossTimerDetector::DetectRaidSize(const FrameImage& frame, const TextRecognizer& text, const std::optional<PixelRect>& textBounds)
    {
        const int left = static_cast<int>(frame.GetWidth() * STAGE_LEFT_RATIO);       // 가디언 토벌 퀘스트 칸
        const int top = static_cast<int>(frame.GetHeight() * STAGE_TOP_RATIO);
        const PixelRect stageArea = ClampRect(PixelRect{ left, top, static_cast<int>(frame.GetWidth() * STAGE_RIGHT_RATIO) - left,
            static_cast<int>(frame.GetHeight() * STAGE_BOTTOM_RATIO) - top }, frame.GetWidth(), frame.GetHeight());
        if (!stageArea.IsEmpty())
        {
            const FrameImage stageCrop = frame.Crop(stageArea);                       // 목표 가디언 칸
            for (const int scale : { 1, OCR_SCALE })                                  // 1배 → 못 찾으면 2배 (붉은 바탕 위 작은 글씨)
                if (const auto result = text.Recognize(stageCrop, scale))
                    if (std::ranges::any_of(result->m_lines, [](const TextLine& line) { return IsGuardianStage(line.m_text); })) return RAID_SIZE_FOUR;   // 가디언 토벌
        }
        const int nameLeft = static_cast<int>(frame.GetWidth() * NAME_LEFT_RATIO);    // 보스 이름 고정 영역 (체력바를 못 찾아도)
        const int nameTopFixed = static_cast<int>(frame.GetHeight() * NAME_AREA_TOP_RATIO);
        const PixelRect fixedName = ClampRect(PixelRect{ nameLeft, nameTopFixed, static_cast<int>(frame.GetWidth() * NAME_RIGHT_RATIO) - nameLeft,
            static_cast<int>(frame.GetHeight() * NAME_AREA_BOTTOM_RATIO) - nameTopFixed }, frame.GetWidth(), frame.GetHeight());
        if (!fixedName.IsEmpty())
            if (const auto size = ReadRaidSizeFromName(frame.Crop(fixedName), text)) return size;   // 고정 자리에서 찾음
        if (textBounds)                                                               // 체력 숫자 줄을 찾았으면 그 위 띠
        {
            const int nameTop = textBounds->m_y - static_cast<int>(textBounds->m_height * NAME_TOP_FACTOR);         // 이름 줄 위
            const int nameBottom = textBounds->m_y - static_cast<int>(textBounds->m_height * NAME_BOTTOM_FACTOR);   // 이름 줄 아래
            const int side = static_cast<int>(textBounds->m_width * NAME_SIDE_FACTOR);                               // 좌우 여유
            const PixelRect nameArea = ClampRect(PixelRect{ textBounds->m_x - side, nameTop, textBounds->m_width + side * 2, nameBottom - nameTop }, frame.GetWidth(), frame.GetHeight());
            if (!nameArea.IsEmpty())
                if (const auto size = ReadRaidSizeFromName(frame.Crop(nameArea), text)) return size;
        }
        const PixelRect barArea = BossHealthDetector::GetBarArea(frame.GetWidth(), frame.GetHeight());   // 마지막: 보스 체력바 영역 전체로 넓혀 다시 (숫자 줄은 이름으로 안 잡힘)
        if (barArea.IsEmpty()) return std::nullopt;
        return ReadRaidSizeFromName(frame.Crop(barArea), text);
    }

    std::optional<int> BossTimerDetector::ReadRaidSizeFromName(const FrameImage& nameArea, const TextRecognizer& text)
    {
        const auto readLines = [&text](const FrameImage& image) -> std::optional<int> // 1배 → 2배 (배율마다 읽히는 글자가 달라서 둘 다)
            {
                for (const int scale : { 1, OCR_SCALE })
                    if (const auto result = text.Recognize(image, scale))
                        for (const TextLine& line : result->m_lines)                  // 줄마다
                            if (const auto size = FindRaidSize(line.m_text)) return size;   // 목록의 보스 이름
                return std::nullopt;
            };
        if (const auto size = readLines(nameArea)) return size;                       // 원본 그대로 (어두운 바탕)
        return readLines(MakeRedMask(nameArea));                                      // 빨간 글자만 (밝은 빛 바탕 · 상단바 숫자가 섞인 화면)
    }
}
