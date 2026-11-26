module LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 이미지 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr int CHANNEL_BLUE{ 0 };                 // BGRA에서 파랑 위치
        constexpr int CHANNEL_GREEN{ 1 };                // BGRA에서 초록 위치
        constexpr int CHANNEL_RED{ 2 };                  // BGRA에서 빨강 위치
        constexpr std::uint32_t LUMA_RED_WEIGHT{ 77 };   // BT.601 밝기 가중치 (0.299 * 256)
        constexpr std::uint32_t LUMA_GREEN_WEIGHT{ 150 };// BT.601 밝기 가중치 (0.587 * 256)
        constexpr std::uint32_t LUMA_BLUE_WEIGHT{ 29 };  // BT.601 밝기 가중치 (0.114 * 256)
        constexpr std::uint32_t LUMA_SHIFT{ 8 };         // 가중치 합 256으로 나누기 (비트 이동)
        constexpr std::uint8_t MAX_LEVEL{ 255 };         // 밝기 최댓값
        constexpr std::uint8_t MIN_LEVEL{ 0 };           // 밝기 최솟값
        constexpr int LEVEL_COUNT{ 256 };                // 밝기 단계 수 (히스토그램 칸 수)
        constexpr double HALF_PIXEL{ 0.5 };              // 픽셀 중심 보정 (쌍선형 보간)

        // 크기 · 버퍼 길이 계산 (음수 크기는 0으로)
        [[nodiscard]] std::size_t GetPixelCount(int width, int height) noexcept
        {
            return static_cast<std::size_t>(std::max(width, 0)) * static_cast<std::size_t>(std::max(height, 0));   // 가로 * 세로
        }

        // BGRA 한 픽셀 → 밝기
        [[nodiscard]] std::uint8_t ToLuma(const std::uint8_t* pixel) noexcept
        {
            const std::uint32_t sum = pixel[CHANNEL_RED] * LUMA_RED_WEIGHT     // 빨강 몫
                + pixel[CHANNEL_GREEN] * LUMA_GREEN_WEIGHT                     // 초록 몫
                + pixel[CHANNEL_BLUE] * LUMA_BLUE_WEIGHT;                      // 파랑 몫
            return static_cast<std::uint8_t>(sum >> LUMA_SHIFT);               // 256으로 나눈 밝기
        }
    }

    // ===== PixelRect =====

    bool PixelRect::IsEmpty() const noexcept
    {
        return m_width <= 0 || m_height <= 0;   // 가로나 세로가 0 이하면 빈 사각형
    }

    int PixelRect::GetRight() const noexcept
    {
        return m_x + m_width;   // 오른쪽 끝 (포함하지 않음)
    }

    int PixelRect::GetBottom() const noexcept
    {
        return m_y + m_height;   // 아래쪽 끝 (포함하지 않음)
    }

    PixelRect ClampRect(const PixelRect& rect, int width, int height) noexcept
    {
        const int left = std::clamp(rect.m_x, 0, std::max(width, 0));             // 왼쪽을 이미지 안으로
        const int top = std::clamp(rect.m_y, 0, std::max(height, 0));             // 위쪽을 이미지 안으로
        const int right = std::clamp(rect.GetRight(), left, std::max(width, 0));  // 오른쪽을 이미지 안으로 (왼쪽보다 작지 않게)
        const int bottom = std::clamp(rect.GetBottom(), top, std::max(height, 0));// 아래쪽을 이미지 안으로 (위쪽보다 작지 않게)
        return PixelRect{ left, top, right - left, bottom - top };                // 잘린 사각형
    }

    // ===== GrayImage =====

    GrayImage::GrayImage(int width, int height, std::uint8_t fill)
        : m_width{ std::max(width, 0) }                       // 음수 크기는 0
        , m_height{ std::max(height, 0) }                     // 음수 크기는 0
        , m_pixels(GetPixelCount(width, height), fill)        // 픽셀 버퍼를 fill로 채움
    {
    }

    int GrayImage::GetWidth() const noexcept
    {
        return m_width;   // 가로 크기
    }

    int GrayImage::GetHeight() const noexcept
    {
        return m_height;   // 세로 크기
    }

    bool GrayImage::IsEmpty() const noexcept
    {
        return m_pixels.empty();   // 픽셀이 없으면 빈 이미지
    }

    std::uint8_t GrayImage::At(int x, int y) const noexcept
    {
        if (x < 0 || y < 0 || x >= m_width || y >= m_height) return MIN_LEVEL;          // 범위 밖은 0 (경계 검사)
        return m_pixels[static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(x)];   // 픽셀 값
    }

    void GrayImage::Set(int x, int y, std::uint8_t value) noexcept
    {
        if (x < 0 || y < 0 || x >= m_width || y >= m_height) return;                     // 범위 밖은 무시 (경계 검사)
        m_pixels[static_cast<std::size_t>(y) * m_width + static_cast<std::size_t>(x)] = value;  // 픽셀 값 설정
    }

    std::span<const std::uint8_t> GrayImage::GetPixels() const noexcept
    {
        return m_pixels;   // 전체 픽셀
    }

    GrayImage GrayImage::Crop(const PixelRect& rect) const
    {
        const PixelRect area = ClampRect(rect, m_width, m_height);   // 이미지 안으로 자른 영역
        GrayImage result{ area.m_width, area.m_height };              // 결과 이미지
        for (int y = 0; y < area.m_height; ++y)                       // 줄마다
        {
            const auto source = m_pixels.begin() + static_cast<std::ptrdiff_t>((area.m_y + y) * m_width + area.m_x);   // 원본 줄 시작
            std::copy_n(source, area.m_width, result.m_pixels.begin() + static_cast<std::ptrdiff_t>(y * area.m_width)); // 한 줄 복사
        }
        return result;   // 잘라낸 이미지
    }

    GrayImage GrayImage::Resize(int width, int height) const
    {
        GrayImage result{ width, height };                    // 결과 이미지
        if (IsEmpty() || result.IsEmpty()) return result;      // 크기가 0이면 빈 이미지 그대로

        const double scaleX = static_cast<double>(m_width) / result.m_width;    // 가로 배율 (원본 / 결과)
        const double scaleY = static_cast<double>(m_height) / result.m_height;  // 세로 배율
        for (int y = 0; y < result.m_height; ++y)                               // 결과 줄마다
        {
            const double sourceY = std::clamp((y + HALF_PIXEL) * scaleY - HALF_PIXEL, 0.0, m_height - 1.0);   // 원본 Y (픽셀 중심 기준)
            const int y0 = static_cast<int>(sourceY);                           // 위 줄
            const int y1 = std::min(y0 + 1, m_height - 1);                      // 아래 줄
            const double weightY = sourceY - y0;                                // 아래 줄 가중치
            for (int x = 0; x < result.m_width; ++x)                            // 결과 칸마다
            {
                const double sourceX = std::clamp((x + HALF_PIXEL) * scaleX - HALF_PIXEL, 0.0, m_width - 1.0);  // 원본 X
                const int x0 = static_cast<int>(sourceX);                       // 왼쪽 칸
                const int x1 = std::min(x0 + 1, m_width - 1);                   // 오른쪽 칸
                const double weightX = sourceX - x0;                            // 오른쪽 칸 가중치
                const double top = At(x0, y0) * (1.0 - weightX) + At(x1, y0) * weightX;       // 위 줄 보간
                const double bottom = At(x0, y1) * (1.0 - weightX) + At(x1, y1) * weightX;    // 아래 줄 보간
                result.Set(x, y, static_cast<std::uint8_t>(std::lround(top * (1.0 - weightY) + bottom * weightY)));   // 세로 보간 후 저장
            }
        }
        return result;   // 크기를 바꾼 이미지
    }

    GrayImage GrayImage::Threshold(std::uint8_t level) const
    {
        GrayImage result{ m_width, m_height };   // 결과 이미지
        std::ranges::transform(m_pixels, result.m_pixels.begin(),
            [level](std::uint8_t value) { return value >= level ? MAX_LEVEL : MIN_LEVEL; });   // 기준 이상이면 255, 아니면 0
        return result;   // 이진 이미지
    }

    GrayImage GrayImage::Invert() const
    {
        GrayImage result{ m_width, m_height };   // 결과 이미지
        std::ranges::transform(m_pixels, result.m_pixels.begin(),
            [](std::uint8_t value) { return static_cast<std::uint8_t>(MAX_LEVEL - value); });  // 밝기 뒤집기
        return result;   // 뒤집은 이미지
    }

    std::uint8_t GrayImage::GetOtsuLevel() const noexcept
    {
        if (m_pixels.empty()) return MAX_LEVEL;                       // 빈 이미지는 기준 최댓값 (전부 0으로)

        std::array<std::uint64_t, LEVEL_COUNT> histogram{};           // 밝기별 픽셀 수
        for (std::uint8_t value : m_pixels) ++histogram[value];       // 히스토그램 채우기

        const double total = static_cast<double>(m_pixels.size());    // 전체 픽셀 수
        double weightedSum = 0.0;                                     // 밝기 * 개수 합
        for (int level = 0; level < LEVEL_COUNT; ++level) weightedSum += static_cast<double>(level) * histogram[level];   // 전체 합

        double backgroundCount = 0.0;          // 기준 아래 픽셀 수
        double backgroundSum = 0.0;            // 기준 아래 밝기 합
        double bestVariance = -1.0;            // 지금까지 가장 큰 집단 간 분산
        int bestLevel = 0;                     // 그때의 기준값
        for (int level = 0; level < LEVEL_COUNT; ++level)   // 기준값 후보마다
        {
            backgroundCount += static_cast<double>(histogram[level]);           // 아래 집단에 추가
            if (backgroundCount == 0.0) continue;                               // 아래 집단이 비면 건너뜀
            const double foregroundCount = total - backgroundCount;             // 위 집단 수
            if (foregroundCount == 0.0) break;                                  // 위 집단이 비면 끝
            backgroundSum += static_cast<double>(level) * histogram[level];     // 아래 집단 밝기 합
            const double backgroundMean = backgroundSum / backgroundCount;      // 아래 집단 평균
            const double foregroundMean = (weightedSum - backgroundSum) / foregroundCount;   // 위 집단 평균
            const double difference = backgroundMean - foregroundMean;          // 평균 차이
            const double variance = backgroundCount * foregroundCount * difference * difference;   // 집단 간 분산 (비례값)
            if (variance > bestVariance)                                        // 더 잘 나뉘면
            {
                bestVariance = variance;   // 최고값 갱신
                bestLevel = level;         // 기준값 갱신
            }
        }
        return static_cast<std::uint8_t>(std::min(bestLevel + 1, static_cast<int>(MAX_LEVEL)));   // 기준값 "초과"를 "이상"으로 맞춤
    }

    std::optional<PixelRect> GrayImage::FindInkBounds() const noexcept
    {
        int left = m_width;    // 가장 왼쪽 (처음엔 오른쪽 끝)
        int top = m_height;    // 가장 위 (처음엔 아래쪽 끝)
        int right = -1;        // 가장 오른쪽
        int bottom = -1;       // 가장 아래
        for (int y = 0; y < m_height; ++y)       // 줄마다
        {
            for (int x = 0; x < m_width; ++x)    // 칸마다
            {
                if (m_pixels[static_cast<std::size_t>(y) * m_width + x] == MIN_LEVEL) continue;   // 빈 칸은 건너뜀
                left = std::min(left, x);         // 왼쪽 갱신
                right = std::max(right, x);       // 오른쪽 갱신
                top = std::min(top, y);           // 위쪽 갱신
                bottom = std::max(bottom, y);     // 아래쪽 갱신
            }
        }
        if (right < 0) return std::nullopt;                                    // 잉크가 없음
        return PixelRect{ left, top, right - left + 1, bottom - top + 1 };     // 잉크를 감싸는 사각형
    }

    // ===== FrameImage =====

    FrameImage::FrameImage(int width, int height, std::vector<std::uint8_t> bgra)
        : m_width{ std::max(width, 0) }      // 음수 크기는 0
        , m_height{ std::max(height, 0) }    // 음수 크기는 0
        , m_pixels{ std::move(bgra) }        // 픽셀 버퍼 이동 (복사 없음)
    {
        if (m_pixels.size() != GetPixelCount(width, height) * BYTES_PER_PIXEL)    // 버퍼 길이가 크기와 안 맞으면
            throw std::invalid_argument("FrameImage: pixel buffer size mismatch");  // 잘못된 인자
    }

    FrameImage FrameImage::FromRows(int width, int height, std::span<const std::uint8_t> source, int rowPitch)
    {
        const std::size_t rowBytes = static_cast<std::size_t>(std::max(width, 0)) * BYTES_PER_PIXEL;   // 한 줄 바이트 수
        if (rowPitch < static_cast<int>(rowBytes)                                                       // 줄 간격이 줄보다 짧거나
            || source.size() < static_cast<std::size_t>(std::max(height - 1, 0)) * rowPitch + rowBytes)  // 원본이 모자라면
            throw std::invalid_argument("FrameImage: source buffer too small");                         // 버퍼 넘침 방지 (경계 검사)

        std::vector<std::uint8_t> pixels(rowBytes * static_cast<std::size_t>(std::max(height, 0)));    // 빈칸 없는 버퍼
        for (int y = 0; y < height; ++y)                                                                // 줄마다
        {
            const auto row = source.subspan(static_cast<std::size_t>(y) * rowPitch, rowBytes);          // 원본 줄 (경계 검사된 span)
            std::ranges::copy(row, pixels.begin() + static_cast<std::ptrdiff_t>(y * rowBytes));         // 한 줄 복사
        }
        return FrameImage{ width, height, std::move(pixels) };                                          // 프레임 만들기
    }

    int FrameImage::GetWidth() const noexcept
    {
        return m_width;   // 가로 크기
    }

    int FrameImage::GetHeight() const noexcept
    {
        return m_height;   // 세로 크기
    }

    bool FrameImage::IsEmpty() const noexcept
    {
        return m_pixels.empty();   // 픽셀이 없으면 빈 프레임
    }

    std::span<const std::uint8_t> FrameImage::GetPixels() const noexcept
    {
        return m_pixels;   // 전체 픽셀
    }

    FrameImage FrameImage::Crop(const PixelRect& rect) const
    {
        const PixelRect area = ClampRect(rect, m_width, m_height);                          // 이미지 안으로 자른 영역
        const std::span<const std::uint8_t> all{ m_pixels };                                 // 원본 전체
        const int rowPitch = m_width * BYTES_PER_PIXEL;                                      // 원본 줄 간격
        const std::size_t offset = static_cast<std::size_t>(area.m_y) * rowPitch
            + static_cast<std::size_t>(area.m_x) * BYTES_PER_PIXEL;                          // 시작 위치
        if (area.IsEmpty()) return FrameImage{};                                             // 빈 영역은 빈 프레임
        return FromRows(area.m_width, area.m_height, all.subspan(offset), rowPitch);         // 줄 단위 복사
    }

    GrayImage FrameImage::ToGray() const
    {
        return ToGray(PixelRect{ 0, 0, m_width, m_height });   // 전체 영역 변환
    }

    GrayImage FrameImage::ToGray(const PixelRect& rect) const
    {
        const PixelRect area = ClampRect(rect, m_width, m_height);   // 이미지 안으로 자른 영역
        GrayImage result{ area.m_width, area.m_height };              // 결과 이미지
        for (int y = 0; y < area.m_height; ++y)                       // 줄마다
        {
            const std::uint8_t* row = m_pixels.data()
                + (static_cast<std::size_t>(area.m_y + y) * m_width + area.m_x) * BYTES_PER_PIXEL;   // 원본 줄 시작 (경계 검사된 영역)
            for (int x = 0; x < area.m_width; ++x)                    // 칸마다
                result.Set(x, y, ToLuma(row + static_cast<std::size_t>(x) * BYTES_PER_PIXEL));      // 밝기 변환
        }
        return result;   // 흑백 이미지
    }
}
