export module LostArkMultiTool.Views.CalculatorManager;

import std;
import LostArkMultiTool.Core.AssetCalculator;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.DataRepository;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.Character;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Views.CalculatorView;

export namespace LostArkMultiTool::Views
{
    // 귀속재료 계산기 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = CalculatorView, 모델 = DataRepository(저장 파일) + AssetCalculator(시세 · 계산)
    // 매니저는 둘 사이에서 사용자 동작 → 모델 변경 → 화면 갱신을 이어 줌 (원본 Form1의 이벤트 처리 부분)
    class CalculatorManager final : public Core::ToolManager, public Core::Singleton<CalculatorManager>
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

    private:
        friend class Core::Singleton<CalculatorManager>;   // GetInstance만 생성 · 소멸할 수 있게

        CalculatorManager();
        ~CalculatorManager() override;

        // ===== 사용자 동작 =====
        void SyncRoster(const std::wstring& mainCharacterName);   // 원정대 동기화 (백그라운드)
        void StartRefresh(bool isAuto);                             // 시세 갱신 (백그라운드)
        void SetAutoRefresh(bool isEnabled);
        void SelectCharacter();
        void ResetInput();   // [입력 초기화] (고른 캐릭터의 수량을 확인 후 모두 0으로)
        void EditQuantity(MaterialTier tier, std::size_t materialIndex, double quantity);

        // ===== 백그라운드 작업이 끝났을 때 (화면 스레드에서) =====
        void FinishRefresh();
        void FinishRosterSync();
        void HandleAPIKeyError(const std::string& message);

        // ===== 화면 갱신 =====
        void LoadCharacterList();
        void RefreshView();   // 계산이 끝날 때마다 호출 (원본 RefreshUI)
        [[nodiscard]] double UpdateChart();
        void ApplyQuantityLock(const Models::Character& character);

        [[nodiscard]] std::vector<const Models::Character*> GetDisplayedCharacters() const;
        [[nodiscard]] double CalculateRosterTotal() const;   // 모든 캐릭터의 상위 + 하위 재료값 합계
        void ShowRosterQuantities();                         // 재료마다 모든 캐릭터의 상위 · 하위 보유수량 합계 → ★ 원정대 자산 ★ 행 (파편 · 숨결은 총합 하나)
        [[nodiscard]] std::optional<std::string> GetSelectedCharacterName() const;
        void TrySave();

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (계산기는 저장소를, 화면은 둘을 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::DataRepository> m_repository;
        std::unique_ptr<Core::AssetCalculator> m_calculator;
        std::unique_ptr<CalculatorView> m_view;

        std::vector<std::string> m_characterNames;   // 캐릭터 선택 목록과 같은 순서의 이름 (선택 번호 = 이름 번호)

        Core::BackgroundTask<void> m_refreshTask;                                     // 시세 갱신
        Core::BackgroundTask<std::vector<Models::CharacterInfo>> m_rosterTask;        // 원정대 동기화
        Core::IntervalTimer m_autoRefreshTimer;
        bool m_isAutoRefreshEnabled{};
        bool m_isAutoRefreshRun{};   // 지금 진행 중인 시세 갱신이 자동 갱신인지 (완료 · 오류 알림 방식이 다름)
    };
}