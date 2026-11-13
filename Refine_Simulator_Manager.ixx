export module LostArkMultiTool.Views.RefineSimulatorManager;

import std;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.RefineSimulationService;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.RefinePlan;
import LostArkMultiTool.Views.RefineSimulatorView;

export namespace LostArkMultiTool::Views
{
    // 재련 시뮬레이터 최상위 매니저 (로아업 통합 재련 형식: 일반 재련 + 상급 재련 + 계승을 한 번에)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = RefineSimulatorView, 모델 = RefineSimulationService(시작 · 목표 · 재료 가격 · 귀속, 저장),
    // 계산 = RefineCalculator (백그라운드 스레드, 비용표는 가격이 바뀔 때만 다시 만듦)
    class RefineSimulatorManager final : public Core::ToolManager, public Core::Singleton<RefineSimulatorManager>
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
        friend class Core::Singleton<RefineSimulatorManager>;   // GetInstance만 생성 · 소멸할 수 있게

        // 백그라운드 계산 결과 (요청 · 비용표를 함께 돌려받아 화면 스레드에서 캐시)
        struct CalculationOutput
        {
            std::shared_ptr<const Models::RefineCostCatalog> m_catalog;
            std::uint64_t m_priceRevision{};
            Models::RefineRequest m_request;
            Models::RefineRouteResult m_result;
        };

        RefineSimulatorManager();
        ~RefineSimulatorManager() override;

        // ===== 시세 조회 · 자동 갱신 =====
        void RequestRefresh(bool isAuto);
        void SetAutoRefresh(bool isEnabled);
        void FinishRefresh();   // 백그라운드 조회가 끝났을 때 (화면 스레드에서)
        void HandleAPIKeyError(const std::string& message);
        void ShowAutoError(std::wstring_view title, const std::wstring& message);

        // ===== 계산 =====
        void OnCalculateClicked();   // [계산] 버튼: 지금 입력으로 다시 계산 (진행 표시 + 완료 토스트)
        void StartCalculation();
        void FinishCalculation();
        void MarkChanged(bool isPriceChanged);   // 입력이 바뀜 → 저장 + 다시 계산 + 화면 갱신

        // ===== 사용자 입력 =====
        void OnStartPresetSelected(std::size_t presetIndex);
        void RequestLookup(const std::wstring& characterName);   // 캐릭터 이름으로 무기고 조회 시작 (백그라운드)
        void FinishLookup();                                      // 조회가 끝났을 때 (화면 스레드에서)
        void ApplyArmory(const Models::CharacterArmory& armory);  // 무기고 장비 → 시작 상태 (표를 직접 고친 것과 같은 경로)
        void OnTargetModeSelected(std::size_t modeIndex);
        void OnTargetItemLevelChanged(const std::wstring& text);
        void OnSlotCellClicked(std::size_t row, RefineSlotColumn column);
        void OnSlotCellEdited(std::size_t row, RefineSlotColumn column, const std::wstring& text);
        void OnMaterialCellClicked(std::size_t row, RefineMaterialColumn column);
        void OnMaterialCellEdited(std::size_t row, RefineMaterialColumn column, const std::wstring& text);

        // ===== 화면 갱신 =====
        void RefreshAll();
        void RefreshSetup();
        void RefreshSlots();
        void RefreshMaterials();
        void RefreshRoute();
        void RefreshStatus();
        void SaveData();

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 서비스 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::RefineSimulationService> m_service;
        std::unique_ptr<RefineSimulatorView> m_view;

        Core::BackgroundTask<std::vector<Core::RefinePriceQuote>> m_refreshTask;
        Core::BackgroundTask<CalculationOutput> m_calculationTask;
        Core::BackgroundTask<Models::CharacterArmory> m_armoryTask;   // 무기고 조회 (결과는 화면 스레드에서 받음)
        std::wstring m_lookupName;                                    // 지금 조회 중인(또는 마지막으로 조회한) 캐릭터 이름
        std::optional<std::wstring> m_lookupMessage;                  // 조회 결과 · 오류 안내 (상태 줄에 표시)
        Core::IntervalTimer m_autoTimer;                 // 자동 갱신 주기 (5분)

        std::shared_ptr<const Models::RefineCostCatalog> m_catalog;   // 마지막 비용표 (가격이 그대로면 다시 씀)
        std::uint64_t m_catalogRevision{};
        std::optional<Models::RefineRouteResult> m_result;            // 마지막 계산 결과
        Models::RefineRequest m_resultRequest;                        // 그 결과의 입력 (계산 중 입력이 바뀌어도 화면은 결과와 맞게)
        std::optional<std::wstring> m_calculationError;
        bool m_isCalculationDirty{ true };
        bool m_isManualCalculation{};                                 // [계산] 버튼으로 시작한 계산이 끝나기를 기다리는 중 (끝나면 토스트)

        bool m_isRunningAuto{};
        bool m_isAutoRefreshWanted{};
        std::optional<std::wstring> m_lastAutoError;
    };
}
