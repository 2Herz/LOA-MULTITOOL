export module LostArkMultiTool.Vision.ScreenWatcher;  // 화면 상시 감시 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 미리 보기 그림 형식 가져오기
import LostArkMultiTool.Vision.BossHealthDetector;  // 보스 체력 형식 가져오기
import LostArkMultiTool.Vision.CharacterDetector;  // 현재 캐릭터 별칭 형식 가져오기
import LostArkMultiTool.Vision.CombatAnalyzerDetector;  // 전투 분석기 형식 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.NameplateMemory;  // 이름표 모양 기억 가져오기
import LostArkMultiTool.Vision.ScreenLayout;  // 화면 배치 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

// 게임 화면 상시 감시 ("마우스를 올려 두면 읽기")
// - 감시 스레드가 짧은 주기로 게임 창 · 커서를 보고, 커서가 게임 화면 위에서 잠깐 멈추면 그때만 캡처
// - 커서 주변을 한 번 OCR해서 등록된 화면 인식기(전략 패턴)에 넘김 → 툴팁 · 상단바 · 낙원 UI를 같은 방식으로 읽음
// - 커서가 멈춰 있는 동안 확정에 필요한 횟수만 읽고 쉼 (가만히 둬도 계속 OCR하지 않음 → CPU 절약)
// - 같은 결과가 연속으로 나와야 확정 (깜빡임 · 애니메이션 중간 장면 오인식 방지)
// - 현재 캐릭터: 커서와 상관없이 몇 초마다 화면 전체를 읽어 원정대 이름 · 로딩 화면을 찾음 (CharacterDetector)
// - 확정 결과는 잠금으로 보호된 큐에 쌓이고, 화면 스레드가 TakeResults로 가져감 (데이터는 화면 스레드에서만 바꿈 → 데이터 레이스 없음)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 읽은 값 하나 (예: { "dest_top", 12345 })
    struct ScanValue
    {
        std::string m_key;          // 값 이름 (MAT_KEYS · 낙원력 등, 인식기가 정함)
        std::int64_t m_value{};     // 읽은 수
        [[nodiscard]] bool operator==(const ScanValue&) const = default;   // 값 비교
    };

    // 화면 한 장에서 읽은 결과
    struct ScanResult
    {
        std::string m_screenName;                          // 화면 이름 ("storage" 등)
        std::string m_characterName;                       // 화면에서 읽은 캐릭터 이름 (UTF-8, 못 읽으면 빈 글자)
        std::vector<ScanValue> m_values;                   // 읽은 값들
        std::vector<std::string> m_pendingKeys;            // "9999+"처럼 다른 화면(툴팁)에서 다시 읽어야 하는 값 이름
        std::chrono::system_clock::time_point m_capturedAt;// 캡처 시각

        [[nodiscard]] bool HasSameContent(const ScanResult& other) const noexcept;   // 시각을 뺀 내용이 같은지
    };

    // 인식기에 넘기는 공용 도구
    struct DetectorContext
    {
        const ScreenLayoutStore& m_layouts;   // 화면 배치
        const TextRecognizer& m_text;         // 글자 인식기 (감시 스레드에서만 씀)
        const TextResult& m_hoverText;        // 커서 주변을 OCR한 결과 (위치는 m_hoverArea 기준)
        PixelRect m_hoverArea;                // OCR한 커서 주변 영역 (프레임 기준)
        PixelPoint m_cursor;                  // 커서 위치 (프레임 기준)
    };

    // 화면 인식기 (전략 인터페이스: 화면 종류마다 하나씩 구현)
    class ScreenDetector
    {
    public:
        virtual ~ScreenDetector() = default;                                          // 가상 소멸자
        [[nodiscard]] virtual std::string_view GetScreenName() const noexcept = 0;    // 담당 화면 이름
        // 프레임에서 담당 화면을 찾아 읽기 (화면이 안 떠 있거나 못 읽으면 nullopt)
        [[nodiscard]] virtual std::optional<ScanResult> Detect(const FrameImage& frame, const DetectorContext& context) = 0;
    };

    // 연속으로 같은 결과가 나와야 확정하는 도구 (화면 이름별로 따로 셈)
    class ScanConfirmer
    {
    public:
        static constexpr int DEFAULT_REQUIRED_REPEATS{ 2 };                       // 기본: 2프레임 연속

        explicit ScanConfirmer(int requiredRepeats = DEFAULT_REQUIRED_REPEATS);   // 생성자

        // 새 결과 넣기 → 이번에 확정되면 결과 (이미 확정한 것과 같으면 다시 내지 않음)
        [[nodiscard]] std::optional<ScanResult> Feed(const ScanResult& result);
        void Reset(std::string_view screenName);                                  // 화면이 사라졌을 때 연속 횟수 초기화

    private:
        // 화면 하나의 확정 상태
        struct State
        {
            ScanResult m_candidate;                  // 지금 세고 있는 후보
            int m_count{};                           // 연속 횟수
            std::optional<ScanResult> m_confirmed;   // 마지막으로 확정한 결과
        };

        int m_requiredRepeats;                                  // 필요한 연속 횟수
        std::map<std::string, State, std::less<>> m_states;     // 화면별 상태
    };

    // 캐릭터를 다시 확인할 때까지 값 결과를 붙잡아 두는 문
    // - 캐릭터를 바꾸는 사이(로딩 전후 · 확정 전) 읽은 재료 수량이 다른 캐릭터에 들어가지 않게 "캐릭터 먼저, 값은 나중에"
    // - 같은 캐릭터를 다시 보면 내보내고, 로딩 화면 · 다른 이름을 보면 버림
    // - 이름표가 안 보여 확인을 못 해도 캐릭터가 이미 정해져 있으면 MAX_HOLD 뒤에 내보냄
    class CharacterValueGate
    {
    public:
        using Clock = std::chrono::steady_clock;                                   // 시계
        static constexpr std::chrono::milliseconds MAX_HOLD{ 3000 };              // 오래 기다린 값을 내보내는 시간 (캐릭터를 바꾸면 반드시 로딩 화면을 거쳐 버려지므로 짧게)
        static constexpr std::size_t MAX_PENDING{ 32 };                           // 붙잡아 둘 결과 수 상한 (넘으면 오래된 것부터 버림)

        void Hold(ScanResult result, Clock::time_point now);                      // 붙잡기
        [[nodiscard]] std::vector<ScanResult> Release();                          // 같은 캐릭터 확인 → 모두 내보냄
        [[nodiscard]] std::vector<ScanResult> ReleaseExpired(Clock::time_point now);   // MAX_HOLD 넘게 기다린 값만 내보냄
        void Discard() noexcept;                                                   // 로딩 · 다른 캐릭터 → 모두 버림
        [[nodiscard]] bool HasPending() const noexcept;                            // 붙잡은 값이 있는지

    private:
        // 붙잡은 결과 하나
        struct Entry
        {
            ScanResult m_result;            // 결과
            Clock::time_point m_heldAt;     // 붙잡은 시각
        };
        std::deque<Entry> m_entries;        // 붙잡은 결과들 (오래된 것부터)
    };

    // 감시 상태
    enum class WatcherState
    {
        WATCHER_STOPPED,       // 멈춤
        WATCHER_UNSUPPORTED,   // 이 윈도우에서 창 캡처를 못 씀
        WATCHER_NO_GAME,       // 게임 창이 없음
        WATCHER_BACKGROUND,    // 게임이 맨 앞이 아니거나 최소화됨 (쉬는 중)
        WATCHER_WATCHING,      // 감시 중
        WATCHER_ERROR          // 캡처 오류 (다음 주기에 다시 시도)
    };

    // 감시 상태 + 안내
    struct WatcherStatus
    {
        WatcherState m_state{ WatcherState::WATCHER_STOPPED };   // 상태
        std::string m_message;                                   // 안내 문구 (오류 이유 등, UTF-8)
        std::chrono::system_clock::time_point m_lastFrameAt;     // 마지막으로 프레임을 받은 시각
    };

    // 설정 탭 "인식 설정"에 보여 줄 감시 상태 (복사본)
    struct WatcherDiagnostics
    {
        std::shared_ptr<const Core::ImageData> m_preview;   // 줄인 게임 화면 (미리 보기를 켰을 때만, 없으면 nullptr)
        int m_frameWidth{};                                  // 원래 프레임 가로
        int m_frameHeight{};                                 // 원래 프레임 세로
        std::optional<PixelRect> m_nameplate;                // 마지막으로 찾은 내 이름표 (프레임 기준)
        std::optional<PixelRect> m_hoverArea;                // 마지막 커서 주변 읽기 영역 (프레임 기준)
        double m_hoverScanMs{};                              // 커서 주변 읽기 평균 시간 (밀리초)
        double m_characterScanMs{};                          // 캐릭터 확인 평균 시간 (밀리초)
        std::string m_lastRead;                              // 마지막으로 확정한 값 (UTF-8 설명)
        std::chrono::system_clock::time_point m_lastReadAt;  // 그 시각
    };

    // 마지막으로 읽은 보스 체력 (시각 포함 → 깎이는 속도 계산용)
    struct BossHealthSample
    {
        BossHealthReading m_reading;                          // 체력 · 줄 수
        std::chrono::steady_clock::time_point m_capturedAt;   // 읽은 시각
    };

    // 마지막으로 읽은 전투 분석기 (내 피해량 · DPS)
    struct CombatSample
    {
        CombatReading m_reading;                              // 피해량 · DPS
        std::chrono::steady_clock::time_point m_capturedAt;   // 읽은 시각
    };

    // 상시 감시기
    class ScreenWatcher
    {
    public:
        static constexpr std::chrono::milliseconds DEFAULT_INTERVAL{ 200 };    // 기본 감시 주기 (커서 멈춤 확인)

        ScreenWatcher();    // 생성자
        ~ScreenWatcher();   // 소멸자 (감시 스레드를 멈추고 기다림 → RAII)

        ScreenWatcher(const ScreenWatcher&) = delete;              // 복사 금지
        ScreenWatcher& operator=(const ScreenWatcher&) = delete;   // 복사 대입 금지

        void AddDetector(std::unique_ptr<ScreenDetector> detector);   // 인식기 등록 (감시 중이면 무시 → 멈춘 뒤 등록)
        void AddFrameDetector(std::unique_ptr<ScreenDetector> detector);   // 커서와 상관없이 주기적으로 화면 전체를 읽는 인식기 등록 (창고 · 소지품 슬롯)
        void NotifyCharacterPicked() noexcept;                        // 현재 캐릭터를 직접 골랐음 (이름표를 못 봐도 붙잡은 값을 내보낼 수 있게)
        void SetInterval(std::chrono::milliseconds interval) noexcept;// 감시 주기 (다음 주기부터)
        void RequestLayoutReload() noexcept;                          // 교정 후 화면 배치 다시 읽기 요청
        // 현재 캐릭터 인식에 쓸 원정대 이름 · 별칭 · 이름표 모양 기억 (이름이 비면 로딩 화면만 봄)
        void SetCharacterNames(std::vector<std::wstring> names, std::vector<CharacterAlias> aliases = {}, std::shared_ptr<NameplateMemory> memory = nullptr);
        void SetCharacterScanEnabled(bool isEnabled) noexcept;        // 현재 캐릭터 확인 켜기 · 끄기 (끄면 화면 전체 OCR을 하지 않음)
        void SetCharacterInterval(std::chrono::milliseconds interval) noexcept;   // 현재 캐릭터 확인 간격 (같은 캐릭터를 계속 보면 두 배로 늘어남)
        void SetPreviewEnabled(bool isEnabled) noexcept;              // 미리 보기 그림 만들기 켜기 · 끄기 (설정 탭이 보일 때만)
        [[nodiscard]] WatcherDiagnostics GetDiagnostics() const;      // 감시 상태 복사본
        void SetBossScanEnabled(bool isEnabled) noexcept;             // 보스 체력 읽기 켜기 · 끄기
        [[nodiscard]] std::optional<BossHealthSample> GetBossHealth() const;   // 마지막 보스 체력 (보스가 안 보인 지 오래면 nullopt)
        void SetCombatScanEnabled(bool isEnabled) noexcept;           // 전투 분석기 읽기 켜기 · 끄기 (딜 지분 패널이 켜져 있을 때만)
        [[nodiscard]] std::optional<CombatSample> GetCombatStats() const;      // 마지막 전투 분석기 (안 보인 지 오래면 nullopt)

        void Start();                                  // 감시 시작 (이미 감시 중이면 그대로)
        void Stop();                                   // 감시 멈춤 (스레드가 끝날 때까지 기다림)
        [[nodiscard]] bool IsRunning() const noexcept; // 감시 중인지

        [[nodiscard]] std::vector<ScanResult> TakeResults();   // 확정된 결과 가져가기 (가져간 것은 큐에서 빠짐)
        [[nodiscard]] WatcherStatus GetStatus() const;         // 지금 상태

        // 커서 주변 OCR 영역 (툴팁이 커서 왼쪽 · 오른쪽 · 위 어디에 떠도 들어오게, 프레임 안으로 자름)
        [[nodiscard]] static PixelRect GetHoverArea(PixelPoint cursor, int frameWidth, int frameHeight) noexcept;

        // 게임 화면 한 장 캡처 (교정 · 샘플 저장용, 별도 스레드에서 최대 timeout 동안 기다림, 실패하면 nullopt + error)
        [[nodiscard]] static std::optional<FrameImage> CaptureOnce(std::chrono::milliseconds timeout, std::string& error);

    private:
        void Run(std::stop_token stopToken);              // 감시 스레드 본문
        void SetStatus(WatcherState state, std::string message);   // 상태 바꾸기 (잠금 안에서)

        mutable std::mutex m_mutex;                                    // 큐 · 상태 보호
        std::condition_variable_any m_wakeUp;                          // 주기 대기 · 멈춤 알림
        std::vector<std::unique_ptr<ScreenDetector>> m_detectors;      // 인식기들 (감시 중에는 감시 스레드만 씀)
        std::vector<std::unique_ptr<ScreenDetector>> m_frameDetectors; // 주기 인식기들 (감시 중에는 감시 스레드만 씀)
        std::atomic<bool> m_isCharacterPicked{};                       // 직접 고른 캐릭터가 있음 (로딩 화면을 보면 지움)
        std::vector<ScanResult> m_results;                             // 확정 결과 큐
        WatcherStatus m_status;                                        // 지금 상태
        std::atomic<std::int64_t> m_intervalMs{ DEFAULT_INTERVAL.count() };   // 감시 주기 (밀리초)
        std::atomic<bool> m_isReloadRequested{};                       // 배치 다시 읽기 요청
        std::atomic<bool> m_isCharacterScanEnabled{ true };            // 현재 캐릭터 확인 켜짐
        std::atomic<std::int64_t> m_characterIntervalMs{ 2000 };      // 현재 캐릭터 확인 간격 (밀리초)
        std::atomic<bool> m_isPreviewEnabled{};                        // 미리 보기 그림 만들기
        std::atomic<bool> m_isBossScanEnabled{ true };                 // 보스 체력 읽기
        std::optional<BossHealthSample> m_bossHealth;                  // 마지막 보스 체력 (m_mutex로 보호)
        std::atomic<bool> m_isCombatScanEnabled{};                     // 전투 분석기 읽기
        std::optional<CombatSample> m_combatStats;                     // 마지막 전투 분석기 (m_mutex로 보호)
        WatcherDiagnostics m_diagnostics;                              // 감시 상태 (m_mutex로 보호)
        std::vector<std::wstring> m_characterNames;                    // 원정대 이름 (m_mutex로 보호)
        std::vector<CharacterAlias> m_characterAliases;                // 별칭 (m_mutex로 보호)
        std::shared_ptr<NameplateMemory> m_nameplateMemory;            // 이름표 모양 기억 (m_mutex로 보호, 기억 자체는 스스로 잠금)
        std::uint64_t m_characterNamesVersion{};                       // 이름이 바뀐 횟수 (감시 스레드가 새로 받을 때 비교, m_mutex로 보호)
        std::atomic<bool> m_isRunning{};                               // 감시 중인지
        std::jthread m_thread;                                         // 감시 스레드 (소멸 때 멈춤 요청 + 기다림)
    };
}
