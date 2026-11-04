module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define NOUSER   // winuser.h 제외: GetClassName 같은 매크로 충돌 방지
#include <Windows.h>

module LostArkMultiTool.Core.TextEncoding;

import std;

namespace LostArkMultiTool::Core::TextEncoding
{
    namespace
    {
        constexpr std::string_view CONVERSION_ERROR_MESSAGE{ "문자 변환에 실패했습니다." };
    }

    std::wstring UTF8ToWide(std::string_view text)
    {
        if (text.empty()) return {};

        const int inputLength{ static_cast<int>(text.size()) };
        const int wideLength{ ::MultiByteToWideChar(CP_UTF8, 0, text.data(), inputLength, nullptr, 0) };
        if (wideLength == 0) throw std::runtime_error{ std::string{ CONVERSION_ERROR_MESSAGE } };

        std::wstring wide(static_cast<std::size_t>(wideLength), L'\0');
        ::MultiByteToWideChar(CP_UTF8, 0, text.data(), inputLength, wide.data(), wideLength);
        return wide;
    }

    std::string WideToUTF8(std::wstring_view text)
    {
        if (text.empty()) return {};

        const int inputLength{ static_cast<int>(text.size()) };
        const int utf8Length{ ::WideCharToMultiByte(CP_UTF8, 0, text.data(), inputLength, nullptr, 0, nullptr, nullptr) };
        if (utf8Length == 0) throw std::runtime_error{ std::string{ CONVERSION_ERROR_MESSAGE } };

        std::string utf8(static_cast<std::size_t>(utf8Length), '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, text.data(), inputLength, utf8.data(), utf8Length, nullptr, nullptr);
        return utf8;
    }
}