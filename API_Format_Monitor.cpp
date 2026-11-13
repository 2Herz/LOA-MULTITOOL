module LostArkMultiTool.Core.APIFormatMonitor;

import std;
import LostArkMultiTool.Core.AppLogger;
import LostArkMultiTool.Core.TextEncoding;

namespace LostArkMultiTool::Core::APIFormatMonitor
{
    namespace
    {
        // 알림 문구 ({} = 응답 이름)
        constexpr std::wstring_view NOTICE_FORMAT{ L"로스트아크 API 응답 형식이 바뀐 것 같아요 ({}). 일부 값이 비어 보일 수 있어요. [문제 보고]로 알려 주세요!" };

        std::mutex g_mutex;                          // 조회 스레드 · 화면 스레드 동시 접근 보호
        std::set<std::string> g_reportedContexts;    // 이미 신고된 응답 이름 (실행 중 한 번만)
        std::deque<std::string> g_pendingContexts;   // 아직 화면에 알리지 않은 응답 이름
    }

    void Report(std::string_view context, std::string_view detail) noexcept
    {
        try
        {
            const std::scoped_lock lock{ g_mutex };                                        // 동시 접근 보호
            if (!g_reportedContexts.emplace(context).second) return;                       // 이미 신고됨
            g_pendingContexts.emplace_back(context);                                       // 화면 알림 대기
            AppLogger::GetInstance().Warning(std::format("API 형식 변경 의심 [{}] {}", context, detail));   // 로그 기록
        }
        catch (...)
        {
            // 신고 실패가 조회를 멈추게 하지 않음
        }
    }

    std::optional<std::wstring> TakePendingNotice()
    {
        std::string context;                                                               // 꺼낸 응답 이름
        {
            const std::scoped_lock lock{ g_mutex };                                        // 동시 접근 보호
            if (g_pendingContexts.empty()) return std::nullopt;                            // 알릴 것 없음
            context = std::move(g_pendingContexts.front());                                // 가장 먼저 신고된 것
            g_pendingContexts.pop_front();                                                 // 꺼냄
        }
        const std::wstring wideContext{ TextEncoding::UTF8ToWide(context) };               // 화면용 UTF-16
        return std::vformat(NOTICE_FORMAT, std::make_wformat_args(wideContext));           // 알림 문구
    }
}
