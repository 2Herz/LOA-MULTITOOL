export module LostArkMultiTool.Services.AuctionGemPriceService;

import std;
import LostArkMultiTool.Core.GemDataRepository;
import LostArkMultiTool.Core.GemPriceService;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.GemQuote;
import LostArkMultiTool.Models.PriceAlarm;
import LostArkMultiTool.Models.RefreshSettings;

export namespace LostArkMultiTool::Services
{
    // 경매장 시세로 보석 최저가 · 목표가 알림 · 일별 기록을 관리 (원본 GemPriceService)
    // 스레드 규칙은 Core::GemPriceService 주석 참고 (FetchPrices만 백그라운드, 나머지는 화면 스레드)
    class AuctionGemPriceService final : public Core::GemPriceService
    {
    public:
        explicit AuctionGemPriceService(Core::GemDataRepository& repository);

        AuctionGemPriceService(const AuctionGemPriceService&) = delete;
        AuctionGemPriceService& operator=(const AuctionGemPriceService&) = delete;

        [[nodiscard]] const std::map<std::string, Models::GemQuote>& GetQuotes() const noexcept override;
        [[nodiscard]] const std::vector<Models::PriceAlarm>& GetAlarms() const noexcept override;
        [[nodiscard]] const std::optional<std::int64_t>& GetLastFullUpdated() const noexcept override;
        [[nodiscard]] const std::optional<std::int64_t>& GetLastWatchUpdated() const noexcept override;

        [[nodiscard]] std::vector<Models::GemSelection> GetAllTargets() const override;
        [[nodiscard]] std::vector<Models::GemSelection> GetWatchedTargets() const override;

        [[nodiscard]] std::vector<Core::FetchedPrice> FetchPrices(Core::MarketAPI& api, const std::vector<Models::GemSelection>& targets) const override;
        [[nodiscard]] Core::RefreshResult ApplyPrices(const std::vector<Core::FetchedPrice>& prices, bool isFull) override;

        void SetAlarm(const Models::GemSelection& gem, std::int64_t targetPrice) override;
        void RemoveAlarm(std::string_view gemKey) override;

        [[nodiscard]] Models::RefreshSettings GetSettings() const override;
        void UpdateSettings(const Models::RefreshSettings& settings) override;

        [[nodiscard]] const std::map<std::string, std::int64_t>& GetHistory(std::string_view gemKey) const override;

    private:
        // 새 시세 하나를 반영하고 등락 · 기록을 갱신 (저장할 변경이 생겼으면 true)
        [[nodiscard]] bool ApplyPrice(const Core::FetchedPrice& fetched, bool isFull, const std::string& today);

        // 하루에 여러 번 갱신해도 그날의 최저가 하나만 기록 (기록이 바뀌었으면 true)
        [[nodiscard]] bool RecordDailyLowest(const std::string& gemKey, const std::string& day, std::int64_t price);

        // 모든 알림을 판정해서 새로 알릴 것을 reached에 담음 (저장할 상태가 바뀌었으면 true)
        [[nodiscard]] bool EvaluateAlarms(std::vector<Core::ReachedAlarm>& reached);

        // 목표가 이하이고 이번 실행에서 아직 안 알렸으면 true (가격이 다시 오르면 다음 하락 때 또 알림)
        // canNotify = false면 알린 것으로만 표시 (알림 추가 직후 풍선 생략용)
        [[nodiscard]] bool UpdateAlarmState(Models::PriceAlarm& alarm, bool canNotify);

        Core::GemDataRepository& m_repository;
        std::map<std::string, Models::GemQuote> m_quotes;   // 보석 키 → 시세 (조회 대상만, 키 순서 고정)
        std::optional<std::int64_t> m_lastFullUpdated;
        std::optional<std::int64_t> m_lastWatchUpdated;
    };
}