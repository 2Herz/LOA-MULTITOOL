export module LostArkMultiTool.Core.APIFormatMonitor;

import std;

// API 응답 형식 변경 감시 (옵저버 대신 "알림 우편함" 방식)
// 백그라운드 조회 스레드가 예상한 필드가 없다고 신고하면 로그에 남기고, 화면 스레드가 매 프레임 꺼내서 한 번만 알림
// → 형식이 바뀌어 값이 조용히 0으로 처리되는 일을 사용자 · 개발자가 바로 알 수 있게 함
export namespace LostArkMultiTool::Core::APIFormatMonitor
{
    // 형식 이상 신고 (context = 어느 응답인지, detail = 무엇이 없는지 / 같은 context는 실행 중 한 번만 기록 · 알림)
    void Report(std::string_view context, std::string_view detail) noexcept;

    // 아직 보여 주지 않은 알림 문구 꺼내기 (없으면 nullopt, 화면 스레드에서 호출)
    [[nodiscard]] std::optional<std::wstring> TakePendingNotice();
}
