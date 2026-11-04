export module LostArkMultiTool.Core.ServerConfigChecker;

import std;
import LostArkMultiTool.Models.ServerConfig;

export namespace LostArkMultiTool::Core
{
    // 원격 설정(점검 스위치 + 최신 버전 + 공지) 확인 인터페이스
    // 실행 직후 확인과 실행 중 주기 확인(CONFIG_CHECK_INTERVAL_MS)이 같이 사용
    class ServerConfigChecker
    {
    public:
        virtual ~ServerConfigChecker();

        // 원격 설정 받기 (네트워크 오류·형식 오류·설정 없음이면 nullopt, 예외를 밖으로 던지지 않음)
        // 블로킹이므로 백그라운드 스레드에서 호출할 것
        [[nodiscard]] virtual std::optional<Models::ServerConfig> Fetch() = 0;

        // 현재 프로그램 버전 (APPConstants::APP_VERSION)
        [[nodiscard]] static std::string_view GetCurrentVersion() noexcept;

        // 서버 쪽 버전이 더 높을 때만 true ("1.0.5"와 "1.0.5.0"처럼 자릿수가 달라도 같은 버전으로 취급)
        [[nodiscard]] static bool IsNewerVersion(std::string_view latest, std::string_view current);
    };
}