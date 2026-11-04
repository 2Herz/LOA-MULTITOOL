export module LostArkMultiTool.Core.AccessorySearchService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;

export namespace LostArkMultiTool::Core
{
    // 악세 검색 (악세 검색기의 모델)
    // 두 조회 함수 모두 데이터를 건드리지 않아 백그라운드 스레드에서 호출해도 안전
    // 받은 결과(옵션 목록 · 현재 페이지)는 화면 스레드에서 매니저가 보관
    class AccessorySearchService
    {
    public:
        virtual ~AccessorySearchService();

        // 악세에 거는 연마 효과 옵션 목록 (API 검색 옵션 중 연마 효과 묶음만, 없으면 전체)
        [[nodiscard]] virtual std::vector<Models::AuctionOption> FetchOptions(MarketAPI& api) const = 0;

        // 부위에 맞는 딜러 유효옵션만 골라냄 (정해 둔 순서대로, 하나도 맞지 않으면 전체를 그대로)
        [[nodiscard]] virtual std::vector<Models::AuctionOption> FilterOptions(const std::vector<Models::AuctionOption>& options, int categoryCode) const = 0;

        // 조건에 맞는 매물 한 페이지 (즉시 구매가 낮은 순)
        [[nodiscard]] virtual Models::AccessorySearchPage Search(MarketAPI& api, const Models::AccessorySearchQuery& query) const = 0;
    };
}
