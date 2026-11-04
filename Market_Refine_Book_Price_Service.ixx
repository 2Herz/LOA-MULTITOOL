export module LostArkMultiTool.Services.MarketRefineBookPriceService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.RefineBookPriceService;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RefineBookQuote;

export namespace LostArkMultiTool::Services
{
    // 거래소 기반 강화책 시세 (조회 결과는 실행 중에만 보관 → 등락은 직전 갱신 대비)
    class MarketRefineBookPriceService final : public Core::RefineBookPriceService
    {
    public:
        MarketRefineBookPriceService();

        [[nodiscard]] const std::vector<Models::RefineBookQuote>& GetQuotes() const noexcept override;
        [[nodiscard]] const std::optional<std::int64_t>& GetLastUpdated() const noexcept override;

        [[nodiscard]] std::vector<Models::MarketItem> FetchQuotes(Core::MarketAPI& api) const override;
        void ApplyQuotes(std::vector<Models::MarketItem> items) override;

    private:
        std::vector<Models::RefineBookQuote> m_quotes;
        std::optional<std::int64_t> m_lastUpdated;
    };
}
