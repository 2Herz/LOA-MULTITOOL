module LostArkMultiTool.Views.UI.ToastHost;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr std::size_t MAX_TOAST_COUNT{ 4 };       // 넘치면 오래된 것부터 지움
        constexpr double TOAST_LIFE_SECONDS{ 3.0 };
        constexpr double ERROR_TOAST_LIFE_SECONDS{ 5.0 }; // 오류는 읽을 시간을 더 줌
        constexpr double FADE_IN_SECONDS{ 0.2 };
        constexpr double FADE_OUT_SECONDS{ 0.4 };
        constexpr float FULL_OPACITY{ 1.0f };

        // ===== 배치 (DIP) =====
        constexpr float TOAST_WIDTH{ 320.0f };
        constexpr float TOAST_PADDING{ 12.0f };
        constexpr float TOAST_GAP{ 8.0f };                // 알림 사이 간격
        constexpr float SCREEN_MARGIN{ 16.0f };           // 창 오른쪽 · 아래 여백
        constexpr float ACCENT_BAR_WIDTH{ 4.0f };         // 왼쪽 색 막대 (알림 종류)
        constexpr float TEXT_LEFT_GAP{ 10.0f };
        constexpr float SLIDE_DISTANCE{ 12.0f };          // 나타날 때 아래에서 올라오는 거리
        constexpr float BOTH_SIDES{ 2.0f };               // 여백을 양쪽(왼쪽 · 오른쪽 또는 위 · 아래)에 적용

        constexpr Core::TextStyle MESSAGE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_TOP, Core::TextWrap::WORD_WRAP };
    }

    ToastHost::ToastHost() = default;

    void ToastHost::Add(std::wstring message, Core::NoticeKind kind)
    {
        const double lifeSeconds{ kind == Core::NoticeKind::NOTICE_ERROR ? ERROR_TOAST_LIFE_SECONDS : TOAST_LIFE_SECONDS };
        m_toasts.push_back(Toast{ std::move(message), kind, 0.0, lifeSeconds });

        while (m_toasts.size() > MAX_TOAST_COUNT) m_toasts.pop_front();
    }

    void ToastHost::Update(double deltaSeconds)
    {
        for (Toast& toast : m_toasts) toast.m_ageSeconds += deltaSeconds;

        std::erase_if(m_toasts, [](const Toast& toast) { return toast.m_ageSeconds >= toast.m_lifeSeconds; });
    }

    void ToastHost::Render(Core::Renderer& renderer, const Core::RectF& screenArea)
    {
        const float toastWidth{ std::min(TOAST_WIDTH, std::max(0.0f, screenArea.GetWidth() - SCREEN_MARGIN * BOTH_SIDES)) };
        const float textWidth{ std::max(0.0f, toastWidth - TOAST_PADDING * BOTH_SIDES - ACCENT_BAR_WIDTH - TEXT_LEFT_GAP) };
        const float right{ screenArea.m_right - SCREEN_MARGIN };
        float bottom{ screenArea.m_bottom - SCREEN_MARGIN };

        // 최근 알림부터 아래에서 위로 쌓음
        for (const Toast& toast : std::views::reverse(m_toasts))
        {
            const float opacity{ GetOpacity(toast) };
            const float textHeight{ renderer.MeasureString(toast.m_message, MESSAGE_STYLE, textWidth).m_height };
            const float toastHeight{ textHeight + TOAST_PADDING * BOTH_SIDES };
            const float slide{ (FULL_OPACITY - opacity) * SLIDE_DISTANCE };   // 나타날 때 · 사라질 때 살짝 아래로

            const Core::RectF panel{ right - toastWidth, bottom - toastHeight + slide, right, bottom + slide };
            {
                const Core::OpacityScope fade{ renderer, opacity };
                renderer.FillRoundedRect(panel, Theme::PANEL_CORNER_RADIUS, Theme::PANEL_BACK_COLOR);
                renderer.DrawRoundedRect(panel, Theme::PANEL_CORNER_RADIUS, Theme::PANEL_BORDER_COLOR, Theme::BORDER_WIDTH);

                const Core::RectF accentBar{ panel.m_left + TOAST_PADDING, panel.m_top + TOAST_PADDING,
                    panel.m_left + TOAST_PADDING + ACCENT_BAR_WIDTH, panel.m_bottom - TOAST_PADDING };
                renderer.FillRect(accentBar, Theme::GetNoticeColor(toast.m_kind));

                const Core::RectF textRect{ accentBar.m_right + TEXT_LEFT_GAP, panel.m_top + TOAST_PADDING,
                    accentBar.m_right + TEXT_LEFT_GAP + textWidth, panel.m_bottom - TOAST_PADDING };
                renderer.DrawString(toast.m_message, textRect, Theme::TEXT_COLOR, MESSAGE_STYLE);
            }

            bottom -= toastHeight + TOAST_GAP;
            if (bottom < screenArea.m_top) break;   // 창 위로 넘치는 알림은 그리지 않음
        }
    }

    // 처음엔 서서히 나타나고, 끝나기 직전엔 서서히 사라짐
    float ToastHost::GetOpacity(const Toast& toast) noexcept
    {
        const double fadeIn{ toast.m_ageSeconds / FADE_IN_SECONDS };
        const double fadeOut{ (toast.m_lifeSeconds - toast.m_ageSeconds) / FADE_OUT_SECONDS };
        return static_cast<float>(std::clamp(std::min(fadeIn, fadeOut), 0.0, static_cast<double>(FULL_OPACITY)));
    }
}