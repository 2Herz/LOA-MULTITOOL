module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>    // CoInitializeEx (링크 열기 ShellExecuteW 권장 사항)
#include <imm.h>        // 한글 입력기(IME) 조합 · 위치
#include <shellapi.h>   // ShellExecuteW (링크 열기) · Shell_NotifyIconW (트레이 아이콘) · ExtractIconExW (프로그램 아이콘)
#include <dwmapi.h>     // DwmSetWindowAttribute (어두운 제목 표시줄)

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "imm32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")

module LostArkMultiTool.Views.MainWindow;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr const wchar_t* WINDOW_CLASS_NAME{ L"LostArkMultiToolWindow" };
        constexpr DWORD WINDOW_STYLE{ WS_OVERLAPPEDWINDOW };
        constexpr DWORD WINDOW_EX_STYLE{ 0 };

        constexpr float DEFAULT_DPI{ 96.0f };            // 배율 100%일 때 DPI (DIP 기준)
        constexpr int MIN_WINDOW_WIDTH_DIP{ 960 };       // 계산기 도구 막대 · 표 · 그래프가 찌그러지지 않는 최소 크기
        constexpr int MIN_WINDOW_HEIGHT_DIP{ 640 };

        constexpr DWORD DARK_TITLE_BAR_ATTRIBUTE{ 20 };  // DWMWA_USE_IMMERSIVE_DARK_MODE (예전 SDK에는 이름이 없어 값으로 지정)

        constexpr int EXIT_CODE_NORMAL{ 0 };
        constexpr int EXIT_CODE_ERROR{ 1 };

        // ===== 입력 =====
        constexpr int SINGLE_CLICK_COUNT{ 1 };
        constexpr int DOUBLE_CLICK_COUNT{ 2 };
        constexpr float WHEEL_NOTCH{ static_cast<float>(WHEEL_DELTA) };   // 휠 한 칸 = 120
        constexpr wchar_t FIRST_PRINTABLE_CHARACTER{ L' ' };             // 0x20 미만은 제어 문자 (Ctrl+A = 0x01, Backspace = 0x08 등)
        constexpr wchar_t DELETE_CHARACTER{ 0x7F };                       // Ctrl+Backspace가 만드는 제어 문자
        constexpr WPARAM FIRST_DIGIT_KEY{ '1' };                          // 숫자 키의 가상 키 코드 = 문자 코드
        constexpr WPARAM LAST_DIGIT_KEY{ '9' };

        // ===== 커서 · 링크 =====
        constexpr std::size_t CURSOR_SHAPE_COUNT{ 3 };                    // Core::CursorShape 개수
        constexpr std::wstring_view HTTPS_PREFIX{ L"https://" };
        constexpr std::wstring_view HTTP_PREFIX{ L"http://" };
        constexpr INT_PTR SHELL_EXECUTE_ERROR_LIMIT{ 32 };                // ShellExecuteW 반환값이 32 이하면 실패

        // ===== 트레이 =====
        constexpr UINT TRAY_CALLBACK_MESSAGE{ WM_APP + 1 };               // 트레이 아이콘 마우스 알림을 받을 메시지
        constexpr UINT TRAY_ICON_ID{ 1 };
        constexpr UINT_PTR TRAY_MENU_OPEN{ 1 };                           // 트레이 메뉴 항목 번호 (0은 "아무것도 안 고름")
        constexpr UINT_PTR TRAY_MENU_CHANGE_API_KEY{ 2 };
        constexpr UINT_PTR TRAY_MENU_EXIT{ 3 };
        constexpr const wchar_t* TRAY_MENU_OPEN_TEXT{ L"열기" };
        constexpr const wchar_t* TRAY_MENU_CHANGE_API_KEY_TEXT{ L"API 키 변경" };
        constexpr const wchar_t* TRAY_MENU_EXIT_TEXT{ L"종료" };
        constexpr const wchar_t* TASKBAR_CREATED_MESSAGE_NAME{ L"TaskbarCreated" };   // 탐색기가 다시 시작되면 오는 알림 (아이콘 다시 등록)

        // ===== 프로그램 아이콘 =====
        constexpr int FIRST_ICON_INDEX{ 0 };                              // exe에 넣은 첫 번째 아이콘
        constexpr UINT ICON_EXTRACT_COUNT{ 1 };
        constexpr DWORD MAX_PATH_LENGTH{ 32768 };                         // Windows 긴 경로 최대 길이 (글자 수)

        // 고정 크기 Windows 문자 배열에 복사 (넘치면 잘라서 끝에 0 → 경계 검사)
        template <std::size_t Size>
        void CopyToFixedText(wchar_t(&destination)[Size], std::wstring_view source) noexcept
        {
            const std::size_t length{ std::min(source.size(), Size - 1) };
            std::ranges::copy(source.substr(0, length), destination);
            destination[length] = L'\0';
        }

        // 실행 파일 경로 (아이콘을 꺼낼 때 사용, 실패하면 빈 문자열)
        std::wstring GetExecutablePath()
        {
            std::wstring path(MAX_PATH, L'\0');
            while (path.size() <= MAX_PATH_LENGTH)
            {
                const DWORD length{ ::GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size())) };
                if (length == 0) return {};
                if (length < path.size())
                {
                    path.resize(length);
                    return path;
                }
                path.resize(path.size() * 2);   // 잘렸으면 버퍼를 늘려 다시 시도
            }
            return {};
        }

        // 알림 종류 → 트레이 풍선 아이콘
        DWORD ToBalloonIcon(Core::NoticeKind kind) noexcept
        {
            switch (kind)
            {
                case Core::NoticeKind::NOTICE_WARNING: return NIIF_WARNING;
                case Core::NoticeKind::NOTICE_ERROR:   return NIIF_ERROR;
                default:                               return NIIF_INFO;
            }
        }

        // DIP → 실제 픽셀
        int ScaleToPixels(int dip, float dpi) noexcept
        {
            return static_cast<int>(std::lround(static_cast<float>(dip) * dpi / DEFAULT_DPI));
        }

        // 마우스 메시지의 lParam → 좌표 (보조 모니터에서는 음수일 수 있어 부호 있는 16비트로 읽음)
        // windowsx.h의 GET_X_LPARAM과 같은 계산: windowsx.h는 IsMinimized 같은 매크로를 정의해 멤버 함수 이름과 충돌하므로 쓰지 않음
        int GetMouseX(LPARAM lParam) noexcept
        {
            return static_cast<short>(LOWORD(lParam));
        }

        int GetMouseY(LPARAM lParam) noexcept
        {
            return static_cast<short>(HIWORD(lParam));
        }

        // Windows 가상 키 → 화면 코드용 키 (쓰지 않는 키는 KEY_UNKNOWN → 보내지 않음)
        Core::Key TranslateKey(WPARAM virtualKey) noexcept
        {
            switch (virtualKey)
            {
                case VK_TAB:    return Core::Key::KEY_TAB;
                case VK_RETURN: return Core::Key::KEY_ENTER;
                case VK_ESCAPE: return Core::Key::KEY_ESCAPE;
                case VK_BACK:   return Core::Key::KEY_BACKSPACE;
                case VK_DELETE: return Core::Key::KEY_DELETE;
                case VK_LEFT:   return Core::Key::KEY_LEFT;
                case VK_RIGHT:  return Core::Key::KEY_RIGHT;
                case VK_UP:     return Core::Key::KEY_UP;
                case VK_DOWN:   return Core::Key::KEY_DOWN;
                case VK_HOME:   return Core::Key::KEY_HOME;
                case VK_END:    return Core::Key::KEY_END;
                case VK_PRIOR:  return Core::Key::KEY_PAGE_UP;
                case VK_NEXT:   return Core::Key::KEY_PAGE_DOWN;
                case VK_SPACE:  return Core::Key::KEY_SPACE;
                case 'A':       return Core::Key::KEY_A;
                case 'C':       return Core::Key::KEY_C;
                case 'V':       return Core::Key::KEY_V;
                case 'X':       return Core::Key::KEY_X;
                default:        break;
            }

            if (virtualKey >= FIRST_DIGIT_KEY && virtualKey <= LAST_DIGIT_KEY)
            {
                return static_cast<Core::Key>(std::to_underlying(Core::Key::KEY_1) + static_cast<int>(virtualKey - FIRST_DIGIT_KEY));
            }
            return Core::Key::KEY_UNKNOWN;
        }

        // ===== RAII 도우미 (얻은 자원은 범위를 벗어나면 자동으로 돌려줌) =====

        // COM 초기화 (ShellExecuteW가 셸 확장을 쓸 수 있어 권장됨)
        class COMApartment
        {
        public:
            COMApartment() noexcept
                : m_isInitialized{ SUCCEEDED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) }
            {}

            ~COMApartment()
            {
                if (m_isInitialized) ::CoUninitialize();
            }

            COMApartment(const COMApartment&) = delete;
            COMApartment& operator=(const COMApartment&) = delete;

        private:
            bool m_isInitialized{};
        };

        // 창의 IME 상태 (IME를 꺼 둔 상태면 비어 있음)
        class IMEContext
        {
        public:
            explicit IMEContext(HWND window) noexcept
                : m_window{ window }
                , m_context{ ::ImmGetContext(window) }
            {}

            ~IMEContext()
            {
                if (m_context != nullptr) ::ImmReleaseContext(m_window, m_context);
            }

            IMEContext(const IMEContext&) = delete;
            IMEContext& operator=(const IMEContext&) = delete;

            [[nodiscard]] HIMC Get() const noexcept
            {
                return m_context;
            }

            [[nodiscard]] explicit operator bool() const noexcept
            {
                return m_context != nullptr;
            }

        private:
            HWND m_window{};
            HIMC m_context{};
        };

        // 클립보드 열기 · 닫기
        class ClipboardSession
        {
        public:
            explicit ClipboardSession(HWND window) noexcept
                : m_isOpen{ ::OpenClipboard(window) != FALSE }
            {}

            ~ClipboardSession()
            {
                if (m_isOpen) ::CloseClipboard();
            }

            ClipboardSession(const ClipboardSession&) = delete;
            ClipboardSession& operator=(const ClipboardSession&) = delete;

            [[nodiscard]] bool IsOpen() const noexcept
            {
                return m_isOpen;
            }

        private:
            bool m_isOpen{};
        };

        // 전역 메모리 잠금 (GlobalLock · GlobalUnlock)
        class GlobalLockGuard
        {
        public:
            explicit GlobalLockGuard(HGLOBAL memory) noexcept
                : m_memory{ memory }
                , m_data{ memory != nullptr ? ::GlobalLock(memory) : nullptr }
            {}

            ~GlobalLockGuard()
            {
                if (m_data != nullptr) ::GlobalUnlock(m_memory);
            }

            GlobalLockGuard(const GlobalLockGuard&) = delete;
            GlobalLockGuard& operator=(const GlobalLockGuard&) = delete;

            [[nodiscard]] void* Get() const noexcept
            {
                return m_data;
            }

        private:
            HGLOBAL m_memory{};
            void* m_data{};
        };

        // 클립보드에 넘길 전역 메모리 (넘기기 전에 실패하면 자동 해제, 넘기면 시스템이 소유)
        class GlobalMemory
        {
        public:
            explicit GlobalMemory(std::size_t byteCount) noexcept
                : m_memory{ ::GlobalAlloc(GMEM_MOVEABLE, byteCount) }
            {}

            ~GlobalMemory()
            {
                if (m_memory != nullptr) ::GlobalFree(m_memory);
            }

            GlobalMemory(const GlobalMemory&) = delete;
            GlobalMemory& operator=(const GlobalMemory&) = delete;

            [[nodiscard]] HGLOBAL Get() const noexcept
            {
                return m_memory;
            }

            void Release() noexcept
            {
                m_memory = nullptr;
            }

        private:
            HGLOBAL m_memory{};
        };

        // 아이콘 (exe에서 꺼낸 아이콘은 직접 해제, 시스템 공용 아이콘은 해제하지 않음)
        class IconHandle
        {
        public:
            IconHandle() noexcept = default;

            IconHandle(HICON icon, bool isOwned) noexcept
                : m_icon{ icon }
                , m_isOwned{ isOwned }
            {}

            ~IconHandle()
            {
                Reset();
            }

            IconHandle(const IconHandle&) = delete;
            IconHandle& operator=(const IconHandle&) = delete;

            IconHandle(IconHandle&& other) noexcept
                : m_icon{ std::exchange(other.m_icon, nullptr) }
                , m_isOwned{ std::exchange(other.m_isOwned, false) }
            {}

            IconHandle& operator=(IconHandle&& other) noexcept
            {
                if (this != &other)
                {
                    Reset();
                    m_icon = std::exchange(other.m_icon, nullptr);
                    m_isOwned = std::exchange(other.m_isOwned, false);
                }
                return *this;
            }

            [[nodiscard]] HICON Get() const noexcept
            {
                return m_icon;
            }

        private:
            void Reset() noexcept
            {
                if (m_icon != nullptr && m_isOwned) ::DestroyIcon(m_icon);
                m_icon = nullptr;
                m_isOwned = false;
            }

            HICON m_icon{};
            bool m_isOwned{};
        };

        // 팝업 메뉴 (트레이 메뉴, 범위를 벗어나면 자동 해제)
        class PopupMenu
        {
        public:
            PopupMenu() noexcept
                : m_menu{ ::CreatePopupMenu() }
            {}

            ~PopupMenu()
            {
                if (m_menu != nullptr) ::DestroyMenu(m_menu);
            }

            PopupMenu(const PopupMenu&) = delete;
            PopupMenu& operator=(const PopupMenu&) = delete;

            [[nodiscard]] HMENU Get() const noexcept
            {
                return m_menu;
            }

        private:
            HMENU m_menu{};
        };

        // IME 조합 문자열 읽기 (GCS_RESULTSTR = 확정된 글자, GCS_COMPSTR = 조합 중인 글자)
        std::wstring ReadCompositionString(HIMC context, DWORD kind)
        {
            const LONG byteCount{ ::ImmGetCompositionStringW(context, kind, nullptr, 0) };
            if (byteCount <= 0) return {};

            std::wstring text(static_cast<std::size_t>(byteCount) / sizeof(wchar_t), L'\0');
            ::ImmGetCompositionStringW(context, kind, text.data(), static_cast<DWORD>(byteCount));
            return text;
        }
    }

    WindowEventHandler::~WindowEventHandler() = default;

    // ===== 실제 Win32 창 구현 =====
    class MainWindow::Impl
    {
    public:
        Impl(std::wstring_view title, int width, int height, WindowEventHandler& handler);
        ~Impl();

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        void Show(int showCommand);
        void RequestClose();
        [[nodiscard]] bool ProcessMessages();
        void WaitForMessage(int timeoutMilliseconds);

        [[nodiscard]] HWND GetHandle() const noexcept;
        [[nodiscard]] float GetDPI() const noexcept;
        [[nodiscard]] PixelSize GetClientPixelSize() const;
        [[nodiscard]] bool IsMinimized() const noexcept;
        [[nodiscard]] int GetExitCode() const noexcept;

        [[nodiscard]] std::wstring GetClipboardText();
        bool SetClipboardText(std::wstring_view text);
        void SetCursor(Core::CursorShape shape);
        void SetIMEEnabled(bool isEnabled);
        void SetIMEPosition(const Core::RectF& caretRect);
        void CancelComposition();
        [[nodiscard]] bool OpenURL(std::wstring_view url);
        [[nodiscard]] bool IsWindowShown() const;
        void RestoreWindow();
        void ShowTrayBalloon(std::wstring_view title, std::wstring_view message, Core::NoticeKind kind);
        void SetTrayTooltip(std::wstring_view text);

    private:
        // Windows가 부르는 창 프로시저 (C 콜백이라 static) → 창마다 저장해 둔 Impl로 넘김
        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

        void Emit(const Core::InputEvent& event);
        void EmitMouseButton(LPARAM lParam, Core::MouseButton button, bool isPressed, int clickCount);
        void HandleComposition(LPARAM flags);

        [[nodiscard]] float ToDIP(int pixels) const noexcept;
        [[nodiscard]] LONG ToPixels(float dip) const noexcept;
        void RethrowPendingException();
        void TrackMouseLeave();

        // ===== 트레이 =====
        void LoadProgramIcons();
        void AddTrayIcon();
        void RemoveTrayIcon() noexcept;
        void HideToTray();
        void HandleTrayMessage(LPARAM mouseMessage);
        void ShowTrayMenu();

        COMApartment m_comApartment;             // 가장 먼저 만들고 가장 나중에 해제
        WindowEventHandler& m_handler;
        HINSTANCE m_instance{};
        HWND m_handle{};
        float m_dpi{ DEFAULT_DPI };
        bool m_isMinimized{};
        bool m_isTrackingMouse{};
        bool m_isLeftButtonDown{};
        bool m_isIMEEnabled{ true };             // 창을 만들면 기본으로 켜져 있음 → 생성 직후 끔
        float m_lastMouseX{};
        float m_lastMouseY{};
        int m_exitCode{ EXIT_CODE_NORMAL };
        Core::CursorShape m_cursor{ Core::CursorShape::CURSOR_ARROW };
        std::array<HCURSOR, CURSOR_SHAPE_COUNT> m_cursors{};   // 시스템 공용 커서 (해제할 필요 없음)
        std::exception_ptr m_pendingException;   // 창 프로시저 안에서 난 예외 (C 콜백 밖으로 던질 수 없어 보관했다가 다시 던짐)

        // ===== 트레이 · 아이콘 =====
        IconHandle m_largeIcon;                  // 창 제목줄 · 작업 표시줄
        IconHandle m_smallIcon;                  // 트레이 · 제목줄 작은 아이콘
        UINT m_taskbarCreatedMessage{};          // 탐색기 재시작 알림 메시지 번호 (실행 중에 정해짐)
        bool m_isTrayIconAdded{};
        bool m_isHiddenInTray{};
        std::wstring m_trayTooltip;              // 탐색기가 다시 시작돼도 같은 글자로 다시 등록
    };

    MainWindow::Impl::Impl(std::wstring_view title, int width, int height, WindowEventHandler& handler)
        : m_handler{ handler }
        , m_instance{ ::GetModuleHandleW(nullptr) }
    {
        // 모니터마다 배율이 달라도 선명하게 (이미 설정돼 있으면 실패하지만 문제 없음)
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        m_cursors[std::to_underlying(Core::CursorShape::CURSOR_ARROW)] = ::LoadCursorW(nullptr, IDC_ARROW);
        m_cursors[std::to_underlying(Core::CursorShape::CURSOR_HAND)] = ::LoadCursorW(nullptr, IDC_HAND);
        m_cursors[std::to_underlying(Core::CursorShape::CURSOR_TEXT)] = ::LoadCursorW(nullptr, IDC_IBEAM);
        LoadProgramIcons();
        m_taskbarCreatedMessage = ::RegisterWindowMessageW(TASKBAR_CREATED_MESSAGE_NAME);

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;   // CS_DBLCLKS: 더블 클릭 메시지 받기
        windowClass.lpfnWndProc = &Impl::WindowProc;
        windowClass.hInstance = m_instance;
        windowClass.hCursor = m_cursors[std::to_underlying(Core::CursorShape::CURSOR_ARROW)];
        windowClass.hIcon = m_largeIcon.Get();
        windowClass.hIconSm = m_smallIcon.Get();
        windowClass.lpszClassName = WINDOW_CLASS_NAME;
        if (::RegisterClassExW(&windowClass) == 0 && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            throw std::runtime_error{ "창 클래스를 등록하지 못했어요!" };
        }

        const std::wstring titleText{ title };
        m_trayTooltip = titleText;
        ::CreateWindowExW(WINDOW_EX_STYLE, WINDOW_CLASS_NAME, titleText.c_str(), WINDOW_STYLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, m_instance, this);
        if (m_handle == nullptr) throw std::runtime_error{ "창을 만들지 못했어요!" };   // m_handle은 WM_NCCREATE에서 채워짐

        // 창이 놓인 모니터 배율에 맞춰 안쪽 크기를 DIP → 픽셀로 맞춤
        m_dpi = static_cast<float>(::GetDpiForWindow(m_handle));
        RECT windowRect{ 0, 0, ScaleToPixels(width, m_dpi), ScaleToPixels(height, m_dpi) };
        ::AdjustWindowRectExForDpi(&windowRect, WINDOW_STYLE, FALSE, WINDOW_EX_STYLE, static_cast<UINT>(m_dpi));
        ::SetWindowPos(m_handle, nullptr, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        // 다크 테마에 맞춰 제목 표시줄도 어둡게 (지원하지 않는 Windows에서는 무시됨)
        const BOOL useDarkTitleBar{ TRUE };
        ::DwmSetWindowAttribute(m_handle, DARK_TITLE_BAR_ATTRIBUTE, &useDarkTitleBar, sizeof(useDarkTitleBar));

        // 글자 입력칸에 포커스가 없을 때는 IME를 꺼 둠 (한글 모드에서도 단축키가 그대로 동작)
        SetIMEEnabled(false);

        // 트레이 아이콘은 처음부터 표시 (원본과 같음, 마우스를 올리면 원정대 자산 표시)
        AddTrayIcon();
    }

    MainWindow::Impl::~Impl()
    {
        RemoveTrayIcon();   // 종료 후 트레이에 유령 아이콘이 남지 않게
        if (m_handle != nullptr) ::DestroyWindow(m_handle);   // 이미 닫힌 창이면 WM_NCDESTROY에서 nullptr이 됨
        ::UnregisterClassW(WINDOW_CLASS_NAME, m_instance);
    }

    void MainWindow::Impl::Show(int showCommand)
    {
        ::ShowWindow(m_handle, showCommand);
        ::UpdateWindow(m_handle);
    }

    void MainWindow::Impl::RequestClose()
    {
        if (m_handle != nullptr) ::PostMessageW(m_handle, WM_CLOSE, 0, 0);
    }

    bool MainWindow::Impl::ProcessMessages()
    {
        MSG message{};
        while (::PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                m_exitCode = static_cast<int>(message.wParam);
                RethrowPendingException();
                return false;
            }

            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }

        RethrowPendingException();
        return true;
    }

    void MainWindow::Impl::WaitForMessage(int timeoutMilliseconds)
    {
        // 메시지가 오면 바로, 안 오면 시간이 지나면 깨어남 (WaitMessage와 달리 백그라운드 작업 결과를 주기적으로 확인 가능)
        ::MsgWaitForMultipleObjectsEx(0, nullptr, static_cast<DWORD>(std::max(timeoutMilliseconds, 0)), QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    }

    HWND MainWindow::Impl::GetHandle() const noexcept
    {
        return m_handle;
    }

    float MainWindow::Impl::GetDPI() const noexcept
    {
        return m_dpi;
    }

    PixelSize MainWindow::Impl::GetClientPixelSize() const
    {
        RECT clientRect{};
        ::GetClientRect(m_handle, &clientRect);
        return PixelSize{ clientRect.right - clientRect.left, clientRect.bottom - clientRect.top };
    }

    bool MainWindow::Impl::IsMinimized() const noexcept
    {
        return m_isMinimized;
    }

    int MainWindow::Impl::GetExitCode() const noexcept
    {
        return m_exitCode;
    }

    std::wstring MainWindow::Impl::GetClipboardText()
    {
        const ClipboardSession session{ m_handle };
        if (!session.IsOpen()) return {};

        const HANDLE data{ ::GetClipboardData(CF_UNICODETEXT) };
        if (data == nullptr) return {};

        const GlobalLockGuard lock{ data };
        const auto* text{ static_cast<const wchar_t*>(lock.Get()) };
        if (text == nullptr) return {};

        // 메모리 크기 안에서만 읽음 (끝에 0이 없는 잘못된 데이터라도 넘쳐 읽지 않게 → 경계 검사)
        const std::wstring_view raw{ text, ::GlobalSize(data) / sizeof(wchar_t) };
        return std::wstring{ raw.substr(0, raw.find(L'\0')) };
    }

    bool MainWindow::Impl::SetClipboardText(std::wstring_view text)
    {
        const ClipboardSession session{ m_handle };
        if (!session.IsOpen() || !::EmptyClipboard()) return false;

        GlobalMemory memory{ (text.size() + 1) * sizeof(wchar_t) };   // + 1: 끝의 0
        if (memory.Get() == nullptr) return false;

        {
            const GlobalLockGuard lock{ memory.Get() };
            auto* destination{ static_cast<wchar_t*>(lock.Get()) };
            if (destination == nullptr) return false;

            std::ranges::copy(text, destination);
            destination[text.size()] = L'\0';
        }

        if (::SetClipboardData(CF_UNICODETEXT, memory.Get()) == nullptr) return false;
        memory.Release();   // 성공하면 메모리는 클립보드(시스템)가 소유
        return true;
    }

    void MainWindow::Impl::SetCursor(Core::CursorShape shape)
    {
        if (shape == m_cursor) return;

        m_cursor = shape;
        ::SetCursor(m_cursors[std::to_underlying(shape)]);
    }

    void MainWindow::Impl::SetIMEEnabled(bool isEnabled)
    {
        if (isEnabled == m_isIMEEnabled || m_handle == nullptr) return;

        // 기본 IME 연결 · 해제 (다시 켜도 한/영 상태는 그대로 유지됨)
        ::ImmAssociateContextEx(m_handle, nullptr, isEnabled ? IACE_DEFAULT : 0);
        m_isIMEEnabled = isEnabled;
    }

    void MainWindow::Impl::SetIMEPosition(const Core::RectF& caretRect)
    {
        const IMEContext context{ m_handle };
        if (!context) return;

        const LONG left{ ToPixels(caretRect.m_left) };
        const LONG top{ ToPixels(caretRect.m_top) };
        const LONG right{ ToPixels(caretRect.m_right) };
        const LONG bottom{ ToPixels(caretRect.m_bottom) };

        COMPOSITIONFORM composition{};
        composition.dwStyle = CFS_FORCE_POSITION;
        composition.ptCurrentPos = POINT{ left, top };
        ::ImmSetCompositionWindow(context.Get(), &composition);

        // 한자 변환 후보 창은 커서 아래에 (커서 영역은 가리지 않게)
        CANDIDATEFORM candidate{};
        candidate.dwIndex = 0;
        candidate.dwStyle = CFS_EXCLUDE;
        candidate.ptCurrentPos = POINT{ left, bottom };
        candidate.rcArea = RECT{ left, top, right, bottom };
        ::ImmSetCandidateWindow(context.Get(), &candidate);
    }

    void MainWindow::Impl::CancelComposition()
    {
        const IMEContext context{ m_handle };
        if (context) ::ImmNotifyIME(context.Get(), NI_COMPOSITIONSTR, CPS_CANCEL, 0);
    }

    bool MainWindow::Impl::OpenURL(std::wstring_view url)
    {
        // 웹 주소만 열기 (파일 · 프로그램 실행 방지)
        if (!url.starts_with(HTTPS_PREFIX) && !url.starts_with(HTTP_PREFIX)) return false;

        const std::wstring target{ url };
        const auto result{ reinterpret_cast<INT_PTR>(::ShellExecuteW(m_handle, L"open", target.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) };
        return result > SHELL_EXECUTE_ERROR_LIMIT;
    }

    bool MainWindow::Impl::IsWindowShown() const
    {
        return m_handle != nullptr && !m_isHiddenInTray && ::IsWindowVisible(m_handle) != FALSE && ::IsIconic(m_handle) == FALSE;
    }

    void MainWindow::Impl::RestoreWindow()
    {
        if (m_handle == nullptr) return;

        // 숨었거나 최소화된 창은 다시 보이게 하면서 원래 크기로 (보이는 창이면 앞으로만 가져옴)
        if (m_isHiddenInTray || ::IsIconic(m_handle) != FALSE || ::IsWindowVisible(m_handle) == FALSE) ::ShowWindow(m_handle, SW_RESTORE);
        m_isHiddenInTray = false;
        ::SetForegroundWindow(m_handle);
    }

    void MainWindow::Impl::ShowTrayBalloon(std::wstring_view title, std::wstring_view message, Core::NoticeKind kind)
    {
        if (!m_isTrayIconAdded) return;

        NOTIFYICONDATAW trayData{};
        trayData.cbSize = sizeof(NOTIFYICONDATAW);
        trayData.hWnd = m_handle;
        trayData.uID = TRAY_ICON_ID;
        trayData.uFlags = NIF_INFO;
        trayData.dwInfoFlags = ToBalloonIcon(kind);
        CopyToFixedText(trayData.szInfoTitle, title);
        CopyToFixedText(trayData.szInfo, message);
        ::Shell_NotifyIconW(NIM_MODIFY, &trayData);
    }

    void MainWindow::Impl::SetTrayTooltip(std::wstring_view text)
    {
        if (text == m_trayTooltip) return;   // 같은 글자면 다시 보내지 않음 (매번 갱신되는 자산 표시)
        m_trayTooltip = text;
        if (!m_isTrayIconAdded) return;

        NOTIFYICONDATAW trayData{};
        trayData.cbSize = sizeof(NOTIFYICONDATAW);
        trayData.hWnd = m_handle;
        trayData.uID = TRAY_ICON_ID;
        trayData.uFlags = NIF_TIP;
        CopyToFixedText(trayData.szTip, m_trayTooltip);
        ::Shell_NotifyIconW(NIM_MODIFY, &trayData);
    }

    // exe에 넣은 아이콘(리소스)을 창 · 트레이에 사용, 없으면 Windows 기본 프로그램 아이콘 (원본 APPIcon과 같은 순서)
    void MainWindow::Impl::LoadProgramIcons()
    {
        const std::wstring executablePath{ GetExecutablePath() };
        if (!executablePath.empty())
        {
            HICON largeIcon{};
            HICON smallIcon{};
            if (::ExtractIconExW(executablePath.c_str(), FIRST_ICON_INDEX, &largeIcon, &smallIcon, ICON_EXTRACT_COUNT) > 0)
            {
                m_largeIcon = IconHandle{ largeIcon, true };
                m_smallIcon = IconHandle{ smallIcon, true };
            }
        }

        if (m_largeIcon.Get() == nullptr) m_largeIcon = IconHandle{ ::LoadIconW(nullptr, IDI_APPLICATION), false };
        if (m_smallIcon.Get() == nullptr) m_smallIcon = IconHandle{ ::LoadIconW(nullptr, IDI_APPLICATION), false };
    }

    void MainWindow::Impl::AddTrayIcon()
    {
        if (m_handle == nullptr) return;

        NOTIFYICONDATAW trayData{};
        trayData.cbSize = sizeof(NOTIFYICONDATAW);
        trayData.hWnd = m_handle;
        trayData.uID = TRAY_ICON_ID;
        trayData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        trayData.uCallbackMessage = TRAY_CALLBACK_MESSAGE;
        trayData.hIcon = m_smallIcon.Get();
        CopyToFixedText(trayData.szTip, m_trayTooltip);
        m_isTrayIconAdded = (::Shell_NotifyIconW(NIM_ADD, &trayData) != FALSE);
    }

    void MainWindow::Impl::RemoveTrayIcon() noexcept
    {
        if (!m_isTrayIconAdded || m_handle == nullptr) return;

        NOTIFYICONDATAW trayData{};
        trayData.cbSize = sizeof(NOTIFYICONDATAW);
        trayData.hWnd = m_handle;
        trayData.uID = TRAY_ICON_ID;
        ::Shell_NotifyIconW(NIM_DELETE, &trayData);
        m_isTrayIconAdded = false;
    }

    // 최소화하면 작업 표시줄에서 사라지고 트레이에만 남음 (원본 Form1_Resize와 같음)
    void MainWindow::Impl::HideToTray()
    {
        if (m_isHiddenInTray || !m_isTrayIconAdded) return;   // 트레이 아이콘이 없으면 숨기지 않음 (다시 열 방법이 없어지므로)

        m_isHiddenInTray = true;
        ::ShowWindow(m_handle, SW_HIDE);
        m_handler.OnHiddenToTray();
    }

    void MainWindow::Impl::HandleTrayMessage(LPARAM mouseMessage)
    {
        switch (static_cast<UINT>(mouseMessage))
        {
            case WM_LBUTTONDBLCLK:
                m_handler.OnTrayCommand(TrayCommand::TRAY_OPEN);
                break;

            case WM_RBUTTONUP:
                ShowTrayMenu();
                break;

            default:
                break;
        }
    }

    void MainWindow::Impl::ShowTrayMenu()
    {
        const PopupMenu menu;
        if (menu.Get() == nullptr) return;

        ::AppendMenuW(menu.Get(), MF_STRING, TRAY_MENU_OPEN, TRAY_MENU_OPEN_TEXT);
        ::AppendMenuW(menu.Get(), MF_STRING, TRAY_MENU_CHANGE_API_KEY, TRAY_MENU_CHANGE_API_KEY_TEXT);
        ::AppendMenuW(menu.Get(), MF_SEPARATOR, 0, nullptr);
        ::AppendMenuW(menu.Get(), MF_STRING, TRAY_MENU_EXIT, TRAY_MENU_EXIT_TEXT);

        POINT cursor{};
        ::GetCursorPos(&cursor);

        // 메뉴 밖을 누르면 닫히도록 창을 앞으로 가져온 뒤 띄움 (트레이 메뉴의 정해진 순서)
        ::SetForegroundWindow(m_handle);
        const auto selected{ static_cast<UINT_PTR>(::TrackPopupMenu(menu.Get(), TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
            cursor.x, cursor.y, 0, m_handle, nullptr)) };
        ::PostMessageW(m_handle, WM_NULL, 0, 0);

        switch (selected)
        {
            case TRAY_MENU_OPEN:
                m_handler.OnTrayCommand(TrayCommand::TRAY_OPEN);
                break;

            case TRAY_MENU_CHANGE_API_KEY:
                m_handler.OnTrayCommand(TrayCommand::TRAY_CHANGE_API_KEY);
                break;

            case TRAY_MENU_EXIT:
                m_handler.OnTrayCommand(TrayCommand::TRAY_EXIT);
                break;

            default:
                break;   // 메뉴 밖을 눌러 닫음
        }
    }

    LRESULT CALLBACK MainWindow::Impl::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // 창이 만들어질 때 CreateWindowExW에 넘긴 this를 창에 붙여 둠
        if (message == WM_NCCREATE)
        {
            const auto* createInfo{ reinterpret_cast<const CREATESTRUCTW*>(lParam) };
            auto* owner{ static_cast<Impl*>(createInfo->lpCreateParams) };
            owner->m_handle = window;
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(owner));
        }

        auto* owner{ reinterpret_cast<Impl*>(::GetWindowLongPtrW(window, GWLP_USERDATA)) };
        if (owner == nullptr) return ::DefWindowProcW(window, message, wParam, lParam);

        try
        {
            return owner->HandleMessage(window, message, wParam, lParam);
        }
        catch (...)
        {
            // C 콜백 밖으로 예외를 던질 수 없으므로 보관하고 메시지 루프를 끝냄 → ProcessMessages에서 다시 던짐
            owner->m_pendingException = std::current_exception();
            ::PostQuitMessage(EXIT_CODE_ERROR);
            return 0;
        }
    }

    LRESULT MainWindow::Impl::HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        // 탐색기가 다시 시작되면 트레이 아이콘이 사라지므로 다시 등록 (메시지 번호가 실행 중에 정해져서 switch 밖에서 확인)
        if (m_taskbarCreatedMessage != 0 && message == m_taskbarCreatedMessage)
        {
            m_isTrayIconAdded = false;
            AddTrayIcon();
            return 0;
        }

        switch (message)
        {
            case WM_SIZE:
                m_isMinimized = (wParam == SIZE_MINIMIZED);
                if (m_isMinimized)
                {
                    HideToTray();
                }
                else
                {
                    m_isHiddenInTray = false;
                    m_handler.OnResize(LOWORD(lParam), HIWORD(lParam));
                }
                return 0;

            case TRAY_CALLBACK_MESSAGE:
                HandleTrayMessage(lParam);
                return 0;

            case WM_DPICHANGED:
            {
                // 렌더러가 새 배율을 먼저 알아야 이어지는 WM_SIZE에서 올바른 크기로 다시 만듦
                m_dpi = static_cast<float>(HIWORD(wParam));
                m_handler.OnDPIChanged(m_dpi);

                const auto* suggested{ reinterpret_cast<const RECT*>(lParam) };
                ::SetWindowPos(window, nullptr, suggested->left, suggested->top,
                    suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
                return 0;
            }

            case WM_GETMINMAXINFO:
            {
                auto* sizeInfo{ reinterpret_cast<MINMAXINFO*>(lParam) };
                sizeInfo->ptMinTrackSize.x = ScaleToPixels(MIN_WINDOW_WIDTH_DIP, m_dpi);
                sizeInfo->ptMinTrackSize.y = ScaleToPixels(MIN_WINDOW_HEIGHT_DIP, m_dpi);
                return 0;
            }

            case WM_SETCURSOR:
                // 창 안쪽이면 화면 부품이 정한 커서 (테두리 · 제목 표시줄은 Windows 기본 처리)
                if (LOWORD(lParam) == HTCLIENT)
                {
                    ::SetCursor(m_cursors[std::to_underlying(m_cursor)]);
                    return TRUE;
                }
                break;

            case WM_MOUSEMOVE:
                TrackMouseLeave();
                m_lastMouseX = ToDIP(GetMouseX(lParam));
                m_lastMouseY = ToDIP(GetMouseY(lParam));
                Emit(Core::MouseMoveEvent{ m_lastMouseX, m_lastMouseY });
                return 0;

            case WM_MOUSELEAVE:
                m_isTrackingMouse = false;
                Emit(Core::MouseLeaveEvent{});
                return 0;

            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK:
                // 누른 채 창 밖으로 끌어도 뗄 때까지 메시지를 받도록 마우스를 붙잡음 (입력칸 드래그 선택 등)
                m_isLeftButtonDown = true;
                ::SetCapture(window);
                EmitMouseButton(lParam, Core::MouseButton::MOUSE_BUTTON_LEFT, true,
                    message == WM_LBUTTONDBLCLK ? DOUBLE_CLICK_COUNT : SINGLE_CLICK_COUNT);
                return 0;

            case WM_LBUTTONUP:
                if (m_isLeftButtonDown)
                {
                    m_isLeftButtonDown = false;   // ReleaseCapture가 부르는 WM_CAPTURECHANGED에서 한 번 더 보내지 않게 먼저 내림
                    EmitMouseButton(lParam, Core::MouseButton::MOUSE_BUTTON_LEFT, false, SINGLE_CLICK_COUNT);
                }
                ::ReleaseCapture();
                return 0;

            case WM_CAPTURECHANGED:
                // 누른 상태에서 다른 창이 마우스를 가져감 (Alt+Tab 등) → 누름 상태가 남지 않게 뗀 것으로 처리
                if (m_isLeftButtonDown && reinterpret_cast<HWND>(lParam) != window)
                {
                    m_isLeftButtonDown = false;
                    Emit(Core::MouseButtonEvent{ m_lastMouseX, m_lastMouseY, Core::MouseButton::MOUSE_BUTTON_LEFT, false, SINGLE_CLICK_COUNT });
                }
                return 0;

            case WM_RBUTTONDOWN:
                EmitMouseButton(lParam, Core::MouseButton::MOUSE_BUTTON_RIGHT, true, SINGLE_CLICK_COUNT);
                return 0;

            case WM_RBUTTONUP:
                EmitMouseButton(lParam, Core::MouseButton::MOUSE_BUTTON_RIGHT, false, SINGLE_CLICK_COUNT);
                return 0;

            case WM_MOUSEWHEEL:
            {
                // 휠 메시지의 좌표는 화면 기준 → 창 안쪽 기준으로 바꿈
                POINT point{ GetMouseX(lParam), GetMouseY(lParam) };
                ::ScreenToClient(window, &point);
                const float notches{ static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_NOTCH };
                Emit(Core::MouseWheelEvent{ ToDIP(point.x), ToDIP(point.y), notches });
                return 0;
            }

            case WM_KEYDOWN:
            {
                const Core::Key key{ TranslateKey(wParam) };
                if (key != Core::Key::KEY_UNKNOWN)
                {
                    Emit(Core::KeyEvent{ key, ::GetKeyState(VK_CONTROL) < 0, ::GetKeyState(VK_SHIFT) < 0 });
                }
                return 0;
            }

            case WM_CHAR:
            {
                // 제어 문자(Ctrl+A · Backspace · Enter 등)는 KeyEvent로 따로 처리하므로 글자로 보내지 않음
                const auto character{ static_cast<wchar_t>(wParam) };
                if (character >= FIRST_PRINTABLE_CHARACTER && character != DELETE_CHARACTER)
                {
                    Emit(Core::TextInputEvent{ std::wstring(1, character) });
                }
                return 0;
            }

            case WM_IME_SETCONTEXT:
                // 기본 조합 창은 숨김 (조합 중인 글자는 입력칸 안에 직접 그림)
                lParam &= ~static_cast<LPARAM>(ISC_SHOWUICOMPOSITIONWINDOW);
                return ::DefWindowProcW(window, message, wParam, lParam);

            case WM_IME_STARTCOMPOSITION:
                return 0;   // 기본 조합 창을 띄우지 않음

            case WM_IME_COMPOSITION:
                HandleComposition(lParam);
                return 0;   // 직접 처리했으므로 WM_IME_CHAR · WM_CHAR가 한 번 더 오지 않게 기본 처리를 건너뜀

            case WM_IME_ENDCOMPOSITION:
                Emit(Core::CompositionEvent{});
                return 0;

            case WM_ERASEBKGND:
                return 1;   // 배경은 DX11이 그리므로 지우지 않음 (깜빡임 방지)

            case WM_DESTROY:
                RemoveTrayIcon();
                ::PostQuitMessage(EXIT_CODE_NORMAL);
                return 0;

            case WM_NCDESTROY:
                // 창이 완전히 사라짐 → 소멸자에서 다시 없애지 않도록 연결을 끊음
                ::SetWindowLongPtrW(window, GWLP_USERDATA, 0);
                m_handle = nullptr;
                break;

            default:
                break;
        }
        return ::DefWindowProcW(window, message, wParam, lParam);
    }

    void MainWindow::Impl::Emit(const Core::InputEvent& event)
    {
        m_handler.OnInput(event);
    }

    void MainWindow::Impl::EmitMouseButton(LPARAM lParam, Core::MouseButton button, bool isPressed, int clickCount)
    {
        m_lastMouseX = ToDIP(GetMouseX(lParam));
        m_lastMouseY = ToDIP(GetMouseY(lParam));
        Emit(Core::MouseButtonEvent{ m_lastMouseX, m_lastMouseY, button, isPressed, clickCount });
    }

    // 한글 조합: 확정된 글자 → TextInputEvent, 조합 중인 글자 → CompositionEvent
    // (다음 글자를 치기 시작하면 "확정 + 새 조합"이 메시지 하나로 함께 옴 → 확정을 먼저 보냄)
    void MainWindow::Impl::HandleComposition(LPARAM flags)
    {
        const IMEContext context{ m_handle };
        if (!context) return;

        const bool hasResult{ (flags & GCS_RESULTSTR) != 0 };
        const bool hasComposition{ (flags & GCS_COMPSTR) != 0 };

        if (hasResult)
        {
            std::wstring result{ ReadCompositionString(context.Get(), GCS_RESULTSTR) };
            if (!result.empty()) Emit(Core::TextInputEvent{ std::move(result) });
        }

        if (hasComposition)
        {
            Emit(Core::CompositionEvent{ ReadCompositionString(context.Get(), GCS_COMPSTR) });
        }
        else if (!hasResult)
        {
            Emit(Core::CompositionEvent{});   // 둘 다 없으면 조합 취소
        }
    }

    float MainWindow::Impl::ToDIP(int pixels) const noexcept
    {
        return static_cast<float>(pixels) * DEFAULT_DPI / m_dpi;
    }

    LONG MainWindow::Impl::ToPixels(float dip) const noexcept
    {
        return static_cast<LONG>(std::lround(dip * m_dpi / DEFAULT_DPI));
    }

    void MainWindow::Impl::RethrowPendingException()
    {
        if (m_pendingException) std::rethrow_exception(std::exchange(m_pendingException, nullptr));
    }

    // 마우스가 창 밖으로 나갈 때 WM_MOUSELEAVE를 한 번 받도록 등록 (탭 · 버튼 강조 해제용)
    void MainWindow::Impl::TrackMouseLeave()
    {
        if (m_isTrackingMouse) return;

        TRACKMOUSEEVENT trackInfo{};
        trackInfo.cbSize = sizeof(TRACKMOUSEEVENT);
        trackInfo.dwFlags = TME_LEAVE;
        trackInfo.hwndTrack = m_handle;
        m_isTrackingMouse = (::TrackMouseEvent(&trackInfo) != FALSE);
    }

    // ===== 공개 인터페이스 (구현은 Impl에 위임) =====
    MainWindow::MainWindow(std::wstring_view title, int width, int height, WindowEventHandler& handler)
        : m_impl{ std::make_unique<Impl>(title, width, height, handler) }
    {}

    MainWindow::~MainWindow() = default;

    void MainWindow::Show(int showCommand)
    {
        m_impl->Show(showCommand);
    }

    void MainWindow::RequestClose()
    {
        m_impl->RequestClose();
    }

    bool MainWindow::ProcessMessages()
    {
        return m_impl->ProcessMessages();
    }

    void MainWindow::WaitForMessage(int timeoutMilliseconds)
    {
        m_impl->WaitForMessage(timeoutMilliseconds);
    }

    void* MainWindow::GetNativeHandle() const noexcept
    {
        return m_impl->GetHandle();
    }

    float MainWindow::GetDPI() const noexcept
    {
        return m_impl->GetDPI();
    }

    PixelSize MainWindow::GetClientPixelSize() const
    {
        return m_impl->GetClientPixelSize();
    }

    bool MainWindow::IsMinimized() const noexcept
    {
        return m_impl->IsMinimized();
    }

    int MainWindow::GetExitCode() const noexcept
    {
        return m_impl->GetExitCode();
    }

    std::wstring MainWindow::GetClipboardText()
    {
        return m_impl->GetClipboardText();
    }

    bool MainWindow::SetClipboardText(std::wstring_view text)
    {
        return m_impl->SetClipboardText(text);
    }

    void MainWindow::SetCursor(Core::CursorShape shape)
    {
        m_impl->SetCursor(shape);
    }

    void MainWindow::SetIMEEnabled(bool isEnabled)
    {
        m_impl->SetIMEEnabled(isEnabled);
    }

    void MainWindow::SetIMEPosition(const Core::RectF& caretRect)
    {
        m_impl->SetIMEPosition(caretRect);
    }

    void MainWindow::CancelComposition()
    {
        m_impl->CancelComposition();
    }

    bool MainWindow::OpenURL(std::wstring_view url)
    {
        return m_impl->OpenURL(url);
    }

    bool MainWindow::IsWindowShown() const
    {
        return m_impl->IsWindowShown();
    }

    void MainWindow::RestoreWindow()
    {
        m_impl->RestoreWindow();
    }

    void MainWindow::ShowTrayBalloon(std::wstring_view title, std::wstring_view message, Core::NoticeKind kind)
    {
        m_impl->ShowTrayBalloon(title, message, kind);
    }

    void MainWindow::SetTrayTooltip(std::wstring_view text)
    {
        m_impl->SetTrayTooltip(text);
    }
}