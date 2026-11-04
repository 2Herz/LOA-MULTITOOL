module LostArkMultiTool.Views.UI.CheckBox;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr float BOX_SIZE{ 16.0f };
        constexpr float BOX_RADIUS{ 3.0f };
        constexpr float TEXT_GAP{ 8.0f };               // 상자와 글자 사이
        constexpr float FOCUS_RING_GAP{ 2.0f };         // 키보드 포커스 테두리와 상자 사이
        constexpr float CHECK_LINE_WIDTH{ 2.0f };
        constexpr float HALF{ 0.5f };

        // 체크 표시(✓) 꺾이는 점: 상자 크기에 대한 비율
        constexpr float CHECK_START_X{ 0.22f };
        constexpr float CHECK_START_Y{ 0.52f };
        constexpr float CHECK_MIDDLE_X{ 0.42f };
        constexpr float CHECK_MIDDLE_Y{ 0.72f };
        constexpr float CHECK_END_X{ 0.78f };
        constexpr float CHECK_END_Y{ 0.30f };

        constexpr Core::TextStyle LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
    }

    CheckBox::CheckBox(std::wstring text, std::function<void(bool isChecked)> onCheckedChanged)
        : m_text{ std::move(text) }
        , m_onCheckedChanged{ std::move(onCheckedChanged) }
    {}

    void CheckBox::SetChecked(bool isChecked) noexcept
    {
        m_isChecked = isChecked;
    }

    bool CheckBox::IsChecked() const noexcept
    {
        return m_isChecked;
    }

    bool CheckBox::IsFocusable() const noexcept
    {
        return true;
    }

    Core::CursorShape CheckBox::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_HAND;
    }

    void CheckBox::Render(Core::Renderer& renderer)
    {
        const Core::RectF& bounds{ GetBounds() };
        const bool isEnabled{ IsEnabled() };
        const float boxTop{ bounds.m_top + (bounds.GetHeight() - BOX_SIZE) * HALF };
        const Core::RectF box{ bounds.m_left, boxTop, bounds.m_left + BOX_SIZE, boxTop + BOX_SIZE };

        // 상자: 켜져 있으면 주요 색으로 채움
        Core::Color boxColor{ Theme::INPUT_BACK_COLOR };
        if (!isEnabled) boxColor = Theme::DISABLED_BACK_COLOR;
        else if (m_isChecked) boxColor = IsHovered() ? Theme::PRIMARY_HOVER_COLOR : Theme::PRIMARY_BACK_COLOR;
        renderer.FillRoundedRect(box, BOX_RADIUS, boxColor);

        const Core::Color& borderColor{ isEnabled && (IsHovered() || m_isChecked) ? Theme::FOCUS_BORDER_COLOR : Theme::GRID_LINE_COLOR };
        renderer.DrawRoundedRect(box, BOX_RADIUS, borderColor, Theme::BORDER_WIDTH);

        if (m_isChecked)
        {
            const Core::Color& checkColor{ isEnabled ? Theme::HEADER_TEXT_COLOR : Theme::MUTED_TEXT_COLOR };
            const float middleX{ box.m_left + BOX_SIZE * CHECK_MIDDLE_X };
            const float middleY{ box.m_top + BOX_SIZE * CHECK_MIDDLE_Y };
            renderer.DrawLine(box.m_left + BOX_SIZE * CHECK_START_X, box.m_top + BOX_SIZE * CHECK_START_Y, middleX, middleY, checkColor, CHECK_LINE_WIDTH);
            renderer.DrawLine(middleX, middleY, box.m_left + BOX_SIZE * CHECK_END_X, box.m_top + BOX_SIZE * CHECK_END_Y, checkColor, CHECK_LINE_WIDTH);
        }

        // 키보드 포커스 표시 (Tab으로 옮겨 왔을 때)
        if (IsFocused() && isEnabled)
        {
            const Core::RectF focusRing{ box.m_left - FOCUS_RING_GAP, box.m_top - FOCUS_RING_GAP, box.m_right + FOCUS_RING_GAP, box.m_bottom + FOCUS_RING_GAP };
            renderer.DrawRoundedRect(focusRing, BOX_RADIUS + FOCUS_RING_GAP, Theme::FOCUS_BORDER_COLOR, Theme::BORDER_WIDTH);
        }

        const Core::RectF textRect{ box.m_right + TEXT_GAP, bounds.m_top, bounds.m_right, bounds.m_bottom };
        renderer.DrawString(m_text, textRect, isEnabled ? Theme::TEXT_COLOR : Theme::MUTED_TEXT_COLOR, LABEL_STYLE);
    }

    void CheckBox::OnMouseUp(const Core::MouseButtonEvent& /*event*/, bool isInside)
    {
        if (isInside) Toggle();   // 누른 뒤 밖에서 떼면 취소 (버튼과 같은 동작)
    }

    bool CheckBox::OnKeyDown(const Core::KeyEvent& event)
    {
        if (event.m_key != Core::Key::KEY_SPACE && event.m_key != Core::Key::KEY_ENTER) return false;

        Toggle();
        return true;
    }

    void CheckBox::Toggle()
    {
        if (!IsEnabled()) return;

        m_isChecked = !m_isChecked;
        if (!m_onCheckedChanged) return;

        const std::function<void(bool)> onCheckedChanged{ m_onCheckedChanged };   // 콜백 안에서 SetChecked를 불러도 안전하게
        onCheckedChanged(m_isChecked);
    }
}