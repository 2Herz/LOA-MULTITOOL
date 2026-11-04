module LostArkMultiTool.Core.Renderer;

namespace LostArkMultiTool::Core
{
    float RectF::GetWidth() const noexcept
    {
        return m_right - m_left;
    }

    float RectF::GetHeight() const noexcept
    {
        return m_bottom - m_top;
    }

    bool RectF::Contains(float x, float y) const noexcept
    {
        return x >= m_left && x < m_right && y >= m_top && y < m_bottom;
    }

    bool RectF::operator==(const RectF& other) const noexcept
    {
        return m_left == other.m_left && m_top == other.m_top && m_right == other.m_right && m_bottom == other.m_bottom;
    }

    Renderer::~Renderer() = default;

    ClipScope::ClipScope(Renderer& renderer, const RectF& rect)
        : m_renderer{ renderer }
    {
        m_renderer.PushClip(rect);
    }

    ClipScope::~ClipScope()
    {
        m_renderer.PopClip();
    }

    OpacityScope::OpacityScope(Renderer& renderer, float opacity)
        : m_renderer{ renderer }
    {
        m_renderer.PushOpacity(opacity);
    }

    OpacityScope::~OpacityScope()
    {
        m_renderer.PopOpacity();
    }
}