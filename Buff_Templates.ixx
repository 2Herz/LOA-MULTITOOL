export module LostArkMultiTool.Vision.BuffTemplates;  // 버프 타이머 템플릿 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기

// 버프 타이머가 쓰는 템플릿 (tools/Make-BuffTemplates.py가 샘플에서 만든 데이터, Buff_Templates.cpp)
// - 아이콘: 버프 · 디버프 아이콘 안쪽(테두리 뺀 부분)을 16x16 RGB로 줄인 그림 (상자 평균)
// - 남은 시간 글자: 아이콘 아래 "18초" · "0.7초" · "5분"의 글자 모양 ("#" = 글자, "." = 바탕, 's' = 초, 'm' = 분)
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 버프 타이머가 알아보는 버프 · 디버프 종류
    enum class BuffKind
    {
        BUFF_GRENADE,             // 암흑 수류탄 (보스 디버프)
        BUFF_ADRENALINE,          // 아드로핀 (내 버프)
        BUFF_SUPPORT_AWAKENING,   // 서폿 초각성기 (보라)
        BUFF_SUPPORT_HYPER,       // 서폿 초각성 스킬 (파랑)
        BUFF_SUPPORT_ATTACK,      // 서폿 공격력 증가 (빨강)
        BUFF_SUPPORT_IDENTITY     // 서폿 아이덴티티 (노랑)
    };

    inline constexpr std::size_t BUFF_KIND_COUNT{ 6 };   // 종류 수 (패널 칸 수)

    // 아이콘 템플릿 하나
    struct BuffIconTemplate
    {
        BuffKind m_kind{};                     // 종류
        int m_width{};                         // 가로 (16)
        int m_height{};                        // 세로 (16)
        std::span<const std::uint8_t> m_rgb;   // 픽셀 (R · G · B 순서, 줄 단위)
    };

    // 남은 시간 글자 템플릿 하나
    struct BuffGlyphTemplate
    {
        char m_symbol{};             // 글자 ('0' ~ '9' · 's' 초 · 'm' 분)
        int m_width{};               // 가로
        int m_height{};              // 세로
        std::string_view m_bits;     // 모양 ("#" · ".", 가로 × 세로 글자)
    };

    [[nodiscard]] std::span<const BuffIconTemplate> GetBuffIcons() noexcept;     // 아이콘 목록
    [[nodiscard]] std::span<const BuffGlyphTemplate> GetBuffGlyphs() noexcept;   // 글자 목록
}
