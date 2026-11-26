export module LostArkMultiTool.Views.CharacterPanel;  // 오버레이 현재 캐릭터 패널 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.CalculatorManager;  // 현재 캐릭터 요약 형식 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기

// 오버레이 현재 캐릭터 패널: "현재 캐릭터 : 이름 · 직업 · 레벨" + 알아본 방법 · 경고
// - 조작 모드(Ctrl+Shift+I)에서 [변경]을 누르면 원정대 목록에서 직접 고름 (화면 인식 값은 이 캐릭터에 들어감)
// - 캐릭터 정보 창에서 읽은 아이템 레벨이 저장된 값과 다르면 경고 + [갱신] (원정대 정보 다시 받기)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 현재 캐릭터 패널
    class CharacterPanel final : public OverlayPanel
    {
    public:
        static constexpr std::string_view PANEL_ID{ "character" };   // 패널 이름

        // provider: 요약 함수 · onPick: [변경]으로 고른 후보 번호 · onRefresh: [갱신]
        CharacterPanel(std::function<CurrentCharacterSummary()> provider, std::function<void(std::size_t index)> onPick, std::function<void()> onRefresh);

        void Update(double deltaSeconds) override;   // 요약 다시 받기 · 높이 맞추기

    protected:
        void RenderContent(Core::Renderer& renderer, const Core::RectF& content) override;                           // 이름 · 상태 · 목록 그리기
        [[nodiscard]] bool HandleContentInput(const Core::InputEvent& event, const Core::RectF& content) override;   // [변경] · [갱신] · 목록 고르기
        [[nodiscard]] std::wstring GetTitleText() const override;                                                   // "현재 캐릭터" · "현재 캐릭터 · 고르기"

    private:
        [[nodiscard]] Core::RectF GetChangeRect(const Core::RectF& content) const noexcept;    // [변경] · [취소] 단추 영역
        [[nodiscard]] Core::RectF GetRefreshRect(const Core::RectF& content) const noexcept;   // [갱신] 단추 영역
        [[nodiscard]] Core::RectF GetRowRect(const Core::RectF& content, std::size_t row) const noexcept;   // 고르기 목록 한 줄 (0 = 첫 후보 줄)
        [[nodiscard]] std::size_t GetVisibleRowCount() const noexcept;                         // 고르기 목록에 보일 줄 수
        [[nodiscard]] bool CanRefresh() const noexcept;                                        // [갱신]을 보일 때인지
        void UpdateHeight();                                                                   // 내용에 맞춘 높이

        std::function<CurrentCharacterSummary()> m_provider;   // 요약 함수 (계산기 매니저)
        std::function<void(std::size_t)> m_onPick;              // [변경]으로 골랐을 때
        std::function<void()> m_onRefresh;                      // [갱신]
        CurrentCharacterSummary m_summary;                      // 마지막 요약
        double m_refreshElapsed{};                              // 마지막으로 받은 뒤 지난 시간
        bool m_hasSummary{};                                    // 한 번이라도 받았는지
        bool m_isPicking{};                                     // 캐릭터 고르는 중
        std::size_t m_scrollIndex{};                            // 고르기 목록 맨 위 후보 번호
        std::optional<std::size_t> m_hoverRow;                  // 마우스를 올린 후보 번호
        bool m_isHoverChange{};                                 // [변경] 위에 마우스
        bool m_isHoverRefresh{};                                // [갱신] 위에 마우스
    };
}
