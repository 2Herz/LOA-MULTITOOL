export module LostArkMultiTool.Models.BoundGemRosterData;

import std;
import LostArkMultiTool.Models.BoundGemData;
import LostArkMultiTool.Models.CharacterInfo;

export namespace LostArkMultiTool::Models
{
    // 귀속 젬 계산기 캐릭터 한 명 (원정대 동기화로 받은 이름 · 직업 · 레벨 + 보유 수량)
    struct BoundGemCharacter
    {
        std::string m_name;                        // 캐릭터 이름 (UTF-8, 저장 키)
        std::optional<std::string> m_className;    // 직업 (없을 수 있음)
        double m_itemLevel{};                      // 아이템 레벨 (목록 정렬용)
        BoundGemData m_quantities;                 // 젬 6종 × 등급 3단계 보유 수량
    };

    // 원정대 전체의 귀속 젬 보유 수량 (캐릭터별 수량 + 원정대 합계, 귀속 젬 계산기의 저장 데이터)
    // 캐릭터 목록은 아이템 레벨이 높은 순서 (레벨이 같으면 이름순)
    class BoundGemRosterData
    {
    public:
        BoundGemRosterData() noexcept;

        [[nodiscard]] const std::vector<BoundGemCharacter>& GetCharacters() const noexcept;
        [[nodiscard]] BoundGemCharacter* FindCharacter(std::string_view name) noexcept;               // 없으면 nullptr
        [[nodiscard]] const BoundGemCharacter* FindCharacter(std::string_view name) const noexcept;

        // 저장 파일에서 읽은 캐릭터 추가 (같은 이름이 있으면 덮어씀)
        void AddCharacter(BoundGemCharacter character);

        // 원정대 동기화 결과로 캐릭터 목록 교체 (기존 캐릭터는 수량 유지, 원정대에서 빠진 캐릭터는 제거)
        // 캐릭터별 저장 이전의 수량(원정대 공용)이 남아 있으면 레벨이 가장 높은 캐릭터에게 옮기고 true
        [[nodiscard]] bool SetRoster(const std::vector<CharacterInfo>& roster);

        // 캐릭터별 저장 이전 파일의 수량 (원정대 동기화 전까지 원정대 합계에만 포함)
        [[nodiscard]] BoundGemData& GetUnassigned() noexcept;
        [[nodiscard]] const BoundGemData& GetUnassigned() const noexcept;
        [[nodiscard]] bool HasUnassigned() const;

        // 모든 캐릭터 + 공용 수량의 합계
        [[nodiscard]] BoundGemData GetTotal() const;

        // 날짜별 원정대 귀속 젬 총 가치 ("yyyy-MM-dd" → 골드, 그래프용, 하루에 하나 · 같은 날은 덮어씀)
        [[nodiscard]] const std::map<std::string, double>& GetHistory() const noexcept;
        void RecordHistory(const std::string& dateKey, double totalValue);   // 오래된 기록은 보관 기간만큼만 남김

    private:
        void SortCharacters();

        std::vector<BoundGemCharacter> m_characters;
        BoundGemData m_unassigned;
        std::map<std::string, double> m_history;
    };
}
