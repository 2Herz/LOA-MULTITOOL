module LostArkMultiTool.Vision.ScreenWatcher;  // 화면 상시 감시 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.GameWindowFinder;  // 게임 창 찾기 가져오기
import LostArkMultiTool.Vision.ScreenCaptureService;  // 화면 캡처 가져오기
import LostArkMultiTool.Vision.ScreenLayout;  // 화면 배치 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::chrono::milliseconds MIN_INTERVAL{ 200 };          // 감시 주기 하한 (CPU 보호)
        constexpr std::chrono::milliseconds CAPTURE_POLL_INTERVAL{ 50 };  // 한 장 캡처 때 프레임 확인 간격
        constexpr int MIN_REQUIRED_REPEATS{ 1 };                          // 확정 연속 횟수 하한
        constexpr std::string_view NO_GAME_TEXT{ "로스트아크 창을 찾지 못했어요" };                 // 게임 없음 안내
        constexpr std::string_view BACKGROUND_TEXT{ "게임이 맨 앞에 있을 때만 읽어요" };              // 쉬는 중 안내
        constexpr std::string_view WATCHING_TEXT{ "게임 화면을 보고 있어요" };                        // 감시 중 안내
        constexpr std::string_view UNSUPPORTED_TEXT{ "이 윈도우는 창 캡처를 지원하지 않아요 (윈도우 10 1903 이상 필요)" };   // 미지원 안내
        constexpr std::string_view NO_FRAME_TEXT{ "게임 화면을 받지 못했어요 (최소화돼 있거나 화면이 멈춰 있어요)" };          // 한 장 캡처 실패 안내
    }

    // ===== ScanResult =====

    bool ScanResult::HasSameContent(const ScanResult& other) const noexcept
    {
        return m_screenName == other.m_screenName            // 같은 화면
            && m_characterName == other.m_characterName      // 같은 캐릭터
            && m_values == other.m_values                    // 같은 값
            && m_pendingKeys == other.m_pendingKeys;         // 같은 대기 목록
    }

    // ===== ScanConfirmer =====

    ScanConfirmer::ScanConfirmer(int requiredRepeats)
        : m_requiredRepeats{ std::max(requiredRepeats, MIN_REQUIRED_REPEATS) }   // 1 미만은 1
    {
    }

    std::optional<ScanResult> ScanConfirmer::Feed(const ScanResult& result)
    {
        State& state = m_states[result.m_screenName];                         // 화면별 상태 (없으면 만듦)
        if (state.m_count > 0 && state.m_candidate.HasSameContent(result)) ++state.m_count;   // 같은 결과면 횟수 증가
        else                                                                   // 다르면
        {
            state.m_candidate = result;                                        // 새 후보
            state.m_count = 1;                                                 // 처음부터 셈
        }
        if (state.m_count < m_requiredRepeats) return std::nullopt;            // 아직 모자람
        if (state.m_confirmed && state.m_confirmed->HasSameContent(result)) return std::nullopt;   // 이미 낸 결과
        state.m_confirmed = result;                                            // 확정 기록
        return result;                                                         // 이번에 확정
    }

    void ScanConfirmer::Reset(std::string_view screenName)
    {
        const auto found = m_states.find(screenName);   // 화면 상태 찾기
        if (found != m_states.end()) found->second.m_count = 0;   // 연속 횟수만 초기화 (마지막 확정은 유지 → 같은 화면을 다시 열어도 중복 반영 안 함)
    }

    // ===== ScreenWatcher =====

    ScreenWatcher::ScreenWatcher() = default;   // 기본 생성

    ScreenWatcher::~ScreenWatcher()
    {
        Stop();   // 감시 스레드 정리 (RAII)
    }

    void ScreenWatcher::AddDetector(std::unique_ptr<ScreenDetector> detector)
    {
        if (!detector || m_isRunning) return;             // 빈 인식기 · 감시 중에는 무시 (스레드와 공유하지 않음)
        m_detectors.push_back(std::move(detector));       // 등록
    }

    void ScreenWatcher::SetInterval(std::chrono::milliseconds interval) noexcept
    {
        m_intervalMs = std::max(interval, MIN_INTERVAL).count();   // 하한 적용 후 저장
    }

    void ScreenWatcher::RequestLayoutReload() noexcept
    {
        m_isReloadRequested = true;   // 다음 주기에 다시 읽음
    }

    void ScreenWatcher::Start()
    {
        if (m_isRunning) return;                                                      // 이미 감시 중
        if (m_thread.joinable()) m_thread.join();                                     // 끝난 예전 스레드 정리
        m_isRunning = true;                                                           // 감시 중 표시
        m_thread = std::jthread{ [this](std::stop_token stopToken) { Run(stopToken); } };   // 감시 스레드 시작
    }

    void ScreenWatcher::Stop()
    {
        if (!m_thread.joinable()) return;   // 스레드 없음
        m_thread.request_stop();            // 멈춤 요청 (대기 중이면 바로 깨어남)
        m_thread.join();                    // 끝날 때까지 기다림
        m_isRunning = false;                // 멈춤 표시
        SetStatus(WatcherState::WATCHER_STOPPED, {});   // 상태 갱신
    }

    bool ScreenWatcher::IsRunning() const noexcept
    {
        return m_isRunning;   // 감시 중인지
    }

    std::vector<ScanResult> ScreenWatcher::TakeResults()
    {
        const std::scoped_lock lock{ m_mutex };   // 큐 보호
        return std::exchange(m_results, {});      // 통째로 가져가고 비움
    }

    WatcherStatus ScreenWatcher::GetStatus() const
    {
        const std::scoped_lock lock{ m_mutex };   // 상태 보호
        return m_status;                          // 복사본
    }

    void ScreenWatcher::SetStatus(WatcherState state, std::string message)
    {
        const std::scoped_lock lock{ m_mutex };   // 상태 보호
        m_status.m_state = state;                 // 상태
        m_status.m_message = std::move(message);  // 안내
    }

    void ScreenWatcher::Run(std::stop_token stopToken)
    {
        const VisionApartment apartment;                                           // 이 스레드의 WinRT 준비
        if (!ScreenCaptureService::IsSupported())                                  // 창 캡처를 못 쓰면
        {
            SetStatus(WatcherState::WATCHER_UNSUPPORTED, std::string{ UNSUPPORTED_TEXT });   // 안내
            m_isRunning = false;                                                   // 끝
            return;                                                                // 스레드 종료
        }

        auto layouts = std::make_unique<ScreenLayoutStore>();                      // 화면 배치 (이 스레드 전용)
        const TextRecognizer text;                                                 // 글자 인식기 (이 스레드 전용)
        ScreenCaptureService capture;                                              // 캡처 서비스 (이 스레드 전용)
        ScanConfirmer confirmer;                                                   // 연속 확정 도구
        void* gameWindow = nullptr;                                                // 지난번에 찾은 게임 창

        while (!stopToken.stop_requested())                                        // 멈춤 요청 전까지
        {
            if (m_isReloadRequested.exchange(false)) layouts = std::make_unique<ScreenLayoutStore>();   // 교정 후 배치 다시 읽기

            std::optional<GameWindowInfo> info = gameWindow ? GameWindowFinder::Refresh(gameWindow) : std::nullopt;   // 지난 창 확인
            if (!info) info = GameWindowFinder::Find();                            // 없으면 새로 찾기

            if (!info)                                                             // 게임이 없음
            {
                gameWindow = nullptr;                                              // 기록 지움
                capture.Stop();                                                    // 캡처 정리
                SetStatus(WatcherState::WATCHER_NO_GAME, std::string{ NO_GAME_TEXT });   // 안내
            }
            else if (!info->m_isForeground || info->m_isMinimized)                 // 게임이 뒤에 있거나 최소화
            {
                gameWindow = info->m_handle;                                       // 창 기록
                SetStatus(WatcherState::WATCHER_BACKGROUND, std::string{ BACKGROUND_TEXT });   // 쉬는 중
            }
            else if (gameWindow = info->m_handle; !capture.Start(gameWindow))      // 캡처 시작 실패
            {
                SetStatus(WatcherState::WATCHER_ERROR, capture.GetLastError());    // 오류 안내 (다음 주기에 다시)
            }
            else if (const auto frame = capture.TakeLatestFrame())                 // 새 프레임이 있으면
            {
                const auto now = std::chrono::system_clock::now();                 // 캡처 시각
                {
                    const std::scoped_lock lock{ m_mutex };                        // 상태 보호
                    m_status.m_state = WatcherState::WATCHER_WATCHING;             // 감시 중
                    m_status.m_message = std::string{ WATCHING_TEXT };             // 안내
                    m_status.m_lastFrameAt = now;                                  // 마지막 프레임 시각
                }
                const DetectorContext context{ *layouts, text };                   // 인식기 공용 도구
                for (const auto& detector : m_detectors)                           // 인식기마다
                {
                    try
                    {
                        auto result = detector->Detect(*frame, context);           // 담당 화면 읽기
                        if (!result)                                               // 화면이 없으면
                        {
                            confirmer.Reset(detector->GetScreenName());            // 연속 횟수 초기화
                            continue;                                              // 다음 인식기
                        }
                        result->m_screenName = std::string{ detector->GetScreenName() };   // 화면 이름 보장
                        result->m_capturedAt = now;                                // 시각 기록
                        if (auto confirmed = confirmer.Feed(*result))              // 확정되면
                        {
                            const std::scoped_lock lock{ m_mutex };                // 큐 보호
                            m_results.push_back(std::move(*confirmed));            // 큐에 추가
                        }
                    }
                    catch (const std::exception& error)
                    {
                        SetStatus(WatcherState::WATCHER_ERROR, error.what());      // 인식기 오류 안내 (다른 인식기는 계속)
                    }
                }
            }

            std::unique_lock lock{ m_mutex };                                      // 대기용 잠금
            m_wakeUp.wait_for(lock, stopToken, std::chrono::milliseconds{ m_intervalMs.load() },
                [] { return false; });                                             // 주기만큼 쉬기 (멈춤 요청이면 바로 깸)
        }
        m_isRunning = false;   // 감시 끝
    }

    std::optional<FrameImage> ScreenWatcher::CaptureOnce(std::chrono::milliseconds timeout, std::string& error)
    {
        std::optional<FrameImage> result;   // 결과
        std::thread worker{ [&]                                                    // WinRT 아파트를 따로 쓰는 일꾼 스레드
            {
                const VisionApartment apartment;                                   // 이 스레드의 WinRT 준비
                const auto info = GameWindowFinder::Find();                        // 게임 창 찾기
                if (!info) { error = NO_GAME_TEXT; return; }                       // 게임 없음
                ScreenCaptureService capture;                                      // 캡처 서비스
                if (!capture.Start(info->m_handle)) { error = capture.GetLastError(); return; }   // 시작 실패
                const auto deadline = std::chrono::steady_clock::now() + timeout;  // 기다릴 한계
                while (std::chrono::steady_clock::now() < deadline)                // 한계까지
                {
                    if (auto frame = capture.TakeLatestFrame()) { result = std::move(frame); return; }   // 프레임을 받으면 끝
                    std::this_thread::sleep_for(CAPTURE_POLL_INTERVAL);            // 잠깐 쉬고 다시
                }
                error = NO_FRAME_TEXT;                                             // 시간 초과
            } };
        worker.join();   // 일꾼이 끝날 때까지 기다림 (참조 캡처가 안전하도록)
        return result;   // 프레임 (실패하면 nullopt)
    }
}
