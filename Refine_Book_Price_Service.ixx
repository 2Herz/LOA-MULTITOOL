export module LostArkMultiTool.Core.RefineBookPriceService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.MarketItem;
import LostArkMultiTool.Models.RefineBookQuote;

export namespace LostArkMultiTool::Core
{
    // 강화책(야금술 · 재봉술) 시세 조회 · 반영 (강화책 검색기의 모델)
    // 스레드 규칙: FetchQuotes만 백그라운드 스레드에서 호출 가능, 나머지는 화면 스레드에서만 호출
    class RefineBookPriceService
    {
    public:
        virtual ~RefineBookPriceService();

        // 마지막 조회 결과 (종류별로 게임 아이템 사전 순서: 장인의 1 · 2단계 → 업화 → 강화 업화 → 장인의 3단계 → 전율 → 장인의 4단계, 그 밖의 책은 제외)
        [[nodiscard]] virtual const std::vector<Models::RefineBookQuote>& GetQuotes() const noexcept = 0;
        [[nodiscard]] virtual const std::optional<std::int64_t>& GetLastUpdated() const noexcept = 0;   // 마지막 갱신 시각 (UTC 초)

        // 거래소에서 야금술 · 재봉술 조회 (데이터를 건드리지 않아 백그라운드 스레드에서 호출해도 안전)
        [[nodiscard]] virtual std::vector<Models::MarketItem> FetchQuotes(MarketAPI& api) const = 0;

        // 조회 결과 반영 (종류 분류 · 순서 정리, 이전 최저가를 등락 기준으로 넘김)
        virtual void ApplyQuotes(std::vector<Models::MarketItem> items) = 0;
    };
}
