module LostArkMultiTool.Core.IntervalTimer;

import std;

namespace LostArkMultiTool::Core
{
    namespace
    {
        using Clock = std::chrono::steady_clock;   // 시스템 시간을 바꿔도 거꾸로 가지 않는 시계

        constexpr std::int64_t MS_PER_SECOND{ 1000 };
        constexpr int MIN_INTERVAL_MS{ 1 };

        // 현재 시각 (시계 눈금 수) — duration 변환 템플릿을 거치지 않아 편집기(IntelliSense)가 해석하지 못하는 부분이 없음
        [[nodiscard]] std::int64_t GetClockTicks() noexcept
        {
            return static_cast<std::int64_t>(Clock::now().time_since_epoch().count());
        }

        // 밀리초 → 시계 눈금 수 (눈금 단위 = Clock::period, MSVC는 나노초)
        [[nodiscard]] std::int64_t MillisecondsToTicks(int milliseconds) noexcept
        {
            return static_cast<std::int64_t>(milliseconds) * Clock::period::den / (Clock::period::num * MS_PER_SECOND);
        }
    }

    void IntervalTimer::Start(int intervalMilliseconds)
    {
        m_intervalTicks = MillisecondsToTicks(std::max(intervalMilliseconds, MIN_INTERVAL_MS));
        m_nextTicks = GetClockTicks() + m_intervalTicks;
        m_isRunning = true;
    }

    void IntervalTimer::Stop() noexcept
    {
        m_isRunning = false;
    }

    bool IntervalTimer::IsRunning() const noexcept
    {
        return m_isRunning;
    }

    bool IntervalTimer::Poll()
    {
        if (!m_isRunning) return false;

        const std::int64_t nowTicks{ GetClockTicks() };
        if (nowTicks < m_nextTicks) return false;

        // 오래 멈춰 있었어도(절전 등) 밀린 횟수만큼 몰아서 알리지 않고 지금부터 다시 셈
        m_nextTicks = nowTicks + m_intervalTicks;
        return true;
    }
}