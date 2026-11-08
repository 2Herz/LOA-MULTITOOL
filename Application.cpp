module LostArkMultiTool.Views.Application;

import std;
import LostArkMultiTool.Core.APIKeyStore;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.ServerConfigChecker;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.ServerConfig;
import LostArkMultiTool.Services.DPAPIKeyStore;
import LostArkMultiTool.Services.GistServerConfigChecker;
import LostArkMultiTool.Services.WinHTTPMarketAPI;
import LostArkMultiTool.Views.AccessoryFinderManager;
import LostArkMultiTool.Views.APIKeyDialog;
import LostArkMultiTool.Views.ArkGemFinderManager;
import LostArkMultiTool.Views.CalculatorManager;
import LostArkMultiTool.Views.CombatSimulatorManager;
import LostArkMultiTool.Views.DX11Renderer;
import LostArkMultiTool.Views.EngravingFinderManager;
import LostArkMultiTool.Views.GemFinderManager;
import LostArkMultiTool.Views.MainWindow;
import LostArkMultiTool.Views.RefineBookFinderManager;
import LostArkMultiTool.Views.RefineSimulatorManager;
import LostArkMultiTool.Views.ServerCheckDialog;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.TrackerManager;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.OverlayNotifier;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view WINDOW_TITLE{ L"로스트아크 멀티툴" };

        // 원격 점검 상태를 따를지 (작업 중에는 점검을 켜 두므로 Debug 빌드는 무시하고 바로 테스트, Release 배포판은 항상 따름)
#ifdef _DEBUG  // Debug 빌드에서만 적용
        constexpr bool IS_MAINTENANCE_LOCK_ENABLED{ false };  // Debug 빌드는 원격 점검 상태를 따르지 않음 (바로 테스트)
#else  // Release 빌드에서는
        constexpr bool IS_MAINTENANCE_LOCK_ENABLED{ true };  // 원격 점검 상태를 항상 따름
#endif  // 빌드 구성 분기 끝
        constexpr int WINDOW_WIDTH{ 1920 };    // 처음 창 안쪽 크기 (DIP)
        constexpr int WINDOW_HEIGHT{ 1080 };

        // ===== 탭 막대 (DIP) =====
        constexpr float TAB_BAR_HEIGHT{ 44.0f };
        constexpr float TAB_WIDTH{ 180.0f };            // 탭 하나의 최대 너비 (탭이 많고 창이 좁으면 GetTabWidth가 줄임)
        constexpr float TAB_INDICATOR_HEIGHT{ 3.0f };   // 선택한 탭 밑줄 두께
        constexpr float TAB_DIVIDER_MARGIN{ 12.0f };    // 탭 사이 구분선의 위아래 여백
        constexpr float LINE_WIDTH{ 1.0f };
        constexpr float HEADER_BUTTON_WIDTH{ 110.0f };  // 오른쪽 "API 키 설정" 버튼
        constexpr float HEADER_BUTTON_MARGIN{ 7.0f };
        constexpr float HEADER_MARGIN_SIDES{ 2.0f };    // 버튼 양옆 여백 수 (탭 너비 계산용)

        // ===== 탭 전환 애니메이션 =====
        constexpr double INDICATOR_SMOOTHING{ 14.0 };   // 클수록 밑줄이 빨리 따라감 (초당 감쇠 계수)
        constexpr float INDICATOR_SNAP_DISTANCE{ 0.001f };
        constexpr double CONTENT_FADE_SECONDS{ 0.18 };  // 새 탭 내용이 나타나는 시간
        constexpr float FULL_FADE{ 1.0f };
        constexpr float NO_FADE{ 0.0f };
        constexpr float HOVER_ALPHA{ 0.35f };           // 마우스를 올린 탭 배경 투명도
        constexpr double MAX_FRAME_SECONDS{ 0.1 };      // 창을 끌거나 멈췄다 돌아왔을 때 애니메이션이 튀지 않게
        constexpr int BACKGROUND_TICK_MS{ 250 };        // 트레이에 숨은 동안 백그라운드 작업 · 자동 갱신을 확인하는 간격

        // ===== 프레임 간격 측정 =====
        using Clock = std::chrono::steady_clock;        // 시스템 시간을 바꿔도 거꾸로 가지 않는 시계

        // 시계 눈금(tick) 1초당 개수 (눈금 단위 = Clock::period, MSVC는 나노초 → 1'000'000'000)
        constexpr double CLOCK_TICKS_PER_SECOND{ static_cast<double>(Clock::period::den) / static_cast<double>(Clock::period::num) };

        // ===== 단축키 =====
        constexpr std::size_t NEXT_TAB_STEP{ 1 };

        // ===== 문구 =====
        constexpr std::wstring_view API_KEY_BUTTON_TEXT{ L"API 키 설정" };
        constexpr std::wstring_view KEY_SAVE_ERROR_TITLE{ L"API 키 저장 실패" };
        constexpr std::wstring_view KEY_SAVE_ERROR_TEXT{ L"API 키를 저장하지 못했어요!\n" };
        constexpr std::wstring_view KEY_SAVED_TEXT{ L"API 키를 저장했어요!" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };

        // ===== 트레이 · 원격 설정 문구 =====
        constexpr std::wstring_view TRAY_HIDDEN_TITLE{ L"로스트아크 멀티툴 백그라운드 실행 중" };
        constexpr std::wstring_view TRAY_HIDDEN_DEFAULT_TEXT{ L"트레이 아이콘을 더블클릭하면 다시 열려요." };
        constexpr std::wstring_view LINE_BREAK{ L"\n" };
        constexpr std::wstring_view PARAGRAPH_BREAK{ L"\n\n" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"서버 점검 안내" };
        constexpr std::wstring_view MAINTENANCE_EXIT_TEXT{ L"\n\n프로그램을 종료해요." };
        constexpr std::wstring_view UPDATE_TITLE{ L"업데이트 안내" };
        constexpr std::wstring_view UPDATE_TEXT_FORMAT{ L"새로운 버전({})이 출시됐어요!\n최신 버전으로 업데이트를 권장해요." };
        constexpr std::wstring_view UPDATE_QUESTION_TEXT{ L"\n\n다운로드 페이지를 열까요?" };
        constexpr std::wstring_view UPDATE_OPEN_FAILED_TEXT{ L"다운로드 페이지를 열지 못했어요.\n" };
        constexpr std::wstring_view NOTICE_TITLE{ L"공지" };
        constexpr std::wstring_view HTTPS_PREFIX{ L"https://" };
        constexpr std::wstring_view HTTP_PREFIX{ L"http://" };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" \t\r\n" };

        constexpr Core::TextStyle ACTIVE_TAB_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::BOLD, Core::TextAlign::CENTER };
        constexpr Core::TextStyle INACTIVE_TAB_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };

        // 제목 표시줄: "로스트아크 멀티툴 v1.1.0"
        [[nodiscard]] std::wstring BuildWindowTitle()
        {
            return std::wstring{ WINDOW_TITLE } + L" v" + Core::TextEncoding::UTF8ToWide(Core::APPConstants::APP_VERSION);
        }

        // 현재 시각을 시계 눈금 수로 (duration 변환 템플릿을 거치지 않아 편집기가 해석하지 못하는 부분이 없음)
        [[nodiscard]] Clock::rep GetClockTicks() noexcept
        {
            return Clock::now().time_since_epoch().count();
        }

        // 두 눈금 사이 시간 (초) — duration<double> 변환과 같은 계산 (정수 눈금 → double → 1초당 눈금 수로 나눔)
        [[nodiscard]] double TicksToSeconds(Clock::rep elapsedTicks) noexcept
        {
            return static_cast<double>(elapsedTicks) / CLOCK_TICKS_PER_SECOND;
        }

        // 원격 설정 글자(UTF-8) → 화면 문구 (앞뒤 공백 제거, 비었거나 변환할 수 없으면 빈 문자열)
        [[nodiscard]] std::wstring ToTrimmedText(const std::optional<std::string>& text)
        {
            if (!text.has_value()) return {};

            try
            {
                const std::wstring wideText{ Core::TextEncoding::UTF8ToWide(*text) };
                const std::size_t first{ wideText.find_first_not_of(TRIM_CHARACTERS) };
                if (first == std::wstring::npos) return {};
                return wideText.substr(first, wideText.find_last_not_of(TRIM_CHARACTERS) - first + 1);
            }
            catch (const std::exception&)
            {
                return {};
            }
        }

        [[nodiscard]] bool IsWebURL(std::wstring_view url) noexcept
        {
            return url.starts_with(HTTPS_PREFIX) || url.starts_with(HTTP_PREFIX);
        }

        // 예외 메시지(UTF-8) → 화면 문구(UTF-16), 변환할 수 없으면 기본 문구
        [[nodiscard]] std::wstring ToWideMessage(const char* message)
        {
            try
            {
                return Core::TextEncoding::UTF8ToWide(message);
            }
            catch (const std::exception&)
            {
                return std::wstring{ UNKNOWN_ERROR_TEXT };
            }
        }
    }

    Application::Application()
        : m_tools{ CalculatorManager::GetInstance(), GemFinderManager::GetInstance(), TrackerManager::GetInstance(), EngravingFinderManager::GetInstance(), AccessoryFinderManager::GetInstance(), RefineBookFinderManager::GetInstance(), ArkGemFinderManager::GetInstance(), CombatSimulatorManager::GetInstance(), RefineSimulatorManager::GetInstance() }  // 탭 순서대로 매니저 목록 초기화 (전투력 시뮬레이터 탭 등록 포함, 매니저는 싱글톤이 소유)
    {
        // 창을 만드는 도중에도 WM_SIZE가 OnResize로 들어오므로, 모든 unique_ptr가 비어 있는 상태로 만들어진 뒤에 생성
        m_window = std::make_unique<MainWindow>(BuildWindowTitle(), WINDOW_WIDTH, WINDOW_HEIGHT, *this);
        m_renderer = std::make_unique<DX11Renderer>(m_window->GetNativeHandle(), m_window->GetDPI());

        // 공용 서비스 (세 툴이 API 키 하나 · API 하나 · 알림 층 하나를 공유)
        m_keyStore = std::make_unique<Services::DPAPIKeyStore>();
        const std::optional<std::string> savedKey{ m_keyStore->Load() };
        m_hasAPIKey = savedKey.has_value() && !savedKey->empty();
        m_marketAPI = std::make_unique<Services::WinHTTPMarketAPI>(savedKey.value_or(std::string{}));
        m_notifier = std::make_unique<UI::OverlayNotifier>(*m_window);   // 창이 숨어 있으면 토스트 → 트레이 풍선 알림
        m_configChecker = std::make_unique<Services::GistServerConfigChecker>();

        m_toolContext = std::make_unique<Core::ToolContext>(Core::ToolContext{ *m_window, *m_notifier, *m_marketAPI,
            [this] { OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_CHANGE); } });

        // 탭 막대 오른쪽 버튼 (키보드 포커스를 가져가지 않음 → 탭 화면 입력칸의 포커스 유지)
        m_apiKeyButton = &m_headerHost.Emplace<UI::Button>(std::wstring{ API_KEY_BUTTON_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY,
            [this] { OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_CHANGE); });
        m_apiKeyButton->SetFocusable(false);
    }

    Application::~Application()
    {
        // 예외로 Run을 빠져나온 경우에도 매니저 저장 기회를 줌 (소멸자 밖으로 예외를 던지지 않음)
        try
        {
            ShutdownTools();
        }
        catch (...)
        {
        }

        // 원격 설정 확인이 진행 중이면 끝날 때까지 기다림 (확인 객체보다 먼저)
        m_configTask.Wait();

        // 서비스 · 렌더러(스왑체인)를 먼저 놓고 창을 닫음 → 창이 닫히며 오는 메시지는 빈 포인터 검사로 무시됨
        m_toolContext.reset();
        m_notifier.reset();
        m_renderer.reset();
        m_window.reset();
    }

    int Application::Run(int showCommand)
    {
        InitializeTools();
        GetActiveTool().OnActivated();
        m_window->Show(showCommand);

        // 서버 상태부터 확인 (원본 ServerCheckForm) → 점검이면 안내 후 종료, 아니면 API 키 확인 · 업데이트 안내
        StartServerCheck(true);

        Clock::rep previousTicks{ GetClockTicks() };
        while (m_window->ProcessMessages())
        {
            const Clock::rep currentTicks{ GetClockTicks() };
            const double deltaSeconds{ TicksToSeconds(currentTicks - previousTicks) };
            previousTicks = currentTicks;

            // 트레이에 숨어 있어도 갱신은 계속 (백그라운드 작업 결과 · 자동 갱신 · 점검 확인)
            Update(std::min(deltaSeconds, MAX_FRAME_SECONDS));

            if (m_window->IsMinimized())
            {
                // 그리지 않고 잠깐씩만 깨어남 (메시지가 오면 바로 깨어남 → CPU · GPU 사용 0에 가깝게)
                m_window->WaitForMessage(BACKGROUND_TICK_MS);
                continue;
            }
            Render();   // EndFrame의 수직 동기화가 프레임 속도를 모니터 주사율로 맞춤
        }

        if (!m_isModal) GetActiveTool().OnDeactivated();   // 대화상자가 열려 있었다면 이미 내려놓은 상태
        ShutdownTools();
        return m_window->GetExitCode();
    }

    void Application::OnResize(int pixelWidth, int pixelHeight)
    {
        if (!m_renderer) return;   // 창을 만드는 도중 (렌더러 생성 전)

        m_renderer->Resize(pixelWidth, pixelHeight);
        Render();   // 창 크기를 끄는 동안(메시지 루프가 멈춘 상태)에도 화면이 따라오게
    }

    void Application::OnDPIChanged(float dpi)
    {
        if (m_renderer) m_renderer->SetDPI(dpi);
    }

    void Application::OnInput(const Core::InputEvent& event)
    {
        if (!m_notifier || !m_window || m_initializedToolCount < TOOL_COUNT) return;   // 만드는 중 · 닫는 중

        SyncModalState();
        if (m_isModal)
        {
            m_notifier->HandleInput(event);   // 대화상자가 열려 있으면 대화상자만 입력을 받음
        }
        else
        {
            const auto* key{ std::get_if<Core::KeyEvent>(&event) };
            if (key == nullptr || !HandleShortcut(*key)) RouteInput(event);
        }

        SyncModalState();   // 입력 처리 중 대화상자가 열리거나 닫혔을 수 있음
        UpdateCursor();
    }

    void Application::OnTrayCommand(TrayCommand command)
    {
        switch (command)
        {
            case TrayCommand::TRAY_OPEN:
                m_window->RestoreWindow();
                break;

            case TrayCommand::TRAY_CHANGE_API_KEY:
                // 창을 꺼내서 키 입력 창을 띄움 (점검 안내 중이거나 아직 시작 전이면 열지 않음)
                m_window->RestoreWindow();
                if (m_notifier && !m_isMaintenanceLocked && m_initializedToolCount == TOOL_COUNT) OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_CHANGE);
                break;

            case TrayCommand::TRAY_EXIT:
                m_window->RequestClose();
                break;

            default:
                break;
        }
    }

    // 트레이로 숨으면 백그라운드에서 무엇이 계속 도는지 알려 줌 (원본 Form1_Resize 풍선 알림)
    void Application::OnHiddenToTray()
    {
        std::wstring message;
        if (m_initializedToolCount == TOOL_COUNT)
        {
            for (const Core::ToolManager& tool : m_tools)
            {
                const std::optional<std::wstring> status{ tool.GetBackgroundStatus() };
                if (!status.has_value()) continue;

                if (!message.empty()) message += LINE_BREAK;
                message += *status;
            }
        }
        if (message.empty()) message = TRAY_HIDDEN_DEFAULT_TEXT;

        m_window->ShowTrayBalloon(TRAY_HIDDEN_TITLE, message, Core::NoticeKind::NOTICE_INFO);
    }

    // 원격 설정 받기 시작 (블로킹 요청이라 백그라운드에서, 실행 직후면 확인 중 대화상자를 띄움)
    void Application::StartServerCheck(bool isStartup)
    {
        if (m_configTask.IsRunning() || m_isMaintenanceLocked) return;

        m_isStartupCheck = isStartup;
        if (isStartup)
        {
            auto dialog{ std::make_unique<ServerCheckDialog>() };
            m_serverCheckDialog = dialog.get();
            m_notifier->OpenDialog(std::move(dialog));
        }

        Core::ServerConfigChecker* checker{ m_configChecker.get() };
        m_configTask.Start([checker] { return checker->Fetch(); });
    }

    // 원격 설정을 받음 → 점검이면 종료 안내, 실행 직후면 주기 확인 시작 · 공지 · 첫 실행 키 입력
    void Application::FinishServerCheck()
    {
        std::optional<Models::ServerConfig> config;
        try
        {
            config = m_configTask.TakeResult();
        }
        catch (const std::exception&)
        {
            config.reset();   // 확인 실패는 점검 아님으로 취급 (원본과 같음 → 서버 문제로 프로그램이 막히지 않게)
        }

        const bool isStartup{ m_isStartupCheck };
        if (m_serverCheckDialog != nullptr)
        {
            m_serverCheckDialog->Close();   // 실제 삭제는 다음 Update에서 (OverlayNotifier)
            m_serverCheckDialog = nullptr;
        }

        // Debug 빌드는 점검 중이어도 잠그지 않음 (작업 중 테스트용, IS_MAINTENANCE_LOCK_ENABLED 참고)
        if (IS_MAINTENANCE_LOCK_ENABLED && config.has_value() && config->IsMaintenance())
        {
            LockForMaintenance(config->GetMaintenanceMessage());
            return;
        }
        if (!isStartup) return;

        m_configTimer.Start(Core::APPConstants::CONFIG_CHECK_INTERVAL_MS);

        // 대화상자는 나중에 연 것이 위 → 키 입력 창을 맨 위로, 업데이트 · 공지는 그다음에 보이게
        if (config.has_value()) ShowStartupNotices(*config);
        if (!m_hasAPIKey) OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_FIRST_RUN);
    }

    // 점검 중: 숨어 있어도 창을 꺼내 안내 → [확인]을 누르면 종료 (원본: 실행 중 점검 감지 → 안내 후 Application.Exit)
    void Application::LockForMaintenance(const std::optional<std::string>& maintenanceMessage)
    {
        m_isMaintenanceLocked = true;
        m_configTimer.Stop();
        m_window->RestoreWindow();

        std::wstring message{ ToTrimmedText(maintenanceMessage) };
        if (message.empty()) message = ToTrimmedText(std::string{ Core::APPConstants::DEFAULT_MAINTENANCE_MESSAGE });

        m_notifier->ShowMessage(std::wstring{ MAINTENANCE_TITLE }, message + std::wstring{ MAINTENANCE_EXIT_TEXT }, Core::NoticeKind::NOTICE_WARNING,
            [this] { m_window->RequestClose(); });
    }

    // 새 버전 안내 (공지 포함) 또는 공지만 (원본 ShowUpdateNotice)
    void Application::ShowStartupNotices(const Models::ServerConfig& config)
    {
        const std::wstring notice{ ToTrimmedText(config.GetNotice()) };
        const std::wstring latestVersion{ ToTrimmedText(config.GetLatestVersion()) };
        const std::string latestVersionText{ config.GetLatestVersion().value_or(std::string{}) };

        if (latestVersion.empty() || !Core::ServerConfigChecker::IsNewerVersion(latestVersionText, Core::ServerConfigChecker::GetCurrentVersion()))
        {
            if (!notice.empty()) m_notifier->ShowMessage(std::wstring{ NOTICE_TITLE }, notice, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        std::wstring message{ std::format(UPDATE_TEXT_FORMAT, latestVersion) };
        if (!notice.empty()) message += std::wstring{ PARAGRAPH_BREAK } + notice;

        // 다운로드 주소가 웹 주소일 때만 열지 물어봄 (파일 · 프로그램 실행 방지)
        const std::wstring downloadURL{ ToTrimmedText(config.GetDownloadURL()) };
        if (!IsWebURL(downloadURL))
        {
            m_notifier->ShowMessage(std::wstring{ UPDATE_TITLE }, message, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        m_notifier->AskYesNo(std::wstring{ UPDATE_TITLE }, message + std::wstring{ UPDATE_QUESTION_TEXT }, [this, downloadURL](bool isYes)
        {
            if (!isYes || m_window->OpenURL(downloadURL)) return;
            m_notifier->ShowMessage(std::wstring{ UPDATE_TITLE }, std::wstring{ UPDATE_OPEN_FAILED_TEXT } + downloadURL, Core::NoticeKind::NOTICE_WARNING, nullptr);
        });
    }

    void Application::InitializeTools()
    {
        for (Core::ToolManager& tool : m_tools)
        {
            tool.Initialize(*m_toolContext);
            ++m_initializedToolCount;
        }
    }

    void Application::ShutdownTools()
    {
        // 초기화한 역순으로 정리, 하나가 실패해도 나머지는 끝까지 저장하고 첫 예외만 다시 던짐
        std::exception_ptr firstError;
        while (m_initializedToolCount > 0)
        {
            --m_initializedToolCount;
            try
            {
                m_tools[m_initializedToolCount].get().Shutdown();
            }
            catch (...)
            {
                if (!firstError) firstError = std::current_exception();
            }
        }

        if (firstError) std::rethrow_exception(firstError);
    }

    void Application::SelectTool(std::size_t index)
    {
        if (index >= TOOL_COUNT || index == m_activeIndex) return;

        GetActiveTool().OnDeactivated();
        m_activeIndex = index;
        GetActiveTool().OnActivated();

        m_contentFade = FULL_FADE;   // 새 탭 내용이 배경에서 서서히 나타나게
    }

    void Application::OpenAPIKeyDialog(APIKeyDialogMode mode)
    {
        if (m_isAPIKeyDialogOpen) return;

        // 재입력이면 기존 키를 채워서 보여 줌 (원본과 같음)
        std::wstring currentKey;
        if (mode == APIKeyDialogMode::API_KEY_CHANGE)
        {
            try
            {
                if (const std::optional<std::string> savedKey{ m_keyStore->Load() }) currentKey = Core::TextEncoding::UTF8ToWide(*savedKey);
            }
            catch (const std::exception&)
            {
                currentKey.clear();   // 읽을 수 없으면 빈칸으로 새로 입력
            }
        }

        m_isAPIKeyDialogOpen = true;
        m_notifier->OpenDialog(std::make_unique<APIKeyDialog>(*m_window, *m_notifier, mode, std::move(currentKey),
            [this](const std::string& apiKey) { return SaveAPIKey(apiKey); },
            [this, mode]
        {
            m_isAPIKeyDialogOpen = false;
            if (mode == APIKeyDialogMode::API_KEY_FIRST_RUN) m_window->RequestClose();   // 처음 실행에서 [종료] → 프로그램 종료 (원본과 같음)
        }));
    }

    bool Application::SaveAPIKey(const std::string& apiKey)
    {
        try
        {
            m_keyStore->Save(apiKey);
        }
        catch (const std::exception& error)
        {
            m_notifier->ShowMessage(std::wstring{ KEY_SAVE_ERROR_TITLE }, std::wstring{ KEY_SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
            return false;
        }

        m_marketAPI->SetAPIKey(apiKey);   // 이후 요청부터 새 키 사용 (세 툴 공용)
        m_hasAPIKey = true;
        m_isAPIKeyDialogOpen = false;
        m_notifier->ShowToast(std::wstring{ KEY_SAVED_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);

        // 키 오류로 멈춘 자동 갱신 등을 툴마다 다시 시작할 수 있게 알림 (초기화가 끝난 툴만)
        for (std::size_t index{}; index < m_initializedToolCount; ++index) m_tools[index].get().OnAPIKeyChanged();
        return true;
    }

    bool Application::HandleShortcut(const Core::KeyEvent& event)
    {
        if (!event.m_isCtrlDown) return false;

        // Ctrl+Tab: 다음 탭, Ctrl+Shift+Tab: 이전 탭 (끝에서 처음으로 돌아감)
        if (event.m_key == Core::Key::KEY_TAB)
        {
            const std::size_t step{ event.m_isShiftDown ? TOOL_COUNT - NEXT_TAB_STEP : NEXT_TAB_STEP };
            SelectTool((m_activeIndex + step) % TOOL_COUNT);
            return true;
        }

        // Ctrl+1 ~ Ctrl+3: 해당 탭으로 바로 이동 (Ctrl+A · C · V · X는 입력칸으로 보냄)
        const int tabIndex{ std::to_underlying(event.m_key) - std::to_underlying(Core::Key::KEY_1) };
        if (tabIndex >= 0 && static_cast<std::size_t>(tabIndex) < TOOL_COUNT)
        {
            SelectTool(static_cast<std::size_t>(tabIndex));
            return true;
        }
        return false;
    }

    // 탭 막대(위) · 탭 내용(아래)으로 입력을 나눠 줌 (키보드 · 글자 입력은 탭 내용으로)
    void Application::RouteInput(const Core::InputEvent& event)
    {
        Core::ToolManager& tool{ GetActiveTool() };

        std::visit(Core::Overloaded{
            [&](const Core::MouseMoveEvent& move)
            {
                m_isPointerInHeader = IsInHeader(move.m_y);

                // 누른 채 움직이면 누른 곳으로만 보냄
                if (m_pointerOwner == PointerOwner::POINTER_TOOL)
                {
                    static_cast<void>(tool.HandleInput(event));
                    return;
                }
                if (m_pointerOwner == PointerOwner::POINTER_HEADER)
                {
                    m_headerHost.HandleInput(event);
                    return;
                }

                m_hoverIndex = FindTabAt(move.m_x, move.m_y);
                m_headerHost.HandleInput(event);
                static_cast<void>(tool.HandleInput(m_isPointerInHeader ? Core::InputEvent{ Core::MouseLeaveEvent{} } : event));
            },
            [&](const Core::MouseLeaveEvent&)
            {
                m_isPointerInHeader = false;
                m_hoverIndex.reset();
                m_headerHost.HandleInput(event);
                static_cast<void>(tool.HandleInput(event));
            },
            [&](const Core::MouseButtonEvent& button) { RouteMouseButton(event, button); },
            [&](const Core::MouseWheelEvent& wheel)
            {
                if (IsInHeader(wheel.m_y)) m_headerHost.HandleInput(event);
                else static_cast<void>(tool.HandleInput(event));
            },
            [&](const auto&)
            {
                static_cast<void>(tool.HandleInput(event));   // 키 · 글자 · 한글 조합
            }
            }, event);
    }

    void Application::RouteMouseButton(const Core::InputEvent& event, const Core::MouseButtonEvent& button)
    {
        Core::ToolManager& tool{ GetActiveTool() };
        const bool isLeft{ button.m_button == Core::MouseButton::MOUSE_BUTTON_LEFT };

        if (button.m_isPressed)
        {
            const bool isHeader{ IsInHeader(button.m_y) };
            if (isLeft) m_pointerOwner = isHeader ? PointerOwner::POINTER_HEADER : PointerOwner::POINTER_TOOL;

            if (!isHeader)
            {
                static_cast<void>(tool.HandleInput(event));
                return;
            }

            if (const auto index{ FindTabAt(button.m_x, button.m_y) }; index.has_value() && isLeft) SelectTool(*index);
            m_headerHost.HandleInput(event);
            return;
        }

        // 뗄 때는 누른 곳으로 (누른 적이 없으면 마우스 위치 기준)
        PointerOwner owner{ isLeft ? m_pointerOwner : PointerOwner::POINTER_NONE };
        if (owner == PointerOwner::POINTER_NONE) owner = IsInHeader(button.m_y) ? PointerOwner::POINTER_HEADER : PointerOwner::POINTER_TOOL;
        if (isLeft) m_pointerOwner = PointerOwner::POINTER_NONE;

        if (owner == PointerOwner::POINTER_HEADER) m_headerHost.HandleInput(event);
        else static_cast<void>(tool.HandleInput(event));
    }

    // 대화상자가 열리고 닫힐 때 탭 화면의 입력 상태를 맞춤 (열리면 포커스 내려놓기, 닫히면 되돌리기)
    void Application::SyncModalState()
    {
        const bool isModal{ m_notifier->HasDialog() };
        if (isModal == m_isModal) return;

        m_isModal = isModal;
        m_pointerOwner = PointerOwner::POINTER_NONE;
        m_hoverIndex.reset();

        if (isModal)
        {
            m_headerHost.SuspendFocus();
            GetActiveTool().OnDeactivated();
        }
        else
        {
            GetActiveTool().OnActivated();
        }
    }

    // 마우스 위치 · 누른 곳에 맞는 커서 (글자 입력칸 = I자, 버튼 · 탭 = 손 모양)
    void Application::UpdateCursor()
    {
        Core::CursorShape shape{ Core::CursorShape::CURSOR_ARROW };
        if (m_isModal) shape = m_notifier->GetCursor();
        else if (m_pointerOwner == PointerOwner::POINTER_TOOL) shape = GetActiveTool().GetCursor();
        else if (m_pointerOwner == PointerOwner::POINTER_HEADER) shape = m_headerHost.GetCursor();
        else if (m_isPointerInHeader) shape = m_hoverIndex.has_value() ? Core::CursorShape::CURSOR_HAND : m_headerHost.GetCursor();
        else shape = GetActiveTool().GetCursor();

        m_window->SetCursor(shape);
    }

    void Application::Update(double deltaSeconds)
    {
        // 원격 설정 확인 결과 · 30분 주기 확인 (대화상자를 열 수 있으므로 알림 층 갱신보다 먼저)
        if (m_configTask.IsReady()) FinishServerCheck();
        if (m_configTimer.Poll()) StartServerCheck(false);

        m_notifier->Update(deltaSeconds);   // 닫힌 대화상자 정리 → 모달 상태 확인보다 먼저
        SyncModalState();

        // 탭 밑줄: 남은 거리의 일정 비율씩 다가감 (지수 감쇠 → 프레임 속도가 달라도 같은 빠르기)
        const float targetPosition{ static_cast<float>(m_activeIndex) };
        const float followRatio{ static_cast<float>(1.0 - std::exp(-INDICATOR_SMOOTHING * deltaSeconds)) };
        m_indicatorPosition += (targetPosition - m_indicatorPosition) * followRatio;
        if (std::abs(targetPosition - m_indicatorPosition) < INDICATOR_SNAP_DISTANCE) m_indicatorPosition = targetPosition;

        const float fadeStep{ static_cast<float>(deltaSeconds / CONTENT_FADE_SECONDS) };
        m_contentFade = std::max(NO_FADE, m_contentFade - fadeStep);

        m_headerHost.Update(deltaSeconds);

        // 모든 탭을 갱신 (다른 탭을 보거나 대화상자가 떠 있어도 백그라운드 작업 결과 · 자동 갱신 진행), 점검 중이면 멈춤
        if (!m_isMaintenanceLocked)
        {
            for (Core::ToolManager& tool : m_tools) tool.Update(deltaSeconds);
        }
        UpdateCursor();   // 대화상자가 닫히는 등 마우스를 움직이지 않아도 커서가 바뀔 수 있음
    }

    void Application::Render()
    {
        if (!m_renderer || !m_notifier || !m_renderer->BeginFrame()) return;

        m_renderer->Clear(Theme::BACKGROUND_COLOR);

        // 1) 선택한 탭 내용
        const Core::RectF contentRect{ GetContentRect() };
        GetActiveTool().Render(*m_renderer, contentRect);

        // 2) 탭 전환 직후: 내용 위에 배경색을 덮었다가 걷어 냄 (페이드 인)
        if (m_contentFade > NO_FADE)
        {
            m_renderer->FillRect(contentRect, Core::WithAlpha(Theme::BACKGROUND_COLOR, m_contentFade));
        }

        // 3) 탭 막대는 내용 위에 그려서 항상 보이게
        RenderTabBar();

        // 4) 대화상자 · 알림은 맨 위
        m_notifier->Render(*m_renderer, GetScreenRect());

        m_renderer->EndFrame();
    }

    void Application::RenderTabBar()
    {
        const float width{ m_renderer->GetWidth() };
        m_renderer->FillRect(Core::RectF{ 0.0f, 0.0f, width, TAB_BAR_HEIGHT }, Theme::HEADER_BACK_COLOR);

        for (std::size_t index{}; index < TOOL_COUNT; ++index)
        {
            const Core::RectF tabRect{ GetTabRect(index) };
            const bool isActive{ index == m_activeIndex };

            if (!isActive && m_hoverIndex == index)
            {
                m_renderer->FillRect(tabRect, Core::WithAlpha(Theme::SELECTION_BACK_COLOR, HOVER_ALPHA));
            }

            m_renderer->DrawString(m_tools[index].get().GetTabTitle(), tabRect,
                isActive ? Theme::HEADER_TEXT_COLOR : Theme::MUTED_TEXT_COLOR,
                isActive ? ACTIVE_TAB_STYLE : INACTIVE_TAB_STYLE);

            // 탭 오른쪽 구분선
            m_renderer->DrawLine(tabRect.m_right, tabRect.m_top + TAB_DIVIDER_MARGIN, tabRect.m_right, tabRect.m_bottom - TAB_DIVIDER_MARGIN,
                Theme::GRID_LINE_COLOR, LINE_WIDTH);
        }

        // 탭 막대 아래 경계선
        m_renderer->DrawLine(0.0f, TAB_BAR_HEIGHT, width, TAB_BAR_HEIGHT, Theme::GRID_LINE_COLOR, LINE_WIDTH);

        // 선택한 탭 밑줄 (m_indicatorPosition을 따라 미끄러지듯 이동)
        const float tabWidth{ GetTabWidth() };
        const float indicatorLeft{ m_indicatorPosition * tabWidth };
        m_renderer->FillRect(Core::RectF{ indicatorLeft, TAB_BAR_HEIGHT - TAB_INDICATOR_HEIGHT, indicatorLeft + tabWidth, TAB_BAR_HEIGHT },
            Theme::TOTAL_TEXT_COLOR);

        // 오른쪽 버튼 (창 너비가 바뀌면 따라 움직임)
        LayoutHeader();
        m_headerHost.Render(*m_renderer);
    }

    void Application::LayoutHeader()
    {
        const float right{ m_renderer->GetWidth() - HEADER_BUTTON_MARGIN };
        m_apiKeyButton->SetBounds(Core::RectF{ right - HEADER_BUTTON_WIDTH, HEADER_BUTTON_MARGIN, right, TAB_BAR_HEIGHT - HEADER_BUTTON_MARGIN });
    }

    Core::RectF Application::GetTabRect(std::size_t index) const noexcept
    {
        const float tabWidth{ GetTabWidth() };
        const float tabLeft{ static_cast<float>(index) * tabWidth };
        return Core::RectF{ tabLeft, 0.0f, tabLeft + tabWidth, TAB_BAR_HEIGHT };
    }

    // 오른쪽 버튼 자리를 뺀 너비를 탭 수로 나눔 (최대 TAB_WIDTH → 창이 넓으면 예전과 같은 크기)
    float Application::GetTabWidth() const noexcept
    {
        const float availableWidth{ m_renderer->GetWidth() - HEADER_BUTTON_WIDTH - HEADER_BUTTON_MARGIN * HEADER_MARGIN_SIDES };
        return std::clamp(availableWidth / static_cast<float>(TOOL_COUNT), 0.0f, TAB_WIDTH);
    }

    Core::RectF Application::GetContentRect() const noexcept
    {
        return Core::RectF{ 0.0f, TAB_BAR_HEIGHT, m_renderer->GetWidth(), m_renderer->GetHeight() };
    }

    Core::RectF Application::GetScreenRect() const noexcept
    {
        return Core::RectF{ 0.0f, 0.0f, m_renderer->GetWidth(), m_renderer->GetHeight() };
    }

    std::optional<std::size_t> Application::FindTabAt(float x, float y) const noexcept
    {
        for (std::size_t index{}; index < TOOL_COUNT; ++index)
        {
            if (GetTabRect(index).Contains(x, y)) return index;
        }
        return std::nullopt;
    }

    bool Application::IsInHeader(float y) noexcept
    {
        return y < TAB_BAR_HEIGHT;
    }

    Core::ToolManager& Application::GetActiveTool() const noexcept
    {
        return m_tools[m_activeIndex].get();   // m_activeIndex는 SelectTool에서 TOOL_COUNT 미만만 허용
    }
}