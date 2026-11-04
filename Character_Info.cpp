module LostArkMultiTool.Models.CharacterInfo;

import std;

namespace LostArkMultiTool::Models
{
    namespace
    {
        // 숫자로 바꾸기 전에 지우는 글자 (천 단위 콤마, 공백)
        bool IsIgnoredLevelChar(char c)
        {
            return c == ',' || std::isspace(static_cast<unsigned char>(c));
        }
    }

    CharacterInfo::CharacterInfo(std::string characterName,
        std::optional<std::string> characterClassName,
        std::optional<std::string> itemMaxLevel,
        std::optional<std::string> itemAvgLevel)
        : m_characterName{ std::move(characterName) }
        , m_characterClassName{ std::move(characterClassName) }
        , m_itemMaxLevel{ std::move(itemMaxLevel) }
        , m_itemAvgLevel{ std::move(itemAvgLevel) }
    {}

    const std::string& CharacterInfo::GetCharacterName() const noexcept
    {
        return m_characterName;
    }
    const std::optional<std::string>& CharacterInfo::GetCharacterClassName() const noexcept
    {
        return m_characterClassName;
    }
    const std::optional<std::string>& CharacterInfo::GetItemMaxLevel() const noexcept
    {
        return m_itemMaxLevel;
    }
    const std::optional<std::string>& CharacterInfo::GetItemAvgLevel() const noexcept
    {
        return m_itemAvgLevel;
    }

    double CharacterInfo::GetNumericItemLevel() const
    {
        // 원본과 같은 규칙: 최대 레벨 값이 있으면 그것을, 없으면 평균 레벨 사용
        const std::optional<std::string>& source{ m_itemMaxLevel.has_value() ? m_itemMaxLevel : m_itemAvgLevel };
        if (!source.has_value()) return 0.0;

        std::string cleaned;
        for (const char c : *source)
        {
            if (!IsIgnoredLevelChar(c)) cleaned.push_back(c);
        }
        if (cleaned.empty()) return 0.0;

        double level{};
        const char* first{ cleaned.data() };
        const char* last{ cleaned.data() + cleaned.size() };
        const auto [ptr, ec] { std::from_chars(first, last, level) };
        return (ec == std::errc{} && ptr == last) ? level : 0.0;
    }
}