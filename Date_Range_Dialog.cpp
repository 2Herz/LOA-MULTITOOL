module LostArkMultiTool.Views.UI.DateRangeDialog;

import std;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        // ===== 문구 =====
        constexpr std::wstring_view CONFIRM_TEXT{ L"확인" };
        constexpr std::wstring_view CANCEL_TEXT{ L"취소" };
        constexpr std::wstring_view START_LABEL_TEXT{ L"시작일 (수요일)" };
        constexpr std::wstring_view END_LABEL_TEXT{ L"종료일" };
        constexpr std::wstring_view DATE_PLACEHOLDER{ L"yyyy-MM-dd" };
        constexpr std::wstring_view FORMAT_HINT_TEXT{ L"날짜는 yyyy-MM-dd 형식으로 입력해 주세요. (예: 2026-11-12)" };
        constexpr std::wstring_view INVALID_START_TEXT{ L"시작일이 올바른 날짜가 아니에요!" };
        constexpr std::wstring_view INVALID_END_TEXT{ L"종료일이 올바른 날짜가 아니에요!" };
        constexpr std::wstring_view END_BEFORE_START_TEXT{ L"종료일은 시작일보다 앞설 수 없어요!" };
        constexpr std::wstring_view PREVIEW_FORMAT{ L"총 {}주 · {} ~ {}" };
        constexpr std::wstring_view DATE_LABEL_FORMAT{ L"{:02}/{:02}({})" };
        constexpr std::array<std::wstring_view, 7> WEEKDAY_NAMES{ L"일", L"월", L"화", L"수", L"목", L"금", L"토" };   // c_encoding 순서 (일요일 = 0)

        // ===== 배치 (DIP) =====
        constexpr float DIALOG_WIDTH{ 420.0f };
        constexpr float LABEL_WIDTH{ 110.0f };
        constexpr float DATE_BOX_WIDTH{ 130.0f };
        constexpr float STATUS_HEIGHT{ 18.0f };
        constexpr float BUTTON_WIDTH{ 96.0f };
        constexpr float BUTTON_GAP{ 8.0f };
        constexpr float MESSAGE_BOX_GAP{ 12.0f };
        constexpr float ROW_GAP{ 8.0f };
        constexpr float BOX_STATUS_GAP{ 8.0f };
        constexpr float STATUS_BUTTON_GAP{ 18.0f };
        constexpr std::size_t DATE_MAX_LENGTH{ 10 };   // "yyyy-MM-dd"
        constexpr int DAYS_PER_WEEK{ 7 };

        constexpr Core::TextStyle MESSAGE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_TOP, Core::TextWrap::WORD_WRAP };
        constexpr Core::TextStyle FIELD_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle STATUS_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };

        // 날짜 칸에 받을 글자 (숫자 · 하이픈)
        [[nodiscard]] bool IsDateCharacter(wchar_t character) noexcept
        {
            return (character >= L'0' && character <= L'9') || character == L'-';
        }

        // 입력칸 글자 → 날짜 (형식이 다르면 nullopt)
        [[nodiscard]] std::optional<std::chrono::year_month_day> ParseDate(const std::wstring& text)
        {
            if (text.size() != DATE_MAX_LENGTH) return std::nullopt;
            return Core::LocalTime::ParseDateKey(Core::TextEncoding::WideToUTF8(text));
        }

        [[nodiscard]] std::wstring ToDateText(const std::chrono::year_month_day& date)
        {
            return Core::TextEncoding::UTF8ToWide(Core::LocalTime::MakeDateKey(date));
        }

        // "11/12(수)"
        [[nodiscard]] std::wstring ToDateLabel(const std::chrono::year_month_day& date)
        {
            const std::chrono::weekday weekday{ std::chrono::sys_days{ date } };
            return std::format(DATE_LABEL_FORMAT, static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()),
                WEEKDAY_NAMES.at(weekday.c_encoding()));
        }
    }

    DateRangeDialog::DateRangeDialog(Core::Platform& platform, std::wstring title, std::wstring message,
        const std::chrono::year_month_day& initialStart, const std::chrono::year_month_day& initialEnd, DateRangeConfirm onConfirm)
        : Dialog{ std::move(title), Core::NoticeKind::NOTICE_INFO, DIALOG_WIDTH }
        , m_messageLabel{ GetHost().Emplace<Label>(std::move(message), MESSAGE_STYLE, Theme::TEXT_COLOR) }
        , m_startLabel{ GetHost().Emplace<Label>(std::wstring{ START_LABEL_TEXT }, FIELD_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_startBox{ GetHost().Emplace<TextBox>(platform, std::wstring{ DATE_PLACEHOLDER }) }
        , m_endLabel{ GetHost().Emplace<Label>(std::wstring{ END_LABEL_TEXT }, FIELD_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_endBox{ GetHost().Emplace<TextBox>(platform, std::wstring{ DATE_PLACEHOLDER }) }
        , m_statusLabel{ GetHost().Emplace<Label>(std::wstring{}, STATUS_STYLE, Theme::MUTED_TEXT_COLOR) }
        , m_confirmButton{ GetHost().Emplace<Button>(std::wstring{ CONFIRM_TEXT }, ButtonStyle::BUTTON_PRIMARY, [this] { Confirm(); }) }
        , m_cancelButton{ GetHost().Emplace<Button>(std::wstring{ CANCEL_TEXT }, ButtonStyle::BUTTON_SECONDARY, [this] { Cancel(); }) }
        , m_onConfirm{ std::move(onConfirm) }
    {
        // 날짜 전용 칸: IME 끔 (한/영 상태와 상관없이 숫자 입력), 숫자 · 하이픈만, Enter = 확인
        for (TextBox* box : { &m_startBox, &m_endBox })
        {
            box->SetIMEAllowed(false);
            box->SetInputFilter(IsDateCharacter);
            box->SetMaxLength(DATE_MAX_LENGTH);
            box->SetOnTextChanged([this] { ShowPreview(); });
            box->SetOnSubmit([this] { Confirm(); });
        }
        m_startBox.SetText(ToDateText(initialStart));
        m_endBox.SetText(ToDateText(initialEnd));

        ShowPreview();
    }

    float DateRangeDialog::MeasureContentHeight(Core::Renderer& renderer, float contentWidth)
    {
        m_messageHeight = m_messageLabel.MeasureHeight(renderer, contentWidth);
        return m_messageHeight + MESSAGE_BOX_GAP + Theme::CONTROL_HEIGHT + ROW_GAP + Theme::CONTROL_HEIGHT
            + BOX_STATUS_GAP + STATUS_HEIGHT + STATUS_BUTTON_GAP + Theme::CONTROL_HEIGHT;
    }

    // 안내 → [시작일 칸] → [종료일 칸] → 미리보기/오류 → 버튼 (위에서 아래로)
    void DateRangeDialog::LayoutContent(const Core::RectF& contentArea)
    {
        float top{ contentArea.m_top };
        m_messageLabel.SetBounds(Core::RectF{ contentArea.m_left, top, contentArea.m_right, top + m_messageHeight });

        // 날짜 줄 하나: 왼쪽 이름표 + 오른쪽 입력칸
        const float boxLeft{ contentArea.m_left + LABEL_WIDTH };
        const auto placeRow{ [&](Label& label, TextBox& box, float rowTop)
        {
            label.SetBounds(Core::RectF{ contentArea.m_left, rowTop, boxLeft, rowTop + Theme::CONTROL_HEIGHT });
            box.SetBounds(Core::RectF{ boxLeft, rowTop, boxLeft + DATE_BOX_WIDTH, rowTop + Theme::CONTROL_HEIGHT });
        } };

        top += m_messageHeight + MESSAGE_BOX_GAP;
        placeRow(m_startLabel, m_startBox, top);
        top += Theme::CONTROL_HEIGHT + ROW_GAP;
        placeRow(m_endLabel, m_endBox, top);

        top += Theme::CONTROL_HEIGHT + BOX_STATUS_GAP;
        m_statusLabel.SetBounds(Core::RectF{ contentArea.m_left, top, contentArea.m_right, top + STATUS_HEIGHT });

        const float buttonTop{ contentArea.m_bottom - Theme::CONTROL_HEIGHT };
        const float cancelLeft{ contentArea.m_right - BUTTON_WIDTH };
        m_cancelButton.SetBounds(Core::RectF{ cancelLeft, buttonTop, contentArea.m_right, contentArea.m_bottom });

        const float confirmRight{ cancelLeft - BUTTON_GAP };
        m_confirmButton.SetBounds(Core::RectF{ confirmRight - BUTTON_WIDTH, buttonTop, confirmRight, contentArea.m_bottom });
    }

    // 시작일 칸부터 (전체 선택 → 바로 입력하면 교체)
    void DateRangeDialog::FocusInitialControl()
    {
        GetHost().SetFocus(&m_startBox);
        m_startBox.SelectAll();
    }

    void DateRangeDialog::OnSubmit()
    {
        Confirm();
    }

    void DateRangeDialog::OnCancel()
    {
        Cancel();
    }

    void DateRangeDialog::Confirm()
    {
        if (IsClosed()) return;

        const std::optional<std::chrono::year_month_day> startDate{ ParseDate(m_startBox.GetText()) };
        if (!startDate.has_value())
        {
            ShowError(std::wstring{ INVALID_START_TEXT });
            GetHost().SetFocus(&m_startBox);
            return;
        }
        const std::optional<std::chrono::year_month_day> endDate{ ParseDate(m_endBox.GetText()) };
        if (!endDate.has_value())
        {
            ShowError(std::wstring{ INVALID_END_TEXT });
            GetHost().SetFocus(&m_endBox);
            return;
        }
        if (std::chrono::sys_days{ *endDate } < std::chrono::sys_days{ *startDate })
        {
            ShowError(std::wstring{ END_BEFORE_START_TEXT });
            GetHost().SetFocus(&m_endBox);
            return;
        }

        // 받는 쪽이 거절하면(예: 시작일이 수요일이 아님) 창을 열어 둔 채 다시 고를 수 있게
        if (m_onConfirm)
        {
            if (std::optional<std::wstring> error{ m_onConfirm(*startDate, *endDate) }; error.has_value())
            {
                ShowError(std::move(*error));
                GetHost().SetFocus(&m_startBox);
                m_startBox.SelectAll();
                return;
            }
        }
        Close();
    }

    void DateRangeDialog::Cancel()
    {
        if (!IsClosed()) Close();
    }

    void DateRangeDialog::ShowPreview()
    {
        m_statusLabel.SetColor(Theme::MUTED_TEXT_COLOR);

        const std::optional<std::chrono::year_month_day> startDate{ ParseDate(m_startBox.GetText()) };
        const std::optional<std::chrono::year_month_day> endDate{ ParseDate(m_endBox.GetText()) };

        // 두 날짜가 모두 올바른데 종료일이 시작일보다 앞서면 입력 중에도 바로 빨간 안내 + [확인] 끔
        const bool isReversed{ startDate.has_value() && endDate.has_value()
            && std::chrono::sys_days{ *endDate } < std::chrono::sys_days{ *startDate } };
        m_confirmButton.SetEnabled(!isReversed);
        if (isReversed)
        {
            ShowError(std::wstring{ END_BEFORE_START_TEXT });
            return;
        }

        if (!startDate.has_value() || !endDate.has_value())
        {
            m_statusLabel.SetText(std::wstring{ FORMAT_HINT_TEXT });
            return;
        }

        // 주 수 = (종료일 - 시작일) ÷ 7 + 1 (낙원력 트래커 Season과 같은 계산)
        const auto days{ (std::chrono::sys_days{ *endDate } - std::chrono::sys_days{ *startDate }).count() };
        m_statusLabel.SetText(std::format(PREVIEW_FORMAT, days / DAYS_PER_WEEK + 1, ToDateLabel(*startDate), ToDateLabel(*endDate)));
    }

    void DateRangeDialog::ShowError(std::wstring message)
    {
        m_statusLabel.SetText(std::move(message));
        m_statusLabel.SetColor(Theme::NOTICE_ERROR_COLOR);
    }
}
