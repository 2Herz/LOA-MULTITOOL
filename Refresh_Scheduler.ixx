export module LostArkMultiTool.Services.RefreshScheduler;

import std;
import LostArkMultiTool.Models.RateLimitState;

export namespace LostArkMultiTool::Services
{
    // 갱신 종류: 전체(표 전체) / 감시(알림 걸린 보석만)
    enum class RefreshMode
    {
        REFRESH_FULL,
        REFRESH_WATCH
    };

    // 갱신을 실행할지 판단한 결과
    enum class RefreshDecision
    {
        DECISION_RUN,               // 실행
        DECISION_SKIP_PAUSED,       // 429 · 점검으로 쉬는 중
        DECISION_SKIP_NO_TARGET,    // 조회할 보석 없음 (감시할 알림 없음)
        DECISION_SKIP_RATE_LIMIT    // 남은 호출 부족
    };

    // 실행 계획 (실제로 실행할 갱신 종류 + 판단 결과)
    struct RefreshPlan
    {
        RefreshMode m_mode{ RefreshMode::REFRESH_FULL };
        RefreshDecision m_decision{ RefreshDecision::DECISION_RUN };
    };

    // 일시정지 이유 (화면 표시용)
    enum class PauseReason
    {
        PAUSE_NONE,
        PAUSE_RATE_LIMIT,     // 호출 한도 초과 (429)
        PAUSE_MAINTENANCE     // API 서버 점검 중 (503)
    };

    // 자동 갱신 일정과 실행 여부 판단만 담당 (화면 없이 테스트 가능, 원본 RefreshScheduler)
    //  - 감시 주기마다 전체 / 감시 중 무엇을 할지
    //  - 호출 한도 · 일시정지 상태에서 실행해도 되는지
    //  - 429 · 점검 시 언제까지 쉴지
    // 시각은 모두 이 PC 시계 기준 UTC 초
    class RefreshScheduler
    {
    public:
        // 자동 갱신을 새로 켰을 때 (주기 처음부터 + 일시정지 해제)
        void Restart() noexcept;

        // 감시 주기 1회: 누적 시간을 더해서 전체 주기가 됐으면 전체, 아니면 감시
        [[nodiscard]] RefreshMode OnTick(int watchSeconds, int fullMinutes) noexcept;

        // 요청받은 갱신을 지금 실행해도 되는지 판단 (수동 갱신은 일시정지와 상관없이 허용)
        [[nodiscard]] RefreshPlan Plan(RefreshMode requested, bool isAuto, std::size_t totalCount, std::size_t watchedCount,
            const Models::RateLimitState& rate, std::int64_t now) const noexcept;

        // 실제로 갱신이 시작될 때 (전체 갱신이면 누적 시간 초기화)
        void MarkStarted(RefreshMode mode) noexcept;

        [[nodiscard]] bool IsPaused(std::int64_t now) const noexcept;
        [[nodiscard]] const std::optional<std::int64_t>& GetPausedUntil() const noexcept;
        [[nodiscard]] PauseReason GetPauseReason() const noexcept;

        // 429: 초기화 시각을 알면 그때까지, 모르면 정해진 시간만큼 쉼
        void PauseForRateLimit(const Models::RateLimitState& rate, std::int64_t now) noexcept;
        // 503: 정해진 간격으로만 다시 확인
        void PauseForMaintenance(std::int64_t now) noexcept;
        // 일시정지 해제 (갱신 성공 시)
        void ClearPause() noexcept;

        // 분당 예상 요청 수 (최소치, 페이지 넘김 제외)
        // 전체 주기 한 바퀴 동안 감시 주기가 N번 → 그중 1번은 전체(감시 보석 포함), 나머지 N-1번은 감시
        [[nodiscard]] static int EstimateRequestsPerMinute(std::size_t totalCount, std::size_t watchedCount, int watchSeconds, int fullMinutes) noexcept;

    private:
        void Pause(std::int64_t until, PauseReason reason) noexcept;

        int m_secondsSinceFullRefresh{};          // 마지막 전체 갱신 이후 지난 시간
        std::optional<std::int64_t> m_pausedUntil; // 이 시각까지 자동 갱신을 쉼
        PauseReason m_pauseReason{ PauseReason::PAUSE_NONE };
    };
}