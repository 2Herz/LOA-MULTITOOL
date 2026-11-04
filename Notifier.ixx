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

        // 확인 버튼 하나짜리 대화상자
        virtual void ShowMessage(std::wstring title, std::wstring message, NoticeKind kind) = 0;

        // 예 / 아니요 대화상자 (답을 고르면 onAnswer(예 = true) 호출)
        virtual void AskYesNo(std::wstring title, std::wstring message, std::function<void(bool isYes)> onAnswer) = 0;

        // 잠깐 떴다가 사라지는 알림 (입력을 막지 않음)
        virtual void ShowToast(std::wstring message, NoticeKind kind) = 0;
    };
}