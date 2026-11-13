module LostArkMultiTool.Services.GistServerConfigChecker;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.AppLogger;
import LostArkMultiTool.Models.ServerConfig;
import LostArkMultiTool.Services.JSONValue;
import LostArkMultiTool.Services.WinHTTPClient;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 원격 설정 위치 (GitHub Gist raw) =====
        constexpr std::wstring_view GIST_HOST{ L"gist.githubusercontent.com" };

        // ⚙️ 멀티툴 전용 Gist의 raw 경로 (커밋 해시 없는 주소 → 항상 최신 내용)
        //    예: L"/2Herz/<Gist ID>/raw/LostArkMultiToolConfig.json"
        //    비워 두면 원격 확인을 하지 않음 (점검 아님 · 업데이트 없음으로 취급)
        constexpr std::wstring_view CONFIG_PATH{ L"/2Herz/80a6fde5d092ec7b39aa593cfad348c7/raw/LostArkMultiToolConfig.json" };

        constexpr std::wstring_view VERB_GET{ L"GET" };
        constexpr std::wstring_view NO_HEADERS{};
        constexpr int CONFIG_TIMEOUT_MS{ Core::APPConstants::CONFIG_TIMEOUT_SECONDS * Core::APPConstants::MS_PER_SECOND };

        // ===== 설정 JSON 키 (원본 AppConfig.json과 같음) =====
        constexpr std::string_view KEY_IS_MAINTENANCE{ "IsMaintenance" };
        constexpr std::string_view KEY_MAINTENANCE_MESSAGE{ "MaintenanceMessage" };
        constexpr std::string_view KEY_LATEST_VERSION{ "LatestVersion" };
        constexpr std::string_view KEY_DOWNLOAD_URL{ "DownloadUrl" };
        constexpr std::string_view KEY_NOTICE{ "Notice" };
        constexpr std::string_view KEY_PACKAGE_URL{ "PackageUrl" };         // 자동 업데이트 zip 주소 (Set-RemoteSwitch.ps1 -Version이 채움)
        constexpr std::string_view KEY_PACKAGE_SHA256{ "PackageSha256" };   // zip SHA-256

        // 마지막으로 받은 원격 설정 (네트워크 · Gist 문제로 못 받았을 때 버전 · 공지 안내에만 사용)
        constexpr std::string_view CONFIG_CACHE_FILE_NAME{ "ServerConfigCache.json" };

        // 문자열이면 값, 아니면 nullopt (원본 string? 속성과 같음)
        std::optional<std::string> ReadOptionalString(const JSONValue& object, std::string_view key)
        {
            const JSONValue& value{ object.FindMember(key) };
            if (value.GetType() != JSONType::STRING_VALUE) return std::nullopt;
            return value.GetString();
        }

        // 설정 JSON → 값 객체 (useMaintenance = false면 점검 스위치는 무시 → 옛 캐시로 프로그램이 막히지 않게)
        std::optional<Models::ServerConfig> ParseConfig(const std::string& text, bool useMaintenance)
        {
            const JSONValue root{ ParseJSONText(text) };
            if (root.GetType() != JSONType::OBJECT_VALUE) return std::nullopt;   // 내용이 "null" 등이면 설정 없음 (원본과 같음)

            return Models::ServerConfig{
                useMaintenance && root.FindMember(KEY_IS_MAINTENANCE).GetBool(),   // 캐시면 항상 점검 아님
                ReadOptionalString(root, KEY_MAINTENANCE_MESSAGE),
                ReadOptionalString(root, KEY_LATEST_VERSION),
                ReadOptionalString(root, KEY_DOWNLOAD_URL),
                ReadOptionalString(root, KEY_NOTICE),
                ReadOptionalString(root, KEY_PACKAGE_URL),                         // 자동 업데이트 zip
                ReadOptionalString(root, KEY_PACKAGE_SHA256) };                    // zip 해시
        }

        // 받은 설정 원문을 캐시 파일에 저장 (실패해도 무시 → 다음 확인 때 다시 시도)
        void SaveCachedConfig(const std::string& text) noexcept
        {
            try
            {
                const std::filesystem::path cachePath{ Core::APPConstants::GetUserDataFilePath(CONFIG_CACHE_FILE_NAME) };   // 사용자 데이터 폴더
                std::error_code error;                                                    // 폴더 만들기 실패는 쓰기에서 드러남
                std::filesystem::create_directories(cachePath.parent_path(), error);     // 첫 실행이면 폴더가 없을 수 있음
                std::ofstream file{ cachePath, std::ios::binary | std::ios::trunc };     // 통째로 덮어씀 (작은 파일)
                file << text;                                                            // 원문 그대로
            }
            catch (...)
            {
                // 캐시 저장 실패는 기능에 영향 없음
            }
        }

        // 캐시 파일에서 마지막 설정 읽기 (없거나 깨졌으면 nullopt = 점검 아님 · 업데이트 없음)
        std::optional<Models::ServerConfig> LoadCachedConfig() noexcept
        {
            try
            {
                std::ifstream file{ Core::APPConstants::GetUserDataFilePath(CONFIG_CACHE_FILE_NAME), std::ios::binary };   // 캐시 파일
                if (!file) return std::nullopt;                                                        // 캐시 없음
                const std::string text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };   // 전체 읽기
                Core::AppLogger::GetInstance().Info("원격 설정 대신 마지막으로 받은 설정 사용 (점검 스위치는 무시)");   // 대체 사용 기록
                return ParseConfig(text, false);                                                       // 점검은 아님으로
            }
            catch (...)
            {
                return std::nullopt;   // 캐시도 깨짐 → 설정 없음
            }
        }

        // 캐시된 옛 내용을 받지 않도록 매번 다른 주소로 요청 (원본 ?t=Ticks와 같은 역할)
        std::wstring MakeNoCachePath()
        {
            const auto ticks{ std::chrono::system_clock::now().time_since_epoch().count() };
            return std::format(L"{}?t={}", CONFIG_PATH, ticks);
        }
    }

    std::optional<Models::ServerConfig> GistServerConfigChecker::Fetch()
    {
        if (CONFIG_PATH.empty()) return std::nullopt;   // 설정 주소를 아직 정하지 않음 → 확인하지 않음

        try
        {
            WinHTTPClient client{ GIST_HOST, CONFIG_TIMEOUT_MS };
            const HTTPResponse response{ client.Send(VERB_GET, MakeNoCachePath(), NO_HEADERS, std::string{}) };
            if (!WinHTTPClient::IsSuccessStatus(response.m_statusCode))
            {
                Core::AppLogger::GetInstance().Warning(std::format("원격 설정 응답 오류 HTTP {}", response.m_statusCode));   // 상태 코드 기록
                return LoadCachedConfig();                                                                                   // 마지막 성공 값으로 대신
            }

            std::optional<Models::ServerConfig> config{ ParseConfig(response.m_body, true) };   // 점검 스위치까지 그대로 사용
            if (config.has_value()) SaveCachedConfig(response.m_body);                          // 다음 실패 때 쓸 마지막 성공 값
            return config;
        }
        catch (const std::exception& exception)
        {
            Core::AppLogger::GetInstance().Warning(std::format("원격 설정 확인 실패: {}", exception.what()));   // 네트워크 · Gist 문제 기록
            return LoadCachedConfig();   // 마지막 성공 값의 버전 · 공지만 사용 (점검은 아님으로 취급 → 서버 문제로 막히지 않게)
        }
    }
}
