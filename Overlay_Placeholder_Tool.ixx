export module LostArkMultiTool.Views.OverlayPlaceholderTool;  // "오버레이에서 사용 중" 안내 탭 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.ToolContext;  // 공용 서비스 가져오기
import LostArkMultiTool.Core.ToolManager;  // 탭 인터페이스 가져오기

// 인게임 오버레이 미니 모드로 띄운 탭을 메인 창에서 대신하는 빈 탭 (Null Object 패턴)
// 같은 탭을 두 창이 동시에 그리면 배치 · 입력 좌표가 섞이므로, 메인 창에는 안내만 그리고 입력은 받지 않음
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 안내 탭
    class OverlayPlaceholderTool final : public Core::ToolManager
    {
    public:
        [[nodiscard]] std::wstring_view GetTabTitle() const noexcept override;   // 제목 (쓰이지 않음)
        void Initialize(Core::ToolContext& context) override;                    // 할 일 없음
        void Shutdown() override;                                                // 할 일 없음
        void OnActivated() override;                                             // 할 일 없음
        void OnDeactivated() override;                                           // 할 일 없음
        void Update(double deltaSeconds) override;                               // 할 일 없음
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea) override;   // 안내 글자
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event) override;  // 입력 받지 않음
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;     // 기본 커서
    };
}
