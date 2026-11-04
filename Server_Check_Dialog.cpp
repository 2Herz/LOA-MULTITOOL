module LostArkMultiTool.Views.ServerCheckDialog;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.ProgressBar;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view DIALOG_TITLE{ L"로스트아크 멀티툴" };
        constexpr std::wstring_view CHECKING_TEXT{ L"서버 상태를 확인하고 있어요..." };

        constexpr float DIALOG_WIDTH{ 360.0f };
        constexpr float MESSAGE_HEIGHT{ 22.0f };
        constexpr float MESSAGE_BAR_GAP{ 12.0f };
        constexpr float PROGRESS_BAR_HEIGHT{ 8.0f };

        constexpr Core::TextStyle MESSAGE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
    }

    ServerCheckDialog::ServerCheckDialog()
        : UI::Dialog{ std::wstring{ DIALOG_TITLE }, Core::NoticeKind::NOTICE_INFO, DIALOG_WIDTH }
        , m_messageLabel{ GetHost().Emplace<UI::Label>(std::wstring{ CHECKING_TEXT }, MESSAGE_STYLE, Theme::TEXT_COLOR) }
        , m_progressBar{ GetHost().Emplace<UI::ProgressBar>() }
    {}

    float ServerCheckDialog::MeasureContentHeight(Core::Renderer& /*renderer*/, float /*contentWidth*/)
    {
        return MESSAGE_HEIGHT + MESSAGE_BAR_GAP + PROGRESS_BAR_HEIGHT;
    }

    void ServerCheckDialog::LayoutContent(const Core::RectF& contentArea)
    {
        m_messageLabel.SetBounds(Core::RectF{ contentArea.m_left, contentArea.m_top, contentArea.m_right, contentArea.m_top + MESSAGE_HEIGHT });

        const float barTop{ contentArea.m_top + MESSAGE_HEIGHT + MESSAGE_BAR_GAP };
        m_progressBar.SetBounds(Core::RectF{ contentArea.m_left, barTop, contentArea.m_right, barTop + PROGRESS_BAR_HEIGHT });
    }
}