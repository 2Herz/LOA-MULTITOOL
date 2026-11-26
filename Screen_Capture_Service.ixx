export module LostArkMultiTool.Vision.ScreenCaptureService;  // 게임 화면 캡처 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기

// 게임 창 화면 캡처 (윈도우 공식 API: Windows.Graphics.Capture)
// 안전: 게임 프로세스에 아무것도 넣지 않음 (DLL 주입 · D3D 후킹 없음). 윈도우가 합성한 창 화면을 받아 오기만 함
// 스레드: 한 객체는 한 스레드(감시 스레드)에서만 씀. 그 스레드는 먼저 VisionApartment를 만들어 둘 것
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 지금 스레드를 WinRT 다중 스레드 아파트로 준비 (범위가 끝나면 정리, RAII)
    class VisionApartment
    {
    public:
        VisionApartment();     // 초기화 (이미 다른 방식으로 초기화돼 있으면 그대로 씀)
        ~VisionApartment();    // 내가 초기화했을 때만 정리

        VisionApartment(const VisionApartment&) = delete;              // 복사 금지
        VisionApartment& operator=(const VisionApartment&) = delete;   // 복사 대입 금지

    private:
        bool m_isOwner{};   // 내가 초기화했는지
    };

    // 게임 창 캡처 서비스 (pimpl: WinRT · D3D 형식을 모듈 밖에 숨김)
    class ScreenCaptureService
    {
    public:
        ScreenCaptureService();    // 생성자 (장치는 Start 때 만듦)
        ~ScreenCaptureService();   // 소멸자 (캡처 정리)

        ScreenCaptureService(const ScreenCaptureService&) = delete;              // 복사 금지
        ScreenCaptureService& operator=(const ScreenCaptureService&) = delete;   // 복사 대입 금지

        [[nodiscard]] static bool IsSupported() noexcept;   // 이 윈도우에서 창 캡처를 쓸 수 있는지 (윈도우 10 1903 이상)

        // 창 캡처 시작 (이미 같은 창을 캡처 중이면 그대로 true, 실패하면 false + GetLastError 문구)
        [[nodiscard]] bool Start(void* windowHandle);
        void Stop() noexcept;                                           // 캡처 멈춤
        [[nodiscard]] bool IsCapturing(void* windowHandle) const noexcept;   // 이 창을 캡처 중인지

        // 가장 최근 프레임의 게임 화면 영역(제목 표시줄 · 테두리 제외)을 CPU 메모리로 복사 (새 프레임이 없으면 nullopt)
        [[nodiscard]] std::optional<FrameImage> TakeLatestFrame();

        [[nodiscard]] const std::string& GetLastError() const noexcept;   // 마지막 실패 이유 (UTF-8, 로그용)

    private:
        struct Impl;                     // 구현 (WinRT · D3D 객체)
        std::unique_ptr<Impl> m_impl;    // 구현 소유
    };
}
