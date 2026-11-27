export module LostArkMultiTool.Vision.SlotTemplates;  // 슬롯 인식 템플릿 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기

// 창고 · 소지품 슬롯을 마우스 없이 읽을 때 쓰는 템플릿 (tools/Make-SlotTemplates.py가 샘플에서 만든 데이터)
// - 아이콘: 슬롯 숫자 아래쪽 아이콘 부분 RGB (1080p 게임 화면 기준 크기)
// - 숫자 글자: 슬롯 오른쪽 위 흰 숫자 모양 ("#" = 글자, "." = 바탕, 왼쪽 위부터 줄 순서)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 아이콘 템플릿 하나
    struct SlotIconTemplate
    {
        std::string_view m_valueKey;           // 값 이름 ("Yong/level" · 계산기 재료 키 + 구분)
        int m_width{};                         // 가로
        int m_height{};                        // 세로
        std::span<const std::uint8_t> m_rgb;   // 픽셀 (R · G · B 순서, 줄 단위)
    };

    // 숫자 글자 템플릿 하나
    struct SlotGlyphTemplate
    {
        char m_symbol{};             // 글자 ('0' ~ '9' · '+')
        int m_width{};               // 가로
        int m_height{};              // 세로
        std::string_view m_bits;     // 모양 ("#" · ".", 가로 × 세로 글자)
    };

    [[nodiscard]] std::span<const SlotIconTemplate> GetSlotIcons() noexcept;     // 아이콘 목록
    [[nodiscard]] std::span<const SlotGlyphTemplate> GetSlotGlyphs() noexcept;   // 숫자 글자 목록
}
