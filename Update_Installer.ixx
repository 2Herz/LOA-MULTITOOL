export module LostArkMultiTool.Services.UpdateInstaller;

import std;

export namespace LostArkMultiTool::Services
{
    // 원격 설정이 알려 준 새 버전 배포 파일 (GitHub Releases zip)
    struct UpdatePackage
    {
        std::string m_version;   // 새 버전 (예: "1.6.0")
        std::string m_url;       // zip 주소 (https만)
        std::string m_sha256;    // zip SHA-256 (소문자 16진수 64자)
    };

    // 자동 업데이트: zip 받기 → SHA-256 검증 → (프로그램 종료 후) 압축 풀어 덮어쓰기 → 새 버전 실행
    // 받기 · 검증은 블로킹이므로 백그라운드에서, 설치 시작은 화면 스레드에서 호출하고 바로 프로그램을 닫을 것
    namespace UpdateInstaller
    {
        // 자동 업데이트에 필요한 값이 모두 있는지 (https 주소 + 64자 16진수 해시)
        [[nodiscard]] bool IsValidPackage(const UpdatePackage& package) noexcept;

        // zip을 임시 폴더에 받고 SHA-256을 확인 (실패하면 std::runtime_error, 메시지는 UTF-8 한글) → 받은 파일 경로
        [[nodiscard]] std::filesystem::path DownloadAndVerify(const UpdatePackage& package);

        // 설치 스크립트를 띄움 (이 프로그램이 끝나기를 기다렸다가 덮어쓰고 새 버전 실행) → 띄웠으면 true
        [[nodiscard]] bool LaunchInstaller(const std::filesystem::path& zipPath) noexcept;
    }
}
