module LostArkMultiTool.Services.AuctionAccessorySearchService;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.AccessorySearchService;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Models.AccessorySearchQuery;
import LostArkMultiTool.Models.AuctionOption;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 부위별 딜러 유효옵션 (띄어쓰기를 뺀 이름, 화면에 보여 줄 순서) =====
        // API 옵션 이름의 띄어쓰기가 달라도("공격력 +" / "공격력+") 같은 옵션으로 보도록 비교할 때 띄어쓰기를 뺌
        constexpr std::size_t DEALER_OPTION_COUNT{ 4 };
        using DealerOptionNames = std::array<std::string_view, DEALER_OPTION_COUNT>;
        constexpr std::size_t SUPPORTER_OPTION_COUNT{ 2 };   // 서포터 부위별 유효옵션 수
        using SupporterOptionNames = std::array<std::string_view, SUPPORTER_OPTION_COUNT>;   // 서포터 유효옵션 이름 묶음

        constexpr DealerOptionNames NECKLACE_DEALER_OPTIONS{ "추가피해", "적에게주는피해증가", "공격력+", "무기공격력+" };
        constexpr DealerOptionNames EARRING_DEALER_OPTIONS{ "공격력+", "공격력%", "무기공격력+", "무기공격력%" };
        constexpr DealerOptionNames RING_DEALER_OPTIONS{ "공격력+", "무기공격력+", "치명타적중률", "치명타피해" };

        // ===== 부위별 서포터 유효옵션 (lopec 서포터 측정에서 전투력에 들어가는 옵션) =====
        constexpr SupporterOptionNames NECKLACE_SUPPORTER_OPTIONS{ "낙인력", "세레나데,신앙,조화게이지획득량증가" };   // 목걸이: 낙인력 · 아덴 게이지
        constexpr SupporterOptionNames EARRING_SUPPORTER_OPTIONS{ "무기공격력%", "무기공격력+" };                // 귀걸이: 무기 공격력
        constexpr SupporterOptionNames RING_SUPPORTER_OPTIONS{ "아군공격력강화효과", "아군피해량강화효과" };       // 반지: 아공강 · 아피강

        constexpr std::string_view IGNORED_CHARACTERS{ " \t" };   // 비교할 때 무시하는 글자

        // 띄어쓰기를 뺀 이름
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

        // ===== 연마 효과 상 · 중 · 하 수치 (띄어쓰기를 뺀 이름 → 상 · 중 · 하 순서, 퍼센트 옵션은 % 단위) =====
        constexpr std::size_t POLISH_GRADE_COUNT{ 3 };
        struct PolishTier
        {
            std::string_view m_name;
            bool m_isPercent{};
            std::array<double, POLISH_GRADE_COUNT> m_amounts{};
        };

        constexpr std::size_t POLISH_TIER_COUNT{ 12 };
        constexpr std::array<PolishTier, POLISH_TIER_COUNT> POLISH_TIERS
        { {
            { "추가피해",           true,  { 2.6, 1.6, 0.7 } },
            { "적에게주는피해증가", true,  { 2.0, 1.2, 0.55 } },
            { "공격력+",            false, { 390.0, 195.0, 80.0 } },
            { "무기공격력+",        false, { 960.0, 480.0, 195.0 } },
            { "공격력%",            true,  { 1.55, 0.95, 0.4 } },
            { "무기공격력%",        true,  { 3.0, 1.8, 0.8 } },
            { "치명타적중률",       true,  { 1.55, 0.95, 0.4 } },
            { "치명타피해",         true,  { 4.0, 2.4, 1.1 } },
            { "낙인력",             true,  { 8.0, 4.8, 2.15 } },   // 서포터 목걸이
            { "세레나데,신앙,조화게이지획득량증가", true, { 6.0, 3.6, 1.6 } },   // 서포터 목걸이
            { "아군공격력강화효과", true,  { 5.0, 3.0, 1.35 } },   // 서포터 반지
            { "아군피해량강화효과", true,  { 7.5, 4.5, 2.0 } },    // 서포터 반지
        } };

        constexpr std::array<Models::PolishGrade, POLISH_GRADE_COUNT> POLISH_GRADES
        {
            Models::PolishGrade::GRADE_HIGH, Models::PolishGrade::GRADE_MIDDLE, Models::PolishGrade::GRADE_LOW
        };
        constexpr std::array<std::string_view, POLISH_GRADE_COUNT> POLISH_GRADE_NAMES{ "상", "중", "하" };

        constexpr double AMOUNT_TOLERANCE{ 0.001 };       // 표시 수치 비교 오차 (소수 둘째 자리까지)
        constexpr double PERCENT_REQUEST_SCALE{ 100.0 };  // API 값을 못 찾았을 때: 2.60% → 260 (API 표시 "0.60%" = 값 60 기준)

        // 표시 글자에서 첫 숫자 읽기 ("+2.60%" → 2.6, "1,234" → 1234, 숫자가 없으면 nullopt)
        [[nodiscard]] std::optional<double> ParseAmount(std::string_view text)
        {
            const std::size_t first{ text.find_first_of("0123456789") };
            if (first == std::string_view::npos) return std::nullopt;

            std::string digits;
            for (const char character : text.substr(first))
            {
                if (character == ',') continue;   // 천 단위 구분
                if (!std::isdigit(static_cast<unsigned char>(character)) && character != '.') break;
                digits.push_back(character);
            }

            double amount{};
            const auto [end, error]{ std::from_chars(digits.data(), digits.data() + digits.size(), amount) };
            if (error != std::errc{}) return std::nullopt;
            return amount;
        }

        // 수치 → 검색 요청 값: API 값 목록에서 같은 수치를 찾고, 없으면 API 값 · 표시 비율로 환산, 그것도 없으면 단위 규칙으로
        [[nodiscard]] int FindRequestValue(const Models::AuctionOption& option, double amount, bool isPercent)
        {
            std::optional<double> ratio;
            for (const Models::AuctionOptionValue& value : option.m_values)
            {
                const std::optional<double> shown{ ParseAmount(value.m_displayText) };
                if (!shown.has_value()) continue;
                if (std::abs(*shown - amount) < AMOUNT_TOLERANCE) return value.m_value;
                if (!ratio.has_value() && *shown > 0.0) ratio = static_cast<double>(value.m_value) / *shown;
            }

            const double scale{ ratio.value_or(isPercent ? PERCENT_REQUEST_SCALE : 1.0) };
            return static_cast<int>(std::lround(amount * scale));
        }

        // 딜러 유효옵션이면 값 목록을 상 · 중 · 하 세 개로 바꿈 (표에 없는 옵션은 API 값 그대로)
        void ApplyPolishTiers(Models::AuctionOption& option, std::string_view compactName)
        {
            const auto tier{ std::ranges::find(POLISH_TIERS, compactName, &PolishTier::m_name) };
            if (tier == POLISH_TIERS.end()) return;

            std::vector<Models::AuctionOptionValue> values;
            values.reserve(POLISH_GRADE_COUNT);
            for (std::size_t gradeIndex{}; gradeIndex < POLISH_GRADE_COUNT; ++gradeIndex)
            {
                const double amount{ tier->m_amounts[gradeIndex] };
                const std::string displayText{ tier->m_isPercent
                    ? std::format("{} (+{:.2f}%)", POLISH_GRADE_NAMES[gradeIndex], amount)
                    : std::format("{} (+{:.0f})", POLISH_GRADE_NAMES[gradeIndex], amount) };
                values.push_back(Models::AuctionOptionValue{ displayText, FindRequestValue(option, amount, tier->m_isPercent), POLISH_GRADES[gradeIndex] });
            }
            option.m_values = std::move(values);
        }

        // 부위 카테고리 · 역할 → 유효옵션 목록 (모르는 부위면 빈 목록)
        [[nodiscard]] std::span<const std::string_view> FindValidOptions(int categoryCode, bool isSupporter) noexcept
        {
            switch (categoryCode)   // 부위별 분기
            {
                case Core::APPConstants::NECKLACE_CATEGORY_CODE: return isSupporter ? std::span<const std::string_view>{ NECKLACE_SUPPORTER_OPTIONS } : std::span<const std::string_view>{ NECKLACE_DEALER_OPTIONS };   // 목걸이
                case Core::APPConstants::EARRING_CATEGORY_CODE:  return isSupporter ? std::span<const std::string_view>{ EARRING_SUPPORTER_OPTIONS } : std::span<const std::string_view>{ EARRING_DEALER_OPTIONS };     // 귀걸이
                case Core::APPConstants::RING_CATEGORY_CODE:     return isSupporter ? std::span<const std::string_view>{ RING_SUPPORTER_OPTIONS } : std::span<const std::string_view>{ RING_DEALER_OPTIONS };           // 반지
                default:                                         return {};   // 모르는 부위
            }
        }
    }

    AuctionAccessorySearchService::AuctionAccessorySearchService() = default;

    // 묶음 이름에 "연마"가 들어간 옵션만 (찾지 못하면 이름이 바뀌었을 수 있으니 전체를 그대로 보여 줌)
    std::vector<Models::AuctionOption> AuctionAccessorySearchService::FetchOptions(Core::MarketAPI& api) const
    {
        std::vector<Models::AuctionOption> options{ api.GetAuctionOptions() };

        std::vector<Models::AuctionOption> polishOptions;
        for (Models::AuctionOption& option : options)
        {
            if (option.m_groupName.contains(Core::APPConstants::POLISH_OPTION_KEYWORD)) polishOptions.push_back(std::move(option));
        }
        return polishOptions.empty() ? options : polishOptions;
    }

    // 유효옵션 순서대로 이름이 같은 옵션을 찾아 담음 (API 이름이 바뀌어 하나도 못 찾으면 고를 수 있게 전체를 그대로 돌려줌)
    std::vector<Models::AuctionOption> AuctionAccessorySearchService::FilterOptions(const std::vector<Models::AuctionOption>& options, int categoryCode, bool isSupporter) const
    {
        const std::span<const std::string_view> validOptions{ FindValidOptions(categoryCode, isSupporter) };   // 부위 · 역할별 유효옵션
        if (validOptions.empty()) return options;   // 모르는 부위면 전체

        std::vector<Models::AuctionOption> filtered;
        filtered.reserve(validOptions.size());
        for (const std::string_view dealerOption : validOptions)   // 정해 둔 순서대로
        {
            const auto found{ std::ranges::find_if(options, [dealerOption](const Models::AuctionOption& option)
            {
                return RemoveSpaces(option.m_name) == dealerOption;
            }) };
            if (found == options.end()) continue;

            // 값 고르기는 상 · 중 · 하만
            Models::AuctionOption& option{ filtered.emplace_back(*found) };
            ApplyPolishTiers(option, dealerOption);
        }
        return filtered.empty() ? options : filtered;
    }

    Models::AccessorySearchPage AuctionAccessorySearchService::Search(Core::MarketAPI& api, const Models::AccessorySearchQuery& query) const
    {
        return api.SearchAccessories(query);
    }
}
