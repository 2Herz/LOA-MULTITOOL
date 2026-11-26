module LostArkMultiTool.Vision.DigitReader;  // 숫자 읽기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기
import LostArkMultiTool.Vision.ImageFile;  // 이미지 파일 읽기 · 쓰기 가져오기
import LostArkMultiTool.Vision.TemplateMatcher;  // 템플릿 매칭 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr char UNKNOWN_SYMBOL{ '?' };            // 못 알아본 글자
        constexpr char PLUS_SYMBOL{ '+' };               // 넘침 표시 ("9999+")
        constexpr char COMMA_SYMBOL{ ',' };              // 천 단위 쉼표
        constexpr std::uint8_t BINARY_LEVEL{ 128 };      // 이미 이진화된 템플릿 파일을 다시 이진화할 기준
        constexpr int DECIMAL_BASE{ 10 };                // 십진수
        constexpr std::int64_t MAX_VALUE{ std::numeric_limits<std::int64_t>::max() / DECIMAL_BASE };   // 자릿수 넘침 방지 한계
        constexpr std::string_view IMAGE_EXTENSION{ ".png" };   // 템플릿 파일 확장자
        constexpr std::string_view PLUS_FILE_STEM{ "plus" };    // '+' 템플릿 파일 이름
        constexpr std::string_view COMMA_FILE_STEM{ "comma" };  // ',' 템플릿 파일 이름
        constexpr std::string_view DIGIT_SYMBOLS{ "0123456789" };   // 숫자 글자들

        // 글자 → 파일 이름 (확장자 제외)
        [[nodiscard]] std::string GetFileStem(char symbol)
        {
            if (symbol == PLUS_SYMBOL) return std::string{ PLUS_FILE_STEM };     // '+'
            if (symbol == COMMA_SYMBOL) return std::string{ COMMA_FILE_STEM };   // ','
            return std::string(1, symbol);                                       // 숫자는 그대로
        }

        // 읽기 옵션대로 이진화 (글자 = 255)
        [[nodiscard]] GrayImage Binarize(const GrayImage& image, const DigitReadOptions& options)
        {
            const GrayImage oriented = options.m_isBrightText ? image : image.Invert();               // 글자가 밝게 오도록
            const std::uint8_t level = options.m_threshold.value_or(oriented.GetOtsuLevel());         // 이진화 기준
            return oriented.Threshold(level);                                                          // 이진 이미지
        }

        // 줄 안에서 글자 칸(잉크가 있는 연속 열) 나누기
        [[nodiscard]] std::vector<PixelRect> SplitGlyphs(const GrayImage& binary, const PixelRect& lineBounds)
        {
            std::vector<PixelRect> glyphs;                                         // 글자 칸들
            int start = -1;                                                        // 지금 글자 시작 열 (-1 = 글자 밖)
            for (int x = lineBounds.m_x; x <= lineBounds.GetRight(); ++x)          // 열마다 (끝 다음 열에서 마지막 글자 닫기)
            {
                bool hasInk = false;                                               // 이 열에 잉크가 있는지
                if (x < lineBounds.GetRight())                                     // 줄 안 열이면
                {
                    for (int y = lineBounds.m_y; y < lineBounds.GetBottom() && !hasInk; ++y)   // 줄 높이만큼
                        hasInk = binary.At(x, y) != 0;                             // 잉크 확인
                }
                if (hasInk && start < 0) start = x;                                // 글자 시작
                if (!hasInk && start >= 0)                                         // 글자 끝
                {
                    const PixelRect column{ start, lineBounds.m_y, x - start, lineBounds.m_height };   // 글자 열 띠
                    const auto ink = binary.Crop(column).FindInkBounds();          // 띠 안에서 잉크를 딱 맞게
                    if (ink) glyphs.push_back(PixelRect{ column.m_x + ink->m_x, column.m_y + ink->m_y, ink->m_width, ink->m_height });   // 위아래까지 딱 맞춘 칸
                    start = -1;                                                    // 글자 밖으로
                }
            }
            return glyphs;   // 왼쪽부터 글자 칸
        }

        // 칸들 중 가장 큰 높이 (상대 높이 기준, 칸이 없으면 1)
        [[nodiscard]] int GetTallestHeight(std::span<const PixelRect> cells) noexcept
        {
            int tallest = 1;                                                       // 0으로 나누기 방지
            for (const PixelRect& cell : cells) tallest = std::max(tallest, cell.m_height);   // 최댓값
            return tallest;                                                        // 가장 큰 높이
        }

        // 두 값의 비율 일치도 (1 = 같음, 작을수록 다름)
        [[nodiscard]] double GetRatioFactor(double a, double b) noexcept
        {
            const double smaller = std::min(a, b);                                 // 작은 쪽
            const double larger = std::max(a, b);                                  // 큰 쪽
            return larger <= 0.0 ? 0.0 : smaller / larger;                         // 비율
        }

        // 가로세로 비율 차이 벌점 (1 = 같음, 작을수록 다름)
        [[nodiscard]] double GetAspectFactor(const GrayImage& a, const GrayImage& b) noexcept
        {
            const double ratioA = static_cast<double>(a.GetWidth()) / std::max(a.GetHeight(), 1);   // a 비율
            const double ratioB = static_cast<double>(b.GetWidth()) / std::max(b.GetHeight(), 1);   // b 비율
            const double smaller = std::min(ratioA, ratioB);                                       // 작은 쪽
            const double larger = std::max(ratioA, ratioB);                                        // 큰 쪽
            return larger <= 0.0 ? 0.0 : std::sqrt(smaller / larger);                              // 비율 차이가 클수록 낮게
        }

        // 흑백 → BGRA (저장용)
        [[nodiscard]] FrameImage ToFrame(const GrayImage& image)
        {
            std::vector<std::uint8_t> pixels;                                                    // BGRA 버퍼
            pixels.reserve(image.GetPixels().size() * FrameImage::BYTES_PER_PIXEL);             // 할당 한 번
            constexpr std::uint8_t OPAQUE_ALPHA{ 255 };                                          // 불투명
            for (std::uint8_t value : image.GetPixels())                                         // 픽셀마다
                pixels.insert(pixels.end(), { value, value, value, OPAQUE_ALPHA });              // 회색 BGRA
            return FrameImage{ image.GetWidth(), image.GetHeight(), std::move(pixels) };         // 프레임
        }
    }

    DigitReader::DigitReader(std::vector<GlyphTemplate> glyphs)
        : m_glyphs{ std::move(glyphs) }   // 템플릿 이동 (복사 없음)
    {
    }

    std::optional<DigitReader> DigitReader::LoadFromFolder(const std::filesystem::path& folder)
    {
        std::vector<GlyphTemplate> glyphs;                                                     // 읽은 템플릿
        std::string symbols{ DIGIT_SYMBOLS };                                                  // 찾을 글자들
        symbols += PLUS_SYMBOL;                                                                // '+' 추가
        symbols += COMMA_SYMBOL;                                                               // ',' 추가
        bool hasDigit = false;                                                                 // 숫자 템플릿이 하나라도 있는지
        for (char symbol : symbols)                                                            // 글자마다
        {
            const auto image = ImageFile::Load(folder / (GetFileStem(symbol) + std::string{ IMAGE_EXTENSION }));   // 파일 읽기
            if (!image) continue;                                                              // 없는 글자는 건너뜀
            glyphs.push_back(GlyphTemplate{ symbol, image->ToGray().Threshold(BINARY_LEVEL) }); // 이진 템플릿
            hasDigit = hasDigit || DIGIT_SYMBOLS.find(symbol) != std::string_view::npos;      // 숫자 여부
        }
        if (!hasDigit) return std::nullopt;                                                    // 숫자가 없으면 쓸 수 없음

        int tallest = 1;                                                                       // 가장 큰 템플릿 높이 (0으로 나누기 방지)
        for (const GlyphTemplate& glyph : glyphs) tallest = std::max(tallest, glyph.m_image.GetHeight());   // 최댓값
        for (GlyphTemplate& glyph : glyphs)                                                    // 템플릿마다
            glyph.m_relativeHeight = static_cast<double>(glyph.m_image.GetHeight()) / tallest; // 상대 높이 (만들 때와 같은 기준)
        return DigitReader{ std::move(glyphs) };                                               // 도구
    }

    std::optional<std::vector<GlyphTemplate>> DigitReader::MakeGlyphsFromLine(const GrayImage& line,
        std::string_view text, const DigitReadOptions& options)
    {
        const GrayImage binary = Binarize(line, options);                  // 이진화
        const auto lineBounds = binary.FindInkBounds();                    // 줄 높이 (글자 전체의 위 ~ 아래)
        if (!lineBounds) return std::nullopt;                              // 글자가 없음
        const auto cells = SplitGlyphs(binary, *lineBounds);               // 글자 칸
        if (cells.size() != text.size()) return std::nullopt;              // 글자 수가 다르면 짝을 못 맞춤
        const double tallest = GetTallestHeight(cells);                    // 줄에서 가장 큰 글자 높이

        std::vector<GlyphTemplate> glyphs;                                 // 결과
        for (std::size_t index = 0; index < cells.size(); ++index)         // 글자마다
        {
            const char symbol = text[index];                               // 답 글자
            const bool isDuplicate = std::ranges::any_of(glyphs, [symbol](const GlyphTemplate& glyph) { return glyph.m_symbol == symbol; });   // 이미 만든 글자인지
            if (!isDuplicate) glyphs.push_back(GlyphTemplate{ symbol, binary.Crop(cells[index]), cells[index].m_height / tallest });   // 처음 나온 글자만 템플릿으로
        }
        return glyphs;   // 글자 템플릿
    }

    bool DigitReader::SaveToFolder(const std::filesystem::path& folder, std::span<const GlyphTemplate> glyphs)
    {
        bool isSaved = true;   // 전부 저장했는지
        for (const GlyphTemplate& glyph : glyphs)   // 글자마다
            isSaved = ImageFile::SavePNG(folder / (GetFileStem(glyph.m_symbol) + std::string{ IMAGE_EXTENSION }), ToFrame(glyph.m_image)) && isSaved;   // PNG 저장
        return isSaved;   // 결과
    }

    bool DigitReader::IsEmpty() const noexcept
    {
        return m_glyphs.empty();   // 템플릿이 없으면 빈 도구
    }

    std::size_t DigitReader::GetGlyphCount() const noexcept
    {
        return m_glyphs.size();   // 템플릿 수
    }

    DigitReadResult DigitReader::Read(const GrayImage& area, const DigitReadOptions& options) const
    {
        DigitReadResult result;                                     // 결과
        if (m_glyphs.empty() || area.IsEmpty()) return result;      // 읽을 수 없음

        const GrayImage binary = Binarize(area, options);           // 이진화
        const auto lineBounds = binary.FindInkBounds();             // 줄 높이
        if (!lineBounds) return result;                             // 글자가 없음

        result.m_lowestScore = 1.0;                                 // 최저 점수 시작값 (최고점)
        bool hasUnknown = false;                                    // 모르는 글자가 있는지
        bool hasDigit = false;                                      // 숫자가 하나라도 있는지
        std::int64_t value = 0;                                     // 누적 숫자 값
        const auto cells = SplitGlyphs(binary, *lineBounds);        // 글자 칸
        const double tallest = GetTallestHeight(cells);             // 줄에서 가장 큰 글자 높이 (숫자 높이)
        for (const PixelRect& cell : cells)                         // 글자 칸마다
        {
            const GrayImage glyph = binary.Crop(cell);              // 글자 그림
            const double relativeHeight = cell.m_height / tallest;  // 이 글자의 상대 높이
            char bestSymbol = UNKNOWN_SYMBOL;                       // 가장 비슷한 글자
            double bestScore = -1.0;                                // 그 점수
            for (const GlyphTemplate& candidate : m_glyphs)         // 템플릿마다
            {
                const GrayImage scaled = glyph.Resize(candidate.m_image.GetWidth(), candidate.m_image.GetHeight());   // 템플릿 크기로 맞춤
                const double score = TemplateMatcher::Score(scaled, candidate.m_image, 0, 0)
                    * GetAspectFactor(glyph, candidate.m_image)     // 가로세로 비율 벌점
                    * GetRatioFactor(relativeHeight, candidate.m_relativeHeight);   // 상대 높이 벌점 (쉼표 · '+' 구분)
                if (score > bestScore)                              // 더 비슷하면
                {
                    bestScore = score;                              // 최고점 갱신
                    bestSymbol = candidate.m_symbol;                // 글자 갱신
                }
            }
            if (bestScore < options.m_minScore) bestSymbol = UNKNOWN_SYMBOL;   // 기준 미달이면 모르는 글자
            result.m_lowestScore = std::min(result.m_lowestScore, bestScore);  // 최저 점수 갱신
            result.m_text += bestSymbol;                                       // 읽은 글자 추가

            if (bestSymbol == UNKNOWN_SYMBOL) hasUnknown = true;               // 모르는 글자
            else if (bestSymbol == PLUS_SYMBOL) result.m_isOverflow = true;    // 넘침 표시
            else if (bestSymbol != COMMA_SYMBOL)                               // 숫자면
            {
                if (value > MAX_VALUE) hasUnknown = true;                      // 자릿수 넘침은 오인식으로 봄
                else value = value * DECIMAL_BASE + (bestSymbol - DIGIT_SYMBOLS.front());   // 자릿수 추가
                hasDigit = true;                                               // 숫자 있음
            }
        }
        if (hasDigit && !hasUnknown) result.m_value = value;   // 모두 알아봤을 때만 값 확정
        return result;                                          // 결과
    }
}
