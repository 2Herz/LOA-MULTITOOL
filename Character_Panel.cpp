module LostArkMultiTool.Views.CharacterPanel;  // 오버레이 현재 캐릭터 패널 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.CalculatorManager;  // 현재 캐릭터 요약 형식 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.Theme;  // 색 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수
    {
        constexpr float DEFAULT_WIDTH{ 320.0f };          // 기본 가로
        constexpr float DEFAULT_HEIGHT{ 90.0f };          // 기본 세로 (내용에 맞춰 다시 정함)
        constexpr float ROW_HEIGHT{ 22.0f };              // 줄 높이
        constexpr float BUTTON_WIDTH{ 46.0f };            // 단추 가로
        constexpr float BUTTON_HEIGHT{ 20.0f };           // 단추 세로
        constexpr float BUTTON_RADIUS{ 5.0f };            // 단추 모서리
        constexpr float BUTTON_GAP{ 6.0f };               // 글자와 단추 사이
        constexpr float NAME_GAP{ 8.0f };                 // 이름과 직업 · 레벨 사이
        constexpr float NAME_FONT_SIZE{ 14.0f };          // 이름 글자 크기
        constexpr float DETAIL_FONT_SIZE{ 12.0f };        // 직업 · 레벨 글자 크기
        constexpr float NOTE_FONT_SIZE{ 11.0f };          // 둘째 줄 글자 크기
        constexpr float BUTTON_FONT_SIZE{ 11.0f };        // 단추 글자 크기
        constexpr float BORDER_WIDTH{ 1.0f };             // 테두리 두께
        constexpr float HOVER_ALPHA{ 0.30f };             // 마우스 올림 바탕 투명도
        constexpr float BUTTON_BACK_ALPHA{ 0.12f };       // 단추 바탕 투명도
        constexpr float ROW_HOVER_ALPHA{ 0.18f };         // 목록 줄 마우스 올림 바탕 투명도
        constexpr float HALF{ 0.5f };                     // 절반
        constexpr double REFRESH_SECONDS{ 0.3 };          // 요약 다시 받는 간격
        constexpr std::size_t NORMAL_ROW_COUNT{ 2 };      // 평소 줄 수 (이름 · 상태)
        constexpr std::size_t PICK_HEADER_ROWS{ 1 };      // 고르기 안내 줄 수
        constexpr std::size_t MAX_PICK_ROWS{ 8 };         // 고르기 목록에 한 번에 보일 후보 수 (넘으면 휠)
        constexpr std::wstring_view TITLE_TEXT{ L"현재 캐릭터" };                          // 제목
        constexpr std::wstring_view PICK_TITLE_TEXT{ L"현재 캐릭터 · 고르기" };              // 고르기 중 제목
        constexpr std::wstring_view CHANGE_TEXT{ L"변경" };                                // [변경]
        constexpr std::wstring_view CANCEL_TEXT{ L"취소" };                                // [취소]
        constexpr std::wstring_view REFRESH_TEXT{ L"갱신" };                               // [갱신]
        constexpr std::wstring_view CHECKING_TEXT{ L"캐릭터 확인 중..." };                  // 확인 중
        constexpr std::wstring_view CHECKING_HINT{ L"머리 위 이름이 보이게 하거나 캐릭터 정보(P) 창을 열어 주세요" };
        constexpr std::wstring_view VISION_OFF_TEXT{ L"화면 인식이 꺼져 있어요" };          // 꺼짐
        constexpr std::wstring_view VISION_OFF_HINT{ L"귀속재료 계산기에서 [화면 인식]을 켜 주세요" };
        constexpr std::wstring_view NO_ROSTER_TEXT{ L"원정대를 아직 불러오지 않았어요" };   // 원정대 없음
        constexpr std::wstring_view NO_ROSTER_HINT{ L"귀속재료 계산기에서 원정대를 불러와 주세요" };
        constexpr std::wstring_view DETAIL_FORMAT{ L"{} · {:.2f}" };                       // 직업 · 레벨
        constexpr std::wstring_view LEVEL_FORMAT{ L"{:.2f}" };                             // 레벨만
        constexpr std::wstring_view SOURCE_PLATE_TEXT{ L"머리 위 이름표로 알아봤어요" };     // 알아본 방법
        constexpr std::wstring_view SOURCE_INFO_TEXT{ L"캐릭터 정보 창으로 알아봤어요" };
        constexpr std::wstring_view SOURCE_MANUAL_TEXT{ L"직접 골랐어요" };
        constexpr std::wstring_view AUTO_OFF_SUFFIX{ L" · 자동 인식 꺼짐" };               // 자동 인식 꺼짐 표시
        constexpr std::wstring_view PASSIVE_CHANGE_HINT{ L" · Ctrl+Shift+I로 변경" };       // 평소엔 단추 대신 안내
        constexpr std::wstring_view OUTDATED_FORMAT{ L"아이템 레벨이 달라요 (저장 {:.2f} → 화면 {:.2f})" };   // 경고
        constexpr std::wstring_view SYNCING_TEXT{ L"원정대 정보를 받는 중..." };            // 갱신 중
        constexpr std::wstring_view PICK_HINT_TEXT{ L"화면 인식 값을 넣을 캐릭터를 골라 주세요" };   // 고르기 안내
        constexpr std::wstring_view CURRENT_MARK_FORMAT{ L"●  {}" };                       // 지금 캐릭터 표시
        constexpr std::wstring_view OTHER_MARK_FORMAT{ L"    {}" };                        // 다른 캐릭터 (들여쓰기 맞춤)
        constexpr Core::TextStyle NAME_STYLE{ NAME_FONT_SIZE, Core::FontWeight::BOLD };                                     // 이름
        constexpr Core::TextStyle DETAIL_STYLE{ DETAIL_FONT_SIZE };                                                         // 직업 · 레벨
        constexpr Core::TextStyle NOTE_STYLE{ NOTE_FONT_SIZE };                                                             // 둘째 줄
        constexpr Core::TextStyle BUTTON_STYLE{ BUTTON_FONT_SIZE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };        // 단추
        constexpr Core::TextStyle ROW_STYLE{ DETAIL_FONT_SIZE };                                                            // 목록 줄

        // 작은 단추 하나 그리기
        void DrawButton(Core::Renderer& renderer, const Core::RectF& rect, std::wstring_view text, bool isHover)
        {
            renderer.FillRoundedRect(rect, BUTTON_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, isHover ? HOVER_ALPHA : BUTTON_BACK_ALPHA));   // 바탕
            renderer.DrawRoundedRect(rect, BUTTON_RADIUS, Theme::ACCENT_COLOR, BORDER_WIDTH);                                                // 테두리
            renderer.DrawString(text, rect, Theme::TEXT_COLOR, BUTTON_STYLE);                                                                // 글자
        }
    }

    CharacterPanel::CharacterPanel(std::function<CurrentCharacterSummary()> provider, std::function<void(std::size_t index)> onPick, std::function<void()> onRefresh)
        : OverlayPanel{ std::string{ PANEL_ID }, std::wstring{ TITLE_TEXT }, Core::SizeF{ DEFAULT_WIDTH, DEFAULT_HEIGHT }, false }   // 크기 고정 패널
        , m_provider{ std::move(provider) }    // 요약 함수
        , m_onPick{ std::move(onPick) }        // 고르기
        , m_onRefresh{ std::move(onRefresh) }  // 갱신
    {
    }

    void CharacterPanel::Update(double deltaSeconds)
    {
        if (m_isPicking && !IsInteractive()) m_isPicking = false;              // 조작 모드가 끝나면 고르기도 끝
        m_refreshElapsed += deltaSeconds;                                      // 시간 누적
        if (!m_hasSummary || m_refreshElapsed >= REFRESH_SECONDS)              // 받을 때
        {
            m_refreshElapsed = 0.0;                                            // 초기화
            if (m_provider) m_summary = m_provider();                          // 요약 받기
            m_hasSummary = true;                                               // 받음
            if (m_summary.m_candidates.empty()) m_isPicking = false;           // 고를 후보가 없으면 고르기 끝
        }
        UpdateHeight();                                                        // 높이 맞추기
    }

    std::wstring CharacterPanel::GetTitleText() const
    {
        return std::wstring{ m_isPicking ? PICK_TITLE_TEXT : TITLE_TEXT };   // 제목
    }

    std::size_t CharacterPanel::GetVisibleRowCount() const noexcept
    {
        return std::min(m_summary.m_candidates.size(), MAX_PICK_ROWS);       // 후보 수 (최대 8)
    }

    bool CharacterPanel::CanRefresh() const noexcept
    {
        return m_summary.m_state == CurrentCharacterState::CHARACTER_KNOWN && m_summary.m_isOutdated && !m_summary.m_isSyncing;   // 경고 중 · 갱신 중 아님
    }

    void CharacterPanel::UpdateHeight()
    {
        const std::size_t rows = m_isPicking ? PICK_HEADER_ROWS + GetVisibleRowCount() : NORMAL_ROW_COUNT;   // 줄 수
        SetHeight(static_cast<float>(rows) * ROW_HEIGHT);                      // 내용 높이
    }

    Core::RectF CharacterPanel::GetChangeRect(const Core::RectF& content) const noexcept
    {
        const float top = content.m_top + (ROW_HEIGHT - BUTTON_HEIGHT) * HALF;  // 첫 줄 세로 가운데
        return Core::RectF{ content.m_right - BUTTON_WIDTH, top, content.m_right, top + BUTTON_HEIGHT };   // 첫 줄 오른쪽
    }

    Core::RectF CharacterPanel::GetRefreshRect(const Core::RectF& content) const noexcept
    {
        const float top = content.m_top + ROW_HEIGHT + (ROW_HEIGHT - BUTTON_HEIGHT) * HALF;   // 둘째 줄 세로 가운데
        return Core::RectF{ content.m_right - BUTTON_WIDTH, top, content.m_right, top + BUTTON_HEIGHT };   // 둘째 줄 오른쪽
    }

    Core::RectF CharacterPanel::GetRowRect(const Core::RectF& content, std::size_t row) const noexcept
    {
        const float top = content.m_top + ROW_HEIGHT * static_cast<float>(PICK_HEADER_ROWS + row);   // 안내 줄 아래
        return Core::RectF{ content.m_left, top, content.m_right, top + ROW_HEIGHT };                // 한 줄
    }

    void CharacterPanel::RenderContent(Core::Renderer& renderer, const Core::RectF& content)
    {
        const bool isInteractive = IsInteractive();                                                          // 조작 모드 (단추를 보임)
        const bool canChange = isInteractive && !m_summary.m_candidates.empty()
            && (m_summary.m_state == CurrentCharacterState::CHARACTER_KNOWN || m_summary.m_state == CurrentCharacterState::CHARACTER_CHECKING);   // [변경] 보일 때
        const float textRight = canChange ? content.m_right - BUTTON_WIDTH - BUTTON_GAP : content.m_right;  // 첫 줄 글자 오른쪽 끝
        const Core::RectF firstRow{ content.m_left, content.m_top, textRight, content.m_top + ROW_HEIGHT };   // 첫 줄
        const Core::RectF secondRow{ content.m_left, content.m_top + ROW_HEIGHT, content.m_right, content.m_top + ROW_HEIGHT * NORMAL_ROW_COUNT };   // 둘째 줄

        if (m_isPicking)                                                                                     // 고르기 목록
        {
            renderer.DrawString(PICK_HINT_TEXT, firstRow, Theme::MUTED_TEXT_COLOR, NOTE_STYLE);             // 안내
            DrawButton(renderer, GetChangeRect(content), CANCEL_TEXT, m_isHoverChange);                     // [취소]
            for (std::size_t row = 0; row < GetVisibleRowCount(); ++row)                                    // 보이는 줄마다
            {
                const std::size_t index = m_scrollIndex + row;                                              // 후보 번호
                if (index >= m_summary.m_candidates.size()) break;                                          // 끝
                const Core::RectF rect = GetRowRect(content, row);                                          // 줄 영역
                if (m_hoverRow == index) renderer.FillRect(rect, Core::WithAlpha(Theme::ACCENT_COLOR, ROW_HOVER_ALPHA));   // 마우스 올림
                const bool isCurrent = m_summary.m_currentIndex == index;                                   // 지금 캐릭터
                const std::wstring label = isCurrent ? std::format(CURRENT_MARK_FORMAT, m_summary.m_candidates[index])
                    : std::format(OTHER_MARK_FORMAT, m_summary.m_candidates[index]);                         // 표시 글자 (형식 글자는 상수여야 해서 따로)
                renderer.DrawString(label, rect, isCurrent ? Theme::ACCENT_COLOR : Theme::TEXT_COLOR, ROW_STYLE);   // 이름
            }
            return;                                                                                          // 목록만
        }

        if (canChange) DrawButton(renderer, GetChangeRect(content), CHANGE_TEXT, m_isHoverChange);          // [변경]
        switch (m_summary.m_state)                                                                           // 상태별
        {
        case CurrentCharacterState::CHARACTER_VISION_OFF:                                                    // 꺼짐
            renderer.DrawString(VISION_OFF_TEXT, firstRow, Theme::MUTED_TEXT_COLOR, DETAIL_STYLE);
            renderer.DrawString(VISION_OFF_HINT, secondRow, Theme::MUTED_TEXT_COLOR, NOTE_STYLE);
            return;
        case CurrentCharacterState::CHARACTER_NO_ROSTER:                                                     // 원정대 없음
            renderer.DrawString(NO_ROSTER_TEXT, firstRow, Theme::CAUTION_TEXT_COLOR, DETAIL_STYLE);
            renderer.DrawString(NO_ROSTER_HINT, secondRow, Theme::MUTED_TEXT_COLOR, NOTE_STYLE);
            return;
        case CurrentCharacterState::CHARACTER_CHECKING:                                                      // 확인 중
            renderer.DrawString(CHECKING_TEXT, firstRow, Theme::CAUTION_TEXT_COLOR, DETAIL_STYLE);
            renderer.DrawString(CHECKING_HINT, secondRow, Theme::MUTED_TEXT_COLOR, NOTE_STYLE);
            return;
        case CurrentCharacterState::CHARACTER_KNOWN:                                                         // 확인됨 (아래에서 그림)
            break;
        }

        // 첫 줄: 이름 (굵게) + 직업 · 레벨 (흐리게)
        const float nameWidth = std::min(renderer.MeasureString(m_summary.m_name, NAME_STYLE, firstRow.GetWidth()).m_width, firstRow.GetWidth());   // 이름 너비
        renderer.DrawString(m_summary.m_name, firstRow, Theme::HEADER_TEXT_COLOR, NAME_STYLE);              // 이름
        const std::wstring detail = m_summary.m_itemLevel <= 0.0 ? std::wstring{}
            : (m_summary.m_className.empty() ? std::format(LEVEL_FORMAT, m_summary.m_itemLevel) : std::format(DETAIL_FORMAT, m_summary.m_className, m_summary.m_itemLevel));   // 직업 · 레벨
        renderer.DrawString(detail, Core::RectF{ firstRow.m_left + nameWidth + NAME_GAP, firstRow.m_top, firstRow.m_right, firstRow.m_bottom },
            Theme::MUTED_TEXT_COLOR, DETAIL_STYLE);                                                          // 직업 · 레벨

        // 둘째 줄: 경고 · 갱신 중 · 알아본 방법
        if (m_summary.m_isSyncing)                                                                           // 갱신 중
        {
            renderer.DrawString(SYNCING_TEXT, secondRow, Theme::MUTED_TEXT_COLOR, NOTE_STYLE);
            return;
        }
        if (m_summary.m_isOutdated && m_summary.m_readItemLevel.has_value())                                 // 저장된 정보가 옛날 것
        {
            const bool canRefresh = isInteractive && CanRefresh();                                           // [갱신] 보일 때
            const Core::RectF warningRow{ secondRow.m_left, secondRow.m_top, canRefresh ? secondRow.m_right - BUTTON_WIDTH - BUTTON_GAP : secondRow.m_right, secondRow.m_bottom };
            renderer.DrawString(std::format(OUTDATED_FORMAT, m_summary.m_itemLevel, *m_summary.m_readItemLevel), warningRow, Theme::CAUTION_TEXT_COLOR, NOTE_STYLE);   // 경고
            if (canRefresh) DrawButton(renderer, GetRefreshRect(content), REFRESH_TEXT, m_isHoverRefresh);   // [갱신]
            return;
        }
        std::wstring source{ m_summary.m_source == CurrentCharacterSource::SOURCE_MANUAL ? SOURCE_MANUAL_TEXT
            : (m_summary.m_source == CurrentCharacterSource::SOURCE_INFO_WINDOW ? SOURCE_INFO_TEXT : SOURCE_PLATE_TEXT) };   // 알아본 방법
        if (!m_summary.m_isAutoEnabled) source += AUTO_OFF_SUFFIX;                                           // 자동 인식 꺼짐
        if (!isInteractive) source += PASSIVE_CHANGE_HINT;                                                   // 평소엔 바꾸는 방법 안내
        renderer.DrawString(source, secondRow, Theme::MUTED_TEXT_COLOR, NOTE_STYLE);                         // 둘째 줄
    }

    bool CharacterPanel::HandleContentInput(const Core::InputEvent& event, const Core::RectF& content)
    {
        const bool canChange = !m_summary.m_candidates.empty()
            && (m_summary.m_state == CurrentCharacterState::CHARACTER_KNOWN || m_summary.m_state == CurrentCharacterState::CHARACTER_CHECKING);   // [변경] 누를 수 있음
        const auto findRow = [&](float x, float y) -> std::optional<std::size_t>                             // 좌표 아래 후보 번호
            {
                for (std::size_t row = 0; row < GetVisibleRowCount(); ++row)                                // 보이는 줄마다
                {
                    const std::size_t index = m_scrollIndex + row;                                          // 후보 번호
                    if (index < m_summary.m_candidates.size() && GetRowRect(content, row).Contains(x, y)) return index;   // 찾음
                }
                return std::nullopt;                                                                         // 없음
            };

        if (const auto* move = std::get_if<Core::MouseMoveEvent>(&event))                                    // 마우스 이동
        {
            m_isHoverChange = canChange && GetChangeRect(content).Contains(move->m_x, move->m_y);            // [변경] 위
            m_isHoverRefresh = CanRefresh() && GetRefreshRect(content).Contains(move->m_x, move->m_y);       // [갱신] 위
            m_hoverRow = m_isPicking ? findRow(move->m_x, move->m_y) : std::nullopt;                         // 목록 줄 위
            return false;                                                                                    // 이동은 삼키지 않음
        }
        if (std::holds_alternative<Core::MouseLeaveEvent>(event))                                            // 오버레이 밖으로
        {
            m_isHoverChange = false;                                                                         // 마우스 올림 지우기
            m_isHoverRefresh = false;
            m_hoverRow.reset();
            return false;                                                                                    // 다른 패널도 받게
        }
        if (const auto* button = std::get_if<Core::MouseButtonEvent>(&event))                                // 마우스 버튼
        {
            if (!content.Contains(button->m_x, button->m_y)) return false;                                   // 내용 밖
            if (!button->m_isPressed || button->m_button != Core::MouseButton::MOUSE_BUTTON_LEFT) return true;   // 왼쪽 누를 때만 (내용 위 입력은 삼킴)
            if (canChange && GetChangeRect(content).Contains(button->m_x, button->m_y))                      // [변경] · [취소]
            {
                m_isPicking = !m_isPicking;                                                                  // 고르기 켜기 · 끄기
                const std::size_t current = m_summary.m_currentIndex.value_or(0);                           // 지금 캐릭터
                const std::size_t visible = std::max<std::size_t>(MAX_PICK_ROWS, 1);                         // 한 번에 보일 수
                m_scrollIndex = current >= visible ? current - visible + 1 : 0;                              // 지금 캐릭터가 보이게
                m_hoverRow.reset();                                                                          // 마우스 올림 초기화
                UpdateHeight();                                                                              // 높이
                return true;                                                                                 // 처리함
            }
            if (!m_isPicking && CanRefresh() && GetRefreshRect(content).Contains(button->m_x, button->m_y))  // [갱신]
            {
                if (m_onRefresh) m_onRefresh();                                                              // 원정대 정보 다시 받기
                return true;                                                                                 // 처리함
            }
            if (m_isPicking)                                                                                 // 목록에서 고름
            {
                if (const auto index = findRow(button->m_x, button->m_y))                                    // 후보 줄
                {
                    if (m_onPick) m_onPick(*index);                                                          // 고른 캐릭터 알림
                    m_isPicking = false;                                                                     // 고르기 끝
                    m_hoverRow.reset();                                                                      // 초기화
                    m_refreshElapsed = REFRESH_SECONDS;                                                      // 바로 다시 받기
                    UpdateHeight();                                                                          // 높이
                }
            }
            return true;                                                                                     // 내용 위 클릭은 삼킴
        }
        if (const auto* wheel = std::get_if<Core::MouseWheelEvent>(&event))                                  // 휠
        {
            if (!m_isPicking || !content.Contains(wheel->m_x, wheel->m_y)) return false;                     // 고르기 중 · 내용 위만
            const std::size_t count = m_summary.m_candidates.size();                                         // 후보 수
            const std::size_t maxScroll = count > MAX_PICK_ROWS ? count - MAX_PICK_ROWS : 0;                 // 최대 스크롤
            if (wheel->m_notches > 0.0f && m_scrollIndex > 0) --m_scrollIndex;                               // 위로
            else if (wheel->m_notches < 0.0f && m_scrollIndex < maxScroll) ++m_scrollIndex;                  // 아래로
            return true;                                                                                     // 처리함
        }
        if (const auto* key = std::get_if<Core::KeyEvent>(&event))                                           // 키
        {
            if (m_isPicking && key->m_key == Core::Key::KEY_ESCAPE)                                          // Esc → 고르기 취소
            {
                m_isPicking = false;                                                                         // 끝
                UpdateHeight();                                                                              // 높이
                return true;                                                                                 // 처리함
            }
        }
        return false;   // 처리 안 함
    }
}
