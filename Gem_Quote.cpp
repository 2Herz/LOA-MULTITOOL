module LostArkMultiTool.Models.GemQuote;

import std;
import LostArkMultiTool.Models.GemCatalog;

namespace LostArkMultiTool::Models
{
    GemQuote::GemQuote(GemSelection gem)
        : m_gem{ std::move(gem) }
    {}

    const GemSelection& GemQuote::GetGem() const noexcept
    {
        return m_gem;
    }

    const std::optional<std::int64_t>& GemQuote::GetPrice() const noexcept
    {
        return m_price;
    }

    void GemQuote::SetPrice(std::optional<std::int64_t> price) noexcept
    {
        m_price = price;
    }

    const std::optional<std::int64_t>& GemQuote::GetPreviousPrice() const noexcept
    {
        return m_previousPrice;
    }

    void GemQuote::SetPreviousPrice(std::optional<std::int64_t> previousPrice) noexcept
    {
        m_previousPrice = previousPrice;
    }

    const std::optional<std::int64_t>& GemQuote::GetLastFullPrice() const noexcept
    {
        return m_lastFullPrice;
    }

    void GemQuote::SetLastFullPrice(std::optional<std::int64_t> lastFullPrice) noexcept
    {
        m_lastFullPrice = lastFullPrice;
    }

    bool GemQuote::IsQueried() const noexcept
    {
        return m_isQueried;
    }

    void GemQuote::SetQueried(bool isQueried) noexcept
    {
        m_isQueried = isQueried;
    }

    std::optional<std::int64_t> GemQuote::GetDiff() const noexcept
    {
        if (!m_price.has_value() || !m_previousPrice.has_value()) return std::nullopt;
        return *m_price - *m_previousPrice;
    }
}