module LostArkMultiTool.Models.PriceAlarm;

import std;
import LostArkMultiTool.Models.GemCatalog;

namespace LostArkMultiTool::Models
{
    PriceAlarm::PriceAlarm(GemSelection gem, std::int64_t targetPrice)
        : m_gem{ std::move(gem) }
        , m_targetPrice{ targetPrice }
    {}

    const GemSelection& PriceAlarm::GetGem() const noexcept
    {
        return m_gem;
    }

    std::string PriceAlarm::GetGemKey() const
    {
        return GemCatalog::MakeKey(m_gem);
    }

    std::string PriceAlarm::GetDisplayName() const
    {
        return GemCatalog::MakeDisplayName(m_gem);
    }

    std::int64_t PriceAlarm::GetTargetPrice() const noexcept
    {
        return m_targetPrice;
    }

    void PriceAlarm::SetTargetPrice(std::int64_t targetPrice) noexcept
    {
        m_targetPrice = targetPrice;
    }

    bool PriceAlarm::IsTriggered() const noexcept
    {
        return m_isTriggered;
    }

    void PriceAlarm::SetTriggered(bool isTriggered) noexcept
    {
        m_isTriggered = isTriggered;
    }

    bool PriceAlarm::IsNotified() const noexcept
    {
        return m_isNotified;
    }

    void PriceAlarm::SetNotified(bool isNotified) noexcept
    {
        m_isNotified = isNotified;
    }
}