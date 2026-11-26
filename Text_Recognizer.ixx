export module LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식(OCR) 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기

// 윈도우 내장 OCR(Windows.Media.Ocr, 한국어)로 글자 읽기 → 캐릭터 이름 · 툴팁 재료 이름 · 창 제목 확인용
// 숫자는 DigitReader가 더 정확하니 숫자 읽기에는 쓰지 않음
// 스레드: 감시 스레드(VisionApartment를 만든 스레드)에서만 부를 것 (화면 스레드에서 부르면 기다리다 멈출 수 있음)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 읽은 단어 하나
    struct TextWord
    {
        std::wstring m_text;     // 단어 글자
        PixelRect m_bounds;      // 단어 위치 (넘긴 이미지 기준)
    };

    // 읽은 글자 한 줄
    struct TextLine
    {
        std::wstring m_text;            // 줄 글자 (단어 사이 띄어쓰기 포함)
        PixelRect m_bounds;             // 줄 위치 (넘긴 이미지 기준)
        std::vector<TextWord> m_words;  // 단어들 (왼쪽부터)
    };

    // 글자 인식 결과
    struct TextResult
    {
        std::vector<TextLine> m_lines;                    // 위에서 아래 순서의 줄들
        [[nodiscard]] std::wstring GetText() const;       // 모든 줄을 줄바꿈으로 이은 글자
    };

    // 글자 인식기 (pimpl: WinRT 형식을 모듈 밖에 숨김)
    class TextRecognizer
    {
    public:
        TextRecognizer();    // 한국어 엔진 준비 (없으면 사용자 언어 엔진, 그것도 없으면 쓸 수 없음)
        ~TextRecognizer();   // 소멸자

        TextRecognizer(const TextRecognizer&) = delete;              // 복사 금지
        TextRecognizer& operator=(const TextRecognizer&) = delete;   // 복사 대입 금지

        [[nodiscard]] bool IsAvailable() const noexcept;             // 엔진을 쓸 수 있는지
        [[nodiscard]] const std::wstring& GetLanguageTag() const noexcept;   // 엔진 언어 ("ko" 등, 없으면 빈 글자)
        [[nodiscard]] static std::uint32_t GetMaxImageDimension() noexcept;  // 엔진이 받는 최대 가로 · 세로 (넘으면 잘라서 넘길 것)

        // 이미지 글자 읽기 (작은 글자는 scale배 키워서 읽음, 엔진이 없거나 너무 크면 nullopt)
        [[nodiscard]] std::optional<TextResult> Recognize(const FrameImage& image, int scale = 1) const;

    private:
        struct Impl;                     // 구현 (WinRT OCR 엔진)
        std::unique_ptr<Impl> m_impl;    // 구현 소유
    };
}
