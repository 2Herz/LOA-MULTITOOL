module LostArkMultiTool.Views.UI.Button;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr float LINK_UNDERLINE_OFFSET{ 3.0f };   // 글자 아래 밑줄 위치 (세로 가운데 기준 아래로, 글자 크기 절반 + 여백)
        constexpr float HALF{ 0.5f };

        constexpr Core::TextStyle BUTTON_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::CENTER };
        constexpr Core::TextStyle LINK_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
    }

    Button::Button(std::wstring text, ButtonStyle style, std::function<void()> onClick)
        : m_text{ std::move(text) }
        , m_style{ style }
        , m_onClick{ std::move(onClick) }
    {}

    void Button::SetText(std::wstring text)
    {
        m_text = std::move(text);
    }

    void Button::SetOnClick(std::function<void()> onClick)
    {
        m_onClick = std::move(onClick);
    }

    void Button::SetFocusable(bool isFocusable) noexcept
    {
        m_isFocusable = isFocusable;
    }

    bool Button::IsFocusable() const noexcept
    {
        return m_isFocusable;
    }

    Core::CursorShape Button::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_HAND;
    }

    void Button::Render(Core::Renderer& renderer)
    {
        if (m_style == ButtonStyle::BUTTON_LINK)
        {
            RenderLink(renderer);
            return;
        }

        const Core::RectF& bounds{ GetBounds() };
        renderer.FillRoundedRect(bounds, Theme::CORNER_RADIUS, GetBackColor());

        // 키보드 포커스 표시 (Tab으로 옮겨 왔을 때 어디에 있는지 보이게)
        if (IsFocused() && IsEnabled())
        {
            renderer.DrawRoundedRect(bounds, Theme::CORNER_RADIUS, Theme::FOCUS_BORDER_COLOR, Theme::FOCUS_BORDER_WIDTH);
        }

        const Core::Color& textColor{ IsEnabled() ? Theme::HEADER_TEXT_COLOR : Theme::MUTED_TEXT_COLOR };
        renderer.DrawString(m_text, bounds, textColor, BUTTON_TEXT_STYLE);
    }

    void Button::OnMouseUp(const Core::MouseButtonEvent& /*event*/, bool isInside)
    {
        if (isInside) Click();   // 누른 뒤 버튼 밖에서 떼면 취소 (Windows 버튼과 같은 동작)
    }

    bool Button::OnKeyDown(const Core::KeyEvent& event)
    {
        if (event.m_key != Core::Key::KEY_ENTER) return false;

        Click();
        return true;
    }

    void Button::Click()
    {
        if (!IsEnabled() || !m_onClick) return;

        // 콜백 안에서 SetOnClick으로 자신을 바꿔도 안전하게 복사본으로 호출
        const std::function<void()> onClick{ m_onClick };
        onClick();
    }

    void Button::RenderLink(Core::Renderer& renderer) const
    {
        const Core::RectF& bounds{ GetBounds() };
        const bool isActive{ IsEnabled() && (IsHovered() || IsFocused()) };
        const Core::Color& textColor{ IsEnabled() ? Theme::FOCUS_BORDER_COLOR : Theme::MUTED_TEXT_COLOR };

        renderer.DrawString(m_text, bounds, textColor, LINK_TEXT_STYLE);

        // 마우스를 올리거나 포커스가 있으면 글자 너비만큼 밑줄
        if (!isActive) return;

        const float textWidth{ std::min(renderer.MeasureString(m_text, LINK_TEXT_STYLE, bounds.GetWidth()).m_width, bounds.GetWidth()) };
        const float underlineY{ bounds.m_top + bounds.GetHeight() * HALF + LINK_TEXT_STYLE.m_size * HALF + LINK_UNDERLINE_OFFSET };
        renderer.DrawLine(bounds.m_left, underlineY, bounds.m_left + textWidth, underlineY, textColor, Theme::BORDER_WIDTH);
    }

    Core::Color Button::GetBackColor() const noexcept
    {
        if (!IsEnabled()) return Theme::DISABLED_BACK_COLOR;

        const bool isPrimary{ m_style == ButtonStyle::BUTTON_PRIMARY };
        if (IsPressed()) return isPrimary ? Theme::PRIMARY_PRESSED_COLOR : Theme::BUTTON_PRESSED_COLOR;
        if (IsHovered()) return isPrimary ? Theme::PRIMARY_HOVER_COLOR : Theme::BUTTON_HOVER_COLOR;
        return isPrimary ? Theme::PRIMARY_BACK_COLOR : Theme::BUTTON_BACK_COLOR;
    }
}