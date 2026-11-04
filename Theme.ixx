export module LostArkMultiTool.Views.Theme;

import LostArkMultiTool.Core.Renderer;

// 다크 테마 색 · 글꼴 (원본 GridTheme와 같은 색 체계 → 탭 · 표 · 그래프가 한 가지 톤으로 보이게)
// 상수만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Views::Theme
{
    // ===== 기본 색 (원본 GridTheme) =====
    inline constexpr Core::Color BACKGROUND_COLOR{ Core::MakeColor(28, 28, 32) };        // 빈 공간 (그래프 배경과 동일)
    inline constexpr Core::Color CELL_BACK_COLOR{ Core::MakeColor(36, 36, 42) };         // 일반 칸
    inline constexpr Core::Color HEADER_BACK_COLOR{ Core::MakeColor(48, 48, 56) };       // 머리글 · 탭 막대
    inline constexpr Core::Color GRID_LINE_COLOR{ Core::MakeColor(60, 60, 68) };         // 경계선
    inline constexpr Core::Color TEXT_COLOR{ Core::MakeColor(220, 220, 220) };           // 일반 글자 (Gainsboro)
    inline constexpr Core::Color HEADER_TEXT_COLOR{ Core::MakeColor(255, 255, 255) };    // 머리글 · 선택된 탭 글자
    inline constexpr Core::Color SELECTION_BACK_COLOR{ Core::MakeColor(70, 82, 110) };   // 선택 · 마우스를 올린 탭
    inline constexpr Core::Color MUTED_TEXT_COLOR{ Core::MakeColor(140, 140, 150) };     // 보조 글자 · 선택 안 된 탭
    inline constexpr Core::Color TOTAL_TEXT_COLOR{ Core::MakeColor(128, 222, 234) };     // 강조 (합계 · 탭 밑줄)

    // ===== 글꼴 =====
    inline constexpr const wchar_t* FONT_FAMILY{ L"Malgun Gothic" };   // 맑은 고딕 (원본 표 글꼴)
    inline constexpr const wchar_t* FONT_LOCALE{ L"ko-KR" };
    inline constexpr float FONT_SIZE_NORMAL{ 12.0f };   // 원본 9pt ≈ 12 DIP
    inline constexpr float FONT_SIZE_TAB{ 14.0f };
    inline constexpr float FONT_SIZE_TITLE{ 22.0f };
}