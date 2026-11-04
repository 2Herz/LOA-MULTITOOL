module LostArkMultiTool.Services.MarketArkGemPriceService;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.ArkGemPriceService;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.ArkGemQuote;
import LostArkMultiTool.Models.MarketItem;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 표에 보여 줄 순서 (이름 끝 글자로 판정, 게임 거래소와 같은 순서) =====
        constexpr std::size_t GEM_ORDER_COUNT{ 3 };
        constexpr std::array<std::string_view, GEM_ORDER_COUNT> ORDER_GEM_SUFFIXES{ "안정", "불변", "견고" };
        constexpr std::array<std::string_view, GEM_ORDER_COUNT> CHAOS_GEM_SUFFIXES{ "침식", "왜곡", "붕괴" };

        // 이름으로 종류 판정 (질서 · 혼돈 둘 다 아니면 nullopt → 다른 재료는 버림)
        [[nodiscard]] std::optional<Models::ArkGemKind> FindKind(std::string_view name) noexcept
        {
            if (name.contains(Core::APPConstants::ORDER_GEM_KEYWORD)) return Models::ArkGemKind::GEM_ORDER;
            if (name.contains(Core::APPConstants::CHAOS_GEM_KEYWORD)) return Models::ArkGemKind::GEM_CHAOS;
            return std::nullopt;
        }

        // 정해 둔 순서의 번호 (목록에 없는 이름은 맨 뒤 → 받은 순서 유지)
        [[nodiscard]] std::size_t GetOrderIndex(const Models::ArkGemQuote& quote) noexcept
        {
            const auto& suffixes{ quote.m_kind == Models::ArkGemKind::GEM_ORDER ? ORDER_GEM_SUFFIXES : CHAOS_GEM_SUFFIXES };
            for (std::size_t index{}; index < suffixes.size(); ++index)
            {
                if (quote.m_item.m_name.ends_with(suffixes[index])) return index;
            }
            return suffixes.size();
        }
    }

    MarketArkGemPriceService::MarketArkGemPriceService() = default;

    const std::vector<Models::ArkGemQuote>& MarketArkGemPriceService::GetQuotes() const noexcept
    {
        return m_quotes;
    }

    const std::optional<std::int64_t>& MarketArkGemPriceService::GetLastUpdated() const noexcept
    {
        return m_lastUpdated;
    }

    // 강화 재료 카테고리에서 "질서의 젬" · "혼돈의 젬"을 각각 검색 (귀속 젬 계산기에 고급 · 희귀 단가도 필요 → 등급 조건 없이)
    std::vector<Models::MarketItem> MarketArkGemPriceService::FetchQuotes(Core::MarketAPI& api) const
    {
        std::vector<Models::MarketItem> items{ api.SearchMarketItems(Core::APPConstants::REFINE_MATERIAL_CATEGORY_CODE,
            std::string_view{}, Core::APPConstants::ORDER_GEM_KEYWORD) };
        std::vector<Models::MarketItem> chaosItems{ api.SearchMarketItems(Core::APPConstants::REFINE_MATERIAL_CATEGORY_CODE,
            std::string_view{}, Core::APPConstants::CHAOS_GEM_KEYWORD) };

        items.insert(items.end(), std::make_move_iterator(chaosItems.begin()), std::make_move_iterator(chaosItems.end()));
        return items;
    }

    // 종류로 나누고 정해 둔 순서로 정렬 + 같은 이름의 이전 최저가를 등락 기준으로 기억
    void MarketArkGemPriceService::ApplyQuotes(std::vector<Models::MarketItem> items)
    {
        // 이름 + 등급 → 이전 최저가 (매물이 없던 젬은 비교 기준이 없으므로 제외)
        std::unordered_map<std::string, double> previousPrices;
        previousPrices.reserve(m_quotes.size());
        for (const Models::ArkGemQuote& quote : m_quotes)
        {
            if (quote.m_item.m_currentMinPrice > 0.0) previousPrices.emplace(quote.m_item.m_name + quote.m_item.m_gradeName, quote.m_item.m_currentMinPrice);
        }

        std::vector<Models::ArkGemQuote> quotes;
        quotes.reserve(items.size());
        std::unordered_set<std::string> seenItems;   // 두 검색 결과에 같은 젬 · 등급이 겹치면 한 번만
        for (Models::MarketItem& item : items)
        {
            const std::optional<Models::ArkGemKind> kind{ FindKind(item.m_name) };
            if (!kind.has_value() || !seenItems.insert(item.m_name + item.m_gradeName).second) continue;

            const auto previous{ previousPrices.find(item.m_name + item.m_gradeName) };
            quotes.push_back(Models::ArkGemQuote{ *kind, std::move(item),
                previous != previousPrices.end() ? std::optional<double>{ previous->second } : std::nullopt });
        }

        // 종류(질서 → 혼돈) → 정해 둔 순서 (목록에 없는 젬은 받은 순서대로 뒤에)
        std::ranges::stable_sort(quotes, [](const Models::ArkGemQuote& left, const Models::ArkGemQuote& right)
        {
            if (left.m_kind != right.m_kind) return left.m_kind < right.m_kind;
            return GetOrderIndex(left) < GetOrderIndex(right);
        });

        m_quotes = std::move(quotes);
        m_lastUpdated = Core::LocalTime::GetUnixSeconds();
    }
}
