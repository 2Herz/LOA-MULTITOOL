module LostArkMultiTool.Core.NumberFormat;

import std;

namespace LostArkMultiTool::Core::NumberFormat
{
    namespace
    {
        constexpr int MIN_DECIMALS{ 0 };
        constexpr int MAX_DECIMALS{ 6 };
        constexpr std::size_t DIGIT_GROUP_SIZE{ 3 };      // 천 단위
        constexpr char DECIMAL_POINT{ '.' };
        constexpr char MINUS_SIGN{ '-' };
        constexpr std::size_t SIGN_LENGTH{ 1 };
        constexpr std::string_view NEGATIVE_ZERO_CHARACTERS{ "-0." };   // 이 글자만 있으면 "-0" · "-0.00" 같은 음의 0
        constexpr wchar_t GROUP_SEPARATOR{ L',' };
        constexpr std::wstring_view ZERO_TEXT{ L"0" };
        constexpr std::size_t MAX_NUMBER_LENGTH{ 64 };    // 이보다 긴 글자는 숫자로 보지 않음

        // 숫자를 읽을 때 건너뛸 글자 (콤마 · 일반 공백 · 전각 공백)
        constexpr std::wstring_view IGNORED_CHARACTERS{ L", \u3000" };
        constexpr wchar_t MAX_ASCII_CHARACTER{ 0x7F };
    }

    std::wstring FormatNumber(double value, int decimals)
    {
        if (!std::isfinite(value)) return std::wstring{ ZERO_TEXT };

        const int clampedDecimals{ std::clamp(decimals, MIN_DECIMALS, MAX_DECIMALS) };
        std::string fixedText{ std::format("{:.{}f}", value, clampedDecimals) };

        // 반올림 결과가 0이면 "-0" 대신 "0" (예: -0.4 → "0")
        const bool isNegative{ !fixedText.empty() && fixedText.front() == MINUS_SIGN };
        if (isNegative && fixedText.find_first_not_of(NEGATIVE_ZERO_CHARACTERS) == std::string::npos) fixedText.erase(0, SIGN_LENGTH);

        const bool hasSign{ !fixedText.empty() && fixedText.front() == MINUS_SIGN };
        const std::size_t integerStart{ hasSign ? SIGN_LENGTH : std::size_t{} };
        const std::size_t pointPosition{ fixedText.find(DECIMAL_POINT) };
        const std::size_t integerEnd{ pointPosition == std::string::npos ? fixedText.size() : pointPosition };

        std::wstring result;
        result.reserve(fixedText.size() + fixedText.size() / DIGIT_GROUP_SIZE);
        if (hasSign) result.push_back(static_cast<wchar_t>(MINUS_SIGN));

        // 정수 부분: 오른쪽에서 세 자리마다 콤마
        const std::size_t integerLength{ integerEnd - integerStart };
        for (std::size_t index{}; index < integerLength; ++index)
        {
            if (index > 0 && (integerLength - index) % DIGIT_GROUP_SIZE == 0) result.push_back(GROUP_SEPARATOR);
            result.push_back(static_cast<wchar_t>(fixedText[integerStart + index]));
        }

        // 소수 부분은 그대로
        for (std::size_t index{ integerEnd }; index < fixedText.size(); ++index) result.push_back(static_cast<wchar_t>(fixedText[index]));
        return result;
    }

    double ParseNumber(std::wstring_view text) noexcept
    {
        // 숫자에 쓰이는 글자만 남김 (영문 · 숫자 외 글자가 섞이면 0)
        std::array<char, MAX_NUMBER_LENGTH> buffer{};
        std::size_t length{};
        for (const wchar_t character : text)
        {
            if (IGNORED_CHARACTERS.find(character) != std::wstring_view::npos) continue;
            if (character > MAX_ASCII_CHARACTER || length >= buffer.size()) return 0.0;

            buffer[length] = static_cast<char>(character);
            ++length;
        }
        if (length == 0) return 0.0;

        double value{};
        const char* const end{ buffer.data() + length };
        const auto [ptr, ec] { std::from_chars(buffer.data(), end, value) };
        if (ec != std::errc{} || ptr != end || !std::isfinite(value)) return 0.0;
        return value;
    }
}