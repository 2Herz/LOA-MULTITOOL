export module LostArkMultiTool.Services.GrowthCalculationService;

import std;
import LostArkMultiTool.Core.GrowthCalculator;
import LostArkMultiTool.Models.WeeklyRecord;

export namespace LostArkMultiTool::Services
{
    // 성장 통계 계산 (원본 GrowthCalculator)
    // 그 주차에 기록된 값만 계산 → 원정대가 바뀌어도 지난 주차 숫자가 바뀌지 않음
    // (원정대에서 빠진 캐릭터의 기록은 표에 "원정대 제외" 열로 보여서, 보이는 칸과 통계가 항상 일치)
    class GrowthCalculationService final : public Core::GrowthCalculator
    {
    public:
        [[nodiscard]] double GetTotal(const Models::WeeklyRecord& record) const override;
        [[nodiscard]] double GetAverage(const Models::WeeklyRecord& record) const override;
        [[nodiscard]] double GetChangeRate(double currentTotal, double baseTotal) const noexcept override;
    };
}
