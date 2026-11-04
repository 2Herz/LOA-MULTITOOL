export module LostArkMultiTool.Models.Season;

import std;
import LostArkMultiTool.Models.WeeklyRecord;

export namespace LostArkMultiTool::Models
{
    // 시즌 하나 (주차 기록 묶음, 주차는 시즌마다 1부터, 원본 Season)
    class Season
    {
    public:
        explicit Season(int seasonNumber = 0);

        [[nodiscard]] int GetSeasonNumber() const noexcept;
        void SetSeasonNumber(int seasonNumber) noexcept;

        [[nodiscard]] const std::vector<WeeklyRecord>& GetRecords() const noexcept;
        [[nodiscard]] std::vector<WeeklyRecord>& GetRecords() noexcept;
        void SetRecords(std::vector<WeeklyRecord> records);

        // 다음 주차 추가 (주차 번호 = 지금 주차 수 + 1, 원본 Button2_Click)
        void AddWeek();

        // 시즌 선택 목록 · 그래프 제목에 표시되는 이름 "시즌 4" (UTF-8)
        [[nodiscard]] std::string GetDisplayName() const;

        // ===== 시즌 기간 (시즌 5부터 입력, 기간이 있는 시즌만 주차 자동 추가 · 지난 주차 입력 잠금) =====
        // 한 주 = 수요일 ~ 화요일 (주차 N = 시작일 + 7(N-1) ~ +6일, 입력 마감 = 다음 주 수요일 오전 6시 점검 전)
        static constexpr int DAYS_PER_WEEK{ 7 };

        [[nodiscard]] bool HasPeriod() const noexcept;
        [[nodiscard]] const std::optional<std::chrono::year_month_day>& GetStartDate() const noexcept;
        [[nodiscard]] const std::optional<std::chrono::year_month_day>& GetEndDate() const noexcept;
        void SetPeriod(const std::chrono::year_month_day& startDate, const std::chrono::year_month_day& endDate);   // 종료일이 앞이면 시작일로 맞춤
        void ClearPeriod() noexcept;

        [[nodiscard]] std::size_t GetPeriodWeekCount() const noexcept;   // 기간 안의 주차 수 (기간이 없으면 0)
        // 로스트아크 기준 날짜(06시에 바뀜)가 속한 주차 번호 (0부터, 기간이 없거나 기간 밖이면 nullopt)
        [[nodiscard]] std::optional<std::size_t> FindWeekIndex(const std::chrono::year_month_day& gameDay) const noexcept;
        // 주차가 시작하는 날짜 (0부터, 기간이 없으면 nullopt) → 다음 주차 시작일 = 이 주차의 입력 마감일
        [[nodiscard]] std::optional<std::chrono::year_month_day> GetWeekStartDate(std::size_t weekIndex) const noexcept;

    private:
        int m_seasonNumber{};
        std::vector<WeeklyRecord> m_records;
        std::optional<std::chrono::year_month_day> m_startDate;
        std::optional<std::chrono::year_month_day> m_endDate;
    };
}
