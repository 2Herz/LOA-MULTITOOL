export module LostArkMultiTool.Core.TextEncoding;

import std;

// 문자열 인코딩 변환 (프로그램 안 문자열은 UTF-8, Windows API는 UTF-16)
// 여러 곳(API 통신 · 파일 경로 · 오류 창)이 같은 변환을 쓰도록 한 곳에 모음
export namespace LostArkMultiTool::Core::TextEncoding
{
    // UTF-8 → UTF-16 (변환할 수 없으면 std::runtime_error)
    [[nodiscard]] std::wstring UTF8ToWide(std::string_view text);

    // UTF-16 → UTF-8 (변환할 수 없으면 std::runtime_error)
    [[nodiscard]] std::string WideToUTF8(std::wstring_view text);
}