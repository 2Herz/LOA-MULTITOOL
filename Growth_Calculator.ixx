export module LostArkMultiTool.Core.GrowthCalculator;

import std;
import LostArkMultiTool.Models.WeeklyRecord;

export namespace LostArkMultiTool::Core
{
    // 성장 통계 계산 인터페이스 (표 · 그래프가 같은 계산식을 공유, 원본 IGrowthCalculator)
    // 그 주차에 기록된 값(0보다 큰 값)만으로 계산 → 원정대가 바뀌어도 지난 주차 숫자가 그대로
    class GrowthCalculator
    {
    public:
        virtual ~GrowthCalculator();

        [[nodiscard]] virtual double GetTotal(const Models::WeeklyRecord& record) const = 0;                 // 주차 총합
        [[nodiscard]] virtual double GetAverage(const Models::WeeklyRecord& record) const = 0;               // 주차 평균 (기록된 캐릭터 수 기준)
        [[nodiscard]] virtual double GetChangeRate(double currentTotal, double baseTotal) const noexcept = 0; // 증감률(%) (기준이 0 이하면 0)
    };
}
