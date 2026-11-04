export module LostArkMultiTool.Views.MainWindow;

import std;

export namespace LostArkMultiTool::Views
{
    // 창에서 일어난 일을 받는 쪽 (Application이 구현) — 마우스 좌표는 DIP 단위
    class WindowEventHandler
    {
    public:
        virtual ~WindowEventHandler();

        virtual void OnResize(int pixelWidth, int pixelHeight) = 0;   // 스왑체인 크기는 실제 픽셀
        virtual void OnDPIChanged(float dpi) = 0;                     // 다른 배율 모니터로 옮겨 갔을 때
        virtual void OnMouseMove(float x, float y) = 0;
        virtual void OnMouseLeave() = 0;
        virtual void OnMouseDown(float x, float y) = 0;               // 왼쪽 버튼
        virtual void OnKeyDown(unsigned virtualKey, bool isCtrlDown, bool isShiftDown) = 0;
    };

    // 창 안쪽 크기 (실제 픽셀)
    struct PixelSize
    {
        int m_width{};
        int m_height{};
    };

    // Win32 창 (DX11 화면이 그려지는 바탕) — Windows 타입(HWND 등)은 구현 파일 안에만 둠
    class MainWindow
    {
    public:
        // width, height: 처음 창 안쪽 크기 (DIP, 모니터 배율에 맞춰 픽셀로 늘어남)
        MainWindow(std::wstring_view title, int width, int height, WindowEventHandler& handler);
        ~MainWindow();

        MainWindow(const MainWindow&) = delete;
        MainWindow& operator=(const MainWindow&) = delete;

        void Show(int showCommand);

        // 쌓인 메시지를 모두 처리 (창이 닫혀 WM_QUIT를 받으면 false)
        [[nodiscard]] bool ProcessMessages();

        // 새 메시지가 올 때까지 대기 (최소화 중 CPU 사용을 줄임)
        void WaitForMessage();

        [[nodiscard]] void* GetNativeHandle() const noexcept;   // HWND (렌더러가 스왑체인을 만들 때 사용)
        [[nodiscard]] float GetDPI() const noexcept;
        [[nodiscard]] PixelSize GetClientPixelSize() const;
        [[nodiscard]] bool IsMinimized() const noexcept;
        [[nodiscard]] int GetExitCode() const noexcept;

    private:
        class Impl;
        std::unique_ptr<Impl> m_impl;
    };
}