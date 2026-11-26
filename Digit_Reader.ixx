export module LostArkMultiTool.Vision.DigitReader;  // 숫자 읽기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기

// 게임 숫자 글꼴 템플릿으로 수량 숫자 읽기 ("12,345" · "9999+")
// 게임 숫자는 글꼴이 고정이라 범용 OCR보다 글자 그림 비교가 더 정확하고 빠름
// 템플릿 규칙: 글자 하나를 위아래 · 좌우 모두 딱 맞게 자른 이진 그림 (MakeGlyphsFromLine이 만들어 줌)
//              쉼표 · '+'처럼 작은 글자는 "숫자 높이 대비 상대 높이"로 구분 (줄마다 높이가 달라져도 흔들리지 않음)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 글자 템플릿 하나
    struct GlyphTemplate
    {
        char m_symbol{};      // 글자 ('0' ~ '9', '+', ',')
        GrayImage m_image;    // 이진화된 글자 그림 (글자 = 255, 바탕 = 0, 딱 맞게 자름)
        double m_relativeHeight{ 1.0 };   // 같은 줄에서 가장 큰 글자 높이 대비 이 글자 높이 (숫자 ≈ 1, 쉼표 · '+'는 작음)
    };

    // 읽기 옵션
    struct DigitReadOptions
    {
        bool m_isBrightText{ true };              // 밝은 글자 + 어두운 바탕인지 (아니면 뒤집어서 읽음)
        std::optional<std::uint8_t> m_threshold;  // 이진화 기준 (nullopt = 오츠 자동)
        double m_minScore{ 0.6 };                 // 글자로 인정할 최소 점수
    };

    // 읽기 결과
    struct DigitReadResult
    {
        std::string m_text;                    // 읽은 글자 그대로 (모르는 글자는 '?')
        std::optional<std::int64_t> m_value;   // 숫자 값 (쉼표 무시, 모르는 글자가 있거나 숫자가 없으면 nullopt)
        bool m_isOverflow{};                   // '+'가 붙었는지 ("9999+" → 실제 수량은 툴팁에서 읽어야 함)
        double m_lowestScore{};                // 글자 점수 중 가장 낮은 값 (신뢰도 확인용)
    };

    // 숫자 읽기 도구 (템플릿 묶음 하나 = 글꼴 · 크기 하나)
    class DigitReader
    {
    public:
        DigitReader() = default;                                  // 템플릿 없는 빈 도구 (항상 실패)
        explicit DigitReader(std::vector<GlyphTemplate> glyphs);  // 템플릿 묶음으로 만들기

        // 폴더에서 템플릿 읽기 (파일 이름: 0.png ~ 9.png, plus.png, comma.png / 없는 글자는 건너뜀, 숫자가 하나도 없으면 nullopt)
        [[nodiscard]] static std::optional<DigitReader> LoadFromFolder(const std::filesystem::path& folder);

        // 답을 아는 숫자 줄 그림에서 글자 템플릿 만들기 (text의 글자 수와 잘린 글자 수가 다르면 nullopt)
        // 스크린샷으로 템플릿을 만들 때 · 자체 검사에서 씀
        [[nodiscard]] static std::optional<std::vector<GlyphTemplate>> MakeGlyphsFromLine(const GrayImage& line,
            std::string_view text, const DigitReadOptions& options);

        // 템플릿 묶음을 폴더에 PNG로 저장 (LoadFromFolder와 같은 파일 이름, 실패하면 false)
        [[nodiscard]] static bool SaveToFolder(const std::filesystem::path& folder, std::span<const GlyphTemplate> glyphs);

        [[nodiscard]] bool IsEmpty() const noexcept;                   // 템플릿이 없는지
        [[nodiscard]] std::size_t GetGlyphCount() const noexcept;      // 템플릿 수

        // 숫자 영역(흑백) 읽기
        [[nodiscard]] DigitReadResult Read(const GrayImage& area, const DigitReadOptions& options) const;

    private:
        std::vector<GlyphTemplate> m_glyphs;   // 글자 템플릿들
    };
}
