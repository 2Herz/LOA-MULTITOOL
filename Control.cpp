module LostArkMultiTool.Views.UI.Control;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;

namespace LostArkMultiTool::Views::UI
{
    Control::~Control() = default;

    void Control::SetBounds(const Core::RectF& bounds) noexcept
    {
        m_bounds = bounds;
    }

    const Core::RectF& Control::GetBounds() const noexcept
    {
        return m_bounds;
    }

    void Control::SetVisible(bool isVisible) noexcept
    {
        m_isVisible = isVisible;
    }

    bool Control::IsVisible() const noexcept
    {
        return m_isVisible;
    }

    void Control::SetEnabled(bool isEnabled) noexcept
    {
        m_isEnabled = isEnabled;
    }

    bool Control::IsEnabled() const noexcept
    {
        return m_isEnabled;
    }

    void Control::SetHovered(bool isHovered) noexcept
    {
        m_isHovered = isHovered;
    }

    void Control::SetPressed(bool isPressed) noexcept
    {
        m_isPressed = isPressed;
    }

    void Control::SetFocused(bool isFocused)
    {
        if (isFocused == m_isFocused) return;

        m_isFocused = isFocused;
        OnFocusChanged(isFocused);
    }

    bool Control::IsHovered() const noexcept
    {
        return m_isHovered;
    }

    bool Control::IsPressed() const noexcept
    {
        return m_isPressed;
    }

    bool Control::IsFocused() const noexcept
    {
        return m_isFocused;
    }

    bool Control::IsFocusable() const noexcept
    {
        return false;
    }

    bool Control::IsHitTestVisible() const noexcept
    {
        return true;
    }

    Core::CursorShape Control::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_ARROW;
    }

    void Control::Update(double /*deltaSeconds*/)
    {}

    void Control::OnMouseDown(const Core::MouseButtonEvent& /*event*/)
    {}

    void Control::OnMouseUp(const Core::MouseButtonEvent& /*event*/, bool /*isInside*/)
    {}

    void Control::OnMouseDrag(const Core::MouseMoveEvent& /*event*/)
    {}

    bool Control::OnMouseWheel(const Core::MouseWheelEvent& /*event*/)
    {
        return false;
    }

    bool Control::OnKeyDown(const Core::KeyEvent& /*event*/)
    {
        return false;
    }

    void Control::OnTextInput(std::wstring_view /*text*/)
    {}

    void Control::OnComposition(std::wstring_view /*text*/)
    {}

    void Control::OnFocusChanged(bool /*isFocused*/)
    {}
}