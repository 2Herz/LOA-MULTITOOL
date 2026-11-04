module LostArkMultiTool.Models.ItemData;

namespace LostArkMultiTool::Models
{
    ItemData::ItemData() noexcept
    {}

    double ItemData::GetTopQty() const noexcept
    {
        return m_topQty;
    }
    void ItemData::SetTopQty(double topQty) noexcept
    {
        m_topQty = topQty;
    }

    double ItemData::GetBottomQty() const noexcept
    {
        return m_bottomQty;
    }
    void ItemData::SetBottomQty(double bottomQty) noexcept
    {
        m_bottomQty = bottomQty;
    }

    double ItemData::GetPreviousTopQty() const noexcept
    {
        return m_previousTopQty;
    }
    void ItemData::SetPreviousTopQty(double previousTopQty) noexcept
    {
        m_previousTopQty = previousTopQty;
    }

    double ItemData::GetPreviousBottomQty() const noexcept
    {
        return m_previousBottomQty;
    }
    void ItemData::SetPreviousBottomQty(double previousBottomQty) noexcept
    {
        m_previousBottomQty = previousBottomQty;
    }

    double ItemData::GetTopQtyChange() const noexcept
    {
        return m_topQty - m_previousTopQty;
    }
    double ItemData::GetBottomQtyChange() const noexcept
    {
        return m_bottomQty - m_previousBottomQty;
    }

    double ItemData::GetCalculatedTopValue() const noexcept
    {
        return m_calculatedTopValue;
    }
    void ItemData::SetCalculatedTopValue(double calculatedTopValue) noexcept
    {
        m_calculatedTopValue = calculatedTopValue;
    }

    double ItemData::GetCalculatedBotValue() const noexcept
    {
        return m_calculatedBotValue;
    }
    void ItemData::SetCalculatedBotValue(double calculatedBotValue) noexcept
    {
        m_calculatedBotValue = calculatedBotValue;
    }
}