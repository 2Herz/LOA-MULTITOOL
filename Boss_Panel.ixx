export module LostArkMultiTool.Views.BossPanel;  // 오버레이 보스 체력 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 보스 체력 형식 가져오기

// 오버레이 보스 체력 패널: 체력바 숫자를 조 · 억 · 만 단위로, 남은 비율(소수 둘째 자리) · 줄 수 · 공대 DPS · 처치 예상 · 광폭화(토벌) 남은 시간
// - 남은 시간 = 화면 타이머(레이드 "광폭화까지" · 가디언 토벌 왼쪽 위)를 읽은 값, 못 읽으면 보스 이름으로 찾은 관문별 광폭화 시간 - 싸운 시간
// - 공대 DPS = 최근 몇 초 동안 체력이 깎인 속도 (파티원이 프로그램을 켜지 않아도 됨)
// - 게임 체력바 위에 고정: 게임이 그린 체력 숫자 자리를 덮어 조 · 억 · 만 · 남은 %를 그리고, 막대 아래에 공대 DPS · 남은 시간 (옮길 수 없음)
// - 보스 체력바가 안 보이면 그리지 않음
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
        void Render(Core::Renderer& renderer, float backOpacity, bool isInteractive) override;   // 게임 체력바 위에 고정해서 그림
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event, bool isInteractive) override;   // 고정이라 입력 없음 (클릭은 다른 패널로)
        void SetPixelScale(float dipPerPixel) noexcept;                               // 게임 화면 픽셀 → 오버레이 DIP 배율 (화면 배율이 바뀔 때)

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
        [[nodiscard]] std::wstring FormatDamageLine(double damagePerSecond) const;   // "공대 DPS · 처치 예상 · 광폭(남은) 시간" 줄

        std::function<std::optional<Vision::BossHealthSample>()> m_provider;   // 보스 체력 함수
        std::optional<Vision::BossHealthSample> m_sample;                      // 마지막 체력
        std::deque<HistoryPoint> m_history;                                    // 최근 체력 기록
        float m_dipPerPixel{ 1.0f };                                           // 픽셀 → DIP
        double m_refreshElapsed{};                                             // 마지막으로 받은 뒤 지난 시간
        bool m_hasSample{};                                                    // 한 번이라도 받았는지
    };
}
