module LostArkMultiTool.Vision.ScreenWatcher;  // 화면 상시 감시 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.TextEncoding;  // 이름 UTF-8 바꾸기 가져오기
import LostArkMultiTool.Vision.BossHealthDetector;  // 보스 체력바 인식기 가져오기
import LostArkMultiTool.Vision.BossTimerDetector;  // 보스 남은 시간 인식기 가져오기
import LostArkMultiTool.Vision.BuffDetector;  // 버프 타이머 인식기 가져오기
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
        constexpr std::chrono::milliseconds MIN_CHARACTER_INTERVAL{ 500 };    // 캐릭터 확인 간격 하한 (CPU 보호)
        constexpr int STEADY_INTERVAL_FACTOR{ 2 };                            // 같은 캐릭터를 계속 확인하는 중이면 간격 × 2 (CPU 절약)
        constexpr std::chrono::milliseconds CHARACTER_LOADING_INTERVAL{ 1000 };// 로딩 화면이면 더 자주 (새 캐릭터를 빨리 알아봄)
        constexpr std::chrono::milliseconds PREVIEW_INTERVAL{ 1000 };         // 미리 보기 그림 갱신 간격
        constexpr std::chrono::milliseconds FULL_SCAN_INTERVAL{ 6000 };       // 화면 전체 OCR(정보 창 찾기) 간격 (1초 안팎으로 무거워서 가끔만)
        constexpr std::chrono::milliseconds BOSS_SCAN_INTERVAL{ 500 };        // 보스 체력 읽기 간격 (작은 영역 · 흑백이라 가벼움)
        constexpr std::chrono::milliseconds BOSS_KEEP_DURATION{ 3000 };       // 보스를 못 읽어도 이 시간까지는 마지막 값 유지 (잠깐 가려짐 · 오인식)
        constexpr std::chrono::milliseconds BOSS_TIMER_INTERVAL{ 1000 };      // 광폭화 · 토벌 타이머 읽기 간격 (1초마다 바뀜)
        constexpr std::chrono::milliseconds BOSS_TIMER_KEEP{ 5000 };          // 타이머를 못 읽어도 이 시간까지는 마지막 값에서 줄여 씀
        constexpr std::chrono::milliseconds BOSS_NAME_INTERVAL{ 3000 };       // 타이머가 안 보일 때 보스 이름(광폭화 표) 다시 읽는 간격
        constexpr std::chrono::milliseconds COMBAT_SCAN_INTERVAL{ 1000 };     // 전투 분석기 읽기 간격 (게임도 1초마다 갱신)
        constexpr std::chrono::milliseconds COMBAT_KEEP_DURATION{ 5000 };     // 전투 분석기를 못 읽어도 이 시간까지는 마지막 값 유지
        constexpr std::chrono::milliseconds BUFF_SCAN_INTERVAL{ 250 };        // 버프 · 디버프 읽기 간격 (픽셀 계산만이라 가벼움, 남은 시간이 1초씩 줄어서 자주)
        constexpr std::chrono::milliseconds BUFF_KEEP_DURATION{ 1500 };       // 이 시간 넘게 못 읽었으면 지난 값을 버림 (게임이 뒤로 감 등)
        constexpr std::chrono::milliseconds FRAME_SCAN_INTERVAL{ 1000 };      // 주기 인식기(창고 · 소지품 슬롯) 간격 (Release 1080p 한 장 약 55ms)
        constexpr std::chrono::milliseconds GATE_CHECK_DELAY{ 300 };          // 값을 붙잡으면 이만큼 뒤에 캐릭터를 다시 확인 (빨리 내보내려고)
        constexpr int PREVIEW_MAX_WIDTH{ 640 };                               // 미리 보기 그림 가로 상한 (줄여서 메모리 · 시간 절약)
        constexpr double AVERAGE_WEIGHT{ 0.3 };                               // 처리 시간 평균: 새 값 비중 (지수 이동 평균)
        constexpr double MILLISECONDS_PER_SECOND{ 1000.0 };                   // 초 → 밀리초

        // 프레임을 가로 PREVIEW_MAX_WIDTH 이하로 줄인 그림 (가까운 픽셀 고르기, 알파는 불투명)
        [[nodiscard]] std::shared_ptr<const Core::ImageData> MakePreview(const FrameImage& frame, std::uint64_t version)
        {
            const int sourceWidth = frame.GetWidth();                          // 원래 가로
            const int sourceHeight = frame.GetHeight();                        // 원래 세로
            if (sourceWidth <= 0 || sourceHeight <= 0) return nullptr;         // 빈 프레임
            const int width = std::min(sourceWidth, PREVIEW_MAX_WIDTH);        // 줄인 가로
            const int height = std::max(1, static_cast<int>(static_cast<long long>(sourceHeight) * width / sourceWidth));   // 비율 유지
            auto image = std::make_shared<Core::ImageData>();                   // 결과
            image->m_width = width;                                            // 가로
            image->m_height = height;                                          // 세로
            image->m_version = version;                                        // 버전 (렌더러 캐시용)
            image->m_pixels.resize(static_cast<std::size_t>(width) * height * FrameImage::BYTES_PER_PIXEL);   // 자리
            const auto source = frame.GetPixels();                             // 원래 픽셀
            constexpr int ALPHA_CHANNEL{ 3 };                                  // BGRA 알파 위치
            constexpr std::uint8_t OPAQUE_ALPHA{ 255 };                        // 불투명
            for (int y = 0; y < height; ++y)                                   // 줄마다
            {
                const int sourceY = static_cast<int>(static_cast<long long>(y) * sourceHeight / height);   // 원래 줄
                for (int x = 0; x < width; ++x)                                // 칸마다
                {
                    const int sourceX = static_cast<int>(static_cast<long long>(x) * sourceWidth / width);   // 원래 칸
                    const std::size_t from = (static_cast<std::size_t>(sourceY) * sourceWidth + sourceX) * FrameImage::BYTES_PER_PIXEL;   // 원래 위치
                    const std::size_t to = (static_cast<std::size_t>(y) * width + x) * FrameImage::BYTES_PER_PIXEL;                      // 새 위치
                    std::copy_n(source.begin() + static_cast<std::ptrdiff_t>(from), FrameImage::BYTES_PER_PIXEL, image->m_pixels.begin() + static_cast<std::ptrdiff_t>(to));   // BGRA 복사
                    image->m_pixels[to + ALPHA_CHANNEL] = OPAQUE_ALPHA;        // 불투명 (미리 곱한 알파 그대로)
                }
            }
            return image;   // 미리 보기
        }

        // 결과 → 설정 탭에 보일 설명 ("tooltip · Dest/top = 163439")
        [[nodiscard]] std::string DescribeResult(const ScanResult& result)
        {
            std::string text = result.m_screenName;                            // 화면 이름
            for (const ScanValue& value : result.m_values) text += std::format(" · {} = {}", value.m_key, value.m_value);   // 값들
            return text;   // 설명
        }

        // 지수 이동 평균 (처음 값이면 그대로)
        [[nodiscard]] double Average(double previous, double sample) noexcept
        {
            return previous <= 0.0 ? sample : previous + (sample - previous) * AVERAGE_WEIGHT;   // 평균
        }
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

    void ScreenWatcher::AddFrameDetector(std::unique_ptr<ScreenDetector> detector)
    {
        if (!detector || m_isRunning) return;             // 빈 인식기 · 감시 중에는 무시
        m_frameDetectors.push_back(std::move(detector));  // 등록
    }

    void ScreenWatcher::NotifyCharacterPicked() noexcept
    {
        m_isCharacterPicked = true;                       // 다음 주기부터
    }

    bool RegionChangeDetector::IsSame(const FrameImage& frame, const PixelRect& area)
    {
        constexpr int SAMPLE_CHANNEL{ 1 };                                             // BGRA 초록 (밝기에 가장 가까움)
        const PixelRect rect = ClampRect(area, frame.GetWidth(), frame.GetHeight());   // 프레임 안
        std::vector<std::uint8_t> samples;                                             // 이번 표본
        samples.reserve(static_cast<std::size_t>(rect.m_width / SAMPLE_STEP + 1) * (rect.m_height / SAMPLE_STEP + 1));
        const auto pixels = frame.GetPixels();                                         // BGRA
        for (int y = rect.m_y; y < rect.GetBottom(); y += SAMPLE_STEP)
            for (int x = rect.m_x; x < rect.GetRight(); x += SAMPLE_STEP)
                samples.push_back(pixels[(static_cast<std::size_t>(y) * frame.GetWidth() + x) * FrameImage::BYTES_PER_PIXEL + SAMPLE_CHANNEL]);
        bool isSame = !samples.empty() && m_rect == rect && m_samples.size() == samples.size();   // 같은 자리 · 크기
        if (isSame)
        {
            std::uint64_t difference = 0;                                              // 차이 합 (SAD)
            for (std::size_t index = 0; index < samples.size(); ++index)
                difference += static_cast<std::uint64_t>(std::abs(static_cast<int>(samples[index]) - static_cast<int>(m_samples[index])));
            isSame = static_cast<double>(difference) / static_cast<double>(samples.size()) <= MAX_SAME_DIFFERENCE;   // 평균 차이
        }
        m_rect = rect;                                                                 // 이번 영역
        m_samples = std::move(samples);                                                // 이번 표본
        return isSame;
    }

    void RegionChangeDetector::Reset() noexcept
    {
        m_samples.clear();   // 다음엔 "바뀜"
    }

    void CharacterValueGate::Hold(ScanResult result, Clock::time_point now)
    {
        if (m_entries.size() >= MAX_PENDING) m_entries.pop_front();   // 넘치면 오래된 것부터
        m_entries.push_back(Entry{ std::move(result), now });        // 붙잡기
    }

    std::vector<ScanResult> CharacterValueGate::Release()
    {
        std::vector<ScanResult> released;                             // 결과
        released.reserve(m_entries.size());                           // 자리
        for (Entry& entry : m_entries) released.push_back(std::move(entry.m_result));   // 모두
        m_entries.clear();                                            // 비움
        return released;
    }

    std::vector<ScanResult> CharacterValueGate::ReleaseExpired(Clock::time_point now)
    {
        std::vector<ScanResult> released;                             // 결과
        while (!m_entries.empty() && now - m_entries.front().m_heldAt >= MAX_HOLD)   // 오래된 것부터
        {
            released.push_back(std::move(m_entries.front().m_result));  // 내보냄
            m_entries.pop_front();                                       // 뺌
        }
        return released;
    }

    void CharacterValueGate::Discard() noexcept
    {
        m_entries.clear();   // 모두 버림
    }

    bool CharacterValueGate::HasPending() const noexcept
    {
        return !m_entries.empty();   // 붙잡은 값
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

    void ScreenWatcher::SetCharacterInterval(std::chrono::milliseconds interval) noexcept
    {
        m_characterIntervalMs = std::max(interval, MIN_CHARACTER_INTERVAL).count();   // 하한 적용 (다음 확인부터)
    }

    void ScreenWatcher::SetPreviewEnabled(bool isEnabled) noexcept
    {
        m_isPreviewEnabled = isEnabled;   // 다음 주기부터
    }

    void ScreenWatcher::SetBossScanEnabled(bool isEnabled) noexcept
    {
        m_isBossScanEnabled = isEnabled;   // 다음 주기부터
    }

    std::optional<BossHealthSample> ScreenWatcher::GetBossHealth() const
    {
        const std::scoped_lock lock{ m_mutex };                                                   // 보호
        if (!m_bossHealth || std::chrono::steady_clock::now() - m_bossHealth->m_capturedAt > BOSS_KEEP_DURATION) return std::nullopt;   // 오래됨
        return m_bossHealth;                                                                      // 복사본
    }

    void ScreenWatcher::SetCombatScanEnabled(bool isEnabled) noexcept
    {
        m_isCombatScanEnabled = isEnabled;   // 다음 주기부터
    }

    std::optional<CombatSample> ScreenWatcher::GetCombatStats() const
    {
        const std::scoped_lock lock{ m_mutex };                                                   // 보호
        if (!m_combatStats || std::chrono::steady_clock::now() - m_combatStats->m_capturedAt > COMBAT_KEEP_DURATION) return std::nullopt;   // 오래됨
        return m_combatStats;                                                                     // 복사본
    }

    void ScreenWatcher::SetBuffScanEnabled(bool isEnabled) noexcept
    {
        m_isBuffScanEnabled = isEnabled;   // 다음 주기부터
    }

    std::optional<BuffSample> ScreenWatcher::GetBuffs() const
    {
        const std::scoped_lock lock{ m_mutex };                                                   // 보호
        if (!m_buffs || std::chrono::steady_clock::now() - m_buffs->m_capturedAt > BUFF_KEEP_DURATION) return std::nullopt;   // 오래됨
        return m_buffs;                                                                           // 복사본
    }

    WatcherDiagnostics ScreenWatcher::GetDiagnostics() const
    {
        const std::scoped_lock lock{ m_mutex };   // 보호
        return m_diagnostics;                     // 복사본 (그림은 공유 포인터라 가벼움)
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
        ScanConfirmer frameConfirmer;                                              // 주기 인식기 확정 도구
        CharacterValueGate gate;                                                   // 캐릭터 확인 전 값 붙잡기
        auto nextFrameScan = std::chrono::steady_clock::now();                     // 다음 주기 인식기 시각
        const auto deliver = [this](ScanResult&& result)                           // 확정 값을 큐로 보냄
            {
                const std::scoped_lock lock{ m_mutex };                            // 큐 보호
                m_diagnostics.m_lastRead = DescribeResult(result);                 // 마지막으로 읽은 값
                m_diagnostics.m_lastReadAt = result.m_capturedAt;                  // 그 시각
                m_results.push_back(std::move(result));                            // 큐에 추가
            };
        ScanConfirmer characterConfirmer;                                          // 현재 캐릭터 확정 도구 (로딩 화면이면 새로 만듦)
        std::optional<CharacterDetector> characterDetector;                        // 현재 캐릭터 인식기 (이름을 받으면 만듦)
        std::uint64_t characterNamesVersion = 0;                                   // 받은 이름 버전
        auto nextCharacterScan = std::chrono::steady_clock::now();                 // 다음 캐릭터 확인 시각
        bool wasLoading = false;                                                   // 직전 확인이 로딩 화면이었는지 (로딩 알림은 한 번만)
        std::string lastCharacterName;                                             // 마지막으로 확정한 캐릭터 (같으면 확인 간격을 늘림)
        auto nextPreview = std::chrono::steady_clock::now();                       // 다음 미리 보기 그림 시각
        auto nextFullScan = std::chrono::steady_clock::now();                      // 다음 화면 전체 OCR 시각
        auto nextBossScan = std::chrono::steady_clock::now();                      // 다음 보스 체력 읽기 시각
        auto nextCombatScan = std::chrono::steady_clock::now();                    // 다음 전투 분석기 읽기 시각
        auto nextBuffScan = std::chrono::steady_clock::now();                      // 다음 버프 · 디버프 읽기 시각
        BossHealthTracker bossTracker;                                             // 보스 체력 투표 (오인식 걸러내기)
        RegionChangeDetector bossChange;                                               // 보스 체력바 영역 변경 감지
        std::optional<BossHealthReading> lastBossReading;                          // 그 영역에서 마지막으로 읽은 값 (같은 화면이면 다시 씀)
        RegionChangeDetector combatChange;                                             // 전투 분석기 영역 변경 감지
        std::optional<CombatReading> lastCombatReading;                            // 마지막으로 읽은 전투 분석기
        std::optional<PixelRect> combatStrip;                                      // 전투 분석기 머리글 · 값 띠 (찾으면 다음부터 그 띠만 읽음)
        RegionChangeDetector hoverChange;                                              // 커서 주변 영역 변경 감지
        std::optional<TextResult> lastHoverText;                                   // 커서 주변 마지막 OCR (같은 화면이면 다시 씀)
        auto lastBossSeen = std::chrono::steady_clock::now();                      // 마지막으로 체력바를 읽은 시각
        auto nextTimerScan = std::chrono::steady_clock::now();                     // 다음 타이머 읽기 시각
        auto nextNameScan = std::chrono::steady_clock::now();                      // 다음 보스 이름 읽기 시각
        std::optional<BossTimerReading> lastTimer;                                 // 마지막으로 읽은 화면 타이머
        auto lastTimerAt = std::chrono::steady_clock::now();                       // 그 시각
        std::optional<std::int64_t> fightMaximum;                                  // 지금 싸우는 보스의 최대 체력 (바뀌면 새 전투)
        auto fightStart = std::chrono::steady_clock::now();                        // 그 보스를 처음 본 시각 (표로 광폭화 남은 시간 계산)
        std::optional<int> enrageFromName;                                         // 보스 이름으로 찾은 광폭화 시간 (초)
        std::optional<int> raidSize;                                               // 지금 콘텐츠 인원 (4 · 8)
        auto nextRaidScan = std::chrono::steady_clock::now();                      // 다음 인원 확인 시각
        std::uint64_t previewVersion = 0;                                          // 미리 보기 그림 버전
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

                const auto planNow = std::chrono::steady_clock::now();             // 이번 주기에 할 일이 있는지 보는 시각
                const bool isFrameNeeded = (cursor && restTime >= REST_BEFORE_SCAN && scansThisRest < SCANS_PER_REST)   // 커서 주변 읽기
                    || (m_isPreviewEnabled && planNow >= nextPreview)                   // 미리 보기
                    || (m_isBossScanEnabled && planNow >= nextBossScan)                 // 보스 체력
                    || (m_isCombatScanEnabled && planNow >= nextCombatScan)             // 전투 분석기
                    || (m_isBuffScanEnabled && planNow >= nextBuffScan)                 // 버프 · 디버프
                    || (m_isCharacterScanEnabled && planNow >= nextCharacterScan)       // 캐릭터 확인
                    || (!m_frameDetectors.empty() && planNow >= nextFrameScan);         // 창고 슬롯
                auto frame = capture.TakeLatestFrame(isFrameNeeded);               // 최신 프레임 (할 일이 없으면 쌓인 프레임만 비우고 복사 안 함)
                if (frame)                                                         // 받았으면
                {
                    const std::scoped_lock lock{ m_mutex };                        // 상태 보호
                    m_status.m_lastFrameAt = std::chrono::system_clock::now();     // 마지막 프레임 시각
                    m_diagnostics.m_frameWidth = frame->GetWidth();                // 프레임 크기
                    m_diagnostics.m_frameHeight = frame->GetHeight();
                }
                if (frame && m_isPreviewEnabled && std::chrono::steady_clock::now() >= nextPreview)   // 미리 보기 그림 (설정 탭이 보일 때만)
                {
                    nextPreview = std::chrono::steady_clock::now() + PREVIEW_INTERVAL;   // 다음 시각
                    auto preview = MakePreview(*frame, ++previewVersion);          // 줄인 그림 (잠금 밖에서 만듦)
                    const std::scoped_lock lock{ m_mutex };                        // 보호
                    m_diagnostics.m_preview = std::move(preview);                  // 교체
                }

                const auto steadyNow = std::chrono::steady_clock::now();           // 지금 (캐릭터 확인 주기용)
                if (frame && m_isBossScanEnabled && steadyNow >= nextBossScan)     // 보스 체력 (커서와 상관없이)
                {
                    nextBossScan = steadyNow + BOSS_SCAN_INTERVAL;                 // 다음 시각
                    const bool isSameBoss = bossChange.IsSame(*frame, BossHealthDetector::GetBarArea(frame->GetWidth(), frame->GetHeight()));   // 지난번과 같은 화면
                    if (!isSameBoss) lastBossReading = BossHealthDetector::Detect(*frame, text);   // 바뀌었을 때만 OCR
                    if (const auto& boss = lastBossReading)                        // 체력바 (같은 화면이면 지난 값)
                    {
                        lastBossSeen = steadyNow;                                  // 봤음
                        if (const auto confirmed = bossTracker.Feed(*boss))        // 투표로 믿을 만한 값만
                        {
                            if (fightMaximum != confirmed->m_maximum)              // 새 보스 · 관문
                            {
                                fightMaximum = confirmed->m_maximum;               // 기록
                                fightStart = steadyNow;                            // 전투 시작
                                enrageFromName.reset();                            // 이름 표 다시
                                nextNameScan = steadyNow;
                                raidSize.reset();                                  // 인원 다시
                                nextRaidScan = steadyNow;
                            }
                            if (steadyNow >= nextTimerScan)                        // 화면 타이머 (1초마다)
                            {
                                nextTimerScan = steadyNow + BOSS_TIMER_INTERVAL;
                                if (const auto timer = BossTimerDetector::Detect(*frame, text)) { lastTimer = timer; lastTimerAt = steadyNow; }   // 읽음
                            }
                            const bool isTimerFresh = lastTimer && steadyNow - lastTimerAt <= BOSS_TIMER_KEEP;   // 최근에 읽은 타이머
                            if (!isTimerFresh && !enrageFromName && confirmed->m_textBounds && steadyNow >= nextNameScan)   // 타이머가 안 보임 → 보스 이름으로 표 찾기
                            {
                                nextNameScan = steadyNow + BOSS_NAME_INTERVAL;
                                enrageFromName = BossTimerDetector::ReadEnrageFromName(*frame, text, *confirmed->m_textBounds);
                            }
                            if (!raidSize && steadyNow >= nextRaidScan)            // 인원 (가디언 토벌 · 보스 이름, 알 때까지 가끔)
                            {
                                nextRaidScan = steadyNow + BOSS_NAME_INTERVAL;
                                raidSize = BossTimerDetector::DetectRaidSize(*frame, text, confirmed->m_textBounds);
                            }
                            BossHealthSample sample{ *confirmed, steadyNow };      // 기록할 값
                            sample.m_raidSize = raidSize;                          // 인원
                            if (isTimerFresh)                                      // 화면 타이머에서 지난 시간만큼 줄임
                            {
                                sample.m_timeLeft = std::max(0.0, lastTimer->m_seconds - std::chrono::duration<double>(steadyNow - lastTimerAt).count());
                                sample.m_isEnrage = lastTimer->m_isEnrage;
                            }
                            else if (enrageFromName)                               // 표의 광폭화 시간 - 싸운 시간
                            {
                                sample.m_timeLeft = std::max(0.0, *enrageFromName - std::chrono::duration<double>(steadyNow - fightStart).count());
                                sample.m_isEnrage = true;
                            }
                            const std::scoped_lock lock{ m_mutex };                // 보호
                            m_bossHealth = std::move(sample);                      // 기록
                        }
                    }
                    else if (steadyNow - lastBossSeen > BOSS_KEEP_DURATION) bossTracker.Reset();   // 보스가 사라짐 → 투표 새로
                }
                if (frame && m_isCombatScanEnabled && steadyNow >= nextCombatScan) // 전투 분석기 (커서와 상관없이)
                {
                    nextCombatScan = steadyNow + COMBAT_SCAN_INTERVAL;             // 다음 시각
                    const bool isSameCombat = combatChange.IsSame(*frame, CombatAnalyzerDetector::GetArea(frame->GetWidth(), frame->GetHeight()));   // 지난번과 같은 화면
                    if (!isSameCombat) lastCombatReading = CombatAnalyzerDetector::Detect(*frame, text, &combatStrip);   // 바뀌었을 때만 OCR (찾은 띠만)
                    if (const auto& combat = lastCombatReading)                    // 피해량 · DPS (같은 화면이면 지난 값)
                    {
                        const std::scoped_lock lock{ m_mutex };                    // 보호
                        m_combatStats = CombatSample{ *combat, steadyNow };        // 기록
                    }
                }
                if (frame && m_isBuffScanEnabled && steadyNow >= nextBuffScan)     // 버프 · 디버프 (커서와 상관없이, OCR 없는 픽셀 계산)
                {
                    nextBuffScan = steadyNow + BUFF_SCAN_INTERVAL;                 // 다음 시각
                    BuffReading buffs = BuffDetector::Detect(*frame);              // 읽기 (잠금 밖에서)
                    const std::scoped_lock lock{ m_mutex };                        // 보호
                    m_buffs = BuffSample{ std::move(buffs), steadyNow };           // 기록 (아무것도 없으면 빈 목록 = 버프 끝남)
                }
                if (frame && m_isCharacterScanEnabled && steadyNow >= nextCharacterScan)   // 캐릭터 확인할 때 (커서와 상관없이)
                {
                    const std::chrono::milliseconds characterInterval{ m_characterIntervalMs.load() };   // 설정한 간격
                    nextCharacterScan = steadyNow + characterInterval;             // 다음 확인 시각
                    {
                        const std::scoped_lock lock{ m_mutex };                    // 이름 보호
                        if (!characterDetector || characterNamesVersion != m_characterNamesVersion)   // 이름이 바뀌었으면
                        {
                            characterDetector.emplace(m_characterNames, m_characterAliases, m_nameplateMemory);   // 인식기 다시 만들기 (이름 · 별칭 복사, 기억은 공유)
                            characterNamesVersion = m_characterNamesVersion;       // 버전 기록
                        }
                    }
                    const auto scanStart = std::chrono::steady_clock::now();      // 처리 시간 재기
                    const bool isFullScanAllowed = steadyNow >= nextFullScan;      // 화면 전체 OCR 해도 되는지
                    const bool isVerifyRequired = lastCharacterName.empty();       // 로딩 직후 · 시작 직후 → 이름표 글자로 다시 확인 (다른 캐릭터 모양과 헷갈리지 않게)
                    const CharacterReading reading = characterDetector->Detect(*frame, text, isFullScanAllowed, isVerifyRequired);   // 로딩 · 이름표 (+ 가끔 화면 전체)
                    if (isFullScanAllowed && reading.m_signal != CharacterSignal::SIGNAL_NAME) nextFullScan = steadyNow + FULL_SCAN_INTERVAL;   // 화면 전체를 봤으면 한동안 쉼 (이름표 모양으로 알아보면 화면 전체는 안 봄)
                    {
                        const std::scoped_lock lock{ m_mutex };                    // 보호
                        const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - scanStart).count() * MILLISECONDS_PER_SECOND;   // 밀리초
                        m_diagnostics.m_characterScanMs = Average(m_diagnostics.m_characterScanMs, elapsed);   // 평균
                        if (reading.m_signal != CharacterSignal::SIGNAL_LOADING) m_diagnostics.m_nameplate = reading.m_plateBounds;   // 이름표 위치 (없으면 지움)
                    }
                    ScanResult characterResult;                                    // 결과
                    characterResult.m_capturedAt = std::chrono::system_clock::now();   // 시각
                    if (reading.m_signal == CharacterSignal::SIGNAL_LOADING)       // 로딩 화면 → 캐릭터가 바뀌었을 수 있음
                    {
                        nextCharacterScan = steadyNow + CHARACTER_LOADING_INTERVAL;// 로딩 중엔 자주 확인
                        characterConfirmer = ScanConfirmer{};                      // 확정 기록 지움 (로딩 뒤 같은 캐릭터도 다시 확정)
                        lastCharacterName.clear();                                 // 확정 이름도 지움
                        m_isCharacterPicked = false;                               // 직접 고른 캐릭터도 이제 모름
                        gate.Discard();                                            // 로딩 전후에 읽은 값은 어느 캐릭터 것인지 모름 → 버림
                        confirmer = ScanConfirmer{};                               // 새 캐릭터에서 같은 값을 다시 읽어도 다시 확정되게
                        frameConfirmer = ScanConfirmer{};
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
                        if (isSameAsConfirmed) nextCharacterScan = steadyNow + characterInterval * STEADY_INTERVAL_FACTOR;   // 같은 캐릭터면 천천히
                        if (lastCharacterName == characterResult.m_characterName)  // 확정한 캐릭터를 지금 봄 → 붙잡은 값은 이 캐릭터 것
                        {
                            for (ScanResult& released : gate.Release()) deliver(std::move(released));   // 내보냄
                        }
                        else if (gate.HasPending())                                // 다른 캐릭터가 보이기 시작 → 그 사이 값은 어느 캐릭터 것인지 모름
                        {
                            gate.Discard();                                        // 버림
                            confirmer = ScanConfirmer{};                           // 다시 읽으면 다시 확정되게
                            frameConfirmer = ScanConfirmer{};
                        }
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

                if (frame && !m_frameDetectors.empty() && steadyNow >= nextFrameScan)   // 주기 인식기 (커서와 상관없이)
                {
                    nextFrameScan = steadyNow + FRAME_SCAN_INTERVAL;               // 다음 시각
                    const TextResult noText;                                       // 주기 인식기는 OCR 안 씀
                    const DetectorContext frameContext{ *layouts, text, noText, PixelRect{ 0, 0, frame->GetWidth(), frame->GetHeight() }, PixelPoint{} };   // 화면 전체
                    for (const auto& detector : m_frameDetectors)                  // 인식기마다
                    {
                        try
                        {
                            auto result = detector->Detect(*frame, frameContext);  // 읽기
                            if (!result) { frameConfirmer.Reset(detector->GetScreenName()); continue; }   // 안 보임
                            result->m_screenName = std::string{ detector->GetScreenName() };   // 화면 이름 보장
                            result->m_capturedAt = std::chrono::system_clock::now();           // 시각
                            if (auto confirmed = frameConfirmer.Feed(*result))     // 두 번 연속 같으면
                            {
                                if (!m_isCharacterScanEnabled) deliver(std::move(*confirmed));   // 캐릭터 확인을 끔 → 바로
                                else
                                {
                                    gate.Hold(std::move(*confirmed), steadyNow);   // 캐릭터 확인 뒤에
                                    nextCharacterScan = std::min(nextCharacterScan, steadyNow + GATE_CHECK_DELAY);   // 곧 확인
                                }
                            }
                        }
                        catch (const std::exception& error)
                        {
                            SetStatus(WatcherState::WATCHER_ERROR, error.what());  // 인식기 오류 안내
                        }
                    }
                }
                if (gate.HasPending())                                             // 붙잡은 값
                {
                    if (!m_isCharacterScanEnabled)                                 // 캐릭터 확인을 껐으면 모두 바로
                        for (ScanResult& released : gate.Release()) deliver(std::move(released));
                    else if ((!lastCharacterName.empty() || m_isCharacterPicked) && !wasLoading)   // 캐릭터가 정해져 있으면 오래 기다린 값만
                        for (ScanResult& released : gate.ReleaseExpired(std::chrono::steady_clock::now())) deliver(std::move(released));
                }

                const bool isReadyToScan = cursor && restTime >= REST_BEFORE_SCAN && scansThisRest < SCANS_PER_REST;   // 읽을 때인지
                if (isReadyToScan && frame)                                        // 멈춘 커서 + 프레임이 있으면
                {
                    ++scansThisRest;                                               // 읽은 횟수
                    const auto hoverStart = std::chrono::steady_clock::now();     // 처리 시간 재기
                    const PixelRect hoverArea = GetHoverArea(*cursor, frame->GetWidth(), frame->GetHeight());   // 커서 주변
                    if (!hoverChange.IsSame(*frame, hoverArea) || !lastHoverText)   // 바뀌었거나 처음이면 OCR (같은 멈춤의 두 번째 읽기는 보통 같은 화면)
                        lastHoverText = text.Recognize(frame->Crop(hoverArea), OCR_SCALE);   // 커서 주변 OCR
                    const auto& hoverText = lastHoverText;                         // 이번에 쓸 OCR 결과
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
                                if (!m_isCharacterScanEnabled) deliver(std::move(*confirmed));   // 캐릭터 확인을 끔 → 바로 보냄
                                else                                               // 캐릭터를 다시 확인한 뒤에 보냄
                                {
                                    gate.Hold(std::move(*confirmed), std::chrono::steady_clock::now());
                                    nextCharacterScan = std::min(nextCharacterScan, std::chrono::steady_clock::now() + GATE_CHECK_DELAY);   // 곧 확인
                                }
                            }
                        }
                        catch (const std::exception& error)
                        {
                            SetStatus(WatcherState::WATCHER_ERROR, error.what());  // 인식기 오류 안내 (다른 인식기는 계속)
                        }
                    }
                    const std::scoped_lock lock{ m_mutex };                        // 보호
                    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - hoverStart).count() * MILLISECONDS_PER_SECOND;   // 밀리초
                    m_diagnostics.m_hoverScanMs = Average(m_diagnostics.m_hoverScanMs, elapsed);   // 커서 주변 읽기 평균
                    m_diagnostics.m_hoverArea = hoverArea;                         // 읽은 영역
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
