module LostArkMultiTool.Views.UI.Dialog;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.ControlHost;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr float PANEL_PADDING{ 20.0f };          // 패널 안쪽 여백
        constexpr float TITLE_HEIGHT{ 28.0f };
        constexpr float TITLE_GAP{ 12.0f };              // 제목과 내용 사이
        constexpr float ACCENT_BAR_WIDTH{ 4.0f };        // 제목 왼쪽 색 막대 (알림 종류 표시)
        constexpr float ACCENT_BAR_HEIGHT{ 16.0f };
        constexpr float ACCENT_BAR_GAP{ 10.0f };
        constexpr float ACCENT_BAR_RADIUS{ 2.0f };
        constexpr float SCREEN_MARGIN{ 24.0f };          // 창이 작을 때 패널과 창 가장자리 사이 최소 여백
        constexpr float HALF{ 0.5f };
        constexpr float BOTH_SIDES{ 2.0f };               // 여백을 양쪽(왼쪽 · 오른쪽 또는 위 · 아래)에 적용

        constexpr double FADE_IN_SECONDS{ 0.15 };
        constexpr float FULL_OPACITY{ 1.0f };

        constexpr Core::TextStyle TITLE_STYLE{ Theme::FONT_SIZE_DIALOG_TITLE, Core::FontWeight::BOLD, Core::TextAlign::LEADING };
    }

    Dialog::Dialog(std::wstring title, Core::NoticeKind kind, float width)
        : m_title{ std::move(title) }
        , m_kind{ kind }
        , m_width{ width }
    {}

    Dialog::~Dialog() = default;

    void Dialog::Update(double deltaSeconds)
    {
        m_ageSeconds += deltaSeconds;
        if (!m_isClosed) m_host.Update(deltaSeconds);
    }

    void Dialog::Render(Core::Renderer& renderer, const Core::RectF& screenArea)
    {
        UpdateLayout(renderer, screenArea);

        const float opacity{ GetOpacity() };
        renderer.FillRect(screenArea, Core::WithAlpha(Theme::OVERLAY_DIM_COLOR, Theme::OVERLAY_DIM_COLOR.m_alpha * opacity));

        const Core::OpacityScope fade{ renderer, opacity };
        renderer.FillRoundedRect(m_panelRect, Theme::PANEL_CORNER_RADIUS, Theme::PANEL_BACK_COLOR);
        renderer.DrawRoundedRect(m_panelRect, Theme::PANEL_CORNER_RADIUS, Theme::PANEL_BORDER_COLOR, Theme::BORDER_WIDTH);

        // 제목 (왼쪽 색 막대 = 알림 종류)
        const float titleTop{ m_panelRect.m_top + PANEL_PADDING };
        const float barTop{ titleTop + (TITLE_HEIGHT - ACCENT_BAR_HEIGHT) * HALF };
        const Core::RectF accentBar{ m_panelRect.m_left + PANEL_PADDING, barTop, m_panelRect.m_left + PANEL_PADDING + ACCENT_BAR_WIDTH, barTop + ACCENT_BAR_HEIGHT };
        renderer.FillRoundedRect(accentBar, ACCENT_BAR_RADIUS, Theme::GetNoticeColor(m_kind));

        const Core::RectF titleRect{ accentBar.m_right + ACCENT_BAR_GAP, titleTop, m_panelRect.m_right - PANEL_PADDING, titleTop + TITLE_HEIGHT };
        renderer.DrawString(m_title, titleRect, Theme::HEADER_TEXT_COLOR, TITLE_STYLE);

        m_host.Render(renderer);
    }

    bool Dialog::HandleInput(const Core::InputEvent& event)
    {
        if (m_isClosed) return true;   // 닫히는 중에는 아무 입력도 받지 않음 (모달 유지)

        if (m_host.HandleInput(event)) return true;

        // 부품이 쓰지 않은 Enter · Esc → 대화상자 기본 동작
        if (const auto* key{ std::get_if<Core::KeyEvent>(&event) }; key != nullptr)
        {
            if (key->m_key == Core::Key::KEY_ENTER) OnSubmit();
            else if (key->m_key == Core::Key::KEY_ESCAPE) OnCancel();
        }
        return true;   // 모달: 대화상자 밖으로 입력이 새지 않음
    }

    void Dialog::Close()
    {
        if (m_isClosed) return;

        m_isClosed = true;
        m_host.SuspendFocus();   // 조합 중인 글자 확정 · IME 끔
    }

    bool Dialog::IsClosed() const noexcept
    {
        return m_isClosed;
    }

    void Dialog::Activate()
    {
        if (m_isClosed) return;

        if (!m_isActivated)
        {
            m_isActivated = true;
            FocusInitialControl();
            return;
        }
        m_host.RestoreFocus();
    }

    void Dialog::Deactivate()
    {
        m_host.SuspendFocus();
    }

    Core::CursorShape Dialog::GetCursor() const noexcept
    {
        return m_isClosed ? Core::CursorShape::CURSOR_ARROW : m_host.GetCursor();
    }

    ControlHost& Dialog::GetHost() noexcept
    {
        return m_host;
    }

    void Dialog::FocusInitialControl()
    {
        m_host.FocusNext(false);
    }

    void Dialog::OnSubmit()
    {}

    void Dialog::OnCancel()
    {}

    // 화면 크기가 바뀌었을 때만 패널 크기 · 내용 배치를 다시 계산 (글자 높이 측정이 필요해 그리기 단계에서)
    void Dialog::UpdateLayout(Core::Renderer& renderer, const Core::RectF& screenArea)
    {
        if (m_hasLayout && m_layoutScreen == screenArea) return;

        const float panelWidth{ std::min(m_width, std::max(0.0f, screenArea.GetWidth() - SCREEN_MARGIN * BOTH_SIDES)) };
        const float contentWidth{ std::max(0.0f, panelWidth - PANEL_PADDING * BOTH_SIDES) };
        const float contentHeight{ MeasureContentHeight(renderer, contentWidth) };
        const float panelHeight{ PANEL_PADDING + TITLE_HEIGHT + TITLE_GAP + contentHeight + PANEL_PADDING };

        const float panelLeft{ screenArea.m_left + (screenArea.GetWidth() - panelWidth) * HALF };
        const float panelTop{ screenArea.m_top + std::max(SCREEN_MARGIN, (screenArea.GetHeight() - panelHeight) * HALF) };
        m_panelRect = Core::RectF{ panelLeft, panelTop, panelLeft + panelWidth, panelTop + panelHeight };

        const float contentTop{ panelTop + PANEL_PADDING + TITLE_HEIGHT + TITLE_GAP };
        LayoutContent(Core::RectF{ panelLeft + PANEL_PADDING, contentTop, panelLeft + PANEL_PADDING + contentWidth, contentTop + contentHeight });

        m_layoutScreen = screenArea;
        m_hasLayout = true;
    }

    float Dialog::GetOpacity() const noexcept
    {
        return static_cast<float>(std::min(m_ageSeconds / FADE_IN_SECONDS, static_cast<double>(FULL_OPACITY)));
    }
}