export module LostArkMultiTool.Views.UI.Dialog;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.ControlHost;

export namespace LostArkMultiTool::Views::UI
{
    // 모달 대화상자의 공통 틀 (화면 어둡게 + 가운데 패널 + 제목 + 내용 부품)
    // 내용 배치 · Enter/Esc 동작은 상속받은 대화상자가 정함 (Template Method 패턴)
    class Dialog
    {
    public:
        virtual ~Dialog();

        Dialog(const Dialog&) = delete;
        Dialog& operator=(const Dialog&) = delete;

        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& screenArea);
        bool HandleInput(const Core::InputEvent& event);

        void Close();   // 닫기 요청 (실제 삭제는 OverlayNotifier가 다음 Update에서 → 버튼 콜백 도중 자신이 사라지지 않게)
        [[nodiscard]] bool IsClosed() const noexcept;

        void Activate();     // 맨 위 대화상자가 됨 (처음 열림 · 위의 대화상자가 닫힘)
        void Deactivate();   // 위에 다른 대화상자가 열림

        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;

    protected:
        Dialog(std::wstring title, Core::NoticeKind kind, float width);

        [[nodiscard]] ControlHost& GetHost() noexcept;

        // ===== 상속받은 대화상자가 구현 =====
        // 내용 높이 (패널 크기를 정할 때, contentWidth = 패널 안쪽 너비)
        [[nodiscard]] virtual float MeasureContentHeight(Core::Renderer& renderer, float contentWidth) = 0;
        // 내용 부품 배치 (contentArea = 제목 아래 안쪽 영역)
        virtual void LayoutContent(const Core::RectF& contentArea) = 0;
        // 처음 포커스를 받을 부품 고르기 (기본: 첫 부품)
        virtual void FocusInitialControl();
        // Enter (포커스 부품이 처리하지 않았을 때) · Esc
        virtual void OnSubmit();
        virtual void OnCancel();

    private:
        void UpdateLayout(Core::Renderer& renderer, const Core::RectF& screenArea);
        [[nodiscard]] float GetOpacity() const noexcept;

        std::wstring m_title;
        Core::NoticeKind m_kind;
        float m_width{};
        ControlHost m_host;
        Core::RectF m_panelRect;
        Core::RectF m_layoutScreen;   // 마지막으로 배치한 화면 크기 (바뀌면 다시 배치)
        bool m_hasLayout{};
        bool m_isClosed{};
        bool m_isActivated{};
        double m_ageSeconds{};        // 열린 뒤 지난 시간 (서서히 나타나기)
    };
}