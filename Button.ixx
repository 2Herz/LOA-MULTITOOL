export module LostArkMultiTool.Views.UI.Button;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 버튼 모양
    enum class ButtonStyle
    {
        BUTTON_PRIMARY,     // 주요 동작 (저장 · 확인 · 예)
        BUTTON_SECONDARY,   // 보조 동작 (취소 · 아니요 · 설정)
        BUTTON_LINK         // 글자만 있는 링크 (마우스를 올리면 밑줄)
    };

    // 누르면 onClick 호출 (마우스: 버튼 위에서 뗐을 때, 키보드: 포커스 상태에서 Enter · Space)
    class Button final : public Control
    {
    public:
        Button(std::wstring text, ButtonStyle style, std::function<void()> onClick);

        void SetText(std::wstring text);
        void SetOnClick(std::function<void()> onClick);
        void SetFocusable(bool isFocusable) noexcept;   // 탭 막대 버튼처럼 키보드 포커스가 필요 없는 경우 false

        [[nodiscard]] bool IsFocusable() const noexcept override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;

        void Render(Core::Renderer& renderer) override;
        void OnMouseUp(const Core::MouseButtonEvent& event, bool isInside) override;
        [[nodiscard]] bool OnKeyDown(const Core::KeyEvent& event) override;

    private:
        void Click();
        void RenderLink(Core::Renderer& renderer) const;
        [[nodiscard]] Core::Color GetBackColor() const noexcept;

        std::wstring m_text;
        ButtonStyle m_style;
        std::function<void()> m_onClick;
        bool m_isFocusable{ true };
    };
}