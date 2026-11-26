module LostArkMultiTool.Views.MiniToolPanel;  // 오버레이 탭 미니 모드 패널 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.ToolManager;  // 탭 매니저 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float DEFAULT_WIDTH{ 1000.0f };          // 기본 가로 (탭 화면이 찌그러지지 않는 정도)
        constexpr float DEFAULT_HEIGHT{ 640.0f };          // 기본 세로
        constexpr float SWITCHER_HEIGHT{ 26.0f };          // 탭 고르기 줄 높이
        constexpr float SWITCHER_GAP{ 6.0f };              // 탭 고르기 줄과 탭 화면 사이
        constexpr float ARROW_WIDTH{ 30.0f };              // ◀ ▶ 단추 너비
        constexpr float ARROW_RADIUS{ 5.0f };              // 단추 모서리
        constexpr float FONT_SIZE{ 13.0f };                // 글자 크기
        constexpr float ARROW_BACK_ALPHA{ 0.35f };         // 단추 바탕 불투명도
        constexpr float TOOL_BACK_ALPHA{ 0.96f };          // 탭 화면 바탕 불투명도 (글자가 게임 화면과 겹쳐 안 보이지 않게)
        constexpr std::wstring_view PREVIOUS_TEXT{ L"◀" }; // 이전 탭
        constexpr std::wstring_view NEXT_TEXT{ L"▶" };     // 다음 탭
        constexpr std::wstring_view TITLE_FORMAT{ L"미니 모드 · {}" };   // 제목
        constexpr std::wstring_view SWITCHER_FORMAT{ L"{}  ({}/{})" };  // 탭 이름 (번호/전체)
        constexpr Core::TextStyle CENTER_STYLE{ FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };   // 가운데 글자
    }

    MiniToolPanel::MiniToolPanel(std::span<const std::reference_wrapper<Core::ToolManager>> tools, std::size_t initialIndex,
        std::function<void(std::size_t index)> onToolChanged)
        : OverlayPanel{ std::string{ PANEL_ID }, std::wstring{}, Core::SizeF{ DEFAULT_WIDTH, DEFAULT_HEIGHT }, true }   // 크기 조절 패널
        , m_tools(tools.begin(), tools.end())                                     // 탭들
        , m_toolIndex{ tools.empty() ? 0 : std::min(initialIndex, tools.size() - 1) }   // 처음 탭 (범위 안으로)
        , m_onToolChanged{ std::move(onToolChanged) }                             // 알림
    {
        if (m_tools.empty()) throw std::invalid_argument("MiniToolPanel: 탭이 없어요");   // 탭이 있어야 함
    }

    std::size_t MiniToolPanel::GetToolIndex() const noexcept
    {
        return m_toolIndex;   // 지금 탭
    }

    Core::ToolManager& MiniToolPanel::GetTool() const
    {
        return m_tools[m_toolIndex].get();   // 지금 탭 (번호는 생성 · 선택 때 범위 검사)
    }

    std::wstring MiniToolPanel::GetTitleText() const
    {
        return std::format(TITLE_FORMAT, GetTool().GetTabTitle());   // "미니 모드 · 귀속재료 계산기"
    }

    Core::RectF MiniToolPanel::GetSwitcherRect(const Core::RectF& content) const noexcept
    {
        return Core::RectF{ content.m_left, content.m_top, content.m_right, content.m_top + SWITCHER_HEIGHT };   // 위쪽 줄
    }

    Core::RectF MiniToolPanel::GetToolRect(const Core::RectF& content) const noexcept
    {
        return Core::RectF{ content.m_left, content.m_top + SWITCHER_HEIGHT + SWITCHER_GAP, content.m_right, content.m_bottom };   // 나머지
    }

    void MiniToolPanel::SelectTool(std::size_t index)
    {
        if (index >= m_tools.size() || index == m_toolIndex) return;   // 범위 밖 · 그대로
        GetTool().OnDeactivated();                                     // 예전 탭 포커스 내려놓기
        m_toolIndex = index;                                           // 바꾸기
        GetTool().OnActivated();                                       // 새 탭 포커스
        m_isPressedInTool = false;                                     // 끌기 상태 초기화
        if (m_onToolChanged) m_onToolChanged(m_toolIndex);             // 알림 (설정 저장)
    }

    void MiniToolPanel::RenderContent(Core::Renderer& renderer, const Core::RectF& content)
    {
        const Core::RectF switcher = GetSwitcherRect(content);                                         // 탭 고르기 줄
        const Core::RectF previous{ switcher.m_left, switcher.m_top, switcher.m_left + ARROW_WIDTH, switcher.m_bottom };   // ◀
        const Core::RectF next{ switcher.m_right - ARROW_WIDTH, switcher.m_top, switcher.m_right, switcher.m_bottom };     // ▶
        renderer.FillRoundedRect(previous, ARROW_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, ARROW_BACK_ALPHA));          // ◀ 바탕
        renderer.FillRoundedRect(next, ARROW_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, ARROW_BACK_ALPHA));              // ▶ 바탕
        renderer.DrawString(PREVIOUS_TEXT, previous, Theme::TEXT_COLOR, CENTER_STYLE);                                     // ◀
        renderer.DrawString(NEXT_TEXT, next, Theme::TEXT_COLOR, CENTER_STYLE);                                             // ▶
        renderer.DrawString(std::format(SWITCHER_FORMAT, GetTool().GetTabTitle(), m_toolIndex + 1, m_tools.size()),
            Core::RectF{ previous.m_right, switcher.m_top, next.m_left, switcher.m_bottom }, Theme::ACCENT_COLOR, CENTER_STYLE);   // 탭 이름

        const Core::RectF toolRect = GetToolRect(content);                                             // 탭 화면
        renderer.FillRect(toolRect, Core::WithAlpha(Theme::BACKGROUND_COLOR, TOOL_BACK_ALPHA));        // 거의 불투명한 바탕 (글자 읽기 쉽게)
        renderer.PushClip(toolRect);                                                                   // 탭 화면 밖으로 안 나가게
        GetTool().Render(renderer, toolRect);                                                          // 탭 그대로 그리기
        renderer.PopClip();                                                                            // 자르기 끝
    }

    bool MiniToolPanel::HandleContentInput(const Core::InputEvent& event, const Core::RectF& content)
    {
        const Core::RectF switcher = GetSwitcherRect(content);                                         // 탭 고르기 줄
        const Core::RectF toolRect = GetToolRect(content);                                             // 탭 화면

        if (const auto* button = std::get_if<Core::MouseButtonEvent>(&event))                          // 마우스 버튼
        {
            if (!m_isPressedInTool && switcher.Contains(button->m_x, button->m_y))                     // 탭 고르기 줄
            {
                if (button->m_isPressed && button->m_button == Core::MouseButton::MOUSE_BUTTON_LEFT)   // 누를 때
                {
                    const std::size_t count = m_tools.size();                                          // 탭 수
                    if (button->m_x < switcher.m_left + ARROW_WIDTH) SelectTool((m_toolIndex + count - 1) % count);   // ◀ 이전 (처음이면 끝으로)
                    else if (button->m_x > switcher.m_right - ARROW_WIDTH) SelectTool((m_toolIndex + 1) % count);     // ▶ 다음 (끝이면 처음으로)
                }
                return true;                                                                           // 줄 위 입력은 삼킴
            }
            const bool isInside = toolRect.Contains(button->m_x, button->m_y);                         // 탭 화면 안
            if (!isInside && !m_isPressedInTool) return false;                                         // 탭 화면 밖 (끌기 중 아님)
            if (button->m_isPressed && isInside) m_isPressedInTool = true;                             // 끌기 시작
            if (!button->m_isPressed) m_isPressedInTool = false;                                       // 끌기 끝
            (void)GetTool().HandleInput(event);                                                        // 탭에 전달
            return true;                                                                               // 처리함
        }
        if (const auto* move = std::get_if<Core::MouseMoveEvent>(&event))                              // 마우스 이동
        {
            m_isHoverTool = toolRect.Contains(move->m_x, move->m_y);                                   // 탭 화면 위인지
            if (m_isHoverTool || m_isPressedInTool) { (void)GetTool().HandleInput(event); return true; }   // 탭에 전달 (끌기 중이면 밖이어도)
            return false;                                                                              // 밖
        }
        if (const auto* wheel = std::get_if<Core::MouseWheelEvent>(&event))                            // 휠
        {
            if (!toolRect.Contains(wheel->m_x, wheel->m_y)) return false;                              // 밖
            (void)GetTool().HandleInput(event);                                                        // 탭에 전달
            return true;                                                                               // 처리함
        }
        if (std::holds_alternative<Core::MouseLeaveEvent>(event))                                      // 마우스가 오버레이 밖으로
        {
            m_isHoverTool = false;                                                                     // 위에 없음
            (void)GetTool().HandleInput(event);                                                        // 탭에 전달 (마우스 올림 표시 지우기)
            return false;                                                                              // 다른 패널도 받게
        }
        return GetTool().HandleInput(event);                                                           // 키보드 · 글자 → 탭 입력칸
    }

    Core::CursorShape MiniToolPanel::GetCursor() const noexcept
    {
        if (m_isHoverTool || m_isPressedInTool) return GetTool().GetCursor();   // 탭 화면의 커서 (입력칸 · 단추)
        return OverlayPanel::GetCursor();                                       // 머리말 · 닫기 커서
    }
}
