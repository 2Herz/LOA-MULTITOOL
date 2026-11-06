export module LostArkMultiTool.Services.WinHTTPMarketAPI;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RateLimitState;
import LostArkMultiTool.Services.JSONValue;
import LostArkMultiTool.Services.WinHTTPClient;

export namespace LostArkMultiTool::Services
{
    // 로스트아크 Open API 통신 (거래소 시세 · 거래소 목록 · 원정대 목록 · 경매장 보석 최저가)
    // 모든 요청 메서드는 블로킹이므로 백그라운드 스레드에서 호출할 것 (여러 스레드에서 동시에 불러도 안전)
    class WinHTTPMarketAPI final : public Core::MarketAPI
    {
    public:
        explicit WinHTTPMarketAPI(std::string apiKey);

        WinHTTPMarketAPI(const WinHTTPMarketAPI&) = delete;
        WinHTTPMarketAPI& operator=(const WinHTTPMarketAPI&) = delete;

        void SetAPIKey(std::string apiKey) override;

        [[nodiscard]] double GetPrice(std::string_view itemName, bool isShard, bool isBundle) override;
        [[nodiscard]] std::vector<Models::CharacterInfo> GetRoster(std::string_view charName) override;
        [[nodiscard]] Models::CharacterArmory GetArmory(std::string_view charName) override;
        [[nodiscard]] std::vector<Models::MarketItem> SearchMarketItems(int categoryCode, std::string_view itemGrade, std::string_view itemName) override;
        [[nodiscard]] std::vector<Models::AuctionOption> GetAuctionOptions() override;
        [[nodiscard]] Models::AccessorySearchPage SearchAccessories(const Models::AccessorySearchQuery& query) override;
        [[nodiscard]] std::optional<std::int64_t> GetLowestBuyPrice(std::string_view itemName, int itemTier, std::stop_token stopToken) override;
        [[nodiscard]] Models::RateLimitState GetRateLimit() const override;

    private:
        // 요청마다 붙일 헤더 (accept + content-type + 인증)
        [[nodiscard]] std::wstring BuildHeaders();

        // 요청 1회 + 응답 헤더로 호출 한도 갱신 (성공 · 실패와 상관없이 헤더부터 기록)
        [[nodiscard]] HTTPResponse SendRequest(std::wstring_view verb, std::wstring_view path, std::string body);
        void UpdateRateLimit(const HTTPResponse& response);

        // 남은 호출이 안전 여유분 이하인지 (경매장 페이지를 더 넘기기 전에 확인)
        [[nodiscard]] bool IsRateLimitExhausted() const;

        // 경매장 검색 (일시적 오류는 잠깐 기다렸다가 한 번 더 시도, 401 · 429 · 503처럼 다시 해도 같은 오류는 바로 전달)
        [[nodiscard]] JSONValue SearchAuctionWithRetry(std::string_view itemName, int itemTier, int page, std::stop_token stopToken);
        [[nodiscard]] JSONValue SearchAuction(std::string_view itemName, int itemTier, int page);

        WinHTTPClient m_client;                 // WinHTTP 세션·연결 (Windows 타입은 WinHTTPClient 안에 숨겨져 있음)
        std::mutex m_headerMutex;               // SetAPIKey(UI 스레드) ↔ 요청(백그라운드 스레드) 보호
        std::wstring m_authorizationHeader;     // "authorization: Bearer <키>\r\n"

        mutable std::mutex m_rateMutex;         // 동시 요청의 응답 ↔ 화면 읽기 보호
        Models::RateLimitState m_rateLimit;     // 마지막으로 알아낸 호출 한도 상태 (세 툴 공용)
    };
}