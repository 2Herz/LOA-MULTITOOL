export module LostArkMultiTool.Views.RefineBookFinderManager;

import std;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.RefineBookPriceService;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Views.RefineBookFinderView;

export namespace LostArkMultiTool::Views
{
    // 강화책 검색기 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = RefineBookFinderView, 모델 = RefineBookPriceService(거래소 야금술 · 재봉술 시세)
    class RefineBookFinderManager final : public Core::ToolManager, public Core::Singleton<RefineBookFinderManager>
    {
    public:
        [[nodiscard]] std::wstring_view GetTabTitle() const noexcept override;

        void Initialize(Core::ToolContext& context) override;
        void Shutdown() override;

        void OnActivated() override;
        void OnDeactivated() override;

        void Update(double deltaSeconds) override;
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea) override;

        [[nodiscard]] bool HandleInput(const Core::InputEvent& event) override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;
        [[nodiscard]] std::optional<std::wstring> GetBackgroundStatus() const override;
        void OnAPIKeyChanged() override;

    private:
        friend class Core::Singleton<RefineBookFinderManager>;   // GetInstance만 생성 · 소멸할 수 있게

        RefineBookFinderManager();
        ~RefineBookFinderManager() override;

        // ===== 조회 · 자동 갱신 =====
        void RequestRefresh(bool isAuto);
        void SetAutoRefresh(bool isEnabled);
        void FinishRefresh();   // 백그라운드 조회가 끝났을 때 (화면 스레드에서)
        void HandleAPIKeyError(const std::string& message);

        // ===== 화면 갱신 =====
        void RefreshTables();   // 종류별로 나눠 두 표에 채움 + 상태 글자
        void ShowAutoError(std::wstring_view title, const std::wstring& message);

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 서비스 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::RefineBookPriceService> m_service;
        std::unique_ptr<RefineBookFinderView> m_view;

        Core::BackgroundTask<std::vector<Models::MarketItem>> m_refreshTask;
        Core::IntervalTimer m_autoTimer;                 // 자동 갱신 주기 (5분)

        bool m_isRunningAuto{};                          // 진행 중인 조회가 자동 갱신인지 (오류 알림 방식이 다름)
        bool m_isAutoRefreshWanted{};                    // 사용자가 켠 상태 (키 오류로 잠시 꺼도 기억 → 키를 바꾸면 다시 켬)
        std::optional<std::wstring> m_lastAutoError;     // 같은 자동 갱신 오류 알림을 반복해서 띄우지 않기 위함
    };
}
