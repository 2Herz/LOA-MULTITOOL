export module LostArkMultiTool.Views.Application;

import std;
import LostArkMultiTool.Core.APIKeyStore;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.ServerConfigChecker;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.ServerConfig;
import LostArkMultiTool.Views.APIKeyDialog;
import LostArkMultiTool.Views.DX11Renderer;
import LostArkMultiTool.Views.MainWindow;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.OverlayNotifier;

export namespace LostArkMultiTool::Views
{
    // 프로그램 본체 (xmind의 Main)
    // 창 · 렌더러 · 공용 서비스(API 키 · API · 알림)를 소유하고, 최상위 매니저 3종을 탭으로 전환하며
    // 매 프레임 갱신 · 그리기 · 입력 나눠 주기를 맡김 (구현 클래스를 만드는 곳은 여기 한 곳 → 매니저는 인터페이스만 앎)
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
        void OnInput(const Core::InputEvent& event) override;
        void OnTrayCommand(TrayCommand command) override;
        void OnHiddenToTray() override;

    private:
        static constexpr std::size_t TOOL_COUNT{ 9 };   // 귀속재료 계산기 · 보석 최저가 검색기 · 낙원력 트래커 · 유각 시세 검색기 · 악세 검색기 · 강화책 검색기 · 젬 시세 검색기 · 전투력 시뮬레이터 · 재련 시뮬레이터

        // 마우스를 누른 곳 (뗄 때까지 같은 곳으로 입력을 보냄 → 탭 화면에서 끌다가 탭 막대로 넘어가도 끊기지 않음)
        enum class PointerOwner
        {
            POINTER_NONE,
            POINTER_HEADER,   // 왼쪽 메뉴 · 위 막대
            POINTER_TOOL      // 탭 내용
        };

        // ===== 매니저 =====
        void InitializeTools();
        void ShutdownTools();
        void SelectTool(std::size_t index);

        // ===== 원격 설정 (점검 스위치 · 업데이트 · 공지) =====
        void StartServerCheck(bool isStartup);   // isStartup: 실행 직후 확인 (확인 대화상자 표시)
        void FinishServerCheck();
        void LockForMaintenance(const std::optional<std::string>& maintenanceMessage);
        void ShowStartupNotices(const Models::ServerConfig& config);

        // ===== API 키 =====
        void OpenAPIKeyDialog(APIKeyDialogMode mode);
        [[nodiscard]] bool SaveAPIKey(const std::string& apiKey);

        // ===== 입력 =====
        [[nodiscard]] bool HandleShortcut(const Core::KeyEvent& event);
        void RouteInput(const Core::InputEvent& event);
        void RouteMouseButton(const Core::InputEvent& event, const Core::MouseButtonEvent& button);
        void SyncModalState();
        void UpdateCursor();

        // ===== 프레임 =====
        void Update(double deltaSeconds);
        void Render();
        void RenderSidebar();      // 왼쪽 메뉴 (로고 · 탭 목록 · API 키 설정 · API 상태)
        void RenderTopBar();       // 위 막대 (경로 표시 "멀티툴 › 탭 제목" · 버전)
        void RenderPageHeader();   // 탭 내용 위 머리말 (영문 소제목 · 큰 제목 · 설명)
        void LayoutHeader();

        [[nodiscard]] Core::RectF GetTabRect(std::size_t index) const noexcept;   // 왼쪽 메뉴에서 탭 항목 하나의 영역
        [[nodiscard]] Core::RectF GetContentRect() const noexcept;
        [[nodiscard]] Core::RectF GetScreenRect() const noexcept;
        [[nodiscard]] std::optional<std::size_t> FindTabAt(float x, float y) const noexcept;
        [[nodiscard]] bool IsInHeader(float x, float y) const noexcept;   // 왼쪽 메뉴 · 위 막대 · 페이지 머리말 안인지 (탭 내용 밖)
        [[nodiscard]] Core::ToolManager& GetActiveTool() const noexcept;

        std::array<std::reference_wrapper<Core::ToolManager>, TOOL_COUNT> m_tools;   // 탭 순서 (매니저는 싱글톤이 소유)

        // 선언 순서 = 생성 순서, 해제는 반대 순서 (알림 · 렌더러가 창보다 먼저 해제됨)
        std::unique_ptr<MainWindow> m_window;
        std::unique_ptr<DX11Renderer> m_renderer;
        std::unique_ptr<Core::APIKeyStore> m_keyStore;
        std::unique_ptr<Core::MarketAPI> m_marketAPI;
        std::unique_ptr<UI::OverlayNotifier> m_notifier;
        std::unique_ptr<Core::ToolContext> m_toolContext;   // 매니저에 빌려주는 공용 서비스 묶음 (주소가 바뀌지 않게 힙에)
        std::unique_ptr<Core::ServerConfigChecker> m_configChecker;
        Core::BackgroundTask<std::optional<Models::ServerConfig>> m_configTask;   // m_configChecker보다 먼저 해제 (끝날 때까지 기다림)
        Core::IntervalTimer m_configTimer;                  // 실행 중 점검 스위치 확인 (30분)
        UI::Dialog* m_serverCheckDialog{};                  // 실행 직후 확인 대화상자 (m_notifier가 소유, 닫으면 비움)
        UI::ControlHost m_headerHost;                       // 왼쪽 메뉴 아래 버튼 (API 키 설정)
        UI::Button* m_apiKeyButton{};                       // m_headerHost가 소유

        std::size_t m_activeIndex{};
        std::optional<std::size_t> m_hoverIndex;            // 마우스를 올린 탭
        std::size_t m_initializedToolCount{};               // Initialize가 끝난 매니저 수 (그만큼만 Shutdown)
        PointerOwner m_pointerOwner{ PointerOwner::POINTER_NONE };
        bool m_isPointerInHeader{};
        bool m_isModal{};                                   // 대화상자가 열려 있음 (탭 · 단축키 입력 막힘)
        bool m_hasAPIKey{};
        bool m_isAPIKeyDialogOpen{};                        // API 키 창이 두 번 열리지 않게
        bool m_isStartupCheck{};                            // 지금 진행 중인 원격 설정 확인이 실행 직후 확인인지
        bool m_isMaintenanceLocked{};                       // 점검 중 → 안내만 보여 주고 종료 (탭 화면 갱신 · 자동 갱신 멈춤)
        float m_indicatorPosition{};                        // 왼쪽 메뉴 선택 표시 위치 (탭 번호 단위, 선택한 탭을 부드럽게 따라감)
        float m_contentFade{};                              // 탭을 바꾼 직후 내용 영역을 덮는 배경색 투명도 (1 → 0)
    };
}