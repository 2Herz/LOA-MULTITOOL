export module LostArkMultiTool.Views.MaterialPanel;  // 오버레이 재료 시세 · 총자산 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.CalculatorManager;  // 계산기 요약 형식 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기

// 오버레이 재료 패널: 귀속재료 계산기에서 고른 캐릭터의 재료 수량 · 단가 · 재료값 + 캐릭터 · 원정대 자산 (읽기 전용)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 재료 패널
    class MaterialPanel final : public OverlayPanel
    {
    public:
        static constexpr std::string_view PANEL_ID{ "material" };   // 패널 이름

        explicit MaterialPanel(std::function<CalculatorOverlaySummary()> provider);   // 요약을 받아 올 함수

        void Update(double deltaSeconds) override;   // 요약 다시 받기 (주기적으로)

    protected:
        void RenderContent(Core::Renderer& renderer, const Core::RectF& content) override;   // 표 그리기
        [[nodiscard]] std::wstring GetTitleText() const override;                           // "재료 · 캐릭터"

    private:
        std::function<CalculatorOverlaySummary()> m_provider;   // 요약 함수 (계산기 매니저)
        CalculatorOverlaySummary m_summary;                     // 마지막 요약
        double m_refreshElapsed{};                              // 마지막으로 받은 뒤 지난 시간
        bool m_hasSummary{};                                    // 한 번이라도 받았는지
    };
}
