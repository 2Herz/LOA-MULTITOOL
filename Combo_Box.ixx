export module LostArkMultiTool.Views.UI.ComboBox;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 목록에서 하나 고르기 (원본 ComboBox DropDownList: 글자 입력 없이 고르기만)
    // 누르면 아래로 목록이 펼쳐지고, 펼친 목록은 다른 부품 위에 그려지며 마우스도 먼저 받음 (ControlHost 팝업)
    class ComboBox final : public Control
    {
    public:
        ComboBox();

        void SetItems(std::vector<std::wstring> items);   // 선택은 해제됨 (항목 글자색도 기본색으로)
        // 항목마다 글자색 (items와 같은 순서, nullopt · 모자란 항목은 기본색) → SetItems 뒤에 호출
        void SetItemColors(std::vector<std::optional<Core::Color>> itemColors);
        [[nodiscard]] std::size_t GetItemCount() const noexcept;

        void SetSelectedIndex(std::optional<std::size_t> index);   // 코드에서 바꿈 (알림 없음, 범위를 벗어나면 선택 해제)
        [[nodiscard]] std::optional<std::size_t> GetSelectedIndex() const noexcept;
        void SetOnSelectionChanged(std::function<void()> onSelectionChanged);   // 사용자가 다른 항목을 골랐을 때

        [[nodiscard]] bool IsFocusable() const noexcept override;
        [[nodiscard]] bool HitTest(float x, float y) const noexcept override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;

        void Render(Core::Renderer& renderer) override;
        [[nodiscard]] bool IsPopupOpen() const noexcept override;
        void RenderPopup(Core::Renderer& renderer) override;

        void OnMouseDown(const Core::MouseButtonEvent& event) override;
        void OnMouseMove(const Core::MouseMoveEvent& event) override;
        [[nodiscard]] bool OnMouseWheel(const Core::MouseWheelEvent& event) override;
        [[nodiscard]] bool OnKeyDown(const Core::KeyEvent& event) override;

    protected:
        void OnFocusChanged(bool isFocused) override;

    private:
        void OpenPopup();
        void ClosePopup() noexcept;
        void Select(std::size_t index);   // 사용자가 고름 (바뀌었으면 알림)
        void MoveHighlight(int step);
        void EnsureVisible(std::size_t index) noexcept;
        void Scroll(int itemCount) noexcept;

        [[nodiscard]] Core::RectF GetPopupBounds() const noexcept;
        [[nodiscard]] std::size_t GetVisibleItemCount() const noexcept;
        [[nodiscard]] std::size_t GetMaxScrollIndex() const noexcept;
        [[nodiscard]] std::optional<std::size_t> FindItemAt(float x, float y) const noexcept;
        void RenderArrow(Core::Renderer& renderer, const Core::Color& color) const;

        [[nodiscard]] std::optional<Core::Color> GetItemColor(std::size_t index) const noexcept;   // 경계 검사 포함

        std::vector<std::wstring> m_items;
        std::vector<std::optional<Core::Color>> m_itemColors;
        std::optional<std::size_t> m_selectedIndex;
        std::optional<std::size_t> m_highlightIndex;   // 펼친 목록에서 마우스 · 방향키로 가리키는 항목
        std::size_t m_scrollIndex{};                   // 펼친 목록 맨 위에 보이는 항목
        bool m_isPopupOpen{};
        std::function<void()> m_onSelectionChanged;
    };
}