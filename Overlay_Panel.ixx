export module LostArkMultiTool.Views.OverlayPanel;  // 인게임 오버레이 패널 기반 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기

// 인게임 오버레이에 띄우는 패널 하나 (컴포넌트 패턴: 오버레이 매니저가 패널들을 모아 그림)
// 공통: 반투명 카드 + 머리말(제목) / 조작 모드에서만 머리말 끌어 옮기기 · 닫기(×) · 크기 조절(오른쪽 아래)
// 내용은 파생 패널이 RenderContent · HandleContentInput으로 채움 (템플릿 메서드 패턴)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 패널 기반 (추상)
    class OverlayPanel
    {
    public:
        OverlayPanel(std::string id, std::wstring title, Core::SizeF defaultSize, bool isResizable);   // 이름 · 제목 · 기본 크기 · 크기 조절 여부
        virtual ~OverlayPanel();                                                                        // 가상 소멸자

        OverlayPanel(const OverlayPanel&) = delete;              // 복사 금지
        OverlayPanel& operator=(const OverlayPanel&) = delete;   // 복사 대입 금지

        [[nodiscard]] const std::string& GetId() const noexcept;      // 패널 이름 (설정 저장 키)
        [[nodiscard]] const Core::RectF& GetBounds() const noexcept;  // 위치 · 크기 (오버레이 기준 DIP)
        void SetBounds(const Core::RectF& bounds) noexcept;          // 위치 · 크기 바꾸기
        [[nodiscard]] bool IsVisible() const noexcept;                // 보이는지
        void SetVisible(bool isVisible) noexcept;                     // 보이기 · 숨기기
        [[nodiscard]] Core::SizeF GetDefaultSize() const noexcept;    // 기본 크기
        void ClampInto(const Core::RectF& area) noexcept;             // 오버레이 영역 밖으로 나가지 않게

        virtual void Update(double deltaSeconds);                                                 // 프레임 갱신 (기본: 할 일 없음)
        void Render(Core::Renderer& renderer, float backOpacity, bool isInteractive);             // 카드 · 머리말 · 내용 그리기
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event, bool isInteractive);        // 입력 (처리했으면 true)
        [[nodiscard]] virtual Core::CursorShape GetCursor() const noexcept;                      // 마우스 커서 모양
        [[nodiscard]] bool IsChanged() noexcept;                                                  // 사용자가 옮기거나 닫았는지 (확인하면 초기화 → 설정 저장 신호)

    protected:
        virtual void RenderContent(Core::Renderer& renderer, const Core::RectF& content) = 0;   // 내용 그리기
        [[nodiscard]] virtual bool HandleContentInput(const Core::InputEvent& event, const Core::RectF& content);   // 내용 입력 (기본: 안 받음)
        [[nodiscard]] virtual std::wstring GetTitleText() const;                                // 머리말 글자 (기본: 제목)
        [[nodiscard]] Core::RectF GetContentRect() const noexcept;                              // 머리말을 뺀 내용 영역
        void SetHeight(float height) noexcept;                                                  // 내용에 맞춰 높이 바꾸기 (크기 조절 안 되는 패널)

    private:
        [[nodiscard]] Core::RectF GetHeaderRect() const noexcept;   // 머리말 영역
        [[nodiscard]] Core::RectF GetCloseRect() const noexcept;    // 닫기 단추 영역
        [[nodiscard]] Core::RectF GetGripRect() const noexcept;     // 크기 조절 손잡이 영역

        std::string m_id;                 // 패널 이름
        std::wstring m_title;             // 제목
        Core::SizeF m_defaultSize;        // 기본 크기
        bool m_isResizable{};             // 크기 조절 되는지
        Core::RectF m_bounds;             // 위치 · 크기
        bool m_isVisible{ true };         // 보이는지
        bool m_isDragging{};              // 머리말 끄는 중
        bool m_isResizing{};              // 크기 조절 중
        bool m_isHoverClose{};            // 닫기 위에 마우스
        bool m_isHoverHeader{};           // 머리말 위에 마우스
        bool m_isChanged{};               // 옮김 · 닫음 · 크기 바뀜
        Core::PointF m_dragOffset;        // 끌기 시작점과 패널 왼쪽 위의 차이
    };
}
