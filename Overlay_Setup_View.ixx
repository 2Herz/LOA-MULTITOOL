export module LostArkMultiTool.Views.OverlaySetupView;  // 오버레이 · 인식 설정 화면 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Platform;  // 플랫폼 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.CalculatorManager;  // 현재 캐릭터 · 화면 인식 상태 형식 가져오기
import LostArkMultiTool.Views.OverlayManager;  // 오버레이 요소 형식 가져오기
import LostArkMultiTool.Views.UI.Button;  // 버튼 부품 가져오기
import LostArkMultiTool.Views.UI.CheckBox;  // 체크 상자 부품 가져오기
import LostArkMultiTool.Views.UI.ComboBox;  // 펼침 목록 부품 가져오기
import LostArkMultiTool.Views.UI.ControlHost;  // 부품 호스트 가져오기
import LostArkMultiTool.Views.UI.Slider;  // 값 막대 부품 가져오기

// 오버레이 · 인식 설정 탭 화면 (세 쪽)
// - 현재 캐릭터: 자동 인식 켜기 · 현재 캐릭터 정보 · 직접 고르기 · 정보 갱신 · 기억한 이름표/글자 초기화
// - UI 구성: 오버레이 켜기 · 잠금/편집(조작 모드) · 패널 바탕 진하기 · 요소별 표시 · 투명도 · 기본 위치로
// - 인식 설정: 인식 화면(미리 보기 + 읽는 곳 상자) · 인식 정보 · 화면 인식 켜기 · 다시 찾기 · 캐릭터 확인 간격
// 부품 배치 · 표시만 알고, 실제 동작은 콜백으로 OverlaySetupManager가 맡음
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 설정 쪽
    enum class SetupPage : std::size_t
    {
        PAGE_CHARACTER,   // 현재 캐릭터
        PAGE_LAYOUT,      // UI 구성
        PAGE_VISION,      // 인식 설정
        PAGE_COUNT        // 쪽 수 (끝 표식)
    };

    // 화면에 보여 줄 상태 (매니저가 주기적으로 모아 넘김)
    struct OverlaySetupSnapshot
    {
        CurrentCharacterSummary m_character;              // 현재 캐릭터
        std::size_t m_rosterCount{};                      // 원정대 캐릭터 수
        bool m_isOverlayEnabled{};                        // 오버레이 켜짐
        bool m_isCaptureHidden{};                         // 캡처 · 방송에서 숨기기
        bool m_isEditing{};                               // 편집(조작) 모드
        bool m_isGameFound{};                             // 게임 창을 찾음
        float m_backOpacity{};                            // 패널 바탕 진하기 (0.3 ~ 1)
        std::vector<OverlayElementInfo> m_elements;       // 요소들
        bool m_isVisionEnabled{};                         // 화면 인식 켜짐
        std::optional<VisionStatusSnapshot> m_vision;     // 화면 인식 상태 (꺼져 있으면 nullopt)
        int m_characterIntervalSeconds{};                 // 캐릭터 확인 간격 (초)
    };

    // 사용자 동작 알림 (매니저가 연결)
    struct OverlaySetupCallbacks
    {
        std::function<void(SetupPage)> m_onPageChanged;                          // 쪽 바뀜
        std::function<void(bool)> m_onAutoChanged;                                // 자동 인식 켜기 · 끄기
        std::function<void(std::size_t)> m_onPickCharacter;                       // 직접 고른 캐릭터 (후보 번호)
        std::function<void()> m_onRefreshCharacter;                               // 정보 갱신
        std::function<void()> m_onClearShapes;                                    // 이름표 모양 초기화
        std::function<void()> m_onClearAliases;                                   // 읽은 글자 초기화
        std::function<void(bool)> m_onOverlayEnabled;                             // 오버레이 켜기 · 끄기
        std::function<void(bool)> m_onCaptureHidden;                              // 캡처 · 방송에서 숨기기
        std::function<void(bool)> m_onEditing;                                    // 편집 · 잠금
        std::function<void(std::string_view, bool)> m_onElementVisible;           // 요소 표시
        std::function<void(std::string_view, float, bool)> m_onElementAlpha;      // 요소 투명도 (끝이면 true)
        std::function<void(std::string_view)> m_onElementReset;                   // 기본 위치로
        std::function<void(float, bool)> m_onBackOpacity;                         // 바탕 진하기 (끝이면 true)
        std::function<void(bool)> m_onVisionEnabled;                              // 화면 인식 켜기 · 끄기
        std::function<void()> m_onRestartVision;                                  // 다시 찾기
        std::function<void(int)> m_onIntervalChanged;                             // 캐릭터 확인 간격 (초)
    };

    // 설정 화면
    class OverlaySetupView
    {
    public:
        static constexpr std::size_t ELEMENT_COUNT{ 5 };   // 설정할 요소 수 (현재 캐릭터 · 보스 체력 · 재료 · 낙원력 · 미니 탭)

        explicit OverlaySetupView(Core::Platform& platform);   // 생성자: 부품을 만들어 호스트에 추가
        ~OverlaySetupView();                                    // 소멸자

        OverlaySetupView(const OverlaySetupView&) = delete;              // 복사 금지 (부품 참조 보유)
        OverlaySetupView& operator=(const OverlaySetupView&) = delete;   // 복사 대입 금지

        void SetCallbacks(OverlaySetupCallbacks callbacks);     // 동작 알림 연결
        void Show(const OverlaySetupSnapshot& snapshot);        // 상태 반영 (부품 값 맞추기, 알림 없음)
        [[nodiscard]] SetupPage GetPage() const noexcept;       // 지금 쪽

        void Update(double deltaSeconds);                                         // 부품 갱신
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);   // 그리기
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);           // 입력
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;              // 커서
        void SuspendFocus();                                                      // 포커스 잠시 놓기
        void RestoreFocus();                                                      // 포커스 되돌리기

    private:
        // 요소 한 줄의 부품
        struct ElementRow
        {
            UI::CheckBox* m_visibleBox{};    // 표시
            UI::Slider* m_alphaSlider{};     // 투명도
            UI::Button* m_resetButton{};     // 기본 위치로
            Core::RectF m_textBounds;        // 이름 · 설명 자리
            Core::RectF m_percentBounds;     // % 글자 자리
        };

        void SelectPage(SetupPage page);                                 // 쪽 바꾸기 (부품 보이기 · 숨기기)
        void Layout(const Core::RectF& contentArea);                     // 부품 배치 (쪽마다)
        void LayoutCharacter(const Core::RectF& area);                   // 현재 캐릭터 쪽
        void LayoutLayout(const Core::RectF& area);                      // UI 구성 쪽
        void LayoutVision(const Core::RectF& area);                      // 인식 설정 쪽
        void RenderCharacter(Core::Renderer& renderer) const;            // 현재 캐릭터 쪽 글자 · 카드
        void RenderLayout(Core::Renderer& renderer) const;               // UI 구성 쪽 글자 · 카드
        void RenderVision(Core::Renderer& renderer) const;               // 인식 설정 쪽 글자 · 카드 · 미리 보기
        void RenderPreview(Core::Renderer& renderer, const Core::RectF& area) const;   // 미리 보기 그림 + 읽는 곳 상자
        [[nodiscard]] const OverlayElementInfo* FindElement(std::size_t index) const noexcept;   // 요소 정보 (없으면 nullptr)

        UI::ControlHost m_host;                                           // 부품 호스트
        OverlaySetupCallbacks m_callbacks;                                // 동작 알림
        OverlaySetupSnapshot m_snapshot;                                  // 마지막 상태
        SetupPage m_page{ SetupPage::PAGE_CHARACTER };                    // 지금 쪽
        std::array<UI::Button*, static_cast<std::size_t>(SetupPage::PAGE_COUNT)> m_pageButtons{};   // 쪽 단추

        // 현재 캐릭터 쪽
        UI::CheckBox& m_autoBox;              // 자동 인식
        UI::ComboBox& m_characterBox;         // 직접 고르기 후보
        UI::Button& m_pickButton;             // 이 캐릭터로 지정
        UI::Button& m_refreshButton;          // 정보 갱신
        UI::Button& m_clearShapesButton;      // 이름표 모양 초기화
        UI::Button& m_clearAliasesButton;     // 읽은 글자 초기화
        std::vector<std::wstring> m_candidateItems;   // 후보 목록 (바뀔 때만 다시 넣음)

        // UI 구성 쪽
        UI::CheckBox& m_overlayBox;           // 오버레이 켜기
        UI::CheckBox& m_captureBox;           // 캡처 · 방송에서 숨기기
        UI::Button& m_lockButton;             // 잠금
        UI::Button& m_editButton;             // 편집
        UI::Slider& m_backSlider;             // 바탕 진하기
        std::array<ElementRow, ELEMENT_COUNT> m_elementRows{};   // 요소 줄

        // 인식 설정 쪽
        UI::CheckBox& m_visionBox;            // 화면 인식 켜기
        UI::Button& m_restartButton;          // 다시 찾기
        UI::ComboBox& m_intervalBox;          // 캐릭터 확인 간격

        // 배치 결과 (그리기용)
        Core::RectF m_pageBarBounds;          // 쪽 단추 줄
        Core::RectF m_firstCard;              // 첫 카드
        Core::RectF m_secondCard;             // 둘째 카드
        Core::RectF m_firstContent;           // 첫 카드 본문 (머리말 아래)
        Core::RectF m_secondContent;          // 둘째 카드 본문
        Core::RectF m_infoBounds;             // 정보 줄 영역
        Core::RectF m_descriptionBounds;      // 설명 글 영역
        Core::RectF m_pickLabelBounds;        // "직접 고르기" 글자
        Core::RectF m_shapeTextBounds;        // 이름표 모양 설명
        Core::RectF m_aliasTextBounds;        // 읽은 글자 설명
        Core::RectF m_editLabelBounds;        // "끌어 옮기기" 글자
        Core::RectF m_editStatusBounds;       // 편집 상태 글자
        Core::RectF m_backLabelBounds;        // "패널 바탕 진하기" 글자
        Core::RectF m_backPercentBounds;      // 바탕 % 글자
        Core::RectF m_previewBounds;          // 미리 보기 영역
        Core::RectF m_legendBounds;           // 상자 색 설명
        Core::RectF m_intervalLabelBounds;    // "캐릭터 확인 간격" 글자
    };
}
