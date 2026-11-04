export module LostArkMultiTool.Core.EngravingPriceService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.EngravingQuote;
import LostArkMultiTool.Models.MarketItem;

export namespace LostArkMultiTool::Core
{
    // 유물 각인서 시세 조회 · 반영 (유각 시세 검색기의 모델)
    // 스레드 규칙: FetchQuotes만 백그라운드 스레드에서 호출 가능, 나머지는 화면 스레드에서만 호출
    // (보석 검색기와 같은 구조 → 내부 데이터에 잠금이 없어도 데이터 레이스가 없음)
    class EngravingPriceService
    {
    public:
        virtual ~EngravingPriceService();

        [[nodiscard]] virtual const std::vector<Models::EngravingQuote>& GetQuotes() const noexcept = 0;   // 마지막 조회 결과 (거래소 순서)
        [[nodiscard]] virtual const std::optional<std::int64_t>& GetLastUpdated() const noexcept = 0;       // 마지막 갱신 시각 (UTC 초)

        // 거래소에서 유물 각인서 전체 조회 (데이터를 건드리지 않아 백그라운드 스레드에서 호출해도 안전)
        [[nodiscard]] virtual std::vector<Models::MarketItem> FetchQuotes(MarketAPI& api) const = 0;

        // 조회 결과 반영 (이전 최저가를 등락 기준으로 넘김)
        virtual void ApplyQuotes(std::vector<Models::MarketItem> items) = 0;
    };
}
