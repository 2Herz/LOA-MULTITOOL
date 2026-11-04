module LostArkMultiTool.Models.PriceData;

namespace LostArkMultiTool::Models
{
    PriceData::PriceData() noexcept
    {}

    double PriceData::GetCurrent() const noexcept
    {
        return m_current;
    }
    void PriceData::SetCurrent(double current) noexcept
    {
        m_current = current;
    }

    double PriceData::GetPrevious() const noexcept
    {
        return m_previous;
    }
    void PriceData::SetPrevious(double previous) noexcept
    {
        m_previous = previous;
    }

    double PriceData::GetDiff() const noexcept
    {
        return m_current - m_previous;
    }
}