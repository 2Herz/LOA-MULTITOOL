export module LostArkMultiTool.Views.DX11Renderer;

import std;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Views
{
    // 그리기 표면 방식
    enum class SurfaceMode
    {
        SURFACE_OPAQUE,        // 일반 창 (메인 창, 배경을 꽉 채움)
        SURFACE_TRANSPARENT    // 투명 창 (인게임 오버레이, DirectComposition + 미리 곱한 알파 → 그리지 않은 곳은 게임이 비침)
    };

    // Direct3D 11 장치 + DXGI 스왑체인 위에 Direct2D · DirectWrite로 2D 화면(탭 · 표 · 그래프 · 글자)을 그림
    // D3D11 장치를 기반으로 두어 나중에 3D 화면을 같은 창에 함께 그릴 수 있음
    // COM 객체(DirectX 타입)는 구현 파일 안에만 둠 → 다른 모듈은 Windows·DirectX 헤더를 몰라도 됨
    class DX11Renderer final : public Core::Renderer
    {
    public:
        // windowHandle: MainWindow::GetNativeHandle(), dpi: 창이 놓인 모니터 배율
        DX11Renderer(void* windowHandle, float dpi, SurfaceMode mode = SurfaceMode::SURFACE_OPAQUE);   // mode: 일반 · 투명 창
        ~DX11Renderer() override;

        DX11Renderer(const DX11Renderer&) = delete;
        DX11Renderer& operator=(const DX11Renderer&) = delete;

        // 한 프레임 그리기 시작 · 끝 (BeginFrame이 false면 이번 프레임은 그리지 말 것)
        [[nodiscard]] bool BeginFrame();
        void EndFrame();   // 수직 동기화로 화면에 표시 (장치를 잃으면 자동으로 다시 만듦)

        void Resize(int pixelWidth, int pixelHeight);
        void SetDPI(float dpi);

        // ===== Core::Renderer =====
        [[nodiscard]] float GetWidth() const noexcept override;
        [[nodiscard]] float GetHeight() const noexcept override;

        void Clear(const Core::Color& color) override;
        void FillRect(const Core::RectF& rect, const Core::Color& color) override;
        void DrawRect(const Core::RectF& rect, const Core::Color& color, float strokeWidth) override;
        void FillRoundedRect(const Core::RectF& rect, float radius, const Core::Color& color) override;
        void DrawRoundedRect(const Core::RectF& rect, float radius, const Core::Color& color, float strokeWidth) override;
        void DrawLine(float startX, float startY, float endX, float endY, const Core::Color& color, float strokeWidth) override;
        void DrawDashedLine(float startX, float startY, float endX, float endY, const Core::Color& color, float strokeWidth) override;
        void FillEllipse(const Core::PointF& center, float radiusX, float radiusY, const Core::Color& color) override;
        void DrawEllipse(const Core::PointF& center, float radiusX, float radiusY, const Core::Color& color, float strokeWidth) override;
        void DrawCurve(const Core::PointF& start, std::span<const Core::BezierSegment> segments, const Core::Color& color, float strokeWidth) override;
        void FillCurveArea(const Core::PointF& start, std::span<const Core::BezierSegment> segments, float baselineY, float gradientTop,
            const Core::Color& topColor, const Core::Color& bottomColor) override;
        void DrawString(std::wstring_view text, const Core::RectF& rect, const Core::Color& color, const Core::TextStyle& style) override;

        [[nodiscard]] Core::SizeF MeasureString(std::wstring_view text, const Core::TextStyle& style, float maxWidth) override;
        [[nodiscard]] std::vector<float> MeasureCaretPositions(std::wstring_view text, const Core::TextStyle& style) override;

        void DrawImage(const Core::ImageData& image, const Core::RectF& rect, float opacity) override;   // 그림 (같은 그림 · 버전은 비트맵 재사용)
        void PushClip(const Core::RectF& rect) override;
        void PopClip() override;
        void PushOpacity(float opacity) override;
        void PopOpacity() override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}