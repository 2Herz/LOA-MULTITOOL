module LostArkMultiTool.Core.ServerConfigChecker;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Core
{
    namespace
    {
        constexpr std::size_t MIN_VERSION_PARTS{ 2 };   // "1.0"     (C# Version 최소 자릿수)
        constexpr std::size_t MAX_VERSION_PARTS{ 4 };   // "1.0.5.0" (C# Version 최대 자릿수)
        constexpr char VERSION_SEPARATOR{ '.' };
        constexpr std::string_view WHITESPACE_CHARS{ " \t\r\n" };

        // 버전 숫자 묶음 (비어 있는 자리는 0 → C# Normalize의 Build·Revision -1 → 0과 같음)
        using VersionParts = std::array<int, MAX_VERSION_PARTS>;

        // 앞뒤 공백 제거 (C# Trim과 같음)
        std::string_view TrimWhitespace(std::string_view text) noexcept
        {
            const std::size_t first{ text.find_first_not_of(WHITESPACE_CHARS) };
            if (first == std::string_view::npos) return {};

            const std::size_t last{ text.find_last_not_of(WHITESPACE_CHARS) };
            return text.substr(first, last - first + 1);
        }

        // "1.0.5" → {1, 0, 5, 0} (형식이 틀리면 nullopt, C# Version.TryParse와 같은 규칙)
        std::optional<VersionParts> ParseVersion(std::string_view text)
        {
            VersionParts parts{};
            std::size_t count{};

            for (const auto part : std::views::split(TrimWhitespace(text), VERSION_SEPARATOR))
            {
                if (count >= MAX_VERSION_PARTS) return std::nullopt;   // 다섯 자리 이상

                const std::string_view digits{ part.begin(), part.end() };
                int value{};
                const auto [ptr, ec] { std::from_chars(digits.data(), digits.data() + digits.size(), value) };
                if (digits.empty() || ec != std::errc{} || ptr != digits.data() + digits.size() || value < 0) return std::nullopt;

                parts.at(count) = value;
                ++count;
            }

            if (count < MIN_VERSION_PARTS) return std::nullopt;   // "1"처럼 한 자리만 있으면 C#도 실패
            return parts;
        }
    }

    ServerConfigChecker::~ServerConfigChecker() = default;

    std::string_view ServerConfigChecker::GetCurrentVersion() noexcept
    {
        return APPConstants::APP_VERSION;
    }

    bool ServerConfigChecker::IsNewerVersion(std::string_view latest, std::string_view current)
    {
        const std::optional<VersionParts> latestParts{ ParseVersion(latest) };
        const std::optional<VersionParts> currentParts{ ParseVersion(current) };
        if (!latestParts.has_value() || !currentParts.has_value()) return false;   // 읽을 수 없으면 안내하지 않음

        return *latestParts > *currentParts;   // 앞자리부터 차례로 비교
    }
}