module LostArkMultiTool.Models.UserData;

import std;

namespace LostArkMultiTool::Models
{
    namespace
    {
        constexpr std::size_t LEGACY_KEY_LENGTH{ 5 };     // "MM-dd"
        constexpr std::size_t HISTORY_KEY_LENGTH{ 10 };   // "yyyy-MM-dd"
        constexpr std::size_t HISTORY_YEAR_LENGTH{ 4 };   // "yyyy"
        constexpr std::size_t HISTORY_MONTH_START{ 5 };
        constexpr std::size_t HISTORY_MONTH_END{ 7 };
        constexpr std::size_t HISTORY_DAY_START{ 8 };
        constexpr char HISTORY_SEPARATOR{ '-' };

        // 숫자 문자열 전체를 값으로 바꿈 (남는 글자가 있으면 실패)
        template <typename T>
        bool TryParseDigits(std::string_view text, T& value)
        {
            const char* first{ text.data() };
            const char* last{ text.data() + text.size() };
            const auto [ptr, ec] { std::from_chars(first, last, value) };
            return ec == std::errc{} && ptr == last;
        }

        // "MM-dd" → 날짜 (기준일 이후면 작년으로 간주, 달력에 없는 날짜면 nullopt)
        std::optional<std::chrono::year_month_day> ConvertLegacyKey(std::string_view legacyKey, const std::chrono::year_month_day& today)
        {
            const std::size_t separatorPos{ legacyKey.find(HISTORY_SEPARATOR) };
            if (separatorPos == std::string_view::npos) return std::nullopt;

            unsigned month{};
            unsigned day{};
            if (!TryParseDigits(legacyKey.substr(0, separatorPos), month)) return std::nullopt;
            if (!TryParseDigits(legacyKey.substr(separatorPos + 1), day)) return std::nullopt;

            const std::chrono::month monthValue{ month };
            const std::chrono::day dayValue{ day };
            const bool isFuture{ monthValue > today.month() || (monthValue == today.month() && dayValue > today.day()) };
            const std::chrono::year year{ isFuture ? today.year() - std::chrono::years{ 1 } : today.year() };

            const std::chrono::year_month_day candidate{ year, monthValue, dayValue };
            if (!candidate.ok()) return std::nullopt;
            return candidate;
        }
    }

    const std::unordered_map<std::string, Character>& UserData::GetCharacters() const noexcept
    {
        return m_characters;
    }

    void UserData::SetCharacters(std::unordered_map<std::string, Character> characters)
    {
        m_characters = std::move(characters);
    }

    Character* UserData::FindCharacter(std::string_view name)
    {
        const auto found{ m_characters.find(std::string{ name }) };
        return found != m_characters.end() ? &found->second : nullptr;
    }

    const Character* UserData::FindCharacter(std::string_view name) const
    {
        const auto found{ m_characters.find(std::string{ name }) };
        return found != m_characters.end() ? &found->second : nullptr;
    }

    const std::map<std::string, double>& UserData::GetHistory() const noexcept
    {
        return m_history;
    }

    void UserData::SetHistory(std::map<std::string, double> history)
    {
        m_history = std::move(history);
    }

    void UserData::RecordHistory(const std::string& dateKey, double totalAsset)
    {
        m_history.insert_or_assign(dateKey, totalAsset);
    }

    void UserData::MigrateLegacyHistory(const std::chrono::year_month_day& today)
    {
        // 옛 형식 키만 먼저 모음 (맵을 돌면서 지우면 순회가 꼬이므로)
        std::vector<std::string> legacyKeys;
        for (const auto& entry : m_history)
        {
            if (entry.first.size() == LEGACY_KEY_LENGTH) legacyKeys.push_back(entry.first);
        }

        for (const std::string& oldKey : legacyKeys)
        {
            const double value{ m_history.at(oldKey) };
            m_history.erase(oldKey);   // 옛 키는 변환 성공 여부와 관계없이 항상 제거 (원본과 동일)

            if (const auto migrated{ ConvertLegacyKey(oldKey, today) }; migrated)
            {
                m_history.try_emplace(MakeHistoryKey(*migrated), value);   // 같은 날짜 키가 이미 있으면 기존 값 유지
            }
        }
    }

    std::string UserData::MakeHistoryKey(const std::chrono::year_month_day& date)
    {
        return std::format("{:04}-{:02}-{:02}",
            static_cast<int>(date.year()),
            static_cast<unsigned>(date.month()),
            static_cast<unsigned>(date.day()));
    }

    std::optional<std::chrono::year_month_day> UserData::ParseHistoryKey(std::string_view key)
    {
        if (key.size() != HISTORY_KEY_LENGTH) return std::nullopt;
        if (key[HISTORY_YEAR_LENGTH] != HISTORY_SEPARATOR || key[HISTORY_MONTH_END] != HISTORY_SEPARATOR) return std::nullopt;

        int year{};
        unsigned month{};
        unsigned day{};
        if (!TryParseDigits(key.substr(0, HISTORY_YEAR_LENGTH), year)) return std::nullopt;
        if (!TryParseDigits(key.substr(HISTORY_MONTH_START, HISTORY_MONTH_END - HISTORY_MONTH_START), month)) return std::nullopt;
        if (!TryParseDigits(key.substr(HISTORY_DAY_START), day)) return std::nullopt;

        const std::chrono::year_month_day date{ std::chrono::year{ year }, std::chrono::month{ month }, std::chrono::day{ day } };
        if (!date.ok()) return std::nullopt;
        return date;
    }
}