export module LostArkMultiTool.Views.APIKeyDialog;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // API 키 입력 창을 연 상황
    enum class APIKeyDialogMode
    {
        API_KEY_FIRST_RUN,   // 처음 실행 (저장된 키 없음): [저장 및 시작] [종료]
        API_KEY_CHANGE       // 키 변경 · 키 오류 후 재입력: [저장] [취소]
    };

    // API 키 입력 대화상자 (원본 APIKeyForm 대응, 세 툴 공용 키를 한 번만 입력)
    class APIKeyDialog final : public UI::Dialog
    {
    public:
        // onSave: 저장에 성공하면 true (실패하면 창을 닫지 않음, 실패 안내는 저장하는 쪽에서)
        // onCancel: [종료] · [취소]
        APIKeyDialog(Core::Platform& platform, Core::Notifier& notifier, APIKeyDialogMode mode, std::wstring currentKey,
            std::function<bool(const std::string& apiKey)> onSave, std::function<void()> onCancel);

    protected:
        [[nodiscard]] float MeasureContentHeight(Core::Renderer& renderer, float contentWidth) override;
        void LayoutContent(const Core::RectF& contentArea) override;
        void FocusInitialControl() override;
        void OnSubmit() override;
        void OnCancel() override;

    private:
        void Save();
        void Cancel();
        void OpenKeyPage();
        void ShowHint();
        void ShowError(std::wstring message);

        Core::Platform& m_platform;
        Core::Notifier& m_notifier;
        APIKeyDialogMode m_mode;

        // 부품은 대화상자의 ControlHost가 소유 (참조는 배치 · 값 읽기용)
        UI::Label& m_infoLabel;
        UI::Button& m_linkButton;
        UI::TextBox& m_keyBox;
        UI::Label& m_statusLabel;     // 평소엔 저장 방식 안내, 잘못 입력하면 빨간 오류 문구
        UI::Button& m_saveButton;
        UI::Button& m_cancelButton;
        float m_infoHeight{};

        std::function<bool(const std::string&)> m_onSave;
        std::function<void()> m_onCancel;
    };
}