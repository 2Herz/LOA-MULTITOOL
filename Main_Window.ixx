export module LostArkMultiTool.Views.MainWindow;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;

export namespace LostArkMultiTool::Views
{
    // 트레이 아이콘 메뉴에서 고른 항목 (원본 트레이 메뉴: 열기 · API 키 변경 · 종료)
    enum class TrayCommand
    {
        TRAY_OPEN,             // 열기 (트레이 아이콘 더블 클릭도 같음)
        TRAY_CHANGE_API_KEY,   // API 키 변경
        TRAY_EXIT              // 종료
    };

    // 창에서 일어난 일을 받는 쪽 (Application이 구현)
    class WindowEventHandler
    {
    public:
        virtual ~WindowEventHandler();

        virtual void OnResize(int pixelWidth, int pixelHeight) = 0;   // 스왑체인 크기는 실제 픽셀
        virtual void OnDPIChanged(float dpi) = 0;                     // 다른 배율 모니터로 옮겨 갔을 때
        virtual void OnInput(const Core::InputEvent& event) = 0;      // 마우스 · 키보드 · 한글 입력 (좌표는 DIP)
        virtual void OnTrayCommand(TrayCommand command) = 0;          // 트레이 메뉴 · 더블 클릭
        virtual void OnHiddenToTray() = 0;                            // 최소화 → 트레이로 숨음 (풍선 알림 문구는 받는 쪽이 정함)
    };

    // 창 안쪽 크기 (실제 픽셀)
    struct PixelSize
    {
        int m_width{};
        int m_height{};
    };

    // Win32 창 (DX11 화면이 그려지는 바탕) + 운영체제 기능(Core::Platform: 클립보드 · IME · 커서 · 링크 열기 · 트레이)
    // 창 핸들이 있어야 쓸 수 있는 기능들이라 창이 함께 맡음 — Windows 타입(HWND 등)은 구현 파일 안에만 둠
    class MainWindow final : public Core::Platform
    {
    public:
        // width, height: 처음 창 안쪽 크기 (DIP, 모니터 배율에 맞춰 픽셀로 늘어남)
        MainWindow(std::wstring_view title, int width, int height, WindowEventHandler& handler);
        ~MainWindow() override;

        MainWindow(const MainWindow&) = delete;
        MainWindow& operator=(const MainWindow&) = delete;

        void Show(int showCommand);
        void RequestClose();   // 닫기 요청 (메시지 루프에서 처리 → WM_QUIT)
        void SetTitle(std::wstring_view title);   // 제목 표시줄 글자 (현재 탭 이름과 동기화)

        // 쌓인 메시지를 모두 처리 (창이 닫혀 WM_QUIT를 받으면 false)
        [[nodiscard]] bool ProcessMessages();

        // 새 메시지가 오거나 timeoutMilliseconds가 지날 때까지 대기 (트레이에 숨은 동안 CPU 사용을 줄이면서 백그라운드 작업은 확인)
        void WaitForMessage(int timeoutMilliseconds);

        [[nodiscard]] void* GetNativeHandle() const noexcept;   // HWND (렌더러가 스왑체인을 만들 때 사용)
        [[nodiscard]] float GetDPI() const noexcept;
        [[nodiscard]] PixelSize GetClientPixelSize() const;
        [[nodiscard]] bool IsMinimized() const noexcept;
        [[nodiscard]] int GetExitCode() const noexcept;

        // ===== Core::Platform =====
        [[nodiscard]] std::wstring GetClipboardText() override;
        bool SetClipboardText(std::wstring_view text) override;
        void SetCursor(Core::CursorShape shape) override;
        void SetIMEEnabled(bool isEnabled) override;
        void SetIMEPosition(const Core::RectF& caretRect) override;
        void CancelComposition() override;
        [[nodiscard]] bool OpenURL(std::wstring_view url) override;
        [[nodiscard]] bool IsWindowShown() const override;
        void RestoreWindow() override;
        void ShowTrayBalloon(std::wstring_view title, std::wstring_view message, Core::NoticeKind kind) override;
        void SetTrayTooltip(std::wstring_view text) override;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}