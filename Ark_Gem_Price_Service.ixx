export module LostArkMultiTool.Core.ArkGemPriceService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.ArkGemQuote;
import LostArkMultiTool.Models.MarketItem;

export namespace LostArkMultiTool::Core
{
    // 아크 그리드 젬(질서 · 혼돈) 시세 조회 · 반영 (젬 시세 검색기의 모델)
    // 스레드 규칙: FetchQuotes만 백그라운드 스레드에서 호출 가능, 나머지는 화면 스레드에서만 호출
    class ArkGemPriceService
    {
    public:
        virtual ~ArkGemPriceService();

        // 마지막 조회 결과 (종류별로 정해 둔 순서: 질서 = 안정 · 불변 · 견고, 혼돈 = 침식 · 왜곡 · 붕괴 → 그 밖)
        [[nodiscard]] virtual const std::vector<Models::ArkGemQuote>& GetQuotes() const noexcept = 0;
        [[nodiscard]] virtual const std::optional<std::int64_t>& GetLastUpdated() const noexcept = 0;   // 마지막 갱신 시각 (UTC 초)

        // 거래소에서 질서 · 혼돈의 젬 조회 (모든 등급, 데이터를 건드리지 않아 백그라운드 스레드에서 호출해도 안전)
        [[nodiscard]] virtual std::vector<Models::MarketItem> FetchQuotes(MarketAPI& api) const = 0;

        // 조회 결과 반영 (종류 분류 · 순서 정리, 이전 최저가를 등락 기준으로 넘김)
        virtual void ApplyQuotes(std::vector<Models::MarketItem> items) = 0;
    };
}
