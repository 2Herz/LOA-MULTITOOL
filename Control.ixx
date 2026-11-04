export module LostArkMultiTool.Views.UI.Control;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Views::UI
{
    // 화면 부품의 공통 기반 (버튼 · 입력칸 · 글자 등)
    // ControlHost가 여러 부품을 묶어 마우스 · 키보드 입력을 알맞은 부품에 나눠 줌 (Composite 패턴)
    class Control
    {
    public:
        virtual ~Control();

        Control(const Control&) = delete;
        Control& operator=(const Control&) = delete;

        void SetBounds(const Core::RectF& bounds) noexcept;
        [[nodiscard]] const Core::RectF& GetBounds() const noexcept;

        void SetVisible(bool isVisible) noexcept;
        [[nodiscard]] bool IsVisible() const noexcept;
        void SetEnabled(bool isEnabled) noexcept;
        [[nodiscard]] bool IsEnabled() const noexcept;

        // ===== ControlHost가 바꾸는 상태 =====
        void SetHovered(bool isHovered) noexcept;
        void SetPressed(bool isPressed) noexcept;
        void SetFocused(bool isFocused);   // 바뀔 때 OnFocusChanged 호출
        [[nodiscard]] bool IsHovered() const noexcept;
        [[nodiscard]] bool IsPressed() const noexcept;
        [[nodiscard]] bool IsFocused() const noexcept;

        [[nodiscard]] virtual bool IsFocusable() const noexcept;          // 키보드 포커스를 받는 부품인지 (기본 false)
        [[nodiscard]] virtual bool IsHitTestVisible() const noexcept;     // 마우스를 받는 부품인지 (기본 true)
        [[nodiscard]] virtual bool HitTest(float x, float y) const noexcept;   // (x, y)가 부품 위인지 (기본: 테두리 안, 펼친 목록이 있으면 목록도 포함)
        [[nodiscard]] virtual Core::CursorShape GetCursor() const noexcept;

        virtual void Update(double deltaSeconds);
        virtual void Render(Core::Renderer& renderer) = 0;

        // ===== 펼친 목록(팝업) — 콤보 상자 목록처럼 다른 부품 위에 그려지고 마우스도 먼저 받음 =====
        [[nodiscard]] virtual bool IsPopupOpen() const noexcept;   // 기본 false
        virtual void RenderPopup(Core::Renderer& renderer);       // 모든 부품을 그린 뒤 호출

        // ===== 입력 (ControlHost가 알맞은 부품에만 전달) =====
        virtual void OnMouseDown(const Core::MouseButtonEvent& event);
        virtual void OnMouseUp(const Core::MouseButtonEvent& event, bool isInside);   // isInside: 부품 위에서 뗐는지 (클릭 판정)
        virtual void OnMouseDrag(const Core::MouseMoveEvent& event);                  // 누른 채 움직임
        virtual void OnMouseMove(const Core::MouseMoveEvent& event);                  // 누르지 않고 움직임 (마우스를 올린 부품만 받음)
        [[nodiscard]] virtual bool OnMouseWheel(const Core::MouseWheelEvent& event);
        [[nodiscard]] virtual bool OnKeyDown(const Core::KeyEvent& event);
        virtual void OnTextInput(std::wstring_view text);
        virtual void OnComposition(std::wstring_view text);

    protected:
        Control() = default;

        virtual void OnFocusChanged(bool isFocused);

    private:
        Core::RectF m_bounds;
        bool m_isVisible{ true };
        bool m_isEnabled{ true };
        bool m_isHovered{};
        bool m_isPressed{};
        bool m_isFocused{};
    };
}