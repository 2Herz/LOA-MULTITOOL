export module LostArkMultiTool.Views.UI.DateRangeDialog;

import std;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views::UI
{
    // 기간(시작일 ~ 종료일) 입력 대화상자 (낙원력 트래커 시즌 기간, 날짜 형식 "yyyy-MM-dd")
    // 입력하는 동안 아래 안내 줄에 "N주 · 11/12(수) ~ 12/09(화)"처럼 결과를 미리 보여 줌
    // [확인] · Enter: 두 날짜가 올바르고 종료일 ≥ 시작일이면 onConfirm 호출 → 오류 문구를 돌려받으면 창을 열어 둔 채 빨간 글자로 안내
    // [취소] · Esc: 그냥 닫힘
    class DateRangeDialog final : public Dialog
    {
    public:
        using DateRangeConfirm = std::function<std::optional<std::wstring>(const std::chrono::year_month_day& startDate, const std::chrono::year_month_day& endDate)>;

        DateRangeDialog(Core::Platform& platform, std::wstring title, std::wstring message,
            const std::chrono::year_month_day& initialStart, const std::chrono::year_month_day& initialEnd, DateRangeConfirm onConfirm);

    protected:
        [[nodiscard]] float MeasureContentHeight(Core::Renderer& renderer, float contentWidth) override;
        void LayoutContent(const Core::RectF& contentArea) override;
        void FocusInitialControl() override;
        void OnSubmit() override;
        void OnCancel() override;

    private:
        void Confirm();
        void Cancel();
        void ShowPreview();                  // 두 날짜가 올바르면 주 수 · 요일 미리보기, 아니면 형식 안내
        void ShowError(std::wstring message);

        // 부품은 대화상자의 ControlHost가 소유 (참조는 배치 · 값 읽기용)
        Label& m_messageLabel;
        Label& m_startLabel;
        TextBox& m_startBox;
        Label& m_endLabel;
        TextBox& m_endBox;
        Label& m_statusLabel;      // 평소엔 미리보기 · 형식 안내, 잘못 입력하면 빨간 오류 문구
        Button& m_confirmButton;
        Button& m_cancelButton;
        float m_messageHeight{};

        DateRangeConfirm m_onConfirm;
    };
}
