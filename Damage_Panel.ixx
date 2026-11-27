export module LostArkMultiTool.Views.DamagePanel;  // 오버레이 딜 지분 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 보스 체력 · 전투 분석기 형식 가져오기

// 오버레이 딜 지분 패널: 전투 분석기의 내 피해량 · DPS + 보스 체력 → 지분 · 몇 인분 · 단계
// - 깎인 체력 대비 지분 = 내 피해량 / (최대 체력 - 현재 체력), 최대 체력 대비 = 내 피해량 / 최대 체력
// - 몇 인분 = 깎인 체력 대비 지분 × 딜러 수 (8인 6명, 4인 3명)
// - 단계 기준 (깎인 체력 대비 지분): 8인 15% · 20%, 4인 30% · 40% → 투사 · 강투 · 잔혈
// - 8인 · 4인은 조작 모드에서 패널 단추로 바꿈 (오버레이 설정에 저장)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 딜 지분에 쓰는 값 묶음
    struct DamageShareSource
    {
        std::optional<Vision::BossHealthSample> m_boss;     // 보스 체력
        std::optional<Vision::CombatSample> m_combat;       // 전투 분석기
    };

    // 딜 지분 계산 결과
    struct DamageShare
    {
        double m_depletedShare{};    // 깎인 체력 대비 (0 ~ 1)
        double m_maximumShare{};     // 최대 체력 대비 (0 ~ 1)
        double m_portions{};         // 몇 인분
        int m_stage{};               // 단계 (0 투사 · 1 강투 · 2 잔혈)
    };

    // 딜 지분 패널
    class DamagePanel final : public OverlayPanel
    {
    public:
        static constexpr std::string_view PANEL_ID{ "damage" };   // 패널 이름
        static constexpr int RAID_SIZE_EIGHT{ 8 };                // 8인 레이드
        static constexpr int RAID_SIZE_FOUR{ 4 };                 // 4인 레이드

        // provider: 보스 체력 · 전투 분석기 / getRaidSize · setRaidSize: 8인 · 4인 설정
        DamagePanel(std::function<DamageShareSource()> provider, std::function<int()> getRaidSize, std::function<void(int)> setRaidSize);

        void Update(double deltaSeconds) override;                          // 값 받기
        [[nodiscard]] bool IsContentAvailable() const noexcept override;    // 전투 분석기 값이 있을 때만

        // 지분 계산 (자체 검사용으로도 공개, 깎인 체력이 없으면 nullopt)
        [[nodiscard]] static std::optional<DamageShare> Calculate(std::int64_t damage, std::int64_t current, std::int64_t maximum, int raidSize) noexcept;

    protected:
        void RenderContent(Core::Renderer& renderer, const Core::RectF& content) override;                           // 피해량 · 지분 · 단계
        [[nodiscard]] bool HandleContentInput(const Core::InputEvent& event, const Core::RectF& content) override;   // 8인 · 4인 단추
        [[nodiscard]] std::wstring GetTitleText() const override;                                                   // "딜 지분 · 8인"

    private:
        [[nodiscard]] Core::RectF GetSizeButtonRect(const Core::RectF& content) const noexcept;   // 8인 · 4인 단추

        std::function<DamageShareSource()> m_provider;   // 값 함수
        std::function<int()> m_getRaidSize;              // 인원 읽기
        std::function<void(int)> m_setRaidSize;          // 인원 바꾸기
        DamageShareSource m_source;                      // 마지막 값
        double m_refreshElapsed{};                       // 마지막으로 받은 뒤 지난 시간
        bool m_hasSource{};                              // 한 번이라도 받았는지
        bool m_isHoverButton{};                          // 단추 위에 마우스
    };
}
