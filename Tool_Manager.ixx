export module LostArkMultiTool.Core.ToolManager;

import std;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Core
{
    // 최상위 매니저 인터페이스 (귀속재료 계산기 · 보석 최저가 검색기 · 낙원력 트래커 매니저가 각각 구현)
    // Application(Main)은 이 인터페이스만 보고 탭을 전환하고 매 프레임 갱신·그리기를 맡김
    class ToolManager
    {
    public:
        virtual ~ToolManager();

        // 탭에 표시할 이름
        [[nodiscard]] virtual std::wstring_view GetTabTitle() const noexcept = 0;

        virtual void Initialize() = 0;   // 프로그램 시작 때 한 번 (모델 불러오기 등)
        virtual void Shutdown() = 0;     // 프로그램 종료 때 한 번 (저장 등)

        virtual void OnActivated() = 0;     // 이 탭이 선택될 때
        virtual void OnDeactivated() = 0;   // 다른 탭으로 넘어갈 때

        virtual void Update(double deltaSeconds) = 0;                            // 매 프레임 상태 갱신 (선택된 탭만)
        virtual void Render(Renderer& renderer, const RectF& contentArea) = 0;   // 탭 아래 내용 영역에 그리기
    };
}