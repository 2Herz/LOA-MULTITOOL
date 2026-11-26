export module LostArkMultiTool.Views.TrackerManager;

import std;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.GrowthCalculator;
import LostArkMultiTool.Core.GrowthDataRepository;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.RosterService;
import LostArkMultiTool.Core.Singleton;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.Season;
import LostArkMultiTool.Views.TrackerView;

export namespace LostArkMultiTool::Views
{
    // 화면 인식으로 읽은 낙원력을 반영한 결과
    enum class ScanApplyResult
    {
        SCAN_APPLIED,        // 이번 주차에 기록함
        SCAN_UNCHANGED,      // 이미 같은 값
        SCAN_NO_CHARACTER,   // 트래커에 없는 캐릭터
        SCAN_NO_WEEK,        // 기록할 이번 주차가 없음 (고른 시즌 기간 밖 등)
        SCAN_DECREASED       // 기록보다 작은 값 → 오인식으로 보고 무시
    };

    // 낙원력 트래커 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    // 화면구성 · 렌더링 = TrackerView, 모델 = GrowthDataRepository(저장 파일) + GrowthCalculator(통계) + RosterService(닉네임 변경 · 원정대 제외)
    // 매니저는 사용자 동작 → 모델 변경 → 화면 갱신을 이어 줌 (원본 Form1의 이벤트 처리 부분)
    class TrackerManager final : public Core::ToolManager, public Core::Singleton<TrackerManager>
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

        // 화면 인식으로 읽은 낙원력을 이번 주차에 기록 (귀속재료 계산기의 화면 인식이 부름)
        [[nodiscard]] ScanApplyResult ApplyScannedScore(const std::string& characterName, double score);

    private:
        friend class Core::Singleton<TrackerManager>;   // GetInstance만 생성 · 소멸할 수 있게

        TrackerManager();
        ~TrackerManager() override;

        // ===== 사용자 동작 =====
        void SyncRoster(const std::wstring& characterName);   // 원정대 동기화 (백그라운드)
        void AddWeek();
        void AddSeason();
        void EditPeriod();    // 지금 시즌의 기간(시작일 · 종료일) 바꾸기
        void UnlockWeek();    // 고른 지난 주차를 확인 후 잠깐 풀기
        void SelectSeason();
        void EditScore(std::size_t weekIndex, std::size_t characterIndex, std::optional<double> score);

        // ===== 백그라운드 작업이 끝났을 때 (화면 스레드에서) =====
        void FinishRosterSync();
        void AskNextRename();      // 닉네임 변경 후보를 하나씩 확인 (예 → 모든 시즌의 기록 이전)
        void FinishRenames();
        void HandleAPIKeyError(const std::string& message);

        // ===== 화면 갱신 =====
        void RefreshSeasonList();       // 시즌 선택 목록 (번호순, 지금 시즌 선택)
        void RefreshGridStructure();    // 열 구성: 주차 | 현재 원정대 | 원정대 제외 | 통계
        void PopulateGridData();        // 지금 시즌의 주차별 점수 + 통계 + 그래프
        void UpdateStatCells();
        void UpdateChart();
        void UpdateOverview();          // 통계 카드 4장 + 캐릭터별 목록 (저장된 주차 기록에서 계산한 값만)
        void TrySave();

        // ===== 시즌 기간 (주차 자동 추가 · 입력 잠금 · 이번 주차 포커스) =====
        void CheckSchedule();                          // 1분마다: 로스트아크 날짜(06시 기준)가 바뀌었으면 다시 적용
        void ApplyWeekSchedule(bool isFocusRequested); // 이번 주차까지 행 추가 → 잠금 → 안내 줄 → (요청 · 주차 변경 시) 포커스
        [[nodiscard]] bool IsWeekEditable(std::size_t weekIndex) const;   // 지금 시각 기준 (기간 없는 시즌은 항상 true)

        [[nodiscard]] Models::Season& GetCurrentSeason();

        Core::ToolContext* m_context{};   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)

        // 선언 순서 = 생성 순서 (화면은 모델을 쓰므로 반대 순서로 해제)
        std::unique_ptr<Core::GrowthDataRepository> m_repository;
        std::unique_ptr<Core::GrowthCalculator> m_calculator;
        std::unique_ptr<Core::RosterService> m_rosterService;
        std::unique_ptr<TrackerView> m_view;

        std::vector<std::string> m_columnCharacterNames;   // 캐릭터 열 순서대로 이름 (앞: 현재 원정대 = 입력 가능, 뒤: 원정대 제외)
        std::vector<int> m_seasonNumbers;                  // 시즌 선택 목록과 같은 순서의 시즌 번호

        Core::BackgroundTask<std::vector<Models::CharacterInfo>> m_rosterTask;   // 원정대 동기화

        Core::IntervalTimer m_scheduleTimer;                              // 주차 바뀜 확인 (1분)
        std::optional<std::chrono::year_month_day> m_lastGameDay;        // 마지막으로 적용한 로스트아크 날짜
        std::optional<std::size_t> m_lastWeekIndex;                       // 마지막으로 적용한 이번 주차 (바뀌면 포커스)
        std::optional<std::size_t> m_unlockedWeek;                        // [지난 주차 수정]으로 잠깐 푼 주차

        // 닉네임 변경 확인 진행 상태 (대화상자를 하나씩 띄우므로 끝날 때까지 다음 동기화는 받지 않음)
        std::vector<Core::RenameCandidate> m_pendingRenames;
        std::size_t m_renameIndex{};
        int m_renamedCount{};
        std::size_t m_syncedCount{};
        bool m_isRenaming{};
    };
}
