export module LostArkMultiTool.Models.RateLimitState;

import std;

export namespace LostArkMultiTool::Models
{
    // 응답 헤더 한 번에서 읽은 호출 한도 정보 (없는 헤더는 nullopt)
    struct RateLimitHeaders
    {
        std::optional<int> m_limit;                   // X-RateLimit-Limit (분당 한도)
        std::optional<int> m_remaining;               // X-RateLimit-Remaining (이번 구간에 남은 호출 수)
        std::optional<std::int64_t> m_resetAt;        // X-RateLimit-Reset (다시 채워지는 시각, 서버 시계 UTC 초)
        std::optional<std::int64_t> m_serverTime;     // Date (응답한 서버 시각, UTC 초)
    };

    // 응답 헤더로 알아낸 호출 한도 상태 (원본 RateLimitState)
    // 시각은 모두 1970-01-01 UTC부터 지난 초 / "local"이 붙은 값은 이 PC 시계 기준, 나머지는 서버 시계 기준
    class RateLimitState
    {
    public:
        // 응답 하나를 반영 (동시 요청 응답이 뒤바뀌어 도착해도 옛 응답이 값을 되돌리지 못하게 함)
        void ApplyResponse(const RateLimitHeaders& headers, std::int64_t localNow);

        [[nodiscard]] const std::optional<int>& GetLimit() const noexcept;
        [[nodiscard]] const std::optional<int>& GetRemaining() const noexcept;

        // 실제로 판단에 쓰는 초기화 시각 (Reset 헤더가 있으면 그 값, 없으면 "구간 시작 + 60초"로 가정 → 헤더가 없어도 보호가 동작)
        [[nodiscard]] std::optional<std::int64_t> GetEffectiveReset() const noexcept;

        // 요청 requiredRequests회를 보낸 뒤에도 안전 여유분이 남는지 (아직 아무 정보 없으면 허용)
        [[nodiscard]] bool CanAfford(int requiredRequests, std::int64_t localNow) const noexcept;

        // 남은 호출이 안전 여유분 이하 (요청 0회도 보낼 수 없는 상태)
        [[nodiscard]] bool IsExhausted(std::int64_t localNow) const noexcept;

        // 초기화 시각을 이 PC 시계 기준으로 (일시정지 계산 · 화면 표시용)
        [[nodiscard]] std::optional<std::int64_t> GetResetAtLocal() const noexcept;

    private:
        [[nodiscard]] std::int64_t ToServerTime(std::int64_t localNow) const noexcept;

        std::optional<int> m_limit;
        std::optional<int> m_remaining;
        std::optional<std::int64_t> m_resetAt;        // 서버 시계 기준
        std::optional<std::int64_t> m_windowStart;    // 이번 구간을 처음 관측한 시각 (Reset 헤더가 없을 때 대신 사용)
        std::int64_t m_clockOffset{};                 // 서버 시계 − 내 PC 시계 (초)
    };
}