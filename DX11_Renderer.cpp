module;

#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <d2d1_1.h>
#include <dwrite.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

module LostArkMultiTool.Views.DX11Renderer;

import std;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr float DEFAULT_DPI{ 96.0f };                 // 배율 100%일 때 DPI (DIP 기준)
        constexpr UINT SWAP_CHAIN_BUFFER_COUNT{ 2 };          // 플립 모델 최소 버퍼 수
        constexpr UINT BACK_BUFFER_INDEX{ 0 };
        constexpr UINT SYNC_INTERVAL_VSYNC{ 1 };              // 모니터 주사율에 맞춰 표시 (불필요한 CPU · GPU 사용 방지)
        constexpr float FONT_SIZE_KEY_SCALE{ 10.0f };         // 글꼴 캐시 키: 크기를 0.1 단위 정수로

        // 그래픽 카드가 지원하는 기능 수준 (높은 것부터 시도)
        constexpr std::array FEATURE_LEVELS{
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0
        };

        // COM 객체 전용 스마트 포인터 (소멸 시 Release 자동 호출 → 해제 누락 없음, 이동만 허용)
        // WRL ComPtr 대신 직접 구현: <wrl/client.h>는 C++ 표준 헤더(<new>)를 끌어와 import std와 같은 파일에서 섞이면 안 되기 때문
        template <typename T>
        class COMPtr
        {
        public:
            COMPtr() noexcept = default;

            ~COMPtr()
            {
                Reset();
            }

            COMPtr(const COMPtr&) = delete;
            COMPtr& operator=(const COMPtr&) = delete;

            COMPtr(COMPtr&& other) noexcept
                : m_pointer{ std::exchange(other.m_pointer, nullptr) }
            {}

            COMPtr& operator=(COMPtr&& other) noexcept
            {
                if (this != &other)
                {
                    Reset();
                    m_pointer = std::exchange(other.m_pointer, nullptr);
                }
                return *this;
            }

            [[nodiscard]] T* Get() const noexcept
            {
                return m_pointer;
            }

            [[nodiscard]] T* operator->() const noexcept
            {
                return m_pointer;
            }

            [[nodiscard]] explicit operator bool() const noexcept
            {
                return m_pointer != nullptr;
            }

            // DirectX 함수의 출력 매개변수용 주소 (갖고 있던 객체는 먼저 놓아줌)
            [[nodiscard]] T** ReleaseAndGetAddressOf() noexcept
            {
                Reset();
                return &m_pointer;
            }

            void Reset() noexcept
            {
                if (m_pointer != nullptr)
                {
                    m_pointer->Release();
                    m_pointer = nullptr;
                }
            }

            // 같은 객체를 다른 인터페이스로 받기 (QueryInterface)
            template <typename U>
            [[nodiscard]] HRESULT As(COMPtr<U>& target) const noexcept
            {
                return m_pointer->QueryInterface(__uuidof(U), reinterpret_cast<void**>(target.ReleaseAndGetAddressOf()));
            }

        private:
            T* m_pointer{ nullptr };
        };

        // 실패한 DirectX 호출을 예외로 (어떤 단계에서 실패했는지 + HRESULT 값)
        void ThrowIfFailed(HRESULT result, std::string_view stepName)
        {
            if (SUCCEEDED(result)) return;

            const auto code{ static_cast<unsigned long>(result) };
            throw std::runtime_error{ std::vformat("{} 실패 (HRESULT 0x{:08X})", std::make_format_args(stepName, code)) };
        }

        // 그래픽 장치를 잃은 경우 (드라이버 업데이트 · 장치 분리 등) → 장치를 다시 만들면 복구됨
        [[nodiscard]] bool IsDeviceLost(HRESULT result) noexcept
        {
            return result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET;
        }

        [[nodiscard]] D2D1_COLOR_F ToD2DColor(const Core::Color& color) noexcept
        {
            return D2D1_COLOR_F{ color.m_red, color.m_green, color.m_blue, color.m_alpha };
        }

        [[nodiscard]] D2D1_RECT_F ToD2DRect(const Core::RectF& rect) noexcept
        {
            return D2D1_RECT_F{ rect.m_left, rect.m_top, rect.m_right, rect.m_bottom };
        }

        [[nodiscard]] DWRITE_TEXT_ALIGNMENT ToTextAlignment(Core::TextAlign align) noexcept
        {
            switch (align)
            {
                case Core::TextAlign::CENTER:   return DWRITE_TEXT_ALIGNMENT_CENTER;
                case Core::TextAlign::TRAILING: return DWRITE_TEXT_ALIGNMENT_TRAILING;
                default:                        return DWRITE_TEXT_ALIGNMENT_LEADING;
            }
        }

        [[nodiscard]] DWRITE_FONT_WEIGHT ToFontWeight(Core::FontWeight weight) noexcept
        {
            return weight == Core::FontWeight::BOLD ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
        }
    }

    // ===== 실제 DirectX 구현 =====
    class DX11Renderer::Impl
    {
    public:
        Impl(HWND window, float dpi);

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        [[nodiscard]] bool BeginFrame();
        void EndFrame();
        void Resize(int pixelWidth, int pixelHeight);
        void SetDPI(float dpi);

        [[nodiscard]] float GetWidth() const noexcept;
        [[nodiscard]] float GetHeight() const noexcept;

        void Clear(const Core::Color& color);
        void FillRect(const Core::RectF& rect, const Core::Color& color);
        void DrawRect(const Core::RectF& rect, const Core::Color& color, float strokeWidth);
        void DrawLine(float startX, float startY, float endX, float endY, const Core::Color& color, float strokeWidth);
        void DrawString(std::wstring_view text, const Core::RectF& rect, const Core::Color& color, const Core::TextStyle& style);

    private:
        // 장치 자원: 그래픽 장치를 잃으면 버리고 다시 만듦
        void CreateDeviceResources();
        void ReleaseDeviceResources() noexcept;
        void HandleDeviceLost();

        // 스왑체인 뒷버퍼를 Direct2D 그리기 대상으로 연결 (크기 · 배율이 바뀔 때마다 다시 만듦)
        void CreateTargetBitmap();
        void ReleaseTargetBitmap() noexcept;

        [[nodiscard]] HRESULT CreateD3DDevice(D3D_DRIVER_TYPE driverType);
        [[nodiscard]] ID2D1SolidColorBrush* PrepareBrush(const Core::Color& color);
        [[nodiscard]] IDWriteTextFormat* GetTextFormat(const Core::TextStyle& style);

        HWND m_window{};
        float m_dpi{ DEFAULT_DPI };
        int m_pixelWidth{};
        int m_pixelHeight{};
        bool m_isDrawing{};

        // 장치와 무관한 자원 (한 번만 만듦)
        COMPtr<ID2D1Factory1> m_d2dFactory;
        COMPtr<IDWriteFactory> m_writeFactory;
        std::map<std::pair<int, Core::FontWeight>, COMPtr<IDWriteTextFormat>> m_textFormats;   // (크기 × 10, 굵기) → 글꼴

        // 장치 자원 (선언 순서대로 만들고 반대 순서로 해제)
        COMPtr<ID3D11Device> m_d3dDevice;
        COMPtr<ID3D11DeviceContext> m_d3dContext;
        COMPtr<IDXGISwapChain1> m_swapChain;
        COMPtr<ID2D1Device> m_d2dDevice;
        COMPtr<ID2D1DeviceContext> m_d2dContext;
        COMPtr<ID2D1SolidColorBrush> m_brush;     // 색만 바꿔 가며 재사용
        COMPtr<ID2D1Bitmap1> m_targetBitmap;
    };

    DX11Renderer::Impl::Impl(HWND window, float dpi)
        : m_window{ window }
        , m_dpi{ dpi }
    {
        const D2D1_FACTORY_OPTIONS factoryOptions{};
        ThrowIfFailed(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &factoryOptions,
            reinterpret_cast<void**>(m_d2dFactory.ReleaseAndGetAddressOf())), "Direct2D 팩토리 생성");

        ThrowIfFailed(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
            reinterpret_cast<IUnknown**>(m_writeFactory.ReleaseAndGetAddressOf())), "DirectWrite 팩토리 생성");

        CreateDeviceResources();
    }

    HRESULT DX11Renderer::Impl::CreateD3DDevice(D3D_DRIVER_TYPE driverType)
    {
        // Direct2D와 같은 화면을 쓰려면 BGRA 형식 지원이 필요
        return ::D3D11CreateDevice(nullptr, driverType, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            FEATURE_LEVELS.data(), static_cast<UINT>(FEATURE_LEVELS.size()), D3D11_SDK_VERSION,
            m_d3dDevice.ReleaseAndGetAddressOf(), nullptr, m_d3dContext.ReleaseAndGetAddressOf());
    }

    void DX11Renderer::Impl::CreateDeviceResources()
    {
        // 1) D3D11 장치 (그래픽 카드가 없거나 실패하면 WARP 소프트웨어 장치로 대체)
        HRESULT result{ CreateD3DDevice(D3D_DRIVER_TYPE_HARDWARE) };
        if (FAILED(result)) result = CreateD3DDevice(D3D_DRIVER_TYPE_WARP);
        ThrowIfFailed(result, "Direct3D 11 장치 생성");

        // 2) 스왑체인 (DXGI 장치 → 어댑터 → 팩토리 순서로 올라가 장치를 만든 팩토리에서 생성)
        COMPtr<IDXGIDevice1> dxgiDevice;
        ThrowIfFailed(m_d3dDevice.As(dxgiDevice), "DXGI 장치 얻기");

        COMPtr<IDXGIAdapter> adapter;
        ThrowIfFailed(dxgiDevice->GetAdapter(adapter.ReleaseAndGetAddressOf()), "그래픽 어댑터 얻기");

        COMPtr<IDXGIFactory2> dxgiFactory;
        ThrowIfFailed(adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(dxgiFactory.ReleaseAndGetAddressOf())),
            "DXGI 팩토리 얻기");

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.Width = 0;                                   // 0이면 창 안쪽 크기를 따름
        swapChainDesc.Height = 0;
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;         // Direct2D가 그릴 수 있는 형식
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // 플립 모델 (Windows 10 이상 권장 방식)
        swapChainDesc.Scaling = DXGI_SCALING_NONE;                 // 크기를 바꾸는 동안 화면이 늘어나 보이지 않게
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

        ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_d3dDevice.Get(), m_window, &swapChainDesc, nullptr, nullptr,
            m_swapChain.ReleaseAndGetAddressOf()), "스왑체인 생성");

        // 도구 프로그램이라 Alt+Enter 전체 화면 전환은 막음
        dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER);

        // 3) Direct2D 장치 · 컨텍스트 (같은 DXGI 장치 위에 만들어 D3D11과 같은 화면을 공유)
        ThrowIfFailed(m_d2dFactory->CreateDevice(dxgiDevice.Get(), m_d2dDevice.ReleaseAndGetAddressOf()), "Direct2D 장치 생성");
        ThrowIfFailed(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dContext.ReleaseAndGetAddressOf()),
            "Direct2D 컨텍스트 생성");

        ThrowIfFailed(m_d2dContext->CreateSolidColorBrush(ToD2DColor(Theme::TEXT_COLOR), m_brush.ReleaseAndGetAddressOf()),
            "브러시 생성");

        CreateTargetBitmap();
    }

    void DX11Renderer::Impl::ReleaseDeviceResources() noexcept
    {
        ReleaseTargetBitmap();
        m_brush.Reset();
        m_d2dContext.Reset();
        m_d2dDevice.Reset();
        m_swapChain.Reset();
        m_d3dContext.Reset();
        m_d3dDevice.Reset();
    }

    void DX11Renderer::Impl::HandleDeviceLost()
    {
        m_isDrawing = false;
        ReleaseDeviceResources();
        CreateDeviceResources();   // 글꼴(DirectWrite)은 장치와 무관해서 그대로 씀
    }

    void DX11Renderer::Impl::CreateTargetBitmap()
    {
        COMPtr<IDXGISurface> backBuffer;
        ThrowIfFailed(m_swapChain->GetBuffer(BACK_BUFFER_INDEX, __uuidof(IDXGISurface), reinterpret_cast<void**>(backBuffer.ReleaseAndGetAddressOf())),
            "뒷버퍼 얻기");

        D2D1_BITMAP_PROPERTIES1 bitmapProperties{};
        bitmapProperties.pixelFormat = D2D1_PIXEL_FORMAT{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE };
        bitmapProperties.dpiX = m_dpi;
        bitmapProperties.dpiY = m_dpi;
        bitmapProperties.bitmapOptions = static_cast<D2D1_BITMAP_OPTIONS>(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW);

        ThrowIfFailed(m_d2dContext->CreateBitmapFromDxgiSurface(backBuffer.Get(), &bitmapProperties, m_targetBitmap.ReleaseAndGetAddressOf()),
            "그리기 대상 생성");

        m_d2dContext->SetTarget(m_targetBitmap.Get());
        m_d2dContext->SetDpi(m_dpi, m_dpi);   // 그리기 좌표는 DIP → 배율에 맞춰 자동 확대

        DXGI_SWAP_CHAIN_DESC1 currentDesc{};
        m_swapChain->GetDesc1(&currentDesc);
        m_pixelWidth = static_cast<int>(currentDesc.Width);
        m_pixelHeight = static_cast<int>(currentDesc.Height);
    }

    void DX11Renderer::Impl::ReleaseTargetBitmap() noexcept
    {
        // 뒷버퍼를 붙잡고 있는 D2D 대상부터 놓아야 ResizeBuffers가 성공함
        if (m_d2dContext) m_d2dContext->SetTarget(nullptr);
        m_targetBitmap.Reset();
    }

    bool DX11Renderer::Impl::BeginFrame()
    {
        if (!m_targetBitmap || m_isDrawing) return false;

        m_d2dContext->BeginDraw();
        m_isDrawing = true;
        return true;
    }

    void DX11Renderer::Impl::EndFrame()
    {
        if (!m_isDrawing) return;
        m_isDrawing = false;

        const HRESULT drawResult{ m_d2dContext->EndDraw() };
        if (drawResult == D2DERR_RECREATE_TARGET)
        {
            HandleDeviceLost();
            return;
        }
        ThrowIfFailed(drawResult, "Direct2D 그리기");

        const HRESULT presentResult{ m_swapChain->Present(SYNC_INTERVAL_VSYNC, 0) };
        if (IsDeviceLost(presentResult))
        {
            HandleDeviceLost();
            return;
        }
        ThrowIfFailed(presentResult, "화면 표시");
    }

    void DX11Renderer::Impl::Resize(int pixelWidth, int pixelHeight)
    {
        if (pixelWidth <= 0 || pixelHeight <= 0) return;                               // 최소화 등
        if (pixelWidth == m_pixelWidth && pixelHeight == m_pixelHeight) return;         // 크기 변화 없음
        if (m_isDrawing) return;                                                        // 그리는 도중에는 바꾸지 않음

        ReleaseTargetBitmap();

        const HRESULT result{ m_swapChain->ResizeBuffers(0, static_cast<UINT>(pixelWidth), static_cast<UINT>(pixelHeight), DXGI_FORMAT_UNKNOWN, 0) };
        if (IsDeviceLost(result))
        {
            HandleDeviceLost();
            return;
        }
        ThrowIfFailed(result, "스왑체인 크기 변경");

        CreateTargetBitmap();
    }

    void DX11Renderer::Impl::SetDPI(float dpi)
    {
        if (dpi == m_dpi) return;

        // 그리기 대상은 만들 때의 배율을 들고 있으므로 새 배율로 다시 만듦
        m_dpi = dpi;
        ReleaseTargetBitmap();
        CreateTargetBitmap();
    }

    float DX11Renderer::Impl::GetWidth() const noexcept
    {
        return static_cast<float>(m_pixelWidth) * DEFAULT_DPI / m_dpi;
    }

    float DX11Renderer::Impl::GetHeight() const noexcept
    {
        return static_cast<float>(m_pixelHeight) * DEFAULT_DPI / m_dpi;
    }

    void DX11Renderer::Impl::Clear(const Core::Color& color)
    {
        m_d2dContext->Clear(ToD2DColor(color));
    }

    void DX11Renderer::Impl::FillRect(const Core::RectF& rect, const Core::Color& color)
    {
        m_d2dContext->FillRectangle(ToD2DRect(rect), PrepareBrush(color));
    }

    void DX11Renderer::Impl::DrawRect(const Core::RectF& rect, const Core::Color& color, float strokeWidth)
    {
        m_d2dContext->DrawRectangle(ToD2DRect(rect), PrepareBrush(color), strokeWidth);
    }

    void DX11Renderer::Impl::DrawLine(float startX, float startY, float endX, float endY, const Core::Color& color, float strokeWidth)
    {
        m_d2dContext->DrawLine(D2D1_POINT_2F{ startX, startY }, D2D1_POINT_2F{ endX, endY }, PrepareBrush(color), strokeWidth);
    }

    void DX11Renderer::Impl::DrawString(std::wstring_view text, const Core::RectF& rect, const Core::Color& color, const Core::TextStyle& style)
    {
        IDWriteTextFormat* textFormat{ GetTextFormat(style) };
        textFormat->SetTextAlignment(ToTextAlignment(style.m_align));   // 글꼴은 공유하므로 그릴 때마다 정렬을 맞춤

        m_d2dContext->DrawText(text.data(), static_cast<UINT32>(text.size()), textFormat, ToD2DRect(rect), PrepareBrush(color),
            D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    ID2D1SolidColorBrush* DX11Renderer::Impl::PrepareBrush(const Core::Color& color)
    {
        m_brush->SetColor(ToD2DColor(color));
        return m_brush.Get();
    }

    // 같은 (크기, 굵기) 글꼴은 한 번만 만들어 재사용 (반환 포인터는 캐시가 소유)
    IDWriteTextFormat* DX11Renderer::Impl::GetTextFormat(const Core::TextStyle& style)
    {
        const std::pair key{ static_cast<int>(std::lround(style.m_size * FONT_SIZE_KEY_SCALE)), style.m_weight };
        if (const auto found{ m_textFormats.find(key) }; found != m_textFormats.end()) return found->second.Get();

        COMPtr<IDWriteTextFormat> textFormat;
        ThrowIfFailed(m_writeFactory->CreateTextFormat(Theme::FONT_FAMILY, nullptr, ToFontWeight(style.m_weight),
            DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, style.m_size, Theme::FONT_LOCALE,
            textFormat.ReleaseAndGetAddressOf()), "글꼴 생성");

        textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);   // 사각형 안에서 세로 가운데
        textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);              // 한 줄로만

        IDWriteTextFormat* rawFormat{ textFormat.Get() };
        m_textFormats.emplace(key, std::move(textFormat));
        return rawFormat;
    }

    // ===== 공개 인터페이스 (구현은 Impl에 위임) =====
    DX11Renderer::DX11Renderer(void* windowHandle, float dpi)
        : m_impl{ std::make_unique<Impl>(static_cast<HWND>(windowHandle), dpi) }
    {}

    DX11Renderer::~DX11Renderer() = default;

    bool DX11Renderer::BeginFrame()
    {
        return m_impl->BeginFrame();
    }

    void DX11Renderer::EndFrame()
    {
        m_impl->EndFrame();
    }

    void DX11Renderer::Resize(int pixelWidth, int pixelHeight)
    {
        m_impl->Resize(pixelWidth, pixelHeight);
    }

    void DX11Renderer::SetDPI(float dpi)
    {
        m_impl->SetDPI(dpi);
    }

    float DX11Renderer::GetWidth() const noexcept
    {
        return m_impl->GetWidth();
    }

    float DX11Renderer::GetHeight() const noexcept
    {
        return m_impl->GetHeight();
    }

    void DX11Renderer::Clear(const Core::Color& color)
    {
        m_impl->Clear(color);
    }

    void DX11Renderer::FillRect(const Core::RectF& rect, const Core::Color& color)
    {
        m_impl->FillRect(rect, color);
    }

    void DX11Renderer::DrawRect(const Core::RectF& rect, const Core::Color& color, float strokeWidth)
    {
        m_impl->DrawRect(rect, color, strokeWidth);
    }

    void DX11Renderer::DrawLine(float startX, float startY, float endX, float endY, const Core::Color& color, float strokeWidth)
    {
        m_impl->DrawLine(startX, startY, endX, endY, color, strokeWidth);
    }

    void DX11Renderer::DrawString(std::wstring_view text, const Core::RectF& rect, const Core::Color& color, const Core::TextStyle& style)
    {
        m_impl->DrawString(text, rect, color, style);
    }
}