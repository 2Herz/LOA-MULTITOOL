module LostArkMultiTool.Services.RefreshScheduler;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Models.RateLimitState;

namespace LostArkMultiTool::Services
{
    namespace
    {
        constexpr int MIN_TICKS_PER_CYCLE{ 1 };
    }

    void RefreshScheduler::Restart() noexcept
    {
        m_secondsSinceFullRefresh = 0;
        ClearPause();
    }

    RefreshMode RefreshScheduler::OnTick(int watchSeconds, int fullMinutes) noexcept
    {
        m_secondsSinceFullRefresh += watchSeconds;
        return m_secondsSinceFullRefresh >= fullMinutes * Core::APPConstants::SECONDS_PER_MINUTE ? RefreshMode::REFRESH_FULL : RefreshMode::REFRESH_WATCH;
    }

    RefreshPlan RefreshScheduler::Plan(RefreshMode requested, bool isAuto, std::size_t totalCount, std::size_t watchedCount,
        const Models::RateLimitState& rate, std::int64_t now) const noexcept
    {
        // 429 · 점검으로 쉬는 중이면 자동 갱신은 건너뜀 (수동은 사용자가 직접 재시도하는 것이므로 허용)
        if (isAuto && IsPaused(now)) return RefreshPlan{ requested, RefreshDecision::DECISION_SKIP_PAUSED };

        const int total{ static_cast<int>(totalCount) };
        const int watched{ static_cast<int>(watchedCount) };

        // 자동 전체 갱신을 할 만큼은 부족해도, 감시 갱신은 가능하면 감시라도 실행
        // (누적 시간은 그대로라 다음 회차에 다시 전체 갱신을 시도함)
        if (requested == RefreshMode::REFRESH_FULL && total > 0 && !rate.CanAfford(total, now))
        {
            if (isAuto && watched > 0 && rate.CanAfford(watched, now)) return RefreshPlan{ RefreshMode::REFRESH_WATCH, RefreshDecision::DECISION_RUN };
            return RefreshPlan{ RefreshMode::REFRESH_FULL, RefreshDecision::DECISION_SKIP_RATE_LIMIT };
        }

        const int required{ requested == RefreshMode::REFRESH_FULL ? total : watched };
        if (required == 0) return RefreshPlan{ requested, RefreshDecision::DECISION_SKIP_NO_TARGET };
        if (!rate.CanAfford(required, now)) return RefreshPlan{ requested, RefreshDecision::DECISION_SKIP_RATE_LIMIT };

        return RefreshPlan{ requested, RefreshDecision::DECISION_RUN };
    }

    void RefreshScheduler::MarkStarted(RefreshMode mode) noexcept
    {
        if (mode == RefreshMode::REFRESH_FULL) m_secondsSinceFullRefresh = 0;
    }

    bool RefreshScheduler::IsPaused(std::int64_t now) const noexcept
    {
        return m_pausedUntil.has_value() && now < *m_pausedUntil;
    }

    const std::optional<std::int64_t>& RefreshScheduler::GetPausedUntil() const noexcept
    {
        return m_pausedUntil;
    }

    PauseReason RefreshScheduler::GetPauseReason() const noexcept
    {
        return m_pauseReason;
    }

    void RefreshScheduler::PauseForRateLimit(const Models::RateLimitState& rate, std::int64_t now) noexcept
    {
        const std::optional<std::int64_t> reset{ rate.GetResetAtLocal() };
        const std::int64_t until{ reset.has_value() && *reset > now ? *reset : now + Core::APPConstants::RATE_LIMIT_FALLBACK_COOLDOWN_SECONDS };
        Pause(until, PauseReason::PAUSE_RATE_LIMIT);
    }

    void RefreshScheduler::PauseForMaintenance(std::int64_t now) noexcept
    {
        Pause(now + static_cast<std::int64_t>(Core::APPConstants::MAINTENANCE_RETRY_MINUTES) * Core::APPConstants::SECONDS_PER_MINUTE,
            PauseReason::PAUSE_MAINTENANCE);
    }

    void RefreshScheduler::ClearPause() noexcept
    {
        m_pausedUntil.reset();
        m_pauseReason = PauseReason::PAUSE_NONE;
    }

    int RefreshScheduler::EstimateRequestsPerMinute(std::size_t totalCount, std::size_t watchedCount, int watchSeconds, int fullMinutes) noexcept
    {
        const int fullSeconds{ fullMinutes * Core::APPConstants::SECONDS_PER_MINUTE };
        if (fullSeconds <= 0 || watchSeconds <= 0) return 0;

        const int ticksPerCycle{ std::max(MIN_TICKS_PER_CYCLE, fullSeconds / watchSeconds) };
        const int requestsPerCycle{ static_cast<int>(totalCount) + static_cast<int>(watchedCount) * (ticksPerCycle - 1) };
        return static_cast<int>(std::ceil(static_cast<double>(requestsPerCycle) * Core::APPConstants::SECONDS_PER_MINUTE / fullSeconds));
    }

    void RefreshScheduler::Pause(std::int64_t until, PauseReason reason) noexcept
    {
        m_pausedUntil = until;
        m_pauseReason = reason;
    }
}