export module LostArkMultiTool.Core.ToolManager;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.ToolContext;

export namespace LostArkMultiTool::Core
{
    // 최상위 매니저 인터페이스 (귀속재료 계산기 · 보석 최저가 검색기 · 낙원력 트래커 매니저가 각각 구현)
    // Application(Main)은 이 인터페이스만 보고 탭을 전환하고 매 프레임 갱신 · 그리기 · 입력 전달을 맡김
    class ToolManager
    {
    public:
        virtual ~ToolManager();

        // 탭에 표시할 이름
        [[nodiscard]] virtual std::wstring_view GetTabTitle() const noexcept = 0;

        virtual void Initialize(ToolContext& context) = 0;   // 프로그램 시작 때 한 번 (공용 서비스 받기 · 모델 불러오기)
        virtual void Shutdown() = 0;                          // 프로그램 종료 때 한 번 (저장 등)

        virtual void OnActivated() = 0;     // 이 탭이 입력을 받기 시작할 때 (탭 선택 · 대화상자가 닫힘)
        virtual void OnDeactivated() = 0;   // 입력을 넘겨줄 때 (다른 탭 선택 · 대화상자가 열림)

        // 매 프레임 상태 갱신 (모든 탭 → 백그라운드 작업 결과 · 자동 갱신은 다른 탭을 보거나 트레이에 숨어 있어도 진행)
        virtual void Update(double deltaSeconds) = 0;
        virtual void Render(Renderer& renderer, const RectF& contentArea) = 0;   // 탭 아래 내용 영역에 그리기

        // 입력 처리 (처리했으면 true), 마우스를 올린 곳에 맞는 커서 모양
        [[nodiscard]] virtual bool HandleInput(const InputEvent& event) = 0;
        [[nodiscard]] virtual CursorShape GetCursor() const noexcept = 0;

        // 창이 트레이로 숨을 때 알려 줄 백그라운드 작업 안내 (예: "5분마다 시세를 자동으로 갱신해요.", 없으면 nullopt)
        [[nodiscard]] virtual std::optional<std::wstring> GetBackgroundStatus() const;

        // API 키를 새로 저장했을 때 (키 오류로 멈춘 자동 갱신을 다시 켜는 등, 기본: 아무것도 안 함)
        virtual void OnAPIKeyChanged();
    };
}