export module LostArkMultiTool.Models.GemQuote;

import std;
import LostArkMultiTool.Models.GemCatalog;

export namespace LostArkMultiTool::Models
{
    // 보석 하나의 현재 시세 (원본 GemQuote)
    class GemQuote
    {
    public:
        explicit GemQuote(GemSelection gem);

        [[nodiscard]] const GemSelection& GetGem() const noexcept;

        // 현재 최저가 (nullopt = 즉시 구매 가능한 매물 없음)
        [[nodiscard]] const std::optional<std::int64_t>& GetPrice() const noexcept;
        void SetPrice(std::optional<std::int64_t> price) noexcept;

        // 직전 "전체 갱신" 때의 가격 (등락 기준)
        [[nodiscard]] const std::optional<std::int64_t>& GetPreviousPrice() const noexcept;
        void SetPreviousPrice(std::optional<std::int64_t> previousPrice) noexcept;

        // 가장 최근 전체 갱신 때의 가격 (다음 전체 갱신의 비교 기준)
        [[nodiscard]] const std::optional<std::int64_t>& GetLastFullPrice() const noexcept;
        void SetLastFullPrice(std::optional<std::int64_t> lastFullPrice) noexcept;

        // 이번 실행에서 한 번이라도 조회했는지 ("-" / "매물 없음" 구분)
        [[nodiscard]] bool IsQueried() const noexcept;
        void SetQueried(bool isQueried) noexcept;

        // 등락폭 = 현재가 − 직전 전체 갱신가 (둘 중 하나라도 없으면 nullopt)
        [[nodiscard]] std::optional<std::int64_t> GetDiff() const noexcept;

    private:
        GemSelection m_gem;
        std::optional<std::int64_t> m_price;
        std::optional<std::int64_t> m_previousPrice;
        std::optional<std::int64_t> m_lastFullPrice;
        bool m_isQueried{};
    };
}