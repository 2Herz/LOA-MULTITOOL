module LostArkMultiTool.Services.GistServerConfigChecker;

import std;
import LostArkMultiTool.Core.APPConstants;
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

        // 문자열이면 값, 아니면 nullopt (원본 string? 속성과 같음)
        std::optional<std::string> ReadOptionalString(const JSONValue& object, std::string_view key)
        {
            const JSONValue& value{ object.FindMember(key) };
            if (value.GetType() != JSONType::STRING_VALUE) return std::nullopt;
            return value.GetString();
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
            if (!WinHTTPClient::IsSuccessStatus(response.m_statusCode)) return std::nullopt;

            const JSONValue root{ ParseJSONText(response.m_body) };
            if (root.GetType() != JSONType::OBJECT_VALUE) return std::nullopt;   // 내용이 "null" 등이면 설정 없음 (원본과 같음)

            return Models::ServerConfig{
                root.FindMember(KEY_IS_MAINTENANCE).GetBool(),
                ReadOptionalString(root, KEY_MAINTENANCE_MESSAGE),
                ReadOptionalString(root, KEY_LATEST_VERSION),
                ReadOptionalString(root, KEY_DOWNLOAD_URL),
                ReadOptionalString(root, KEY_NOTICE) };
        }
        catch (const std::exception&)
        {
            return std::nullopt;   // 네트워크 오류 · Gist 문제 · 형식 오류 → 점검 아님으로 취급 (원본과 같음)
        }
    }
}