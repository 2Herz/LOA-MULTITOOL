module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOUSER   // winuser.h 제외: GetClassName 같은 매크로 충돌 방지
#include <Windows.h>

module LostArkMultiTool.Core.LocalTime;

import std;

namespace LostArkMultiTool::Core::LocalTime
{
    namespace
    {
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

    std::string MakeTimestamp()
    {
        const SYSTEMTIME now{ GetLocalNow() };
        return std::format("{:04}{:02}{:02}_{:02}{:02}{:02}",
            static_cast<int>(now.wYear), static_cast<int>(now.wMonth), static_cast<int>(now.wDay),
            static_cast<int>(now.wHour), static_cast<int>(now.wMinute), static_cast<int>(now.wSecond));
    }
}