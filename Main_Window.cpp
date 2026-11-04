module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dwmapi.h>     // DwmSetWindowAttribute (어두운 제목 표시줄)

#pragma comment(lib, "dwmapi.lib")

module LostArkMultiTool.Views.MainWindow;

import std;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr const wchar_t* WINDOW_CLASS_NAME{ L"LostArkMultiToolWindow" };
        constexpr DWORD WINDOW_STYLE{ WS_OVERLAPPEDWINDOW };
        constexpr DWORD WINDOW_EX_STYLE{ 0 };

        constexpr float DEFAULT_DPI{ 96.0f };            // 배율 100%일 때 DPI (DIP 기준)
        constexpr int MIN_WINDOW_WIDTH_DIP{ 800 };       // 표 · 그래프가 찌그러지지 않는 최소 크기
        constexpr int MIN_WINDOW_HEIGHT_DIP{ 500 };

        constexpr DWORD DARK_TITLE_BAR_ATTRIBUTE{ 20 };  // DWMWA_USE_IMMERSIVE_DARK_MODE (예전 SDK에는 이름이 없어 값으로 지정)

        constexpr int EXIT_CODE_NORMAL{ 0 };
        constexpr int EXIT_CODE_ERROR{ 1 };

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
        [[nodiscard]] bool ProcessMessages();
        void WaitForMessage();

        [[nodiscard]] HWND GetHandle() const noexcept;
        [[nodiscard]] float GetDPI() const noexcept;
        [[nodiscard]] PixelSize GetClientPixelSize() const;
        [[nodiscard]] bool IsMinimized() const noexcept;
        [[nodiscard]] int GetExitCode() const noexcept;

    private:
        // Windows가 부르는 창 프로시저 (C 콜백이라 static) → 창마다 저장해 둔 Impl로 넘김
        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
        LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

        [[nodiscard]] float ToDIP(int pixels) const noexcept;
        void RethrowPendingException();
        void TrackMouseLeave();

        WindowEventHandler& m_handler;
        HINSTANCE m_instance{};
        HWND m_handle{};
        float m_dpi{ DEFAULT_DPI };
        bool m_isMinimized{};
        bool m_isTrackingMouse{};
        int m_exitCode{ EXIT_CODE_NORMAL };
        std::exception_ptr m_pendingException;   // 창 프로시저 안에서 난 예외 (C 콜백 밖으로 던질 수 없어 보관했다가 다시 던짐)
    };

    MainWindow::Impl::Impl(std::wstring_view title, int width, int height, WindowEventHandler& handler)
        : m_handler{ handler }
        , m_instance{ ::GetModuleHandleW(nullptr) }
    {
        // 모니터마다 배율이 달라도 선명하게 (이미 설정돼 있으면 실패하지만 문제 없음)
        ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(WNDCLASSEXW);
        windowClass.style = CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = &Impl::WindowProc;
        windowClass.hInstance = m_instance;
        windowClass.hCursor = ::LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = WINDOW_CLASS_NAME;
        if (::RegisterClassExW(&windowClass) == 0 && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        {
            throw std::runtime_error{ "창 클래스를 등록하지 못했어요!" };
        }

        const std::wstring titleText{ title };
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
    }

    MainWindow::Impl::~Impl()
    {
        if (m_handle != nullptr) ::DestroyWindow(m_handle);   // 이미 닫힌 창이면 WM_NCDESTROY에서 nullptr이 됨
        ::UnregisterClassW(WINDOW_CLASS_NAME, m_instance);
    }

    void MainWindow::Impl::Show(int showCommand)
    {
        ::ShowWindow(m_handle, showCommand);
        ::UpdateWindow(m_handle);
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

    void MainWindow::Impl::WaitForMessage()
    {
        ::WaitMessage();
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
        switch (message)
        {
            case WM_SIZE:
                m_isMinimized = (wParam == SIZE_MINIMIZED);
                if (!m_isMinimized) m_handler.OnResize(LOWORD(lParam), HIWORD(lParam));
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

            case WM_MOUSEMOVE:
                TrackMouseLeave();
                m_handler.OnMouseMove(ToDIP(GetMouseX(lParam)), ToDIP(GetMouseY(lParam)));
                return 0;

            case WM_MOUSELEAVE:
                m_isTrackingMouse = false;
                m_handler.OnMouseLeave();
                return 0;

            case WM_LBUTTONDOWN:
                m_handler.OnMouseDown(ToDIP(GetMouseX(lParam)), ToDIP(GetMouseY(lParam)));
                return 0;

            case WM_KEYDOWN:
                m_handler.OnKeyDown(static_cast<unsigned>(wParam), ::GetKeyState(VK_CONTROL) < 0, ::GetKeyState(VK_SHIFT) < 0);
                return 0;

            case WM_ERASEBKGND:
                return 1;   // 배경은 DX11이 그리므로 지우지 않음 (깜빡임 방지)

            case WM_DESTROY:
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

    float MainWindow::Impl::ToDIP(int pixels) const noexcept
    {
        return static_cast<float>(pixels) * DEFAULT_DPI / m_dpi;
    }

    void MainWindow::Impl::RethrowPendingException()
    {
        if (m_pendingException) std::rethrow_exception(std::exchange(m_pendingException, nullptr));
    }

    // 마우스가 창 밖으로 나갈 때 WM_MOUSELEAVE를 한 번 받도록 등록 (탭 강조 해제용)
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

    bool MainWindow::ProcessMessages()
    {
        return m_impl->ProcessMessages();
    }

    void MainWindow::WaitForMessage()
    {
        m_impl->WaitForMessage();
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
}