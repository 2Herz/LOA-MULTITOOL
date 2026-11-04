export module LostArkMultiTool.Models.AccessoryListing;

import std;

// 경매장 악세 매물 (Open API /auctions/items 응답의 Items 한 칸 + 페이지 정보)
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Models
{
    // 매물에 붙은 옵션 한 줄 (예: 추가 피해 +0.60%)
    struct AccessoryOptionLine
    {
        std::string m_type;          // 옵션 종류 (예: ACCESSORY_UPGRADE = 연마 효과, STAT = 기본 스탯)
        std::string m_name;          // 옵션 이름 (UTF-8)
        double m_value{};
        bool m_isPercentage{};       // 값이 % 단위인지
    };

    struct AccessoryListing
    {
        std::string m_name;                           // 아이템 이름 (UTF-8)
        std::string m_gradeName;                      // 등급 (UTF-8)
        int m_quality{};                              // 품질 (0 ~ 100)
        std::vector<AccessoryOptionLine> m_options;
        std::optional<std::int64_t> m_buyPrice;       // 즉시 구매가 (입찰 전용이면 nullopt)
        std::int64_t m_bidStartPrice{};               // 최소 입찰가
        std::string m_endDate;                        // 판매 마감 (API 형식 "yyyy-MM-ddTHH:mm:ss...")
        std::optional<int> m_tradeAllowCount;         // 남은 거래 가능 횟수
    };

    // 검색 결과 한 페이지
    struct AccessorySearchPage
    {
        std::vector<AccessoryListing> m_listings;
        int m_page{};             // 이번 페이지 번호 (1부터)
        int m_pageSize{};         // 한 페이지 매물 수 (보통 10)
        int m_totalCount{};       // 조건에 맞는 전체 매물 수
    };
}
