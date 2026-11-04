module LostArkMultiTool.Views.UI.ProgressBar;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr double CYCLE_SECONDS{ 1.2 };       // 막대가 한 번 지나가는 시간
        constexpr double FULL_CYCLE{ 1.0 };
        constexpr float SEGMENT_RATIO{ 0.3f };       // 흐르는 막대 길이 (전체 너비 대비)
        constexpr float HALF{ 0.5f };
    }

    ProgressBar::ProgressBar() = default;

    bool ProgressBar::IsHitTestVisible() const noexcept
    {
        return false;
    }

    void ProgressBar::Update(double deltaSeconds)
    {
        m_phase = std::fmod(m_phase + deltaSeconds / CYCLE_SECONDS, FULL_CYCLE);
    }

    void ProgressBar::Render(Core::Renderer& renderer)
    {
        const Core::RectF& bounds{ GetBounds() };
        const float radius{ bounds.GetHeight() * HALF };
        renderer.FillRoundedRect(bounds, radius, Theme::INPUT_BACK_COLOR);

        // 막대는 왼쪽 밖에서 들어와 오른쪽 밖으로 나감 → 보이는 부분만 그림
        const float width{ bounds.GetWidth() };
        const float segmentWidth{ width * SEGMENT_RATIO };
        const float segmentLeft{ bounds.m_left - segmentWidth + (width + segmentWidth) * static_cast<float>(m_phase) };
        const Core::RectF segment{ std::max(segmentLeft, bounds.m_left), bounds.m_top,
            std::min(segmentLeft + segmentWidth, bounds.m_right), bounds.m_bottom };

        if (segment.GetWidth() > 0.0f) renderer.FillRoundedRect(segment, radius, Theme::TOTAL_TEXT_COLOR);
        renderer.DrawRoundedRect(bounds, radius, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);
    }
}