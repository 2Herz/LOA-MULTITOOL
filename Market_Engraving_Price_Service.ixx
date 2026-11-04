export module LostArkMultiTool.Services.MarketEngravingPriceService;

import std;
import LostArkMultiTool.Core.EngravingPriceService;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.EngravingQuote;
import LostArkMultiTool.Models.MarketItem;

export namespace LostArkMultiTool::Services
{
    // 거래소 기반 유물 각인서 시세 (조회 결과는 실행 중에만 보관 → 등락은 직전 갱신 대비)
    class MarketEngravingPriceService final : public Core::EngravingPriceService
    {
    public:
        MarketEngravingPriceService();

        [[nodiscard]] const std::vector<Models::EngravingQuote>& GetQuotes() const noexcept override;
        [[nodiscard]] const std::optional<std::int64_t>& GetLastUpdated() const noexcept override;

        [[nodiscard]] std::vector<Models::MarketItem> FetchQuotes(Core::MarketAPI& api) const override;
        void ApplyQuotes(std::vector<Models::MarketItem> items) override;

    private:
        std::vector<Models::EngravingQuote> m_quotes;
        std::optional<std::int64_t> m_lastUpdated;
    };
}
