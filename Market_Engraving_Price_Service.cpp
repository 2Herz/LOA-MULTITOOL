module LostArkMultiTool.Services.MarketEngravingPriceService;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.EngravingPriceService;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.EngravingQuote;
import LostArkMultiTool.Models.MarketItem;

namespace LostArkMultiTool::Services
{
    MarketEngravingPriceService::MarketEngravingPriceService() = default;

    const std::vector<Models::EngravingQuote>& MarketEngravingPriceService::GetQuotes() const noexcept
    {
        return m_quotes;
    }

    const std::optional<std::int64_t>& MarketEngravingPriceService::GetLastUpdated() const noexcept
    {
        return m_lastUpdated;
    }

    // 거래소 각인서 카테고리 중 유물 등급만
    std::vector<Models::MarketItem> MarketEngravingPriceService::FetchQuotes(Core::MarketAPI& api) const
    {
        return api.SearchMarketItems(Core::APPConstants::ENGRAVING_CATEGORY_CODE, Core::APPConstants::RELIC_GRADE_NAME, std::string_view{});
    }

    // 새 결과로 바꾸면서 같은 이름의 이전 최저가를 등락 기준으로 기억
    void MarketEngravingPriceService::ApplyQuotes(std::vector<Models::MarketItem> items)
    {
        // 이름 → 이전 최저가 (매물이 없던 각인서는 비교 기준이 없으므로 제외)
        std::unordered_map<std::string, double> previousPrices;
        previousPrices.reserve(m_quotes.size());
        for (const Models::EngravingQuote& quote : m_quotes)
        {
            if (quote.GetCurrentMinPrice() > 0.0) previousPrices.emplace(quote.GetName(), quote.GetCurrentMinPrice());
        }

        std::vector<Models::EngravingQuote> quotes;
        quotes.reserve(items.size());
        for (Models::MarketItem& item : items)
        {
            const auto previous{ previousPrices.find(item.m_name) };
            Models::EngravingQuote& quote{ quotes.emplace_back(std::move(item)) };
            if (previous != previousPrices.end()) quote.SetPreviousMinPrice(previous->second);
        }

        m_quotes = std::move(quotes);
        m_lastUpdated = Core::LocalTime::GetUnixSeconds();
    }
}
