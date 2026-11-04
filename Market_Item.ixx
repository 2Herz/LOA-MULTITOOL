export module LostArkMultiTool.Models.MarketItem;

import std;

// 거래소 검색 결과의 아이템 하나 (Open API /markets/items 응답의 Items 한 칸)
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Models
{
    struct MarketItem
    {
        std::string m_name;               // 아이템 이름 (UTF-8, 예: "원한 각인서")
        double m_currentMinPrice{};       // 현재 최저가 (매물이 없으면 0)
        double m_yesterdayAvgPrice{};     // 전일 평균 거래가 (거래가 없었으면 0)
        double m_recentPrice{};           // 최근 거래가 (거래가 없었으면 0)
        std::string m_gradeName;          // 등급 (UTF-8, 예: "유물" — 이름을 등급 색으로 칠할 때 사용)
    };
}
