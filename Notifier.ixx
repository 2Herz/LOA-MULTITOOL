export module LostArkMultiTool.Core.Notifier;

import std;

export namespace LostArkMultiTool::Core
{
    // 알림 종류 (색 · 표시 시간이 달라짐)
    enum class NoticeKind
    {
        NOTICE_INFO,
        NOTICE_SUCCESS,
        NOTICE_WARNING,
        NOTICE_ERROR
    };

    // 사용자 알림 인터페이스 (원본 MessageBox · 풍선 알림 대응 → 실제 구현은 Views::UI::OverlayNotifier)
    // 대화상자는 모달: 열려 있는 동안 탭 · 화면 부품은 입력을 받지 않음
    class Notifier
    {
    public:
        virtual ~Notifier();

        // 확인 버튼 하나짜리 대화상자 (닫히면 onClosed 호출, 필요 없으면 nullptr)
        virtual void ShowMessage(std::wstring title, std::wstring message, NoticeKind kind, std::function<void()> onClosed) = 0;

        // 예 / 아니요 대화상자 (답을 고르면 onAnswer(예 = true) 호출)
        virtual void AskYesNo(std::wstring title, std::wstring message, std::function<void(bool isYes)> onAnswer) = 0;

        // 기간(시작일 ~ 종료일) 입력받기 (낙원력 트래커 시즌 기간, 날짜 형식 "yyyy-MM-dd")
        // [확인]을 누르면 올바른 두 날짜(종료일 ≥ 시작일)로 onConfirm 호출 → 오류 문구를 돌려주면 창을 닫지 않고 그 문구를 표시, nullopt면 닫힘
        // [취소] · Esc는 아무것도 하지 않고 닫힘
        virtual void AskDateRange(std::wstring title, std::wstring message,
            const std::chrono::year_month_day& initialStart, const std::chrono::year_month_day& initialEnd,
            std::function<std::optional<std::wstring>(const std::chrono::year_month_day& startDate, const std::chrono::year_month_day& endDate)> onConfirm) = 0;

        // 잠깐 떴다가 사라지는 알림 (입력을 막지 않음, 창이 트레이에 숨어 있으면 트레이 풍선 알림으로)
        virtual void ShowToast(std::wstring message, NoticeKind kind) = 0;
    };
}