export module LostArkMultiTool.Models.AuctionOption;

import std;

// 경매장 검색 옵션 (Open API /auctions/options 응답의 EtcOptions → 악세 검색기의 연마 효과 선택지)
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Models
{
    // 연마 효과 수치 단계 (상 = 노랑 / 중 = 보라 / 하 = 파랑)
    enum class PolishGrade
    {
        GRADE_HIGH,
        GRADE_MIDDLE,
        GRADE_LOW
    };

    // 옵션 값 하나 (예: 표시 "0.60%", 요청 값 60)
    struct AuctionOptionValue
    {
        std::string m_displayText;            // 화면에 보여 줄 글자 (UTF-8)
        int m_value{};                        // 검색 요청에 넣을 값 (MinValue)
        std::optional<PolishGrade> m_grade;   // 연마 효과 상 · 중 · 하 (해당 없으면 nullopt)
    };

    // 검색 옵션 하나 (예: 연마 효과 → 추가 피해)
    struct AuctionOption
    {
        int m_firstOption{};                        // 옵션 묶음 코드 (EtcOptions.Value, 예: 연마 효과)
        int m_secondOption{};                       // 옵션 코드 (EtcSubs.Value, 예: 추가 피해)
        std::string m_groupName;                    // 옵션 묶음 이름 (UTF-8, 예: "연마 효과")
        std::string m_name;                         // 옵션 이름 (UTF-8)
        std::vector<AuctionOptionValue> m_values;   // 고를 수 있는 값 (API가 주지 않으면 비어 있음 → 값 조건 없이 검색)
    };
}
