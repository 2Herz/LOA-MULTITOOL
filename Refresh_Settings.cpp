module LostArkMultiTool.Models.RefreshSettings;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Models
{
    RefreshSettings::RefreshSettings() noexcept
        : m_watchIntervalSeconds{ Core::APPConstants::DEFAULT_WATCH_INTERVAL_SECONDS }
        , m_fullIntervalMinutes{ Core::APPConstants::DEFAULT_FULL_INTERVAL_MINUTES }
    {}

    int RefreshSettings::GetWatchIntervalSeconds() const noexcept
    {
        return m_watchIntervalSeconds;
    }

    void RefreshSettings::SetWatchIntervalSeconds(int seconds) noexcept
    {
        m_watchIntervalSeconds = seconds;
    }

    int RefreshSettings::GetFullIntervalMinutes() const noexcept
    {
        return m_fullIntervalMinutes;
    }

    void RefreshSettings::SetFullIntervalMinutes(int minutes) noexcept
    {
        m_fullIntervalMinutes = minutes;
    }

    bool RefreshSettings::IsAutoRefreshEnabled() const noexcept
    {
        return m_isAutoRefreshEnabled;
    }

    void RefreshSettings::SetAutoRefreshEnabled(bool isEnabled) noexcept
    {
        m_isAutoRefreshEnabled = isEnabled;
    }

    void RefreshSettings::Normalize() noexcept
    {
        const auto& watchOptions{ Core::APPConstants::WATCH_INTERVAL_SECONDS_OPTIONS };
        if (std::find(watchOptions.begin(), watchOptions.end(), m_watchIntervalSeconds) == watchOptions.end())
        {
            m_watchIntervalSeconds = Core::APPConstants::DEFAULT_WATCH_INTERVAL_SECONDS;
        }

        const auto& fullOptions{ Core::APPConstants::FULL_INTERVAL_MINUTES_OPTIONS };
        if (std::find(fullOptions.begin(), fullOptions.end(), m_fullIntervalMinutes) == fullOptions.end())
        {
            m_fullIntervalMinutes = Core::APPConstants::DEFAULT_FULL_INTERVAL_MINUTES;
        }
    }
}