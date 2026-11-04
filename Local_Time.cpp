module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOUSER   // winuser.h 제외: GetClassName 같은 매크로 충돌 방지
#include <Windows.h>

module LostArkMultiTool.Core.LocalTime;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Core::LocalTime
{
    namespace
    {
        constexpr std::int64_t FILETIME_TICKS_PER_SECOND{ 10'000'000 };           // FILETIME 단위 = 100ns
        constexpr std::int64_t UNIX_EPOCH_FILETIME_SECONDS{ 11'644'473'600 };      // 1601-01-01 → 1970-01-01 (초)
        constexpr unsigned DWORD_BITS{ 32 };
        constexpr std::wstring_view INVALID_CLOCK_TEXT{ L"--:--:--" };            // 변환할 수 없는 시각

        // 지금 시각 (Windows 로컬 시간)
        SYSTEMTIME GetLocalNow()
        {
            SYSTEMTIME now{};
            ::GetLocalTime(&now);
            return now;
        }
    }

    std::chrono::year_month_day GetLocalToday()
    {
        const SYSTEMTIME now{ GetLocalNow() };
        return std::chrono::year_month_day{
            std::chrono::year{ static_cast<int>(now.wYear) },
            std::chrono::month{ static_cast<unsigned>(now.wMonth) },
            std::chrono::day{ static_cast<unsigned>(now.wDay) }
        };
    }

    std::chrono::year_month_day GetGameToday()
    {
        // 날짜와 시각을 한 번에 읽어야 자정 · 06시 경계에서 날짜와 시각이 어긋나지 않음
        const SYSTEMTIME now{ GetLocalNow() };
        const std::chrono::year_month_day today{
            std::chrono::year{ static_cast<int>(now.wYear) },
            std::chrono::month{ static_cast<unsigned>(now.wMonth) },
            std::chrono::day{ static_cast<unsigned>(now.wDay) }
        };

        constexpr int PREVIOUS_DAY{ -1 };
        return static_cast<int>(now.wHour) < APPConstants::DAILY_RESET_HOUR ? AddDays(today, PREVIOUS_DAY) : today;
    }

    std::string MakeTimestamp()
    {
        const SYSTEMTIME now{ GetLocalNow() };
        return std::format("{:04}{:02}{:02}_{:02}{:02}{:02}",
            static_cast<int>(now.wYear), static_cast<int>(now.wMonth), static_cast<int>(now.wDay),
            static_cast<int>(now.wHour), static_cast<int>(now.wMinute), static_cast<int>(now.wSecond));
    }

    std::int64_t GetUnixSeconds()
    {
        FILETIME now{};
        ::GetSystemTimeAsFileTime(&now);

        const std::uint64_t ticks{ (static_cast<std::uint64_t>(now.dwHighDateTime) << DWORD_BITS) | now.dwLowDateTime };
        return static_cast<std::int64_t>(ticks) / FILETIME_TICKS_PER_SECOND - UNIX_EPOCH_FILETIME_SECONDS;
    }

    std::wstring FormatClock(std::int64_t unixSeconds)
    {
        // UTC 초 → FILETIME → UTC 시각 → 이 PC의 시간대 시각
        const auto ticks{ static_cast<std::uint64_t>((unixSeconds + UNIX_EPOCH_FILETIME_SECONDS) * FILETIME_TICKS_PER_SECOND) };
        FILETIME fileTime{};
        fileTime.dwLowDateTime = static_cast<DWORD>(ticks);
        fileTime.dwHighDateTime = static_cast<DWORD>(ticks >> DWORD_BITS);

        SYSTEMTIME utcTime{};
        SYSTEMTIME localTime{};
        if (!::FileTimeToSystemTime(&fileTime, &utcTime) || !::SystemTimeToTzSpecificLocalTime(nullptr, &utcTime, &localTime))
        {
            return std::wstring{ INVALID_CLOCK_TEXT };   // 표시만 하는 값이라 예외 대신 자리 표시
        }

        return std::format(L"{:02}:{:02}:{:02}", static_cast<int>(localTime.wHour), static_cast<int>(localTime.wMinute),
            static_cast<int>(localTime.wSecond));
    }

    std::string MakeDateKey(const std::chrono::year_month_day& date)
    {
        return std::format("{:04}-{:02}-{:02}", static_cast<int>(date.year()), static_cast<unsigned>(date.month()),
            static_cast<unsigned>(date.day()));
    }

    std::optional<std::chrono::year_month_day> ParseDateKey(std::string_view key)
    {
        // "yyyy-MM-dd" = 4자리 - 2자리 - 2자리
        constexpr std::size_t KEY_LENGTH{ 10 };
        constexpr std::size_t YEAR_LENGTH{ 4 };
        constexpr std::size_t MONTH_START{ 5 };
        constexpr std::size_t DAY_START{ 8 };
        constexpr std::size_t PART_LENGTH{ 2 };
        constexpr char SEPARATOR{ '-' };
        if (key.size() != KEY_LENGTH || key[YEAR_LENGTH] != SEPARATOR || key[DAY_START - 1] != SEPARATOR) return std::nullopt;

        // 숫자만으로 된 부분을 정수로 (숫자가 아닌 글자가 있으면 실패)
        const auto parse{ [](std::string_view part, auto& value) -> bool
        {
            const auto [end, error]{ std::from_chars(part.data(), part.data() + part.size(), value) };
            return error == std::errc{} && end == part.data() + part.size();
        } };

        int year{};
        unsigned month{};
        unsigned day{};
        if (!parse(key.substr(0, YEAR_LENGTH), year) || !parse(key.substr(MONTH_START, PART_LENGTH), month)
            || !parse(key.substr(DAY_START, PART_LENGTH), day)) return std::nullopt;

        const std::chrono::year_month_day date{ std::chrono::year{ year }, std::chrono::month{ month }, std::chrono::day{ day } };
        if (!date.ok()) return std::nullopt;
        return date;
    }

    std::chrono::year_month_day AddDays(const std::chrono::year_month_day& date, int days)
    {
        return std::chrono::year_month_day{ std::chrono::sys_days{ date } + std::chrono::days{ days } };
    }
}