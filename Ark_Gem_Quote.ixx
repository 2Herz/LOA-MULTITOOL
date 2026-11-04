export module LostArkMultiTool.Models.ArkGemQuote;

import std;
import LostArkMultiTool.Models.MarketItem;

// 아크 그리드 젬 하나의 시세 (거래소 조회 결과 + 직전 갱신 때의 최저가)
// 타입 정의만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Models
{
    // 젬 종류 (표를 나누는 기준)
    enum class ArkGemKind
    {
        GEM_ORDER,   // 질서의 젬 (안정 · 불변 · 견고)
        GEM_CHAOS    // 혼돈의 젬 (침식 · 왜곡 · 붕괴)
    };

    struct ArkGemQuote
    {
        ArkGemKind m_kind{ ArkGemKind::GEM_ORDER };
        MarketItem m_item;
        std::optional<double> m_previousMinPrice;   // 직전 갱신 때의 현재 최저가 (처음 조회했으면 nullopt)
    };
}
