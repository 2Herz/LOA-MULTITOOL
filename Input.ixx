export module LostArkMultiTool.Core.Input;

import std;

// 입력 이벤트 (창 → Application → 탭 · 대화상자 → 화면 부품)
// Windows 가상 키 코드 대신 자체 키 값을 써서, 화면 코드가 Windows 헤더(와 GetClassName 같은 매크로)를 몰라도 되게 함
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Core
{
    // 화면 코드가 쓰는 키 (그 밖의 키는 창에서 걸러서 보내지 않음)
    enum class Key
    {
        KEY_UNKNOWN,
        KEY_TAB,
        KEY_ENTER,
        KEY_ESCAPE,
        KEY_BACKSPACE,
        KEY_DELETE,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_UP,
        KEY_DOWN,
        KEY_HOME,
        KEY_END,
        KEY_PAGE_UP,
        KEY_PAGE_DOWN,
        KEY_A,
        KEY_C,
        KEY_V,
        KEY_X,
        KEY_K,      // Ctrl+K 도구 검색
        KEY_SPACE,
        KEY_1,      // KEY_1 ~ KEY_9는 순서대로 이어져 있어야 함 (탭 단축키 계산)
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9
    };

    enum class MouseButton
    {
        MOUSE_BUTTON_LEFT,
        MOUSE_BUTTON_RIGHT
    };

    // 마우스 커서 모양
    enum class CursorShape
    {
        CURSOR_ARROW,   // 기본
        CURSOR_HAND,    // 누를 수 있는 곳 (버튼 · 탭 · 링크)
        CURSOR_TEXT     // 글자 입력칸
    };

    // ===== 이벤트 (좌표는 모두 DIP, 창 안쪽 왼쪽 위 기준) =====
    struct MouseMoveEvent
    {
        float m_x{};
        float m_y{};
    };

    struct MouseLeaveEvent
    {};

    struct MouseButtonEvent
    {
        float m_x{};
        float m_y{};
        MouseButton m_button{ MouseButton::MOUSE_BUTTON_LEFT };
        bool m_isPressed{};      // true = 누름, false = 뗌
        int m_clickCount{ 1 };   // 2 = 더블 클릭
    };

    struct MouseWheelEvent
    {
        float m_x{};
        float m_y{};
        float m_notches{};       // 위로 한 칸 = +1, 아래로 한 칸 = -1 (정밀 터치패드는 소수)
    };

    struct KeyEvent
    {
        Key m_key{ Key::KEY_UNKNOWN };
        bool m_isCtrlDown{};
        bool m_isShiftDown{};
        bool m_isAltDown{};   // Alt+↑/↓ 도구 이동 (WM_SYSKEYDOWN)
    };

    // 확정된 글자 (일반 타자 · 한글 조합 완료)
    struct TextInputEvent
    {
        std::wstring m_text;
    };

    // 한글 조합 중인 글자 (빈 문자열 = 조합 끝 · 취소)
    struct CompositionEvent
    {
        std::wstring m_text;
    };

    using InputEvent = std::variant<MouseMoveEvent, MouseLeaveEvent, MouseButtonEvent, MouseWheelEvent,
        KeyEvent, TextInputEvent, CompositionEvent>;

    // std::visit에 람다 여러 개를 한 번에 넘기기 위한 묶음 (이벤트 종류별 처리)
    template <typename... Handlers>
    struct Overloaded : Handlers...
    {
        using Handlers::operator()...;
    };

    template <typename... Handlers>
    Overloaded(Handlers...) -> Overloaded<Handlers...>;
}