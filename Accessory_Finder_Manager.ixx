export module LostArkMultiTool.Views.AccessoryFinderManager;

import std;
import LostArkMultiTool.Core.AccessorySearchService;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;
import LostArkMultiTool.Views.AccessoryFinderView;

export namespace LostArkMultiTool::Views
{
    // 악세 검색기 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = AccessoryFinderView, 모델 = AccessorySearchService(경매장 악세 검색)
    // 매니저는 연마 효과 목록 불러오기 · 검색 조건 만들기 · 페이지 이동 · 오류 안내를 맡음
    class AccessoryFinderManager final : public Core::ToolManager, public Core::Singleton<AccessoryFinderManager>
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
        void OnAPIKeyChanged() override;

    private:
        friend class Core::Singleton<AccessoryFinderManager>;   // GetInstance만 생성 · 소멸할 수 있게

        AccessoryFinderManager();
        ~AccessoryFinderManager() override;

        // ===== 연마 효과 목록 =====
        void RequestOptions();         // 아직 불러오지 않았을 때만 (탭을 처음 열 때 · 키를 바꿨을 때)
        void FinishOptions();
        void ChangeOption(std::size_t slotIndex);   // 옵션을 바꾸면 그 옵션의 값 목록으로 값 칸을 채움
        void ApplyCategoryOptions();                // 지금 부위의 딜러 유효옵션만 연마 효과 칸에 채움 (목록을 받았을 때 · 부위를 바꿨을 때)

        // ===== 검색 =====
        void Search();                 // 화면 조건으로 첫 페이지
        void MovePage(int pageStep);   // 지난 조건으로 이전 · 다음 페이지
        void RequestSearch(Models::AccessorySearchQuery query);
        void FinishSearch();
        [[nodiscard]] Models::AccessorySearchQuery BuildQuery() const;

        // ===== 안내 =====
        [[nodiscard]] bool CheckRateLimit();   // 남은 호출이 부족하면 안내하고 false
        void ShowRequestError(const std::exception& error);   // 키 오류면 재입력 안내, 나머지는 메시지
        void HandleAPIKeyError(const std::string& message);
        void UpdatePageInfo();

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 서비스 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::AccessorySearchService> m_service;
        std::unique_ptr<AccessoryFinderView> m_view;

        Core::BackgroundTask<std::vector<Models::AuctionOption>> m_optionsTask;
        Core::BackgroundTask<Models::AccessorySearchPage> m_searchTask;

        std::vector<Models::AuctionOption> m_allOptions;       // API에서 받은 연마 효과 전체
        std::vector<Models::AuctionOption> m_options;          // 지금 부위의 유효옵션 (콤보 상자 항목 순서)
        bool m_isOptionsLoaded{};
        bool m_isOptionsFailed{};                              // 마지막 불러오기가 실패함 (자동 재시도 멈춤)
        std::optional<Models::AccessorySearchQuery> m_runningQuery;   // 조회 중인 조건
        std::optional<Models::AccessorySearchQuery> m_lastQuery;      // 마지막으로 결과를 받은 조건 (페이지 이동 기준)
        Models::AccessorySearchPage m_page;                           // 지금 보이는 결과
    };
}
