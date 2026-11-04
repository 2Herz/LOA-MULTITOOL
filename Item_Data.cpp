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