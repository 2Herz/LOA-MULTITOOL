export module LostArkMultiTool.Views.UI.OverlayNotifier;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.ToastHost;

export namespace LostArkMultiTool::Views::UI
{
    // 화면 맨 위에 그리는 알림 층 (Core::Notifier 구현)
    // 대화상자는 쌓아서 맨 위 것만 입력을 받고, 토스트는 입력과 상관없이 표시 (Facade: 대화상자 쌓기 + ToastHost)
    class OverlayNotifier final : public Core::Notifier
    {
    public:
        OverlayNotifier();
        ~OverlayNotifier() override;

        OverlayNotifier(const OverlayNotifier&) = delete;
        OverlayNotifier& operator=(const OverlayNotifier&) = delete;

        // ===== Core::Notifier =====
        void ShowMessage(std::wstring title, std::wstring message, Core::NoticeKind kind) override;
        void AskYesNo(std::wstring title, std::wstring message, std::function<void(bool isYes)> onAnswer) override;
        void ShowToast(std::wstring message, Core::NoticeKind kind) override;

        // 직접 만든 대화상자 열기 (API 키 입력 등)
        void OpenDialog(std::unique_ptr<Dialog> dialog);

        [[nodiscard]] bool HasDialog() const noexcept;   // 열려 있는(닫히는 중이 아닌) 대화상자가 있는지 → 있으면 모달
        bool HandleInput(const Core::InputEvent& event);
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;

        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& screenArea);

    private:
        [[nodiscard]] Dialog* GetTopDialog() const noexcept;

        std::vector<std::unique_ptr<Dialog>> m_dialogs;   // 뒤쪽이 맨 위
        Dialog* m_activeDialog{};                         // 지금 입력을 받는 대화상자 (m_dialogs가 소유, 지우기 전에 비움 → 댕글링 없음)
        ToastHost m_toastHost;
    };
}