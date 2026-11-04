export module LostArkMultiTool.Core.Renderer;

import std;

export namespace LostArkMultiTool::Core
{
    // 색 (각 채널 0.0 ~ 1.0, 알파 1.0 = 불투명)
    struct Color
    {
        float m_red{};
        float m_green{};
        float m_blue{};
        float m_alpha{ 1.0f };
    };

    inline constexpr float COLOR_CHANNEL_MAX{ 255.0f };   // 0~255 정수 채널의 최댓값
    inline constexpr float FULL_ALPHA{ 1.0f };            // 완전 불투명

    // 0~255 정수 RGB로 색 만들기
    // 다른 모듈의 constexpr 상수(테마 색)를 만들 때 컴파일 시간에 계산돼야 해서 정의도 여기 둠 (constexpr 함수는 사용하는 곳에서 정의가 보여야 함)
    [[nodiscard]] constexpr Color MakeColor(int red, int green, int blue, float alpha = FULL_ALPHA) noexcept
    {
        return Color{ red / COLOR_CHANNEL_MAX, green / COLOR_CHANNEL_MAX, blue / COLOR_CHANNEL_MAX, alpha };
    }

    // 사각형 (DIP 단위 = 배율 100% 기준 논리 픽셀 → 모니터 배율이 달라도 같은 크기로 보임)
    struct RectF
    {
        float m_left{};
        float m_top{};
        float m_right{};
        float m_bottom{};

        [[nodiscard]] float GetWidth() const noexcept;
        [[nodiscard]] float GetHeight() const noexcept;
        [[nodiscard]] bool Contains(float x, float y) const noexcept;
    };

    // 글자 가로 정렬
    enum class TextAlign
    {
        LEADING,    // 왼쪽
        CENTER,
        TRAILING    // 오른쪽
    };

    // 글자 굵기
    enum class FontWeight
    {
        NORMAL,
        BOLD
    };

    // 글자 모양 (크기는 DIP)
    struct TextStyle
    {
        float m_size{};
        FontWeight m_weight{ FontWeight::NORMAL };
        TextAlign m_align{ TextAlign::LEADING };
    };

    // 2D 그리기 인터페이스 (화면 코드는 DirectX를 모르고 이것만 사용 → 실제 구현은 Views::DX11Renderer)
    class Renderer
    {
    public:
        virtual ~Renderer();

        // 그리기 영역 크기 (DIP)
        [[nodiscard]] virtual float GetWidth() const noexcept = 0;
        [[nodiscard]] virtual float GetHeight() const noexcept = 0;

        virtual void Clear(const Color& color) = 0;
        virtual void FillRect(const RectF& rect, const Color& color) = 0;
        virtual void DrawRect(const RectF& rect, const Color& color, float strokeWidth) = 0;
        virtual void DrawLine(float startX, float startY, float endX, float endY, const Color& color, float strokeWidth) = 0;

        // 사각형 안에 한 줄 글자 (세로 가운데 정렬, 넘치는 부분은 잘림)
        // 이름을 DrawText로 하지 않은 이유: Windows 헤더의 DrawText 매크로(DrawTextW)와 겹치지 않게
        virtual void DrawString(std::wstring_view text, const RectF& rect, const Color& color, const TextStyle& style) = 0;
    };
}