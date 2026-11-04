export module LostArkMultiTool.Views.Application;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Views.MainWindow;
import LostArkMultiTool.Views.DX11Renderer;

export namespace LostArkMultiTool::Views
{
    // 프로그램 본체 (xmind의 Main)
    // 창 · 렌더러를 소유하고, 최상위 매니저 3종을 탭으로 전환하며 매 프레임 갱신 · 그리기를 맡김
    class Application final : public WindowEventHandler
    {
    public:
        Application();
        ~Application() override;

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;

        // 창을 띄우고 닫힐 때까지 실행 (반환값: 프로그램 종료 코드)
        [[nodiscard]] int Run(int showCommand);

        // ===== WindowEventHandler =====
        void OnResize(int pixelWidth, int pixelHeight) override;
        void OnDPIChanged(float dpi) override;
        void OnMouseMove(float x, float y) override;
        void OnMouseLeave() override;
        void OnMouseDown(float x, float y) override;
        void OnKeyDown(unsigned virtualKey, bool isCtrlDown, bool isShiftDown) override;

    private:
        static constexpr std::size_t TOOL_COUNT{ 3 };   // 귀속재료 계산기 · 보석 최저가 검색기 · 낙원력 트래커

        void InitializeTools();
        void ShutdownTools();
        void SelectTool(std::size_t index);

        void Update(double deltaSeconds);
        void Render();
        void RenderTabBar();

        [[nodiscard]] Core::RectF GetTabRect(std::size_t index) const noexcept;
        [[nodiscard]] Core::RectF GetContentRect() const noexcept;
        [[nodiscard]] std::optional<std::size_t> FindTabAt(float x, float y) const noexcept;
        [[nodiscard]] Core::ToolManager& GetActiveTool() const noexcept;

        std::array<std::reference_wrapper<Core::ToolManager>, TOOL_COUNT> m_tools;   // 탭 순서 (매니저는 싱글톤이 소유)
        std::unique_ptr<MainWindow> m_window;        // 렌더러보다 먼저 선언 → 렌더러(스왑체인)가 창보다 먼저 해제됨
        std::unique_ptr<DX11Renderer> m_renderer;

        std::size_t m_activeIndex{};
        std::optional<std::size_t> m_hoverIndex;     // 마우스를 올린 탭
        std::size_t m_initializedToolCount{};        // Initialize가 끝난 매니저 수 (그만큼만 Shutdown)
        float m_indicatorPosition{};                 // 탭 밑줄 위치 (탭 번호 단위, 선택한 탭을 부드럽게 따라감)
        float m_contentFade{};                       // 탭을 바꾼 직후 내용 영역을 덮는 배경색 투명도 (1 → 0)
    };
}