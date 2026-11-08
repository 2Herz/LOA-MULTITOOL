export module LostArkMultiTool.Services.MarketRefineSimulationService;

import std;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.RefineSimulationService;
import LostArkMultiTool.Models.RefineData;
import LostArkMultiTool.Models.RefinePlan;
import LostArkMultiTool.Services.JSONFileStore;

export namespace LostArkMultiTool::Services
{
    // 재련 시뮬레이터 모델 구현 (거래소 시세 + RefineSimulatorData.json 저장, 사용자 데이터 폴더)
    // 형식: { "TargetMode": "ItemLevel" | "Stage", "TargetItemLevel": 1750,
    //         "Starts": [ { "Slot": "무기", "Tier": "세르카", "Enhancement": 11, "Advanced": 0 }, ... ],
    //         "Targets": [ ... ],
    //         "Prices": { "아비도스 융화 재료": 192.0, ... }, "Bound": [ "골드", ... ], "LastUpdated": 1790000000,
    //         "CharacterName": "캐릭터 이름" }   (CharacterName은 없어도 됨 = 빈 이름)
    class MarketRefineSimulationService final : public Core::RefineSimulationService
    {
    public:
        MarketRefineSimulationService();

        [[nodiscard]] const Models::RefineRequest& GetRequest() const noexcept override;
        void SetStart(Models::RefineSlot slot, const Models::RefineData::EquipmentState& state) override;
        void SetTarget(Models::RefineSlot slot, const Models::RefineData::EquipmentState& state) override;
        void SetTargetMode(Models::RefineTargetMode mode) override;
        void SetTargetItemLevel(int itemLevel) override;
        void ApplyStartPreset(Core::RefineStartPreset preset) override;

        [[nodiscard]] const std::wstring& GetCharacterName() const noexcept override;   // 마지막으로 조회한 캐릭터 이름
        void SetCharacterName(std::wstring name) override;                              // 조회한 캐릭터 이름 기억

        [[nodiscard]] const Models::RefinePrices& GetPrices() const noexcept override;
        void SetPrice(Models::RefineData::RefineMaterial material, double price) override;
        void SetBound(Models::RefineData::RefineMaterial material, bool isBound) override;
        [[nodiscard]] bool HasPrices() const noexcept override;
        [[nodiscard]] std::uint64_t GetPriceRevision() const noexcept override;
        [[nodiscard]] const std::optional<std::int64_t>& GetLastUpdated() const noexcept override;

        [[nodiscard]] std::vector<Core::RefinePriceQuote> FetchPrices(Core::MarketAPI& api) const override;
        void ApplyPrices(const std::vector<Core::RefinePriceQuote>& quotes) override;

        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept override;
        void Save() override;

    private:
        void Load();   // 저장 파일 → 모델 (없거나 손상이면 기본값)

        JSONFileStore m_fileStore;
        Models::RefineRequest m_request;
        Models::RefinePrices m_prices;
        bool m_hasPrices{};
        std::uint64_t m_priceRevision{};
        std::optional<std::int64_t> m_lastUpdated;
        std::optional<std::string> m_loadWarning;
        std::wstring m_characterName;   // 마지막으로 조회한 캐릭터 이름 (저장 파일에 없으면 빈 글자)
    };
}
