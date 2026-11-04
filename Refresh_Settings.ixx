export module LostArkMultiTool.Models.RefreshSettings;

import std;

export namespace LostArkMultiTool::Models
{
    // 보석 검색기 사용자 설정 (재시작해도 유지, 원본 RefreshSettings)
    class RefreshSettings
    {
    public:
        RefreshSettings() noexcept;

        [[nodiscard]] int GetWatchIntervalSeconds() const noexcept;   // 감시 갱신 주기 (초)
        void SetWatchIntervalSeconds(int seconds) noexcept;

        [[nodiscard]] int GetFullIntervalMinutes() const noexcept;    // 전체 갱신 주기 (분)
        void SetFullIntervalMinutes(int minutes) noexcept;

        [[nodiscard]] bool IsAutoRefreshEnabled() const noexcept;     // 자동 갱신을 켜 둔 상태로 종료했는지
        void SetAutoRefreshEnabled(bool isEnabled) noexcept;

        // 선택지에 없는 값이 들어 있으면 기본값으로 되돌림 (저장 파일을 직접 고친 경우 등)
        void Normalize() noexcept;

    private:
        int m_watchIntervalSeconds{};
        int m_fullIntervalMinutes{};
        bool m_isAutoRefreshEnabled{};
    };
}