export module LostArkMultiTool.Views.UI.ProgressBar;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 끝을 알 수 없는 작업의 진행 표시 (원본 ProgressBar Marquee: 막대가 왼쪽에서 오른쪽으로 계속 흘러감)
    // 마우스를 받지 않아 아래 부품을 가리지 않음
    class ProgressBar final : public Control
    {
    public:
        ProgressBar();

        [[nodiscard]] bool IsHitTestVisible() const noexcept override;
        void Update(double deltaSeconds) override;
        void Render(Core::Renderer& renderer) override;

    private:
        double m_phase{};   // 흐르는 막대 위치 (0 ~ 1 반복)
    };
}