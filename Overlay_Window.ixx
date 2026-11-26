export module LostArkMultiTool.Views.OverlayWindow;  // 인게임 오버레이 창 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기

// 게임 화면 위에 띄우는 투명 최상위 창 (게임 프로세스와는 무관한 내 프로그램의 창)
// - 평소: 클릭 통과 + 포커스를 가져가지 않음 → 게임 조작에 아무 영향 없음
// - 조작 모드: 마우스 · 키보드를 받음 (패널 옮기기 · 미니 탭 사용), 다시 끄면 게임으로 포커스를 돌려줌
// - 메시지는 메인 창과 같은 스레드 · 같은 메시지 루프에서 처리됨 (PeekMessage가 스레드의 모든 창을 처리)
export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 오버레이 창 (pimpl: Win32 형식을 모듈 밖에 숨김)
    class OverlayWindow
    {
    public:
        OverlayWindow();    // 숨은 상태로 창 만들기
        ~OverlayWindow();   // 창 없애기 · 단축키 해제

        OverlayWindow(const OverlayWindow&) = delete;              // 복사 금지
        OverlayWindow& operator=(const OverlayWindow&) = delete;   // 복사 대입 금지

        [[nodiscard]] void* GetNativeHandle() const noexcept;      // 창 핸들 (렌더러 연결용)
        [[nodiscard]] float GetDPI() const noexcept;               // 창이 놓인 모니터 배율 (DPI)

        void SetOnInput(std::function<void(const Core::InputEvent&)> onInput);   // 입력 알림 (DIP 좌표)
        void SetOnHotkey(std::function<void(int hotkeyId)> onHotkey);           // 전역 단축키 알림
        void SetOnResize(std::function<void(int pixelWidth, int pixelHeight)> onResize);   // 크기 바뀜 알림
        void SetOnDPIChanged(std::function<void(float dpi)> onDPIChanged);       // 배율 바뀜 알림

        // 전역 단축키 등록 (modifiers: Ctrl · Shift 등 MOD_ 값, 실패하면 false → 다른 프로그램이 이미 씀)
        [[nodiscard]] bool RegisterHotkey(int hotkeyId, unsigned modifiers, unsigned virtualKey);

        void Place(const Vision::PixelRect& screenRect);   // 게임 화면 위치 · 크기에 맞춤 (화면 좌표, 물리 픽셀)
        void Show();                                       // 보이기 (포커스는 가져가지 않음)
        void Hide();                                       // 숨기기
        [[nodiscard]] bool IsShown() const noexcept;       // 보이는지

        void SetInteractive(bool isInteractive);           // 조작 모드 켜기 · 끄기
        [[nodiscard]] bool IsInteractive() const noexcept; // 조작 모드인지
        [[nodiscard]] bool IsForeground() const noexcept;  // 오버레이가 맨 앞 창인지 (조작 중)
        void Activate();                                   // 오버레이를 맨 앞으로 (조작 모드에서 키보드 입력용)
        static void ActivateWindow(void* windowHandle);    // 다른 창(게임)을 맨 앞으로 (조작 모드를 끌 때)

        void SetCursor(Core::CursorShape shape) noexcept;  // 마우스 커서 모양

    private:
        class Impl;                     // 구현 (Win32)
        std::unique_ptr<Impl> m_impl;   // 구현 소유
    };
}
