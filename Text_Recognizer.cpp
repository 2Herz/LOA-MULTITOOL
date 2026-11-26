module;  // 전역 모듈 조각 시작 (윈도우 · WinRT 헤더는 모듈 밖에서 포함)

#define NOMINMAX                 // min · max 매크로 끄기
#define WIN32_LEAN_AND_MEAN      // 잘 안 쓰는 윈도우 헤더 빼기
#include <Windows.h>             // 기본 윈도우 형식
#include <MemoryBuffer.h>        // IMemoryBufferByteAccess (SoftwareBitmap 픽셀 직접 쓰기)
#include <winrt/base.h>                                 // C++/WinRT 기본
#include <winrt/Windows.Foundation.h>                   // IAsyncOperation · IMemoryBufferReference
#include <winrt/Windows.Foundation.Collections.h>       // IVectorView (줄 · 단어 목록)
#include <winrt/Windows.Globalization.h>                // Language ("ko")
#include <winrt/Windows.Graphics.Imaging.h>             // SoftwareBitmap
#include <winrt/Windows.Media.Ocr.h>                    // OcrEngine

#pragma comment(lib, "windowsapp.lib")   // WinRT 라이브러리

module LostArkMultiTool.Vision.TextRecognizer;  // 글자 인식 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 이미지 값 객체 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 별칭 · 도우미
    {
        namespace Ocr = winrt::Windows::Media::Ocr;            // OCR API
        namespace Imaging = winrt::Windows::Graphics::Imaging; // 비트맵 API

        constexpr std::wstring_view KOREAN_LANGUAGE_TAG{ L"ko" };   // 한국어 언어 태그
        constexpr wchar_t LINE_SEPARATOR{ L'\n' };                  // 줄 구분 글자
        constexpr int MIN_SCALE{ 1 };                               // 확대 배율 최솟값
        constexpr int MAX_SCALE{ 4 };                               // 확대 배율 최댓값 (그 이상은 효과 없음)
        constexpr std::int32_t FIRST_PLANE{ 0 };                    // BGRA 비트맵의 첫(유일한) 평면

        // BGRA 이미지를 정수배로 키우기 (가장 가까운 픽셀 → 글자 가장자리가 흐려지지 않음)
        [[nodiscard]] FrameImage ScaleUp(const FrameImage& image, int scale)
        {
            if (scale <= MIN_SCALE) return image;                                       // 1배면 그대로
            const int width = image.GetWidth() * scale;                                 // 새 가로
            const int height = image.GetHeight() * scale;                               // 새 세로
            const auto source = image.GetPixels();                                      // 원본 픽셀
            std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * FrameImage::BYTES_PER_PIXEL);   // 새 버퍼
            for (int y = 0; y < height; ++y)                                            // 새 줄마다
            {
                const std::size_t sourceRow = static_cast<std::size_t>(y / scale) * image.GetWidth();   // 원본 줄
                for (int x = 0; x < width; ++x)                                         // 새 칸마다
                {
                    const std::size_t from = (sourceRow + static_cast<std::size_t>(x / scale)) * FrameImage::BYTES_PER_PIXEL;   // 원본 위치
                    const std::size_t to = (static_cast<std::size_t>(y) * width + x) * FrameImage::BYTES_PER_PIXEL;              // 새 위치
                    std::copy_n(source.begin() + static_cast<std::ptrdiff_t>(from), FrameImage::BYTES_PER_PIXEL,
                        pixels.begin() + static_cast<std::ptrdiff_t>(to));              // 한 픽셀 복사
                }
            }
            return FrameImage{ width, height, std::move(pixels) };                      // 키운 이미지
        }

        // FrameImage → SoftwareBitmap (픽셀 복사)
        [[nodiscard]] Imaging::SoftwareBitmap ToSoftwareBitmap(const FrameImage& image)
        {
            Imaging::SoftwareBitmap bitmap{ Imaging::BitmapPixelFormat::Bgra8, image.GetWidth(), image.GetHeight(),
                Imaging::BitmapAlphaMode::Premultiplied };                              // 같은 크기 비트맵
            {
                const auto buffer = bitmap.LockBuffer(Imaging::BitmapBufferAccessMode::Write);   // 쓰기 잠금 (범위 끝에서 풀림)
                const auto reference = buffer.CreateReference();                                  // 메모리 참조
                const auto access = reference.as<::Windows::Foundation::IMemoryBufferByteAccess>();   // 바이트 접근
                BYTE* destination = nullptr;                                                      // 비트맵 메모리
                UINT32 capacity = 0;                                                              // 메모리 크기
                winrt::check_hresult(access->GetBuffer(&destination, &capacity));                 // 메모리 얻기
                const auto plane = buffer.GetPlaneDescription(FIRST_PLANE);                       // 줄 간격 · 시작 위치
                const std::span<BYTE> target{ destination, capacity };                            // 경계가 있는 span
                const auto source = image.GetPixels();                                            // 원본 픽셀
                const std::size_t rowBytes = static_cast<std::size_t>(image.GetWidth()) * FrameImage::BYTES_PER_PIXEL;   // 한 줄 바이트
                for (int y = 0; y < image.GetHeight(); ++y)                                       // 줄마다
                {
                    const std::size_t offset = static_cast<std::size_t>(plane.StartIndex) + static_cast<std::size_t>(y) * plane.Stride;   // 대상 줄 시작
                    if (offset + rowBytes > target.size()) break;                                 // 버퍼 넘침 방지 (경계 검사)
                    std::ranges::copy(source.subspan(static_cast<std::size_t>(y) * rowBytes, rowBytes),
                        target.subspan(offset, rowBytes).begin());                                // 한 줄 복사
                }
            }
            return bitmap;   // 비트맵
        }
    }

    // ===== TextResult =====

    std::wstring TextResult::GetText() const
    {
        std::wstring text;                                    // 결과
        for (const TextLine& line : m_lines)                  // 줄마다
        {
            if (!text.empty()) text += LINE_SEPARATOR;        // 줄바꿈
            text += line.m_text;                              // 줄 글자
        }
        return text;   // 전체 글자
    }

    // ===== TextRecognizer::Impl =====

    struct TextRecognizer::Impl
    {
        Ocr::OcrEngine m_engine{ nullptr };   // OCR 엔진 (없으면 nullptr)
        std::wstring m_languageTag;           // 엔진 언어
    };

    // ===== TextRecognizer =====

    TextRecognizer::TextRecognizer()
        : m_impl{ std::make_unique<Impl>() }   // 구현 만들기
    {
        try
        {
            const winrt::Windows::Globalization::Language korean{ winrt::hstring{ KOREAN_LANGUAGE_TAG } };   // 한국어
            if (Ocr::OcrEngine::IsLanguageSupported(korean))                                                 // 한국어 OCR이 설치돼 있으면
                m_impl->m_engine = Ocr::OcrEngine::TryCreateFromLanguage(korean);                            // 한국어 엔진
            if (!m_impl->m_engine)                                                                           // 없으면
                m_impl->m_engine = Ocr::OcrEngine::TryCreateFromUserProfileLanguages();                      // 사용자 언어 엔진
            if (m_impl->m_engine)                                                                            // 엔진이 있으면
                m_impl->m_languageTag = std::wstring{ m_impl->m_engine.RecognizerLanguage().LanguageTag() }; // 언어 기록
        }
        catch (const winrt::hresult_error&)
        {
            m_impl->m_engine = nullptr;   // OCR 자체를 못 씀
        }
    }

    TextRecognizer::~TextRecognizer() = default;   // 엔진은 WinRT 참조 계수로 해제

    bool TextRecognizer::IsAvailable() const noexcept
    {
        return m_impl->m_engine != nullptr;   // 엔진이 있는지
    }

    const std::wstring& TextRecognizer::GetLanguageTag() const noexcept
    {
        return m_impl->m_languageTag;   // 엔진 언어
    }

    std::uint32_t TextRecognizer::GetMaxImageDimension() noexcept
    {
        try
        {
            return Ocr::OcrEngine::MaxImageDimension();   // 엔진 한계
        }
        catch (...)
        {
            return 0;   // OCR을 못 씀
        }
    }

    std::optional<TextResult> TextRecognizer::Recognize(const FrameImage& image, int scale) const
    {
        if (!m_impl->m_engine || image.IsEmpty()) return std::nullopt;                  // 엔진이 없거나 빈 이미지
        const int appliedScale = std::clamp(scale, MIN_SCALE, MAX_SCALE);              // 배율 범위 제한
        const std::uint32_t limit = GetMaxImageDimension();                            // 엔진 한계
        if (static_cast<std::uint32_t>(image.GetWidth() * appliedScale) > limit
            || static_cast<std::uint32_t>(image.GetHeight() * appliedScale) > limit) return std::nullopt;   // 너무 큼

        try
        {
            const FrameImage scaled = ScaleUp(image, appliedScale);                    // 작은 글자 키우기
            const auto result = m_impl->m_engine.RecognizeAsync(ToSoftwareBitmap(scaled)).get();   // 인식 (감시 스레드라 기다려도 됨)

            TextResult text;                                                           // 결과
            for (const auto& line : result.Lines())                                    // 줄마다
            {
                double left = std::numeric_limits<double>::max();                      // 줄 왼쪽
                double top = std::numeric_limits<double>::max();                       // 줄 위쪽
                double right = 0.0;                                                    // 줄 오른쪽
                double bottom = 0.0;                                                   // 줄 아래쪽
                for (const auto& word : line.Words())                                  // 단어마다
                {
                    const auto box = word.BoundingRect();                              // 단어 위치
                    left = std::min(left, static_cast<double>(box.X));                 // 왼쪽 갱신
                    top = std::min(top, static_cast<double>(box.Y));                   // 위쪽 갱신
                    right = std::max(right, static_cast<double>(box.X + box.Width));   // 오른쪽 갱신
                    bottom = std::max(bottom, static_cast<double>(box.Y + box.Height));// 아래쪽 갱신
                }
                const bool hasWords = right > 0.0;                                     // 단어가 있었는지
                const PixelRect bounds = hasWords
                    ? PixelRect{ static_cast<int>(left / appliedScale), static_cast<int>(top / appliedScale),
                        static_cast<int>((right - left) / appliedScale), static_cast<int>((bottom - top) / appliedScale) }   // 원래 배율 위치
                    : PixelRect{};                                                     // 위치 없음
                text.m_lines.push_back(TextLine{ std::wstring{ line.Text() }, bounds });   // 줄 추가
            }
            return text;   // 결과
        }
        catch (const winrt::hresult_error&)
        {
            return std::nullopt;   // 인식 실패
        }
    }
}
