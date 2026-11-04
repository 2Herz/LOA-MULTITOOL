export module LostArkMultiTool.Core.NumberFormat;

import std;

// 화면에 보여 줄 숫자 형식 (원본 C# "N0" · "N2" 형식과 같은 모양)
export namespace LostArkMultiTool::Core::NumberFormat
{
    // 천 단위 콤마 + 소수 자릿수 고정 (예: 1234567.891 → decimals 0: "1,234,568", decimals 2: "1,234,567.89")
    [[nodiscard]] std::wstring FormatNumber(double value, int decimals);

    // 콤마 · 공백을 빼고 숫자로 바꿈 (읽을 수 없으면 0, 원본 GetSafeDouble과 같음)
    [[nodiscard]] double ParseNumber(std::wstring_view text) noexcept;
}