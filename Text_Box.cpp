module LostArkMultiTool.Views.UI.TextBox;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr std::size_t DEFAULT_MAX_LENGTH{ 256 };
        constexpr int DOUBLE_CLICK_COUNT{ 2 };

        // ===== 배치 (DIP) =====
        constexpr float TEXT_PADDING{ 10.0f };          // 칸 안쪽 좌우 여백
        constexpr float CARET_WIDTH{ 1.0f };
        constexpr float CARET_MARGIN{ 7.0f };           // 커서 위아래 여백
        constexpr float SELECTION_MARGIN{ 6.0f };       // 선택 영역 위아래 여백
        constexpr float COMPOSITION_LINE_MARGIN{ 7.0f };
        constexpr float COMPOSITION_LINE_WIDTH{ 1.5f };
        constexpr float TEXT_RIGHT_SLACK{ 4.0f };       // 마지막 글자가 반올림으로 잘리지 않게 그리기 영역에 더하는 여유
        constexpr float INACTIVE_SELECTION_ALPHA{ 0.5f };

        constexpr double CARET_BLINK_SECONDS{ 0.53 };   // Windows 기본 깜빡임 간격
        constexpr double BLINK_CYCLE_FACTOR{ 2.0 };     // 보임 + 숨김 = 한 주기

        // ===== 글자 판정 =====
        constexpr wchar_t FIRST_PRINTABLE_CHARACTER{ L' ' };   // 0x20 미만은 제어 문자 (줄바꿈 · 탭 포함)
        constexpr wchar_t DELETE_CHARACTER{ 0x7F };
        constexpr wchar_t HIGH_SURROGATE_FIRST{ 0xD800 };      // UTF-16 대리 쌍 (이모지 등 두 칸짜리 글자)
        constexpr wchar_t HIGH_SURROGATE_LAST{ 0xDBFF };
        constexpr wchar_t LOW_SURROGATE_FIRST{ 0xDC00 };
        constexpr wchar_t LOW_SURROGATE_LAST{ 0xDFFF };

        constexpr Core::TextStyle TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };

        [[nodiscard]] bool IsHighSurrogate(wchar_t character) noexcept
        {
            return character >= HIGH_SURROGATE_FIRST && character <= HIGH_SURROGATE_LAST;
        }

        [[nodiscard]] bool IsLowSurrogate(wchar_t character) noexcept
        {
            return character >= LOW_SURROGATE_FIRST && character <= LOW_SURROGATE_LAST;
        }

        [[nodiscard]] bool IsPrintable(wchar_t character) noexcept
        {
            return character >= FIRST_PRINTABLE_CHARACTER && character != DELETE_CHARACTER;
        }
    }

    TextBox::TextBox(Core::Platform& platform, std::wstring placeholder)
        : m_platform{ platform }
        , m_placeholder{ std::move(placeholder) }
        , m_maxLength{ DEFAULT_MAX_LENGTH }
    {}

    void TextBox::SetText(std::wstring text)
    {
        if (!m_composition.empty())
        {
            m_composition.clear();
            if (IsFocused()) m_platform.CancelComposition();
        }

        m_text = std::move(text);
        if (m_text.size() > m_maxLength) m_text.resize(m_maxLength);

        m_caret = m_text.size();
        m_anchor = m_caret;
        m_scrollX = 0.0f;
    }

    const std::wstring& TextBox::GetText() const noexcept
    {
        return m_text;
    }

    void TextBox::SetMaxLength(std::size_t maxLength) noexcept
    {
        m_maxLength = maxLength;
    }

    void TextBox::SetIMEAllowed(bool isAllowed) noexcept
    {
        m_isIMEAllowed = isAllowed;
    }

    void TextBox::SetInputFilter(std::function<bool(wchar_t character)> inputFilter)
    {
        m_inputFilter = std::move(inputFilter);
    }

    void TextBox::SetOnTextChanged(std::function<void()> onTextChanged)
    {
        m_onTextChanged = std::move(onTextChanged);
    }

    void TextBox::SetOnSubmit(std::function<void()> onSubmit)
    {
        m_onSubmit = std::move(onSubmit);
    }

    void TextBox::SelectAll()
    {
        CommitComposition();
        m_anchor = 0;
        m_caret = m_text.size();
    }

    // 조합 중인 글자를 직접 확정하고 IME 쪽 조합은 버림
    // (IME에 확정을 맡기면 결과가 늦게 와서 이미 다른 칸으로 포커스가 옮겨 갔을 수 있음 → 마지막 글자가 사라지는 문제 방지)
    void TextBox::CommitComposition()
    {
        if (m_composition.empty()) return;

        const std::wstring composed{ std::exchange(m_composition, std::wstring{}) };
        InsertText(composed);
        m_platform.CancelComposition();
    }

    bool TextBox::IsFocusable() const noexcept
    {
        return true;
    }

    Core::CursorShape TextBox::GetCursor() const noexcept
    {
        return Core::CursorShape::CURSOR_TEXT;
    }

    void TextBox::Update(double deltaSeconds)
    {
        if (IsFocused()) m_blinkSeconds += deltaSeconds;
    }

    void TextBox::Render(Core::Renderer& renderer)
    {
        const Core::RectF& bounds{ GetBounds() };
        const bool isFocused{ IsFocused() };

        // 바탕 · 테두리 (포커스가 있으면 강조색)
        renderer.FillRoundedRect(bounds, Theme::CORNER_RADIUS, IsEnabled() ? Theme::INPUT_BACK_COLOR : Theme::DISABLED_BACK_COLOR);
        const Core::Color& borderColor{ isFocused ? Theme::FOCUS_BORDER_COLOR : (IsHovered() ? Theme::BUTTON_HOVER_COLOR : Theme::GRID_LINE_COLOR) };
        renderer.DrawRoundedRect(bounds, Theme::CORNER_RADIUS, borderColor, isFocused ? Theme::FOCUS_BORDER_WIDTH : Theme::BORDER_WIDTH);

        // 글자 위치 측정 (글자가 바뀌었을 때만)
        const Core::RectF area{ GetTextArea() };
        const std::wstring displayText{ BuildDisplayText() };
        const std::size_t displayCaret{ m_caret + m_composition.size() };
        if (displayText != m_measuredText || m_caretPositions.size() != displayText.size() + 1)
        {
            m_caretPositions = renderer.MeasureCaretPositions(displayText, TEXT_STYLE);
            m_measuredText = displayText;
        }
        EnsureCaretVisible(area.GetWidth(), displayCaret);

        const float originX{ area.m_left - m_scrollX };   // 글자 시작 x (스크롤만큼 왼쪽으로)
        {
            const Core::ClipScope clip{ renderer, area };

            if (displayText.empty())
            {
                renderer.DrawString(m_placeholder, area, Theme::MUTED_TEXT_COLOR, TEXT_STYLE);
            }
            else
            {
                if (HasSelection() && m_composition.empty())
                {
                    const Core::RectF selectionRect{ originX + m_caretPositions[GetSelectionStart()], area.m_top + SELECTION_MARGIN,
                        originX + m_caretPositions[GetSelectionEnd()], area.m_bottom - SELECTION_MARGIN };
                    renderer.FillRect(selectionRect, isFocused ? Theme::SELECTION_BACK_COLOR : Core::WithAlpha(Theme::SELECTION_BACK_COLOR, INACTIVE_SELECTION_ALPHA));
                }

                const Core::RectF textRect{ originX, area.m_top, originX + m_caretPositions.back() + TEXT_RIGHT_SLACK, area.m_bottom };
                renderer.DrawString(displayText, textRect, IsEnabled() ? Theme::TEXT_COLOR : Theme::MUTED_TEXT_COLOR, TEXT_STYLE);

                // 조합 중인 글자 밑줄
                if (!m_composition.empty())
                {
                    const float lineY{ area.m_bottom - COMPOSITION_LINE_MARGIN };
                    renderer.DrawLine(originX + m_caretPositions[m_caret], lineY, originX + m_caretPositions[displayCaret], lineY,
                        Theme::COMPOSITION_LINE_COLOR, COMPOSITION_LINE_WIDTH);
                }
            }
        }

        if (!isFocused) return;

        const float caretX{ originX + m_caretPositions[displayCaret] };
        const Core::RectF caretRect{ caretX, area.m_top + CARET_MARGIN, caretX + CARET_WIDTH, area.m_bottom - CARET_MARGIN };
        if (IsCaretVisible()) renderer.FillRect(caretRect, Theme::CARET_COLOR);
        UpdateIMEPosition(caretRect);
    }

    void TextBox::OnMouseDown(const Core::MouseButtonEvent& event)
    {
        CommitComposition();
        ResetCaretBlink();

        if (event.m_clickCount >= DOUBLE_CLICK_COUNT)
        {
            m_isDragSelecting = false;
            SelectAll();
            return;
        }

        m_isDragSelecting = true;
        MoveCaret(FindPositionAt(event.m_x), false);
    }

    void TextBox::OnMouseUp(const Core::MouseButtonEvent& /*event*/, bool /*isInside*/)
    {
        m_isDragSelecting = false;
    }

    void TextBox::OnMouseDrag(const Core::MouseMoveEvent& event)
    {
        if (!m_isDragSelecting) return;

        MoveCaret(FindPositionAt(event.m_x), true);
        ResetCaretBlink();
    }

    bool TextBox::OnKeyDown(const Core::KeyEvent& event)
    {
        // IME가 처리하지 않고 넘긴 키 → 조합 중인 글자부터 확정
        CommitComposition();
        ResetCaretBlink();

        const bool isExtending{ event.m_isShiftDown };
        switch (event.m_key)
        {
            case Core::Key::KEY_LEFT:
                if (HasSelection() && !isExtending) MoveCaret(GetSelectionStart(), false);
                else MoveCaret(GetPreviousPosition(m_caret), isExtending);
                return true;

            case Core::Key::KEY_RIGHT:
                if (HasSelection() && !isExtending) MoveCaret(GetSelectionEnd(), false);
                else MoveCaret(GetNextPosition(m_caret), isExtending);
                return true;

            case Core::Key::KEY_HOME:
                MoveCaret(0, isExtending);
                return true;

            case Core::Key::KEY_END:
                MoveCaret(m_text.size(), isExtending);
                return true;

            case Core::Key::KEY_BACKSPACE:
                DeleteBackward();
                return true;

            case Core::Key::KEY_DELETE:
                DeleteForward();
                return true;

            case Core::Key::KEY_ENTER:
            {
                if (!m_onSubmit) return false;   // 처리할 곳이 없으면 대화상자 기본 버튼 등에 양보

                const std::function<void()> onSubmit{ m_onSubmit };
                onSubmit();
                return true;
            }

            case Core::Key::KEY_A:
                if (!event.m_isCtrlDown) return false;
                SelectAll();
                return true;

            case Core::Key::KEY_C:
                if (!event.m_isCtrlDown) return false;
                CopySelection();
                return true;

            case Core::Key::KEY_X:
                if (!event.m_isCtrlDown) return false;
                CutSelection();
                return true;

            case Core::Key::KEY_V:
                if (!event.m_isCtrlDown) return false;
                Paste();
                return true;

            default:
                return false;
        }
    }

    // 확정된 글자 (타자 · 한글 조합 완료 · 붙여넣기)
    void TextBox::OnTextInput(std::wstring_view text)
    {
        m_composition.clear();   // 조합이 끝나면 확정 글자가 함께 옴 → 조합 표시는 지움
        InsertText(text);
        ResetCaretBlink();
    }

    void TextBox::OnComposition(std::wstring_view text)
    {
        // 조합을 시작할 때 선택된 글자가 있으면 먼저 지움 (선택 영역을 덮어쓰며 입력)
        if (!text.empty() && m_composition.empty() && DeleteSelection()) NotifyTextChanged();

        m_composition = text;
        ResetCaretBlink();
    }

    void TextBox::OnFocusChanged(bool isFocused)
    {
        if (isFocused)
        {
            m_platform.SetIMEEnabled(m_isIMEAllowed);
            m_hasIMERect = false;   // 다음 그리기에서 후보 창 위치를 다시 알려 줌
            ResetCaretBlink();
            return;
        }

        CommitComposition();   // IME를 끄기 전에 조합 중인 글자 확정
        m_platform.SetIMEEnabled(false);
        m_isDragSelecting = false;
    }

    void TextBox::InsertText(std::wstring_view text)
    {
        // 한 줄 입력칸이라 줄바꿈 · 탭 같은 제어 문자는 뺌 (붙여넣은 API 키 끝의 줄바꿈 등)
        std::wstring filtered;
        filtered.reserve(text.size());
        for (const wchar_t character : text)
        {
            if (IsPrintable(character) && (!m_inputFilter || m_inputFilter(character))) filtered.push_back(character);
        }
        if (filtered.empty()) return;   // 받을 수 없는 글자뿐 (숫자 칸에 영문 등) → 선택한 글자도 지우지 않음

        const bool isDeleted{ DeleteSelection() };

        // 최대 길이를 넘는 부분은 자름 (대리 쌍 앞쪽 반만 남지 않게)
        const std::size_t available{ m_maxLength > m_text.size() ? m_maxLength - m_text.size() : 0 };
        if (filtered.size() > available)
        {
            filtered.resize(available);
            if (!filtered.empty() && IsHighSurrogate(filtered.back())) filtered.pop_back();
        }

        if (filtered.empty())
        {
            if (isDeleted) NotifyTextChanged();
            return;
        }

        m_text.insert(m_caret, filtered);
        m_caret += filtered.size();
        m_anchor = m_caret;
        NotifyTextChanged();
    }

    bool TextBox::DeleteSelection()
    {
        if (!HasSelection()) return false;

        const std::size_t start{ GetSelectionStart() };
        m_text.erase(start, GetSelectionEnd() - start);
        m_caret = start;
        m_anchor = start;
        return true;
    }

    void TextBox::DeleteBackward()
    {
        if (DeleteSelection())
        {
            NotifyTextChanged();
            return;
        }
        if (m_caret == 0) return;

        const std::size_t previous{ GetPreviousPosition(m_caret) };
        m_text.erase(previous, m_caret - previous);
        m_caret = previous;
        m_anchor = previous;
        NotifyTextChanged();
    }

    void TextBox::DeleteForward()
    {
        if (DeleteSelection())
        {
            NotifyTextChanged();
            return;
        }
        if (m_caret >= m_text.size()) return;

        m_text.erase(m_caret, GetNextPosition(m_caret) - m_caret);
        NotifyTextChanged();
    }

    void TextBox::CopySelection()
    {
        if (!HasSelection()) return;

        const std::size_t start{ GetSelectionStart() };
        m_platform.SetClipboardText(std::wstring_view{ m_text }.substr(start, GetSelectionEnd() - start));
    }

    void TextBox::CutSelection()
    {
        if (!HasSelection()) return;

        CopySelection();
        if (DeleteSelection()) NotifyTextChanged();
    }

    void TextBox::Paste()
    {
        const std::wstring clipboardText{ m_platform.GetClipboardText() };
        if (!clipboardText.empty()) InsertText(clipboardText);
    }

    bool TextBox::HasSelection() const noexcept
    {
        return m_caret != m_anchor;
    }

    std::size_t TextBox::GetSelectionStart() const noexcept
    {
        return std::min(m_caret, m_anchor);
    }

    std::size_t TextBox::GetSelectionEnd() const noexcept
    {
        return std::max(m_caret, m_anchor);
    }

    void TextBox::MoveCaret(std::size_t position, bool isExtending) noexcept
    {
        m_caret = std::min(position, m_text.size());
        if (!isExtending) m_anchor = m_caret;
    }

    // 대리 쌍(두 칸짜리 글자) 가운데에 커서가 서지 않게 한 글자씩 이동
    std::size_t TextBox::GetPreviousPosition(std::size_t position) const noexcept
    {
        if (position == 0 || m_text.empty()) return 0;

        std::size_t previous{ std::min(position, m_text.size()) - 1 };
        if (previous > 0 && IsLowSurrogate(m_text[previous]) && IsHighSurrogate(m_text[previous - 1])) --previous;
        return previous;
    }

    std::size_t TextBox::GetNextPosition(std::size_t position) const noexcept
    {
        if (position >= m_text.size()) return m_text.size();

        std::size_t next{ position + 1 };
        if (next < m_text.size() && IsHighSurrogate(m_text[position]) && IsLowSurrogate(m_text[next])) ++next;
        return next;
    }

    // 마우스 x → 가장 가까운 글자 경계 (마지막 그리기에서 잰 위치 기준)
    std::size_t TextBox::FindPositionAt(float x) const noexcept
    {
        if (m_caretPositions.empty()) return m_text.size();

        const float localX{ x - GetTextArea().m_left + m_scrollX };
        const std::size_t lastIndex{ std::min(m_caretPositions.size() - 1, m_text.size()) };   // 위치 표가 오래됐어도 글자 범위를 넘지 않게

        std::size_t nearest{};
        float nearestDistance{ std::numeric_limits<float>::max() };
        for (std::size_t index{}; index <= lastIndex; ++index)
        {
            const float distance{ std::abs(m_caretPositions[index] - localX) };
            if (distance < nearestDistance)   // 같은 거리면 앞쪽 (한 글자 묶음 안쪽 위치는 묶음 시작과 x가 같음)
            {
                nearestDistance = distance;
                nearest = index;
            }
        }
        return nearest;
    }

    Core::RectF TextBox::GetTextArea() const noexcept
    {
        const Core::RectF& bounds{ GetBounds() };
        return Core::RectF{ bounds.m_left + TEXT_PADDING, bounds.m_top + Theme::BORDER_WIDTH,
            bounds.m_right - TEXT_PADDING, bounds.m_bottom - Theme::BORDER_WIDTH };
    }

    // 화면에 보일 글자 = 입력된 글자 + 커서 위치에 끼운 조합 중인 글자
    std::wstring TextBox::BuildDisplayText() const
    {
        if (m_composition.empty()) return m_text;

        std::wstring displayText{ m_text };
        displayText.insert(m_caret, m_composition);
        return displayText;
    }

    void TextBox::EnsureCaretVisible(float areaWidth, std::size_t displayCaret) noexcept
    {
        if (displayCaret >= m_caretPositions.size()) return;

        const float caretX{ m_caretPositions[displayCaret] };
        if (caretX - m_scrollX > areaWidth - CARET_WIDTH) m_scrollX = caretX - areaWidth + CARET_WIDTH;   // 오른쪽으로 벗어남
        if (caretX - m_scrollX < 0.0f) m_scrollX = caretX;                                               // 왼쪽으로 벗어남

        // 글자를 지워서 오른쪽에 빈 공간이 생기면 다시 당겨 옴
        const float maxScroll{ std::max(0.0f, m_caretPositions.back() + CARET_WIDTH - areaWidth) };
        m_scrollX = std::clamp(m_scrollX, 0.0f, maxScroll);
    }

    void TextBox::UpdateIMEPosition(const Core::RectF& caretRect)
    {
        if (m_hasIMERect && m_lastIMERect == caretRect) return;

        m_platform.SetIMEPosition(caretRect);
        m_lastIMERect = caretRect;
        m_hasIMERect = true;
    }

    bool TextBox::IsCaretVisible() const noexcept
    {
        return std::fmod(m_blinkSeconds, CARET_BLINK_SECONDS * BLINK_CYCLE_FACTOR) < CARET_BLINK_SECONDS;
    }

    void TextBox::ResetCaretBlink() noexcept
    {
        m_blinkSeconds = 0.0;
    }

    void TextBox::NotifyTextChanged()
    {
        if (!m_onTextChanged) return;

        const std::function<void()> onTextChanged{ m_onTextChanged };
        onTextChanged();
    }
}