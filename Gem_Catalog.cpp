module LostArkMultiTool.Models.GemCatalog;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Models::GemCatalog
{
    namespace
    {
        constexpr char KEY_SEPARATOR{ '_' };
    }

    const GemType* FindType(std::string_view shortName) noexcept
    {
        const auto found{ std::find_if(GEM_TYPES.begin(), GEM_TYPES.end(), [shortName](const GemType& type) { return type.m_shortName == shortName; }) };
        return found != GEM_TYPES.end() ? &*found : nullptr;
    }

    bool IsLevelInRange(int level) noexcept
    {
        return level >= Core::APPConstants::MIN_GEM_LEVEL && level <= Core::APPConstants::MAX_GEM_LEVEL;
    }

    std::string MakeKey(std::string_view typeName, int level)
    {
        return std::format("{}{}{}", typeName, KEY_SEPARATOR, level);
    }

    std::string MakeKey(const GemSelection& gem)
    {
        return MakeKey(gem.m_typeName, gem.m_level);
    }

    std::string MakeItemName(const GemType& type, int level)
    {
        return std::format("{}레벨 {}", level, type.m_itemSuffix);
    }

    std::string MakeDisplayName(const GemSelection& gem)
    {
        return std::format("{}레벨 {}", gem.m_level, gem.m_typeName);
    }

    bool IsTrackedKey(std::string_view gemKey)
    {
        // "종류_레벨"을 나눠서 종류가 있고 레벨이 범위 안인지 확인
        const std::size_t separator{ gemKey.rfind(KEY_SEPARATOR) };
        if (separator == std::string_view::npos) return false;

        int level{};
        const std::string_view levelText{ gemKey.substr(separator + 1) };
        const auto [end, error] { std::from_chars(levelText.data(), levelText.data() + levelText.size(), level) };
        if (error != std::errc{} || end != levelText.data() + levelText.size()) return false;

        return FindType(gemKey.substr(0, separator)) != nullptr && IsLevelInRange(level);
    }

    std::vector<GemSelection> GetAllGems()
    {
        std::vector<GemSelection> gems;
        gems.reserve(TRACKED_GEM_COUNT);
        for (const GemType& type : GEM_TYPES)
        {
            for (int level{ Core::APPConstants::MAX_GEM_LEVEL }; level >= Core::APPConstants::MIN_GEM_LEVEL; --level)
            {
                gems.push_back(GemSelection{ std::string{ type.m_shortName }, level });
            }
        }
        return gems;
    }
}