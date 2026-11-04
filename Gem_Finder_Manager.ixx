export module LostArkMultiTool.Views.GemFinderManager;

import std;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.GemDataRepository;
import LostArkMultiTool.Core.GemPriceService;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Services.RefreshScheduler;
import LostArkMultiTool.Views.GemFinderView;

export namespace LostArkMultiTool::Views
{
    // 보석 최저가 검색기 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = GemFinderView, 모델 = GemDataRepository(저장 파일) + GemPriceService(시세 · 알림 · 기록)
    // 매니저는 사용자 동작 → 모델 변경 → 화면 갱신을 이어 주고, 자동 갱신 일정은 RefreshScheduler에 맡김 (원본 MainForm의 이벤트 처리 부분)
    class GemFinderManager final : public Core::ToolManager, public Core::Singleton<GemFinderManager>
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
        friend class Core::Singleton<GemFinderManager>;   // GetInstance만 생성 · 소멸할 수 있게

        GemFinderManager();
        ~GemFinderManager() override;

        // ===== 자동 갱신 · 조회 =====
        void RequestRefresh(Services::RefreshMode requestedMode, bool isAuto);   // 전체 / 감시 갱신 공통 (원본 RefreshPricesAsync 앞부분)
        void SetAutoRefresh(bool isEnabled, bool isPersisted);                   // isPersisted = false면 설정 파일에 남기지 않음
        void ChangeIntervals();
        void SaveSettings();

        // ===== 백그라운드 조회가 끝났을 때 (화면 스레드에서) =====
        void FinishRefresh();
        void HandleAPIKeyError(const std::string& message);

        // ===== 알림 · 보석 선택 =====
        void SelectGem(const Models::GemSelection& gem);   // 표의 칸 클릭 → 그래프 · 알림 입력칸에 그 보석
        void SelectAlarm(std::size_t alarmIndex);
        void AddAlarm();
        void RemoveAlarm();

        // ===== 화면 갱신 =====
        void RefreshView();   // 갱신이 끝날 때마다 호출 (원본 RefreshUI)
        void RefreshPriceGrid();
        void RefreshAlarmList();
        void RefreshChart();
        void UpdateStatusText();
        void UpdateRateLimitText();
        void NotifyAlarms(const std::vector<Core::ReachedAlarm>& reached);
        void ShowAutoError(std::wstring_view title, const std::wstring& message);
        void ShowSaveError(const std::exception& error);

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (서비스는 저장소를, 화면은 서비스 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::GemDataRepository> m_repository;
        std::unique_ptr<Core::GemPriceService> m_service;
        std::unique_ptr<GemFinderView> m_view;

        Services::RefreshScheduler m_scheduler;
        Core::BackgroundTask<std::vector<Core::FetchedPrice>> m_refreshTask;
        Core::IntervalTimer m_autoTimer;                 // 감시 주기마다 (전체 주기 판단은 스케줄러가 누적 시간으로)

        Services::RefreshMode m_runningMode{ Services::RefreshMode::REFRESH_FULL };   // 진행 중인 조회 종류
        bool m_isRunningAuto{};             // 진행 중인 조회가 자동 갱신인지 (오류 알림 방식이 다름)
        bool m_isFullRefreshPending{};      // 조회 중에 전체 갱신 요청이 오면 끝나고 바로 실행
        bool m_isPendingFullAuto{ true };   // 예약된 전체 갱신이 자동 요청인지 (자동 갱신을 끄면 취소)
        bool m_isAutoRestorePending{};      // 지난번에 켜 둔 자동 갱신을 첫 프레임에 다시 켬
        std::optional<std::wstring> m_lastAutoError;     // 같은 자동 갱신 오류 알림을 반복해서 띄우지 않기 위함
        std::optional<std::string> m_selectedAlarmKey;   // 알림 목록에서 선택한 알림 (보석 키, 마우스 · 키보드 모두 반영)
    };
}