module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOUSER   // winuser.h 제외: GetClassName 같은 매크로 충돌 방지
#include <Windows.h>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

module LostArkMultiTool.Services.WinHTTPClient;

import std;

namespace LostArkMultiTool::Services
{
    namespace
    {
        constexpr const wchar_t* USER_AGENT{ L"LostArkMultiTool" };

        constexpr int HTTP_STATUS_SUCCESS_MIN{ 200 };
        constexpr int HTTP_STATUS_SUCCESS_MAX{ 299 };

        constexpr std::string_view TIMEOUT_MESSAGE{ "API 응답 시간이 초과되었습니다." };

        // WinHTTP 핸들 전용 RAII 래퍼 (소멸 시 WinHttpCloseHandle 호출, 복사 금지 · 이동 허용)
        // unique_ptr의 조건부 생성자(삭제기 기본 생성 검사)에 기대지 않고 핸들 수명을 직접 관리
        class WinHTTPHandle
        {
        public:
            explicit WinHTTPHandle(HINTERNET handle) noexcept
                : m_handle{ handle }
            {}

            ~WinHTTPHandle()
            {
                Close();
            }

            WinHTTPHandle(const WinHTTPHandle&) = delete;
            WinHTTPHandle& operator=(const WinHTTPHandle&) = delete;

            WinHTTPHandle(WinHTTPHandle&& other) noexcept
                : m_handle{ std::exchange(other.m_handle, nullptr) }
            {}

            WinHTTPHandle& operator=(WinHTTPHandle&& other) noexcept
            {
                if (this != &other)
                {
                    Close();
                    m_handle = std::exchange(other.m_handle, nullptr);
                }
                return *this;
            }

            [[nodiscard]] HINTERNET Get() const noexcept
            {
                return m_handle;
            }

        private:
            void Close() noexcept
            {
                if (m_handle != nullptr)
                {
                    ::WinHttpCloseHandle(m_handle);
                    m_handle = nullptr;
                }
            }

            HINTERNET m_handle{ nullptr };
        };

        [[noreturn]] void ThrowNetworkError()
        {
            const DWORD errorCode{ ::GetLastError() };
            if (errorCode == ERROR_WINHTTP_TIMEOUT) throw std::runtime_error{ std::string{ TIMEOUT_MESSAGE } };
            throw std::runtime_error{ std::format("API 네트워크 오류 (코드 {})", errorCode) };
        }

        WinHTTPHandle MakeHandle(HINTERNET handle)
        {
            if (handle == nullptr) ThrowNetworkError();
            return WinHTTPHandle{ handle };
        }
    }

    // ===== 실제 WinHTTP 통신 구현 =====
    class WinHTTPClient::Impl
    {
    public:
        Impl(std::wstring_view host, int timeoutMs);

        [[nodiscard]] HTTPResponse Send(std::wstring_view verb, std::wstring_view path, std::wstring_view headers, std::string body);

    private:
        WinHTTPHandle m_session;
        WinHTTPHandle m_connection;
    };

    WinHTTPClient::Impl::Impl(std::wstring_view host, int timeoutMs)
        : m_session{ MakeHandle(::WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)) }
        , m_connection{ MakeHandle(::WinHttpConnect(m_session.Get(), std::wstring{ host }.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0)) }
    {
        // 세션에 건 시간 제한은 이후에 만드는 모든 요청에 적용됨
        if (!::WinHttpSetTimeouts(m_session.Get(), timeoutMs, timeoutMs, timeoutMs, timeoutMs))
        {
            ThrowNetworkError();
        }
    }

    HTTPResponse WinHTTPClient::Impl::Send(std::wstring_view verb, std::wstring_view path, std::wstring_view headers, std::string body)
    {
        // WinHTTP는 널 종료 문자열을 받으므로 복사본을 만듦
        const std::wstring verbText{ verb };
        const std::wstring pathText{ path };
        const std::wstring headersText{ headers };

        WinHTTPHandle request{ MakeHandle(::WinHttpOpenRequest(m_connection.Get(), verbText.c_str(), pathText.c_str(), nullptr,
                                                               WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                               WINHTTP_FLAG_SECURE)) };

        // 헤더·본문이 없으면 nullptr (WINHTTP_NO_ADDITIONAL_HEADERS · WINHTTP_NO_REQUEST_DATA와 같은 값)
        const DWORD bodyLength{ static_cast<DWORD>(body.size()) };
        if (!::WinHttpSendRequest(request.Get(),
            headersText.empty() ? nullptr : headersText.c_str(), static_cast<DWORD>(headersText.size()),
            body.empty() ? nullptr : body.data(), bodyLength, bodyLength, 0))
        {
            ThrowNetworkError();
        }

        if (!::WinHttpReceiveResponse(request.Get(), nullptr)) ThrowNetworkError();

        DWORD statusCode{};
        DWORD statusSize{ static_cast<DWORD>(sizeof(statusCode)) };
        if (!::WinHttpQueryHeaders(request.Get(), WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX))
        {
            ThrowNetworkError();
        }

        // 응답 본문을 남은 만큼 전부 읽음
        std::string responseBody;
        DWORD available{};
        while (true)
        {
            if (!::WinHttpQueryDataAvailable(request.Get(), &available)) ThrowNetworkError();
            if (available == 0) break;

            const std::size_t previousSize{ responseBody.size() };
            responseBody.resize(previousSize + available);

            DWORD readSize{};
            if (!::WinHttpReadData(request.Get(), responseBody.data() + previousSize, available, &readSize))
            {
                ThrowNetworkError();
            }
            responseBody.resize(previousSize + readSize);
        }

        return HTTPResponse{ static_cast<int>(statusCode), std::move(responseBody) };
    }

    // ===== 공개 인터페이스 (구현은 Impl에 위임) =====
    WinHTTPClient::WinHTTPClient(std::wstring_view host, int timeoutMs)
        : m_impl{ std::make_unique<Impl>(host, timeoutMs) }
    {}

    WinHTTPClient::~WinHTTPClient() = default;

    HTTPResponse WinHTTPClient::Send(std::wstring_view verb, std::wstring_view path, std::wstring_view headers, std::string body)
    {
        return m_impl->Send(verb, path, headers, std::move(body));
    }

    bool WinHTTPClient::IsSuccessStatus(int statusCode) noexcept
    {
        return statusCode >= HTTP_STATUS_SUCCESS_MIN && statusCode <= HTTP_STATUS_SUCCESS_MAX;
    }
}