export module LostArkMultiTool.Core.MarketAPI;

import std;
import LostArkMultiTool.Models.CharacterInfo;

export namespace LostArkMultiTool::Core
{
    // 로스트아크 Open API (거래소 시세 · 원정대 목록)
    // 메서드는 블로킹이므로 화면(UI) 스레드가 아닌 백그라운드 스레드에서 호출할 것
    class MarketAPI
    {
    public:
        virtual ~MarketAPI();

        virtual void SetAPIKey(std::string apiKey) = 0;

        [[nodiscard]] virtual double GetPrice(std::string_view itemName, bool isShard, bool isBundle) = 0;
        [[nodiscard]] virtual std::vector<Models::CharacterInfo> GetRoster(std::string_view charName) = 0;
    };
}