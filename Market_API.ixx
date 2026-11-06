export module LostArkMultiTool.Core.MarketAPI;

import std;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RateLimitState;

export namespace LostArkMultiTool::Core
{
    // 로스트아크 Open API (거래소 시세 · 거래소 목록 · 원정대 목록 · 경매장 보석 최저가)
    // 메서드는 블로킹이므로 화면(UI) 스레드가 아닌 백그라운드 스레드에서 호출할 것
    // 세 툴이 키 하나를 함께 쓰므로 호출 한도(분당 100회)도 이 객체 하나가 모든 응답 헤더로 추적함
    class MarketAPI
    {
    public:
        virtual ~MarketAPI();

        virtual void SetAPIKey(std::string apiKey) = 0;

        [[nodiscard]] virtual double GetPrice(std::string_view itemName, bool isShard, bool isBundle) = 0;
        [[nodiscard]] virtual std::vector<Models::CharacterInfo> GetRoster(std::string_view charName) = 0;

        // 캐릭터 무기고 (프로필 · 장비 · 보석 → 전투력 시뮬레이터, 없는 캐릭터면 std::runtime_error)
        [[nodiscard]] virtual Models::CharacterArmory GetArmory(std::string_view charName) = 0;

        // 거래소 카테고리의 아이템 전체 (현재 최저가 높은 순, 마지막 페이지까지 넘겨서 모음 → 유각 · 강화책 검색기)
        // itemGrade · itemName이 비어 있으면 그 조건 없이 (itemName은 부분 일치)
        [[nodiscard]] virtual std::vector<Models::MarketItem> SearchMarketItems(int categoryCode, std::string_view itemGrade, std::string_view itemName) = 0;

        // 경매장 검색 옵션 전체 (EtcOptions의 모든 묶음 · 옵션 · 값, 악세 검색기의 연마 효과 선택지)
        [[nodiscard]] virtual std::vector<Models::AuctionOption> GetAuctionOptions() = 0;

        // 경매장 악세 검색 한 페이지 (즉시 구매가 낮은 순)
        [[nodiscard]] virtual Models::AccessorySearchPage SearchAccessories(const Models::AccessorySearchQuery& query) = 0;

        // 경매장 즉시 구매가 기준 최저가 (매물이 없으면 nullopt)
        // 같은 회차의 다른 요청이 실패하면 stopToken으로 나머지를 중단 (중단되면 RequestCanceledException → 결과는 버려짐)
        [[nodiscard]] virtual std::optional<std::int64_t> GetLowestBuyPrice(std::string_view itemName, int itemTier, std::stop_token stopToken) = 0;

        // 마지막 응답 헤더 기준 호출 한도 상태 (복사본 → 읽는 도중 값이 바뀌지 않음, 어느 스레드에서 불러도 안전)
        [[nodiscard]] virtual Models::RateLimitState GetRateLimit() const = 0;
    };
}