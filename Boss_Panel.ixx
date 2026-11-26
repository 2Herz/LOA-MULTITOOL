export module LostArkMultiTool.Views.BossPanel;  // 오버레이 보스 체력 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 보스 체력 형식 가져오기

// 오버레이 보스 체력 패널: 체력바 숫자를 조 · 억 · 만 단위로, 남은 비율(소수 둘째 자리) · 줄 수 · 공대 DPS · 예상 남은 시간
// - 공대 DPS = 최근 몇 초 동안 체력이 깎인 속도 (파티원이 프로그램을 켜지 않아도 됨)
// - 보스 체력바가 안 보이면 평소엔 패널을 그리지 않음 (조작 모드에서는 위치를 잡을 수 있게 보임)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 보스 체력 패널
    class BossPanel final : public OverlayPanel
    {
    public:
        static constexpr std::string_view PANEL_ID{ "boss" };   // 패널 이름

        explicit BossPanel(std::function<std::optional<Vision::BossHealthSample>()> provider);   // 보스 체력을 받아 올 함수

        void Update(double deltaSeconds) override;                                    // 체력 받기 · 깎이는 속도 계산
        [[nodiscard]] bool IsContentAvailable() const noexcept override;              // 보스가 보일 때만

        [[nodiscard]] static std::wstring FormatKoreanNumber(std::int64_t value);     // 83119120458 → "831억 1,912만" (자체 검사용으로도 공개)

    protected:
        void RenderContent(Core::Renderer& renderer, const Core::RectF& content) override;   // 체력 · 비율 · DPS
        [[nodiscard]] std::wstring GetTitleText() const override;                           // "보스 체력"

    private:
        // 깎이는 속도 계산용 기록 하나
        struct HistoryPoint
        {
            std::chrono::steady_clock::time_point m_time;   // 시각
            std::int64_t m_current{};                       // 현재 체력
        };

        [[nodiscard]] std::optional<double> GetDamagePerSecond() const;   // 공대 DPS (기록이 모자라면 nullopt)

        std::function<std::optional<Vision::BossHealthSample>()> m_provider;   // 보스 체력 함수
        std::optional<Vision::BossHealthSample> m_sample;                      // 마지막 체력
        std::deque<HistoryPoint> m_history;                                    // 최근 체력 기록
        double m_refreshElapsed{};                                             // 마지막으로 받은 뒤 지난 시간
        bool m_hasSample{};                                                    // 한 번이라도 받았는지
    };
}
