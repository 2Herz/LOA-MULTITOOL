module LostArkMultiTool.Models.RateLimitState;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Models
{
    void RateLimitState::ApplyResponse(const RateLimitHeaders& headers, std::int64_t localNow)
    {
        // 서버 시계 − 내 PC 시계 (Date 헤더가 있을 때만)
        if (headers.m_serverTime.has_value()) m_clockOffset = *headers.m_serverTime - localNow;
        if (!headers.m_remaining.has_value()) return;   // 남은 호출 헤더가 없으면 한도 정보는 그대로

        const std::int64_t serverNow{ ToServerTime(localNow) };
        const int remaining{ *headers.m_remaining };

        // Reset 헤더만 빠진 응답은 지금 알고 있는 구간의 응답으로 간주
        const std::optional<std::int64_t> effectiveReset{ headers.m_resetAt.has_value() ? headers.m_resetAt : m_resetAt };

        // ① 이미 아는 구간보다 이전 구간의 응답이면 무시 (동시 요청은 응답 도착 순서가 뒤바뀔 수 있음)
        if (m_resetAt.has_value() && effectiveReset.has_value() && *effectiveReset < *m_resetAt) return;

        // ② 같은 구간인지: Reset을 알면 그 값이 같은지, 모르면 구간 시작 후 60초가 안 지났는지
        const bool isSameWindow{ effectiveReset.has_value()
            ? (m_resetAt == effectiveReset && serverNow < *effectiveReset)
            : (m_windowStart.has_value() && serverNow - *m_windowStart < Core::APPConstants::RATE_LIMIT_WINDOW_SECONDS) };

        // ③ 같은 구간이면 더 작은 남은 수를 유지 (늦게 도착한 옛 응답이 값을 되돌리지 못하게)
        if (isSameWindow && m_remaining.has_value() && *m_remaining <= remaining) return;

        if (!isSameWindow) m_windowStart = serverNow;   // 새 구간 시작 기록
        if (headers.m_limit.has_value()) m_limit = headers.m_limit;
        m_remaining = remaining;
        m_resetAt = effectiveReset;
    }

    const std::optional<int>& RateLimitState::GetLimit() const noexcept
    {
        return m_limit;
    }

    const std::optional<int>& RateLimitState::GetRemaining() const noexcept
    {
        return m_remaining;
    }

    std::optional<std::int64_t> RateLimitState::GetEffectiveReset() const noexcept
    {
        if (m_resetAt.has_value()) return m_resetAt;
        if (m_windowStart.has_value()) return *m_windowStart + Core::APPConstants::RATE_LIMIT_WINDOW_SECONDS;
        return std::nullopt;
    }

    bool RateLimitState::CanAfford(int requiredRequests, std::int64_t localNow) const noexcept
    {
        const std::optional<std::int64_t> reset{ GetEffectiveReset() };
        if (!m_remaining.has_value() || !reset.has_value()) return true;   // 아직 아무 정보 없음
        if (ToServerTime(localNow) >= *reset) return true;                  // 이미 초기화됨

        return *m_remaining - requiredRequests > Core::APPConstants::RATE_LIMIT_SAFETY_MARGIN;
    }

    bool RateLimitState::IsExhausted(std::int64_t localNow) const noexcept
    {
        return !CanAfford(0, localNow);
    }

    std::optional<std::int64_t> RateLimitState::GetResetAtLocal() const noexcept
    {
        const std::optional<std::int64_t> reset{ GetEffectiveReset() };
        if (!reset.has_value()) return std::nullopt;
        return *reset - m_clockOffset;
    }

    std::int64_t RateLimitState::ToServerTime(std::int64_t localNow) const noexcept
    {
        return localNow + m_clockOffset;
    }
}