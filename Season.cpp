module LostArkMultiTool.Models.Season;

import std;
import LostArkMultiTool.Models.WeeklyRecord;

namespace LostArkMultiTool::Models
{
    namespace
    {
        constexpr int NEXT_NUMBER_STEP{ 1 };   // 주차 번호는 1부터
    }

    Season::Season(int seasonNumber)
        : m_seasonNumber{ seasonNumber }
    {}

    int Season::GetSeasonNumber() const noexcept
    {
        return m_seasonNumber;
    }

    void Season::SetSeasonNumber(int seasonNumber) noexcept
    {
        m_seasonNumber = seasonNumber;
    }

    const std::vector<WeeklyRecord>& Season::GetRecords() const noexcept
    {
        return m_records;
    }

    std::vector<WeeklyRecord>& Season::GetRecords() noexcept
    {
        return m_records;
    }

    void Season::SetRecords(std::vector<WeeklyRecord> records)
    {
        m_records = std::move(records);
    }

    void Season::AddWeek()
    {
        m_records.emplace_back(static_cast<int>(m_records.size()) + NEXT_NUMBER_STEP);
    }

    std::string Season::GetDisplayName() const
    {
        return std::format("시즌 {}", m_seasonNumber);
    }

    bool Season::HasPeriod() const noexcept
    {
        return m_startDate.has_value() && m_endDate.has_value();
    }

    const std::optional<std::chrono::year_month_day>& Season::GetStartDate() const noexcept
    {
        return m_startDate;
    }

    const std::optional<std::chrono::year_month_day>& Season::GetEndDate() const noexcept
    {
        return m_endDate;
    }

    void Season::SetPeriod(const std::chrono::year_month_day& startDate, const std::chrono::year_month_day& endDate)
    {
        m_startDate = startDate;
        m_endDate = std::chrono::sys_days{ endDate } < std::chrono::sys_days{ startDate } ? startDate : endDate;
    }

    void Season::ClearPeriod() noexcept
    {
        m_startDate.reset();
        m_endDate.reset();
    }

    // (종료일 - 시작일) ÷ 7 + 1 (예: 시작 + 6일 = 1주, 시작 + 13일 = 2주)
    std::size_t Season::GetPeriodWeekCount() const noexcept
    {
        if (!HasPeriod()) return 0;
        const auto days{ (std::chrono::sys_days{ *m_endDate } - std::chrono::sys_days{ *m_startDate }).count() };
        return static_cast<std::size_t>(days / DAYS_PER_WEEK) + 1;
    }

    std::optional<std::size_t> Season::FindWeekIndex(const std::chrono::year_month_day& gameDay) const noexcept
    {
        if (!HasPeriod()) return std::nullopt;

        const auto days{ (std::chrono::sys_days{ gameDay } - std::chrono::sys_days{ *m_startDate }).count() };
        if (days < 0) return std::nullopt;   // 시즌 시작 전

        const std::size_t weekIndex{ static_cast<std::size_t>(days / DAYS_PER_WEEK) };
        if (weekIndex >= GetPeriodWeekCount()) return std::nullopt;   // 시즌 마지막 주 마감 뒤
        return weekIndex;
    }

    std::optional<std::chrono::year_month_day> Season::GetWeekStartDate(std::size_t weekIndex) const noexcept
    {
        if (!m_startDate.has_value()) return std::nullopt;
        return std::chrono::year_month_day{ std::chrono::sys_days{ *m_startDate } + std::chrono::days{ static_cast<int>(weekIndex) * DAYS_PER_WEEK } };
    }
}
