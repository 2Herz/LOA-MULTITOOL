export module LostArkMultiTool.Models.RefineBookQuote;

import std;
import LostArkMultiTool.Models.MarketItem;

// 강화책 하나의 시세 (거래소 조회 결과 + 직전 갱신 때의 최저가)
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Models
{
    // 강화책 종류 (표를 나누는 기준)
    enum class RefineBookKind
    {
        BOOK_METALLURGY,   // 야금술 (무기)
        BOOK_TAILORING     // 재봉술 (방어구)
    };

    struct RefineBookQuote
    {
        RefineBookKind m_kind{ RefineBookKind::BOOK_METALLURGY };
        MarketItem m_item;
        std::optional<double> m_previousMinPrice;   // 직전 갱신 때의 현재 최저가 (처음 조회했으면 nullopt)
    };
}
