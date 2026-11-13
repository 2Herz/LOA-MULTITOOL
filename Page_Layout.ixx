export module LostArkMultiTool.Views.PageLayout;

import std;
import LostArkMultiTool.Core.Renderer;

// 새 디자인(피그마) 공통 화면 틀: 가운데 정렬 본문 열 · 카드 · 카드 머리말 · 통계 카드 · 표 아래 줄 · 추이 선
// 탭마다 같은 모양을 쓰도록 그리기 · 배치 계산만 모음 (상태 없음 → 함수 모음)
export namespace LostArkMultiTool::Views::PageLayout
{
    // ===== 크기 (DIP) =====
    inline constexpr float PAGE_MAX_WIDTH{ 1440.0f };     // 본문 열 최대 너비 (넓은 창에서는 가운데 정렬)
    inline constexpr float CARD_GAP{ 12.0f };             // 카드 사이 간격
    inline constexpr float CARD_PADDING{ 16.0f };         // 카드 안쪽 여백
    inline constexpr float CARD_RADIUS{ 10.0f };          // 카드 모서리
    inline constexpr float CARD_HEADER_HEIGHT{ 56.0f };   // 카드 머리말 (영문 소제목 + 제목) 높이
    inline constexpr float STAT_CARD_HEIGHT{ 78.0f };     // 통계 카드 높이
    inline constexpr float FILTER_BAR_HEIGHT{ 52.0f };    // 검색 · 분류 · 정렬 줄 카드 높이
    inline constexpr float CONTROL_HEIGHT{ 32.0f };       // 카드 안 입력칸 · 버튼 높이
    inline constexpr float TABLE_FOOTER_HEIGHT{ 34.0f };  // 표 아래 "총 N개" 줄 높이
    inline constexpr float GRID_ROW_HEIGHT{ 40.0f };      // 새 디자인 표 행 높이 (배지 · 추이 선이 들어가는 표)

    // 통계 카드 한 장의 내용
    struct StatCard
    {
        std::wstring m_icon;          // 아이콘 글자 (예: L"▤", L"↗", L"◎", L"◷")
        Core::Color m_iconColor;      // 아이콘 · 아이콘 바탕 색
        std::wstring m_label;         // 작은 회색 이름 (예: "평균 시세 변동")
        std::wstring m_value;         // 큰 값 (예: "+2.4%")
        std::wstring m_sub;           // 값 오른쪽 작은 글자 (예: "최근 7일", 비우면 없음)
        Core::Color m_subColor;       // 작은 글자 색 (등락이면 상승 · 하락 색)
    };

    // 본문 영역 → 가운데 정렬된 열 (최대 PAGE_MAX_WIDTH, 좁으면 그대로)
    [[nodiscard]] Core::RectF GetPageColumn(const Core::RectF& area) noexcept;

    // 가로로 count칸 나누기 (사이 간격 gap, weights가 비어 있으면 같은 너비)
    [[nodiscard]] std::vector<Core::RectF> SplitColumns(const Core::RectF& area, std::size_t count, float gap, std::span<const float> weights = {});

    // 위에서부터 height만큼 떼어 냄 → 떼어 낸 영역, area는 남은 아래쪽(간격 gap 포함)으로 줄어듦
    [[nodiscard]] Core::RectF TakeTop(Core::RectF& area, float height, float gap = CARD_GAP) noexcept;

    // 카드 바탕 + 테두리
    void DrawCard(Core::Renderer& renderer, const Core::RectF& rect);

    // 카드 + 머리말 (청록 영문 소제목 · 제목 · 오른쪽 글자/LIVE 점 · 아래 구분선) → 머리말 아래 본문 영역
    Core::RectF DrawCardWithHeader(Core::Renderer& renderer, const Core::RectF& rect, std::wstring_view eyebrow, std::wstring_view title,
        std::wstring_view rightText = {}, bool isLive = false);

    // 통계 카드 한 장 (아이콘 상자 · 이름 · 큰 값 · 작은 글자)
    void DrawStatCard(Core::Renderer& renderer, const Core::RectF& rect, const StatCard& card);

    // 표 아래 줄: 왼쪽 "총 N개의 검색 결과" · 오른쪽 기준 설명 (회색 작은 글자)
    void DrawTableFooter(Core::Renderer& renderer, const Core::RectF& rect, std::wstring_view leftText, std::wstring_view rightText);

    // 데이터가 없을 때 가운데 안내 (제목 + 해결 방법)
    void DrawEmptyState(Core::Renderer& renderer, const Core::RectF& rect, std::wstring_view title, std::wstring_view hint);
}
