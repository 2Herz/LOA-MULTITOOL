export module LostArkMultiTool.Views.DX11Renderer;

import std;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Views
{
    // Direct3D 11 장치 + DXGI 스왑체인 위에 Direct2D · DirectWrite로 2D 화면(탭 · 표 · 그래프 · 글자)을 그림
    // D3D11 장치를 기반으로 두어 나중에 3D 화면을 같은 창에 함께 그릴 수 있음
    // COM 객체(DirectX 타입)는 구현 파일 안에만 둠 → 다른 모듈은 Windows·DirectX 헤더를 몰라도 됨
    class DX11Renderer final : public Core::Renderer
    {
    public:
        // windowHandle: MainWindow::GetNativeHandle(), dpi: 창이 놓인 모니터 배율
        DX11Renderer(void* windowHandle, float dpi);
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
        void DrawLine(float startX, float startY, float endX, float endY, const Core::Color& color, float strokeWidth) override;
        void DrawString(std::wstring_view text, const Core::RectF& rect, const Core::Color& color, const Core::TextStyle& style) override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}