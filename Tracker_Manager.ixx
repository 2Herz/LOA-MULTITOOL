export module LostArkMultiTool.Views.TrackerManager;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Core.Singleton;

export namespace LostArkMultiTool::Views
{
    // 낙원력 트래커 최상위 매니저 (이 툴의 화면구성 · 렌더링 · 모델을 묶어서 관리)
    // ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
    class TrackerManager final : public Core::ToolManager, public Core::Singleton<TrackerManager>
    {
    public:
        [[nodiscard]] std::wstring_view GetTabTitle() const noexcept override;

        void Initialize() override;
        void Shutdown() override;

        void OnActivated() override;
        void OnDeactivated() override;

        void Update(double deltaSeconds) override;
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea) override;

    private:
        friend class Core::Singleton<TrackerManager>;   // GetInstance만 생성 · 소멸할 수 있게

        TrackerManager();
        ~TrackerManager() override;
    };
}