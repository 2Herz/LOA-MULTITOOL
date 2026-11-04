export module LostArkMultiTool.Core.AssetCalculator;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.PriceData;

export namespace LostArkMultiTool::Core
{
    // 재료 자산 계산 인터페이스
    class AssetCalculator
    {
    public:
        virtual ~AssetCalculator();

        // 계산이 끝났을 때 호출될 함수 등록 (원본 C# event Action 대응)
        virtual void AddAssetCalculatedListener(std::function<void()> listener) = 0;

        // 복사본을 반환: 백그라운드 시세 갱신과 동시에 읽어도 안전하도록
        [[nodiscard]] virtual std::unordered_map<std::string, Models::PriceData> GetTopPrices() const = 0;
        [[nodiscard]] virtual std::unordered_map<std::string, Models::PriceData> GetBotPrices() const = 0;
        [[nodiscard]] virtual bool HasPrices() const = 0;

        virtual void FetchMarketPrices(MarketAPI& api) = 0;
        virtual void CalculateCharacterAssets(std::string_view charName) = 0;
        virtual void CalculateAllCharacters() = 0;
        virtual void CalculateRosterHistory() = 0;
    };
}