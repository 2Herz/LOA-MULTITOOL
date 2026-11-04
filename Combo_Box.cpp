module LostArkMultiTool.Views.UI.ComboBox;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        // ===== 배치 (DIP) =====
        constexpr float TEXT_PADDING{ 10.0f };            // 글자 좌우 여백
        constexpr float ARROW_AREA_WIDTH{ 28.0f };        // 오른쪽 화살표 자리
        constexpr float ARROW_HALF_WIDTH{ 4.0f };         // 화살표(∨) 반 너비
        constexpr float ARROW_HALF_HEIGHT{ 2.0f };
        constexpr float ARROW_LINE_WIDTH{ 1.5f };
        constexpr float POPUP_GAP{ 2.0f };                // 상자와 펼친 목록 사이
        constexpr float POPUP_PADDING{ 4.0f };            // 펼친 목록 안쪽 여백
        constexpr float ITEM_HEIGHT{ 28.0f };
        constexpr float SCROLLBAR_WIDTH{ 3.0f };
        constexpr float SCROLLBAR_MARGIN{ 2.0f };
        constexpr float HALF{ 0.5f };
        constexpr float BOTH_SIDES{ 2.0f };

        constexpr std::size_t MAX_VISIBLE_ITEMS{ 10 };    // 이보다 많으면 휠로 스크롤
        constexpr float WHEEL_ITEMS_PER_NOTCH{ 3.0f };    // 휠 한 칸에 넘기는 항목 수
        constexpr int PREVIOUS_STEP{ -1 };
        constexpr int NEXT_STEP{ 1 };

        constexpr Core::TextStyle ITEM_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle SELECTED_ITEM_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };
    }

    ComboBox::ComboBox() = default;

    void ComboBox::SetItems(std::vector<std::wstring> items)
    {
        ClosePopup();
        m_items = std::move(items);
        m_itemColors.clear();
        m_selectedIndex.reset();
        m_highlightIndex.reset();
        m_scrollIndex = 0;
    }

    void ComboBox::SetItemColors(std::vector<std::optional<Core::Color>> itemColors)
    {
        m_itemColors = std::move(itemColors);
    }

    std::optional<Core::Color> ComboBox::GetItemColor(std::size_t index) const noexcept
    {
        return index < m_itemColors.size() ? m_itemColors[index] : std::nullopt;
    }

    std::size_t ComboBox::GetItemCount() const noexcept
    {
        return m_items.size();
    }

    void ComboBox::SetSelectedIndex(std::optional<std::size_t> index)
    {
        m_selectedIndex = (index.has_value() && *index < m_items.size()) ? index : std::nullopt;
    }

    std::optional<std::size_t> ComboBox::GetSelectedIndex() const noexcept
    {
        return m_selectedIndex;
    }

    void ComboBox::SetOnSelectionChanged(std::function<void()> onSelectionChanged)
    {
        m_onSelectionChanged = std::move(onSelectionChanged);
    }

    bool ComboBox::IsFocusable() const noexcept
    {
        return true;
    }

    bool ComboBox::HitTest(float x, float y) const noexcept
    {
        return Control::HitTest(x, y) || (m_isPopupOpen && GetPopupBounds().Contains(x, y));
    }

    Core::CursorShape ComboBox::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_HAND;
    }

    void ComboBox::Render(Core::Renderer& renderer)
    {
        const Core::RectF& bounds{ GetBounds() };
        const bool isEnabled{ IsEnabled() };

        Core::Color backColor{ Theme::BUTTON_BACK_COLOR };
        if (!isEnabled) backColor = Theme::DISABLED_BACK_COLOR;
        else if (m_isPopupOpen || IsPressed()) backColor = Theme::BUTTON_PRESSED_COLOR;
        else if (IsHovered()) backColor = Theme::BUTTON_HOVER_COLOR;
        renderer.FillRoundedRect(bounds, Theme::CORNER_RADIUS, backColor);

        const bool isActive{ isEnabled && (IsFocused() || m_isPopupOpen) };
        renderer.DrawRoundedRect(bounds, Theme::CORNER_RADIUS, isActive ? Theme::FOCUS_BORDER_COLOR : Theme::GRID_LINE_COLOR,
            isActive ? Theme::FOCUS_BORDER_WIDTH : Theme::BORDER_WIDTH);

        const Core::Color& textColor{ isEnabled ? Theme::TEXT_COLOR : Theme::MUTED_TEXT_COLOR };
        if (m_selectedIndex.has_value())
        {
            const Core::RectF textRect{ bounds.m_left + TEXT_PADDING, bounds.m_top, bounds.m_right - ARROW_AREA_WIDTH, bounds.m_bottom };
            // 항목 글자색이 있으면 그 색 (비활성이면 흐리게)
            const std::optional<Core::Color> itemColor{ GetItemColor(*m_selectedIndex) };
            renderer.DrawString(m_items.at(*m_selectedIndex), textRect, isEnabled && itemColor.has_value() ? *itemColor : textColor, ITEM_STYLE);
        }

        RenderArrow(renderer, textColor);
    }

    bool ComboBox::IsPopupOpen() const noexcept
    {
        return m_isPopupOpen;
    }

    void ComboBox::RenderPopup(Core::Renderer& renderer)
    {
        const Core::RectF popup{ GetPopupBounds() };
        renderer.FillRoundedRect(popup, Theme::CORNER_RADIUS, Theme::PANEL_BACK_COLOR);
        renderer.DrawRoundedRect(popup, Theme::CORNER_RADIUS, Theme::PANEL_BORDER_COLOR, Theme::BORDER_WIDTH);

        const Core::ClipScope clip{ renderer, popup };
        const std::size_t visibleEnd{ std::min(m_scrollIndex + GetVisibleItemCount(), m_items.size()) };
        float itemTop{ popup.m_top + POPUP_PADDING };

        for (std::size_t index{ m_scrollIndex }; index < visibleEnd; ++index)
        {
            const Core::RectF itemRect{ popup.m_left + POPUP_PADDING, itemTop, popup.m_right - POPUP_PADDING, itemTop + ITEM_HEIGHT };
            if (m_highlightIndex == index) renderer.FillRoundedRect(itemRect, Theme::CORNER_RADIUS, Theme::SELECTION_BACK_COLOR);

            // 지금 선택된 항목은 굵게 + 강조색 (항목 글자색이 있으면 그 색을 유지)
            const bool isSelected{ m_selectedIndex == index };
            const std::optional<Core::Color> itemColor{ GetItemColor(index) };
            const Core::Color& defaultColor{ isSelected ? Theme::TOTAL_TEXT_COLOR : Theme::TEXT_COLOR };
            const Core::RectF textRect{ itemRect.m_left + TEXT_PADDING, itemRect.m_top, itemRect.m_right - TEXT_PADDING, itemRect.m_bottom };
            renderer.DrawString(m_items[index], textRect, itemColor.has_value() ? *itemColor : defaultColor,
                isSelected ? SELECTED_ITEM_STYLE : ITEM_STYLE);

            itemTop += ITEM_HEIGHT;
        }

        // 항목이 더 있으면 오른쪽에 얇은 스크롤 막대 (전체 중 어디를 보고 있는지)
        if (m_items.size() > GetVisibleItemCount())
        {
            const float trackTop{ popup.m_top + POPUP_PADDING };
            const float trackHeight{ popup.GetHeight() - POPUP_PADDING * BOTH_SIDES };
            const float itemCount{ static_cast<float>(m_items.size()) };
            const float thumbTop{ trackTop + trackHeight * static_cast<float>(m_scrollIndex) / itemCount };
            const float thumbHeight{ trackHeight * static_cast<float>(GetVisibleItemCount()) / itemCount };
            const float thumbRight{ popup.m_right - SCROLLBAR_MARGIN };
            renderer.FillRoundedRect(Core::RectF{ thumbRight - SCROLLBAR_WIDTH, thumbTop, thumbRight, thumbTop + thumbHeight },
                SCROLLBAR_WIDTH * HALF, Theme::MUTED_TEXT_COLOR);
        }
    }

    void ComboBox::OnMouseDown(const Core::MouseButtonEvent& event)
    {
        // 펼친 목록 안: 항목 고르기
        if (m_isPopupOpen && GetPopupBounds().Contains(event.m_x, event.m_y))
        {
            if (const std::optional<std::size_t> item{ FindItemAt(event.m_x, event.m_y) }; item.has_value())
            {
                ClosePopup();
                Select(*item);   // 닫은 뒤에 알림 → 알림에서 다른 부품을 바꿔도 목록이 남지 않음
            }
            return;
        }

        // 상자: 펼치기 · 접기
        if (m_isPopupOpen) ClosePopup();
        else OpenPopup();
    }

    void ComboBox::OnMouseMove(const Core::MouseMoveEvent& event)
    {
        if (!m_isPopupOpen) return;

        if (const std::optional<std::size_t> item{ FindItemAt(event.m_x, event.m_y) }; item.has_value()) m_highlightIndex = item;
    }

    bool ComboBox::OnMouseWheel(const Core::MouseWheelEvent& event)
    {
        if (!m_isPopupOpen) return false;

        Scroll(-static_cast<int>(std::lround(event.m_notches * WHEEL_ITEMS_PER_NOTCH)));   // 휠을 위로 = 목록 위쪽
        return true;
    }

    bool ComboBox::OnKeyDown(const Core::KeyEvent& event)
    {
        if (m_items.empty()) return false;

        const std::size_t lastIndex{ m_items.size() - 1 };
        switch (event.m_key)
        {
            case Core::Key::KEY_UP:
            case Core::Key::KEY_DOWN:
            {
                const int step{ event.m_key == Core::Key::KEY_UP ? PREVIOUS_STEP : NEXT_STEP };
                if (m_isPopupOpen)
                {
                    MoveHighlight(step);
                }
                else if (!m_selectedIndex.has_value())
                {
                    Select(0);
                }
                else if (step == PREVIOUS_STEP ? *m_selectedIndex > 0 : *m_selectedIndex < lastIndex)
                {
                    Select(step == PREVIOUS_STEP ? *m_selectedIndex - 1 : *m_selectedIndex + 1);
                }
                return true;
            }

            case Core::Key::KEY_HOME:
            case Core::Key::KEY_END:
            {
                const std::size_t target{ event.m_key == Core::Key::KEY_HOME ? std::size_t{} : lastIndex };
                if (m_isPopupOpen)
                {
                    m_highlightIndex = target;
                    EnsureVisible(target);
                }
                else
                {
                    Select(target);
                }
                return true;
            }

            case Core::Key::KEY_ENTER:
            case Core::Key::KEY_SPACE:
            {
                if (!m_isPopupOpen)
                {
                    OpenPopup();
                    return true;
                }

                const std::optional<std::size_t> highlighted{ m_highlightIndex };
                ClosePopup();
                if (highlighted.has_value()) Select(*highlighted);
                return true;
            }

            case Core::Key::KEY_ESCAPE:
                if (!m_isPopupOpen) return false;   // 접혀 있으면 대화상자 등이 Esc를 쓰도록 양보
                ClosePopup();
                return true;

            case Core::Key::KEY_TAB:
                ClosePopup();
                return false;   // 포커스 이동은 ControlHost가

            default:
                return false;
        }
    }

    void ComboBox::OnFocusChanged(bool isFocused)
    {
        if (!isFocused) ClosePopup();   // 다른 곳을 누르면 접힘
    }

    void ComboBox::OpenPopup()
    {
        if (m_items.empty()) return;

        m_isPopupOpen = true;
        m_highlightIndex = m_selectedIndex;
        EnsureVisible(m_selectedIndex.value_or(0));
    }

    void ComboBox::ClosePopup() noexcept
    {
        m_isPopupOpen = false;
    }

    void ComboBox::Select(std::size_t index)
    {
        if (index >= m_items.size() || m_selectedIndex == index) return;

        m_selectedIndex = index;
        if (!m_onSelectionChanged) return;

        const std::function<void()> onSelectionChanged{ m_onSelectionChanged };   // 콜백 안에서 SetOnSelectionChanged를 불러도 안전하게
        onSelectionChanged();
    }

    void ComboBox::MoveHighlight(int step)
    {
        if (m_items.empty()) return;

        const auto lastIndex{ static_cast<long long>(m_items.size() - 1) };
        const long long current{ m_highlightIndex.has_value() ? static_cast<long long>(*m_highlightIndex) : (step > 0 ? -1LL : lastIndex + 1) };
        const auto next{ static_cast<std::size_t>(std::clamp(current + step, 0LL, lastIndex)) };
        m_highlightIndex = next;
        EnsureVisible(next);
    }

    void ComboBox::EnsureVisible(std::size_t index) noexcept
    {
        const std::size_t visibleCount{ GetVisibleItemCount() };
        if (index < m_scrollIndex) m_scrollIndex = index;
        else if (visibleCount > 0 && index >= m_scrollIndex + visibleCount) m_scrollIndex = index - visibleCount + 1;
        m_scrollIndex = std::min(m_scrollIndex, GetMaxScrollIndex());
    }

    void ComboBox::Scroll(int itemCount) noexcept
    {
        const auto current{ static_cast<long long>(m_scrollIndex) };
        const auto maxIndex{ static_cast<long long>(GetMaxScrollIndex()) };
        m_scrollIndex = static_cast<std::size_t>(std::clamp(current + itemCount, 0LL, maxIndex));
    }

    // 상자 바로 아래, 상자와 같은 너비
    Core::RectF ComboBox::GetPopupBounds() const noexcept
    {
        const Core::RectF& bounds{ GetBounds() };
        const float top{ bounds.m_bottom + POPUP_GAP };
        const float height{ static_cast<float>(GetVisibleItemCount()) * ITEM_HEIGHT + POPUP_PADDING * BOTH_SIDES };
        return Core::RectF{ bounds.m_left, top, bounds.m_right, top + height };
    }

    std::size_t ComboBox::GetVisibleItemCount() const noexcept
    {
        return std::min(m_items.size(), MAX_VISIBLE_ITEMS);
    }

    std::size_t ComboBox::GetMaxScrollIndex() const noexcept
    {
        return m_items.size() - GetVisibleItemCount();
    }

    std::optional<std::size_t> ComboBox::FindItemAt(float x, float y) const noexcept
    {
        const Core::RectF popup{ GetPopupBounds() };
        if (!popup.Contains(x, y)) return std::nullopt;

        const float offset{ y - popup.m_top - POPUP_PADDING };
        if (offset < 0.0f) return std::nullopt;

        const std::size_t row{ static_cast<std::size_t>(offset / ITEM_HEIGHT) };
        if (row >= GetVisibleItemCount()) return std::nullopt;

        const std::size_t index{ m_scrollIndex + row };
        return index < m_items.size() ? std::optional<std::size_t>{ index } : std::nullopt;
    }

    // 오른쪽 ∨ 화살표 (펼쳐져 있으면 ∧)
    void ComboBox::RenderArrow(Core::Renderer& renderer, const Core::Color& color) const
    {
        const Core::RectF& bounds{ GetBounds() };
        const float centerX{ bounds.m_right - ARROW_AREA_WIDTH * HALF };
        const float centerY{ bounds.m_top + bounds.GetHeight() * HALF };
        const float tipOffset{ m_isPopupOpen ? -ARROW_HALF_HEIGHT : ARROW_HALF_HEIGHT };

        renderer.DrawLine(centerX - ARROW_HALF_WIDTH, centerY - tipOffset, centerX, centerY + tipOffset, color, ARROW_LINE_WIDTH);
        renderer.DrawLine(centerX, centerY + tipOffset, centerX + ARROW_HALF_WIDTH, centerY - tipOffset, color, ARROW_LINE_WIDTH);
    }
}