export module LostArkMultiTool.Models.WeeklyRecord;

import std;

export namespace LostArkMultiTool::Models
{
    // 주차 하나의 기록 (캐릭터 이름 → 낙원력, 원본 WeeklyRecord)
    // 0 이하 값은 "미기록" → 저장하지 않고 평균의 나눗수에서도 빠짐
    class WeeklyRecord
    {
    public:
        explicit WeeklyRecord(int weekNumber = 0);

        [[nodiscard]] int GetWeekNumber() const noexcept;
        void SetWeekNumber(int weekNumber) noexcept;

        // 캐릭터 이름 → 낙원력 (기록된 캐릭터만, 정렬 맵이라 저장 순서가 고정됨)
        [[nodiscard]] const std::map<std::string, double>& GetScores() const noexcept;
        [[nodiscard]] std::map<std::string, double>& GetScores() noexcept;

        // 캐릭터 낙원력 (기록이 없으면 0)
        [[nodiscard]] double GetScore(const std::string& characterName) const;

        // 0보다 크면 기록, 0 이하면 기록 지움 (미기록)
        void SetScore(const std::string& characterName, double score);

    private:
        int m_weekNumber{};
        std::map<std::string, double> m_scores;
    };
}
