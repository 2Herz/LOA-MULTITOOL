export module LostArkMultiTool.Views.UI.MessageDialog;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;

export namespace LostArkMultiTool::Views::UI
{
    // 메시지 대화상자 종류
    enum class MessageButtons
    {
        MESSAGE_OK,       // [확인]
        MESSAGE_YES_NO    // [예] [아니요]
    };

    // 안내 · 오류 · 질문 대화상자 (원본 MessageBox 대응)
    // 답을 고르면 onAnswer(확인 · 예 = true, 아니요 · Esc = false) 호출 후 닫힘
    class MessageDialog final : public Dialog
    {
    public:
        MessageDialog(std::wstring title, std::wstring message, Core::NoticeKind kind, MessageButtons buttons,
            std::function<void(bool isConfirmed)> onAnswer);

    protected:
        [[nodiscard]] float MeasureContentHeight(Core::Renderer& renderer, float contentWidth) override;
        void LayoutContent(const Core::RectF& contentArea) override;
        void FocusInitialControl() override;
        void OnSubmit() override;
        void OnCancel() override;

    private:
        void Answer(bool isConfirmed);

        Label& m_messageLabel;
        Button& m_confirmButton;     // 확인 · 예
        Button* m_declineButton{};   // 아니요 (MESSAGE_OK면 없음, ControlHost가 소유)
        float m_messageHeight{};
        std::function<void(bool)> m_onAnswer;
    };
}