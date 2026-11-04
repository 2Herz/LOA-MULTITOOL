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

    // 같은 색에 투명도만 바꾼 색 (constexpr 함수는 쓰는 곳에서 정의가 보여야 해서 정의를 여기 둠)
    [[nodiscard]] constexpr Color WithAlpha(Color color, float alpha) noexcept
    {
        color.m_alpha = alpha;
        return color;
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
        [[nodiscard]] bool operator==(const RectF& other) const noexcept;   // 네 좌표가 모두 같으면 같은 사각형 (!=는 자동 생성)
    };

    // 크기 (DIP)
    struct SizeF
    {
        float m_width{};
        float m_height{};
    };

    // 점 (DIP)
    struct PointF
    {
        float m_x{};
        float m_y{};
    };

    // 3차 베지에 곡선 한 구간 (앞 구간의 끝점에서 시작해 두 조절점을 거쳐 m_end에서 끝남 → 그래프 곡선)
    struct BezierSegment
    {
        PointF m_control1;
        PointF m_control2;
        PointF m_end;
    };

    // 글자 가로 정렬
    enum class TextAlign
    {
        LEADING,    // 왼쪽
        CENTER,
        TRAILING    // 오른쪽
    };

    // 글자 세로 정렬 (사각형 안에서)
    enum class VerticalAlign
    {
        ALIGN_TOP,
        ALIGN_MIDDLE,
        ALIGN_BOTTOM
    };

    // 줄바꿈 방식
    enum class TextWrap
    {
        SINGLE_LINE,   // 한 줄 (넘치면 잘림)
        WORD_WRAP      // 사각형 너비에 맞춰 여러 줄
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
        VerticalAlign m_verticalAlign{ VerticalAlign::ALIGN_MIDDLE };
        TextWrap m_wrap{ TextWrap::SINGLE_LINE };
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
        virtual void FillRoundedRect(const RectF& rect, float radius, const Color& color) = 0;
        virtual void DrawRoundedRect(const RectF& rect, float radius, const Color& color, float strokeWidth) = 0;
        virtual void DrawLine(float startX, float startY, float endX, float endY, const Color& color, float strokeWidth) = 0;
        virtual void DrawDashedLine(float startX, float startY, float endX, float endY, const Color& color, float strokeWidth) = 0;   // 점선 (그래프 눈금선)

        // 원 · 타원 (그래프 점)
        virtual void FillEllipse(const PointF& center, float radiusX, float radiusY, const Color& color) = 0;
        virtual void DrawEllipse(const PointF& center, float radiusX, float radiusY, const Color& color, float strokeWidth) = 0;

        // 곡선: start에서 시작해 segments를 차례로 이어 그림
        virtual void DrawCurve(const PointF& start, std::span<const BezierSegment> segments, const Color& color, float strokeWidth) = 0;

        // 곡선과 아래 기준선(baselineY) 사이를 세로 그라데이션으로 채움 (gradientTop 높이에서 topColor → baselineY에서 bottomColor)
        virtual void FillCurveArea(const PointF& start, std::span<const BezierSegment> segments, float baselineY, float gradientTop,
            const Color& topColor, const Color& bottomColor) = 0;

        // 사각형 안에 글자 (정렬 · 줄바꿈은 style, 사각형 밖으로 넘치는 부분은 잘림)
        // 이름을 DrawText로 하지 않은 이유: Windows 헤더의 DrawText 매크로(DrawTextW)와 겹치지 않게
        virtual void DrawString(std::wstring_view text, const RectF& rect, const Color& color, const TextStyle& style) = 0;

        // 글자가 차지하는 크기 (WORD_WRAP이면 maxWidth에 맞춰 줄바꿈한 높이)
        [[nodiscard]] virtual SizeF MeasureString(std::wstring_view text, const TextStyle& style, float maxWidth) = 0;

        // 한 줄로 그렸을 때 각 글자 경계의 x 위치 (크기 = 글자 수 + 1, 입력칸 커서 · 선택 영역 계산용)
        [[nodiscard]] virtual std::vector<float> MeasureCaretPositions(std::wstring_view text, const TextStyle& style) = 0;

        // 그리기 범위 제한 · 투명도 (Push와 Pop은 짝을 맞춰야 함 → 아래 ClipScope · OpacityScope 사용)
        virtual void PushClip(const RectF& rect) = 0;
        virtual void PopClip() = 0;
        virtual void PushOpacity(float opacity) = 0;
        virtual void PopOpacity() = 0;
    };

    // 범위 안에서만 그리기 (RAII: 블록을 벗어나면 자동으로 PopClip → 짝이 어긋날 일이 없음)
    class ClipScope
    {
    public:
        ClipScope(Renderer& renderer, const RectF& rect);
        ~ClipScope();

        ClipScope(const ClipScope&) = delete;
        ClipScope& operator=(const ClipScope&) = delete;

    private:
        Renderer& m_renderer;
    };

    // 블록 안에서 그린 것 전체를 반투명하게 (RAII: 블록을 벗어나면 자동으로 PopOpacity)
    class OpacityScope
    {
    public:
        OpacityScope(Renderer& renderer, float opacity);
        ~OpacityScope();

        OpacityScope(const OpacityScope&) = delete;
        OpacityScope& operator=(const OpacityScope&) = delete;

    private:
        Renderer& m_renderer;
    };
}