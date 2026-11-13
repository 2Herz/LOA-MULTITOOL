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
            std::optional<std::string> notice,
            std::optional<std::string> packageURL = std::nullopt,       // 자동 업데이트 zip 주소 (없으면 다운로드 페이지만 안내)
            std::optional<std::string> packageSHA256 = std::nullopt);   // zip의 SHA-256 (소문자 16진수, 받은 파일 검증용)

        [[nodiscard]] bool IsMaintenance() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetMaintenanceMessage() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetLatestVersion() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetDownloadURL() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetNotice() const noexcept;
        [[nodiscard]] const std::optional<std::string>& GetPackageURL() const noexcept;      // 자동 업데이트 zip 주소
        [[nodiscard]] const std::optional<std::string>& GetPackageSHA256() const noexcept;   // zip 검증용 해시

    private:
        bool m_isMaintenance{};
        std::optional<std::string> m_maintenanceMessage;
        std::optional<std::string> m_latestVersion;
        std::optional<std::string> m_downloadURL;
        std::optional<std::string> m_notice;
        std::optional<std::string> m_packageURL;      // 자동 업데이트 zip 주소
        std::optional<std::string> m_packageSHA256;   // zip SHA-256
    };
}