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
import LostArkMultiTool.Vision.FrameImage;       // 픽셀 사각형
import LostArkMultiTool.Vision.GemTooltipDetector; // 젬 값 이름
import LostArkMultiTool.Vision.NameplateMemory;  // 이름표 모양 기억
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

    // 화면 인식으로 알아낸 현재 캐릭터 상태
    enum class CurrentCharacterState
    {
        CHARACTER_VISION_OFF,   // 화면 인식이 꺼져 있음
        CHARACTER_NO_ROSTER,    // 원정대를 아직 불러오지 않음 (비교할 이름이 없음)
        CHARACTER_CHECKING,     // 확인 중 (켠 직후 · 로딩 화면 뒤)
        CHARACTER_KNOWN         // 확인됨
    };

    // 현재 캐릭터를 알아본 방법
    enum class CurrentCharacterSource
    {
        SOURCE_NAMEPLATE,      // 머리 위 이름표
        SOURCE_INFO_WINDOW,    // 캐릭터 정보 창
        SOURCE_MANUAL          // 직접 고름 ([변경])
    };

    // 인게임 오버레이 현재 캐릭터 패널 · 설정 탭 내용
    struct CurrentCharacterSummary
    {
        CurrentCharacterState m_state{ CurrentCharacterState::CHARACTER_VISION_OFF };   // 상태
        CurrentCharacterSource m_source{ CurrentCharacterSource::SOURCE_NAMEPLATE };    // 알아본 방법 (확인됐을 때)
        std::wstring m_name;                         // 캐릭터 이름 (확인됐을 때)
        std::wstring m_className;                    // 직업 (모르면 빈 글자)
        double m_itemLevel{};                        // 저장된 아이템 레벨 (모르면 0)
        std::optional<double> m_readItemLevel;       // 캐릭터 정보 창에서 읽은 아이템 레벨 (안 읽었으면 nullopt)
        bool m_isOutdated{};                         // 읽은 레벨이 저장된 레벨과 다름 → [갱신] 필요
        bool m_isSyncing{};                          // 원정대 갱신 중
        bool m_isAutoEnabled{ true };                // 자동 인식 켜짐
        std::vector<std::wstring> m_candidates;      // [변경]으로 고를 원정대 이름 (계산기 선택 순서)
        std::optional<std::size_t> m_currentIndex;   // 후보 중 현재 캐릭터 번호
        std::size_t m_aliasCount{};                  // 직접 고른 캐릭터(읽은 글자 별칭) 수
        std::size_t m_shapeCount{};                  // 이름표 모양을 기억한 캐릭터 수
    };

    // 설정 탭 "인식 설정"에 보여 줄 화면 인식 상태
    struct VisionStatusSnapshot
    {
        Vision::WatcherStatus m_status;            // 감시 상태 · 안내
        Vision::WatcherDiagnostics m_diagnostics;  // 미리 보기 · 상자 · 처리 시간 · 마지막 값
        Vision::PixelRect m_topBar;                // 맨 위 재화 줄 영역 (프레임 기준)
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
        [[nodiscard]] CurrentCharacterSummary GetCurrentCharacter() const;  // 화면 인식으로 알아낸 현재 캐릭터 (화면 스레드에서)
        void PickCurrentCharacter(std::size_t index);                       // [변경]: 후보 번호로 현재 캐릭터 직접 고르기 (방금 못 맞춘 이름은 별칭으로 기억)
        void RefreshCurrentCharacter();                                     // [갱신]: 현재 캐릭터 기준으로 원정대 정보 다시 받기

        // ===== 설정 탭 (오버레이 · 인식 설정) =====
        [[nodiscard]] bool IsVisionEnabled() const noexcept;                 // 화면 인식 켜짐
        void SetVisionEnabled(bool isEnabled);                              // 화면 인식 켜기 · 끄기 (계산기 체크 상자도 맞춤)
        void RestartVision();                                               // [다시 찾기]: 감시를 처음부터 다시 (게임 창 다시 찾기)
        void SetCharacterAutoEnabled(bool isEnabled);                       // 현재 캐릭터 자동 인식 켜기 · 끄기 (저장)
        void ClearNameplateShapes();                                        // 기억한 이름표 모양 모두 지우기
        void ClearCharacterAliases();                                       // 기억한 읽은 글자(별칭) 모두 지우기
        [[nodiscard]] int GetCharacterIntervalSeconds() const noexcept;      // 현재 캐릭터 확인 간격 (초)
        void SetCharacterIntervalSeconds(int seconds);                      // 간격 바꾸기 (저장)
        void SetVisionPreview(bool isEnabled);                              // 미리 보기 그림 만들기 (설정 탭이 보일 때만)
        [[nodiscard]] std::optional<VisionStatusSnapshot> GetVisionStatus() const;   // 화면 인식 상태 (꺼져 있으면 nullopt)
        [[nodiscard]] std::size_t GetRosterCount() const noexcept;           // 원정대 캐릭터 수
        [[nodiscard]] std::optional<Vision::BossHealthSample> GetBossHealth() const;   // 마지막 보스 체력 (화면 인식이 꺼졌거나 보스가 없으면 nullopt)
        [[nodiscard]] std::optional<Vision::CombatSample> GetCombatStats() const;      // 마지막 전투 분석기 (화면 인식이 꺼졌거나 안 보이면 nullopt)
        void SetCombatScan(bool isEnabled);                                 // 전투 분석기 읽기 (딜 지분 패널이 보일 때만)
        [[nodiscard]] std::optional<Vision::BuffSample> GetBuffs() const;   // 마지막 버프 · 디버프 (화면 인식이 꺼졌거나 오래됐으면 nullopt)
        void SetBuffScan(bool isEnabled);                                   // 버프 · 디버프 읽기 (버프 타이머 패널이 보일 때만)
        void RequestRefresh();                                              // 시세 갱신 (단축키 Ctrl+Shift+F5)

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
        void ApplyGemScan(const Vision::GemValueKey& key, int quantity);   // 젬 (귀속 · 거래 가능 · 가공) → 젬 시세 검색기 귀속 젬 계산기
        void SetCurrentCharacter(const std::string& name, CurrentCharacterSource source, std::optional<double> readItemLevel);   // 현재 캐릭터 바꾸기 (계산기 선택도 따라감)
        void ClearCurrentCharacter();                         // 현재 캐릭터 지우기 (로딩 화면 · 화면 인식 켜고 끄기)
        void SelectCharacterByName(const std::string& name);  // 이름으로 캐릭터 고르기 (이미 골랐으면 그대로)
        void PushCharacterNames();                            // 감시기에 원정대 이름 넘기기 (현재 캐릭터 비교용)
        [[nodiscard]] std::optional<std::string> GetScanTarget();   // 인식 값을 넣을 캐릭터 (현재 캐릭터를 모르면 안내 후 nullopt)

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
        std::shared_ptr<Vision::NameplateMemory> m_nameplateMemory; // 이름표 모양 기억 (감시 스레드와 공유)
        std::optional<std::string> m_currentCharacter;              // 화면에서 확인한 현재 캐릭터 (모르면 nullopt → 인식 값을 넣지 않음)
        CurrentCharacterSource m_currentSource{ CurrentCharacterSource::SOURCE_NAMEPLATE };   // 알아본 방법
        std::optional<double> m_readItemLevel;                      // 캐릭터 정보 창에서 읽은 아이템 레벨
        std::optional<std::string> m_lastUnknownName;               // 정보 창 이름 자리에서 읽었지만 원정대와 안 맞은 글자 (UTF-8)
        std::chrono::steady_clock::time_point m_lastUnknownAt;      // 그 글자를 읽은 시각 (오래되면 별칭으로 쓰지 않음)
        bool m_isWaitNoticeShown{};                                 // "캐릭터 확인 중" 안내를 이미 띄웠는지 (확인될 때까지 한 번만)
        bool m_isPreviewWanted{};                                   // 설정 탭이 미리 보기를 원하는지 (감시를 다시 만들어도 유지)
        bool m_isCombatWanted{};                                    // 딜 지분 패널이 전투 분석기 읽기를 원하는지 (감시를 다시 만들어도 유지)
        bool m_isBuffWanted{};                                      // 버프 타이머 패널이 버프 읽기를 원하는지 (감시를 다시 만들어도 유지)
    };
}
