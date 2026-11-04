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

        // 헤더 원문 해석
        constexpr std::wstring_view HEADER_LINE_END{ L"\r\n" };
        constexpr wchar_t HEADER_NAME_SEPARATOR{ L':' };
        constexpr std::wstring_view HEADER_SPACES{ L" \t" };

        // SYSTEMTIME(UTC) → UTC 초 변환
        constexpr std::int64_t FILETIME_TICKS_PER_SECOND{ 10'000'000 };        // FILETIME 단위 = 100ns
        constexpr std::int64_t UNIX_EPOCH_FILETIME_SECONDS{ 11'644'473'600 };   // 1601-01-01 → 1970-01-01 (초)
        constexpr unsigned DWORD_BITS{ 32 };

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
            if (errorCode == ERROR_WINHTTP_TIMEOUT) throw HTTPNetworkError{ std::string{ TIMEOUT_MESSAGE } };
            throw HTTPNetworkError{ std::format("API 네트워크 오류 (코드 {})", errorCode) };
        }

        WinHTTPHandle MakeHandle(HINTERNET handle)
        {
            if (handle == nullptr) ThrowNetworkError();
            return WinHTTPHandle{ handle };
        }

        // 응답 헤더 원문 (읽지 못하면 빈 문자열 → 헤더는 참고용이라 실패해도 응답은 그대로 씀)
        std::wstring ReadRawHeaders(HINTERNET request)
        {
            DWORD byteCount{};
            ::WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER,
                &byteCount, WINHTTP_NO_HEADER_INDEX);   // 첫 호출은 필요한 크기만 알려 줌 (버퍼 부족 오류가 정상)
            if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER || byteCount == 0) return {};

            std::wstring headers(byteCount / sizeof(wchar_t), L'\0');
            if (!::WinHttpQueryHeaders(request, WINHTTP_QUERY_RAW_HEADERS_CRLF, WINHTTP_HEADER_NAME_BY_INDEX, headers.data(),
                &byteCount, WINHTTP_NO_HEADER_INDEX))
            {
                return {};
            }
            headers.resize(byteCount / sizeof(wchar_t));   // 끝의 널 문자 제외
            return headers;
        }

        // Date 헤더 → UTC 초 (없거나 형식이 틀리면 nullopt)
        std::optional<std::int64_t> ReadServerTime(HINTERNET request)
        {
            SYSTEMTIME serverTime{};
            DWORD size{ static_cast<DWORD>(sizeof(serverTime)) };
            if (!::WinHttpQueryHeaders(request, WINHTTP_QUERY_DATE | WINHTTP_QUERY_FLAG_SYSTEMTIME, WINHTTP_HEADER_NAME_BY_INDEX,
                &serverTime, &size, WINHTTP_NO_HEADER_INDEX))
            {
                return std::nullopt;
            }

            FILETIME fileTime{};
            if (!::SystemTimeToFileTime(&serverTime, &fileTime)) return std::nullopt;

            const std::uint64_t ticks{ (static_cast<std::uint64_t>(fileTime.dwHighDateTime) << DWORD_BITS) | fileTime.dwLowDateTime };
            return static_cast<std::int64_t>(ticks) / FILETIME_TICKS_PER_SECOND - UNIX_EPOCH_FILETIME_SECONDS;
        }

        // ASCII 대소문자 무시 비교 (헤더 이름은 영문)
        bool EqualsIgnoreCase(std::wstring_view left, std::wstring_view right) noexcept
        {
            return left.size() == right.size()
                && std::equal(left.begin(), left.end(), right.begin(), [](wchar_t a, wchar_t b) { return std::towlower(a) == std::towlower(b); });
        }

        std::wstring_view TrimSpaces(std::wstring_view text) noexcept
        {
            const std::size_t first{ text.find_first_not_of(HEADER_SPACES) };
            if (first == std::wstring_view::npos) return {};
            return text.substr(first, text.find_last_not_of(HEADER_SPACES) - first + 1);
        }
    }

    HTTPNetworkError::HTTPNetworkError(const std::string& message)
        : std::runtime_error{ message }
    {}

    std::optional<std::wstring> FindHeader(const HTTPResponse& response, std::wstring_view name)
    {
        // "이름: 값" 줄을 하나씩 확인 (첫 줄은 "HTTP/1.1 200 OK" 상태 줄이라 구분자가 없어 자연히 건너뜀)
        for (const auto line : std::views::split(std::wstring_view{ response.m_headers }, HEADER_LINE_END))
        {
            const std::wstring_view lineText{ line.begin(), line.end() };
            const std::size_t separator{ lineText.find(HEADER_NAME_SEPARATOR) };
            if (separator == std::wstring_view::npos) continue;

            if (EqualsIgnoreCase(TrimSpaces(lineText.substr(0, separator)), name))
            {
                return std::wstring{ TrimSpaces(lineText.substr(separator + 1)) };
            }
        }
        return std::nullopt;
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

        return HTTPResponse{ static_cast<int>(statusCode), std::move(responseBody), ReadRawHeaders(request.Get()), ReadServerTime(request.Get()) };
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