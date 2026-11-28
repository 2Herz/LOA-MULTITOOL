module LostArkMultiTool.Vision.BuffDetector;  // 버프 타이머 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.BuffTemplates;  // 버프 종류 · 템플릿 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미 (tools/Make-BuffTemplates.py와 같은 기준)
    {
        constexpr double DEBUFF_LEFT_RATIO{ 0.35 };      // 디버프 줄 영역 왼쪽 (프레임 가로 비율)
        constexpr double DEBUFF_TOP_RATIO{ 0.14 };       // 위쪽 (세로 비율)
        constexpr double DEBUFF_RIGHT_RATIO{ 0.65 };     // 오른쪽
        constexpr double DEBUFF_BOTTOM_RATIO{ 0.32 };    // 아래쪽
        constexpr double BUFF_LEFT_RATIO{ 0.25 };        // 내 버프 줄 영역 왼쪽
        constexpr double BUFF_TOP_RATIO{ 0.76 };         // 위쪽
        constexpr double BUFF_RIGHT_RATIO{ 0.75 };       // 오른쪽
        constexpr double BUFF_BOTTOM_RATIO{ 0.885 };     // 아래쪽 (스킬 바 아이콘은 빼려고)
        constexpr double MIN_RUN_RATIO{ 0.018 };         // 테두리 한 줄 길이 하한 (프레임 세로 비율, 1080p 약 20px)
        constexpr double MAX_RUN_RATIO{ 0.034 };         // 상한 (약 37px)
        constexpr int AREA_PADDING{ 2 };                 // 테두리 색을 보는 범위 여유 (영역 밖 왼쪽 · 오른쪽 · 위)
        constexpr int AREA_BOTTOM_PADDING{ 60 };         // 아래로 더 보는 범위 (아이콘이 영역 아래로 이어질 때)
        constexpr double SIDE_COVER_RATIO{ 0.75 };       // 양옆 세로 테두리가 이만큼 이어져야 아이콘
        constexpr int SIDE_SEARCH{ 2 };                  // 세로 테두리를 찾는 좌우 폭
        constexpr int EDGE_ROW_SKIP{ 2 };                // 모서리 줄은 빼고 셈 (둥근 모서리)
        constexpr double DUPLICATE_RATIO{ 0.6 };         // 이만큼 가까우면 같은 아이콘
        constexpr int RED_MIN{ 110 };                    // 빨간 테두리: R 하한
        constexpr int RED_OVER_GREEN{ 55 };              // R - G 하한
        constexpr int RED_OVER_BLUE{ 50 };               // R - B 하한
        constexpr int GREEN_MIN{ 120 };                  // 초록 테두리: G 하한
        constexpr int GREEN_OVER_RED{ 25 };              // G - R 하한
        constexpr int GREEN_OVER_BLUE{ 40 };             // G - B 하한
        constexpr int GOLD_RED_MIN{ 140 };               // 금색 테두리: R 하한
        constexpr int GOLD_GREEN_MIN{ 60 };              // G 하한
        constexpr int GOLD_BLUE_MAX{ 80 };               // B 상한
        constexpr int GOLD_RED_OVER_BLUE{ 90 };          // R - B 하한
        constexpr int GOLD_GREEN_SLACK{ 10 };            // G가 R보다 이만큼까지는 커도 됨 (노란 윗변)
        constexpr int CREAM_MIN_LUMA{ 135 };             // 서폿 크림 테두리: 밝기 하한 (아래로 갈수록 어두운 금색이 됨)
        constexpr int CREAM_MIN_CHANNEL{ 90 };           // 가장 어두운 채널 하한
        constexpr int CREAM_MAX_RED_OVER_BLUE{ 110 };    // R - B 상한
        constexpr int CREAM_GREEN_SLACK{ 10 };           // G가 R보다 이만큼까지는 커도 됨
        constexpr int CREAM_DARK_LUMA{ 100 };            // 테두리 옆 짙은 선: 밝기 상한 (밝은 바닥과 구별)
        constexpr int CREAM_DARK_DISTANCE{ 2 };          // 짙은 선을 찾는 거리 (상하좌우 1 ~ 2칸)
        constexpr int RUN_MAX_GAP{ 6 };                  // 테두리 줄이 글자 · 빛에 가려 이만큼 끊겨도 이어 봄
        constexpr int NORM_SIZE{ 16 };                   // 종류 비교 크기
        constexpr int MIN_INSET{ 2 };                    // 테두리 빼는 폭 하한
        constexpr int INSET_DIVISOR{ 8 };                // 테두리 빼는 폭 = 아이콘 크기 / 8
        constexpr int RGB_CHANNELS{ 3 };                 // 템플릿 채널 수
        constexpr int CHANNEL_BLUE{ 0 };                 // BGRA 파랑
        constexpr int CHANNEL_GREEN{ 1 };                // BGRA 초록
        constexpr int CHANNEL_RED{ 2 };                  // BGRA 빨강
        constexpr double TINY{ 1e-6 };                   // 0으로 나누기 방지
        constexpr double BASE_ICON_SIZE{ 26.0 };         // 기준 아이콘 크기 (1080p, 글자 템플릿 크기 기준)
        constexpr double TEXT_BOTTOM_OFFSET{ 14.0 };     // 글자 줄 아래 = 아이콘 아래 + 14 (기준 크기)
        constexpr double TEXT_SIDE_MARGIN{ 8.0 };        // 글자 줄 좌우 여유 (기준 크기)
        constexpr double SCALE_TOLERANCE{ 0.08 };        // 기준 크기와 이만큼 다르면 글자 줄을 기준 크기로 맞춤
        constexpr double HALF{ 0.5 };                    // 절반 (가장 가까운 픽셀)
        constexpr int TEXT_ROWS{ 8 };                    // 글자 높이 (가장 글자가 많은 8줄만 남김)
        constexpr float TEXT_MIN_LUMA{ 125.0f };         // 글자 밝기 하한
        constexpr int TEXT_MIN_CHANNEL{ 90 };            // 글자 가장 어두운 채널 하한 (회색 · 흰 글자)
        constexpr int GREEN_TEXT_SLACK{ 8 };             // 버프 글자: G가 R보다 이만큼까지 작아도 됨 (초록빛 흰 글자)
        constexpr int YELLOW_TEXT_RED_MIN{ 170 };        // 서폿 노란 글자: R 하한
        constexpr int YELLOW_TEXT_GREEN_MIN{ 140 };      // G 하한
        constexpr int YELLOW_BLUE_NUMERATOR{ 7 };        // B ≤ G × 7 / 10
        constexpr int YELLOW_BLUE_DENOMINATOR{ 10 };
        constexpr float DARK_BACKGROUND_LUMA{ 100.0f };  // 글자 줄 밝기 중간값이 이보다 낮으면 어두운 바탕 (색 기준 없이 밝기 · 그림자만)
        constexpr int DARK_BACKGROUND_MIN_CHANNEL{ 60 }; // 어두운 바탕 글자: 가장 어두운 채널 하한 (연한 노란 글자까지)
        constexpr float SHADOW_CONTRAST{ 60.0f };        // 둘레(5x5)에 이만큼 더 어두운 픽셀이 있어야 글자 (그림자 테두리, 밝은 배경 거르기)
        constexpr int SHADOW_RADIUS{ 2 };                // 둘레 반경
        constexpr int UNIT_MIN_WIDTH{ 7 };               // 단위 글자(초 · 분) 너비 하한
        constexpr int UNIT_MAX_WIDTH{ 10 };              // 상한
        constexpr int DIGIT_MAX_WIDTH{ 6 };              // 숫자 너비 상한 (넘으면 두 글자가 붙은 것)
        constexpr int DIGIT_MIN_INK{ 6 };                // 숫자 잉크 하한 (적으면 바탕 잡음)
        constexpr int DIGIT_SPLIT_MARGIN{ 3 };           // 붙은 숫자를 자를 때 양끝에서 떨어질 폭
        constexpr int GROUP_MAX_GAP{ 3 };                // 같은 글자 줄로 보는 빈 칸 상한
        constexpr double CENTER_MAX_DISTANCE{ 14.0 };    // 아이콘 가운데에서 이만큼 안에 글자가 있어야 함
        constexpr int DOT_ROWS{ 3 };                     // 소수점: 아래 3줄 안
        constexpr int DOT_MAX_WIDTH{ 2 };                // 소수점 너비 상한
        constexpr int MIN_RUN_INK{ 2 };                  // 글자 열 덩어리 잉크 하한 (소수점 자리는 1도 받음)
        constexpr int DICE_SHIFT{ 2 };                   // 다이스 비교 때 밀어 볼 칸 (0 ~ 2)
        constexpr double DICE_DOUBLE{ 2.0 };             // 다이스 계수 = 2 × 겹침 / 합
        constexpr double GLYPH_MIN_SCORE{ 0.70 };        // 글자 일치 점수 하한
        constexpr char UNIT_SECONDS{ 's' };              // 초
        constexpr char UNIT_MINUTES{ 'm' };              // 분
        constexpr char DECIMAL_POINT{ '.' };             // 소수점
        constexpr char INK_MARK{ '#' };                  // 템플릿 글자 칸
        constexpr double SECONDS_PER_MINUTE{ 60.0 };     // 분 → 초

        // 테두리 색 종류
        enum class FrameColor { FRAME_RED, FRAME_GREEN, FRAME_GOLD, FRAME_CREAM };

        // 픽셀 R · G · B
        struct Rgb { int m_red{}; int m_green{}; int m_blue{}; };

        // 이진 모양 (줄 단위)
        struct BitImage
        {
            int m_width{};                 // 가로
            int m_height{};                // 세로
            std::vector<std::uint8_t> m_bits;   // 0 · 1

            [[nodiscard]] bool At(int x, int y) const noexcept { return m_bits[static_cast<std::size_t>(y) * m_width + x] != 0; }   // 칸
            [[nodiscard]] int ColumnInk(int x) const noexcept                                       // 열 잉크 수
            {
                int count = 0;
                for (int y = 0; y < m_height; ++y) count += At(x, y) ? 1 : 0;
                return count;
            }
            [[nodiscard]] int Ink(int left, int right) const noexcept                               // 열 범위 잉크 수
            {
                int count = 0;
                for (int x = left; x < right; ++x) count += ColumnInk(x);
                return count;
            }
        };

        // 나눈 글자 하나
        enum class GlyphKind { GLYPH_DIGIT, GLYPH_DOT, GLYPH_UNIT };
        struct GlyphPart { GlyphKind m_kind{}; BitImage m_shape; };

        // 프레임 픽셀 (범위 밖이면 검정)
        [[nodiscard]] Rgb PixelAt(const FrameImage& frame, int x, int y) noexcept
        {
            if (x < 0 || y < 0 || x >= frame.GetWidth() || y >= frame.GetHeight()) return {};      // 범위 밖
            const auto pixels = frame.GetPixels();                                                 // BGRA
            const std::size_t offset = (static_cast<std::size_t>(y) * frame.GetWidth() + x) * FrameImage::BYTES_PER_PIXEL;   // 위치
            return Rgb{ pixels[offset + CHANNEL_RED], pixels[offset + CHANNEL_GREEN], pixels[offset + CHANNEL_BLUE] };
        }

        // 테두리 색인지
        [[nodiscard]] bool IsFrameColor(const Rgb& pixel, FrameColor color) noexcept
        {
            switch (color)
            {
            case FrameColor::FRAME_RED:                                                            // 보스 디버프
                return pixel.m_red >= RED_MIN && pixel.m_red - pixel.m_green >= RED_OVER_GREEN && pixel.m_red - pixel.m_blue >= RED_OVER_BLUE;
            case FrameColor::FRAME_GREEN:                                                          // 내 버프
                return pixel.m_green >= GREEN_MIN && pixel.m_green - pixel.m_red >= GREEN_OVER_RED && pixel.m_green - pixel.m_blue >= GREEN_OVER_BLUE;
            case FrameColor::FRAME_GOLD:                                                           // 서폿 버프
                return pixel.m_red >= GOLD_RED_MIN && pixel.m_green >= GOLD_GREEN_MIN && pixel.m_blue <= GOLD_BLUE_MAX
                    && pixel.m_red - pixel.m_blue >= GOLD_RED_OVER_BLUE && pixel.m_red + GOLD_GREEN_SLACK >= pixel.m_green;
            case FrameColor::FRAME_CREAM:                                                          // 서폿 크림 테두리 (밝은 쪽만, 짙은 선 확인은 지도에서)
                return (pixel.m_red + pixel.m_green + pixel.m_blue) / static_cast<double>(RGB_CHANNELS) >= CREAM_MIN_LUMA
                    && std::min({ pixel.m_red, pixel.m_green, pixel.m_blue }) >= CREAM_MIN_CHANNEL
                    && pixel.m_red - pixel.m_blue <= CREAM_MAX_RED_OVER_BLUE && pixel.m_red >= pixel.m_green - CREAM_GREEN_SLACK;
            }
            return false;                                                                          // 모름
        }

        // 영역 안 테두리 색 지도 (범위 밖은 false)
        class FrameMask
        {
        public:
            FrameMask(const FrameImage& frame, const PixelRect& region, FrameColor color)
                : m_region{ ClampRect(region, frame.GetWidth(), frame.GetHeight()) }               // 프레임 안으로
            {
                m_bits.assign(static_cast<std::size_t>(std::max(0, m_region.m_width)) * std::max(0, m_region.m_height), 0);
                for (int y = 0; y < m_region.m_height; ++y)
                    for (int x = 0; x < m_region.m_width; ++x)
                        m_bits[static_cast<std::size_t>(y) * m_region.m_width + x] = IsFrameColor(PixelAt(frame, m_region.m_x + x, m_region.m_y + y), color) ? 1 : 0;
                if (color != FrameColor::FRAME_CREAM) return;                                      // 크림 테두리만 짙은 선 확인
                std::vector<std::uint8_t> dark(m_bits.size(), 0);                                  // 짙은 픽셀
                for (int y = 0; y < m_region.m_height; ++y)
                    for (int x = 0; x < m_region.m_width; ++x)
                    {
                        const Rgb pixel = PixelAt(frame, m_region.m_x + x, m_region.m_y + y);      // 픽셀
                        dark[static_cast<std::size_t>(y) * m_region.m_width + x] = (pixel.m_red + pixel.m_green + pixel.m_blue) / static_cast<double>(RGB_CHANNELS) < CREAM_DARK_LUMA ? 1 : 0;
                    }
                const auto isDark = [&](int x, int y) { return x >= 0 && y >= 0 && x < m_region.m_width && y < m_region.m_height && dark[static_cast<std::size_t>(y) * m_region.m_width + x] != 0; };
                for (int y = 0; y < m_region.m_height; ++y)
                    for (int x = 0; x < m_region.m_width; ++x)
                    {
                        std::uint8_t& bit = m_bits[static_cast<std::size_t>(y) * m_region.m_width + x];   // 밝은 선
                        if (bit == 0) continue;
                        bool isNearDark = false;                                                   // 1 ~ 2칸 옆 짙은 선
                        for (int distance = 1; distance <= CREAM_DARK_DISTANCE && !isNearDark; ++distance)
                            isNearDark = isDark(x - distance, y) || isDark(x + distance, y) || isDark(x, y - distance) || isDark(x, y + distance);
                        if (!isNearDark) bit = 0;                                                  // 밝은 바닥 → 테두리 아님
                    }
            }

            [[nodiscard]] bool At(int x, int y) const noexcept                                     // 프레임 좌표
            {
                const int localX = x - m_region.m_x;                                               // 지도 안 위치
                const int localY = y - m_region.m_y;
                if (localX < 0 || localY < 0 || localX >= m_region.m_width || localY >= m_region.m_height) return false;   // 범위 밖
                return m_bits[static_cast<std::size_t>(localY) * m_region.m_width + localX] != 0;
            }

            [[nodiscard]] bool AnyInRow(int left, int right, int y) const noexcept                 // left ~ right(포함) 중 하나라도
            {
                for (int x = left; x <= right; ++x) if (At(x, y)) return true;
                return false;
            }

        private:
            PixelRect m_region;                    // 지도 범위 (프레임 좌표)
            std::vector<std::uint8_t> m_bits;      // 색 맞음 (0 · 1)
        };

        // 아이콘 안쪽 → 16x16 RGB (상자 평균)
        [[nodiscard]] std::array<int, NORM_SIZE * NORM_SIZE * RGB_CHANNELS> Normalize(const FrameImage& frame, const PixelRect& icon)
        {
            std::array<int, NORM_SIZE * NORM_SIZE * RGB_CHANNELS> values{};                        // 결과
            const int inset = std::max(MIN_INSET, icon.m_height / INSET_DIVISOR);                  // 테두리 폭
            const int left = icon.m_x + inset;                                                     // 안쪽 영역
            const int top = icon.m_y + inset;
            const int width = icon.m_width - inset * 2;
            const int height = icon.m_height - inset * 2;
            if (width <= 0 || height <= 0) return values;                                          // 너무 작음
            for (int outY = 0; outY < NORM_SIZE; ++outY)
            {
                const int y0 = outY * height / NORM_SIZE;                                          // 칸 위
                const int y1 = std::max(y0 + 1, (outY + 1) * height / NORM_SIZE);                 // 칸 아래
                for (int outX = 0; outX < NORM_SIZE; ++outX)
                {
                    const int x0 = outX * width / NORM_SIZE;                                       // 칸 왼쪽
                    const int x1 = std::max(x0 + 1, (outX + 1) * width / NORM_SIZE);              // 칸 오른쪽
                    std::array<int, RGB_CHANNELS> sum{};                                           // 채널 합
                    for (int y = y0; y < y1; ++y)
                        for (int x = x0; x < x1; ++x)
                        {
                            const Rgb pixel = PixelAt(frame, left + x, top + y);                   // 픽셀
                            sum[0] += pixel.m_red;
                            sum[1] += pixel.m_green;
                            sum[2] += pixel.m_blue;
                        }
                    const int count = (y1 - y0) * (x1 - x0);                                       // 픽셀 수
                    const std::size_t offset = (static_cast<std::size_t>(outY) * NORM_SIZE + outX) * RGB_CHANNELS;   // 결과 위치
                    for (int channel = 0; channel < RGB_CHANNELS; ++channel) values[offset + channel] = sum[channel] / count;   // 평균
                }
            }
            return values;
        }

        // 두 값 묶음의 정규화 상관계수 (-1 ~ 1)
        template <typename Left, typename Right>
        [[nodiscard]] double Correlate(const Left& left, const Right& right) noexcept
        {
            const std::size_t count = std::min(left.size(), right.size());                         // 값 수
            if (count == 0) return 0.0;                                                            // 비교할 값 없음
            double leftMean = 0.0;                                                                 // 평균
            double rightMean = 0.0;
            for (std::size_t index = 0; index < count; ++index) { leftMean += left[index]; rightMean += right[index]; }
            leftMean /= static_cast<double>(count);
            rightMean /= static_cast<double>(count);
            double cross = 0.0;                                                                    // 곱 합
            double leftEnergy = 0.0;                                                               // 분산 합
            double rightEnergy = 0.0;
            for (std::size_t index = 0; index < count; ++index)
            {
                const double a = left[index] - leftMean;                                           // 편차
                const double b = right[index] - rightMean;
                cross += a * b;
                leftEnergy += a * a;
                rightEnergy += b * b;
            }
            return cross / (std::sqrt(leftEnergy * rightEnergy) + TINY);                           // 점수
        }

        // 아이콘 아래 글자 줄 → 글자 지도 (기준 크기로 맞춤) + 아이콘 가운데 열
        [[nodiscard]] std::pair<BitImage, int> MakeTextMask(const FrameImage& frame, const PixelRect& icon, bool isDebuff)
        {
            const double scale = icon.m_height / BASE_ICON_SIZE;                                   // 기준 대비 크기
            const int top = icon.GetBottom() + 1;                                                  // 글자 줄 위
            const int bottom = icon.GetBottom() + static_cast<int>(std::lround(TEXT_BOTTOM_OFFSET * scale));   // 아래
            const int left = icon.m_x - static_cast<int>(std::lround(TEXT_SIDE_MARGIN * scale));   // 왼쪽
            const int right = icon.GetRight() + static_cast<int>(std::lround(TEXT_SIDE_MARGIN * scale));       // 오른쪽
            int width = right - left;                                                              // 원래 크기
            int height = bottom - top;
            int outWidth = width;                                                                  // 맞춘 크기
            int outHeight = height;
            const bool isScaled = std::abs(scale - 1.0) > SCALE_TOLERANCE;                         // 크기 맞춤 필요
            if (isScaled)
            {
                outWidth = static_cast<int>(std::lround(width / scale));
                outHeight = static_cast<int>(std::lround(height / scale));
            }
            const int center = static_cast<int>(std::lround(TEXT_SIDE_MARGIN)) + static_cast<int>(std::lround(icon.m_width / scale * HALF));   // 아이콘 가운데 열
            if (outWidth <= 0 || outHeight <= 0) return { BitImage{}, center };                   // 빈 줄

            // 밝기 · 가장 어두운 채널 (가장 가까운 픽셀로 기준 크기 맞춤)
            std::vector<float> luma(static_cast<std::size_t>(outWidth) * outHeight);
            std::vector<int> lowest(luma.size());
            std::vector<std::uint8_t> isTextColor(luma.size(), 0);                                // 글자 색 (버프 줄: 초록빛 흰색 · 서폿 노란색)
            for (int y = 0; y < outHeight; ++y)
            {
                const int sourceY = isScaled ? std::min(height - 1, static_cast<int>((y + HALF) * height / outHeight)) : y;   // 원래 줄
                for (int x = 0; x < outWidth; ++x)
                {
                    const int sourceX = isScaled ? std::min(width - 1, static_cast<int>((x + HALF) * width / outWidth)) : x;  // 원래 열
                    const Rgb pixel = PixelAt(frame, left + sourceX, top + sourceY);               // 픽셀
                    const std::size_t index = static_cast<std::size_t>(y) * outWidth + x;
                    luma[index] = static_cast<float>(pixel.m_red + pixel.m_green + pixel.m_blue) / static_cast<float>(RGB_CHANNELS);   // 평균 밝기
                    lowest[index] = std::min({ pixel.m_red, pixel.m_green, pixel.m_blue });       // 가장 어두운 채널
                    if (isDebuff) isTextColor[index] = lowest[index] >= TEXT_MIN_CHANNEL ? 1 : 0;   // 디버프: 흰 · 분홍 글자
                    else
                    {
                        const bool isGreenish = pixel.m_green >= pixel.m_red - GREEN_TEXT_SLACK && lowest[index] >= TEXT_MIN_CHANNEL;   // 초록빛 흰 글자
                        const bool isYellow = pixel.m_red >= YELLOW_TEXT_RED_MIN && pixel.m_green >= YELLOW_TEXT_GREEN_MIN
                            && pixel.m_blue * YELLOW_BLUE_DENOMINATOR <= pixel.m_green * YELLOW_BLUE_NUMERATOR;   // 서폿 노란 글자 (B ≤ G × 0.7)
                        isTextColor[index] = isGreenish || isYellow ? 1 : 0;                      // 밝은 베이지 바닥(R > G)은 빠짐
                    }
                }
            }

            // 버프 줄 바탕이 어두우면 색 기준 없이 밝기 · 그림자만 (서폿 연한 노란 글자), 밝으면 글자 색 기준 (밝은 베이지 바닥 거르기)
            if (!isDebuff)
            {
                std::vector<float> sorted = luma;                                                  // 중간값 구하기
                const auto middle = sorted.begin() + static_cast<std::ptrdiff_t>(sorted.size() / 2);
                std::ranges::nth_element(sorted, middle);
                if (*middle < DARK_BACKGROUND_LUMA)                                                // 어두운 바탕
                    for (std::size_t index = 0; index < luma.size(); ++index) isTextColor[index] = lowest[index] >= DARK_BACKGROUND_MIN_CHANNEL ? 1 : 0;
            }

            // 글자 픽셀: 밝음 + 글자 색 + 둘레에 그림자
            BitImage mask{ outWidth, outHeight, std::vector<std::uint8_t>(luma.size(), 0) };
            for (int y = 0; y < outHeight; ++y)
                for (int x = 0; x < outWidth; ++x)
                {
                    const std::size_t index = static_cast<std::size_t>(y) * outWidth + x;
                    const float value = luma[index];                                               // 밝기
                    if (value < TEXT_MIN_LUMA || isTextColor[index] == 0) continue;               // 어두움 · 글자 색 아님
                    float local = value;                                                           // 둘레 최솟값
                    for (int dy = -SHADOW_RADIUS; dy <= SHADOW_RADIUS; ++dy)
                        for (int dx = -SHADOW_RADIUS; dx <= SHADOW_RADIUS; ++dx)
                        {
                            const int nx = std::clamp(x + dx, 0, outWidth - 1);                    // 가장자리는 늘려 씀
                            const int ny = std::clamp(y + dy, 0, outHeight - 1);
                            local = std::min(local, luma[static_cast<std::size_t>(ny) * outWidth + nx]);
                        }
                    if (local < value - SHADOW_CONTRAST) mask.m_bits[index] = 1;                   // 그림자 있음 → 글자
                }

            // 글자가 가장 많은 8줄만 남김
            int bestCount = -1;                                                                    // 가장 많은 잉크
            int bestTop = 0;                                                                       // 그 위치
            for (int rowTop = 0; rowTop < std::max(1, outHeight - TEXT_ROWS + 1); ++rowTop)
            {
                int count = 0;
                for (int y = rowTop; y < std::min(outHeight, rowTop + TEXT_ROWS); ++y)
                    for (int x = 0; x < outWidth; ++x) count += mask.At(x, y) ? 1 : 0;
                if (count > bestCount) { bestCount = count; bestTop = rowTop; }
            }
            for (int y = 0; y < outHeight; ++y)
                if (y < bestTop || y >= bestTop + TEXT_ROWS)
                    for (int x = 0; x < outWidth; ++x) mask.m_bits[static_cast<std::size_t>(y) * outWidth + x] = 0;   // 창 밖 지움
            return { std::move(mask), center };
        }

        // 열 범위 [left, right)를 위아래 빈 줄 없이 잘라낸 모양 + 위쪽 줄 번호
        [[nodiscard]] std::pair<BitImage, int> TrimColumns(const BitImage& bits, int left, int right)
        {
            int top = -1;                                                                          // 첫 잉크 줄
            int bottom = -1;                                                                       // 마지막 잉크 줄
            for (int y = 0; y < bits.m_height; ++y)
                for (int x = left; x < right; ++x)
                    if (bits.At(x, y)) { if (top < 0) top = y; bottom = y; break; }
            if (top < 0) return { BitImage{}, 0 };                                                 // 빈 범위
            BitImage shape{ right - left, bottom - top + 1, {} };                                  // 결과
            shape.m_bits.resize(static_cast<std::size_t>(shape.m_width) * shape.m_height);
            for (int y = 0; y < shape.m_height; ++y)
                for (int x = 0; x < shape.m_width; ++x)
                    shape.m_bits[static_cast<std::size_t>(y) * shape.m_width + x] = bits.At(left + x, top + y) ? 1 : 0;
            return { std::move(shape), top };
        }

        // lo ~ hi 열 중 잉크가 가장 적은 열 (같으면 want에 가까운 열, 그것도 같으면 왼쪽)
        [[nodiscard]] int FindCutColumn(const BitImage& bits, int lo, int hi, int want) noexcept
        {
            int bestX = lo;                                                                        // 결과
            std::pair<int, int> bestKey{ std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };   // (잉크, 거리)
            for (int x = std::max(0, lo); x <= std::min(hi, bits.m_width - 1); ++x)
            {
                const std::pair<int, int> key{ bits.ColumnInk(x), std::abs(x - want) };          // 비교 값
                if (key < bestKey) { bestKey = key; bestX = x; }
            }
            return bestX;
        }

        // 글자 줄 → 숫자 · 소수점 · 단위 글자 (못 나누면 nullopt)
        [[nodiscard]] std::optional<std::vector<GlyphPart>> SplitText(const BitImage& bits, int center)
        {
            // 잉크가 있는 열 덩어리
            std::vector<std::array<int, 2>> runs;                                                  // [왼쪽, 오른쪽)
            for (int x = 0; x < bits.m_width;)
            {
                if (bits.ColumnInk(x) == 0) { ++x; continue; }
                const int start = x;
                while (x < bits.m_width && bits.ColumnInk(x) > 0) ++x;
                bool hasLowInk = false;                                                            // 아래 3줄(소수점 자리)에 잉크
                for (int y = TEXT_ROWS - DOT_ROWS; y < bits.m_height && !hasLowInk; ++y)
                    for (int cx = start; cx < x; ++cx) if (bits.At(cx, y)) { hasLowInk = true; break; }
                if (bits.Ink(start, x) >= MIN_RUN_INK || hasLowInk) runs.push_back({ start, x });  // 한 점은 소수점 자리일 때만
            }
            if (runs.empty()) return std::nullopt;                                                 // 글자 없음

            // 아이콘 가운데에 가장 가까운 덩어리에서 빈 칸 3 이하로 이어진 덩어리들
            const auto runCenter = [&runs](std::size_t index) { return (runs[index][0] + runs[index][1]) * HALF; };   // 덩어리 가운데
            std::size_t nearest = 0;                                                               // 가장 가까운 덩어리
            for (std::size_t index = 1; index < runs.size(); ++index)
                if (std::abs(runCenter(index) - center) < std::abs(runCenter(nearest) - center)) nearest = index;
            if (std::abs(runCenter(nearest) - center) > CENTER_MAX_DISTANCE) return std::nullopt;   // 아이콘 아래 글자 아님
            std::size_t low = nearest;                                                             // 왼쪽 끝
            std::size_t high = nearest;                                                            // 오른쪽 끝
            while (low > 0 && runs[low][0] - runs[low - 1][1] <= GROUP_MAX_GAP) --low;
            while (high + 1 < runs.size() && runs[high + 1][0] - runs[high][1] <= GROUP_MAX_GAP) ++high;
            std::vector<std::array<int, 2>> group(runs.begin() + static_cast<std::ptrdiff_t>(low), runs.begin() + static_cast<std::ptrdiff_t>(high) + 1);

            // 단위 글자 (맨 오른쪽, 숫자와 붙었으면 오른쪽 10칸 근처에서 자름)
            const auto last = group.back();                                                        // 맨 오른쪽 덩어리
            const int lastWidth = last[1] - last[0];
            if (lastWidth < UNIT_MIN_WIDTH) return std::nullopt;                                   // 단위 글자 없음
            if (lastWidth > UNIT_MAX_WIDTH)
            {
                const int cut = FindCutColumn(bits, last[1] - UNIT_MAX_WIDTH - 1, last[1] - UNIT_MIN_WIDTH, last[1] - UNIT_MAX_WIDTH);   // 자를 열
                group.back() = { last[0], cut };                                                   // 숫자 쪽
                group.push_back({ cut + (bits.ColumnInk(cut) > 0 ? 0 : 1), last[1] });           // 단위 쪽
            }
            const auto unit = group.back();                                                        // 단위 글자 범위
            group.pop_back();

            // 숫자 · 소수점 (붙은 숫자는 가운데 근처 잉크가 가장 적은 열에서 자름)
            std::vector<GlyphPart> parts;                                                          // 결과
            std::deque<std::array<int, 2>> queue(group.begin(), group.end());                      // 처리할 범위
            while (!queue.empty())
            {
                const auto [left, right] = queue.front();                                          // 범위
                queue.pop_front();
                if (right - left > DIGIT_MAX_WIDTH)                                                // 숫자 두 개가 붙음
                {
                    const int cut = FindCutColumn(bits, left + DIGIT_SPLIT_MARGIN, right - DIGIT_SPLIT_MARGIN, (left + right) / 2);
                    if (bits.Ink(left, cut) >= DIGIT_MIN_INK && bits.Ink(cut, right) >= DIGIT_MIN_INK)   // 양쪽 다 글자일 때만
                    {
                        queue.push_front({ cut + (bits.ColumnInk(cut) > 0 ? 0 : 1), right });
                        queue.push_front({ left, cut });
                        continue;
                    }
                }
                if (right <= left || bits.Ink(left, right) == 0) continue;                         // 빈 범위
                auto [shape, top] = TrimColumns(bits, left, right);                                // 모양
                if (right - left <= DOT_MAX_WIDTH && top >= TEXT_ROWS - DOT_ROWS)                  // 소수점 크기
                {
                    if (!parts.empty()) parts.push_back({ GlyphKind::GLYPH_DOT, std::move(shape) });   // 숫자 뒤에만 (맨 앞 점은 잡음)
                }
                else if (bits.Ink(left, right) >= DIGIT_MIN_INK) parts.push_back({ GlyphKind::GLYPH_DIGIT, std::move(shape) });                  // 숫자 (적으면 잡음)
            }
            auto [unitShape, unitTop] = TrimColumns(bits, unit[0], unit[1]);                       // 단위 모양
            parts.push_back({ GlyphKind::GLYPH_UNIT, std::move(unitShape) });
            return parts;
        }

        // 템플릿 글자 → 이진 모양
        [[nodiscard]] BitImage ToBits(const BuffGlyphTemplate& glyph)
        {
            BitImage shape{ glyph.m_width, glyph.m_height, std::vector<std::uint8_t>(glyph.m_bits.size(), 0) };
            for (std::size_t index = 0; index < glyph.m_bits.size(); ++index) shape.m_bits[index] = glyph.m_bits[index] == INK_MARK ? 1 : 0;
            return shape;
        }

        // 다이스 계수 (b를 0 ~ 2칸 밀어 보며 가장 높은 값)
        [[nodiscard]] double Dice(const BitImage& a, const BitImage& b) noexcept
        {
            const int canvasHeight = std::max(a.m_height, b.m_height) + DICE_SHIFT;                // 판 크기
            const int canvasWidth = std::max(a.m_width, b.m_width) + DICE_SHIFT;
            int inkA = 0;                                                                          // a 잉크
            for (std::uint8_t bit : a.m_bits) inkA += bit;
            int inkB = 0;                                                                          // b 잉크
            for (std::uint8_t bit : b.m_bits) inkB += bit;
            if (inkA + inkB == 0) return 0.0;                                                      // 빈 모양
            double best = 0.0;                                                                     // 결과
            for (int shiftY = 0; shiftY <= DICE_SHIFT; ++shiftY)
                for (int shiftX = 0; shiftX <= DICE_SHIFT; ++shiftX)
                {
                    if (shiftY + b.m_height > canvasHeight || shiftX + b.m_width > canvasWidth) continue;   // 판 밖
                    int overlap = 0;                                                               // 겹친 잉크
                    for (int y = 0; y < b.m_height; ++y)
                        for (int x = 0; x < b.m_width; ++x)
                        {
                            if (!b.At(x, y)) continue;
                            const int ax = shiftX + x - 1;                                         // a는 (1, 1)에 놓임
                            const int ay = shiftY + y - 1;
                            if (ax >= 0 && ay >= 0 && ax < a.m_width && ay < a.m_height && a.At(ax, ay)) ++overlap;
                        }
                    best = std::max(best, DICE_DOUBLE * overlap / (inkA + inkB));                  // 다이스
                }
            return best;
        }

        // 글자 하나 알아보기 (모르면 nullopt)
        [[nodiscard]] std::optional<char> Classify(const GlyphPart& part)
        {
            if (part.m_kind == GlyphKind::GLYPH_DOT) return DECIMAL_POINT;                         // 소수점
            static const std::vector<std::pair<char, BitImage>> glyphs = []                        // 템플릿 모양 (한 번만)
                {
                    std::vector<std::pair<char, BitImage>> list;
                    for (const BuffGlyphTemplate& glyph : GetBuffGlyphs()) list.emplace_back(glyph.m_symbol, ToBits(glyph));
                    return list;
                }();
            const bool isUnit = part.m_kind == GlyphKind::GLYPH_UNIT;                              // 단위 글자인지
            char bestSymbol = 0;                                                                   // 가장 닮은 글자
            double bestScore = 0.0;                                                                // 그 점수
            for (const auto& [symbol, shape] : glyphs)
            {
                if ((symbol == UNIT_SECONDS || symbol == UNIT_MINUTES) != isUnit) continue;        // 숫자 · 단위 따로
                const double score = Dice(part.m_shape, shape);                                    // 점수
                if (score > bestScore) { bestScore = score; bestSymbol = symbol; }
            }
            if (bestScore < GLYPH_MIN_SCORE) return std::nullopt;                                  // 모름
            return bestSymbol;
        }
    }

    PixelRect BuffDetector::GetDebuffArea(int frameWidth, int frameHeight) noexcept
    {
        const int left = static_cast<int>(frameWidth * DEBUFF_LEFT_RATIO);                         // 왼쪽
        const int top = static_cast<int>(frameHeight * DEBUFF_TOP_RATIO);                          // 위
        return PixelRect{ left, top, static_cast<int>(frameWidth * DEBUFF_RIGHT_RATIO) - left, static_cast<int>(frameHeight * DEBUFF_BOTTOM_RATIO) - top };
    }

    PixelRect BuffDetector::GetBuffArea(int frameWidth, int frameHeight) noexcept
    {
        const int left = static_cast<int>(frameWidth * BUFF_LEFT_RATIO);                           // 왼쪽
        const int top = static_cast<int>(frameHeight * BUFF_TOP_RATIO);                            // 위
        return PixelRect{ left, top, static_cast<int>(frameWidth * BUFF_RIGHT_RATIO) - left, static_cast<int>(frameHeight * BUFF_BOTTOM_RATIO) - top };
    }

    std::vector<PixelRect> BuffDetector::FindIcons(const FrameImage& frame, const PixelRect& area, bool isDebuff)
    {
        std::vector<PixelRect> found;                                                              // 결과
        const PixelRect scan = ClampRect(area, frame.GetWidth(), frame.GetHeight());               // 훑을 영역
        if (scan.IsEmpty()) return found;                                                          // 빈 영역
        const double minRun = frame.GetHeight() * MIN_RUN_RATIO;                                   // 테두리 길이 하한
        const double maxRun = frame.GetHeight() * MAX_RUN_RATIO;                                   // 상한
        const PixelRect region{ scan.m_x - AREA_PADDING, scan.m_y - AREA_PADDING,
            scan.m_width + AREA_PADDING * 2, scan.m_height + AREA_PADDING + AREA_BOTTOM_PADDING }; // 색 지도 범위
        const std::vector<FrameColor> colors = isDebuff ? std::vector<FrameColor>{ FrameColor::FRAME_RED }
            : std::vector<FrameColor>{ FrameColor::FRAME_GREEN, FrameColor::FRAME_GOLD, FrameColor::FRAME_CREAM };   // 볼 테두리 색
        for (const FrameColor color : colors)
        {
            const FrameMask mask{ frame, region, color };                                          // 색 지도
            for (int y = scan.m_y; y < scan.GetBottom(); ++y)
            {
                std::vector<std::array<int, 2>> segments;                                          // 테두리 색 조각 [시작, 끝)
                for (int x = scan.m_x; x < scan.GetRight();)
                {
                    if (!mask.At(x, y)) { ++x; continue; }                                         // 테두리 색 아님
                    const int segmentStart = x;
                    while (x < scan.GetRight() && mask.At(x, y)) ++x;
                    segments.push_back({ segmentStart, x });
                }
                std::vector<std::array<int, 2>> spans;                                             // 조각을 이은 후보 줄 (시작, 길이)
                for (std::size_t first = 0; first < segments.size(); ++first)
                    for (std::size_t last = first; last < segments.size(); ++last)
                    {
                        if (last > first && segments[last][0] - segments[last - 1][1] > RUN_MAX_GAP) break;   // 많이 끊김 → 다른 줄
                        const int spanLength = segments[last][1] - segments[first][0];             // 이은 길이
                        if (spanLength > maxRun) break;                                            // 아이콘보다 김
                        if (spanLength >= minRun) spans.push_back({ segments[first][0], spanLength });   // 아이콘 크기
                    }
                for (const auto& [start, length] : spans)                                          // 후보마다
                {
                    std::optional<PixelRect> icon;                                                 // 찾은 아이콘
                    for (const int top : { y, y - length + 1 })                                    // 이 줄이 윗변 · 아랫변이라고 보고 확인
                    {
                        if (top < 0 || top + length + 1 >= frame.GetHeight()) continue;           // 화면 밖
                        const int rowCount = length - EDGE_ROW_SKIP * 2;                           // 셀 줄 수 (모서리 빼고)
                        if (rowCount <= 0) continue;
                        int leftCover = 0;                                                         // 왼쪽 세로 테두리 줄 수
                        int rightCover = 0;                                                        // 오른쪽
                        for (int row = top + EDGE_ROW_SKIP; row < top + length - EDGE_ROW_SKIP; ++row)
                        {
                            if (mask.AnyInRow(std::max(0, start - SIDE_SEARCH), start, row)) ++leftCover;
                            if (mask.AnyInRow(start + length - 1, start + length - 1 + SIDE_SEARCH, row)) ++rightCover;
                        }
                        if (leftCover >= rowCount * SIDE_COVER_RATIO && rightCover >= rowCount * SIDE_COVER_RATIO)
                        {
                            icon = PixelRect{ start - 1, top, length + 2, length + 2 };            // 테두리 포함 정사각형
                            break;
                        }
                    }
                    if (!icon) continue;                                                           // 아이콘 아님
                    const bool isDuplicate = std::ranges::any_of(found, [&icon](const PixelRect& other)   // 이미 찾은 아이콘
                        {
                            return std::abs(other.m_x - icon->m_x) < icon->m_width * DUPLICATE_RATIO && std::abs(other.m_y - icon->m_y) < icon->m_height * DUPLICATE_RATIO;
                        });
                    if (!isDuplicate) found.push_back(*icon);                                      // 추가
                }
            }
        }
        std::ranges::sort(found, [](const PixelRect& a, const PixelRect& b) { return std::pair{ a.m_x, a.m_y } < std::pair{ b.m_x, b.m_y }; });   // 왼쪽부터
        return found;
    }

    std::optional<std::pair<BuffKind, double>> BuffDetector::Identify(const FrameImage& frame, const PixelRect& icon)
    {
        const auto values = Normalize(frame, icon);                                                // 16x16 RGB
        std::optional<std::pair<BuffKind, double>> best;                                           // 가장 닮은 종류
        for (const BuffIconTemplate& pattern : GetBuffIcons())                                     // 템플릿마다
        {
            const double score = Correlate(values, pattern.m_rgb);                                 // 점수
            if (!best || score > best->second) best = std::pair{ pattern.m_kind, score };          // 더 닮음
        }
        if (!best || best->second < MIN_ICON_SCORE) return std::nullopt;                           // 모르는 아이콘
        return best;
    }

    std::optional<std::pair<std::size_t, double>> BuffDetector::IdentifySkill(const FrameImage& frame, const PixelRect& icon)
    {
        const auto values = Normalize(frame, icon);                                                // 16x16 RGB
        std::optional<std::pair<std::size_t, double>> best;                                        // 가장 닮은 스킬
        const auto skills = GetBuffSkills();                                                       // 스킬 목록
        for (std::size_t index = 0; index < skills.size(); ++index)
        {
            const double score = Correlate(values, skills[index].m_rgb);                           // 점수
            if (!best || score > best->second) best = std::pair{ index, score };
        }
        if (!best || best->second < MIN_SKILL_SCORE) return std::nullopt;                          // 모르는 아이콘
        return best;
    }

    std::optional<std::string> BuffDetector::ReadText(const FrameImage& frame, const PixelRect& icon, bool isDebuff)
    {
        const auto [mask, center] = MakeTextMask(frame, icon, isDebuff);                                     // 글자 지도
        if (mask.m_width == 0) return std::nullopt;                                                // 빈 줄
        const auto parts = SplitText(mask, center);                                                // 글자 나누기
        if (!parts) return std::nullopt;                                                           // 못 나눔
        std::string text;                                                                          // 결과
        for (const GlyphPart& part : *parts)
        {
            const auto symbol = Classify(part);                                                    // 글자
            if (!symbol) return std::nullopt;                                                      // 모르는 글자 (6 · 9 등)
            text += *symbol;
        }
        return text;                                                                               // "18s"
    }

    std::optional<double> BuffDetector::ParseSeconds(std::string_view text) noexcept
    {
        if (text.size() < 2) return std::nullopt;                                                  // 숫자 + 단위가 없음
        const char unit = text.back();                                                             // 단위
        if (unit != UNIT_SECONDS && unit != UNIT_MINUTES) return std::nullopt;                     // 단위 아님
        const std::string_view number = text.substr(0, text.size() - 1);                           // 숫자 부분
        if (number.front() == DECIMAL_POINT || number.back() == DECIMAL_POINT || std::ranges::count(number, DECIMAL_POINT) > 1) return std::nullopt;   // 소수점 위치 틀림
        double value = 0.0;                                                                        // 값
        const auto [end, error] = std::from_chars(number.data(), number.data() + number.size(), value);   // 숫자 읽기
        if (error != std::errc{} || end != number.data() + number.size()) return std::nullopt;     // 숫자 아님
        return unit == UNIT_MINUTES ? value * SECONDS_PER_MINUTE : value;                          // 초
    }

    BuffReading BuffDetector::Detect(const FrameImage& frame)
    {
        BuffReading reading;                                                                       // 결과
        for (const bool isDebuff : { true, false })                                                // 디버프 줄 · 내 버프 줄
        {
            const PixelRect area = isDebuff ? GetDebuffArea(frame.GetWidth(), frame.GetHeight()) : GetBuffArea(frame.GetWidth(), frame.GetHeight());
            for (const PixelRect& icon : FindIcons(frame, area, isDebuff))                         // 아이콘마다
            {
                const auto kind = Identify(frame, icon);                                           // 종류 (통합 아이콘)
                const auto skill = kind || isDebuff ? std::nullopt : IdentifySkill(frame, icon);   // 아니면 서폿 직업 스킬 버프
                if (!kind && !skill) continue;                                                     // 모르는 아이콘
                const auto text = ReadText(frame, icon, isDebuff);                                 // 남은 시간 글자
                BuffIcon found{ kind ? kind->first : BuffKind::BUFF_SUPPORT_SKILL, icon, isDebuff, kind ? kind->second : skill->second, text ? ParseSeconds(*text) : std::nullopt };
                if (skill) found.m_skillIndex = skill->first;                                      // 어떤 스킬인지
                reading.m_icons.push_back(found);
            }
        }
        return reading;
    }
}
