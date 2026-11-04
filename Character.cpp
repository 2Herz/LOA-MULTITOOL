module LostArkMultiTool.Models.Character;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Models
{
    Character::Character()
    {
        EnsureMaterials();
    }

    const std::optional<std::string>& Character::GetClassName() const noexcept
    {
        return m_className;
    }
    void Character::SetClassName(std::optional<std::string> className)
    {
        m_className = std::move(className);
    }

    double Character::GetItemLevel() const noexcept
    {
        return m_itemLevel;
    }
    void Character::SetItemLevel(double itemLevel) noexcept
    {
        m_itemLevel = itemLevel;
    }

    const std::unordered_map<std::string, ItemData>& Character::GetMaterials() const noexcept
    {
        return m_materials;
    }

    void Character::SetMaterials(std::unordered_map<std::string, ItemData> materials)
    {
        m_materials = std::move(materials);
        EnsureMaterials();   // 빠진 키가 있으면 채워서 항상 전체 키를 유지
    }

    ItemData& Character::GetMaterial(std::string_view key)
    {
        return m_materials.at(std::string{ key });
    }

    const ItemData& Character::GetMaterial(std::string_view key) const
    {
        return m_materials.at(std::string{ key });
    }

    void Character::EnsureMaterials()
    {
        for (const std::string_view key : Core::APPConstants::MAT_KEYS)
        {
            if (key.empty()) continue;   // 0번 열(구분)은 재료가 아님
            m_materials.try_emplace(std::string{ key });
        }
    }
}