export module LostArkMultiTool.Views.ArkGemFinderManager;

import std;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.BoundGemRepository;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.PriceHistoryRepository;
import LostArkMultiTool.Core.ArkGemPriceService;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.BoundGemRosterData;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Views.ArkGemFinderView;

export namespace LostArkMultiTool::Views
{
    // 젬 시세 검색기 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = ArkGemFinderView, 모델 = ArkGemPriceService(거래소 질서 · 혼돈의 젬 시세) + BoundGemRepository(귀속 젬 보유 수량)
    class ArkGemFinderManager final : public Core::ToolManager, public Core::Singleton<ArkGemFinderManager>
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

        // ===== 화면 인식 (귀속 젬 툴팁 → 지금 접속한 캐릭터) =====
        enum class GemScanResult
        {
            GEM_APPLIED,         // 넣음
            GEM_NO_CHARACTER,    // 계산기에 그 캐릭터가 없음 (원정대 동기화 필요)
            GEM_UNCHANGED,       // 같은 수량
            GEM_SAVE_FAILED      // 저장 실패 (되돌림)
        };
        [[nodiscard]] GemScanResult ApplyScannedGem(const std::string& characterName, std::size_t gemIndex, std::size_t gradeIndex, int quantity);   // 화면에서 읽은 수량 넣기
        void SetVisionControl(std::function<bool()> isEnabled, std::function<void(bool)> setEnabled);   // [화면 인식] 상태 · 켜기 (Application이 귀속재료 계산기와 연결)

    private:
        friend class Core::Singleton<ArkGemFinderManager>;   // GetInstance만 생성 · 소멸할 수 있게

        ArkGemFinderManager();
        ~ArkGemFinderManager() override;

        // ===== 조회 · 자동 갱신 =====
        void RequestRefresh(bool isAuto);
        void SetAutoRefresh(bool isEnabled);
        void FinishRefresh();   // 백그라운드 조회가 끝났을 때 (화면 스레드에서)
        void HandleAPIKeyError(const std::string& message);

        // ===== 화면 갱신 =====
        void RefreshTables();   // 종류별로 나눠 두 표에 채움 (영웅 등급만) + 귀속 젬 계산기 + 상태 글자
        void RefreshBoundGems();   // 수량 × 단가로 계산기 표 채우기
        void ChangeQuantity(std::size_t gemIndex, std::size_t gradeIndex, const std::wstring& text);   // 수량 칸 확정 → 저장 → 다시 계산

        // ===== 귀속 젬 계산기 원정대 · 캐릭터 =====
        void SyncRoster(const std::wstring& mainCharacterName);   // 원정대 동기화 (백그라운드)
        void FinishRosterSync();                                  // 동기화가 끝났을 때 (화면 스레드에서)
        void LoadCharacterList();                                 // 캐릭터 선택 목록 다시 채우기 (★ 원정대 전체 ★ + 캐릭터)
        void SelectCharacter();                                   // 선택에 맞게 수량 잠금 · 표 갱신
        [[nodiscard]] Models::BoundGemCharacter* GetSelectedCharacter();   // 원정대 전체 · 선택 없음이면 nullptr
        void TrySave();

        // ===== 귀속 젬 가치 그래프 =====
        void RecordHistory();   // 시세가 있으면 오늘(06시 기준) 원정대 총 가치를 기록 → 저장 → 그래프 갱신
        void UpdateChart();
        void ShowAutoError(std::wstring_view title, const std::wstring& message);

        // ===== 젬 시세 일별 최저가 기록 · 그래프 =====
        void RecordPriceHistory();   // 조회가 성공할 때마다 표에 보이는 젬별 오늘(06시 기준) 최저가 기록 + 저장
        void SavePriceHistory();     // 기록 저장 (실패하면 알리고 다음 기회에 다시 시도)
        void SelectPriceRow(bool isOrder, std::size_t row);   // 표에서 고른 행의 젬을 그래프 대상으로
        void RefreshPriceChart();    // 고른 젬의 일별 최저가 그래프

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 서비스 결과를 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::ArkGemPriceService> m_service;
        std::unique_ptr<Core::BoundGemRepository> m_boundGemRepository;
        std::function<bool()> m_isVisionEnabled;      // 화면 인식 켜짐 (귀속재료 계산기 설정)
        std::function<void(bool)> m_setVisionEnabled; // 화면 인식 켜기 · 끄기
        std::unique_ptr<Core::PriceHistoryRepository> m_priceHistoryRepository;   // 젬 시세 일별 최저가 기록 (ArkGemFinderData.json)
        std::unique_ptr<ArkGemFinderView> m_view;

        Core::BackgroundTask<std::vector<Models::MarketItem>> m_refreshTask;
        Core::BackgroundTask<std::vector<Models::CharacterInfo>> m_rosterTask;   // 원정대 동기화
        double m_rosterValue{};                       // 마지막으로 계산한 원정대 귀속 젬 총 가치 (그래프 기록용)
        std::vector<std::string> m_characterNames;   // 캐릭터 선택 1번부터와 같은 순서의 이름 (0번 = 원정대 전체)
        Core::IntervalTimer m_autoTimer;                 // 자동 갱신 주기 (5분)

        bool m_isRunningAuto{};                          // 진행 중인 조회가 자동 갱신인지 (오류 알림 방식이 다름)
        bool m_isAutoRefreshWanted{};                    // 사용자가 켠 상태 (키 오류로 잠시 꺼도 기억 → 키를 바꾸면 다시 켬)
        std::optional<std::wstring> m_lastAutoError;     // 같은 자동 갱신 오류 알림을 반복해서 띄우지 않기 위함

        std::vector<std::string> m_orderKeys;            // 질서 표 행 순서대로의 기록 키
        std::vector<std::string> m_chaosKeys;            // 혼돈 표 행 순서대로의 기록 키
        std::optional<std::string> m_selectedPriceKey;   // 그래프에 보여 줄 젬 (표를 다시 채워도 유지)
        bool m_isPriceHistoryDirty{};                    // 저장하지 못한 기록이 있음 (종료할 때 한 번 더 시도)
    };
}
