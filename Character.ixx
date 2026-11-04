export module LostArkMultiTool.Models.Character;

import std;
import LostArkMultiTool.Models.ItemData;

export namespace LostArkMultiTool::Models
{
    // 캐릭터 한 명의 정보 + 재료 목록
    class Character
    {
    public:
        Character();

        [[nodiscard]] const std::optional<std::string>& GetClassName() const noexcept;
        void SetClassName(std::optional<std::string> className);

        [[nodiscard]] double GetItemLevel() const noexcept;
        void SetItemLevel(double itemLevel) noexcept;

        [[nodiscard]] const std::unordered_map<std::string, ItemData>& GetMaterials() const noexcept;
        void SetMaterials(std::unordered_map<std::string, ItemData> materials);

        [[nodiscard]] ItemData& GetMaterial(std::string_view key);
        [[nodiscard]] const ItemData& GetMaterial(std::string_view key) const;

        // 누락된 재료 키를 빈 ItemData로 채움 (생성·불러오기 후 호출)
        void EnsureMaterials();

    private:
        std::optional<std::string> m_className;   // 직업명 (없을 수 있음)
        double m_itemLevel{};
        std::unordered_map<std::string, ItemData> m_materials;
    };
}