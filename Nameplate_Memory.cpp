module LostArkMultiTool.Vision.NameplateMemory;  // 머리 위 이름표 모양 기억 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPConstants;  // 파일 이름 상수 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // 안전한 JSON 파일 저장 가져오기
import LostArkMultiTool.Services.JSONValue;  // JSON 트리 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        // ----- 저장 형식 -----
        constexpr std::string_view KEY_SHAPES{ "Shapes" };     // 모양 목록 키
        constexpr std::string_view KEY_NAME{ "Name" };         // 캐릭터 이름 키
        constexpr std::string_view KEY_WIDTH{ "Width" };       // 가로 키
        constexpr std::string_view KEY_HEIGHT{ "Height" };     // 세로 키
        constexpr std::string_view KEY_BITS{ "Bits" };         // 픽셀 (16진수 글자) 키
        constexpr int BITS_PER_HEX{ 4 };                       // 16진수 한 글자 = 픽셀 4개
        constexpr std::string_view HEX_DIGITS{ "0123456789abcdef" };   // 16진수 글자
        constexpr int MAX_SHAPE_SIDE{ 512 };                   // 모양 가로 · 세로 상한 (손상 파일 방어)

        // ----- 이름표 찾기 영역 (내 캐릭터는 화면 가운데, 이름표는 머리 위) -----
        constexpr double REGION_MIN_X_RATIO{ 0.30 };           // 가로 시작
        constexpr double REGION_MAX_X_RATIO{ 0.70 };           // 가로 끝
        constexpr double REGION_MIN_Y_RATIO{ 0.12 };           // 세로 시작
        constexpr double REGION_MAX_Y_RATIO{ 0.55 };           // 세로 끝
        constexpr double ANCHOR_Y_RATIO{ 0.36 };               // 이름표가 보통 있는 높이
        constexpr double MAX_CENTER_OFFSET_RATIO{ 0.15 };      // 이름 가운데가 화면 가운데에서 가로로 벗어나도 되는 정도 (칭호가 길면 오른쪽으로 밀림)

        // ----- 노란 글자 (내 이름 = 연한 노란색, 다른 사람 = 흰색, 칭호 = 주황, 길드 = 초록) -----
        constexpr int MIN_YELLOW_CHANNEL{ 150 };               // 빨강 · 초록 하한
        constexpr int MIN_BLUE_GAP{ 15 };                      // 파랑이 이만큼 낮아야 노란 기운
        constexpr int MAX_RED_GREEN_GAP{ 25 };                 // 빨강 · 초록 차이 상한 (주황 제외)
        constexpr int CHANNEL_BLUE{ 0 };                       // BGRA 파랑 위치
        constexpr int CHANNEL_GREEN{ 1 };                      // BGRA 초록 위치
        constexpr int CHANNEL_RED{ 2 };                        // BGRA 빨강 위치

        // ----- 글자 줄 · 덩어리 -----
        constexpr int MIN_ROW_PIXELS{ 2 };                     // 줄에 노란 픽셀이 이만큼 있어야 글자 줄
        constexpr int MAX_ROW_GAP{ 2 };                        // 글자 줄 안에서 비어도 되는 줄 수
        constexpr double MIN_TEXT_HEIGHT_RATIO{ 0.007 };       // 글자 높이 하한 (화면 세로 비율, 1080p ≈ 8px)
        constexpr double MAX_TEXT_HEIGHT_RATIO{ 0.030 };       // 글자 높이 상한 (1080p ≈ 32px)
        constexpr int MIN_TEXT_HEIGHT{ 7 };                    // 글자 높이 하한 (작은 창)
        constexpr double MAX_LETTER_GAP_RATIO{ 0.35 };         // 이름 안 글자 사이 빈칸 상한 (글자 높이 배수, 칭호와 이름 사이는 더 넓음)
        constexpr int MIN_LETTER_GAP{ 3 };                     // 글자 사이 빈칸 하한 (픽셀)
        constexpr double MIN_NAME_PIXELS_RATIO{ 3.0 };         // 이름 픽셀 수 하한 (글자 높이 배수)
        constexpr double MIN_NAME_WIDTH_RATIO{ 1.2 };          // 이름 가로 하한 (글자 높이 배수, 두 글자 이름)
        constexpr double MAX_NAME_WIDTH_RATIO{ 16.0 };         // 이름 가로 상한 (긴 영문 이름)
        constexpr double HALF{ 0.5 };                          // 절반

        // ----- 모양 비교 -----
        constexpr int MAX_HEIGHT_DIFFERENCE{ 1 };              // 세로가 이만큼까지 달라도 비교
        constexpr int MAX_WIDTH_DIFFERENCE{ 3 };               // 가로가 이만큼까지 달라도 비교
        constexpr int MAX_SHIFT{ 1 };                          // 맞춰 볼 어긋남 (픽셀)
        constexpr double AMBIGUOUS_MARGIN{ 0.05 };             // 두 캐릭터 점수가 이만큼 안이면 애매함
        constexpr double DICE_FACTOR{ 2.0 };                   // 다이스 계수 2|A∩B| / (|A|+|B|)

        // 픽셀이 내 이름표 색(연한 노란색)인지
        [[nodiscard]] bool IsOwnNamePixel(std::span<const std::uint8_t> pixels, std::size_t offset) noexcept
        {
            const int blue = pixels[offset + CHANNEL_BLUE];                            // 파랑
            const int green = pixels[offset + CHANNEL_GREEN];                          // 초록
            const int red = pixels[offset + CHANNEL_RED];                              // 빨강
            const int lower = std::min(red, green);                                    // 빨강 · 초록 중 작은 값
            return lower >= MIN_YELLOW_CHANNEL && blue + MIN_BLUE_GAP <= lower && std::abs(red - green) <= MAX_RED_GREEN_GAP;   // 연한 노란색
        }

        // 모양 → 16진수 글자 (픽셀 4개씩)
        [[nodiscard]] std::string ToHex(const NameplateShape& shape)
        {
            std::string text;                                                          // 결과
            text.reserve((shape.m_bits.size() + BITS_PER_HEX - 1) / BITS_PER_HEX);    // 자리 확보
            for (std::size_t index = 0; index < shape.m_bits.size(); index += BITS_PER_HEX)   // 4개씩
            {
                int value = 0;                                                         // 16진수 한 자리
                for (int bit = 0; bit < BITS_PER_HEX; ++bit)                           // 픽셀마다
                {
                    const std::size_t position = index + static_cast<std::size_t>(bit);   // 픽셀 번호
                    value = (value << 1) | (position < shape.m_bits.size() && shape.m_bits[position] != 0 ? 1 : 0);   // 비트 추가
                }
                text += HEX_DIGITS[static_cast<std::size_t>(value)];                   // 글자
            }
            return text;   // 16진수
        }

        // 16진수 글자 → 모양 (형식이 틀리면 빈 모양)
        [[nodiscard]] NameplateShape FromHex(int width, int height, const std::string& text)
        {
            NameplateShape shape;                                                      // 결과
            if (width <= 0 || height <= 0 || width > MAX_SHAPE_SIDE || height > MAX_SHAPE_SIDE) return shape;   // 크기 이상
            const std::size_t count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);       // 픽셀 수
            if (text.size() * BITS_PER_HEX < count) return shape;                      // 글자가 모자람
            shape.m_bits.reserve(count);                                               // 자리 확보
            for (char character : text)                                                // 글자마다
            {
                const auto digit = HEX_DIGITS.find(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));   // 값
                if (digit == std::string_view::npos) return NameplateShape{};          // 형식 오류
                for (int bit = BITS_PER_HEX - 1; bit >= 0 && shape.m_bits.size() < count; --bit)   // 높은 비트부터
                    shape.m_bits.push_back(static_cast<std::uint8_t>((digit >> bit) & 1)); // 픽셀
            }
            shape.m_width = width;                                                     // 가로
            shape.m_height = height;                                                   // 세로
            return shape;                                                              // 모양
        }
    }

    bool NameplateShape::IsEmpty() const noexcept
    {
        return m_width <= 0 || m_height <= 0 || m_bits.empty();   // 크기 · 픽셀 없음
    }

    int NameplateShape::GetPixelCount() const noexcept
    {
        return static_cast<int>(std::ranges::count_if(m_bits, [](std::uint8_t bit) { return bit != 0; }));   // 글자 픽셀 수
    }

    NameplateMemory::NameplateMemory(MemoryStorage storage)
        : m_storage{ storage }                                               // 파일에 둘지
        , m_fileStore{ Core::APPConstants::NAMEPLATE_MEMORY_FILE_NAME }      // 저장 파일
    {
        if (m_storage == MemoryStorage::STORAGE_NONE) return;                // 메모리에만 → 파일을 읽지 않음
        std::vector<Entry> loaded;                                           // 읽은 모양
        [[maybe_unused]] const auto result = m_fileStore.Load([&loaded](const Services::JSONValue& root)   // 파일 읽기
            {
                for (const Services::JSONValue& item : root.FindMember(KEY_SHAPES).GetItems())   // 모양마다
                {
                    const std::string& name = item.FindMember(KEY_NAME).GetString();             // 캐릭터
                    NameplateShape shape = FromHex(static_cast<int>(item.FindMember(KEY_WIDTH).GetNumber()),
                        static_cast<int>(item.FindMember(KEY_HEIGHT).GetNumber()), item.FindMember(KEY_BITS).GetString());   // 모양
                    if (name.empty() || shape.IsEmpty()) continue;                               // 잘못된 항목은 버림
                    auto entry = std::ranges::find(loaded, name, &Entry::m_name);                // 같은 캐릭터
                    if (entry == loaded.end()) { loaded.push_back(Entry{ name, {} }); entry = std::prev(loaded.end()); }   // 새 캐릭터
                    if (entry->m_shapes.size() < MAX_SHAPES_PER_CHARACTER) entry->m_shapes.push_back(std::move(shape));   // 추가
                }
            });
        m_entries = std::move(loaded);                                       // 반영 (파일이 없으면 빈 기억)
    }

    double NameplateMemory::Compare(const NameplateShape& a, const NameplateShape& b) noexcept
    {
        if (a.IsEmpty() || b.IsEmpty()) return 0.0;                                                    // 빈 모양
        if (std::abs(a.m_height - b.m_height) > MAX_HEIGHT_DIFFERENCE || std::abs(a.m_width - b.m_width) > MAX_WIDTH_DIFFERENCE) return 0.0;   // 크기가 많이 다름
        const int countA = a.GetPixelCount();                                                          // a 글자 픽셀
        const int countB = b.GetPixelCount();                                                          // b 글자 픽셀
        if (countA + countB == 0) return 0.0;                                                          // 글자 없음
        int bestOverlap = 0;                                                                           // 가장 많이 겹친 수
        for (int shiftY = -MAX_SHIFT; shiftY <= MAX_SHIFT; ++shiftY)                                   // 세로 어긋남
        {
            for (int shiftX = -MAX_SHIFT; shiftX <= MAX_SHIFT; ++shiftX)                               // 가로 어긋남
            {
                int overlap = 0;                                                                       // 겹친 수
                for (int y = 0; y < a.m_height; ++y)                                                   // a 줄마다
                {
                    const int otherY = y + shiftY;                                                     // b 줄
                    if (otherY < 0 || otherY >= b.m_height) continue;                                  // b 밖
                    for (int x = 0; x < a.m_width; ++x)                                                // a 칸마다
                    {
                        const int otherX = x + shiftX;                                                 // b 칸
                        if (otherX < 0 || otherX >= b.m_width) continue;                               // b 밖
                        if (a.m_bits[static_cast<std::size_t>(y) * a.m_width + x] != 0
                            && b.m_bits[static_cast<std::size_t>(otherY) * b.m_width + otherX] != 0) ++overlap;   // 둘 다 글자
                    }
                }
                bestOverlap = std::max(bestOverlap, overlap);                                          // 최대
            }
        }
        return DICE_FACTOR * bestOverlap / static_cast<double>(countA + countB);                       // 다이스 계수 (0 ~ 1)
    }

    std::optional<NameplateBlob> NameplateMemory::FindOwnNameplate(const FrameImage& frame)
    {
        const int width = frame.GetWidth();                                                            // 프레임 가로
        const int height = frame.GetHeight();                                                          // 프레임 세로
        const PixelRect region = ClampRect(PixelRect{ static_cast<int>(width * REGION_MIN_X_RATIO), static_cast<int>(height * REGION_MIN_Y_RATIO),
            static_cast<int>(width * (REGION_MAX_X_RATIO - REGION_MIN_X_RATIO)), static_cast<int>(height * (REGION_MAX_Y_RATIO - REGION_MIN_Y_RATIO)) },
            width, height);                                                                            // 찾을 영역
        if (region.m_width <= 0 || region.m_height <= 0) return std::nullopt;                          // 너무 작은 프레임
        const auto pixels = frame.GetPixels();                                                         // 픽셀
        const auto isYellow = [&](int x, int y)                                                        // 프레임 좌표가 노란 글자인지
            {
                return IsOwnNamePixel(pixels, (static_cast<std::size_t>(y) * width + x) * FrameImage::BYTES_PER_PIXEL);
            };

        // 1) 줄마다 노란 픽셀 수 → 글자 줄(띠) 나누기
        std::vector<int> rowCounts(static_cast<std::size_t>(region.m_height), 0);                     // 줄별 수
        for (int y = 0; y < region.m_height; ++y)                                                      // 줄마다
            for (int x = 0; x < region.m_width; ++x)                                                   // 칸마다
                if (isYellow(region.m_x + x, region.m_y + y)) ++rowCounts[static_cast<std::size_t>(y)];   // 노란 픽셀
        std::vector<std::pair<int, int>> bands;                                                        // (시작 줄, 끝 줄) 영역 기준
        for (int y = 0; y < region.m_height; ++y)                                                      // 줄마다
        {
            if (rowCounts[static_cast<std::size_t>(y)] < MIN_ROW_PIXELS) continue;                     // 글자 줄 아님
            if (!bands.empty() && y - bands.back().second <= MAX_ROW_GAP + 1) bands.back().second = y; // 앞 띠에 이어 붙임
            else bands.emplace_back(y, y);                                                             // 새 띠
        }

        const int minHeight = std::max(MIN_TEXT_HEIGHT, static_cast<int>(height * MIN_TEXT_HEIGHT_RATIO));   // 글자 높이 하한
        const int maxHeight = std::max(minHeight + 1, static_cast<int>(height * MAX_TEXT_HEIGHT_RATIO));    // 글자 높이 상한
        const double anchorY = height * ANCHOR_Y_RATIO;                                                // 기준 높이
        std::optional<NameplateBlob> best;                                                             // 고른 이름표
        double bestDistance = std::numeric_limits<double>::max();                                      // 기준 높이와의 거리
        for (const auto& [bandTop, bandBottom] : bands)                                                // 띠마다
        {
            const int bandHeight = bandBottom - bandTop + 1;                                           // 띠 높이
            if (bandHeight < minHeight || bandHeight > maxHeight) continue;                            // 글자 높이가 아님

            // 2) 띠 안 칸마다 노란 픽셀 수 → 글자 덩어리(이름) 나누기 (칭호 가장자리의 점들은 따로 떨어짐)
            std::vector<int> columnCounts(static_cast<std::size_t>(region.m_width), 0);               // 칸별 수
            for (int y = bandTop; y <= bandBottom; ++y)                                                // 띠 줄마다
                for (int x = 0; x < region.m_width; ++x)                                               // 칸마다
                    if (isYellow(region.m_x + x, region.m_y + y)) ++columnCounts[static_cast<std::size_t>(x)];
            const int maxGap = std::max(MIN_LETTER_GAP, static_cast<int>(std::lround(bandHeight * MAX_LETTER_GAP_RATIO)));   // 글자 사이 빈칸 상한
            int runStart = -1;                                                                         // 덩어리 시작
            int runEnd = -1;                                                                           // 덩어리 끝
            int runPixels = 0;                                                                         // 덩어리 픽셀 수
            int bestStart = -1;                                                                        // 가장 큰 덩어리 시작
            int bestEnd = -1;                                                                          // 가장 큰 덩어리 끝
            int bestPixels = 0;                                                                        // 가장 큰 덩어리 픽셀 수
            const auto closeRun = [&]                                                                  // 덩어리 하나 끝냄
                {
                    if (runStart >= 0 && runPixels > bestPixels) { bestStart = runStart; bestEnd = runEnd; bestPixels = runPixels; }
                    runStart = -1;
                    runPixels = 0;
                };
            for (int x = 0; x < region.m_width; ++x)                                                   // 칸마다
            {
                const int count = columnCounts[static_cast<std::size_t>(x)];                           // 노란 픽셀 수
                if (count == 0) { if (runStart >= 0 && x - runEnd > maxGap) closeRun(); continue; }   // 빈칸이 길면 덩어리 끝
                if (runStart < 0) runStart = x;                                                        // 새 덩어리
                runEnd = x;                                                                            // 끝 갱신
                runPixels += count;                                                                    // 픽셀 누적
            }
            closeRun();                                                                                // 마지막 덩어리
            if (bestStart < 0) continue;                                                               // 덩어리 없음

            const int nameWidth = bestEnd - bestStart + 1;                                             // 이름 가로
            if (bestPixels < bandHeight * MIN_NAME_PIXELS_RATIO) continue;                             // 글자가 너무 적음 (점 · 잡음)
            if (nameWidth < bandHeight * MIN_NAME_WIDTH_RATIO || nameWidth > bandHeight * MAX_NAME_WIDTH_RATIO) continue;   // 이름 너비가 아님
            const double centerX = region.m_x + bestStart + nameWidth * HALF;                          // 이름 가운데
            if (std::abs(centerX - width * HALF) > width * MAX_CENTER_OFFSET_RATIO) continue;          // 내 캐릭터 머리 위가 아님

            // 3) 이름 덩어리 안에서 실제 글자가 있는 줄만 남기기 (위아래 여백 자르기)
            int top = bandBottom;                                                                      // 글자 첫 줄
            int bottom = bandTop;                                                                      // 글자 끝 줄
            for (int y = bandTop; y <= bandBottom; ++y)                                                // 띠 줄마다
                for (int x = bestStart; x <= bestEnd; ++x)                                             // 덩어리 칸마다
                    if (isYellow(region.m_x + x, region.m_y + y)) { top = std::min(top, y); bottom = std::max(bottom, y); }
            const double centerY = region.m_y + (top + bottom) * HALF;                                 // 이름 세로 가운데
            const double distance = std::abs(centerY - anchorY);                                       // 기준 높이와의 거리
            if (distance >= bestDistance) continue;                                                    // 더 먼 후보

            NameplateBlob blob;                                                                        // 이름표
            blob.m_bounds = PixelRect{ region.m_x + bestStart, region.m_y + top, nameWidth, bottom - top + 1 };   // 프레임 기준 위치
            blob.m_shape.m_width = blob.m_bounds.m_width;                                              // 모양 가로
            blob.m_shape.m_height = blob.m_bounds.m_height;                                            // 모양 세로
            blob.m_shape.m_bits.reserve(static_cast<std::size_t>(blob.m_shape.m_width) * blob.m_shape.m_height);   // 자리 확보
            for (int y = 0; y < blob.m_shape.m_height; ++y)                                            // 줄마다
                for (int x = 0; x < blob.m_shape.m_width; ++x)                                         // 칸마다
                    blob.m_shape.m_bits.push_back(isYellow(blob.m_bounds.m_x + x, blob.m_bounds.m_y + y) ? 1 : 0);   // 글자 픽셀
            best = std::move(blob);                                                                    // 후보 기록
            bestDistance = distance;                                                                   // 거리 기록
        }
        return best;   // 가장 그럴듯한 이름표 (없으면 nullopt)
    }

    std::optional<std::string> NameplateMemory::Match(const NameplateShape& shape) const
    {
        const std::scoped_lock lock{ m_mutex };                                    // 보호
        const Entry* bestEntry = nullptr;                                          // 가장 닮은 캐릭터
        double bestScore = 0.0;                                                    // 그 점수
        double secondScore = 0.0;                                                  // 다른 캐릭터 중 가장 높은 점수
        for (const Entry& entry : m_entries)                                       // 캐릭터마다
        {
            double score = 0.0;                                                    // 이 캐릭터의 가장 닮은 모양
            for (const NameplateShape& known : entry.m_shapes) score = std::max(score, Compare(shape, known));   // 모양마다
            if (score > bestScore) { secondScore = bestScore; bestScore = score; bestEntry = &entry; }          // 1등 갱신
            else secondScore = std::max(secondScore, score);                       // 2등 갱신
        }
        if (bestEntry == nullptr || bestScore < MATCH_SCORE) return std::nullopt;  // 닮은 게 없음
        if (bestScore - secondScore < AMBIGUOUS_MARGIN) return std::nullopt;       // 두 캐릭터가 비슷하게 닮음 → 애매
        return bestEntry->m_name;                                                  // 캐릭터
    }

    void NameplateMemory::Learn(const std::string& name, const NameplateShape& shape)
    {
        if (name.empty() || shape.IsEmpty()) return;                               // 빈 값
        const std::scoped_lock lock{ m_mutex };                                    // 보호
        for (Entry& entry : m_entries)                                             // 다른 캐릭터에 잘못 기억된 같은 모양은 지움 (직접 고르면 바로잡힘)
        {
            if (entry.m_name == name) continue;                                    // 같은 캐릭터는 그대로
            std::erase_if(entry.m_shapes, [&shape](const NameplateShape& known) { return Compare(shape, known) >= MATCH_SCORE; });
        }
        std::erase_if(m_entries, [](const Entry& entry) { return entry.m_shapes.empty(); });   // 빈 캐릭터 정리
        auto entry = std::ranges::find(m_entries, name, &Entry::m_name);          // 이 캐릭터
        if (entry == m_entries.end()) { m_entries.push_back(Entry{ name, {} }); entry = std::prev(m_entries.end()); }   // 새 캐릭터
        const bool isKnown = std::ranges::any_of(entry->m_shapes, [&shape](const NameplateShape& known) { return Compare(shape, known) >= SAME_SHAPE_SCORE; });   // 이미 기억함
        if (isKnown) return;                                                       // 저장할 것 없음
        entry->m_shapes.push_back(shape);                                          // 새 모양
        if (entry->m_shapes.size() > MAX_SHAPES_PER_CHARACTER) entry->m_shapes.erase(entry->m_shapes.begin());   // 오래된 것부터 지움
        SaveLocked();                                                              // 저장
    }

    void NameplateMemory::SetPending(const NameplateShape& shape)
    {
        const std::scoped_lock lock{ m_mutex };                                    // 보호
        m_pending = shape;                                                         // 못 알아본 모양
        m_pendingAt = std::chrono::steady_clock::now();                            // 시각
    }

    bool NameplateMemory::LearnPending(const std::string& name, std::chrono::seconds maxAge)
    {
        std::optional<NameplateShape> pending;                                     // 꺼낸 모양
        {
            const std::scoped_lock lock{ m_mutex };                                // 보호
            if (!m_pending || std::chrono::steady_clock::now() - m_pendingAt > maxAge) return false;   // 없거나 오래됨
            pending = std::exchange(m_pending, std::nullopt);                      // 한 번만 씀
        }
        Learn(name, *pending);                                                     // 이 캐릭터로 기억 (잠금은 Learn이 다시 잡음)
        return true;                                                               // 기억함
    }

    std::size_t NameplateMemory::GetCharacterCount() const
    {
        const std::scoped_lock lock{ m_mutex };   // 보호
        return m_entries.size();                  // 캐릭터 수
    }

    void NameplateMemory::Clear()
    {
        const std::scoped_lock lock{ m_mutex };   // 보호
        m_entries.clear();                        // 모두 지움
        m_pending.reset();                        // 대기 모양도
        SaveLocked();                             // 저장
    }

    void NameplateMemory::RemoveMissing(const std::vector<std::string>& names)
    {
        if (names.empty()) return;                                                 // 원정대를 아직 모름 → 지우지 않음
        const std::scoped_lock lock{ m_mutex };                                    // 보호
        const std::size_t removed = std::erase_if(m_entries, [&names](const Entry& entry) { return std::ranges::find(names, entry.m_name) == names.end(); });   // 없는 캐릭터
        if (removed > 0) SaveLocked();                                             // 바뀌었으면 저장
    }

    void NameplateMemory::SaveLocked()
    {
        if (m_storage == MemoryStorage::STORAGE_NONE) return;                      // 메모리에만
        std::vector<Services::JSONValue> shapes;                                   // 모양 배열
        for (const Entry& entry : m_entries)                                       // 캐릭터마다
        {
            for (const NameplateShape& shape : entry.m_shapes)                     // 모양마다
            {
                shapes.push_back(Services::JSONValue::CreateObject(
                    { std::string{ KEY_NAME }, std::string{ KEY_WIDTH }, std::string{ KEY_HEIGHT }, std::string{ KEY_BITS } },
                    { Services::JSONValue::CreateString(entry.m_name), Services::JSONValue::CreateNumber(shape.m_width),
                      Services::JSONValue::CreateNumber(shape.m_height), Services::JSONValue::CreateString(ToHex(shape)) }));   // { "Name", "Width", "Height", "Bits" }
            }
        }
        try
        {
            m_fileStore.Save(Services::WriteJSONText(Services::JSONValue::CreateObject({ std::string{ KEY_SHAPES } }, { Services::JSONValue::CreateArray(std::move(shapes)) })));   // { "Shapes": [...] }
        }
        catch (const std::exception&)
        {
            // 저장 실패는 다음 실행 때 다시 배우면 되므로 조용히 넘김 (감시 스레드에서 알림을 띄우지 않음)
        }
    }
}
