export module LostArkMultiTool.Services.WinHTTPMarketAPI;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Services.WinHTTPClient;

export namespace LostArkMultiTool::Services
{
    // 로스트아크 Open API 통신 (거래소 시세 · 원정대 목록)
    // 모든 메서드는 블로킹이므로 백그라운드 스레드에서 호출할 것
    class WinHTTPMarketAPI final : public Core::MarketAPI
    {
    public:
        explicit WinHTTPMarketAPI(std::string apiKey);

        WinHTTPMarketAPI(const WinHTTPMarketAPI&) = delete;
        WinHTTPMarketAPI& operator=(const WinHTTPMarketAPI&) = delete;

        void SetAPIKey(std::string apiKey) override;

        [[nodiscard]] double GetPrice(std::string_view itemName, bool isShard, bool isBundle) override;
        [[nodiscard]] std::vector<Models::CharacterInfo> GetRoster(std::string_view charName) override;

    private:
        // 요청마다 붙일 헤더 (accept + content-type + 인증)
        [[nodiscard]] std::wstring BuildHeaders();

        WinHTTPClient m_client;                 // WinHTTP 세션·연결 (Windows 타입은 WinHTTPClient 안에 숨겨져 있음)
        std::mutex m_headerMutex;               // SetAPIKey(UI 스레드) ↔ 요청(백그라운드 스레드) 보호
        std::wstring m_authorizationHeader;     // "authorization: Bearer <키>\r\n"
    };
}