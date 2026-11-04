export module LostArkMultiTool.Models.ServerConfig;

import std;

export namespace LostArkMultiTool::Models
{
    // 원격 설정 (Gist AppConfig.json) — 읽기 전용 값 객체
    class ServerConfig
    {
    public:
        ServerConfig(bool isMaintenance,
            std::optional<std::string> maintenanceMessage,
            std::optional<std::string> latestVersion,
            std::optional<std::string> downloadURL,
            std::optional<std::string> notice);

        [[nodiscard]] bool IsMaintenance() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetMaintenanceMessage() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetLatestVersion() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetDownloadURL() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetNotice() const noexcept;

    private:
        bool m_isMaintenance{};
        std::optional<std::string> m_maintenanceMessage;
        std::optional<std::string> m_latestVersion;
        std::optional<std::string> m_downloadURL;
        std::optional<std::string> m_notice;
    };
}