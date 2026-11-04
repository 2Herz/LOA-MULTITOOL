export module LostArkMultiTool.Models.GrowthData;

import std;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.Season;
import LostArkMultiTool.Models.WeeklyRecord;

export namespace LostArkMultiTool::Models
{
    // 낙원력 트래커 저장 파일 전체 (원정대 + 시즌별 기록 + 마지막으로 본 시즌, 원본 UserData v2)
    class GrowthData
    {
    public:
        static constexpr int CURRENT_DATA_VERSION{ 2 };   // 1 = 주차 목록만, 2 = 시즌별
        static constexpr int FIRST_SEASON_NUMBER{ 4 };   // 이 프로그램의 시즌 시작점 (시즌 4부터 기록, [시즌 추가] → 시즌 5)

        // 원정대 (아이템 레벨 높은 순, 표의 입력 열 순서)
        [[nodiscard]] const std::vector<CharacterInfo>& GetCharacters() const noexcept;
        void SetCharacters(std::vector<CharacterInfo> characters);

        [[nodiscard]] const std::vector<Season>& GetSeasons() const noexcept;
        [[nodiscard]] std::vector<Season>& GetSeasons() noexcept;
        void SetSeasons(std::vector<Season> seasons);

        [[nodiscard]] int GetSelectedSeasonNumber() const noexcept;
        void SetSelectedSeasonNumber(int seasonNumber) noexcept;

        // 지금 보고 있는 시즌 (선택한 번호가 없으면 마지막 시즌, EnsureValid가 시즌을 1개 이상 보장)
        [[nodiscard]] Season& GetCurrentSeason();
        [[nodiscard]] const Season& GetCurrentSeason() const;

        [[nodiscard]] bool HasSeason(int seasonNumber) const noexcept;
        [[nodiscard]] int GetNextSeasonNumber() const noexcept;   // 가장 큰 시즌 번호 + 1

        // 빈 값 정리 + v1 → v2 변환 (읽은 직후 · 새로 만든 데이터에 호출, 원본 EnsureValid)
        // legacyRecords: v1 파일의 주차 목록 → 시즌이 하나도 없으면 "시즌 1"로 옮김 (숫자 · 주차 번호 그대로)
        void EnsureValid(std::vector<WeeklyRecord> legacyRecords = {});

    private:
        std::vector<CharacterInfo> m_characters;
        std::vector<Season> m_seasons;
        int m_selectedSeasonNumber{ FIRST_SEASON_NUMBER };
    };
}
