export module LostArkMultiTool.Views.UI.Label;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 글자 표시 (안내 문구 · 오류 문구) — 마우스를 받지 않아 아래 부품을 가리지 않음
    class Label final : public Control
    {
    public:
        Label(std::wstring text, const Core::TextStyle& style, const Core::Color& color);

        void SetText(std::wstring text);
        [[nodiscard]] const std::wstring& GetText() const noexcept;
        void SetColor(const Core::Color& color) noexcept;

        // 지금 글자를 width 너비에 그릴 때 필요한 높이 (여러 줄 안내 문구의 배치 계산용)
        [[nodiscard]] float MeasureHeight(Core::Renderer& renderer, float width) const;

        [[nodiscard]] bool IsHitTestVisible() const noexcept override;
        void Render(Core::Renderer& renderer) override;

    private:
        std::wstring m_text;
        Core::TextStyle m_style;
        Core::Color m_color;
    };
}