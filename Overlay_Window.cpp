module;  // 전역 모듈 조각 시작 (윈도우 헤더는 모듈 밖에서 포함)

#define NOMINMAX                 // min · max 매크로 끄기
#define WIN32_LEAN_AND_MEAN      // 잘 안 쓰는 윈도우 헤더 빼기
#include <Windows.h>             // 창 · 메시지 · 단축키

module LostArkMultiTool.Views.OverlayWindow;  // 인게임 오버레이 창 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr const wchar_t* OVERLAY_CLASS_NAME{ L"LostArkMultiToolOverlay" };   // 창 종류 이름
        constexpr const wchar_t* OVERLAY_TITLE{ L"로스트아크 멀티툴 오버레이" };      // 창 제목 (작업 표시줄에는 안 보임)
        constexpr DWORD OVERLAY_STYLE{ WS_POPUP };                                     // 테두리 없는 창
        constexpr DWORD BASE_EX_STYLE{ WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_NOREDIRECTIONBITMAP };   // 항상 위 · 작업 표시줄 없음 · 합성 전용
        constexpr DWORD PASS_THROUGH_EX_STYLE{ WS_EX_TRANSPARENT | WS_EX_NOACTIVATE }; // 클릭 통과 · 포커스 안 가져감 (평소)
        constexpr BYTE OPAQUE_ALPHA{ 255 };                                            // 창 전체 불투명도 (투명도는 그림의 알파로 처리)
        constexpr float DEFAULT_DPI{ 96.0f };                                          // 배율 100% DPI
        constexpr int SINGLE_CLICK_COUNT{ 1 };                                         // 한 번 누름
        constexpr int DOUBLE_CLICK_COUNT{ 2 };                                         // 두 번 누름
        constexpr float WHEEL_NOTCH{ static_cast<float>(WHEEL_DELTA) };                // 휠 한 칸
        constexpr wchar_t FIRST_PRINTABLE_CHARACTER{ L' ' };                           // 이보다 작으면 제어 문자
        constexpr wchar_t DELETE_CHARACTER{ 0x7F };                                    // Ctrl+Backspace 제어 문자
        constexpr WPARAM FIRST_DIGIT_KEY{ '1' };                                       // 숫자 키 시작
        constexpr WPARAM LAST_DIGIT_KEY{ '9' };                                        // 숫자 키 끝
        constexpr std::size_t CURSOR_SHAPE_COUNT{ 3 };                                 // 커서 모양 수

        // 마우스 메시지 좌표 (보조 모니터 음수 좌표를 위해 부호 있는 16비트)
        [[nodiscard]] int GetMouseX(LPARAM lParam) noexcept { return static_cast<short>(LOWORD(lParam)); }   // X
        [[nodiscard]] int GetMouseY(LPARAM lParam) noexcept { return static_cast<short>(HIWORD(lParam)); }   // Y

        // 가상 키 → 화면 코드용 키 (메인 창과 같은 표)
        [[nodiscard]] Core::Key TranslateKey(WPARAM virtualKey) noexcept
        {
            switch (virtualKey)
            {
                case VK_TAB:    return Core::Key::KEY_TAB;         // Tab
                case VK_RETURN: return Core::Key::KEY_ENTER;       // Enter
                case VK_ESCAPE: return Core::Key::KEY_ESCAPE;      // Esc
                case VK_BACK:   return Core::Key::KEY_BACKSPACE;   // Backspace
                case VK_DELETE: return Core::Key::KEY_DELETE;      // Delete
                case VK_LEFT:   return Core::Key::KEY_LEFT;        // ←
                case VK_RIGHT:  return Core::Key::KEY_RIGHT;       // →
                case VK_UP:     return Core::Key::KEY_UP;          // ↑
                case VK_DOWN:   return Core::Key::KEY_DOWN;        // ↓
                case VK_HOME:   return Core::Key::KEY_HOME;        // Home
                case VK_END:    return Core::Key::KEY_END;         // End
                case VK_PRIOR:  return Core::Key::KEY_PAGE_UP;     // PageUp
                case VK_NEXT:   return Core::Key::KEY_PAGE_DOWN;   // PageDown
                case VK_SPACE:  return Core::Key::KEY_SPACE;       // Space
                case 'A':       return Core::Key::KEY_A;           // A (전체 선택)
                case 'C':       return Core::Key::KEY_C;           // C (복사)
                case 'V':       return Core::Key::KEY_V;           // V (붙여넣기)
                case 'X':       return Core::Key::KEY_X;           // X (잘라내기)
                case 'K':       return Core::Key::KEY_K;           // K (도구 검색)
                default:        break;                             // 그 밖
            }
            if (virtualKey >= FIRST_DIGIT_KEY && virtualKey <= LAST_DIGIT_KEY)   // 숫자 키
                return static_cast<Core::Key>(std::to_underlying(Core::Key::KEY_1) + static_cast<int>(virtualKey - FIRST_DIGIT_KEY));
            return Core::Key::KEY_UNKNOWN;   // 쓰지 않는 키
        }
    }

    // ===== 구현 =====
    class OverlayWindow::Impl
    {
    public:
        Impl();     // 창 만들기
        ~Impl();    // 창 없애기

        static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);   // 창 메시지 받기
        LRESULT HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);                // 메시지 처리
        void Emit(const Core::InputEvent& event);                                                      // 입력 알림
        void EmitButton(LPARAM lParam, Core::MouseButton button, bool isPressed, int clickCount);      // 마우스 버튼 알림
        [[nodiscard]] float ToDIP(int pixels) const noexcept;                                          // 픽셀 → DIP
        void ApplyExStyle();                                                                           // 조작 모드에 맞는 확장 스타일

        HWND m_handle{};                                         // 창 핸들
        float m_dpi{ DEFAULT_DPI };                              // 배율
        bool m_isInteractive{};                                  // 조작 모드
        bool m_isShown{};                                        // 보이는지
        bool m_isTrackingMouse{};                                // 마우스 나감 추적 중
        bool m_isLeftButtonDown{};                               // 왼쪽 버튼 누름
        float m_lastMouseX{};                                    // 마지막 마우스 X (DIP)
        float m_lastMouseY{};                                    // 마지막 마우스 Y (DIP)
        std::vector<int> m_hotkeyIds;                            // 등록한 단축키
        std::array<HCURSOR, CURSOR_SHAPE_COUNT> m_cursors{};     // 커서들
        Core::CursorShape m_cursor{ Core::CursorShape::CURSOR_ARROW };   // 지금 커서
        std::function<void(const Core::InputEvent&)> m_onInput;          // 입력 알림
        std::function<void(int)> m_onHotkey;                             // 단축키 알림
        std::function<void(int, int)> m_onResize;                        // 크기 알림
        std::function<void(float)> m_onDPIChanged;                       // 배율 알림
    };

    OverlayWindow::Impl::Impl()
    {
        const HINSTANCE instance = ::GetModuleHandleW(nullptr);                       // 실행 파일
        WNDCLASSEXW windowClass{};                                                     // 창 종류
        windowClass.cbSize = sizeof(windowClass);                                      // 크기
        windowClass.style = CS_DBLCLKS;                                                // 두 번 누름 받기
        windowClass.lpfnWndProc = &Impl::WindowProc;                                   // 메시지 함수
        windowClass.hInstance = instance;                                              // 실행 파일
        windowClass.lpszClassName = OVERLAY_CLASS_NAME;                                // 이름
        ::RegisterClassExW(&windowClass);                                              // 등록 (이미 있으면 실패해도 그대로 씀)

        m_cursors[std::to_underlying(Core::CursorShape::CURSOR_ARROW)] = ::LoadCursorW(nullptr, IDC_ARROW);   // 화살표
        m_cursors[std::to_underlying(Core::CursorShape::CURSOR_HAND)] = ::LoadCursorW(nullptr, IDC_HAND);     // 손
        m_cursors[std::to_underlying(Core::CursorShape::CURSOR_TEXT)] = ::LoadCursorW(nullptr, IDC_IBEAM);    // 글자

        m_handle = ::CreateWindowExW(BASE_EX_STYLE | PASS_THROUGH_EX_STYLE, OVERLAY_CLASS_NAME, OVERLAY_TITLE, OVERLAY_STYLE,
            0, 0, 1, 1, nullptr, nullptr, instance, this);                             // 숨은 창 (크기는 Place에서)
        if (m_handle == nullptr) throw std::runtime_error("오버레이 창을 만들지 못했어요");   // 실패
        ::SetLayeredWindowAttributes(m_handle, 0, OPAQUE_ALPHA, LWA_ALPHA);            // 겹친 창을 보이게 (내용 투명도는 그림 알파)
        m_dpi = static_cast<float>(::GetDpiForWindow(m_handle));                       // 배율
    }

    OverlayWindow::Impl::~Impl()
    {
        if (m_handle == nullptr) return;                                               // 이미 없음
        for (int hotkeyId : m_hotkeyIds) ::UnregisterHotKey(m_handle, hotkeyId);       // 단축키 해제
        ::SetWindowLongPtrW(m_handle, GWLP_USERDATA, 0);                               // 연결 끊기 (없앤 뒤 메시지 무시)
        ::DestroyWindow(m_handle);                                                     // 창 없애기
    }

    LRESULT CALLBACK OverlayWindow::Impl::WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_NCCREATE)                                                    // 만들 때 this 붙이기
        {
            const auto* createInfo = reinterpret_cast<const CREATESTRUCTW*>(lParam);   // 생성 정보
            ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createInfo->lpCreateParams));   // 연결
        }
        auto* owner = reinterpret_cast<Impl*>(::GetWindowLongPtrW(window, GWLP_USERDATA));   // 구현
        if (owner == nullptr) return ::DefWindowProcW(window, message, wParam, lParam);       // 연결 전 · 후
        try
        {
            return owner->HandleMessage(window, message, wParam, lParam);             // 처리
        }
        catch (...)
        {
            return 0;   // 오버레이 입력 오류는 무시 (메인 프로그램은 계속)
        }
    }

    LRESULT OverlayWindow::Impl::HandleMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
            case WM_HOTKEY:                                                            // 전역 단축키
                if (m_onHotkey) m_onHotkey(static_cast<int>(wParam));                 // 알림
                return 0;
            case WM_SIZE:                                                              // 크기 바뀜
                if (m_onResize) m_onResize(LOWORD(lParam), HIWORD(lParam));            // 알림
                return 0;
            case WM_DPICHANGED:                                                        // 배율 바뀜 (위치는 게임 창을 따라가므로 무시)
                m_dpi = static_cast<float>(HIWORD(wParam));                            // 새 배율
                if (m_onDPIChanged) m_onDPIChanged(m_dpi);                             // 알림
                return 0;
            case WM_MOUSEACTIVATE:                                                     // 눌러도 평소엔 포커스를 가져가지 않음
                return m_isInteractive ? MA_ACTIVATE : MA_NOACTIVATE;
            case WM_SETCURSOR:                                                         // 커서 모양
                if (LOWORD(lParam) == HTCLIENT) { ::SetCursor(m_cursors[std::to_underlying(m_cursor)]); return TRUE; }
                break;
            case WM_MOUSEMOVE:                                                         // 마우스 이동
                if (!m_isTrackingMouse)                                                // 나감 추적 시작
                {
                    TRACKMOUSEEVENT track{ sizeof(TRACKMOUSEEVENT), TME_LEAVE, window, 0 };   // 나감 알림 요청
                    m_isTrackingMouse = ::TrackMouseEvent(&track) != FALSE;            // 추적 중
                }
                m_lastMouseX = ToDIP(GetMouseX(lParam));                               // DIP 좌표
                m_lastMouseY = ToDIP(GetMouseY(lParam));
                Emit(Core::MouseMoveEvent{ m_lastMouseX, m_lastMouseY });              // 알림
                return 0;
            case WM_MOUSELEAVE:                                                        // 마우스 나감
                m_isTrackingMouse = false;                                             // 추적 끝
                Emit(Core::MouseLeaveEvent{});                                         // 알림
                return 0;
            case WM_LBUTTONDOWN:
            case WM_LBUTTONDBLCLK:                                                     // 왼쪽 누름
                m_isLeftButtonDown = true;                                             // 누름 상태
                ::SetCapture(window);                                                  // 끌기 중 밖으로 나가도 받기
                EmitButton(lParam, Core::MouseButton::MOUSE_BUTTON_LEFT, true, message == WM_LBUTTONDBLCLK ? DOUBLE_CLICK_COUNT : SINGLE_CLICK_COUNT);
                return 0;
            case WM_LBUTTONUP:                                                         // 왼쪽 뗌
                if (m_isLeftButtonDown)
                {
                    m_isLeftButtonDown = false;                                        // 먼저 내림 (캡처 해제 알림과 겹치지 않게)
                    EmitButton(lParam, Core::MouseButton::MOUSE_BUTTON_LEFT, false, SINGLE_CLICK_COUNT);
                }
                ::ReleaseCapture();                                                    // 캡처 해제
                return 0;
            case WM_CAPTURECHANGED:                                                    // 다른 창이 마우스를 가져감
                if (m_isLeftButtonDown && reinterpret_cast<HWND>(lParam) != window)
                {
                    m_isLeftButtonDown = false;                                        // 뗀 것으로 처리
                    Emit(Core::MouseButtonEvent{ m_lastMouseX, m_lastMouseY, Core::MouseButton::MOUSE_BUTTON_LEFT, false, SINGLE_CLICK_COUNT });
                }
                return 0;
            case WM_RBUTTONDOWN:                                                       // 오른쪽 누름
                EmitButton(lParam, Core::MouseButton::MOUSE_BUTTON_RIGHT, true, SINGLE_CLICK_COUNT);
                return 0;
            case WM_RBUTTONUP:                                                         // 오른쪽 뗌
                EmitButton(lParam, Core::MouseButton::MOUSE_BUTTON_RIGHT, false, SINGLE_CLICK_COUNT);
                return 0;
            case WM_MOUSEWHEEL:                                                        // 휠 (화면 좌표 → 창 좌표)
            {
                POINT point{ GetMouseX(lParam), GetMouseY(lParam) };                   // 화면 좌표
                ::ScreenToClient(window, &point);                                      // 창 좌표
                Emit(Core::MouseWheelEvent{ ToDIP(point.x), ToDIP(point.y), static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_NOTCH });
                return 0;
            }
            case WM_KEYDOWN:                                                           // 키 누름
            {
                const Core::Key key = TranslateKey(wParam);                            // 화면 키
                if (key != Core::Key::KEY_UNKNOWN) Emit(Core::KeyEvent{ key, ::GetKeyState(VK_CONTROL) < 0, ::GetKeyState(VK_SHIFT) < 0 });
                return 0;
            }
            case WM_CHAR:                                                              // 글자 (한글은 조합이 끝난 글자로 옴)
            {
                const auto character = static_cast<wchar_t>(wParam);                   // 글자
                if (character >= FIRST_PRINTABLE_CHARACTER && character != DELETE_CHARACTER) Emit(Core::TextInputEvent{ std::wstring(1, character) });
                return 0;
            }
            case WM_ERASEBKGND:                                                        // 배경 지우기 안 함 (합성이 그림)
                return 1;
            case WM_CLOSE:                                                             // Alt+F4 등으로 닫지 않음 (숨기기만)
                ::ShowWindow(window, SW_HIDE);
                m_isShown = false;
                return 0;
            default:
                break;
        }
        return ::DefWindowProcW(window, message, wParam, lParam);   // 기본 처리
    }

    void OverlayWindow::Impl::Emit(const Core::InputEvent& event)
    {
        if (m_onInput) m_onInput(event);   // 매니저에 알림
    }

    void OverlayWindow::Impl::EmitButton(LPARAM lParam, Core::MouseButton button, bool isPressed, int clickCount)
    {
        m_lastMouseX = ToDIP(GetMouseX(lParam));                                       // DIP 좌표
        m_lastMouseY = ToDIP(GetMouseY(lParam));
        Emit(Core::MouseButtonEvent{ m_lastMouseX, m_lastMouseY, button, isPressed, clickCount });   // 알림
    }

    float OverlayWindow::Impl::ToDIP(int pixels) const noexcept
    {
        return static_cast<float>(pixels) * DEFAULT_DPI / m_dpi;   // 픽셀 → DIP
    }

    void OverlayWindow::Impl::ApplyExStyle()
    {
        const DWORD exStyle = BASE_EX_STYLE | (m_isInteractive ? 0 : PASS_THROUGH_EX_STYLE);     // 조작 모드면 클릭 통과 끔
        ::SetWindowLongPtrW(m_handle, GWL_EXSTYLE, static_cast<LONG_PTR>(exStyle));              // 적용
        ::SetWindowPos(m_handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);   // 반영
    }

    // ===== OverlayWindow =====

    OverlayWindow::OverlayWindow()
        : m_impl{ std::make_unique<Impl>() }   // 창 만들기
    {
    }

    OverlayWindow::~OverlayWindow() = default;   // 구현 소멸자가 창을 없앰

    void* OverlayWindow::GetNativeHandle() const noexcept
    {
        return m_impl->m_handle;   // 창 핸들
    }

    float OverlayWindow::GetDPI() const noexcept
    {
        return m_impl->m_dpi;   // 배율
    }

    void OverlayWindow::SetOnInput(std::function<void(const Core::InputEvent&)> onInput)
    {
        m_impl->m_onInput = std::move(onInput);   // 입력 알림 연결
    }

    void OverlayWindow::SetOnHotkey(std::function<void(int hotkeyId)> onHotkey)
    {
        m_impl->m_onHotkey = std::move(onHotkey);   // 단축키 알림 연결
    }

    void OverlayWindow::SetOnResize(std::function<void(int pixelWidth, int pixelHeight)> onResize)
    {
        m_impl->m_onResize = std::move(onResize);   // 크기 알림 연결
    }

    void OverlayWindow::SetOnDPIChanged(std::function<void(float dpi)> onDPIChanged)
    {
        m_impl->m_onDPIChanged = std::move(onDPIChanged);   // 배율 알림 연결
    }

    bool OverlayWindow::RegisterHotkey(int hotkeyId, unsigned modifiers, unsigned virtualKey)
    {
        if (!::RegisterHotKey(m_impl->m_handle, hotkeyId, modifiers | MOD_NOREPEAT, virtualKey)) return false;   // 이미 다른 프로그램이 씀
        m_impl->m_hotkeyIds.push_back(hotkeyId);   // 해제용 기록
        return true;                               // 등록함
    }

    void OverlayWindow::Place(const Vision::PixelRect& screenRect)
    {
        RECT current{};                                                                // 지금 위치
        ::GetWindowRect(m_impl->m_handle, &current);                                   // 읽기
        const bool isSame = current.left == screenRect.m_x && current.top == screenRect.m_y
            && current.right == screenRect.GetRight() && current.bottom == screenRect.GetBottom();   // 같은 위치인지
        if (isSame) return;                                                            // 그대로면 아무것도 안 함
        ::SetWindowPos(m_impl->m_handle, HWND_TOPMOST, screenRect.m_x, screenRect.m_y, screenRect.m_width, screenRect.m_height,
            SWP_NOACTIVATE);                                                           // 게임 화면 위로 (포커스 안 가져감)
        m_impl->m_dpi = static_cast<float>(::GetDpiForWindow(m_impl->m_handle));       // 다른 모니터로 갔을 수 있음
    }

    void OverlayWindow::Show()
    {
        if (m_impl->m_isShown) return;                                                 // 이미 보임
        ::ShowWindow(m_impl->m_handle, SW_SHOWNOACTIVATE);                             // 포커스 없이 보이기
        m_impl->m_isShown = true;                                                      // 보임
    }

    void OverlayWindow::Hide()
    {
        if (!m_impl->m_isShown) return;                                                // 이미 숨음
        ::ShowWindow(m_impl->m_handle, SW_HIDE);                                       // 숨기기
        m_impl->m_isShown = false;                                                     // 숨음
    }

    bool OverlayWindow::IsShown() const noexcept
    {
        return m_impl->m_isShown;   // 보이는지
    }

    void OverlayWindow::SetInteractive(bool isInteractive)
    {
        if (m_impl->m_isInteractive == isInteractive) return;   // 그대로
        m_impl->m_isInteractive = isInteractive;                 // 바꾸기
        m_impl->ApplyExStyle();                                  // 스타일 반영
    }

    bool OverlayWindow::IsInteractive() const noexcept
    {
        return m_impl->m_isInteractive;   // 조작 모드인지
    }

    bool OverlayWindow::IsForeground() const noexcept
    {
        return ::GetForegroundWindow() == m_impl->m_handle;   // 맨 앞인지
    }

    void OverlayWindow::Activate()
    {
        ::SetForegroundWindow(m_impl->m_handle);   // 맨 앞으로 (단축키 · 클릭 직후라 허용됨)
        ::SetFocus(m_impl->m_handle);              // 키보드 포커스
    }

    void OverlayWindow::ActivateWindow(void* windowHandle)
    {
        if (windowHandle != nullptr) ::SetForegroundWindow(static_cast<HWND>(windowHandle));   // 게임으로 포커스 돌려주기
    }

    void OverlayWindow::SetCursor(Core::CursorShape shape) noexcept
    {
        m_impl->m_cursor = shape;   // 다음 WM_SETCURSOR에서 반영
    }
}
