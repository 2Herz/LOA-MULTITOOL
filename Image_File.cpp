module;  // 전역 모듈 조각 시작 (윈도우 헤더는 모듈 밖에서 포함)

#define NOMINMAX                 // min · max 매크로 끄기 (std::min과 충돌 방지)
#define WIN32_LEAN_AND_MEAN      // 잘 안 쓰는 윈도우 헤더 빼기
#include <Windows.h>             // 기본 윈도우 형식
#include <objbase.h>             // CoInitializeEx · CoUninitialize
#include <wincodec.h>            // WIC (이미지 읽기 · 쓰기)
#include <wrl/client.h>          // Microsoft::WRL::ComPtr (COM 스마트 포인터)

#pragma comment(lib, "windowscodecs.lib")   // WIC 라이브러리
#pragma comment(lib, "ole32.lib")           // COM 라이브러리

module LostArkMultiTool.Vision.ImageFile;  // 이미지 파일 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기

namespace LostArkMultiTool::Vision::ImageFile  // 이미지 파일 네임스페이스 시작
{
    namespace  // 이 파일 전용 도우미
    {
        using Microsoft::WRL::ComPtr;   // COM 스마트 포인터 (Release 자동 → 누수 없음)

        constexpr UINT FIRST_FRAME_INDEX{ 0 };   // 이미지 파일의 첫 장면

        // 이 스레드의 COM 초기화를 범위 안에서만 유지 (RAII, 이미 다른 방식으로 초기화돼 있으면 건드리지 않음)
        class COMScope
        {
        public:
            COMScope() noexcept
                : m_result{ ::CoInitializeEx(nullptr, COINIT_MULTITHREADED) }   // 다중 스레드 아파트로 초기화 시도
            {
            }

            ~COMScope()
            {
                if (SUCCEEDED(m_result)) ::CoUninitialize();   // 내가 초기화했을 때만 해제
            }

            COMScope(const COMScope&) = delete;              // 복사 금지
            COMScope& operator=(const COMScope&) = delete;   // 복사 대입 금지

            [[nodiscard]] bool IsUsable() const noexcept
            {
                return SUCCEEDED(m_result) || m_result == RPC_E_CHANGED_MODE;   // 성공했거나 이미 다른 아파트로 초기화됨 → COM 사용 가능
            }

        private:
            HRESULT m_result;   // 초기화 결과
        };

        // WIC 공장 만들기 (실패하면 nullptr)
        [[nodiscard]] ComPtr<IWICImagingFactory> CreateFactory() noexcept
        {
            ComPtr<IWICImagingFactory> factory;   // 결과
            ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));   // 생성 시도
            return factory;   // 실패하면 비어 있음
        }
    }

    std::optional<FrameImage> Load(const std::filesystem::path& path)
    {
        std::error_code error;                                                  // 예외 없는 파일 검사용
        if (!std::filesystem::is_regular_file(path, error)) return std::nullopt;// 파일이 없음

        const COMScope com;                                                     // COM 준비
        if (!com.IsUsable()) return std::nullopt;                               // COM을 못 씀
        const ComPtr<IWICImagingFactory> factory = CreateFactory();             // WIC 공장
        if (!factory) return std::nullopt;                                      // 공장 생성 실패

        ComPtr<IWICBitmapDecoder> decoder;                                      // 해독기
        if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
            WICDecodeMetadataCacheOnDemand, &decoder))) return std::nullopt;    // 파일 열기 실패
        ComPtr<IWICBitmapFrameDecode> frame;                                    // 첫 장면
        if (FAILED(decoder->GetFrame(FIRST_FRAME_INDEX, &frame))) return std::nullopt;   // 장면 읽기 실패

        ComPtr<IWICFormatConverter> converter;                                  // 픽셀 형식 변환기
        if (FAILED(factory->CreateFormatConverter(&converter))) return std::nullopt;     // 변환기 생성 실패
        if (FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
            nullptr, 0.0, WICBitmapPaletteTypeCustom))) return std::nullopt;    // BGRA로 바꾸기 실패

        UINT width = 0;                                                         // 가로 크기
        UINT height = 0;                                                        // 세로 크기
        if (FAILED(converter->GetSize(&width, &height))) return std::nullopt;   // 크기 읽기 실패

        const UINT rowPitch = width * FrameImage::BYTES_PER_PIXEL;              // 줄 간격 (빈칸 없음)
        std::vector<std::uint8_t> pixels(static_cast<std::size_t>(rowPitch) * height);   // 픽셀 버퍼
        if (FAILED(converter->CopyPixels(nullptr, rowPitch, static_cast<UINT>(pixels.size()), pixels.data())))
            return std::nullopt;                                                // 픽셀 복사 실패
        return FrameImage{ static_cast<int>(width), static_cast<int>(height), std::move(pixels) };   // 프레임
    }

    bool SavePNG(const std::filesystem::path& path, const FrameImage& image)
    {
        if (image.IsEmpty()) return false;                                      // 빈 이미지는 저장 안 함
        std::error_code error;                                                  // 예외 없는 폴더 만들기용
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path(), error);   // 폴더 만들기

        const COMScope com;                                                     // COM 준비
        if (!com.IsUsable()) return false;                                      // COM을 못 씀
        const ComPtr<IWICImagingFactory> factory = CreateFactory();             // WIC 공장
        if (!factory) return false;                                             // 공장 생성 실패

        ComPtr<IWICStream> stream;                                              // 파일 스트림
        if (FAILED(factory->CreateStream(&stream))) return false;               // 스트림 생성 실패
        if (FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))) return false;   // 파일 열기 실패

        ComPtr<IWICBitmapEncoder> encoder;                                      // PNG 부호화기
        if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder))) return false;   // 부호화기 생성 실패
        if (FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache))) return false;           // 스트림 연결 실패

        ComPtr<IWICBitmapFrameEncode> frame;                                    // 저장할 장면
        if (FAILED(encoder->CreateNewFrame(&frame, nullptr))) return false;     // 장면 생성 실패
        if (FAILED(frame->Initialize(nullptr))) return false;                   // 장면 초기화 실패
        const UINT width = static_cast<UINT>(image.GetWidth());                 // 가로 크기
        const UINT height = static_cast<UINT>(image.GetHeight());               // 세로 크기
        if (FAILED(frame->SetSize(width, height))) return false;                // 크기 설정 실패
        WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;               // 원하는 픽셀 형식
        if (FAILED(frame->SetPixelFormat(&format))) return false;               // 형식 설정 실패

        const auto pixels = image.GetPixels();                                  // 픽셀
        const UINT rowPitch = width * FrameImage::BYTES_PER_PIXEL;              // 줄 간격
        if (format == GUID_WICPixelFormat32bppBGRA)                             // 부호화기가 BGRA를 그대로 받으면
        {
            if (FAILED(frame->WritePixels(height, rowPitch, static_cast<UINT>(pixels.size()),
                const_cast<BYTE*>(pixels.data())))) return false;               // 픽셀 쓰기 (WIC가 읽기만 함)
        }
        else                                                                    // 다른 형식을 요구하면
        {
            ComPtr<IWICBitmap> bitmap;                                          // 메모리 비트맵
            if (FAILED(factory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, rowPitch,
                static_cast<UINT>(pixels.size()), const_cast<BYTE*>(pixels.data()), &bitmap))) return false;   // 비트맵 만들기
            if (FAILED(frame->WriteSource(bitmap.Get(), nullptr))) return false;   // 형식 변환하며 쓰기
        }
        if (FAILED(frame->Commit())) return false;                              // 장면 확정
        return SUCCEEDED(encoder->Commit());                                    // 파일 확정
    }
}
