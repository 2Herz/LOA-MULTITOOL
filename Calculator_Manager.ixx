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
import LostArkMultiTool.Vision.ScreenWatcher;    // 화면 인식 감시기
import LostArkMultiTool.Vision.VisionSettings;   // 화면 인식 켜기 설정

export namespace LostArkMultiTool::Views
{
    // 인게임 오버레이 재료 패널 한 줄
    struct CalculatorOverlayRow
    {
        std::wstring m_label;    // 재료 이름 ("파괴석")
        double m_quantity{};     // 고른 캐릭터 보유 수량 (레벨에 맞는 상위 · 하위)
        double m_unitPrice{};    // 단가 (골드, 시세가 없으면 0)
        double m_priceDiff{};    // 단가 변동 (지난 갱신 대비)
        double m_value{};        // 재료값 (골드)
    };

    // 인게임 오버레이 재료 패널 내용
    struct CalculatorOverlaySummary
    {
        std::wstring m_characterName;               // 고른 캐릭터 (없으면 빈 글자)
        std::vector<CalculatorOverlayRow> m_rows;   // 재료 줄 (MAT_KEYS 순서)
        double m_characterTotal{};                  // 캐릭터 자산 (골드)
        double m_rosterTotal{};                     // 원정대 자산 (골드)
        bool m_hasPrices{};                         // 시세를 받아 왔는지
        bool m_isVisionOn{};                        // 화면 인식이 켜져 있는지
    };

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

        [[nodiscard]] CalculatorOverlaySummary GetOverlaySummary() const;   // 인게임 오버레이 재료 패널용 요약 (화면 스레드에서)

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

        // ===== 화면 인식 (게임 화면 툴팁 · 상단바 · 낙원 UI → 고른 캐릭터에 반영) =====
        void SetVision(bool isEnabled);                       // [화면 인식] 켜기 · 끄기 (설정 저장)
        void StartVision();                                   // 감시기 만들고 인식기 등록 후 시작
        void ApplyScanResults();                              // 확정된 인식 결과 반영 (화면 스레드에서)
        void ApplyMaterialScan(const std::string& valueKey, double quantity);   // 재료 수량 → 계산기 칸
        void ApplyParadiseScan(double score);                 // 낙원력 → 낙원력 트래커 이번 주차

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

        std::unique_ptr<Vision::VisionSettings> m_visionSettings;   // 화면 인식 켜기 설정
        std::unique_ptr<Vision::ScreenWatcher> m_screenWatcher;     // 화면 인식 감시기 (켜져 있을 때만)
    };
}