module LostArkMultiTool.Models.CharacterArmory;

import std;

namespace LostArkMultiTool::Models
{
    namespace
    {
        // 부위 순서(GearPart)와 같은 순서의 API Type 글자 · 계수 표 부위 이름
        constexpr std::array<std::string_view, GEAR_PART_COUNT> PART_TYPES{ "무기", "투구", "어깨", "상의", "하의", "장갑" };
        constexpr std::array<std::string_view, GEAR_PART_COUNT> TABLE_SUFFIXES{ "무기", "머리", "어깨", "상의", "하의", "장갑" };
    }

    std::optional<GearPart> FindGearPart(std::string_view type) noexcept
    {
        const auto found{ std::ranges::find(PART_TYPES, type) };
        if (found == PART_TYPES.end()) return std::nullopt;
        return static_cast<GearPart>(std::distance(PART_TYPES.begin(), found));
    }

    std::string_view GetGearPartType(GearPart part)
    {
        return PART_TYPES.at(ToGearIndex(part));
    }

    std::string_view GetGearTableSuffix(GearPart part)
    {
        return TABLE_SUFFIXES.at(ToGearIndex(part));
    }

    std::size_t ToGearIndex(GearPart part)
    {
        const std::size_t index{ static_cast<std::size_t>(part) };
        if (index >= GEAR_PART_COUNT) throw std::out_of_range{ "장비 부위 번호가 범위를 벗어났습니다." };
        return index;
    }

    GearPart ToGearPart(std::size_t index)
    {
        if (index >= GEAR_PART_COUNT) throw std::out_of_range{ "장비 부위 번호가 범위를 벗어났습니다." };
        return static_cast<GearPart>(index);
    }
}
