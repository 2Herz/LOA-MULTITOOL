export module LostArkMultiTool.Views.OverlayManager;  // 인게임 오버레이 매니저 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Notifier;  // 알림 종류 가져오기
import LostArkMultiTool.Core.Renderer;  // 도형 형식 가져오기
import LostArkMultiTool.Core.Singleton;  // 싱글톤 가져오기
import LostArkMultiTool.Core.ToolManager;  // 탭 매니저 가져오기 (미니 모드)
import LostArkMultiTool.Views.DX11Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Views.OverlayPanel;  // 패널 기반 가져오기
import LostArkMultiTool.Views.OverlaySettings;  // 설정 가져오기
import LostArkMultiTool.Views.OverlayWindow;  // 오버레이 창 가져오기
import LostArkMultiTool.Views.UI.ToastHost;  // 토스트 가져오기

// 인게임 오버레이 매니저 (프로그램에 하나 → 싱글톤, 탭이 아니라 Application이 직접 부름)
// - 게임 창(로스트아크)을 찾아 그 위에 투명 창을 겹치고, 게임이 맨 앞일 때만 보임
// - 패널(재료 시세 · 총자산, 낙원력 주간 현황, 탭 미니 모드)을 컴포넌트로 모아 그림
// - 전역 단축키: Ctrl+Shift+O = 오버레이 켜기 · 끄기, Ctrl+Shift+I = 조작 모드 (패널 옮기기 · 미니 탭 사용)
// - 알림 토스트: 게임 화면 위에도 띄움
// 안전: 게임 프로세스 · 입력과 무관한 내 프로그램 창일 뿐 (평소엔 클릭이 그대로 게임으로 통과)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 설정 탭에 보여 줄 요소(패널) 하나
    struct OverlayElementInfo
    {
        std::string m_id;          // 패널 이름 ("character" 등)
        std::wstring m_title;      // 표시 이름
        bool m_isVisible{};        // 보이는지
        float m_alpha{ 1.0f };     // 요소 투명도 (0.1 ~ 1)
        Core::RectF m_bounds;      // 지금 위치 · 크기 (오버레이 기준 DIP, 아직 안 놓였으면 0)
    };

    // 오버레이 매니저
    class OverlayManager final : public Core::Singleton<OverlayManager>
    {
    public:
        // tools: 미니 모드에서 그릴 탭들 (Application의 탭 순서, 매니저는 싱글톤이 소유)
        void Initialize(std::span<const std::reference_wrapper<Core::ToolManager>> tools);
        void Shutdown();                                         // 창 · 렌더러 정리 (설정 저장)

        void Update(double deltaSeconds);                        // 게임 창 따라가기 · 패널 갱신
        void Render();                                           // 오버레이 그리기 (보일 때만, 너무 자주 그리지 않음)

        [[nodiscard]] bool IsEnabled() const noexcept;           // 오버레이를 켰는지
        void SetEnabled(bool isEnabled);                         // 켜기 · 끄기 (설정 저장)
        [[nodiscard]] bool IsVisible() const noexcept;           // 지금 게임 위에 보이는지
        [[nodiscard]] std::optional<std::size_t> GetMiniToolIndex() const noexcept;   // 미니 모드로 띄운 탭 (안 띄웠으면 nullopt)

        [[nodiscard]] bool ShowToast(const std::wstring& message, Core::NoticeKind kind);   // 게임 위 토스트 (보일 때만, 띄웠으면 true)
        [[nodiscard]] std::optional<std::wstring> TakeNotice();                             // 메인 창에 보여 줄 안내 (단축키 등록 실패 등)
        void SetOnEnabledChanged(std::function<void(bool isEnabled)> onEnabledChanged);     // 켜짐이 바뀌면 알림 (메뉴 버튼 글자)
        void SetOnRefreshHotkey(std::function<void()> onRefreshHotkey);                     // Ctrl+Shift+F5 (시세 갱신) 알림

        // ===== 설정 탭 (UI 구성) =====
        [[nodiscard]] std::vector<OverlayElementInfo> GetElements() const;                  // 요소 목록 (그리는 순서)
        void SetElementVisible(std::string_view id, bool isVisible);                        // 요소 보이기 · 숨기기 (저장)
        void SetElementAlpha(std::string_view id, float alpha, bool isCommit);              // 요소 투명도 (isCommit이면 저장)
        void ResetElementPosition(std::string_view id);                                     // 기본 위치로 (저장)
        [[nodiscard]] float GetBackOpacity() const noexcept;                                // 패널 바탕 진하기
        void SetBackOpacity(float opacity, bool isCommit);                                  // 바탕 진하기 (isCommit이면 저장)
        [[nodiscard]] bool IsEditing() const noexcept;                                      // 조작(편집) 모드인지
        [[nodiscard]] bool SetEditing(bool isEditing);                                      // 편집 켜기 · 끄기 (게임이 없거나 오버레이가 꺼져 있으면 false)
        [[nodiscard]] bool IsGameFound() const noexcept;                                    // 게임 창을 찾았는지
        [[nodiscard]] bool IsCaptureHidden() const noexcept;                                // 캡처 · 방송에서 숨기기
        void SetCaptureHidden(bool isHidden);                                               // 숨기기 바꾸기 (저장)

    private:
        friend class Core::Singleton<OverlayManager>;   // GetInstance만 생성 · 소멸

        OverlayManager();             // 생성자
        ~OverlayManager();            // 소멸자

        void HandleInput(const Core::InputEvent& event);   // 오버레이 창 입력
        void HandleHotkey(int hotkeyId);                    // 전역 단축키
        void SetInteractive(bool isInteractive);            // 조작 모드 켜기 · 끄기
        void TrackGameWindow();                              // 게임 창 찾기 · 위치 맞추기 · 보이기 결정
        void LoadPanelStates();                              // 설정 → 패널 위치 · 표시
        void SavePanelStates();                              // 패널 위치 · 표시 → 설정 저장
        void SaveSettings();                                 // 설정 파일 저장 (실패하면 안내)
        void PlaceDefaultPanels();                           // 저장된 위치가 없는 패널을 오른쪽 위부터 배치
        void RenderToolbar(Core::Renderer& renderer);        // 조작 모드 위쪽 도구 막대
        [[nodiscard]] bool HandleToolbarInput(const Core::InputEvent& event);   // 도구 막대 입력
        [[nodiscard]] Core::RectF GetOverlayArea() const;    // 오버레이 전체 영역 (DIP)
        [[nodiscard]] OverlayPanel* FindPanel(std::string_view id) const;   // 이름으로 패널 찾기

        // 도구 막대 단추 하나
        struct ToolbarButton
        {
            std::wstring m_text;                   // 글자
            std::function<void()> m_onClick;       // 눌렀을 때
            std::function<bool()> m_isActive;      // 켜진 상태 표시 (없으면 일반 단추)
            Core::RectF m_bounds;                  // 위치 (그릴 때 정함)
        };

        std::unique_ptr<OverlaySettings> m_settings;              // 설정
        std::unique_ptr<OverlayWindow> m_window;                  // 투명 창
        std::unique_ptr<DX11Renderer> m_renderer;                 // 투명 렌더러
        std::vector<std::unique_ptr<OverlayPanel>> m_panels;      // 패널들 (뒤가 위에 그려짐)
        std::vector<ToolbarButton> m_toolbar;                     // 조작 모드 도구 막대
        UI::ToastHost m_toasts;                                   // 게임 위 토스트
        std::vector<std::reference_wrapper<Core::ToolManager>> m_tools;   // 미니 모드 탭들
        std::function<void(bool)> m_onEnabledChanged;            // 켜짐 알림
        std::function<void()> m_onRefreshHotkey;                  // 시세 갱신 단축키 알림
        std::optional<std::wstring> m_pendingNotice;              // 메인 창 안내
        void* m_gameWindow{};                                     // 따라가는 게임 창
        double m_findElapsed{};                                   // 게임 창을 다시 찾은 뒤 지난 시간
        double m_renderElapsed{};                                 // 마지막으로 그린 뒤 지난 시간
        bool m_isVisible{};                                       // 지금 보이는지
        bool m_isDirty{ true };                                   // 바로 다시 그려야 하는지
        bool m_isPlaced{};                                        // 패널 기본 위치를 잡았는지
        Core::CursorShape m_cursor{ Core::CursorShape::CURSOR_ARROW };   // 커서
    };
}
