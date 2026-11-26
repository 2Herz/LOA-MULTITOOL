export module LostArkMultiTool.Views.MiniToolPanel;  // 오버레이 탭 미니 모드 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.ToolManager;  // 탭 매니저 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기

// 오버레이 미니 모드: 멀티툴의 탭 하나를 게임 위 패널 안에 그대로 그림 (◀ ▶로 탭 바꾸기, 크기 조절 가능)
// 탭 매니저는 모두 Render(renderer, 영역) · HandleInput을 갖고 있어서, 오버레이 렌더러와 패널 영역만 넘기면 됨
// 같은 탭을 메인 창과 동시에 그리지 않도록 Application이 GetToolIndex로 확인해 메인 창에는 안내만 그림
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 미니 모드 패널
    class MiniToolPanel final : public OverlayPanel
    {
    public:
        static constexpr std::string_view PANEL_ID{ "mini" };   // 패널 이름

        // tools: 탭들 (Application 순서), initialIndex: 처음 탭, onToolChanged: 탭을 바꾸면 알림 (설정 저장)
        MiniToolPanel(std::span<const std::reference_wrapper<Core::ToolManager>> tools, std::size_t initialIndex,
            std::function<void(std::size_t index)> onToolChanged);

        [[nodiscard]] std::size_t GetToolIndex() const noexcept;                       // 지금 탭 번호
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;           // 탭 화면 커서

    protected:
        void RenderContent(Core::Renderer& renderer, const Core::RectF& content) override;   // 탭 고르기 줄 + 탭 화면
        [[nodiscard]] bool HandleContentInput(const Core::InputEvent& event, const Core::RectF& content) override;   // 탭 고르기 · 탭 입력
        [[nodiscard]] std::wstring GetTitleText() const override;                           // "미니 모드 · 탭 이름"

    private:
        void SelectTool(std::size_t index);                                        // 탭 바꾸기
        [[nodiscard]] Core::RectF GetSwitcherRect(const Core::RectF& content) const noexcept;   // 탭 고르기 줄
        [[nodiscard]] Core::RectF GetToolRect(const Core::RectF& content) const noexcept;       // 탭 화면 영역
        [[nodiscard]] Core::ToolManager& GetTool() const;                          // 지금 탭

        std::vector<std::reference_wrapper<Core::ToolManager>> m_tools;    // 탭들
        std::size_t m_toolIndex{};                                         // 지금 탭
        std::function<void(std::size_t)> m_onToolChanged;                  // 탭 바꿈 알림
        bool m_isPressedInTool{};                                          // 탭 화면 안에서 누른 채 끄는 중 (밖에서 떼도 전달)
        bool m_isHoverTool{};                                              // 마우스가 탭 화면 위
    };
}
