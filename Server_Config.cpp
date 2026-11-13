module LostArkMultiTool.Models.ServerConfig;

import std;

namespace LostArkMultiTool::Models
{
    ServerConfig::ServerConfig(bool isMaintenance,
        std::optional<std::string> maintenanceMessage,
        std::optional<std::string> latestVersion,
        std::optional<std::string> downloadURL,
        std::optional<std::string> notice,
        std::optional<std::string> packageURL,
        std::optional<std::string> packageSHA256)
        : m_isMaintenance{ isMaintenance }
        , m_maintenanceMessage{ std::move(maintenanceMessage) }
        , m_latestVersion{ std::move(latestVersion) }
        , m_downloadURL{ std::move(downloadURL) }
        , m_notice{ std::move(notice) }
        , m_packageURL{ std::move(packageURL) }         // 자동 업데이트 zip 주소
        , m_packageSHA256{ std::move(packageSHA256) }   // zip 검증용 해시
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
    const std::optional<std::string>& ServerConfig::GetPackageURL() const noexcept
    {
        return m_packageURL;      // 자동 업데이트 zip 주소
    }
    const std::optional<std::string>& ServerConfig::GetPackageSHA256() const noexcept
    {
        return m_packageSHA256;   // zip 검증용 해시
    }
}
