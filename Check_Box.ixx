export module LostArkMultiTool.Views.UI.CheckBox;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 켜고 끄는 상자 + 글자 (원본 CheckBox: "시세갱신" 자동 갱신 켜기)
    // 누르면 바뀌고 onCheckedChanged(바뀐 상태) 호출 (마우스: 부품 위에서 뗐을 때, 키보드: Space · Enter)
    class CheckBox final : public Control
    {
    public:
        CheckBox(std::wstring text, std::function<void(bool isChecked)> onCheckedChanged);

        void SetChecked(bool isChecked) noexcept;   // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsChecked() const noexcept;

        [[nodiscard]] bool IsFocusable() const noexcept override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;

        void Render(Core::Renderer& renderer) override;
        void OnMouseUp(const Core::MouseButtonEvent& event, bool isInside) override;
        [[nodiscard]] bool OnKeyDown(const Core::KeyEvent& event) override;

    private:
        void Toggle();

        std::wstring m_text;
        bool m_isChecked{};
        std::function<void(bool)> m_onCheckedChanged;
    };
}