module;  // 전역 모듈 조각 시작 (윈도우 · WinRT 헤더는 모듈 밖에서 포함)

#define NOMINMAX                 // min · max 매크로 끄기
#define WIN32_LEAN_AND_MEAN      // 잘 안 쓰는 윈도우 헤더 빼기
#include <Windows.h>             // 기본 윈도우 형식
#include <d3d11.h>               // Direct3D 11 (캡처 텍스처 · CPU 복사)
#include <dxgi1_2.h>             // DXGI 장치
#include <dwmapi.h>              // DwmGetWindowAttribute (창 실제 테두리 위치)
#include <winrt/base.h>                                   // C++/WinRT 기본
#include <winrt/Windows.Foundation.h>                     // WinRT 기본 형식
#include <winrt/Windows.Graphics.h>                       // SizeInt32
#include <winrt/Windows.Graphics.Capture.h>               // GraphicsCaptureItem · 프레임 풀 · 세션
#include <winrt/Windows.Graphics.DirectX.h>               // DirectXPixelFormat
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>    // IDirect3DDevice
#include <windows.graphics.capture.interop.h>             // IGraphicsCaptureItemInterop (HWND → 캡처 대상)
#include <windows.graphics.directx.direct3d11.interop.h>  // D3D11 ↔ WinRT 장치 연결

#pragma comment(lib, "windowsapp.lib")   // WinRT 라이브러리
#pragma comment(lib, "d3d11.lib")        // Direct3D 11
#pragma comment(lib, "dwmapi.lib")       // DWM

module LostArkMultiTool.Vision.ScreenCaptureService;  // 게임 화면 캡처 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 캡처 프레임 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 별칭
    {
        namespace Capture = winrt::Windows::Graphics::Capture;              // 캡처 API
        namespace DirectX = winrt::Windows::Graphics::DirectX;              // 픽셀 형식
        namespace Direct3D11 = winrt::Windows::Graphics::DirectX::Direct3D11;   // WinRT D3D 장치

        constexpr std::int32_t FRAME_BUFFER_COUNT{ 2 };                     // 프레임 풀 버퍼 수 (최신 프레임 + 다음 프레임)
        constexpr DirectX::DirectXPixelFormat CAPTURE_FORMAT{ DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized };   // BGRA 8비트
        constexpr UINT SUBRESOURCE_INDEX{ 0 };                              // 텍스처 첫 밉맵
        constexpr UINT NO_FLAGS{ 0 };                                       // 추가 옵션 없음
        constexpr UINT SINGLE_LEVEL{ 1 };                                   // 밉맵 한 단계
        constexpr UINT SINGLE_ARRAY{ 1 };                                   // 배열 한 장
        constexpr UINT SINGLE_SAMPLE{ 1 };                                  // 다중 샘플 없음

        // HRESULT → 실패 문구 (UTF-8)
        [[nodiscard]] std::string DescribeFailure(std::string_view step, HRESULT result)
        {
            return std::format("{} 실패 (0x{:08X})", step, static_cast<std::uint32_t>(result));   // 단계 + 오류 코드
        }
    }

    // ===== VisionApartment =====

    VisionApartment::VisionApartment()
    {
        ::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);   // 이 스레드의 창 좌표 · 커서를 물리 픽셀로 (캡처 프레임과 같은 단위)
        try
        {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);   // 다중 스레드 아파트로 초기화
            m_isOwner = true;                                               // 내가 초기화함
        }
        catch (const winrt::hresult_error&)
        {
            m_isOwner = false;   // 이미 다른 아파트로 초기화된 스레드 → 그대로 씀
        }
    }

    VisionApartment::~VisionApartment()
    {
        if (m_isOwner) winrt::uninit_apartment();   // 내가 초기화했을 때만 정리
    }

    // ===== ScreenCaptureService::Impl =====

    struct ScreenCaptureService::Impl
    {
        winrt::com_ptr<ID3D11Device> m_device;                    // 캡처용 D3D 장치
        winrt::com_ptr<ID3D11DeviceContext> m_context;            // 장치 명령 문맥
        Direct3D11::IDirect3DDevice m_winrtDevice{ nullptr };     // WinRT로 감싼 장치
        Capture::GraphicsCaptureItem m_item{ nullptr };           // 캡처 대상 (게임 창)
        Capture::Direct3D11CaptureFramePool m_pool{ nullptr };    // 프레임 풀
        Capture::GraphicsCaptureSession m_session{ nullptr };     // 캡처 세션
        winrt::Windows::Graphics::SizeInt32 m_poolSize{};         // 프레임 풀 크기
        winrt::com_ptr<ID3D11Texture2D> m_staging;                // CPU에서 읽을 복사용 텍스처
        UINT m_stagingWidth{};                                    // 복사용 텍스처 가로
        UINT m_stagingHeight{};                                   // 복사용 텍스처 세로
        HWND m_window{};                                          // 캡처 중인 창
        std::string m_lastError;                                  // 마지막 실패 이유

        // D3D 장치 준비 (한 번만)
        [[nodiscard]] bool EnsureDevice()
        {
            if (m_device) return true;                                                        // 이미 있음
            const HRESULT created = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION,
                m_device.put(), nullptr, m_context.put());                                     // 하드웨어 장치 만들기
            if (FAILED(created)) { m_lastError = DescribeFailure("D3D11 장치 생성", created); return false; }   // 실패

            const auto dxgiDevice = m_device.as<IDXGIDevice>();                               // DXGI 장치로 보기
            winrt::com_ptr<::IInspectable> inspectable;                                       // WinRT 객체
            const HRESULT wrapped = ::CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectable.put());   // WinRT 장치로 감싸기
            if (FAILED(wrapped)) { m_lastError = DescribeFailure("WinRT 장치 연결", wrapped); m_device = nullptr; return false; }   // 실패
            m_winrtDevice = inspectable.as<Direct3D11::IDirect3DDevice>();                    // 형 변환
            return true;                                                                      // 준비 완료
        }

        // 캡처 정리
        void Close() noexcept
        {
            try
            {
                if (m_session) m_session.Close();   // 세션 닫기
                if (m_pool) m_pool.Close();         // 프레임 풀 닫기
            }
            catch (...)
            {
                // 닫는 중 실패는 무시 (이미 끊긴 창)
            }
            m_session = nullptr;    // 세션 해제
            m_pool = nullptr;       // 풀 해제
            m_item = nullptr;       // 대상 해제
            m_window = nullptr;     // 창 기록 지움
        }

        // 복사용 텍스처를 프레임 크기에 맞게 준비
        [[nodiscard]] bool EnsureStaging(UINT width, UINT height)
        {
            if (m_staging && m_stagingWidth == width && m_stagingHeight == height) return true;   // 이미 맞음
            D3D11_TEXTURE2D_DESC description{};                       // 텍스처 설명
            description.Width = width;                                // 가로
            description.Height = height;                              // 세로
            description.MipLevels = SINGLE_LEVEL;                     // 밉맵 한 단계
            description.ArraySize = SINGLE_ARRAY;                     // 한 장
            description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;          // BGRA
            description.SampleDesc.Count = SINGLE_SAMPLE;             // 다중 샘플 없음
            description.Usage = D3D11_USAGE_STAGING;                  // CPU 읽기용
            description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;       // CPU가 읽음
            m_staging = nullptr;                                      // 예전 텍스처 해제
            const HRESULT created = m_device->CreateTexture2D(&description, nullptr, m_staging.put());   // 만들기
            if (FAILED(created)) { m_lastError = DescribeFailure("복사용 텍스처 생성", created); return false; }   // 실패
            m_stagingWidth = width;                                   // 크기 기록
            m_stagingHeight = height;                                 // 크기 기록
            return true;                                              // 준비 완료
        }

        // 캡처 화면 안에서 게임 화면 영역(제목 표시줄 · 테두리 제외) 계산
        [[nodiscard]] PixelRect GetClientRegion(int frameWidth, int frameHeight) const noexcept
        {
            const PixelRect whole{ 0, 0, frameWidth, frameHeight };                 // 기본값: 프레임 전체
            RECT frameBounds{};                                                     // 창 실제 테두리 (그림자 제외)
            if (FAILED(::DwmGetWindowAttribute(m_window, DWMWA_EXTENDED_FRAME_BOUNDS,
                &frameBounds, sizeof(frameBounds)))) return whole;                  // 못 읽으면 전체
            RECT client{};                                                          // 화면 영역 (창 기준)
            if (!::GetClientRect(m_window, &client)) return whole;                  // 못 읽으면 전체
            POINT origin{ client.left, client.top };                                // 화면 영역 왼쪽 위
            if (!::ClientToScreen(m_window, &origin)) return whole;                 // 못 바꾸면 전체
            const PixelRect region{ origin.x - frameBounds.left, origin.y - frameBounds.top,
                client.right - client.left, client.bottom - client.top };           // 캡처 기준 화면 영역
            return ClampRect(region, frameWidth, frameHeight);                      // 프레임 안으로
        }
    };

    // ===== ScreenCaptureService =====

    ScreenCaptureService::ScreenCaptureService()
        : m_impl{ std::make_unique<Impl>() }   // 구현 만들기
    {
    }

    ScreenCaptureService::~ScreenCaptureService()
    {
        Stop();   // 캡처 정리 (RAII)
    }

    bool ScreenCaptureService::IsSupported() noexcept
    {
        try
        {
            return Capture::GraphicsCaptureSession::IsSupported();   // 운영체제 지원 여부
        }
        catch (...)
        {
            return false;   // API 자체가 없음 (오래된 윈도우)
        }
    }

    bool ScreenCaptureService::Start(void* windowHandle)
    {
        const HWND window = static_cast<HWND>(windowHandle);           // 핸들 형 변환
        if (IsCapturing(windowHandle)) return true;                    // 이미 같은 창
        Stop();                                                        // 다른 창이면 정리 후 다시
        if (!IsSupported()) { m_impl->m_lastError = "창 캡처를 지원하지 않는 윈도우"; return false; }   // 미지원
        if (!m_impl->EnsureDevice()) return false;                     // 장치 준비 실패

        try
        {
            const auto interop = winrt::get_activation_factory<Capture::GraphicsCaptureItem, ::IGraphicsCaptureItemInterop>();   // HWND → 대상 변환기
            Capture::GraphicsCaptureItem item{ nullptr };                                                    // 캡처 대상
            winrt::check_hresult(interop->CreateForWindow(window, winrt::guid_of<Capture::GraphicsCaptureItem>(), winrt::put_abi(item)));   // 창으로 대상 만들기

            m_impl->m_poolSize = item.Size();                                                                // 창 크기
            m_impl->m_pool = Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(m_impl->m_winrtDevice,
                CAPTURE_FORMAT, FRAME_BUFFER_COUNT, m_impl->m_poolSize);                                     // 자유 스레드 프레임 풀 (디스패처 불필요)
            m_impl->m_session = m_impl->m_pool.CreateCaptureSession(item);                                   // 세션 만들기
            try { m_impl->m_session.IsCursorCaptureEnabled(false); } catch (...) {}                          // 마우스 커서는 안 찍음 (구형 윈도우는 무시)
            try { m_impl->m_session.IsBorderRequired(false); } catch (...) {}                                // 노란 캡처 테두리 끄기 (윈도우 11, 안 되면 무시)
            m_impl->m_session.StartCapture();                                                                // 캡처 시작
            m_impl->m_item = item;                                                                           // 대상 보관
            m_impl->m_window = window;                                                                       // 창 기록
            m_impl->m_lastError.clear();                                                                     // 오류 없음
            return true;                                                                                     // 성공
        }
        catch (const winrt::hresult_error& error)
        {
            m_impl->m_lastError = DescribeFailure("창 캡처 시작", error.code());   // 실패 이유
            m_impl->Close();                                                      // 반쯤 만든 객체 정리
            return false;                                                         // 실패
        }
    }

    void ScreenCaptureService::Stop() noexcept
    {
        if (m_impl) m_impl->Close();   // 캡처 정리
    }

    bool ScreenCaptureService::IsCapturing(void* windowHandle) const noexcept
    {
        return m_impl->m_session != nullptr && m_impl->m_window == static_cast<HWND>(windowHandle);   // 세션이 있고 같은 창
    }

    std::optional<FrameImage> ScreenCaptureService::TakeLatestFrame(bool isCopyNeeded)
    {
        if (!m_impl->m_pool) return std::nullopt;   // 캡처 중이 아님

        try
        {
            Capture::Direct3D11CaptureFrame latest{ nullptr };                         // 가장 최근 프레임
            for (auto frame = m_impl->m_pool.TryGetNextFrame(); frame; frame = m_impl->m_pool.TryGetNextFrame())   // 쌓인 프레임을 모두 꺼냄
                latest = frame;                                                        // 마지막 것만 남김
            if (!latest) return std::nullopt;                                          // 새 프레임 없음
            if (!isCopyNeeded) return std::nullopt;                                    // 이번 주기엔 할 일이 없음 → 비우기만

            const auto contentSize = latest.ContentSize();                             // 실제 화면 크기
            if (contentSize.Width != m_impl->m_poolSize.Width || contentSize.Height != m_impl->m_poolSize.Height)   // 창 크기가 바뀌었으면
            {
                m_impl->m_poolSize = contentSize;                                      // 새 크기
                m_impl->m_pool.Recreate(m_impl->m_winrtDevice, CAPTURE_FORMAT, FRAME_BUFFER_COUNT, contentSize);   // 풀 다시 만들기 (다음 프레임부터)
            }

            const auto access = latest.Surface().as<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();   // 표면 → D3D 텍스처 접근
            winrt::com_ptr<ID3D11Texture2D> texture;                                   // 프레임 텍스처
            winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(texture.put())));   // 텍스처 꺼내기
            D3D11_TEXTURE2D_DESC description{};                                        // 텍스처 크기
            texture->GetDesc(&description);                                            // 설명 읽기

            const UINT width = std::min(description.Width, static_cast<UINT>(std::max(contentSize.Width, 0)));     // 복사할 가로 (실제 화면 크기까지)
            const UINT height = std::min(description.Height, static_cast<UINT>(std::max(contentSize.Height, 0)));  // 복사할 세로
            if (width == 0 || height == 0) return std::nullopt;                        // 최소화 등으로 크기가 0
            const PixelRect client = m_impl->GetClientRegion(static_cast<int>(width), static_cast<int>(height));   // 게임 화면 영역 (창 테두리 · 제목 줄 제외)
            if (client.IsEmpty()) return std::nullopt;                                 // 영역 없음
            const UINT copyWidth = static_cast<UINT>(client.m_width);                  // 복사할 가로 (게임 영역만)
            const UINT copyHeight = static_cast<UINT>(client.m_height);                // 복사할 세로
            if (!m_impl->EnsureStaging(copyWidth, copyHeight)) return std::nullopt;    // 복사용 텍스처 준비 실패

            const D3D11_BOX box{ static_cast<UINT>(client.m_x), static_cast<UINT>(client.m_y), 0,
                static_cast<UINT>(client.m_x) + copyWidth, static_cast<UINT>(client.m_y) + copyHeight, 1 };   // 게임 영역만 GPU에서 잘라 복사 (한 번만 복사)
            m_impl->m_context->CopySubresourceRegion(m_impl->m_staging.get(), SUBRESOURCE_INDEX, 0, 0, 0,
                texture.get(), SUBRESOURCE_INDEX, &box);                               // GPU에서 복사용 텍스처로

            D3D11_MAPPED_SUBRESOURCE mapped{};                                         // CPU 매핑 결과
            const HRESULT mappedResult = m_impl->m_context->Map(m_impl->m_staging.get(), SUBRESOURCE_INDEX, D3D11_MAP_READ, NO_FLAGS, &mapped);   // CPU로 매핑
            if (FAILED(mappedResult)) { m_impl->m_lastError = DescribeFailure("프레임 읽기", mappedResult); return std::nullopt; }   // 실패

            const std::span<const std::uint8_t> source{ static_cast<const std::uint8_t*>(mapped.pData),
                static_cast<std::size_t>(mapped.RowPitch) * copyHeight };              // 매핑 메모리 (경계가 있는 span)
            FrameImage frame;                                                          // 결과
            try
            {
                frame = FrameImage::FromRows(static_cast<int>(copyWidth), static_cast<int>(copyHeight), source, static_cast<int>(mapped.RowPitch));   // CPU 버퍼로 복사 (게임 영역만)
            }
            catch (...)
            {
                m_impl->m_context->Unmap(m_impl->m_staging.get(), SUBRESOURCE_INDEX);  // 예외가 나도 매핑 해제
                throw;                                                                 // 다시 던짐
            }
            m_impl->m_context->Unmap(m_impl->m_staging.get(), SUBRESOURCE_INDEX);      // 매핑 해제

            return frame;                                                              // 게임 화면 (이미 게임 영역만)
        }
        catch (const winrt::hresult_error& error)
        {
            m_impl->m_lastError = DescribeFailure("프레임 가져오기", error.code());   // 실패 이유 (창이 닫힌 경우 등)
            m_impl->Close();                                                           // 다음 Start에서 다시 연결
            return std::nullopt;                                                       // 프레임 없음
        }
    }

    const std::string& ScreenCaptureService::GetLastError() const noexcept
    {
        return m_impl->m_lastError;   // 마지막 실패 이유
    }
}
