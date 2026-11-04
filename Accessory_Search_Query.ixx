export module LostArkMultiTool.Models.AccessorySearchQuery;

import std;

// 악세 검색 조건 (경매장 /auctions/items 요청 본문으로 바뀜)
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Models
{
    // 옵션 조건 하나 (minValue가 없으면 옵션이 붙어 있기만 하면 됨)
    struct AccessoryOptionFilter
    {
        int m_firstOption{};
        int m_secondOption{};
        std::optional<int> m_minValue;
    };

    struct AccessorySearchQuery
    {
        int m_categoryCode{};                          // 부위 (목걸이 · 귀걸이 · 반지)
        std::string m_gradeName;                       // 등급 (UTF-8, 고대 · 유물)
        std::optional<int> m_minQuality;               // 최소 품질 (nullopt = 전체)
        std::vector<AccessoryOptionFilter> m_options;  // 연마 효과 조건 (모두 만족)
        int m_page{ 1 };                               // 결과 페이지 (1부터)
    };
}
