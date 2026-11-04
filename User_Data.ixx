export module LostArkMultiTool.Models.UserData;

import std;
import LostArkMultiTool.Models.Character;

export namespace LostArkMultiTool::Models
{
    // 저장 파일 전체 (캐릭터 목록 + 날짜별 원정대 자산 기록)
    class UserData
    {
    public:
        [[nodiscard]] const std::unordered_map<std::string, Character>& GetCharacters() const noexcept;
        void SetCharacters(std::unordered_map<std::string, Character> characters);

        // 이름으로 캐릭터 찾기 (없으면 nullptr, 소유권은 UserData가 가짐)
        [[nodiscard]] Character* FindCharacter(std::string_view name);
        [[nodiscard]] const Character* FindCharacter(std::string_view name) const;

        [[nodiscard]] const std::map<std::string, double>& GetHistory() const noexcept;
        void SetHistory(std::map<std::string, double> history);

        // 날짜 키("yyyy-MM-dd")에 원정대 총 자산 기록 (같은 날짜면 덮어씀)
        void RecordHistory(const std::string& dateKey, double totalAsset);

        // 기존 "MM-dd" 키를 "yyyy-MM-dd"로 변환 (기준일 이후 날짜면 작년으로 간주)
        void MigrateLegacyHistory(const std::chrono::year_month_day& today);

        // 날짜 ↔ 저장 키 변환
        [[nodiscard]] static std::string MakeHistoryKey(const std::chrono::year_month_day& date);
        [[nodiscard]] static std::optional<std::chrono::year_month_day> ParseHistoryKey(std::string_view key);

    private:
        std::unordered_map<std::string, Character> m_characters;
        std::map<std::string, double> m_history;   // 키 순서(날짜순)가 필요해서 정렬 맵 사용
    };
}