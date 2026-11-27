module LostArkMultiTool.Vision.MaterialSlotDetector;  // 창고 · 소지품 슬롯 재료 인식기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.BossHealthDetector;  // 흰 글자 흑백 그림 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 결과 형식 가져오기
import LostArkMultiTool.Vision.SlotTemplates;  // 아이콘 · 숫자 글자 템플릿 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 3배 OCR 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr int DOWN_SCALE{ 4 };                  // 후보 찾기용 축소 배율
        constexpr double COARSE_MIN_SCORE{ 0.70 };      // 축소 화면 후보 점수 하한
        constexpr int COARSE_SUPPRESS_RADIUS{ 2 };      // 축소 화면에서 이 칸 안의 더 낮은 후보는 버림
        constexpr std::size_t MAX_CANDIDATES{ 24 };     // 아이콘마다 다시 맞춰 볼 후보 수 상한
        constexpr int REFINE_RADIUS{ 4 };               // 원래 크기에서 다시 맞춰 볼 범위 (픽셀)
        constexpr double FINE_MIN_SCORE{ 0.88 };        // 원래 크기 점수 하한 (샘플: 맞는 슬롯 0.92 이상, 다른 슬롯 0.77 이하)
        constexpr double MAX_COLOR_DIFF{ 12.0 };        // 평균 색 차이 상한 (흑백 모양이 비슷한 다른 색 아이콘 거르기)
        constexpr double OVERLAP_RATIO{ 0.5 };          // 서로 다른 아이콘이 이만큼 겹치면 점수 높은 쪽만
        constexpr int BAND_TOP_OFFSET{ 13 };            // 숫자 띠 위쪽 = 아이콘 위쪽 - 13
        constexpr int BAND_BOTTOM_OFFSET{ 1 };          // 숫자 띠 아래쪽 = 아이콘 위쪽 - 1
        constexpr int BAND_SIDE_MARGIN{ 4 };            // 숫자 띠 왼쪽 여유
        constexpr int BAND_RIGHT_TRIM{ 2 };             // 숫자 띠 오른쪽 = 아이콘 오른쪽 - 2 (슬롯 테두리가 '1'처럼 읽히지 않게)
        constexpr int GLYPH_ROWS{ 10 };                 // 글자 줄 창 높이 (가장 글자가 많은 10줄만 남겨 아이콘 빛 지우기)
        constexpr int TEXT_MIN_LEVEL{ 150 };            // 흰 글자: 세 채널 모두 이 값 이상
        constexpr int TEXT_MAX_SPREAD{ 70 };            // 흰 글자: 채널 차이 이 값 이하 (노란 · 파란 아이콘 빛 거르기)
        constexpr int GLYPH_MIN_HEIGHT{ 7 };            // 글자 높이 하한
        constexpr int GLYPH_MAX_HEIGHT{ 10 };           // 글자 높이 상한
        constexpr int GLYPH_MIN_WIDTH{ 2 };             // 글자 너비 하한
        constexpr int GLYPH_MAX_WIDTH{ 7 };             // 글자 너비 상한
        constexpr int GLYPH_MAX_GAP{ 6 };               // 숫자 사이 빈칸 상한 (넘으면 다른 글자 덩어리, 폭이 좁은 "11" 사이가 5)
        constexpr int GLYPH_CANVAS_MARGIN{ 2 };         // 글자 비교 판 여유 (위아래 · 좌우 한 칸씩 밀어 봄)
        constexpr double GLYPH_MIN_SCORE{ 0.70 };       // 글자 일치 점수 하한 (다이스 계수)
        constexpr double GLYPH_STRONG_SCORE{ 0.85 };    // 모든 글자가 이 이상이면 확실 → 3배 OCR 없이 받음 (애매하면 3배 OCR과 같아야 받음)
        constexpr char OVERFLOW_MARK{ '+' };            // "9999+" 넘침 표시
        constexpr char INK_MARK{ '#' };                 // 템플릿 글자 칸
        constexpr std::size_t MAX_COUNT_DIGITS{ 6 };    // 슬롯 숫자 자릿수 상한
        constexpr int RGB_CHANNELS{ 3 };                // 템플릿 픽셀 채널 수
        constexpr int CHANNEL_BLUE{ 0 };                // BGRA 파랑
        constexpr int CHANNEL_GREEN{ 1 };               // BGRA 초록
        constexpr int CHANNEL_RED{ 2 };                 // BGRA 빨강
        constexpr float RED_WEIGHT{ 0.299f };           // 밝기 가중치 (BT.601)
        constexpr float GREEN_WEIGHT{ 0.587f };
        constexpr float BLUE_WEIGHT{ 0.114f };
        constexpr int DECIMAL_BASE{ 10 };               // 십진수
        constexpr double TINY{ 1e-6 };                  // 0으로 나누기 방지
        constexpr double MIN_PIXEL_VARIANCE{ 1.0 };     // 창 픽셀 분산 하한 (거의 단색인 창은 계산 오차로 점수가 튀므로 0점)

        // 실수 흑백 그림 (NCC 계산용)
        struct LumaImage
        {
            int m_width{};                 // 가로
            int m_height{};                // 세로
            std::vector<float> m_pixels;   // 밝기

            [[nodiscard]] float At(int x, int y) const noexcept { return m_pixels[static_cast<std::size_t>(y) * m_width + x]; }   // 픽셀
        };

        // 글자 덩어리 하나 (이진 모양)
        struct Glyph
        {
            int m_left{};                  // 띠 안 왼쪽
            int m_right{};                 // 띠 안 오른쪽 (포함 안 함)
            int m_width{};                 // 가로
            int m_height{};                // 세로
            std::vector<bool> m_bits;      // 모양
        };

        // 컬러 프레임 → 밝기
        [[nodiscard]] LumaImage ToLuma(const FrameImage& frame)
        {
            LumaImage image{ frame.GetWidth(), frame.GetHeight(), {} };                     // 결과
            image.m_pixels.resize(static_cast<std::size_t>(image.m_width) * image.m_height); // 자리
            const auto pixels = frame.GetPixels();                                           // BGRA
            for (std::size_t index = 0; index < image.m_pixels.size(); ++index)              // 픽셀마다
            {
                const std::size_t offset = index * FrameImage::BYTES_PER_PIXEL;              // 바이트 위치
                image.m_pixels[index] = pixels[offset + CHANNEL_RED] * RED_WEIGHT + pixels[offset + CHANNEL_GREEN] * GREEN_WEIGHT
                    + pixels[offset + CHANNEL_BLUE] * BLUE_WEIGHT;                           // 밝기
            }
            return image;
        }

        // 템플릿 RGB → 밝기
        [[nodiscard]] LumaImage ToLuma(const SlotIconTemplate& icon)
        {
            LumaImage image{ icon.m_width, icon.m_height, {} };                              // 결과
            image.m_pixels.resize(static_cast<std::size_t>(icon.m_width) * icon.m_height);   // 자리
            for (std::size_t index = 0; index < image.m_pixels.size(); ++index)              // 픽셀마다
            {
                const std::size_t offset = index * RGB_CHANNELS;                             // R 위치
                image.m_pixels[index] = icon.m_rgb[offset] * RED_WEIGHT + icon.m_rgb[offset + 1] * GREEN_WEIGHT + icon.m_rgb[offset + 2] * BLUE_WEIGHT;
            }
            return image;
        }

        // 상자 평균으로 factor배 줄이기
        [[nodiscard]] LumaImage Shrink(const LumaImage& source, int factor)
        {
            LumaImage image{ source.m_width / factor, source.m_height / factor, {} };        // 결과 크기
            image.m_pixels.assign(static_cast<std::size_t>(image.m_width) * image.m_height, 0.0f);
            const float area = static_cast<float>(factor * factor);                          // 칸 넓이
            for (int y = 0; y < image.m_height; ++y)
                for (int x = 0; x < image.m_width; ++x)
                {
                    float sum = 0.0f;                                                        // 칸 합
                    for (int dy = 0; dy < factor; ++dy)
                        for (int dx = 0; dx < factor; ++dx) sum += source.At(x * factor + dx, y * factor + dy);
                    image.m_pixels[static_cast<std::size_t>(y) * image.m_width + x] = sum / area;   // 평균
                }
            return image;
        }

        // (x, y)에 pattern을 겹친 정규화 상관계수 (-1 ~ 1)
        [[nodiscard]] double Correlate(const LumaImage& image, const LumaImage& pattern, float patternMean, double patternNorm, int x, int y) noexcept
        {
            double sum = 0.0;                                                                // 창 합
            for (int py = 0; py < pattern.m_height; ++py)
                for (int px = 0; px < pattern.m_width; ++px) sum += image.At(x + px, y + py);
            const double count = static_cast<double>(pattern.m_width) * pattern.m_height;    // 픽셀 수
            const double mean = sum / count;                                                 // 창 평균
            double cross = 0.0;                                                              // 곱 합
            double energy = 0.0;                                                             // 창 분산 합
            for (int py = 0; py < pattern.m_height; ++py)
                for (int px = 0; px < pattern.m_width; ++px)
                {
                    const double value = image.At(x + px, y + py) - mean;                    // 창 편차
                    cross += value * (pattern.At(px, py) - patternMean);                     // 곱
                    energy += value * value;                                                 // 분산
                }
            return cross / (std::sqrt(energy) * patternNorm + TINY);                         // 점수
        }

        // 창 합을 한 번에 구하는 표 (합 · 제곱합, 가로 · 세로 한 칸씩 큼)
        struct SumTable
        {
            int m_stride{};                    // 한 줄 칸 수 (가로 + 1)
            std::vector<double> m_sum;         // 누적 합
            std::vector<double> m_squareSum;   // 누적 제곱합

            // (x, y)부터 width × height 창의 (합, 제곱합)
            [[nodiscard]] std::pair<double, double> Get(int x, int y, int width, int height) const noexcept
            {
                const auto at = [this](const std::vector<double>& table, int column, int row) { return table[static_cast<std::size_t>(row) * m_stride + column]; };
                const auto box = [&](const std::vector<double>& table)
                    { return at(table, x + width, y + height) - at(table, x, y + height) - at(table, x + width, y) + at(table, x, y); };
                return { box(m_sum), box(m_squareSum) };
            }
        };

        // 밝기 그림 → 창 합 표
        [[nodiscard]] SumTable MakeSumTable(const LumaImage& image)
        {
            SumTable table{ image.m_width + 1, {}, {} };                                     // 결과
            const std::size_t size = static_cast<std::size_t>(image.m_width + 1) * (image.m_height + 1);   // 칸 수
            table.m_sum.assign(size, 0.0);
            table.m_squareSum.assign(size, 0.0);
            for (int y = 0; y < image.m_height; ++y)
            {
                double rowSum = 0.0;                                                         // 이 줄 누적
                double rowSquare = 0.0;
                for (int x = 0; x < image.m_width; ++x)
                {
                    const double value = image.At(x, y);                                     // 밝기
                    rowSum += value;
                    rowSquare += value * value;
                    const std::size_t index = static_cast<std::size_t>(y + 1) * table.m_stride + x + 1;   // 표 칸
                    const std::size_t above = static_cast<std::size_t>(y) * table.m_stride + x + 1;       // 위 칸
                    table.m_sum[index] = table.m_sum[above] + rowSum;
                    table.m_squareSum[index] = table.m_squareSum[above] + rowSquare;
                }
            }
            return table;
        }

        // 창 합 표를 쓴 빠른 상관계수 (Σ 창 × (템플릿 - 평균) / (창 편차 크기 × 템플릿 편차 크기))
        [[nodiscard]] double CorrelateFast(const LumaImage& image, const SumTable& table, const std::vector<float>& centeredPattern,
            int patternWidth, int patternHeight, double patternNorm, int x, int y) noexcept
        {
            double cross = 0.0;                                                              // 곱 합 (템플릿 편차 합이 0이라 창 평균을 뺄 필요 없음)
            for (int py = 0; py < patternHeight; ++py)
            {
                const float* row = &image.m_pixels[static_cast<std::size_t>(y + py) * image.m_width + x];   // 창 한 줄
                const float* pattern = &centeredPattern[static_cast<std::size_t>(py) * patternWidth];       // 템플릿 한 줄
                for (int px = 0; px < patternWidth; ++px) cross += row[px] * pattern[px];
            }
            const auto [sum, squareSum] = table.Get(x, y, patternWidth, patternHeight);      // 창 합
            const double count = static_cast<double>(patternWidth) * patternHeight;         // 픽셀 수
            const double energy = squareSum - sum * sum / count;                             // 창 분산 합
            if (energy < MIN_PIXEL_VARIANCE * count) return 0.0;                             // 거의 단색 → 아이콘 아님
            return cross / (std::sqrt(energy) * patternNorm + TINY);                         // 점수
        }

        // 템플릿 평균 · 편차 크기
        [[nodiscard]] std::pair<float, double> GetStats(const LumaImage& pattern) noexcept
        {
            const float mean = std::accumulate(pattern.m_pixels.begin(), pattern.m_pixels.end(), 0.0f) / static_cast<float>(pattern.m_pixels.size());   // 평균
            double energy = 0.0;                                                             // 분산 합
            for (float value : pattern.m_pixels) energy += (value - mean) * (value - mean);
            return { mean, std::sqrt(energy) };                                              // (평균, 크기)
        }

        // 프레임 영역 평균 색 (R · G · B)
        [[nodiscard]] std::array<double, 3> GetMeanColor(const FrameImage& frame, const PixelRect& rect) noexcept
        {
            std::array<double, 3> sum{};                                                     // 합
            const auto pixels = frame.GetPixels();                                           // BGRA
            for (int y = rect.m_y; y < rect.GetBottom(); ++y)
                for (int x = rect.m_x; x < rect.GetRight(); ++x)
                {
                    const std::size_t offset = (static_cast<std::size_t>(y) * frame.GetWidth() + x) * FrameImage::BYTES_PER_PIXEL;
                    sum[0] += pixels[offset + CHANNEL_RED];                                  // 빨강
                    sum[1] += pixels[offset + CHANNEL_GREEN];                                // 초록
                    sum[2] += pixels[offset + CHANNEL_BLUE];                                 // 파랑
                }
            const double count = static_cast<double>(rect.m_width) * rect.m_height;          // 픽셀 수
            for (double& value : sum) value /= count;                                        // 평균
            return sum;
        }

        // 템플릿 평균 색 (R · G · B)
        [[nodiscard]] std::array<double, 3> GetMeanColor(const SlotIconTemplate& icon) noexcept
        {
            std::array<double, 3> sum{};                                                     // 합
            for (std::size_t index = 0; index < icon.m_rgb.size(); ++index) sum[index % RGB_CHANNELS] += icon.m_rgb[index];
            const double count = static_cast<double>(icon.m_width) * icon.m_height;          // 픽셀 수
            for (double& value : sum) value /= count;                                        // 평균
            return sum;
        }

        // 두 사각형 겹친 넓이 / 작은 쪽 넓이
        [[nodiscard]] double GetOverlap(const PixelRect& first, const PixelRect& second) noexcept
        {
            const int width = std::min(first.GetRight(), second.GetRight()) - std::max(first.m_x, second.m_x);    // 겹친 가로
            const int height = std::min(first.GetBottom(), second.GetBottom()) - std::max(first.m_y, second.m_y); // 겹친 세로
            if (width <= 0 || height <= 0) return 0.0;                                                            // 안 겹침
            const double smaller = std::min(static_cast<double>(first.m_width) * first.m_height, static_cast<double>(second.m_width) * second.m_height);
            return width * height / smaller;                                                                      // 비율
        }

        // 띠에서 흰 글자 덩어리들 (왼쪽부터)
        [[nodiscard]] std::vector<Glyph> SplitGlyphs(const FrameImage& band)
        {
            const int width = band.GetWidth();                                               // 가로
            const int height = band.GetHeight();                                             // 세로
            const auto pixels = band.GetPixels();                                            // BGRA
            std::vector<bool> ink(static_cast<std::size_t>(width) * height);                 // 흰 글자 칸
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                {
                    const std::size_t offset = (static_cast<std::size_t>(y) * width + x) * FrameImage::BYTES_PER_PIXEL;
                    const int blue = pixels[offset + CHANNEL_BLUE];                          // 파랑
                    const int green = pixels[offset + CHANNEL_GREEN];                        // 초록
                    const int red = pixels[offset + CHANNEL_RED];                            // 빨강
                    const int low = std::min({ red, green, blue });                          // 가장 어두운 채널
                    const int high = std::max({ red, green, blue });                         // 가장 밝은 채널
                    ink[static_cast<std::size_t>(y) * width + x] = low >= TEXT_MIN_LEVEL && high - low <= TEXT_MAX_SPREAD;   // 흰 글자
                }
            int bestTop = 0;                                                                 // 글자 줄 창 위쪽
            int bestCount = -1;                                                              // 그 창의 글자 칸 수
            for (int top = 0; top + GLYPH_ROWS <= std::max(height, GLYPH_ROWS); ++top)      // 창마다
            {
                int count = 0;                                                               // 칸 수
                for (int y = top; y < std::min(top + GLYPH_ROWS, height); ++y)
                    for (int x = 0; x < width; ++x) count += ink[static_cast<std::size_t>(y) * width + x] ? 1 : 0;
                if (count > bestCount) { bestCount = count; bestTop = top; }                 // 더 많음
            }
            for (int y = 0; y < height; ++y)                                                 // 창 밖 줄은 지움 (숫자 아래 · 위로 붙은 빛 때문에 숫자가 이어지지 않게)
                if (y < bestTop || y >= bestTop + GLYPH_ROWS)
                    for (int x = 0; x < width; ++x) ink[static_cast<std::size_t>(y) * width + x] = false;
            const auto isInkColumn = [&](int x) { for (int y = 0; y < height; ++y) if (ink[static_cast<std::size_t>(y) * width + x]) return true; return false; };
            std::vector<Glyph> glyphs;                                                       // 결과
            for (int x = 0; x < width;)                                                      // 열마다
            {
                if (!isInkColumn(x)) { ++x; continue; }                                      // 빈 열
                const int left = x;                                                          // 덩어리 시작
                while (x < width && isInkColumn(x)) ++x;                                     // 덩어리 끝
                int top = height;                                                            // 글자 위
                int bottom = -1;                                                             // 글자 아래
                for (int y = 0; y < height; ++y)
                    for (int column = left; column < x; ++column)
                        if (ink[static_cast<std::size_t>(y) * width + column]) { top = std::min(top, y); bottom = std::max(bottom, y); }
                Glyph glyph{ left, x, x - left, bottom - top + 1, {} };                      // 덩어리
                if (glyph.m_height < GLYPH_MIN_HEIGHT || glyph.m_height > GLYPH_MAX_HEIGHT
                    || glyph.m_width < GLYPH_MIN_WIDTH || glyph.m_width > GLYPH_MAX_WIDTH) continue;   // 글자 크기가 아님 (테두리 · 아이콘 빛)
                for (int y = top; y <= bottom; ++y)
                    for (int column = left; column < x; ++column) glyph.m_bits.push_back(ink[static_cast<std::size_t>(y) * width + column]);
                glyphs.push_back(std::move(glyph));                                          // 추가
            }
            return glyphs;
        }

        // 두 모양의 다이스 계수 (작은 판 위에서 한 칸씩 밀어 보며 가장 높은 값)
        [[nodiscard]] double CompareGlyph(const Glyph& glyph, const SlotGlyphTemplate& pattern)
        {
            const int canvasHeight = std::max(glyph.m_height, pattern.m_height) + GLYPH_CANVAS_MARGIN;   // 판 세로
            const int canvasWidth = std::max(glyph.m_width, pattern.m_width) + GLYPH_CANVAS_MARGIN;      // 판 가로
            int glyphInk = 0;                                                                            // 글자 칸 수
            for (bool bit : glyph.m_bits) glyphInk += bit ? 1 : 0;
            const int patternInk = static_cast<int>(std::ranges::count(pattern.m_bits, INK_MARK));       // 템플릿 칸 수
            double best = 0.0;                                                                           // 최고 점수
            for (int shiftY = 0; shiftY <= GLYPH_CANVAS_MARGIN; ++shiftY)
                for (int shiftX = 0; shiftX <= GLYPH_CANVAS_MARGIN; ++shiftX)
                {
                    if (shiftY + pattern.m_height > canvasHeight || shiftX + pattern.m_width > canvasWidth) continue;   // 판 밖
                    int both = 0;                                                                        // 겹친 칸
                    for (int y = 0; y < glyph.m_height; ++y)
                        for (int x = 0; x < glyph.m_width; ++x)
                        {
                            if (!glyph.m_bits[static_cast<std::size_t>(y) * glyph.m_width + x]) continue;   // 빈 칸
                            const int patternX = x + 1 - shiftX;                                         // 글자는 판 (1, 1)에 놓음
                            const int patternY = y + 1 - shiftY;
                            if (patternX < 0 || patternY < 0 || patternX >= pattern.m_width || patternY >= pattern.m_height) continue;
                            if (pattern.m_bits[static_cast<std::size_t>(patternY) * pattern.m_width + patternX] == INK_MARK) ++both;   // 둘 다 칸
                        }
                    best = std::max(best, 2.0 * both / std::max(glyphInk + patternInk, 1));              // 다이스
                }
            return best;
        }
    }

    std::optional<std::string> MaterialSlotDetector::ReadCount(const FrameImage& band)
    {
        const auto scored = ReadCountScored(band);                                           // 점수와 함께
        if (!scored) return std::nullopt;                                                    // 못 읽음
        return scored->first;                                                                // 글자
    }

    std::optional<std::pair<std::string, double>> MaterialSlotDetector::ReadCountScored(const FrameImage& band)
    {
        std::vector<Glyph> glyphs = SplitGlyphs(band);                                       // 글자 덩어리
        if (glyphs.empty()) return std::nullopt;                                             // 글자 없음
        std::size_t first = glyphs.size() - 1;                                               // 오른쪽 끝 숫자 묶음 시작
        while (first > 0 && glyphs[first].m_left - glyphs[first - 1].m_right <= GLYPH_MAX_GAP) --first;   // 가까운 글자까지
        std::string text;                                                                    // 결과
        double lowestScore = 1.0;                                                            // 가장 낮은 글자 점수
        for (std::size_t index = first; index < glyphs.size(); ++index)                     // 글자마다
        {
            double bestScore = 0.0;                                                          // 최고 점수
            char bestSymbol = '\0';                                                          // 그 글자
            for (const SlotGlyphTemplate& pattern : GetSlotGlyphs())                         // 템플릿마다
                if (const double score = CompareGlyph(glyphs[index], pattern); score > bestScore) { bestScore = score; bestSymbol = pattern.m_symbol; }
            if (bestScore < GLYPH_MIN_SCORE)                                                 // 모르는 글자
            {
                if (text.empty()) continue;                                                  // 숫자 앞 아이콘 빛 → 건너뜀
                return std::nullopt;                                                         // 숫자 사이 → 믿을 수 없음
            }
            text += bestSymbol;                                                              // 추가
            lowestScore = std::min(lowestScore, bestScore);                                  // 가장 낮은 점수
        }
        if (text.empty()) return std::nullopt;                                               // 숫자 없음
        return std::pair{ text, lowestScore };                                               // "2720" · 점수
    }

    namespace  // 슬롯 하나 읽기 도우미
    {
        // 아이콘 템플릿 하나의 비교 준비물
        struct IconPattern
        {
            const SlotIconTemplate* m_icon{};       // 템플릿
            LumaImage m_luma;                       // 밝기
            float m_mean{};                         // 밝기 평균
            double m_norm{};                        // 편차 크기
            std::array<double, 3> m_color{};        // 평균 색
        };

        // 템플릿 → 비교 준비물
        [[nodiscard]] IconPattern MakeIconPattern(const SlotIconTemplate& icon)
        {
            IconPattern pattern{ &icon, ToLuma(icon) };                                       // 템플릿 · 밝기
            std::tie(pattern.m_mean, pattern.m_norm) = GetStats(pattern.m_luma);             // 통계
            pattern.m_color = GetMeanColor(icon);                                            // 평균 색
            return pattern;
        }

        // 숫자 띠를 3배로 OCR → 숫자만 (흰 글자 흑백 그림, 못 읽으면 nullopt)
        [[nodiscard]] std::optional<std::string> ReadCountByOcr(const FrameImage& band, const TextRecognizer& text)
        {
            constexpr int OCR_SCALE{ 3 };                                                    // 3배 확대
            const auto result = text.Recognize(BossHealthDetector::MakeTextMask(band), OCR_SCALE);   // 흰 글자만 3배 OCR
            if (!result) return std::nullopt;                                                // 실패
            std::string digits;                                                              // 숫자만
            for (const TextLine& line : result->m_lines)
                for (wchar_t character : line.m_text)
                {
                    if (character >= L'0' && character <= L'9') digits += static_cast<char>(character);   // 숫자
                    else if (character == L'+') digits += OVERFLOW_MARK;                     // 넘침
                }
            if (digits.empty()) return std::nullopt;                                         // 숫자 없음
            return digits;
        }

        // (x, y) 근처 radius 안에서 아이콘을 맞춰 보고 숫자 읽기 (아이콘 · 색이 안 맞거나 숫자를 못 읽으면 nullopt)
        [[nodiscard]] std::optional<SlotReading> ReadSlotAt(const FrameImage& frame, const IconPattern& pattern, int x, int y, int radius, const TextRecognizer* text)
        {
            const int width = pattern.m_luma.m_width;                                        // 템플릿 가로
            const int height = pattern.m_luma.m_height;                                      // 템플릿 세로
            const PixelRect region = ClampRect(PixelRect{ x - radius, y - radius, width + radius * 2, height + radius * 2 }, frame.GetWidth(), frame.GetHeight());   // 볼 영역
            if (region.m_width < width || region.m_height < height) return std::nullopt;     // 화면 끝
            const LumaImage local = ToLuma(frame.Crop(region));                              // 그 영역만 밝기
            double bestScore = -1.0;                                                         // 최고 점수
            int bestX = 0;                                                                   // 그 위치 (영역 기준)
            int bestY = 0;
            for (int localY = 0; localY + height <= local.m_height; ++localY)
                for (int localX = 0; localX + width <= local.m_width; ++localX)
                    if (const double score = Correlate(local, pattern.m_luma, pattern.m_mean, pattern.m_norm, localX, localY); score > bestScore)
                    {
                        bestScore = score;                                                   // 더 잘 맞음
                        bestX = localX;
                        bestY = localY;
                    }
            if (bestScore < FINE_MIN_SCORE) return std::nullopt;                             // 다른 아이콘
            const PixelRect bounds{ region.m_x + bestX, region.m_y + bestY, width, height }; // 맞은 자리 (프레임 기준)
            const std::array<double, 3> color = GetMeanColor(frame, bounds);                 // 화면 색
            double colorDiff = 0.0;                                                          // 가장 큰 채널 차이
            for (std::size_t channel = 0; channel < color.size(); ++channel) colorDiff = std::max(colorDiff, std::abs(color[channel] - pattern.m_color[channel]));
            if (colorDiff > MAX_COLOR_DIFF) return std::nullopt;                             // 색이 다름

            const PixelRect bandRect = ClampRect(PixelRect{ bounds.m_x - BAND_SIDE_MARGIN, bounds.m_y - BAND_TOP_OFFSET,
                width + BAND_SIDE_MARGIN - BAND_RIGHT_TRIM, BAND_TOP_OFFSET - BAND_BOTTOM_OFFSET }, frame.GetWidth(), frame.GetHeight());   // 숫자 띠 (테두리 앞까지)
            if (bandRect.IsEmpty()) return std::nullopt;                                     // 화면 위 끝
            const FrameImage band = frame.Crop(bandRect);                                    // 숫자 띠
            const auto scored = MaterialSlotDetector::ReadCountScored(band);                 // 글자 모양 비교
            std::optional<std::string> count;                                                // 받을 숫자
            if (scored && scored->second >= GLYPH_STRONG_SCORE) count = scored->first;       // 확실 → 바로 (3배 OCR 없이, 빠름)
            else if (text)                                                                   // 애매하거나 못 읽음 → 3배 OCR
            {
                const auto ocr = ReadCountByOcr(band, *text);                                // 3배 OCR
                if (scored && ocr && scored->first == *ocr) count = ocr;                     // 둘이 같음 → 받음
                else if (!scored) count = ocr;                                               // 글자 모양 비교 실패 → 3배 OCR 결과
                else return std::nullopt;                                                    // 애매한데 둘이 다름 → 믿을 수 없음 (오인식 막기)
            }
            else if (scored) count = scored->first;                                          // OCR이 없을 때 (자체 검사 등) → 글자 모양 결과
            if (!count || count->empty() || count->contains(OVERFLOW_MARK) || count->size() > MAX_COUNT_DIGITS) return std::nullopt;   // 못 읽음 · 넘침
            std::int64_t quantity = 0;                                                       // 값
            for (char digit : *count) quantity = quantity * DECIMAL_BASE + (digit - '0');    // 숫자
            return SlotReading{ std::string{ pattern.m_icon->m_valueKey }, quantity, bounds, bestScore };
        }

        // 겹치는 슬롯은 점수 높은 것만 남기며 추가
        void AddSlot(std::vector<SlotReading>& found, SlotReading slot)
        {
            if (std::ranges::any_of(found, [&](const SlotReading& other) { return GetOverlap(other.m_iconBounds, slot.m_iconBounds) > OVERLAP_RATIO && other.m_score >= slot.m_score; })) return;   // 더 잘 맞는 게 있음
            std::erase_if(found, [&](const SlotReading& other) { return GetOverlap(other.m_iconBounds, slot.m_iconBounds) > OVERLAP_RATIO; });   // 덜 맞는 것은 지움
            found.push_back(std::move(slot));                                                // 추가
        }

        // 슬롯들 → 재료별 합계 결과
        [[nodiscard]] std::optional<ScanResult> Summarize(const std::vector<SlotReading>& slots)
        {
            if (slots.empty()) return std::nullopt;                                          // 창고 · 소지품이 안 보임
            std::map<std::string, std::int64_t, std::less<>> totals;                         // 재료별 합
            for (const SlotReading& slot : slots) totals[slot.m_valueKey] += slot.m_quantity;   // 더하기
            ScanResult result;                                                               // 결과
            result.m_screenName = std::string{ MaterialSlotDetector::SCREEN_NAME };          // 화면 이름
            for (const auto& [key, quantity] : totals) result.m_values.push_back(ScanValue{ key, quantity });   // 값
            return result;
        }
    }

    std::vector<SlotReading> MaterialSlotDetector::FindSlots(const FrameImage& frame, const TextRecognizer* text)
    {
        std::vector<SlotReading> found;                                                      // 결과
        if (frame.IsEmpty()) return found;                                                   // 빈 프레임
        const LumaImage small = Shrink(ToLuma(frame), DOWN_SCALE);                           // 축소 밝기
        const SumTable smallTable = MakeSumTable(small);                                     // 축소 화면 창 합 표
        for (const SlotIconTemplate& icon : GetSlotIcons())                                  // 아이콘마다
        {
            const IconPattern pattern = MakeIconPattern(icon);                               // 비교 준비물
            const LumaImage smallPattern = Shrink(pattern.m_luma, DOWN_SCALE);               // 축소 템플릿
            const auto [smallMean, smallNorm] = GetStats(smallPattern);                      // 축소 통계
            std::vector<float> centeredSmall(smallPattern.m_pixels.size());                  // 축소 템플릿 - 평균
            std::ranges::transform(smallPattern.m_pixels, centeredSmall.begin(), [smallMean](float value) { return value - smallMean; });

            // 1) 축소 화면에서 후보
            struct Candidate { double m_score; int m_x; int m_y; };                          // 후보
            std::vector<Candidate> candidates;                                               // 후보들
            for (int y = 0; y + smallPattern.m_height <= small.m_height; ++y)
                for (int x = 0; x + smallPattern.m_width <= small.m_width; ++x)
                    if (const double score = CorrelateFast(small, smallTable, centeredSmall, smallPattern.m_width, smallPattern.m_height, smallNorm, x, y); score >= COARSE_MIN_SCORE)
                        candidates.push_back(Candidate{ score, x, y });
            std::ranges::sort(candidates, std::greater{}, &Candidate::m_score);              // 높은 순
            std::vector<Candidate> kept;                                                     // 겹치지 않는 후보
            for (const Candidate& candidate : candidates)
            {
                if (kept.size() >= MAX_CANDIDATES) break;                                    // 충분
                const bool isNear = std::ranges::any_of(kept, [&](const Candidate& other)
                    { return std::abs(other.m_x - candidate.m_x) <= COARSE_SUPPRESS_RADIUS && std::abs(other.m_y - candidate.m_y) <= COARSE_SUPPRESS_RADIUS; });
                if (!isNear) kept.push_back(candidate);                                      // 추가
            }

            // 2) 원래 크기에서 다시 맞춰 보고 색 비교 → 숫자 읽기
            for (const Candidate& candidate : kept)
                if (auto slot = ReadSlotAt(frame, pattern, candidate.m_x * DOWN_SCALE, candidate.m_y * DOWN_SCALE, REFINE_RADIUS, text)) AddSlot(found, std::move(*slot));
        }
        return found;                                                                        // 슬롯들
    }

    std::string_view MaterialSlotDetector::GetScreenName() const noexcept
    {
        return SCREEN_NAME;   // "material_slot"
    }

    std::optional<ScanResult> MaterialSlotDetector::Detect(const FrameImage& frame, const DetectorContext& context)
    {
        if (!m_known.empty() && m_scansSinceSearch < FULL_SEARCH_EVERY)                     // 지난번 슬롯 자리만 다시 확인 (빠름)
        {
            ++m_scansSinceSearch;                                                            // 횟수
            std::vector<SlotReading> again;                                                  // 다시 읽은 슬롯
            bool isAllFound = true;                                                          // 모두 그대로 있는지
            for (const SlotReading& known : m_known)
            {
                const auto icons = GetSlotIcons();                                           // 아이콘 목록
                const auto icon = std::ranges::find(icons, known.m_valueKey, &SlotIconTemplate::m_valueKey);   // 그 아이콘
                if (icon == icons.end()) { isAllFound = false; break; }                      // 없음
                auto slot = ReadSlotAt(frame, MakeIconPattern(*icon), known.m_iconBounds.m_x, known.m_iconBounds.m_y, KNOWN_SLOT_RADIUS, &context.m_text);   // 그 자리
                if (!slot) { isAllFound = false; break; }                                    // 창을 닫았거나 옮김
                again.push_back(std::move(*slot));                                           // 다시 읽음
            }
            if (isAllFound) { m_known = std::move(again); return Summarize(m_known); }      // 그대로
        }
        m_scansSinceSearch = 0;                                                              // 화면 전체 다시 찾기
        m_known = FindSlots(frame, &context.m_text);                                         // 슬롯들
        return Summarize(m_known);
    }

    std::optional<ScanResult> MaterialSlotDetector::DetectFrame(const FrameImage& frame, const TextRecognizer* text)
    {
        return Summarize(FindSlots(frame, text));                                            // 화면 전체 (상태 없음)
    }
}
