module LostArkMultiTool.Services.GrowthCalculationService;

import std;
import LostArkMultiTool.Models.WeeklyRecord;

namespace LostArkMultiTool::Services
{
    namespace
    {
        constexpr double PERCENT{ 100.0 };            // 비율 → 퍼센트 변환
        constexpr double UNRECORDED_SCORE{ 0.0 };     // 0 · 빈칸 = 미기록 (나눗수에서 제외)
        constexpr double NO_CHANGE_RATE{ 0.0 };

        // 0보다 큰 기록만 (저장할 때 이미 걸러지지만 손으로 고친 파일도 같은 결과가 나오게 한 번 더)
        [[nodiscard]] auto GetRecordedScores(const Models::WeeklyRecord& record)
        {
            return record.GetScores() | std::views::values | std::views::filter([](double score) { return score > UNRECORDED_SCORE; });
        }
    }

    // 주차 총합 = 그 주차에 기록된 값의 합
    double GrowthCalculationService::GetTotal(const Models::WeeklyRecord& record) const
    {
        double total{};
        for (const double score : GetRecordedScores(record)) total += score;
        return total;
    }

    // 주차 평균 = 총합 / 기록된 캐릭터 수 (기록이 없으면 0)
    double GrowthCalculationService::GetAverage(const Models::WeeklyRecord& record) const
    {
        double total{};
        std::size_t count{};
        for (const double score : GetRecordedScores(record))
        {
            total += score;
            ++count;
        }
        return count > 0 ? total / static_cast<double>(count) : UNRECORDED_SCORE;
    }

    // 증감률(%) = (현재 - 기준) / 기준 × 100 (기준이 0 이하면 0)
    double GrowthCalculationService::GetChangeRate(double currentTotal, double baseTotal) const noexcept
    {
        return baseTotal > UNRECORDED_SCORE ? (currentTotal - baseTotal) / baseTotal * PERCENT : NO_CHANGE_RATE;
    }
}
