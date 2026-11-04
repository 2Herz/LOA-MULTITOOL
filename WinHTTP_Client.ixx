export module LostArkMultiTool.Services.WinHTTPClient;

import std;

export namespace LostArkMultiTool::Services
{
    // HTTP 응답 (상태 코드 + 본문 + 헤더)
    struct HTTPResponse
    {
        int m_statusCode{};
        std::string m_body;
        std::wstring m_headers;                     // 응답 헤더 원문 ("이름: 값\r\n" 줄들)
        std::optional<std::int64_t> m_serverTime;   // Date 헤더 (서버 시각, UTC 초 — 없으면 nullopt)
    };

    // 네트워크 연결 실패 · 시간 초과 (서버가 응답하지 못한 일시적 오류 → 재시도 판단에 사용)
    class HTTPNetworkError : public std::runtime_error
    {
    public:
        explicit HTTPNetworkError(const std::string& message);
    };

    // 응답 헤더 값 찾기 (이름은 대소문자 구분 없음, 여러 개면 첫 번째, 없으면 nullopt)
    [[nodiscard]] std::optional<std::wstring> FindHeader(const HTTPResponse& response, std::wstring_view name);

    // WinHTTP 기반 HTTPS 클라이언트 (호스트 하나에 대한 세션·연결을 만들어 두고 요청마다 재사용)
    // 시세 API와 원격 설정 확인이 같이 사용 / 요청은 블로킹이므로 백그라운드 스레드에서 호출할 것
    class WinHTTPClient
    {
    public:
        // host: 예) L"developer-lostark.game.onstove.com", timeoutMs: 연결·송신·수신 각각의 최대 대기 시간
        WinHTTPClient(std::wstring_view host, int timeoutMs);
        ~WinHTTPClient();

        WinHTTPClient(const WinHTTPClient&) = delete;
        WinHTTPClient& operator=(const WinHTTPClient&) = delete;

        // verb: L"GET" / L"POST", headers: "이름: 값\r\n"을 이어 붙인 문자열 (없으면 빈 값)
        // 네트워크 오류·시간 초과는 HTTPNetworkError, HTTP 오류 상태는 그대로 응답에 담아 돌려줌
        [[nodiscard]] HTTPResponse Send(std::wstring_view verb, std::wstring_view path, std::wstring_view headers, std::string body);

        // 2xx 성공 상태인지
        [[nodiscard]] static bool IsSuccessStatus(int statusCode) noexcept;

    private:
        // WinHTTP 핸들은 구현 파일 안에만 두어 Windows 헤더 매크로가 다른 파일로 새지 않게 함
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}