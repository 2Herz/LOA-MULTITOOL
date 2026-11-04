export module LostArkMultiTool.Views.UI.ControlHost;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 화면 부품 묶음 (부품 소유 + 입력 나눠 주기 + 포커스 · 마우스 상태 관리)
    // 탭 화면 하나 · 대화상자 하나마다 하나씩 가짐
    class ControlHost
    {
    public:
        ControlHost();
        ~ControlHost();

        ControlHost(const ControlHost&) = delete;
        ControlHost& operator=(const ControlHost&) = delete;

        // 부품을 만들어 추가하고 참조를 돌려줌 (소유는 ControlHost, 나중에 추가한 부품이 위에 그려짐)
        // 템플릿은 쓰는 곳에서 정의가 보여야 하므로 정의를 여기 둠
        template <typename ControlType, typename... Arguments>
        ControlType& Emplace(Arguments&&... arguments)
        {
            auto control{ std::make_unique<ControlType>(std::forward<Arguments>(arguments)...) };
            ControlType& reference{ *control };
            AddControl(std::move(control));
            return reference;
        }

        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer);

        // 입력 처리 (부품이 처리했으면 true)
        bool HandleInput(const Core::InputEvent& event);

        void SetFocus(Control* control);
        [[nodiscard]] Control* GetFocused() const noexcept;
        void FocusNext(bool isReverse);   // Tab · Shift+Tab

        // 입력을 다른 곳(다른 탭 · 대화상자)에 잠시 넘길 때: 포커스를 내려놓되 기억해 둠 (한글 조합 확정 · IME 끔)
        void SuspendFocus();
        void RestoreFocus();

        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;

    private:
        void AddControl(std::unique_ptr<Control> control);

        bool HandleMouseMove(const Core::MouseMoveEvent& event);
        bool HandleMouseButton(const Core::MouseButtonEvent& event);
        bool HandleMouseWheel(const Core::MouseWheelEvent& event);
        bool HandleKey(const Core::KeyEvent& event);

        [[nodiscard]] Control* FindControlAt(float x, float y) const noexcept;
        [[nodiscard]] static bool IsInteractive(const Control& control) noexcept;
        void SetHovered(Control* control);
        void ReleasePointer();
        void DropUnavailableStates();

        std::vector<std::unique_ptr<Control>> m_controls;

        // 아래 포인터는 m_controls가 소유한 부품을 가리키기만 함 (부품은 지우지 않으므로 댕글링 없음)
        Control* m_hovered{};          // 마우스를 올린 부품
        Control* m_pressed{};          // 누르고 있는 부품 (뗄 때까지 입력을 붙잡음)
        Control* m_focused{};          // 키보드 입력을 받는 부품
        Control* m_suspendedFocus{};   // SuspendFocus로 내려놓은 포커스
    };
}