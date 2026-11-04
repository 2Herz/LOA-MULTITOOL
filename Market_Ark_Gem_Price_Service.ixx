export module LostArkMultiTool.Services.MarketArkGemPriceService;

import std;
import LostArkMultiTool.Core.ArkGemPriceService;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.ArkGemQuote;
import LostArkMultiTool.Models.MarketItem;

export namespace LostArkMultiTool::Services
{
    // 거래소 기반 아크 그리드 젬 시세 (조회 결과는 실행 중에만 보관 → 등락은 직전 갱신 대비)
    class MarketArkGemPriceService final : public Core::ArkGemPriceService
    {
    public:
        MarketArkGemPriceService();

        [[nodiscard]] const std::vector<Models::ArkGemQuote>& GetQuotes() const noexcept override;
        [[nodiscard]] const std::optional<std::int64_t>& GetLastUpdated() const noexcept override;

        [[nodiscard]] std::vector<Models::MarketItem> FetchQuotes(Core::MarketAPI& api) const override;
        void ApplyQuotes(std::vector<Models::MarketItem> items) override;

    private:
        std::vector<Models::ArkGemQuote> m_quotes;
        std::optional<std::int64_t> m_lastUpdated;
    };
}
