export module LostArkMultiTool.Views.BuffPanel;  // 오버레이 버프 타이머 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Vision.BuffTemplates;  // 버프 종류 · 아이콘 템플릿 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 버프 읽기 형식 가져오기

// 오버레이 버프 타이머 패널: 암흑 수류탄(보스) · 아드로핀 · 서폿 초각성기 · 초각성 스킬 · 공증 · 아덴의 남은 시간을 크게 보여 줌
// - 화면에서 읽은 남은 시간을 받은 뒤로는 패널이 직접 줄여 가며 보여 줌 (읽기 사이에도 부드럽게, 글자를 못 읽은 번에도 이어서)
// - 아이콘이 잠깐(0.6초) 안 보이면 끝난 것으로 봄 → 흐리게 "-"
// - 3초 이하로 남으면 빨간 글자 (다시 걸 때)
// - 켜진 버프가 하나도 없으면 평소엔 그리지 않음 (조작 모드에서는 보임)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 버프 타이머 패널
    class BuffPanel final : public OverlayPanel
    {
    public:
        static constexpr std::string_view PANEL_ID{ "buff" };   // 패널 이름

        explicit BuffPanel(std::function<std::optional<Vision::BuffSample>()> provider);   // provider: 마지막 버프 · 디버프

        void Update(double deltaSeconds) override;                          // 값 받기 · 남은 시간 따라가기
        [[nodiscard]] bool IsContentAvailable() const noexcept override;    // 켜진 버프가 있을 때만

    protected:
        void RenderContent(Core::Renderer& renderer, const Core::RectF& content) override;   // 칸 6개

    private:
        // 종류 하나의 상태
        struct BuffTrack
        {
            std::optional<double> m_seconds;                        // 마지막으로 읽은 남은 시간 (초)
            std::chrono::steady_clock::time_point m_readAt;         // 그 시각
            std::chrono::steady_clock::time_point m_seenAt;         // 마지막으로 아이콘을 본 시각
            bool m_isActive{};                                      // 지금 걸려 있는지
        };

        [[nodiscard]] std::optional<double> GetRemaining(const BuffTrack& track) const;   // 지금 남은 시간 (모르면 nullopt)

        std::function<std::optional<Vision::BuffSample>()> m_provider;   // 값 함수
        std::array<BuffTrack, Vision::BUFF_KIND_COUNT> m_tracks{};       // 종류별 상태
        std::array<Core::ImageData, Vision::BUFF_KIND_COUNT> m_icons{};  // 종류별 아이콘 그림 (템플릿에서 만듦)
        std::chrono::steady_clock::time_point m_lastSampleAt;            // 마지막으로 반영한 읽기 시각 (같은 값을 두 번 반영하지 않게)
        double m_refreshElapsed{};                                       // 마지막으로 받은 뒤 지난 시간
    };
}
