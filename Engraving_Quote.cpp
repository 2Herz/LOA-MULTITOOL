module LostArkMultiTool.Models.EngravingQuote;

import std;
import LostArkMultiTool.Models.MarketItem;

namespace LostArkMultiTool::Models
{
    namespace
    {
        constexpr double PERCENT_SCALE{ 100.0 };   // 비율 → 퍼센트
    }

    EngravingQuote::EngravingQuote(MarketItem item)
        : m_item{ std::move(item) }
    {}

    const std::string& EngravingQuote::GetName() const noexcept
    {
        return m_item.m_name;
    }

    double EngravingQuote::GetCurrentMinPrice() const noexcept
    {
        return m_item.m_currentMinPrice;
    }

    double EngravingQuote::GetYesterdayAvgPrice() const noexcept
    {
        return m_item.m_yesterdayAvgPrice;
    }

    double EngravingQuote::GetRecentPrice() const noexcept
    {
        return m_item.m_recentPrice;
    }

    const std::optional<double>& EngravingQuote::GetPreviousMinPrice() const noexcept
    {
        return m_previousMinPrice;
    }

    void EngravingQuote::SetPreviousMinPrice(std::optional<double> previousMinPrice) noexcept
    {
        m_previousMinPrice = previousMinPrice;
    }

    // 매물이 없던 쪽(0)이 있으면 비교하지 않음
    std::optional<double> EngravingQuote::GetDiff() const noexcept
    {
        if (!m_previousMinPrice.has_value() || *m_previousMinPrice <= 0.0 || m_item.m_currentMinPrice <= 0.0) return std::nullopt;
        return m_item.m_currentMinPrice - *m_previousMinPrice;
    }

    // (현재 최저가 − 전일 평균가) ÷ 전일 평균가 × 100
    std::optional<double> EngravingQuote::GetDayChangeRate() const noexcept
    {
        if (m_item.m_yesterdayAvgPrice <= 0.0 || m_item.m_currentMinPrice <= 0.0) return std::nullopt;
        return (m_item.m_currentMinPrice - m_item.m_yesterdayAvgPrice) / m_item.m_yesterdayAvgPrice * PERCENT_SCALE;
    }
}
