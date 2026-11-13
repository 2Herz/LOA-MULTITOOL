module LostArkMultiTool.Views.PriceHistoryChart;

import std;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Views.UI.LineChart;

namespace LostArkMultiTool::Views
{
    namespace
    {
        // ===== 날짜 키 "yyyy-MM-dd" → 그래프 글자 "MM/dd" =====
        constexpr std::size_t DATE_KEY_LENGTH{ 10 };         // "yyyy-MM-dd" 길이
        constexpr std::size_t DATE_KEY_MONTH_POSITION{ 5 };  // 월 시작 위치
        constexpr std::size_t DATE_KEY_DAY_POSITION{ 8 };    // 일 시작 위치
        constexpr std::size_t DATE_PART_LENGTH{ 2 };         // 월 · 일 글자 수
        constexpr wchar_t CHART_DATE_SEPARATOR{ L'/' };      // 월/일 구분 글자

        // 저장 키 → 그래프 글자 (형식이 다르면 그대로)
        [[nodiscard]] std::wstring ToChartDateLabel(const std::string& dateKey)
        {
            const std::wstring wideKey{ Core::TextEncoding::UTF8ToWide(dateKey) };   // UTF-8 → 화면 글자
            if (wideKey.size() != DATE_KEY_LENGTH) return wideKey;   // 형식이 다르면 그대로 표시

            std::wstring label{ wideKey.substr(DATE_KEY_MONTH_POSITION, DATE_PART_LENGTH) };   // 월
            label.push_back(CHART_DATE_SEPARATOR);                                              // 구분 글자
            label += wideKey.substr(DATE_KEY_DAY_POSITION, DATE_PART_LENGTH);                   // 일
            return label;
        }
    }

    std::vector<UI::ChartPoint> BuildPriceChartPoints(const std::map<std::string, std::int64_t>& days)
    {
        std::vector<UI::ChartPoint> points;   // 날짜순 점 목록
        points.reserve(days.size());          // 미리 자리 확보
        for (const auto& [dateKey, price] : days) points.push_back(UI::ChartPoint{ ToChartDateLabel(dateKey), static_cast<double>(price) });
        return points;
    }
}
