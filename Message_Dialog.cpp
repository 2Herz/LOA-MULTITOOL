module LostArkMultiTool.Views.UI.MessageDialog;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr float DIALOG_WIDTH{ 440.0f };
        constexpr float MESSAGE_MIN_HEIGHT{ 24.0f };
        constexpr float MESSAGE_MAX_HEIGHT{ 320.0f };   // 아주 긴 오류 문구도 창을 넘지 않게
        constexpr float BUTTON_WIDTH{ 96.0f };
        constexpr float BUTTON_GAP{ 8.0f };
        constexpr float BUTTON_ROW_GAP{ 20.0f };        // 문구와 버튼 줄 사이

        constexpr std::wstring_view OK_TEXT{ L"확인" };
        constexpr std::wstring_view YES_TEXT{ L"예" };
        constexpr std::wstring_view NO_TEXT{ L"아니요" };

        constexpr Core::TextStyle MESSAGE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_TOP, Core::TextWrap::WORD_WRAP };
    }

    MessageDialog::MessageDialog(std::wstring title, std::wstring message, Core::NoticeKind kind, MessageButtons buttons,
        std::function<void(bool isConfirmed)> onAnswer)
        : Dialog{ std::move(title), kind, DIALOG_WIDTH }
        , m_messageLabel{ GetHost().Emplace<Label>(std::move(message), MESSAGE_STYLE, Theme::TEXT_COLOR) }
        , m_confirmButton{ GetHost().Emplace<Button>(std::wstring{ buttons == MessageButtons::MESSAGE_YES_NO ? YES_TEXT : OK_TEXT },
            ButtonStyle::BUTTON_PRIMARY, [this] { Answer(true); }) }
        , m_onAnswer{ std::move(onAnswer) }
    {
        if (buttons == MessageButtons::MESSAGE_YES_NO)
        {
            m_declineButton = &GetHost().Emplace<Button>(std::wstring{ NO_TEXT }, ButtonStyle::BUTTON_SECONDARY, [this] { Answer(false); });
        }
    }

    float MessageDialog::MeasureContentHeight(Core::Renderer& renderer, float contentWidth)
    {
        m_messageHeight = std::clamp(m_messageLabel.MeasureHeight(renderer, contentWidth), MESSAGE_MIN_HEIGHT, MESSAGE_MAX_HEIGHT);
        return m_messageHeight + BUTTON_ROW_GAP + Theme::CONTROL_HEIGHT;
    }

    // 문구는 위, 버튼은 오른쪽 아래 ([예] [아니요] 순서)
    void MessageDialog::LayoutContent(const Core::RectF& contentArea)
    {
        m_messageLabel.SetBounds(Core::RectF{ contentArea.m_left, contentArea.m_top, contentArea.m_right, contentArea.m_top + m_messageHeight });

        const float buttonTop{ contentArea.m_bottom - Theme::CONTROL_HEIGHT };
        float buttonRight{ contentArea.m_right };
        if (m_declineButton != nullptr)
        {
            m_declineButton->SetBounds(Core::RectF{ buttonRight - BUTTON_WIDTH, buttonTop, buttonRight, contentArea.m_bottom });
            buttonRight -= BUTTON_WIDTH + BUTTON_GAP;
        }
        m_confirmButton.SetBounds(Core::RectF{ buttonRight - BUTTON_WIDTH, buttonTop, buttonRight, contentArea.m_bottom });
    }

    void MessageDialog::FocusInitialControl()
    {
        GetHost().SetFocus(&m_confirmButton);
    }

    void MessageDialog::OnSubmit()
    {
        Answer(true);
    }

    void MessageDialog::OnCancel()
    {
        Answer(m_declineButton == nullptr);   // 확인만 있으면 Esc도 확인, 예/아니요면 Esc = 아니요
    }

    void MessageDialog::Answer(bool isConfirmed)
    {
        if (IsClosed()) return;   // 두 번 눌러도 한 번만 응답

        Close();
        if (m_onAnswer) m_onAnswer(isConfirmed);   // 닫은 뒤에 알림 → 응답에서 새 대화상자를 열어도 순서가 맞음
    }
}