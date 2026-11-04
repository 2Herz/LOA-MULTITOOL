module LostArkMultiTool.Views.UI.Label;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    Label::Label(std::wstring text, const Core::TextStyle& style, const Core::Color& color)
        : m_text{ std::move(text) }
        , m_style{ style }
        , m_color{ color }
    {}

    void Label::SetText(std::wstring text)
    {
        m_text = std::move(text);
    }

    const std::wstring& Label::GetText() const noexcept
    {
        return m_text;
    }

    void Label::SetColor(const Core::Color& color) noexcept
    {
        m_color = color;
    }

    float Label::MeasureHeight(Core::Renderer& renderer, float width) const
    {
        return renderer.MeasureString(m_text, m_style, width).m_height;
    }

    bool Label::IsHitTestVisible() const noexcept
    {
        return false;
    }

    void Label::Render(Core::Renderer& renderer)
    {
        renderer.DrawString(m_text, GetBounds(), m_color, m_style);
    }
}