export module LostArkMultiTool.Views.ParadisePanel;  // 오버레이 낙원력 주간 현황 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.TrackerManager;  // 트래커 요약 형식 가져오기

// 오버레이 낙원력 패널: 이번 주차 캐릭터별 낙원력 (기록 높은 순, 미기록 캐릭터는 강조)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 낙원력 패널
    class ParadisePanel final : public OverlayPanel
    {
    public:
        static constexpr std::string_view PANEL_ID{ "paradise" };   // 패널 이름

        explicit ParadisePanel(std::function<TrackerOverlaySummary()> provider);   // 요약을 받아 올 함수

        void Update(double deltaSeconds) override;   // 요약 다시 받기 (주기적으로)

    protected:
        void RenderContent(Core::Renderer& renderer, const Core::RectF& content) override;   // 목록 그리기
        [[nodiscard]] std::wstring GetTitleText() const override;                           // "낙원력 · N주차 (k/n)"

    private:
        std::function<TrackerOverlaySummary()> m_provider;   // 요약 함수 (트래커 매니저)
        TrackerOverlaySummary m_summary;                     // 마지막 요약
        double m_refreshElapsed{};                           // 마지막으로 받은 뒤 지난 시간
        bool m_hasSummary{};                                 // 한 번이라도 받았는지
    };
}
