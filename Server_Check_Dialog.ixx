export module LostArkMultiTool.Views.ServerCheckDialog;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.ProgressBar;

export namespace LostArkMultiTool::Views
{
    // 실행 직후 서버 상태(점검 스위치)를 확인하는 동안 띄우는 대화상자 (원본 ServerCheckForm)
    // 사용자가 닫을 수 없음 (Esc 무시) → 확인이 끝나면 Application이 Close()로 닫음
    class ServerCheckDialog final : public UI::Dialog
    {
    public:
        ServerCheckDialog();

    protected:
        [[nodiscard]] float MeasureContentHeight(Core::Renderer& renderer, float contentWidth) override;
        void LayoutContent(const Core::RectF& contentArea) override;

    private:
        UI::Label& m_messageLabel;
        UI::ProgressBar& m_progressBar;
    };
}