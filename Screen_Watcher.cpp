module LostArkMultiTool.Vision.ScreenWatcher;  // 화면 상시 감시 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.TextEncoding;  // 이름 UTF-8 바꾸기 가져오기
import LostArkMultiTool.Vision.CharacterDetector;  // 현재 캐릭터 인식기 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기
import LostArkMultiTool.Vision.GameWindowFinder;  // 게임 창 찾기 가져오기
import LostArkMultiTool.Vision.ScreenCaptureService;  // 화면 캡처 가져오기
import LostArkMultiTool.Vision.ScreenLayout;  // 화면 배치 가져오기
import LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr std::chrono::milliseconds MIN_INTERVAL{ 100 };          // 감시 주기 하한 (CPU 보호)
        constexpr std::chrono::milliseconds CAPTURE_POLL_INTERVAL{ 50 };  // 한 장 캡처 때 프레임 확인 간격
        constexpr std::chrono::milliseconds REST_BEFORE_SCAN{ 500 };      // 커서가 이만큼 멈춰야 읽기 시작
        constexpr int MIN_REQUIRED_REPEATS{ 1 };                          // 확정 연속 횟수 하한
        constexpr int SCANS_PER_REST{ ScanConfirmer::DEFAULT_REQUIRED_REPEATS };   // 한 번 멈출 때 읽는 횟수 (확정에 필요한 만큼)
        constexpr int MOVE_TOLERANCE{ 4 };                                // 이만큼(픽셀) 안에서 흔들리면 멈춘 것으로 봄
        constexpr int HOVER_HALF_WIDTH{ 700 };                            // 커서 주변 OCR 영역 가로 반폭 (툴팁 너비 + 여유)
        constexpr int HOVER_UP{ 500 };                                    // 커서 위로 볼 높이 (위로 뜨는 툴팁 · 상단바)
        constexpr int HOVER_DOWN{ 450 };                                  // 커서 아래로 볼 높이
        constexpr std::chrono::milliseconds CHARACTER_SCAN_INTERVAL{ 2000 };   // 현재 캐릭터 확인 간격 (화면 전체 OCR이라 가끔만)
        constexpr std::chrono::milliseconds CHARACTER_STEADY_INTERVAL{ 4000 }; // 같은 캐릭터를 계속 확인하는 중이면 더 드물게 (CPU 절약)
        constexpr std::chrono::milliseconds CHARACTER_LOADING_INTERVAL{ 1000 };// 로딩 화면이면 더 자주 (새 캐릭터를 빨리 알아봄)
        constexpr int OCR_SCALE{ 1 };                                     // OCR 확대 배율 (1080p 글자는 원본 크기가 가장 잘 읽힘)
        constexpr std::string_view NO_GAME_TEXT{ "로스트아크 창을 찾지 못했어요" };                 // 게임 없음 안내
        constexpr std::string_view BACKGROUND_TEXT{ "게임이 맨 앞에 있을 때만 읽어요" };              // 쉬는 중 안내
        constexpr std::string_view WATCHING_TEXT{ "읽고 싶은 숫자 · 재료에 마우스를 올려 두세요" };   // 감시 중 안내
        constexpr std::string_view UNSUPPORTED_TEXT{ "이 윈도우는 창 캡처를 지원하지 않아요 (윈도우 10 1903 이상 필요)" };   // 미지원 안내
        constexpr std::string_view NO_OCR_TEXT{ "윈도우 한국어 OCR이 없어요 (설정 → 시간 및 언어 → 언어에서 한국어 설치)" };   // OCR 없음 안내
        constexpr std::string_view NO_FRAME_TEXT{ "게임 화면을 받지 못했어요 (최소화돼 있거나 화면이 멈춰 있어요)" };          // 한 장 캡처 실패 안내

        // 두 점이 흔들림 범위 안인지
        [[nodiscard]] bool IsNear(PixelPoint a, PixelPoint b) noexcept
        {
            return std::abs(a.m_x - b.m_x) <= MOVE_TOLERANCE && std::abs(a.m_y - b.m_y) <= MOVE_TOLERANCE;   // 가로 · 세로 모두 가까움
        }
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

    void ScreenWatcher::SetCharacterNames(std::vector<std::wstring> names, std::vector<CharacterAlias> aliases, std::shared_ptr<NameplateMemory> memory)
    {
        const std::scoped_lock lock{ m_mutex };       // 감시 스레드와 공유
        m_characterNames = std::move(names);          // 새 이름들
        m_characterAliases = std::move(aliases);      // 새 별칭들
        m_nameplateMemory = std::move(memory);        // 이름표 모양 기억
        ++m_characterNamesVersion;                    // 바뀜 표시 (감시 스레드가 다음 확인 때 새로 받음)
    }

    void ScreenWatcher::SetCharacterScanEnabled(bool isEnabled) noexcept
    {
        m_isCharacterScanEnabled = isEnabled;   // 다음 주기부터
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

    PixelRect ScreenWatcher::GetHoverArea(PixelPoint cursor, int frameWidth, int frameHeight) noexcept
    {
        const PixelRect area{ cursor.m_x - HOVER_HALF_WIDTH, cursor.m_y - HOVER_UP,
            HOVER_HALF_WIDTH * 2, HOVER_UP + HOVER_DOWN };          // 커서 주변
        return ClampRect(area, frameWidth, frameHeight);            // 프레임 안으로
    }

    void ScreenWatcher::Run(std::stop_token stopToken)
    {
        const VisionApartment apartment;                                           // 이 스레드의 WinRT · DPI 준비
        if (!ScreenCaptureService::IsSupported())                                  // 창 캡처를 못 쓰면
        {
            SetStatus(WatcherState::WATCHER_UNSUPPORTED, std::string{ UNSUPPORTED_TEXT });   // 안내
            m_isRunning = false;                                                   // 끝
            return;                                                                // 스레드 종료
        }

        auto layouts = std::make_unique<ScreenLayoutStore>();                      // 화면 배치 (이 스레드 전용)
        const TextRecognizer text;                                                 // 글자 인식기 (이 스레드 전용)
        if (!text.IsAvailable())                                                   // OCR을 못 쓰면
        {
            SetStatus(WatcherState::WATCHER_UNSUPPORTED, std::string{ NO_OCR_TEXT });   // 안내
            m_isRunning = false;                                                   // 끝
            return;                                                                // 스레드 종료
        }
        ScreenCaptureService capture;                                              // 캡처 서비스 (이 스레드 전용)
        ScanConfirmer confirmer;                                                   // 연속 확정 도구
        ScanConfirmer characterConfirmer;                                          // 현재 캐릭터 확정 도구 (로딩 화면이면 새로 만듦)
        std::optional<CharacterDetector> characterDetector;                        // 현재 캐릭터 인식기 (이름을 받으면 만듦)
        std::uint64_t characterNamesVersion = 0;                                   // 받은 이름 버전
        auto nextCharacterScan = std::chrono::steady_clock::now();                 // 다음 캐릭터 확인 시각
        bool wasLoading = false;                                                   // 직전 확인이 로딩 화면이었는지 (로딩 알림은 한 번만)
        std::string lastCharacterName;                                             // 마지막으로 확정한 캐릭터 (같으면 확인 간격을 늘림)
        void* gameWindow = nullptr;                                                // 지난번에 찾은 게임 창
        std::optional<PixelPoint> lastCursor;                                      // 지난 주기 커서 (프레임 기준)
        std::chrono::milliseconds restTime{};                                      // 커서가 멈춘 시간
        int scansThisRest = 0;                                                     // 이번 멈춤에서 읽은 횟수

        while (!stopToken.stop_requested())                                        // 멈춤 요청 전까지
        {
            const std::chrono::milliseconds interval{ m_intervalMs.load() };       // 이번 주기
            if (m_isReloadRequested.exchange(false)) layouts = std::make_unique<ScreenLayoutStore>();   // 교정 후 배치 다시 읽기

            std::optional<GameWindowInfo> info = gameWindow ? GameWindowFinder::Refresh(gameWindow) : std::nullopt;   // 지난 창 확인
            if (!info) info = GameWindowFinder::Find();                            // 없으면 새로 찾기

            if (!info)                                                             // 게임이 없음
            {
                gameWindow = nullptr;                                              // 기록 지움
                capture.Stop();                                                    // 캡처 정리
                lastCursor.reset();                                                // 커서 기록 지움
                SetStatus(WatcherState::WATCHER_NO_GAME, std::string{ NO_GAME_TEXT });   // 안내
            }
            else if (!info->m_isForeground || info->m_isMinimized)                 // 게임이 뒤에 있거나 최소화
            {
                gameWindow = info->m_handle;                                       // 창 기록
                capture.Stop();                                                    // 쉬는 동안 캡처도 멈춤 (자원 절약)
                lastCursor.reset();                                                // 커서 기록 지움
                SetStatus(WatcherState::WATCHER_BACKGROUND, std::string{ BACKGROUND_TEXT });   // 쉬는 중
            }
            else if (gameWindow = info->m_handle; !capture.Start(gameWindow))      // 캡처 시작 실패
            {
                SetStatus(WatcherState::WATCHER_ERROR, capture.GetLastError());    // 오류 안내 (다음 주기에 다시)
            }
            else                                                                   // 감시 중
            {
                SetStatus(WatcherState::WATCHER_WATCHING, std::string{ WATCHING_TEXT });   // 안내

                // 커서가 게임 화면 위에서 멈췄는지 확인
                const auto screenCursor = GameWindowFinder::GetCursorPosition();  // 화면 좌표 커서
                std::optional<PixelPoint> cursor;                                  // 게임 화면 기준 커서
                const PixelRect& client = info->m_clientArea;                      // 게임 화면 영역 (화면 좌표)
                if (screenCursor && screenCursor->m_x >= client.m_x && screenCursor->m_x < client.GetRight()
                    && screenCursor->m_y >= client.m_y && screenCursor->m_y < client.GetBottom())   // 게임 화면 안이면
                    cursor = PixelPoint{ screenCursor->m_x - client.m_x, screenCursor->m_y - client.m_y };   // 게임 화면 기준으로

                if (!cursor || !lastCursor || !IsNear(*cursor, *lastCursor))       // 화면 밖이거나 움직였으면
                {
                    restTime = {};                                                 // 멈춤 시간 초기화
                    scansThisRest = 0;                                             // 읽은 횟수 초기화
                    lastCursor = cursor;                                           // 새 위치 기록
                }
                else restTime += interval;                                         // 멈춘 시간 누적

                auto frame = capture.TakeLatestFrame();                            // 최신 프레임 (쌓인 프레임 비우기 겸)
                if (frame)                                                         // 받았으면
                {
                    const std::scoped_lock lock{ m_mutex };                        // 상태 보호
                    m_status.m_lastFrameAt = std::chrono::system_clock::now();     // 마지막 프레임 시각
                }

                const auto steadyNow = std::chrono::steady_clock::now();           // 지금 (캐릭터 확인 주기용)
                if (frame && m_isCharacterScanEnabled && steadyNow >= nextCharacterScan)   // 캐릭터 확인할 때 (커서와 상관없이)
                {
                    nextCharacterScan = steadyNow + CHARACTER_SCAN_INTERVAL;       // 다음 확인 시각
                    {
                        const std::scoped_lock lock{ m_mutex };                    // 이름 보호
                        if (!characterDetector || characterNamesVersion != m_characterNamesVersion)   // 이름이 바뀌었으면
                        {
                            characterDetector.emplace(m_characterNames, m_characterAliases, m_nameplateMemory);   // 인식기 다시 만들기 (이름 · 별칭 복사, 기억은 공유)
                            characterNamesVersion = m_characterNamesVersion;       // 버전 기록
                        }
                    }
                    const CharacterReading reading = characterDetector->Detect(*frame, text);   // 화면 전체 읽기
                    ScanResult characterResult;                                    // 결과
                    characterResult.m_capturedAt = std::chrono::system_clock::now();   // 시각
                    if (reading.m_signal == CharacterSignal::SIGNAL_LOADING)       // 로딩 화면 → 캐릭터가 바뀌었을 수 있음
                    {
                        nextCharacterScan = steadyNow + CHARACTER_LOADING_INTERVAL;// 로딩 중엔 자주 확인
                        characterConfirmer = ScanConfirmer{};                      // 확정 기록 지움 (로딩 뒤 같은 캐릭터도 다시 확정)
                        lastCharacterName.clear();                                 // 확정 이름도 지움
                        if (!wasLoading)                                           // 이번 로딩에서 처음 봄
                        {
                            characterResult.m_screenName = std::string{ CharacterDetector::LOADING_SCREEN_NAME };   // 로딩 알림
                            const std::scoped_lock lock{ m_mutex };                // 큐 보호
                            m_results.push_back(std::move(characterResult));       // 바로 알림 (확정 기다리지 않음)
                        }
                        wasLoading = true;                                         // 기록
                    }
                    else if (reading.m_signal == CharacterSignal::SIGNAL_NAME)     // 원정대 이름을 찾음
                    {
                        wasLoading = false;                                        // 로딩 끝
                        characterResult.m_screenName = std::string{ CharacterDetector::SCREEN_NAME };   // 캐릭터
                        characterResult.m_characterName = Core::TextEncoding::WideToUTF8(reading.m_name); // 이름 (UTF-8)
                        characterResult.m_values.push_back(ScanValue{ std::string{ CharacterDetector::SOURCE_VALUE_KEY }, static_cast<std::int64_t>(reading.m_source) });   // 찾은 곳
                        if (reading.m_itemLevel) characterResult.m_values.push_back(ScanValue{ std::string{ CharacterDetector::ITEM_LEVEL_VALUE_KEY },
                            std::llround(*reading.m_itemLevel * CharacterDetector::ITEM_LEVEL_SCALE) });   // 정보 창 아이템 레벨 (× 100)
                        const bool isSameAsConfirmed = lastCharacterName == characterResult.m_characterName;   // 확정한 캐릭터를 다시 봄
                        if (auto confirmed = characterConfirmer.Feed(characterResult))   // 두 번 연속 같으면 확정
                        {
                            lastCharacterName = confirmed->m_characterName;        // 확정 이름 기록
                            const std::scoped_lock lock{ m_mutex };                // 큐 보호
                            m_results.push_back(std::move(*confirmed));            // 큐에 추가
                        }
                        if (isSameAsConfirmed) nextCharacterScan = steadyNow + CHARACTER_STEADY_INTERVAL;   // 같은 캐릭터면 천천히
                    }
                    else                                                           // 못 찾음 → 연속 횟수만 초기화 (지금 캐릭터는 유지)
                    {
                        characterConfirmer.Reset(CharacterDetector::SCREEN_NAME);  // 캐릭터 연속 횟수 초기화
                        if (!reading.m_unknownName.empty() || reading.m_hasUnknownPlate)   // 이름을 못 맞춤 → 직접 고를 때 별칭 · 모양으로 쓰도록 알림
                        {
                            characterResult.m_screenName = std::string{ CharacterDetector::UNKNOWN_SCREEN_NAME };   // 못 맞춘 이름
                            characterResult.m_characterName = Core::TextEncoding::WideToUTF8(reading.m_unknownName);   // 읽은 글자
                            if (auto confirmed = characterConfirmer.Feed(characterResult))   // 두 번 연속 같으면
                            {
                                const std::scoped_lock lock{ m_mutex };            // 큐 보호
                                m_results.push_back(std::move(*confirmed));        // 큐에 추가
                            }
                        }
                    }
                }

                const bool isReadyToScan = cursor && restTime >= REST_BEFORE_SCAN && scansThisRest < SCANS_PER_REST;   // 읽을 때인지
                if (isReadyToScan && frame)                                        // 멈춘 커서 + 프레임이 있으면
                {
                    ++scansThisRest;                                               // 읽은 횟수
                    const PixelRect hoverArea = GetHoverArea(*cursor, frame->GetWidth(), frame->GetHeight());   // 커서 주변
                    const auto hoverText = text.Recognize(frame->Crop(hoverArea), OCR_SCALE);   // 커서 주변 OCR
                    const TextResult emptyText;                                    // OCR 실패 때 넘길 빈 결과
                    const DetectorContext context{ *layouts, text, hoverText ? *hoverText : emptyText, hoverArea, *cursor };   // 인식기 공용 도구
                    const auto now = std::chrono::system_clock::now();             // 읽은 시각
                    for (const auto& detector : m_detectors)                       // 인식기마다
                    {
                        try
                        {
                            auto result = detector->Detect(*frame, context);       // 담당 화면 읽기
                            if (!result)                                           // 화면이 없으면
                            {
                                confirmer.Reset(detector->GetScreenName());        // 연속 횟수 초기화
                                continue;                                          // 다음 인식기
                            }
                            result->m_screenName = std::string{ detector->GetScreenName() };   // 화면 이름 보장
                            result->m_capturedAt = now;                            // 시각 기록
                            if (auto confirmed = confirmer.Feed(*result))          // 확정되면
                            {
                                const std::scoped_lock lock{ m_mutex };            // 큐 보호
                                m_results.push_back(std::move(*confirmed));        // 큐에 추가
                            }
                        }
                        catch (const std::exception& error)
                        {
                            SetStatus(WatcherState::WATCHER_ERROR, error.what());  // 인식기 오류 안내 (다른 인식기는 계속)
                        }
                    }
                }
            }

            std::unique_lock lock{ m_mutex };                                      // 대기용 잠금
            m_wakeUp.wait_for(lock, stopToken, interval, [] { return false; });    // 주기만큼 쉬기 (멈춤 요청이면 바로 깸)
        }
        m_isRunning = false;   // 감시 끝
    }

    std::optional<FrameImage> ScreenWatcher::CaptureOnce(std::chrono::milliseconds timeout, std::string& error)
    {
        std::optional<FrameImage> result;   // 결과
        std::thread worker{ [&]                                                    // WinRT 아파트를 따로 쓰는 일꾼 스레드
            {
                const VisionApartment apartment;                                   // 이 스레드의 WinRT · DPI 준비
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
