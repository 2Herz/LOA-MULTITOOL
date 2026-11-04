export module LostArkMultiTool.Models.CharacterInfo;

import std;

export namespace LostArkMultiTool::Models
{
    // 원정대 API 응답 한 줄 (읽기 전용 값 객체)
    class CharacterInfo
    {
    public:
        CharacterInfo(std::string characterName,
            std::optional<std::string> characterClassName,
            std::optional<std::string> itemMaxLevel,
            std::optional<std::string> itemAvgLevel);

        [[nodiscard]] const std::string& GetCharacterName() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetCharacterClassName() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetItemMaxLevel() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetItemAvgLevel() const noexcept;

        // "1,740.00" 같은 문자열을 숫자로 바꿈 (실패하면 0)
        [[nodiscard]] double GetNumericItemLevel() const;

    private:
        std::string m_characterName;
        std::optional<std::string> m_characterClassName;
        std::optional<std::string> m_itemMaxLevel;
        std::optional<std::string> m_itemAvgLevel;
    };
}