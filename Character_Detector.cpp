module LostArkMultiTool.Vision.CharacterDetector;  // 현재 접속 캐릭터 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.TextEncoding;  // 이름 UTF-8 바꾸기 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.NameplateMemory;  // 이름표 모양 기억 가져오기
import LostArkMultiTool.Vision.OCRTextParser;  // 띄어쓰기 지우기 · 편집 거리 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        // ----- 로딩 화면 (픽셀) -----
        constexpr double LOADING_UPPER_TOP_RATIO{ 0.935 };             // 진행 막대 위 검은 띠 시작 (화면 세로 비율)
        constexpr double LOADING_UPPER_BOTTOM_RATIO{ 0.947 };          // 진행 막대 위 검은 띠 끝
        constexpr double LOADING_TRACK_TOP_RATIO{ 0.950 };             // 진행 막대 줄 시작
        constexpr double LOADING_TRACK_BOTTOM_RATIO{ 0.970 };          // 진행 막대 줄 끝
        constexpr double LOADING_BOTTOM_TOP_RATIO{ 0.972 };            // 맨 아래 검은 띠 시작
        constexpr double LOADING_BOTTOM_BOTTOM_RATIO{ 0.992 };         // 맨 아래 검은 띠 끝
        constexpr int DARK_CHANNEL_MAX{ 60 };                          // 세 채널이 모두 이 미만이면 검은 픽셀
        constexpr double MIN_DARK_ROW_RATIO{ 0.95 };                   // 검은 띠 줄: 이 비율 이상이 검은 픽셀 (게임 화면은 아래 단축키 막대 때문에 0.6 근처)
        constexpr double MAX_TRACK_DARK_RATIO{ 0.70 };                 // 진행 막대 줄: 검은 픽셀이 이 비율 이하인 줄이 하나는 있어야 함
        constexpr int ROW_SAMPLE_STEP{ 2 };                            // 줄을 볼 때 가로로 건너뛰는 칸 (속도)

        // ----- 로딩 화면 (OCR 보조) -----
        constexpr std::wstring_view LOADING_TIP_TEXT{ L"팁" };          // 로딩 화면 아래쪽 제목
        constexpr double LOADING_TIP_MIN_Y_RATIO{ 0.70 };              // "팁"은 화면 아래쪽 (위에서 이 비율보다 아래)
        constexpr double LOADING_TIP_MIN_X_RATIO{ 0.35 };              // "팁" 가로 가운데 범위 시작
        constexpr double LOADING_TIP_MAX_X_RATIO{ 0.65 };              // "팁" 가로 가운데 범위 끝

        // ----- 캐릭터 정보 창 -----
        constexpr std::wstring_view LEVEL_PREFIX_UPPER{ L"LV" };       // 전투 레벨 줄 머리 ("Lv.70.00", 대문자로 비교)
        constexpr wchar_t ROMAN_SIX{ 0x2165 };                         // Ⅵ (OCR이 "v."를 로마 숫자로 읽음)
        constexpr wchar_t ROMAN_FIVE{ 0x2164 };                        // Ⅴ
        constexpr double NAME_LINE_MAX_GAP_RATIO{ 3.0 };               // 이름 줄은 "Lv." 줄 바로 위 (줄 높이 배수 이내)
        constexpr std::wstring_view ITEM_LEVEL_TEXT{ L"아이템레벨" };   // 장착 아이템 레벨 줄 (띄어쓰기 없이)
        constexpr std::size_t ITEM_LEVEL_MAX_DISTANCE{ 1 };            // 한 글자 오인식 허용
        constexpr double ITEM_LEVEL_SEARCH_HEIGHT_RATIO{ 0.12 };       // 레벨 글자 아래로 숫자를 찾을 높이 (화면 세로 비율)
        constexpr double MIN_ITEM_LEVEL{ 1.0 };                        // 아이템 레벨 하한 (이보다 작으면 오인식)
        constexpr double MAX_ITEM_LEVEL{ 2500.0 };                     // 아이템 레벨 상한
        constexpr std::size_t ITEM_LEVEL_DECIMALS{ 2 };                // 소수 자리 (1775.00)
        constexpr std::size_t MAX_ITEM_LEVEL_DIGITS{ 4 };              // 정수 자리 최대
        constexpr int PLATE_TEXT_OCR_SCALE{ 3 };                       // 이름표만 잘라 OCR할 때 배율 (작은 글자)
        constexpr double PLATE_PADDING_RATIO{ 1.0 };                   // 이름표 OCR 여백 (글자 높이 배수)
        constexpr std::size_t MIN_NAME_LENGTH{ 2 };                    // 이름 글자 수 하한 (별칭 후보)
        constexpr std::size_t MAX_NAME_LENGTH{ 12 };                   // 이름 글자 수 상한 (별칭 후보)

        // ----- 머리 위 이름표 -----
        constexpr double PLATE_MIN_X_RATIO{ 0.30 };                    // 이름표 찾을 영역 가로 시작 (내 캐릭터는 화면 가운데 근처)
        constexpr double PLATE_MAX_X_RATIO{ 0.70 };                    // 이름표 찾을 영역 가로 끝
        constexpr double PLATE_MIN_Y_RATIO{ 0.12 };                    // 이름표 찾을 영역 세로 시작
        constexpr double PLATE_MAX_Y_RATIO{ 0.55 };                    // 이름표 찾을 영역 세로 끝 (캐릭터 머리 위)
        constexpr double PLATE_ANCHOR_Y_RATIO{ 0.36 };                 // 내 이름표가 보통 있는 높이 (가까운 이름 우선)
        constexpr std::size_t LIST_NAME_COUNT{ 2 };                    // 원정대 이름이 이만큼 넘게 보이면 목록 창 → 노란 이름만 인정

        // ----- 이름표 글자색 (내 이름 = 연한 노란색, 다른 사람 = 흰색, 칭호 = 주황) -----
        constexpr int MIN_TEXT_BRIGHTNESS{ 150 };                      // 이보다 밝은 픽셀만 글자로 봄 (바탕 제외)
        constexpr int MIN_YELLOW_CHANNEL{ 165 };                       // 빨강 · 초록이 이 이상 (연한 색)
        constexpr int MIN_BLUE_GAP{ 15 };                              // 파랑이 빨강 · 초록보다 이만큼 낮아야 노란 기운
        constexpr int MAX_RED_GREEN_GAP{ 25 };                         // 빨강 · 초록 차이가 이 이하 (주황 칭호 제외)
        constexpr int MIN_COLOR_PIXELS{ 8 };                           // 색을 판단할 최소 픽셀 수
        constexpr int CHANNEL_BLUE{ 0 };                               // BGRA 파랑 위치
        constexpr int CHANNEL_GREEN{ 1 };                              // BGRA 초록 위치
        constexpr int CHANNEL_RED{ 2 };                                // BGRA 빨강 위치
        constexpr int BRIGHTNESS_CHANNELS{ 3 };                        // 밝기 평균에 쓰는 채널 수

        // ----- 이름 비교 (자모 단위) -----
        constexpr wchar_t HANGUL_FIRST{ 0xAC00 };                      // 한글 음절 첫 글자 (가)
        constexpr wchar_t HANGUL_LAST{ 0xD7A3 };                       // 한글 음절 끝 글자 (힣)
        constexpr int JUNG_JONG_COUNT{ 588 };                          // 초성 하나당 음절 수 (중성 21 × 종성 28)
        constexpr int JONG_COUNT{ 28 };                                // 종성 수 (받침 없음 포함)
        constexpr int CHO_TAG{ 0x10000 };                              // 초성 토큰 구분 값 (다른 글자와 겹치지 않게)
        constexpr int JUNG_TAG{ 0x20000 };                             // 중성 토큰 구분 값
        constexpr int JONG_TAG{ 0x30000 };                             // 종성 토큰 구분 값
        constexpr std::size_t LONG_NAME_LENGTH{ 4 };                   // 이 길이 이상 이름은 자모 두 개까지 오인식 허용
        constexpr std::size_t SHORT_NAME_MAX_DISTANCE{ 1 };            // 짧은 이름 허용 자모 거리
        constexpr std::size_t LONG_NAME_MAX_DISTANCE{ 2 };             // 긴 이름 허용 자모 거리
        constexpr std::size_t MAX_EXTRA_CHARACTERS{ 1 };               // 단어에 더 붙어도 되는 글자 수 (괄호 등)
        constexpr double HALF{ 0.5 };                                  // 절반

        // 사각형 가운데 X · Y
        [[nodiscard]] double CenterX(const PixelRect& rect) noexcept { return rect.m_x + rect.m_width * HALF; }
        [[nodiscard]] double CenterY(const PixelRect& rect) noexcept { return rect.m_y + rect.m_height * HALF; }

        // 글자 → 비교 토큰 (한글은 초성 · 중성 · 종성 세 개, 영문은 소문자, 나머지는 그대로)
        [[nodiscard]] std::vector<int> Tokenize(std::wstring_view text)
        {
            std::vector<int> tokens;                                              // 결과
            tokens.reserve(text.size() * BRIGHTNESS_CHANNELS);                    // 한글은 세 개씩
            for (wchar_t character : text)                                        // 글자마다
            {
                if (character >= HANGUL_FIRST && character <= HANGUL_LAST)       // 한글 음절
                {
                    const int index = character - HANGUL_FIRST;                   // 음절 번호
                    tokens.push_back(CHO_TAG + index / JUNG_JONG_COUNT);          // 초성
                    tokens.push_back(JUNG_TAG + (index % JUNG_JONG_COUNT) / JONG_COUNT);   // 중성
                    tokens.push_back(JONG_TAG + index % JONG_COUNT);              // 종성 (없음 포함)
                }
                else tokens.push_back(static_cast<int>(std::towlower(character)));   // 다른 글자 (영문 대소문자 무시)
            }
            return tokens;   // 토큰
        }

        // 화면 어딘가의 "장착 아이템 레벨" 아래 숫자 (정보 창이 열려 있을 때)
        [[nodiscard]] std::optional<double> FindItemLevel(const TextResult& text, int frameHeight)
        {
            for (const TextLine& label : text.m_lines)                                               // 줄마다
            {
                if (OCRTextParser::FindDistance(ITEM_LEVEL_TEXT, OCRTextParser::Compact(label.m_text)) > ITEM_LEVEL_MAX_DISTANCE) continue;   // 레벨 글자 아님
                if (const auto level = CharacterDetector::ParseItemLevel(label.m_text)) return level;   // 같은 줄에 숫자가 붙어 있음
                std::optional<double> best;                                                          // 가장 가까운 숫자
                double bestGap = std::numeric_limits<double>::max();                                 // 그 거리
                for (const TextLine& number : text.m_lines)                                          // 아래 줄들
                {
                    const double gap = number.m_bounds.m_y - label.m_bounds.GetBottom();             // 아래로 거리
                    if (gap < 0 || gap > frameHeight * ITEM_LEVEL_SEARCH_HEIGHT_RATIO || gap >= bestGap) continue;   // 범위 밖 · 더 멂
                    if (const auto parsed = CharacterDetector::ParseItemLevel(number.m_text)) { best = parsed; bestGap = gap; }   // 숫자 줄
                }
                if (best) return best;                                                               // 찾음
            }
            return std::nullopt;                                                                     // 못 읽음 (큰 장식 숫자는 OCR이 자주 틀림)
        }

        // 원정대 이름 후보 하나 (이름표 영역에서 찾은 것)
        struct PlateCandidate
        {
            std::wstring m_name;        // 원정대 이름
            std::size_t m_distance{};   // 자모 거리
            bool m_isOwnColor{};        // 연한 노란색인지
            double m_anchorDistance{};  // 기준점까지 거리 (제곱)
        };
    }

    CharacterDetector::CharacterDetector(std::vector<std::wstring> names, std::vector<CharacterAlias> aliases, std::shared_ptr<NameplateMemory> memory)
        : m_names{ std::move(names) }       // 원래 이름
        , m_aliases{ std::move(aliases) }   // 별칭
        , m_memory{ std::move(memory) }     // 이름표 모양 기억
    {
        m_compactNames.reserve(m_names.size());                                   // 자리 확보
        for (const std::wstring& name : m_names) m_compactNames.push_back(OCRTextParser::Compact(name));   // 비교용 이름
        for (CharacterAlias& alias : m_aliases) alias.m_readText = OCRTextParser::Compact(alias.m_readText); // 별칭도 띄어쓰기 없이
    }

    bool CharacterDetector::HasNames() const noexcept
    {
        return !m_names.empty();   // 이름이 하나라도 있으면 비교 가능
    }

    std::size_t CharacterDetector::FindJamoDistance(std::wstring_view name, std::wstring_view text)
    {
        const std::vector<int> pattern = Tokenize(name);                          // 이름 토큰
        const std::vector<int> target = Tokenize(text);                           // 읽은 글자 토큰
        if (pattern.empty()) return 0;                                            // 빈 이름
        std::vector<std::size_t> previous(target.size() + 1, 0);                  // 이전 행 (시작은 어디서든 공짜 → 0)
        std::vector<std::size_t> current(target.size() + 1, 0);                   // 지금 행
        for (std::size_t row = 1; row <= pattern.size(); ++row)                   // 이름 토큰마다
        {
            current[0] = row;                                                     // 읽은 글자 없이 지운 수
            for (std::size_t column = 1; column <= target.size(); ++column)       // 읽은 토큰마다
            {
                const std::size_t replace = previous[column - 1] + (pattern[row - 1] == target[column - 1] ? 0 : 1);   // 바꾸기
                current[column] = std::min({ replace, previous[column] + 1, current[column - 1] + 1 });               // 바꾸기 · 지우기 · 넣기 중 최소
            }
            std::swap(previous, current);                                         // 다음 행
        }
        return *std::ranges::min_element(previous);                               // 끝은 어디서든 (가장 작은 거리)
    }

    std::optional<double> CharacterDetector::ParseItemLevel(std::wstring_view text)
    {
        std::wstring normalized;                                                  // 숫자 · 마침표만 남긴 글자
        for (wchar_t character : text)                                            // 글자마다
        {
            if (character >= L'0' && character <= L'9') normalized += character; // 숫자
            else if (character == L'I' || character == L'l' || character == L'|' || character == L'i') normalized += L'1';   // 1을 I · l로 읽은 경우
            else if (character == L'O' || character == L'o') normalized += L'0';  // 0을 O로 읽은 경우
            else if (character == L'.') normalized += L'.';                       // 소수점
            else if (character == L',' || character == L' ') continue;            // 쉼표 · 띄어쓰기는 무시
            else if (!normalized.empty()) break;                                  // 숫자가 끝남
        }
        const std::size_t dot = normalized.find(L'.');                            // 소수점 위치
        if (dot == std::wstring::npos || dot == 0 || dot > MAX_ITEM_LEVEL_DIGITS) return std::nullopt;   // "1775.00" 모양이 아님
        if (normalized.size() < dot + 1 + ITEM_LEVEL_DECIMALS) return std::nullopt;                       // 소수 두 자리가 없음
        const std::wstring_view digits{ normalized.data(), dot + 1 + ITEM_LEVEL_DECIMALS };               // "1775.00"
        if (digits.find(L'.', dot + 1) != std::wstring_view::npos) return std::nullopt;                   // 소수점이 둘
        double value = 0.0;                                                       // 결과
        const std::string ascii{ digits.begin(), digits.end() };                  // 숫자 · 마침표뿐이라 그대로 바꿈
        const auto [end, error] = std::from_chars(ascii.data(), ascii.data() + ascii.size(), value);   // 숫자로
        if (error != std::errc{} || end != ascii.data() + ascii.size()) return std::nullopt;            // 실패
        if (value < MIN_ITEM_LEVEL || value > MAX_ITEM_LEVEL) return std::nullopt;                      // 범위 밖
        return value;   // 아이템 레벨
    }

    bool CharacterDetector::IsOwnNameColor(const FrameImage& frame, const PixelRect& bounds) noexcept
    {
        const PixelRect area = ClampRect(bounds, frame.GetWidth(), frame.GetHeight());   // 프레임 안으로
        const auto pixels = frame.GetPixels();                                           // 픽셀
        long long red = 0;                                                               // 밝은 픽셀 빨강 합
        long long green = 0;                                                             // 초록 합
        long long blue = 0;                                                              // 파랑 합
        int count = 0;                                                                   // 센 픽셀 수
        for (int y = area.m_y; y < area.GetBottom(); ++y)                                // 줄마다
        {
            for (int x = area.m_x; x < area.GetRight(); ++x)                             // 칸마다
            {
                const std::size_t offset = (static_cast<std::size_t>(y) * frame.GetWidth() + x) * FrameImage::BYTES_PER_PIXEL;   // 픽셀 위치
                const int b = pixels[offset + CHANNEL_BLUE];                             // 파랑
                const int g = pixels[offset + CHANNEL_GREEN];                            // 초록
                const int r = pixels[offset + CHANNEL_RED];                              // 빨강
                if ((r + g + b) / BRIGHTNESS_CHANNELS < MIN_TEXT_BRIGHTNESS) continue;   // 어두운 바탕은 뺌
                red += r;                                                                // 합 누적
                green += g;
                blue += b;
                ++count;                                                                 // 수 증가
            }
        }
        if (count < MIN_COLOR_PIXELS) return false;                                      // 글자를 못 찾음
        const long long averageRed = red / count;                                        // 평균 빨강
        const long long averageGreen = green / count;                                    // 평균 초록
        const long long averageBlue = blue / count;                                      // 평균 파랑
        const long long lowerRedGreen = std::min(averageRed, averageGreen);              // 빨강 · 초록 중 작은 값
        return lowerRedGreen >= MIN_YELLOW_CHANNEL                                       // 연한 색
            && averageBlue + MIN_BLUE_GAP <= lowerRedGreen                               // 파랑이 낮음 (노란 기운)
            && std::abs(averageRed - averageGreen) <= MAX_RED_GREEN_GAP;                 // 주황 · 분홍이 아님
    }

    bool CharacterDetector::IsLoadingScreen(const FrameImage& frame) noexcept
    {
        const int width = frame.GetWidth();                                                  // 프레임 가로
        const int height = frame.GetHeight();                                                // 프레임 세로
        if (width <= 0 || height <= 0) return false;                                         // 빈 프레임
        const auto pixels = frame.GetPixels();                                               // 픽셀
        const auto darkRatio = [&](int y)                                                    // 한 줄의 검은 픽셀 비율
            {
                int dark = 0;                                                                // 검은 픽셀 수
                int total = 0;                                                               // 본 픽셀 수
                for (int x = 0; x < width; x += ROW_SAMPLE_STEP)                             // 건너뛰며
                {
                    const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * FrameImage::BYTES_PER_PIXEL;   // 픽셀 위치
                    const int brightest = std::max({ pixels[offset + CHANNEL_BLUE], pixels[offset + CHANNEL_GREEN], pixels[offset + CHANNEL_RED] });   // 가장 밝은 채널
                    if (brightest < DARK_CHANNEL_MAX) ++dark;                                // 검은 픽셀
                    ++total;                                                                 // 센 수
                }
                return total == 0 ? 0.0 : static_cast<double>(dark) / total;               // 비율
            };
        const auto isDarkBand = [&](double topRatio, double bottomRatio)                     // 띠 전체가 검은지
            {
                for (int y = static_cast<int>(height * topRatio); y <= static_cast<int>(height * bottomRatio) && y < height; ++y)
                    if (darkRatio(y) < MIN_DARK_ROW_RATIO) return false;                     // 밝은 줄이 있음
                return true;                                                                 // 모두 검음
            };
        if (!isDarkBand(LOADING_UPPER_TOP_RATIO, LOADING_UPPER_BOTTOM_RATIO)) return false;  // 진행 막대 위가 검지 않음 (게임 화면)
        if (!isDarkBand(LOADING_BOTTOM_TOP_RATIO, LOADING_BOTTOM_BOTTOM_RATIO)) return false;// 맨 아래가 검지 않음
        for (int y = static_cast<int>(height * LOADING_TRACK_TOP_RATIO); y <= static_cast<int>(height * LOADING_TRACK_BOTTOM_RATIO) && y < height; ++y)
            if (darkRatio(y) <= MAX_TRACK_DARK_RATIO) return true;                           // 진행 막대 줄이 있음 → 로딩 화면
        return false;                                                                        // 그냥 검은 화면 (암전 · 컷신)
    }

    std::optional<std::pair<std::wstring, std::size_t>> CharacterDetector::MatchName(std::wstring_view word) const
    {
        const std::wstring compact = OCRTextParser::Compact(word);               // 띄어쓰기 지운 단어
        if (compact.empty()) return std::nullopt;                                // 빈 단어
        for (const CharacterAlias& alias : m_aliases)                            // 별칭부터 (직접 고른 캐릭터)
            if (alias.m_readText == compact) return std::pair{ alias.m_name, std::size_t{ 0 } };   // 정확히 같으면 그 캐릭터

        std::optional<std::size_t> bestIndex;                                    // 가장 잘 맞는 이름 번호
        std::size_t bestDistance = std::numeric_limits<std::size_t>::max();      // 그 거리
        bool isAmbiguous = false;                                                // 같은 거리 다른 이름이 있는지
        for (std::size_t index = 0; index < m_compactNames.size(); ++index)      // 이름마다
        {
            const std::wstring& name = m_compactNames[index];                    // 비교용 이름
            if (name.empty()) continue;                                          // 빈 이름
            std::size_t distance = 0;                                            // 자모 거리
            if (compact != name)                                                 // 정확히 같지 않으면
            {
                if (compact.size() > name.size() + MAX_EXTRA_CHARACTERS) continue;   // 단어가 너무 긺 (다른 말에 이름이 들어 있는 경우)
                if (compact.size() + MAX_EXTRA_CHARACTERS < name.size()) continue;   // 단어가 너무 짧음
                const std::size_t maxDistance = name.size() >= LONG_NAME_LENGTH ? LONG_NAME_MAX_DISTANCE : SHORT_NAME_MAX_DISTANCE;   // 허용 거리
                distance = FindJamoDistance(name, compact);                      // 자모 거리
                if (distance > maxDistance) continue;                            // 안 맞음
            }
            if (distance < bestDistance) { bestDistance = distance; bestIndex = index; isAmbiguous = false; }   // 더 잘 맞음
            else if (distance == bestDistance) isAmbiguous = true;               // 같은 거리 다른 이름 → 애매
        }
        if (!bestIndex || isAmbiguous) return std::nullopt;                      // 못 찾았거나 애매함
        return std::pair{ m_names[*bestIndex], bestDistance };                   // (원래 이름, 거리)
    }

    CharacterReading CharacterDetector::Parse(const TextResult& text, int frameWidth, int frameHeight, const FrameImage* frame) const
    {
        // 1) 로딩 화면: 아래쪽 가운데 "팁"
        for (const TextLine& line : text.m_lines)                                                    // 줄마다
        {
            if (OCRTextParser::Compact(line.m_text) != LOADING_TIP_TEXT) continue;                   // "팁"만 있는 줄
            const double x = CenterX(line.m_bounds);                                                 // 가운데 X
            const bool isCentered = x >= frameWidth * LOADING_TIP_MIN_X_RATIO && x <= frameWidth * LOADING_TIP_MAX_X_RATIO;   // 가로 가운데
            if (isCentered && line.m_bounds.m_y >= frameHeight * LOADING_TIP_MIN_Y_RATIO)            // 아래쪽 가운데
                return CharacterReading{ CharacterSignal::SIGNAL_LOADING, CharacterSource::SOURCE_NONE, {}, std::nullopt, {}, false };   // 로딩 화면
        }

        CharacterReading reading;                                                                    // 결과 (기본: 못 찾음)

        // 2) 캐릭터 정보 창: "Lv." 줄 바로 위가 이름 (직업 · 이름 · 레벨 순서, 1배 OCR은 창 제목을 자주 틀려서 제목 대신 이 모양으로 찾음)
        for (const TextLine& levelLine : text.m_lines)                                               // 줄마다
        {
            std::wstring upper;                                                                      // 대문자 · 로마 숫자 풀기 ("LⅥ70" → "LVI70")
            for (wchar_t c : OCRTextParser::Compact(levelLine.m_text))                               // 띄어쓰기 없이
            {
                if (c == ROMAN_SIX) upper += L"VI";                                                  // Ⅵ (OCR이 "v."를 이렇게 읽음)
                else if (c == ROMAN_FIVE) upper += L"V";                                             // Ⅴ
                else upper += static_cast<wchar_t>(std::towupper(c));                                // 대문자
            }
            if (!upper.starts_with(LEVEL_PREFIX_UPPER)) continue;                                    // "Lv." 줄이 아님
            const std::wstring_view rest = std::wstring_view{ upper }.substr(LEVEL_PREFIX_UPPER.size());   // "Lv" 뒤 ("I70.00" · "I38FAMILY")
            const bool isCombatLevel = rest.find(L'.') != std::wstring_view::npos
                && std::ranges::all_of(rest, [](wchar_t c) { return (c >= L'0' && c <= L'9') || c == L'.' || c == L'I'; });   // 전투 레벨 "70.00"만 (원정대 "Lv.38 Family" 제외)
            if (!isCombatLevel) continue;                                                            // 이름 아래 레벨 줄이 아님
            const TextLine* nameLine = nullptr;                                                      // 바로 위 줄
            for (const TextLine& line : text.m_lines)                                                // 줄마다
            {
                if (&line == &levelLine) continue;                                                   // 자기 자신
                const double gap = levelLine.m_bounds.m_y - line.m_bounds.GetBottom();               // 위로 떨어진 거리
                if (gap < -levelLine.m_bounds.m_height * HALF || gap > levelLine.m_bounds.m_height * NAME_LINE_MAX_GAP_RATIO) continue;   // 바로 위가 아님
                if (std::abs(CenterX(line.m_bounds) - CenterX(levelLine.m_bounds)) > levelLine.m_bounds.m_width) continue;   // 가로로 떨어짐
                if (nameLine == nullptr || line.m_bounds.m_y > nameLine->m_bounds.m_y) nameLine = &line;   // 더 가까운(아래쪽) 줄
            }
            if (nameLine == nullptr || nameLine->m_words.size() != 1) continue;                      // 이름만 있는 한 단어 줄이 아님
            if (auto matched = MatchName(nameLine->m_text))                                          // 원정대 이름이면
            {
                reading.m_signal = CharacterSignal::SIGNAL_NAME;                                     // 이름 찾음
                reading.m_source = CharacterSource::SOURCE_INFO_WINDOW;                              // 정보 창
                reading.m_name = std::move(matched->first);                                          // 이름
                reading.m_itemLevel = FindItemLevel(text, frameHeight);                              // 장착 아이템 레벨 (못 읽으면 nullopt)
                return reading;                                                                      // 가장 확실 → 끝
            }
            const std::wstring unknown = OCRTextParser::Compact(nameLine->m_text);                   // 안 맞은 글자
            if (reading.m_unknownName.empty() && unknown.size() >= MIN_NAME_LENGTH && unknown.size() <= MAX_NAME_LENGTH) reading.m_unknownName = unknown;   // 별칭 후보 (처음 것)
        }
        if (!HasNames()) return reading;                                                             // 비교할 이름이 없음

        // 3) 머리 위 이름표: 화면 가운데 근처의 원정대 이름 (내 이름은 연한 노란색)
        const double anchorX = frameWidth * HALF;                                                    // 기준 X (화면 가운데)
        const double anchorY = frameHeight * PLATE_ANCHOR_Y_RATIO;                                   // 기준 Y (머리 위)
        std::vector<PlateCandidate> candidates;                                                      // 후보들
        for (const TextLine& line : text.m_lines)                                                    // 줄마다
        {
            for (const TextWord& word : line.m_words)                                                // 단어마다
            {
                const double x = CenterX(word.m_bounds);                                             // 단어 가운데
                const double y = CenterY(word.m_bounds);
                const bool isInside = x >= frameWidth * PLATE_MIN_X_RATIO && x <= frameWidth * PLATE_MAX_X_RATIO
                    && y >= frameHeight * PLATE_MIN_Y_RATIO && y <= frameHeight * PLATE_MAX_Y_RATIO; // 가운데 영역
                if (!isInside) continue;                                                             // 영역 밖
                auto matched = MatchName(word.m_text);                                               // 원정대 이름인지
                if (!matched) continue;                                                              // 아님 (칭호 · 다른 사람)
                const bool isOwnColor = frame != nullptr && IsOwnNameColor(*frame, word.m_bounds);   // 노란 이름인지
                const double distance = (x - anchorX) * (x - anchorX) + (y - anchorY) * (y - anchorY);   // 기준점과의 거리
                candidates.push_back(PlateCandidate{ std::move(matched->first), matched->second, isOwnColor, distance });   // 후보
            }
        }
        std::vector<std::wstring> distinctNames;                                                     // 서로 다른 이름들
        for (const PlateCandidate& candidate : candidates)                                           // 후보마다
            if (std::ranges::find(distinctNames, candidate.m_name) == distinctNames.end()) distinctNames.push_back(candidate.m_name);
        const bool isList = distinctNames.size() >= LIST_NAME_COUNT;                                 // 원정대 목록 같은 창
        const PlateCandidate* best = nullptr;                                                        // 고른 후보
        for (const PlateCandidate& candidate : candidates)                                           // 후보마다
        {
            const bool isTrusted = candidate.m_isOwnColor || (!isList && candidate.m_distance == 0); // 노란 이름이거나, 목록이 아니고 정확히 같음
            if (!isTrusted) continue;                                                                // 믿을 수 없음 (흰 글자 오인식 등)
            if (best == nullptr || candidate.m_anchorDistance < best->m_anchorDistance) best = &candidate;   // 더 가까움
        }
        if (best != nullptr)                                                                         // 찾음
        {
            reading.m_signal = CharacterSignal::SIGNAL_NAME;                                         // 이름 찾음
            reading.m_source = CharacterSource::SOURCE_NAMEPLATE;                                    // 이름표
            reading.m_name = best->m_name;                                                           // 이름
        }
        return reading;   // 결과 (못 찾으면 SIGNAL_NONE + 정보 창 레벨 · 별칭 후보)
    }

    CharacterReading CharacterDetector::Detect(const FrameImage& frame, const TextRecognizer& text, bool isFullScanAllowed) const
    {
        const int width = frame.GetWidth();                                                          // 프레임 가로
        const int height = frame.GetHeight();                                                        // 프레임 세로
        if (IsLoadingScreen(frame))                                                                  // 1) 로딩 화면 (픽셀, OCR 없음)
            return CharacterReading{ CharacterSignal::SIGNAL_LOADING, CharacterSource::SOURCE_NONE, {}, std::nullopt, {}, false };

        CharacterReading reading;                                                                    // 결과
        const auto blob = HasNames() ? NameplateMemory::FindOwnNameplate(frame) : std::nullopt;      // 내 이름표 (연한 노란 글자)
        const auto toResult = [](std::wstring name, CharacterSource source)                          // 이름 결과 만들기
            {
                return CharacterReading{ CharacterSignal::SIGNAL_NAME, source, std::move(name), std::nullopt, {}, false };
            };
        if (blob)
        {
            // 2) 기억한 모양과 비교 (OCR 없음)
            if (m_memory)
            {
                if (const auto known = m_memory->Match(blob->m_shape))                               // 닮은 모양
                {
                    std::wstring name = Core::TextEncoding::UTF8ToWide(*known);                     // 넓은 글자
                    if (std::ranges::find(m_names, name) != m_names.end())                          // 원정대 캐릭터
                    {
                        CharacterReading known_reading = toResult(std::move(name), CharacterSource::SOURCE_NAMEPLATE);   // 결과
                        known_reading.m_plateBounds = blob->m_bounds;                               // 이름표 위치
                        return known_reading;
                    }
                }
            }

            // 3) 처음 보는 모양 → 이름표만 잘라 3배 OCR (겹받침 이름은 자모 단위로 맞춤)
            const int padding = std::max(1, static_cast<int>(blob->m_bounds.m_height * PLATE_PADDING_RATIO));   // 여백
            const PixelRect area = ClampRect(PixelRect{ blob->m_bounds.m_x - padding, blob->m_bounds.m_y - padding,
                blob->m_bounds.m_width + padding * 2, blob->m_bounds.m_height + padding * 2 }, width, height);   // OCR 영역
            if (const auto plateText = text.Recognize(frame.Crop(area), PLATE_TEXT_OCR_SCALE))       // 이름표 OCR
            {
                std::wstring longest;                                                                // 가장 긴 단어 (별칭 후보)
                for (const TextLine& line : plateText->m_lines)                                      // 줄마다
                {
                    for (const TextWord& word : line.m_words)                                        // 단어마다
                    {
                        if (auto matched = MatchName(word.m_text))                                   // 원정대 이름
                        {
                            if (m_memory) m_memory->Learn(Core::TextEncoding::WideToUTF8(matched->first), blob->m_shape);   // 모양 기억
                            CharacterReading read_reading = toResult(std::move(matched->first), CharacterSource::SOURCE_NAMEPLATE);   // 결과
                            read_reading.m_plateBounds = blob->m_bounds;                             // 이름표 위치
                            return read_reading;
                        }
                        const std::wstring compact = OCRTextParser::Compact(word.m_text);            // 띄어쓰기 없이
                        if (compact.size() > longest.size()) longest = compact;                      // 가장 긴 단어
                    }
                }
                if (longest.size() >= MIN_NAME_LENGTH && longest.size() <= MAX_NAME_LENGTH) reading.m_unknownName = longest;   // 별칭 후보
            }
            if (m_memory) m_memory->SetPending(blob->m_shape);                                       // 직접 고르면 기억하도록 대기
            reading.m_hasUnknownPlate = true;                                                        // 누군지 모르는 내 이름표
            reading.m_plateBounds = blob->m_bounds;                                                  // 이름표 위치
        }

        // 4) 화면 전체 OCR: 캐릭터 정보 창 · 글자 이름표 · "팁" (1초 안팎으로 무거워서 감시기가 허락할 때만 = 몇 초에 한 번)
        if (!isFullScanAllowed) return reading;                                                      // 건너뜀
        if (const auto whole = text.Recognize(frame))                                                // 원래 크기
        {
            CharacterReading parsed = Parse(*whole, width, height, &frame);                          // 해석
            if (parsed.m_signal == CharacterSignal::SIGNAL_NAME)                                     // 찾음
            {
                if (blob && m_memory && parsed.m_source == CharacterSource::SOURCE_NAMEPLATE)        // 이름표 글자로 찾았으면 모양도 기억
                    m_memory->Learn(Core::TextEncoding::WideToUTF8(parsed.m_name), blob->m_shape);
                return parsed;                                                                       // 결과
            }
            if (parsed.m_signal == CharacterSignal::SIGNAL_LOADING) return parsed;                   // "팁"으로 찾은 로딩 화면
            if (!parsed.m_unknownName.empty()) reading.m_unknownName = parsed.m_unknownName;          // 정보 창 이름이 더 믿을 만함
        }
        return reading;   // 못 찾음 (별칭 후보 · 대기 모양만)
    }
}
