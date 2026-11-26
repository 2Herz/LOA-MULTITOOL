export module LostArkMultiTool.Views.OverlaySetupManager;  // 오버레이 · 인식 설정 탭 매니저 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.Singleton;  // 싱글톤 가져오기
import LostArkMultiTool.Core.ToolContext;  // 공용 서비스 가져오기
import LostArkMultiTool.Core.ToolManager;  // 탭 인터페이스 가져오기
import LostArkMultiTool.Views.OverlaySetupView;  // 설정 화면 가져오기

// 오버레이 · 인식 설정 탭 (현재 캐릭터 · UI 구성 · 인식 설정)
// ToolManager(탭으로 동작) + Singleton(프로그램에 하나만 존재) 다중 상속
// 화면 = OverlaySetupView, 실제 동작은 CalculatorManager(화면 인식 · 현재 캐릭터) · OverlayManager(오버레이 배치)에 맡기고
// 매니저는 둘의 상태를 주기적으로 모아 화면에 넘김 (중재자 패턴)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 설정 탭 매니저
    class OverlaySetupManager final : public Core::ToolManager, public Core::Singleton<OverlaySetupManager>
    {
    public:
        [[nodiscard]] std::wstring_view GetTabTitle() const noexcept override;   // 탭 제목
        void Initialize(Core::ToolContext& context) override;                    // 화면 만들기 · 동작 연결
        void Shutdown() override;                                                // 정리 (미리 보기 끔)
        void OnActivated() override;                                             // 탭이 보임 (미리 보기 켬)
        void OnDeactivated() override;                                           // 탭이 가려짐 (미리 보기 끔)
        void Update(double deltaSeconds) override;                               // 상태 모아 화면에 넘김
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea) override;   // 그리기
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event) override;  // 입력
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;     // 커서

    private:
        friend class Core::Singleton<OverlaySetupManager>;   // GetInstance만 생성 · 소멸

        OverlaySetupManager();              // 생성자
        ~OverlaySetupManager() override;    // 소멸자

        [[nodiscard]] OverlaySetupSnapshot BuildSnapshot() const;   // 두 매니저 상태 모으기
        void Refresh();                                            // 상태를 모아 화면에 넘김
        void UpdatePreview();                                      // 미리 보기 켜기 · 끄기 (보이는 탭 · 인식 설정 쪽일 때만)
        void ShowToast(std::wstring message);                      // 알림

        Core::ToolContext* m_context{};                   // Application이 소유 (Initialize ~ Shutdown 동안만 유효)
        std::unique_ptr<OverlaySetupView> m_view;         // 화면
        double m_refreshElapsed{};                        // 마지막으로 모은 뒤 지난 시간
        bool m_isActive{};                                // 탭이 보이는지
        bool m_isPreviewOn{};                             // 미리 보기를 켜 뒀는지
    };
}
