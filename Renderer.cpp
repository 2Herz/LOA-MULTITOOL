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

    Renderer::~Renderer() = default;
}