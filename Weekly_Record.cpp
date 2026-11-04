module LostArkMultiTool.Models.WeeklyRecord;

import std;

namespace LostArkMultiTool::Models
{
    namespace
    {
        constexpr double UNRECORDED_SCORE{ 0.0 };   // 0 · 빈칸 = 미기록
    }

    WeeklyRecord::WeeklyRecord(int weekNumber)
        : m_weekNumber{ weekNumber }
    {}

    int WeeklyRecord::GetWeekNumber() const noexcept
    {
        return m_weekNumber;
    }

    void WeeklyRecord::SetWeekNumber(int weekNumber) noexcept
    {
        m_weekNumber = weekNumber;
    }

    const std::map<std::string, double>& WeeklyRecord::GetScores() const noexcept
    {
        return m_scores;
    }

    std::map<std::string, double>& WeeklyRecord::GetScores() noexcept
    {
        return m_scores;
    }

    double WeeklyRecord::GetScore(const std::string& characterName) const
    {
        const auto found{ m_scores.find(characterName) };
        return found != m_scores.end() ? found->second : UNRECORDED_SCORE;
    }

    void WeeklyRecord::SetScore(const std::string& characterName, double score)
    {
        if (score > UNRECORDED_SCORE) m_scores.insert_or_assign(characterName, score);
        else m_scores.erase(characterName);   // 미기록 → 평균 계산에서 빠짐 (원본 Scores.Remove)
    }
}
