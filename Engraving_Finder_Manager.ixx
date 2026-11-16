export module LostArkMultiTool.Views.EngravingFinderManager;

import std;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.EngravingPriceService;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.PriceHistoryRepository;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.EngravingQuote;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Views.EngravingFinderView;

export namespace LostArkMultiTool::Views
{
    // 유각 시세 검색기 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = EngravingFinderView, 모델 = EngravingPriceService(거래소 유물 각인서 시세)
    // 매니저는 사용자 동작 → 조회 · 검색 · 정렬 → 화면 갱신을 이어 줌
    class EngravingFinderManager final : public Core::ToolManager, public Core::Singleton<EngravingFinderManager>
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
        friend class Core::Singleton<EngravingFinderManager>;   // GetInstance만 생성 · 소멸할 수 있게

        EngravingFinderManager();
        ~EngravingFinderManager() override;

        // ===== 조회 · 자동 갱신 =====
        void RequestRefresh(bool isAuto);
        void SetAutoRefresh(bool isEnabled);
        void FinishRefresh();   // 백그라운드 조회가 끝났을 때 (화면 스레드에서)
        void HandleAPIKeyError(const std::string& message);

        // ===== 화면 갱신 =====
        void RefreshTable();    // 검색어로 거르고 정렬해서 표에 채움 + 상태 글자
        [[nodiscard]] EngravingStats BuildStats(const std::vector<Models::EngravingQuote>& quotes) const;   // 통계 카드 값 계산
        void ShowAutoError(std::wstring_view title, const std::wstring& message);

        // ===== 일별 최저가 기록 · 그래프 =====
        void RecordHistory();    // 조회가 성공할 때마다 각인서별 오늘(06시 기준) 최저가 기록 + 저장
        void SaveHistory();      // 기록 저장 (실패하면 알리고 다음 기회에 다시 시도)
        void SelectRow();        // 표에서 고른 행의 각인서를 그래프 대상으로
        void RefreshChart();     // 고른 각인서의 일별 최저가 그래프

        // ===== 즐겨찾기 =====
        void ToggleFavorite();          // 고른 각인서 즐겨찾기 추가 ↔ 해제 + 저장
        void RefreshFavoriteButton();   // 고른 각인서에 맞게 [즐겨찾기] 버튼 글자 · 활성 상태

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 서비스 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::EngravingPriceService> m_service;
        std::unique_ptr<Core::PriceHistoryRepository> m_historyRepository;   // 일별 최저가 기록 (EngravingFinderData.json)
        std::unique_ptr<EngravingFinderView> m_view;

        Core::BackgroundTask<std::vector<Models::MarketItem>> m_refreshTask;
        Core::IntervalTimer m_autoTimer;                 // 자동 갱신 주기 (5분)

        bool m_isRunningAuto{};                          // 진행 중인 조회가 자동 갱신인지 (오류 알림 방식이 다름)
        bool m_isAutoRefreshWanted{};                    // 사용자가 켠 상태 (키 오류로 잠시 꺼도 기억 → 키를 바꾸면 다시 켬)
        std::optional<std::wstring> m_lastAutoError;     // 같은 자동 갱신 오류 알림을 반복해서 띄우지 않기 위함

        std::vector<std::string> m_rowKeys;              // 표 행 순서대로의 기록 키 (정렬 · 검색 결과와 같은 순서)
        std::optional<std::string> m_selectedKey;        // 그래프에 보여 줄 각인서 (표를 다시 채워도 유지)
        bool m_isHistoryDirty{};                         // 저장하지 못한 기록이 있음 (종료할 때 한 번 더 시도)
    };
}
