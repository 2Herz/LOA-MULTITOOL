module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <bcrypt.h>                      // BCryptHash (SHA-256)
#pragma comment(lib, "bcrypt.lib")       // BCrypt 구현

module LostArkMultiTool.Services.UpdateInstaller;

import std;
import LostArkMultiTool.Core.AppLogger;
import LostArkMultiTool.Services.WinHTTPClient;

namespace LostArkMultiTool::Services::UpdateInstaller
{
    namespace
    {
        constexpr std::string_view HTTPS_PREFIX{ "https://" };                   // 받을 수 있는 주소는 https만
        constexpr char PATH_SEPARATOR{ '/' };                                     // 호스트와 경로 구분
        constexpr std::size_t SHA256_BYTE_COUNT{ 32 };                            // SHA-256 결과 바이트 수
        constexpr std::size_t SHA256_HEX_LENGTH{ SHA256_BYTE_COUNT * 2 };         // 16진수 글자 수 (64)
        constexpr int SECONDS_PER_MINUTE{ 60 };                                   // 1분
        constexpr int MS_PER_SECOND{ 1000 };                                      // 1초
        constexpr int DOWNLOAD_TIMEOUT_MS{ 2 * SECONDS_PER_MINUTE * MS_PER_SECOND };   // 연결 · 수신 각각 최대 2분
        constexpr std::wstring_view VERB_GET{ L"GET" };                           // 다운로드 요청
        constexpr std::wstring_view NO_HEADERS{};                                 // 추가 헤더 없음
        constexpr std::wstring_view UPDATE_FOLDER_NAME{ L"LostArkMultiToolUpdate" };   // %TEMP% 아래 작업 폴더
        constexpr std::wstring_view PACKAGE_FILE_NAME{ L"package.zip" };          // 받은 zip 이름
        constexpr std::wstring_view SCRIPT_FILE_NAME{ L"Install-Update.ps1" };    // 설치 스크립트 이름
        constexpr std::wstring_view POWERSHELL_EXE{ L"powershell.exe" };          // Windows 기본 PowerShell 5.1

        // 설치 스크립트 (ASCII만 → PowerShell 5.1이 BOM 없는 파일을 CP949로 읽어도 깨지지 않음, 경로는 인자로 받음)
        // 1) 멀티툴 종료 대기 2) 압축 풀기 (폴더 하나로 감싼 zip도 처리) 3) exe 폴더에 덮어쓰기 4) 새 exe 실행
        // 실패하면 update.log에 남기고 원래 프로그램을 다시 실행
        constexpr std::string_view INSTALL_SCRIPT{
            "param([int]$ProcessId, [string]$ZipPath, [string]$TargetDir, [string]$ExePath, [string]$LogPath)\r\n"
            "$ErrorActionPreference = 'Stop'\r\n"
            "function Write-Log([string]$Text) { try { Add-Content -LiteralPath $LogPath -Value ((Get-Date -Format 's') + ' ' + $Text) -Encoding UTF8 } catch {} }\r\n"
            "try {\r\n"
            "    try { Wait-Process -Id $ProcessId -Timeout 60 -ErrorAction SilentlyContinue } catch {}\r\n"
            "    $Work = Join-Path (Split-Path -Parent $ZipPath) 'extract'\r\n"
            "    if (Test-Path -LiteralPath $Work) { Remove-Item -LiteralPath $Work -Recurse -Force }\r\n"
            "    Expand-Archive -LiteralPath $ZipPath -DestinationPath $Work -Force\r\n"
            "    $Root = $Work\r\n"
            "    $Items = @(Get-ChildItem -LiteralPath $Work)\r\n"
            "    if ($Items.Count -eq 1 -and $Items[0].PSIsContainer) { $Root = $Items[0].FullName }\r\n"
            "    $NewExe = Get-ChildItem -LiteralPath $Root -Filter '*.exe' | Select-Object -First 1\r\n"
            "    if (-not $NewExe) { throw 'package has no exe' }\r\n"
            "    Get-ChildItem -LiteralPath $Root | ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $TargetDir -Recurse -Force }\r\n"
            "    Write-Log ('updated ' + $TargetDir)\r\n"
            "    Start-Process -FilePath (Join-Path $TargetDir $NewExe.Name) -WorkingDirectory $TargetDir\r\n"
            "}\r\n"
            "catch {\r\n"
            "    Write-Log ('update failed: ' + $_)\r\n"
            "    Start-Process -FilePath $ExePath\r\n"
            "}\r\n" };

        // 오류 메시지 (UTF-8 한글, 화면 · 로그에 그대로)
        constexpr std::string_view UPDATE_ERROR_BAD_URL{ "업데이트 주소가 올바르지 않아요." };
        constexpr std::string_view UPDATE_ERROR_DOWNLOAD_FORMAT{ "업데이트 파일을 받지 못했어요. (HTTP {})" };
        constexpr std::string_view UPDATE_ERROR_EMPTY{ "받은 업데이트 파일이 비어 있어요." };
        constexpr std::string_view UPDATE_ERROR_HASH_FAILED{ "업데이트 파일 검사(SHA-256)를 하지 못했어요." };
        constexpr std::string_view UPDATE_ERROR_HASH_MISMATCH{ "받은 파일이 배포 파일과 달라요. (SHA-256 불일치)" };
        constexpr std::string_view UPDATE_ERROR_SAVE_FAILED{ "업데이트 파일을 저장하지 못했어요." };

        // 16진수 한 글자인지 (해시 형식 검사)
        [[nodiscard]] bool IsHexDigit(char letter) noexcept
        {
            return (letter >= '0' && letter <= '9') || (letter >= 'a' && letter <= 'f') || (letter >= 'A' && letter <= 'F');   // 0-9 a-f A-F
        }

        // 대문자 → 소문자 (해시 비교를 대소문자 무관하게)
        [[nodiscard]] std::string ToLowerText(std::string_view text)
        {
            std::string lower{ text };                                                            // 복사본
            std::ranges::transform(lower, lower.begin(), [](unsigned char letter) { return static_cast<char>(std::tolower(letter)); });   // ASCII 소문자
            return lower;
        }

        // UTF-8 → UTF-16 (호스트 · 경로를 WinHTTP에 넘기기)
        [[nodiscard]] std::wstring ToWide(std::string_view text)
        {
            if (text.empty()) return {};                                                          // 빈 문자열
            const int length{ ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0) };   // 필요한 길이
            std::wstring wide(static_cast<std::size_t>(length), L'\0');                           // 그 길이만큼 확보
            ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);   // 변환
            return wide;
        }

        // 바이트 → SHA-256 소문자 16진수 (실패하면 nullopt)
        [[nodiscard]] std::optional<std::string> ComputeSHA256(std::string_view bytes) noexcept
        {
            std::array<unsigned char, SHA256_BYTE_COUNT> digest{};                                // 결과 32바이트
            const NTSTATUS status{ ::BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
                reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())), static_cast<ULONG>(bytes.size()),
                digest.data(), static_cast<ULONG>(digest.size())) };                              // 한 번에 해시 (Windows 10+)
            if (!BCRYPT_SUCCESS(status)) return std::nullopt;                                     // 실패

            try
            {
                std::string hex;                                                                  // 16진수 문자열
                hex.reserve(SHA256_HEX_LENGTH);                                                   // 64자
                for (const unsigned char value : digest) hex += std::format("{:02x}", value);     // 바이트마다 두 글자
                return hex;
            }
            catch (...)
            {
                return std::nullopt;                                                              // 메모리 부족 등
            }
        }

        // %TEMP%\LostArkMultiToolUpdate (없으면 만듦)
        [[nodiscard]] std::filesystem::path GetUpdateDirectory()
        {
            const std::filesystem::path directory{ std::filesystem::temp_directory_path() / UPDATE_FOLDER_NAME };   // 임시 폴더 아래
            std::filesystem::create_directories(directory);                                       // 없으면 만들기
            return directory;
        }

        // 지금 실행 중인 exe 전체 경로
        [[nodiscard]] std::filesystem::path GetExecutablePath()
        {
            std::wstring buffer(MAX_PATH, L'\0');                                                 // 처음 크기
            for (;;)
            {
                const DWORD length{ ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())) };   // 경로 읽기
                if (length < buffer.size()) { buffer.resize(length); return buffer; }              // 다 들어감
                buffer.resize(buffer.size() * 2);                                                 // 잘렸으면 두 배로 다시
            }
        }

        // 명령줄 인자 하나를 큰따옴표로 감쌈 (경로의 공백 · 한글 보호, 경로에는 큰따옴표가 올 수 없음)
        [[nodiscard]] std::wstring Quote(std::wstring_view text)
        {
            return std::format(L"\"{}\"", text);                                                  // "경로"
        }
    }

    bool IsValidPackage(const UpdatePackage& package) noexcept
    {
        if (!package.m_url.starts_with(HTTPS_PREFIX)) return false;                               // https 주소만
        if (package.m_sha256.size() != SHA256_HEX_LENGTH) return false;                           // 64자
        return std::ranges::all_of(package.m_sha256, IsHexDigit);                                 // 전부 16진수
    }

    std::filesystem::path DownloadAndVerify(const UpdatePackage& package)
    {
        if (!IsValidPackage(package)) throw std::runtime_error{ std::string{ UPDATE_ERROR_BAD_URL } };   // 형식부터 확인

        const std::string_view address{ std::string_view{ package.m_url }.substr(HTTPS_PREFIX.size()) };   // "github.com/…"
        const std::size_t slash{ address.find(PATH_SEPARATOR) };                                  // 호스트 끝
        if (slash == std::string_view::npos || slash == 0) throw std::runtime_error{ std::string{ UPDATE_ERROR_BAD_URL } };   // 경로 없음

        Core::AppLogger::GetInstance().Info(std::format("업데이트 받기 시작: {} ({})", package.m_version, package.m_url));   // 시작 기록
        WinHTTPClient client{ ToWide(address.substr(0, slash)), DOWNLOAD_TIMEOUT_MS };           // 호스트 연결 (리다이렉트는 WinHTTP가 따라감)
        const HTTPResponse response{ client.Send(VERB_GET, ToWide(address.substr(slash)), NO_HEADERS, std::string{}) };   // zip 받기
        if (!WinHTTPClient::IsSuccessStatus(response.m_statusCode))
            throw std::runtime_error{ std::vformat(UPDATE_ERROR_DOWNLOAD_FORMAT, std::make_format_args(response.m_statusCode)) };   // HTTP 오류
        if (response.m_body.empty()) throw std::runtime_error{ std::string{ UPDATE_ERROR_EMPTY } };      // 빈 응답

        const std::optional<std::string> actualHash{ ComputeSHA256(response.m_body) };            // 받은 파일 해시
        if (!actualHash.has_value()) throw std::runtime_error{ std::string{ UPDATE_ERROR_HASH_FAILED } };   // 해시 계산 실패
        if (*actualHash != ToLowerText(package.m_sha256))
        {
            Core::AppLogger::GetInstance().Warning(std::format("업데이트 해시 불일치: 기대 {} / 실제 {}", package.m_sha256, *actualHash));   // 원인 기록
            throw std::runtime_error{ std::string{ UPDATE_ERROR_HASH_MISMATCH } };                       // 손상 · 변조 · 다른 파일
        }

        const std::filesystem::path zipPath{ GetUpdateDirectory() / PACKAGE_FILE_NAME };          // 저장 위치
        {
            std::ofstream file{ zipPath, std::ios::binary | std::ios::trunc };                    // 덮어쓰기
            file.write(response.m_body.data(), static_cast<std::streamsize>(response.m_body.size()));   // 그대로 저장
            if (!file) throw std::runtime_error{ std::string{ UPDATE_ERROR_SAVE_FAILED } };               // 디스크 문제
        }

        Core::AppLogger::GetInstance().Info(std::format("업데이트 파일 검증 완료 ({} 바이트)", response.m_body.size()));   // 성공 기록
        return zipPath;
    }

    bool LaunchInstaller(const std::filesystem::path& zipPath) noexcept
    {
        try
        {
            const std::filesystem::path scriptPath{ zipPath.parent_path() / SCRIPT_FILE_NAME };   // 같은 임시 폴더
            {
                std::ofstream script{ scriptPath, std::ios::binary | std::ios::trunc };          // ASCII 스크립트 저장
                script << INSTALL_SCRIPT;                                                        // 내용
                if (!script) return false;                                                       // 저장 실패
            }

            const std::filesystem::path exePath{ GetExecutablePath() };                           // 지금 exe
            const std::filesystem::path logPath{ Core::AppLogger::GetLogDirectory() / L"update.log" };   // 설치 결과 기록

            std::wstring commandLine{ std::format(L"{} -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File {} -ProcessId {} -ZipPath {} -TargetDir {} -ExePath {} -LogPath {}",
                POWERSHELL_EXE, Quote(scriptPath.wstring()), ::GetCurrentProcessId(), Quote(zipPath.wstring()),
                Quote(exePath.parent_path().wstring()), Quote(exePath.wstring()), Quote(logPath.wstring())) };   // 인자 모두 따옴표

            STARTUPINFOW startupInfo{};                                                           // 기본 시작 정보
            startupInfo.cb = sizeof(startupInfo);                                                 // 구조체 크기
            startupInfo.dwFlags = STARTF_USESHOWWINDOW;                                           // 창 표시 방식 지정
            startupInfo.wShowWindow = SW_HIDE;                                                    // 콘솔 창 숨김
            PROCESS_INFORMATION processInfo{};                                                    // 만든 프로세스 핸들

            const BOOL isCreated{ ::CreateProcessW(nullptr, commandLine.data(), nullptr, nullptr, FALSE,
                CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo) };               // 설치 스크립트 실행
            if (isCreated == FALSE)
            {
                Core::AppLogger::GetInstance().Error(std::format("업데이트 설치 시작 실패 (오류 {})", ::GetLastError()));   // 원인 기록
                return false;
            }

            ::CloseHandle(processInfo.hThread);                                                   // 핸들 정리 (누수 방지)
            ::CloseHandle(processInfo.hProcess);                                                  // 기다리지 않음
            Core::AppLogger::GetInstance().Info("업데이트 설치 스크립트 실행 → 프로그램 종료");    // 흐름 기록
            return true;
        }
        catch (...)
        {
            return false;                                                                         // 경로 계산 등 실패
        }
    }
}
