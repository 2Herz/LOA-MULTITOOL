module LostArkMultiTool.Views.UI.OverlayNotifier;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.MessageDialog;
import LostArkMultiTool.Views.UI.DateRangeDialog;
import LostArkMultiTool.Views.UI.ToastHost;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr std::wstring_view TRAY_BALLOON_TITLE{ L"로스트아크 멀티툴" };
    }

    OverlayNotifier::OverlayNotifier(Core::Platform& platform)
        : m_platform{ platform }
    {}

    OverlayNotifier::~OverlayNotifier() = default;

    void OverlayNotifier::ShowMessage(std::wstring title, std::wstring message, Core::NoticeKind kind, std::function<void()> onClosed)
    {
        // [확인] 하나짜리라 어떤 답이든 "닫힘"으로 알림
        std::function<void(bool)> onAnswer;
        if (onClosed) onAnswer = [onClosed = std::move(onClosed)](bool /*isConfirmed*/) { onClosed(); };

        OpenDialog(std::make_unique<MessageDialog>(std::move(title), std::move(message), kind, MessageButtons::MESSAGE_OK, std::move(onAnswer)));
    }

    void OverlayNotifier::AskYesNo(std::wstring title, std::wstring message, std::function<void(bool isYes)> onAnswer)
    {
        OpenDialog(std::make_unique<MessageDialog>(std::move(title), std::move(message), Core::NoticeKind::NOTICE_WARNING,
            MessageButtons::MESSAGE_YES_NO, std::move(onAnswer)));
    }

    void OverlayNotifier::AskDateRange(std::wstring title, std::wstring message,
        const std::chrono::year_month_day& initialStart, const std::chrono::year_month_day& initialEnd,
        std::function<std::optional<std::wstring>(const std::chrono::year_month_day& startDate, const std::chrono::year_month_day& endDate)> onConfirm)
    {
        OpenDialog(std::make_unique<DateRangeDialog>(m_platform, std::move(title), std::move(message), initialStart, initialEnd, std::move(onConfirm)));
    }

    void OverlayNotifier::SetToastListener(std::function<bool(const std::wstring& message, Core::NoticeKind kind)> listener)
    {
        m_toastListener = std::move(listener);   // 인게임 오버레이 연결
    }

    void OverlayNotifier::ShowToast(std::wstring message, Core::NoticeKind kind)
    {
        const bool isShownOnOverlay{ m_toastListener && m_toastListener(message, kind) };   // 게임 위 오버레이에도 띄움

        // 창이 숨어 있으면 화면 안 토스트는 아무도 못 보므로 트레이 풍선 알림으로 (오버레이에 띄웠으면 생략)
        if (!m_platform.IsWindowShown() && isShownOnOverlay) return;
        if (!m_platform.IsWindowShown())
        {
            m_platform.ShowTrayBalloon(TRAY_BALLOON_TITLE, message, kind);
            return;
        }
        m_toastHost.Add(std::move(message), kind);
    }

    void OverlayNotifier::OpenDialog(std::unique_ptr<Dialog> dialog)
    {
        if (!dialog) return;

        // 아래 대화상자는 입력을 내려놓고(조합 확정 · IME 끔), 새 대화상자가 포커스를 받음
        if (m_activeDialog != nullptr && !m_activeDialog->IsClosed()) m_activeDialog->Deactivate();

        m_dialogs.push_back(std::move(dialog));
        m_activeDialog = m_dialogs.back().get();
        m_activeDialog->Activate();
    }

    bool OverlayNotifier::HasDialog() const noexcept
    {
        return GetTopDialog() != nullptr;
    }

    bool OverlayNotifier::HandleInput(const Core::InputEvent& event)
    {
        Dialog* top{ GetTopDialog() };
        return top != nullptr && top->HandleInput(event);   // 처리 중 새 대화상자가 쌓여도 top(대화상자 객체)은 그대로 유효
    }

    Core::CursorShape OverlayNotifier::GetCursor() const noexcept
    {
        const Dialog* top{ GetTopDialog() };
        return top != nullptr ? top->GetCursor() : Core::CursorShape::CURSOR_ARROW;
    }

    void OverlayNotifier::Update(double deltaSeconds)
    {
        // 닫힌 대화상자 정리 (버튼 콜백이 끝난 다음 프레임에 지움 → 콜백 도중 자신이 사라지는 일 없음)
        if (m_activeDialog != nullptr && m_activeDialog->IsClosed()) m_activeDialog = nullptr;   // 지우기 전에 가리키던 것을 비움
        std::erase_if(m_dialogs, [](const std::unique_ptr<Dialog>& dialog) { return dialog->IsClosed(); });

        // 위의 대화상자가 닫혀 아래 대화상자가 맨 위가 됨 → 포커스 되돌림
        if (Dialog* top{ GetTopDialog() }; top != m_activeDialog)
        {
            m_activeDialog = top;
            if (m_activeDialog != nullptr) m_activeDialog->Activate();
        }

        for (const auto& dialog : m_dialogs) dialog->Update(deltaSeconds);
        m_toastHost.Update(deltaSeconds);
    }

    void OverlayNotifier::Render(Core::Renderer& renderer, const Core::RectF& screenArea)
    {
        // 아래 대화상자부터 차례로 (위 대화상자의 어두운 막이 아래 대화상자를 한 번 더 덮어 앞뒤가 구분됨)
        for (const auto& dialog : m_dialogs)
        {
            if (!dialog->IsClosed()) dialog->Render(renderer, screenArea);
        }

        m_toastHost.Render(renderer, screenArea);
    }

    // 닫히는 중인 대화상자는 건너뛰고 맨 위의 열린 대화상자
    Dialog* OverlayNotifier::GetTopDialog() const noexcept
    {
        for (const auto& dialog : std::views::reverse(m_dialogs))
        {
            if (!dialog->IsClosed()) return dialog.get();
        }
        return nullptr;
    }
}