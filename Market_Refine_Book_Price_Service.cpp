module LostArkMultiTool.Services.MarketRefineBookPriceService;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.RefineBookPriceService;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RefineBookQuote;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 표에 보여 줄 책 · 순서 (게임 아이템 사전 [재련 추가 재료] 순서, 띄어쓰기를 뺀 이름, "{}" 자리에 야금술 · 재봉술) =====
        // 장인의 1 · 2단계 → 업화 [11-14] · [15-18] · [19-20] → 강화 업화 [19-20] → 업화 [11-14] (귀속)
        // → 장인의 3단계 → 전율 [12-15] · [16-19] → 장인의 4단계
        // 이 목록에 없는 책(이전 티어 · 다른 재료)은 표에서 뺌
        constexpr std::size_t BOOK_ORDER_COUNT{ 11 };
        constexpr std::array<std::string_view, BOOK_ORDER_COUNT> BOOK_ORDER_FORMATS
        {
            "장인의{}:1단계", "장인의{}:2단계",
            "{}:업화[11-14]", "{}:업화[15-18]", "{}:업화[19-20]", "강화{}:업화[19-20]", "{}:업화[11-14](귀속)",
            "장인의{}:3단계",
            "{}:전율[12-15]", "{}:전율[16-19]",
            "장인의{}:4단계"
        };

        constexpr std::string_view IGNORED_CHARACTERS{ " \t" };   // 이름을 비교할 때 무시하는 글자

        // 띄어쓰기를 뺀 이름 ("야금술 : 업화 [15-18]" → "야금술:업화[15-18]")
        [[nodiscard]] std::string RemoveSpaces(std::string_view text)
        {
            std::string compact;
            compact.reserve(text.size());
            for (const char character : text)
            {
                if (!IGNORED_CHARACTERS.contains(character)) compact.push_back(character);
            }
            return compact;
        }

        // 정해 둔 순서의 번호 (목록에 없는 이름 = BOOK_ORDER_FORMATS.size() → 표에서 뺌)
        [[nodiscard]] std::size_t GetOrderIndex(std::string_view name, std::string_view keyword)
        {
            const std::string compactName{ RemoveSpaces(name) };
            for (std::size_t index{}; index < BOOK_ORDER_FORMATS.size(); ++index)
            {
                if (std::vformat(BOOK_ORDER_FORMATS[index], std::make_format_args(keyword)) == compactName) return index;
            }
            return BOOK_ORDER_FORMATS.size();
        }

        // 이름으로 종류 판정 (야금술 · 재봉술 둘 다 아니면 nullopt → 다른 재료는 버림)
        [[nodiscard]] std::optional<Models::RefineBookKind> FindKind(std::string_view name) noexcept
        {
            if (name.contains(Core::APPConstants::METALLURGY_KEYWORD)) return Models::RefineBookKind::BOOK_METALLURGY;
            if (name.contains(Core::APPConstants::TAILORING_KEYWORD)) return Models::RefineBookKind::BOOK_TAILORING;
            return std::nullopt;
        }

        [[nodiscard]] std::string_view GetKeyword(Models::RefineBookKind kind) noexcept
        {
            return kind == Models::RefineBookKind::BOOK_METALLURGY ? Core::APPConstants::METALLURGY_KEYWORD : Core::APPConstants::TAILORING_KEYWORD;
        }
    }

    MarketRefineBookPriceService::MarketRefineBookPriceService() = default;

    const std::vector<Models::RefineBookQuote>& MarketRefineBookPriceService::GetQuotes() const noexcept
    {
        return m_quotes;
    }

    const std::optional<std::int64_t>& MarketRefineBookPriceService::GetLastUpdated() const noexcept
    {
        return m_lastUpdated;
    }

    // 강화 재료 카테고리에서 "야금술" · "재봉술" 이름으로 각각 검색 (등급은 섞여 있으므로 조건 없이)
    std::vector<Models::MarketItem> MarketRefineBookPriceService::FetchQuotes(Core::MarketAPI& api) const
    {
        std::vector<Models::MarketItem> items{ api.SearchMarketItems(Core::APPConstants::REFINE_MATERIAL_CATEGORY_CODE,
            std::string_view{}, Core::APPConstants::METALLURGY_KEYWORD) };
        std::vector<Models::MarketItem> tailoringItems{ api.SearchMarketItems(Core::APPConstants::REFINE_MATERIAL_CATEGORY_CODE,
            std::string_view{}, Core::APPConstants::TAILORING_KEYWORD) };

        items.insert(items.end(), std::make_move_iterator(tailoringItems.begin()), std::make_move_iterator(tailoringItems.end()));
        return items;
    }

    // 종류로 나누고 정해 둔 순서로 정렬 + 같은 이름의 이전 최저가를 등락 기준으로 기억
    void MarketRefineBookPriceService::ApplyQuotes(std::vector<Models::MarketItem> items)
    {
        // 이름 → 이전 최저가 (매물이 없던 책은 비교 기준이 없으므로 제외)
        std::unordered_map<std::string, double> previousPrices;
        previousPrices.reserve(m_quotes.size());
        for (const Models::RefineBookQuote& quote : m_quotes)
        {
            if (quote.m_item.m_currentMinPrice > 0.0) previousPrices.emplace(quote.m_item.m_name, quote.m_item.m_currentMinPrice);
        }

        std::vector<Models::RefineBookQuote> quotes;
        quotes.reserve(items.size());
        std::unordered_set<std::string> seenNames;   // 두 검색 결과에 같은 책이 겹치면 한 번만
        for (Models::MarketItem& item : items)
        {
            const std::optional<Models::RefineBookKind> kind{ FindKind(item.m_name) };
            if (!kind.has_value()) continue;
            if (GetOrderIndex(item.m_name, GetKeyword(*kind)) >= BOOK_ORDER_FORMATS.size()) continue;   // 사전 목록에 없는 책은 제외
            if (!seenNames.insert(item.m_name).second) continue;

            const auto previous{ previousPrices.find(item.m_name) };
            quotes.push_back(Models::RefineBookQuote{ *kind, std::move(item),
                previous != previousPrices.end() ? std::optional<double>{ previous->second } : std::nullopt });
        }

        // 종류(야금술 → 재봉술) → 정해 둔 순서
        std::ranges::stable_sort(quotes, [](const Models::RefineBookQuote& left, const Models::RefineBookQuote& right)
        {
            if (left.m_kind != right.m_kind) return left.m_kind < right.m_kind;
            return GetOrderIndex(left.m_item.m_name, GetKeyword(left.m_kind)) < GetOrderIndex(right.m_item.m_name, GetKeyword(right.m_kind));
        });

        m_quotes = std::move(quotes);
        m_lastUpdated = Core::LocalTime::GetUnixSeconds();
    }
}
