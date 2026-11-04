module LostArkMultiTool.Models.GemData;

import std;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.PriceAlarm;
import LostArkMultiTool.Models.RefreshSettings;

namespace LostArkMultiTool::Models
{
    std::vector<PriceAlarm>& GemData::GetAlarms() noexcept
    {
        return m_alarms;
    }

    const std::vector<PriceAlarm>& GemData::GetAlarms() const noexcept
    {
        return m_alarms;
    }

    std::map<std::string, std::int64_t>& GemData::GetLastPrices() noexcept
    {
        return m_lastPrices;
    }

    const std::map<std::string, std::int64_t>& GemData::GetLastPrices() const noexcept
    {
        return m_lastPrices;
    }

    std::map<std::string, std::map<std::string, std::int64_t>>& GemData::GetHistory() noexcept
    {
        return m_history;
    }

    const std::map<std::string, std::map<std::string, std::int64_t>>& GemData::GetHistory() const noexcept
    {
        return m_history;
    }

    const RefreshSettings& GemData::GetSettings() const noexcept
    {
        return m_settings;
    }

    void GemData::SetSettings(const RefreshSettings& settings) noexcept
    {
        m_settings = settings;
    }

    void GemData::Normalize()
    {
        m_settings.Normalize();

        // 이름이 빠진 잘못된 알림 제거
        std::erase_if(m_alarms, [](const PriceAlarm& alarm) { return alarm.GetGem().m_typeName.empty(); });

        // 같은 보석에 알림이 여러 개면 첫 번째만 남김 (보석 하나당 알림 하나 규칙)
        std::set<std::string> seenKeys;
        std::erase_if(m_alarms, [&seenKeys](const PriceAlarm& alarm) { return !seenKeys.insert(alarm.GetGemKey()).second; });
    }

    bool GemData::Prune(std::string_view oldestKeptKey)
    {
        bool isChanged{};

        // 조회 대상이 아닌 보석의 기록은 통째로, 남은 보석은 보관 기간이 지난 날짜만 지움
        isChanged |= std::erase_if(m_history, [](const auto& entry) { return !GemCatalog::IsTrackedKey(entry.first); }) > 0;
        for (auto& entry : m_history)
        {
            isChanged |= std::erase_if(entry.second, [oldestKeptKey](const auto& day) { return day.first < oldestKeptKey; }) > 0;
        }

        isChanged |= std::erase_if(m_lastPrices, [](const auto& entry) { return !GemCatalog::IsTrackedKey(entry.first); }) > 0;
        return isChanged;
    }
}