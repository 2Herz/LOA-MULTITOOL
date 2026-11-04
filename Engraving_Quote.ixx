export module LostArkMultiTool.Models.EngravingQuote;

import std;
import LostArkMultiTool.Models.MarketItem;

export namespace LostArkMultiTool::Models
{
    // 유물 각인서 하나의 시세 (거래소 조회 결과 + 직전 갱신 때의 최저가)
    class EngravingQuote
    {
    public:
        explicit EngravingQuote(MarketItem item);

        [[nodiscard]] const std::string& GetName() const noexcept;
        [[nodiscard]] double GetCurrentMinPrice() const noexcept;     // 현재 최저가 (0 = 매물 없음)
        [[nodiscard]] double GetYesterdayAvgPrice() const noexcept;   // 전일 평균가 (0 = 거래 없음)
        [[nodiscard]] double GetRecentPrice() const noexcept;         // 최근 거래가 (0 = 거래 없음)

        // 직전 갱신 때의 현재 최저가 (등락 기준, 처음 조회했으면 nullopt)
        [[nodiscard]] const std::optional<double>& GetPreviousMinPrice() const noexcept;
        void SetPreviousMinPrice(std::optional<double> previousMinPrice) noexcept;

        // 등락폭 = 현재 최저가 − 직전 갱신 최저가 (둘 중 하나라도 없으면 nullopt)
        [[nodiscard]] std::optional<double> GetDiff() const noexcept;

        // 전일 평균가 대비 변동률(%) (현재 최저가 · 전일 평균가 중 하나라도 없으면 nullopt)
        [[nodiscard]] std::optional<double> GetDayChangeRate() const noexcept;

    private:
        MarketItem m_item;
        std::optional<double> m_previousMinPrice;
    };
}
