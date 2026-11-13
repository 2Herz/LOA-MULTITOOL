export module LostArkMultiTool.Views.PriceHistoryChart;

import std;
import LostArkMultiTool.Views.UI.LineChart;

export namespace LostArkMultiTool::Views
{
    // 일별 최저가 기록 → 그래프 점 목록 (x축 글자 = "MM/dd", 유각 · 젬 시세 검색기 공용)
    [[nodiscard]] std::vector<UI::ChartPoint> BuildPriceChartPoints(const std::map<std::string, std::int64_t>& days);
}
