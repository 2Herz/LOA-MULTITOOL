export module LostArkMultiTool.Services.AssetCalculationService;

import std;
import LostArkMultiTool.Core.DataRepository;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.AssetCalculator;
import LostArkMultiTool.Models.PriceData;

export namespace LostArkMultiTool::Services
{
    // 재료 자산 계산 서비스
    // 시세 갱신(FetchMarketPrices)만 백그라운드 스레드에서 호출하고,
    // 계산·리스너 등록·저장은 UI 스레드에서 호출할 것
    class AssetCalculationService final : public Core::AssetCalculator
    {
    public:
        // repository는 이 서비스보다 오래 살아 있어야 함
        explicit AssetCalculationService(Core::DataRepository& repository);

        AssetCalculationService(const AssetCalculationService&) = delete;
        AssetCalculationService& operator=(const AssetCalculationService&) = delete;

        void AddAssetCalculatedListener(std::function<void()> listener) override;

        [[nodiscard]] std::unordered_map<std::string, Models::PriceData> GetTopPrices() const override;
        [[nodiscard]] std::unordered_map<std::string, Models::PriceData> GetBotPrices() const override;
        [[nodiscard]] bool HasPrices() const override;

        void FetchMarketPrices(Core::MarketAPI& api) override;
        void CalculateCharacterAssets(std::string_view charName) override;
        void CalculateAllCharacters() override;
        void CalculateRosterHistory() override;

    private:
        // 모든 캐릭터의 재료 가치를 다시 계산하고 원정대 합계를 반환
        double ApplyAllCharacterValues();

        // 등록된 계산 완료 함수들을 차례로 호출
        void NotifyAssetCalculated();

        Core::DataRepository& m_repository;
        mutable std::mutex m_pricesMutex;   // 시세 맵 보호 (백그라운드 갱신 ↔ UI 읽기)
        std::unordered_map<std::string, Models::PriceData> m_topPrices;
        std::unordered_map<std::string, Models::PriceData> m_botPrices;
        std::vector<std::function<void()>> m_assetCalculatedListeners;
    };
}