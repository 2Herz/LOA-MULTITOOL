export module LostArkMultiTool.Services.AuctionAccessorySearchService;

import std;
import LostArkMultiTool.Core.AccessorySearchService;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;

export namespace LostArkMultiTool::Services
{
    // 경매장 기반 악세 검색 (연마 효과 선택지는 API 검색 옵션에서 골라냄 → 게임 패치로 옵션이 바뀌어도 그대로 따라감)
    class AuctionAccessorySearchService final : public Core::AccessorySearchService
    {
    public:
        AuctionAccessorySearchService();

        [[nodiscard]] std::vector<Models::AuctionOption> FetchOptions(Core::MarketAPI& api) const override;
        [[nodiscard]] std::vector<Models::AuctionOption> FilterOptions(const std::vector<Models::AuctionOption>& options, int categoryCode) const override;
        [[nodiscard]] Models::AccessorySearchPage Search(Core::MarketAPI& api, const Models::AccessorySearchQuery& query) const override;
    };
}
