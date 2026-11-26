module LostArkMultiTool.Views.OverlayPanel;  // 인게임 오버레이 패널 기반 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.Theme;  // 색 · 글꼴 크기 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float HEADER_HEIGHT{ 28.0f };          // 머리말 높이
        constexpr float MIN_ALPHA{ 0.1f };               // 요소 투명도 최소 (10%)
        constexpr float MAX_ALPHA{ 1.0f };               // 요소 투명도 최대 (100%, 그대로)
        constexpr float CORNER_RADIUS{ 8.0f };           // 카드 모서리
        constexpr float BORDER_WIDTH{ 1.0f };            // 테두리 두께
        constexpr float PADDING{ 10.0f };                // 안쪽 여백
        constexpr float CLOSE_SIZE{ 20.0f };             // 닫기 단추 크기
        constexpr float GRIP_SIZE{ 14.0f };              // 크기 조절 손잡이 크기
        constexpr float GRIP_LINE_GAP{ 4.0f };           // 손잡이 빗금 간격
        constexpr float MIN_PANEL_WIDTH{ 220.0f };       // 최소 가로
        constexpr float MIN_PANEL_HEIGHT{ 120.0f };      // 최소 세로
        constexpr float TITLE_FONT_SIZE{ 13.0f };        // 제목 글자 크기
        constexpr float CLOSE_FONT_SIZE{ 14.0f };        // × 글자 크기
        constexpr float HEADER_LINE_ALPHA{ 0.6f };       // 머리말 아래 선 투명도
        constexpr float HOVER_BACK_ALPHA{ 0.25f };       // 마우스 올림 바탕 투명도
        constexpr float GRIP_ALPHA{ 0.7f };              // 손잡이 선 투명도
        constexpr int GRIP_LINE_COUNT{ 3 };              // 손잡이 빗금 수
        constexpr float HALF{ 0.5f };                    // 절반
        constexpr float GRIP_INSET{ 2.0f };              // 빗금을 모서리에서 띄우는 거리
        constexpr std::wstring_view CLOSE_TEXT{ L"×" };  // 닫기 글자
        constexpr Core::TextStyle TITLE_STYLE{ TITLE_FONT_SIZE, Core::FontWeight::BOLD };   // 제목 글꼴
        constexpr Core::TextStyle CLOSE_STYLE{ CLOSE_FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };   // × 글꼴
    }

    OverlayPanel::OverlayPanel(std::string id, std::wstring title, Core::SizeF defaultSize, bool isResizable)
        : m_id{ std::move(id) }                 // 이름
        , m_title{ std::move(title) }           // 제목
        , m_defaultSize{ defaultSize }          // 기본 크기
        , m_isResizable{ isResizable }          // 크기 조절
        , m_bounds{ 0.0f, 0.0f, defaultSize.m_width, defaultSize.m_height }   // 처음 위치 (매니저가 다시 정함)
    {
    }

    OverlayPanel::~OverlayPanel() = default;   // 기본 소멸

    const std::string& OverlayPanel::GetId() const noexcept { return m_id; }                  // 이름
    const Core::RectF& OverlayPanel::GetBounds() const noexcept { return m_bounds; }          // 위치 · 크기
    void OverlayPanel::SetBounds(const Core::RectF& bounds) noexcept { m_bounds = bounds; }   // 바꾸기
    bool OverlayPanel::IsVisible() const noexcept { return m_isVisible; }                     // 보임
    void OverlayPanel::SetVisible(bool isVisible) noexcept { m_isVisible = isVisible; }       // 보이기 · 숨기기
    Core::SizeF OverlayPanel::GetDefaultSize() const noexcept { return m_defaultSize; }       // 기본 크기
    void OverlayPanel::Update(double) {}                                                       // 기본: 할 일 없음
    std::wstring OverlayPanel::GetTitleText() const { return m_title; }                       // 기본: 제목
    bool OverlayPanel::HandleContentInput(const Core::InputEvent&, const Core::RectF&) { return false; }   // 기본: 안 받음

    bool OverlayPanel::IsContentAvailable() const noexcept
    {
        return true;   // 기본: 늘 보여 줌
    }

    bool OverlayPanel::IsChanged() noexcept
    {
        return std::exchange(m_isChanged, false);   // 확인하면 초기화
    }

    void OverlayPanel::ClampInto(const Core::RectF& area) noexcept
    {
        const float width = std::min(m_bounds.GetWidth(), area.GetWidth());     // 영역보다 크면 줄임
        const float height = std::min(m_bounds.GetHeight(), area.GetHeight());  // 영역보다 크면 줄임
        const float left = std::clamp(m_bounds.m_left, area.m_left, std::max(area.m_left, area.m_right - width));    // 가로 위치
        const float top = std::clamp(m_bounds.m_top, area.m_top, std::max(area.m_top, area.m_bottom - height));     // 세로 위치
        m_bounds = Core::RectF{ left, top, left + width, top + height };        // 영역 안으로
    }

    Core::RectF OverlayPanel::GetHeaderRect() const noexcept
    {
        return Core::RectF{ m_bounds.m_left, m_bounds.m_top, m_bounds.m_right, m_bounds.m_top + HEADER_HEIGHT };   // 위쪽 띠
    }

    Core::RectF OverlayPanel::GetCloseRect() const noexcept
    {
        const float top = m_bounds.m_top + (HEADER_HEIGHT - CLOSE_SIZE) * HALF;   // 세로 가운데
        return Core::RectF{ m_bounds.m_right - PADDING * HALF - CLOSE_SIZE, top, m_bounds.m_right - PADDING * HALF, top + CLOSE_SIZE };   // 오른쪽 끝
    }

    Core::RectF OverlayPanel::GetGripRect() const noexcept
    {
        return Core::RectF{ m_bounds.m_right - GRIP_SIZE, m_bounds.m_bottom - GRIP_SIZE, m_bounds.m_right, m_bounds.m_bottom };   // 오른쪽 아래
    }

    Core::RectF OverlayPanel::GetContentRect() const noexcept
    {
        return Core::RectF{ m_bounds.m_left + PADDING, m_bounds.m_top + HEADER_HEIGHT + PADDING * HALF,
            m_bounds.m_right - PADDING, m_bounds.m_bottom - PADDING };   // 머리말 · 여백을 뺀 영역
    }

    const std::wstring& OverlayPanel::GetTitle() const noexcept
    {
        return m_title;   // 기본 제목
    }

    float OverlayPanel::GetAlpha() const noexcept
    {
        return m_alpha;   // 요소 투명도
    }

    void OverlayPanel::SetAlpha(float alpha) noexcept
    {
        m_alpha = std::clamp(alpha, MIN_ALPHA, MAX_ALPHA);   // 범위 제한
    }

    bool OverlayPanel::IsInteractive() const noexcept
    {
        return m_isInteractive;   // 조작 모드 여부
    }

    void OverlayPanel::SetHeight(float height) noexcept
    {
        m_bounds.m_bottom = m_bounds.m_top + HEADER_HEIGHT + PADDING * 1.5f + height;   // 내용 높이 + 머리말 + 여백
    }

    void OverlayPanel::Render(Core::Renderer& renderer, float backOpacity, bool isInteractive)
    {
        m_isInteractive = isInteractive;                                                              // 내용 그리기에서 쓸 조작 모드
        const float alpha = isInteractive ? MAX_ALPHA : m_alpha;                                      // 조작 모드에서는 옮기기 쉽게 진하게
        std::optional<Core::OpacityScope> fade;                                                       // 요소 투명도 (100%면 레이어 없이)
        if (alpha < MAX_ALPHA) fade.emplace(renderer, alpha);                                         // 패널 전체를 반투명하게
        renderer.FillRoundedRect(m_bounds, CORNER_RADIUS, Core::WithAlpha(Theme::PANEL_BACK_COLOR, backOpacity));                 // 반투명 카드
        renderer.DrawRoundedRect(m_bounds, CORNER_RADIUS, isInteractive ? Theme::ACCENT_COLOR : Theme::PANEL_BORDER_COLOR, BORDER_WIDTH);   // 테두리 (조작 모드는 강조색)

        const Core::RectF header = GetHeaderRect();                                                   // 머리말
        if (isInteractive && m_isHoverHeader)                                                         // 끌 수 있는 곳 표시
            renderer.FillRoundedRect(header, CORNER_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, HOVER_BACK_ALPHA * HALF));
        renderer.DrawString(GetTitleText(), Core::RectF{ header.m_left + PADDING, header.m_top, header.m_right - PADDING - CLOSE_SIZE, header.m_bottom },
            Theme::ACCENT_COLOR, TITLE_STYLE);                                                   // 제목
        renderer.DrawLine(header.m_left + PADDING, header.m_bottom, header.m_right - PADDING, header.m_bottom,
            Core::WithAlpha(Theme::PANEL_BORDER_COLOR, HEADER_LINE_ALPHA), BORDER_WIDTH);             // 머리말 아래 선

        if (isInteractive)                                                                            // 조작 모드에서만 닫기 · 손잡이
        {
            const Core::RectF close = GetCloseRect();                                                 // 닫기 단추
            if (m_isHoverClose) renderer.FillRoundedRect(close, CORNER_RADIUS * HALF, Core::WithAlpha(Theme::NOTICE_ERROR_COLOR, HOVER_BACK_ALPHA));
            renderer.DrawString(CLOSE_TEXT, close, Theme::TEXT_COLOR, CLOSE_STYLE);                  // ×
            if (m_isResizable)                                                                        // 크기 조절 빗금
            {
                for (int line = 1; line <= GRIP_LINE_COUNT; ++line)                                   // 빗금마다
                {
                    const float offset = GRIP_LINE_GAP * static_cast<float>(line);                    // 모서리에서 거리
                    renderer.DrawLine(m_bounds.m_right - offset, m_bounds.m_bottom - GRIP_INSET, m_bounds.m_right - GRIP_INSET, m_bounds.m_bottom - offset,
                        Core::WithAlpha(Theme::MUTED_TEXT_COLOR, GRIP_ALPHA), BORDER_WIDTH);          // 빗금
                }
            }
        }

        const Core::RectF content = GetContentRect();                                                 // 내용 영역
        renderer.PushClip(content);                                                                   // 내용이 카드 밖으로 안 나가게
        RenderContent(renderer, content);                                                             // 파생 패널 내용
        renderer.PopClip();                                                                           // 자르기 끝
    }

    bool OverlayPanel::HandleInput(const Core::InputEvent& event, bool isInteractive)
    {
        if (!isInteractive) return false;                                                             // 평소엔 클릭 통과 (입력 없음)

        if (const auto* move = std::get_if<Core::MouseMoveEvent>(&event))                             // 마우스 이동
        {
            if (m_isDragging)                                                                         // 머리말 끌기
            {
                const float width = m_bounds.GetWidth();                                              // 가로 유지
                const float height = m_bounds.GetHeight();                                            // 세로 유지
                m_bounds = Core::RectF{ move->m_x - m_dragOffset.m_x, move->m_y - m_dragOffset.m_y,
                    move->m_x - m_dragOffset.m_x + width, move->m_y - m_dragOffset.m_y + height };    // 새 위치
                return true;                                                                          // 처리함
            }
            if (m_isResizing)                                                                         // 크기 조절
            {
                m_bounds.m_right = std::max(move->m_x, m_bounds.m_left + MIN_PANEL_WIDTH);            // 새 가로
                m_bounds.m_bottom = std::max(move->m_y, m_bounds.m_top + MIN_PANEL_HEIGHT);           // 새 세로
                return true;                                                                          // 처리함
            }
            m_isHoverClose = GetCloseRect().Contains(move->m_x, move->m_y);                           // 닫기 위
            m_isHoverHeader = !m_isHoverClose && GetHeaderRect().Contains(move->m_x, move->m_y);      // 머리말 위
        }
        else if (const auto* button = std::get_if<Core::MouseButtonEvent>(&event))                    // 마우스 버튼
        {
            const bool isLeft = button->m_button == Core::MouseButton::MOUSE_BUTTON_LEFT;             // 왼쪽 버튼
            if (isLeft && !button->m_isPressed && (m_isDragging || m_isResizing))                     // 끌기 · 조절 끝
            {
                m_isDragging = false;                                                                 // 끌기 끝
                m_isResizing = false;                                                                 // 조절 끝
                m_isChanged = true;                                                                   // 저장 신호
                return true;                                                                          // 처리함
            }
            if (isLeft && button->m_isPressed && m_bounds.Contains(button->m_x, button->m_y))         // 패널 안 누름
            {
                if (GetCloseRect().Contains(button->m_x, button->m_y))                                // 닫기
                {
                    m_isVisible = false;                                                              // 숨기기
                    m_isChanged = true;                                                               // 저장 신호
                    return true;                                                                      // 처리함
                }
                if (m_isResizable && GetGripRect().Contains(button->m_x, button->m_y))                // 손잡이
                {
                    m_isResizing = true;                                                              // 조절 시작
                    return true;                                                                      // 처리함
                }
                if (GetHeaderRect().Contains(button->m_x, button->m_y))                               // 머리말
                {
                    m_isDragging = true;                                                              // 끌기 시작
                    m_dragOffset = Core::PointF{ button->m_x - m_bounds.m_left, button->m_y - m_bounds.m_top };   // 잡은 위치
                    return true;                                                                      // 처리함
                }
            }
        }

        if (HandleContentInput(event, GetContentRect())) return true;                                 // 내용이 처리
        if (const auto* button = std::get_if<Core::MouseButtonEvent>(&event)) return m_bounds.Contains(button->m_x, button->m_y);   // 패널 안 클릭은 삼킴
        if (const auto* wheel = std::get_if<Core::MouseWheelEvent>(&event)) return m_bounds.Contains(wheel->m_x, wheel->m_y);      // 패널 안 휠도 삼킴
        return false;                                                                                 // 처리 안 함
    }

    Core::CursorShape OverlayPanel::GetCursor() const noexcept
    {
        return (m_isHoverClose || m_isHoverHeader || m_isDragging) ? Core::CursorShape::CURSOR_HAND : Core::CursorShape::CURSOR_ARROW;   // 누를 · 끌 곳은 손
    }
}
