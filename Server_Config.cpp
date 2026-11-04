module LostArkMultiTool.Models.ServerConfig;

import std;

namespace LostArkMultiTool::Models
{
    ServerConfig::ServerConfig(bool isMaintenance,
        std::optional<std::string> maintenanceMessage,
        std::optional<std::string> latestVersion,
        std::optional<std::string> downloadURL,
        std::optional<std::string> notice)
        : m_isMaintenance{ isMaintenance }
        , m_maintenanceMessage{ std::move(maintenanceMessage) }
        , m_latestVersion{ std::move(latestVersion) }
        , m_downloadURL{ std::move(downloadURL) }
        , m_notice{ std::move(notice) }
    {}

    bool ServerConfig::IsMaintenance() const noexcept
    {
        return m_isMaintenance;
    }
    const std::optional<std::string>& ServerConfig::GetMaintenanceMessage() const noexcept
    {
        return m_maintenanceMessage;
    }
    const std::optional<std::string>& ServerConfig::GetLatestVersion() const noexcept
    {
        return m_latestVersion;
    }
    const std::optional<std::string>& ServerConfig::GetDownloadURL() const noexcept
    {
        return m_downloadURL;
    }
    const std::optional<std::string>& ServerConfig::GetNotice() const noexcept
    {
        return m_notice;
    }
}