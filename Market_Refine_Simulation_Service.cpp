module LostArkMultiTool.Services.MarketRefineSimulationService;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.RefineSimulationService;
import LostArkMultiTool.Models.RefineData;
import LostArkMultiTool.Models.RefinePlan;
import LostArkMultiTool.Services.JSONFileStore;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        using Models::RefineData::EquipmentState;
        using Models::RefineData::EquipmentTier;
        using Models::RefineData::RefineMaterial;

        // ===== JSON 키 · 값 =====
        constexpr std::string_view KEY_TARGET_MODE{ "TargetMode" };
        constexpr std::string_view KEY_TARGET_ITEM_LEVEL{ "TargetItemLevel" };
        constexpr std::string_view KEY_STARTS{ "Starts" };
        constexpr std::string_view KEY_TARGETS{ "Targets" };
        constexpr std::string_view KEY_SLOT{ "Slot" };
        constexpr std::string_view KEY_TIER{ "Tier" };
        constexpr std::string_view KEY_ENHANCEMENT{ "Enhancement" };
        constexpr std::string_view KEY_ADVANCED{ "Advanced" };
        constexpr std::string_view KEY_PRICES{ "Prices" };
        constexpr std::string_view KEY_BOUND{ "Bound" };
        constexpr std::string_view KEY_LAST_UPDATED{ "LastUpdated" };
        constexpr std::string_view MODE_ITEM_LEVEL{ "ItemLevel" };
        constexpr std::string_view MODE_STAGE{ "Stage" };
        constexpr std::string_view TIER_AEGIR_NAME{ "에기르" };
        constexpr std::string_view TIER_SERCA_NAME{ "세르카" };

        // 부위 이름 (RefineSlot 순서, 저장 파일 키)
        constexpr std::array<std::string_view, Models::REFINE_SLOT_COUNT> SLOT_NAMES{ "투구", "어깨", "상의", "하의", "장갑", "무기" };

        // ===== 기본값 =====
        constexpr int DEFAULT_TARGET_ITEM_LEVEL{ 1750 };
        constexpr int MIN_TARGET_ITEM_LEVEL{ 1600 };
        constexpr int MAX_TARGET_ITEM_LEVEL{ 1800 };       // 세르카 +25 여섯 부위
        constexpr double GOLD_PRICE{ 1.0 };                // 골드 1 = 1골드
        constexpr double MAX_MATERIAL_PRICE{ 99'999'999.0 };

        [[nodiscard]] Models::RefineStates MakeUniformStates(const EquipmentState& state)
        {
            Models::RefineStates states;
            states.fill(state);
            return states;
        }

        // 저장 파일의 상태 목록 → 부위별 상태 (모르는 부위 · 범위를 벗어난 값은 건너뜀)
        void ReadStates(const JSONValue& entries, Models::RefineStates& states)
        {
            for (const JSONValue& entry : entries.GetItems())
            {
                const auto found{ std::ranges::find(SLOT_NAMES, entry.FindMember(KEY_SLOT).GetString()) };
                if (found == SLOT_NAMES.end()) continue;

                const EquipmentState state{
                    entry.FindMember(KEY_TIER).GetString() == TIER_SERCA_NAME ? EquipmentTier::TIER_SERCA : EquipmentTier::TIER_AEGIR,
                    static_cast<int>(entry.FindMember(KEY_ENHANCEMENT).GetNumber()),
                    static_cast<int>(entry.FindMember(KEY_ADVANCED).GetNumber()) };
                if (!Models::RefineData::IsValidState(state)) continue;
                states[static_cast<std::size_t>(std::distance(SLOT_NAMES.begin(), found))] = state;
            }
        }

        [[nodiscard]] JSONValue BuildStates(const Models::RefineStates& states)
        {
            std::vector<JSONValue> entries;
            entries.reserve(states.size());
            for (std::size_t slot{}; slot < states.size(); ++slot)
            {
                const EquipmentState& state{ states[slot] };
                entries.push_back(JSONValue::CreateObject(
                    { std::string{ KEY_SLOT }, std::string{ KEY_TIER }, std::string{ KEY_ENHANCEMENT }, std::string{ KEY_ADVANCED } },
                    { JSONValue::CreateString(std::string{ SLOT_NAMES[slot] }),
                      JSONValue::CreateString(std::string{ state.m_tier == EquipmentTier::TIER_SERCA ? TIER_SERCA_NAME : TIER_AEGIR_NAME }),
                      JSONValue::CreateNumber(static_cast<double>(state.m_enhancement)),
                      JSONValue::CreateNumber(static_cast<double>(state.m_advancedEnhancement)) }));
            }
            return JSONValue::CreateArray(std::move(entries));
        }

        // 저장 이름 → 재료 (없으면 nullopt)
        [[nodiscard]] std::optional<RefineMaterial> FindMaterial(std::string_view name)
        {
            for (std::size_t index{}; index < Models::RefineData::MATERIAL_COUNT; ++index)
            {
                const RefineMaterial material{ static_cast<RefineMaterial>(index) };
                if (Models::RefineData::GetMaterialInfo(material).m_name == name) return material;
            }
            return std::nullopt;
        }

        [[nodiscard]] std::size_t ToSlotIndex(Models::RefineSlot slot)
        {
            const std::size_t index{ static_cast<std::size_t>(slot) };
            if (index >= Models::REFINE_SLOT_COUNT) throw std::out_of_range{ "재련 시뮬레이터 부위 번호가 범위를 벗어났습니다." };
            return index;
        }
    }

    MarketRefineSimulationService::MarketRefineSimulationService()
        : m_fileStore{ Core::APPConstants::REFINE_SIMULATOR_DATA_FILE_NAME }
    {
        // 기본값: 1730 프리셋 → 1750 목표
        m_request.m_starts = MakeUniformStates(Core::GetStartPresetState(Core::RefineStartPreset::PRESET_1730));
        m_request.m_targets = m_request.m_starts;
        m_request.m_targetItemLevel = DEFAULT_TARGET_ITEM_LEVEL;
        m_prices.m_prices[Models::RefineData::ToIndex(RefineMaterial::MATERIAL_GOLD)] = GOLD_PRICE;

        Load();
    }

    void MarketRefineSimulationService::Load()
    {
        // 변환이 끝까지 성공한 파일만 반영 (실패하면 백업으로 다시 시도, 둘 다 없으면 기본값)
        std::optional<Models::RefineRequest> loadedRequest;
        std::optional<Models::RefinePrices> loadedPrices;
        bool hasPrices{};
        std::optional<std::int64_t> lastUpdated;

        JSONLoadResult result{ m_fileStore.Load([&](const JSONValue& root)
        {
            if (root.GetType() != JSONType::OBJECT_VALUE) throw std::runtime_error{ "재련 시뮬레이터 저장 파일 형식이 올바르지 않습니다." };

            Models::RefineRequest request{ m_request };
            request.m_targetMode = root.FindMember(KEY_TARGET_MODE).GetString() == MODE_STAGE ? Models::RefineTargetMode::TARGET_STAGE : Models::RefineTargetMode::TARGET_ITEM_LEVEL;
            const double targetLevel{ root.FindMember(KEY_TARGET_ITEM_LEVEL).GetNumber() };
            if (targetLevel > 0.0) request.m_targetItemLevel = std::clamp(static_cast<int>(targetLevel), MIN_TARGET_ITEM_LEVEL, MAX_TARGET_ITEM_LEVEL);
            ReadStates(root.FindMember(KEY_STARTS), request.m_starts);
            ReadStates(root.FindMember(KEY_TARGETS), request.m_targets);

            Models::RefinePrices prices{ m_prices };
            const JSONValue& priceObject{ root.FindMember(KEY_PRICES) };
            const std::vector<std::string>& names{ priceObject.GetKeys() };
            const std::vector<JSONValue>& values{ priceObject.GetValues() };
            for (std::size_t index{}; index < std::min(names.size(), values.size()); ++index)
            {
                const std::optional<RefineMaterial> material{ FindMaterial(names[index]) };
                if (!material.has_value() || *material == RefineMaterial::MATERIAL_GOLD) continue;
                prices.m_prices[Models::RefineData::ToIndex(*material)] = std::clamp(values[index].GetNumber(), 0.0, MAX_MATERIAL_PRICE);
                hasPrices = true;
            }
            for (const JSONValue& name : root.FindMember(KEY_BOUND).GetItems())
            {
                if (const std::optional<RefineMaterial> material{ FindMaterial(name.GetString()) }) prices.m_isBound[Models::RefineData::ToIndex(*material)] = true;
            }

            const double updated{ root.FindMember(KEY_LAST_UPDATED).GetNumber() };
            if (updated > 0.0) lastUpdated = static_cast<std::int64_t>(updated);

            loadedRequest = request;
            loadedPrices = prices;
        }) };

        m_loadWarning = std::move(result.m_warning);
        if (!loadedRequest.has_value() || !loadedPrices.has_value()) return;

        m_request = *loadedRequest;
        m_prices = *loadedPrices;
        m_hasPrices = hasPrices;
        m_lastUpdated = lastUpdated;
    }

    const Models::RefineRequest& MarketRefineSimulationService::GetRequest() const noexcept
    {
        return m_request;
    }

    void MarketRefineSimulationService::SetStart(Models::RefineSlot slot, const EquipmentState& state)
    {
        if (!Models::RefineData::IsValidState(state)) return;
        m_request.m_starts[ToSlotIndex(slot)] = state;
    }

    void MarketRefineSimulationService::SetTarget(Models::RefineSlot slot, const EquipmentState& state)
    {
        if (!Models::RefineData::IsValidState(state)) return;
        m_request.m_targets[ToSlotIndex(slot)] = state;
    }

    void MarketRefineSimulationService::SetTargetMode(Models::RefineTargetMode mode)
    {
        m_request.m_targetMode = mode;
    }

    void MarketRefineSimulationService::SetTargetItemLevel(int itemLevel)
    {
        m_request.m_targetItemLevel = std::clamp(itemLevel, MIN_TARGET_ITEM_LEVEL, MAX_TARGET_ITEM_LEVEL);
    }

    void MarketRefineSimulationService::ApplyStartPreset(Core::RefineStartPreset preset)
    {
        m_request.m_starts = MakeUniformStates(Core::GetStartPresetState(preset));
    }

    const Models::RefinePrices& MarketRefineSimulationService::GetPrices() const noexcept
    {
        return m_prices;
    }

    void MarketRefineSimulationService::SetPrice(RefineMaterial material, double price)
    {
        if (material == RefineMaterial::MATERIAL_GOLD) return;   // 골드는 항상 1
        m_prices.m_prices.at(Models::RefineData::ToIndex(material)) = std::clamp(price, 0.0, MAX_MATERIAL_PRICE);
        ++m_priceRevision;
    }

    void MarketRefineSimulationService::SetBound(RefineMaterial material, bool isBound)
    {
        m_prices.m_isBound.at(Models::RefineData::ToIndex(material)) = isBound;
        ++m_priceRevision;
    }

    bool MarketRefineSimulationService::HasPrices() const noexcept
    {
        return m_hasPrices;
    }

    std::uint64_t MarketRefineSimulationService::GetPriceRevision() const noexcept
    {
        return m_priceRevision;
    }

    const std::optional<std::int64_t>& MarketRefineSimulationService::GetLastUpdated() const noexcept
    {
        return m_lastUpdated;
    }

    // 재료마다 거래소 시세 (최근 거래가 → 없으면 현재 최저가, 묶음은 1개 단가, 파편은 주머니 ÷ 3000)
    std::vector<Core::RefinePriceQuote> MarketRefineSimulationService::FetchPrices(Core::MarketAPI& api) const
    {
        std::vector<Core::RefinePriceQuote> quotes;
        quotes.reserve(Models::RefineData::MATERIAL_COUNT);
        for (std::size_t index{}; index < Models::RefineData::MATERIAL_COUNT; ++index)
        {
            const RefineMaterial material{ static_cast<RefineMaterial>(index) };
            const Models::RefineData::MaterialInfo& info{ Models::RefineData::GetMaterialInfo(material) };
            if (info.m_marketName.empty()) continue;   // 골드

            try
            {
                quotes.push_back(Core::RefinePriceQuote{ material, api.GetPrice(info.m_marketName, info.m_isShard, !info.m_isShard) });
            }
            catch (const Core::APIKeyInvalidException&) { throw; }       // 키 · 한도 · 점검 · 통신 오류는 회차 전체 실패
            catch (const Core::APIRateLimitException&) { throw; }
            catch (const Core::APIMaintenanceException&) { throw; }
            catch (const Core::APIRequestException&) { throw; }
            catch (const std::runtime_error&)
            {
                // 매물 · 거래 기록이 없는 재료만 건너뜀 (이전 가격 유지)
            }
        }
        return quotes;
    }

    void MarketRefineSimulationService::ApplyPrices(const std::vector<Core::RefinePriceQuote>& quotes)
    {
        for (const Core::RefinePriceQuote& quote : quotes)
        {
            if (quote.m_material == RefineMaterial::MATERIAL_GOLD || quote.m_price <= 0.0) continue;
            m_prices.m_prices.at(Models::RefineData::ToIndex(quote.m_material)) = std::min(quote.m_price, MAX_MATERIAL_PRICE);
        }

        m_hasPrices = m_hasPrices || !quotes.empty();
        m_lastUpdated = Core::LocalTime::GetUnixSeconds();
        ++m_priceRevision;
    }

    const std::optional<std::string>& MarketRefineSimulationService::GetLoadWarning() const noexcept
    {
        return m_loadWarning;
    }

    void MarketRefineSimulationService::Save()
    {
        std::vector<std::string> priceKeys;
        std::vector<JSONValue> priceValues;
        std::vector<JSONValue> boundNames;
        for (std::size_t index{}; index < Models::RefineData::MATERIAL_COUNT; ++index)
        {
            const std::string name{ Models::RefineData::GetMaterialInfo(static_cast<RefineMaterial>(index)).m_name };
            if (index != Models::RefineData::ToIndex(RefineMaterial::MATERIAL_GOLD) && m_prices.m_prices[index] > 0.0)
            {
                priceKeys.push_back(name);
                priceValues.push_back(JSONValue::CreateNumber(m_prices.m_prices[index]));
            }
            if (m_prices.m_isBound[index]) boundNames.push_back(JSONValue::CreateString(name));
        }

        std::vector<std::string> keys{ std::string{ KEY_TARGET_MODE }, std::string{ KEY_TARGET_ITEM_LEVEL }, std::string{ KEY_STARTS },
            std::string{ KEY_TARGETS }, std::string{ KEY_PRICES }, std::string{ KEY_BOUND } };
        std::vector<JSONValue> values{
            JSONValue::CreateString(std::string{ m_request.m_targetMode == Models::RefineTargetMode::TARGET_STAGE ? MODE_STAGE : MODE_ITEM_LEVEL }),
            JSONValue::CreateNumber(static_cast<double>(m_request.m_targetItemLevel)),
            BuildStates(m_request.m_starts),
            BuildStates(m_request.m_targets),
            JSONValue::CreateObject(std::move(priceKeys), std::move(priceValues)),
            JSONValue::CreateArray(std::move(boundNames)) };
        if (m_lastUpdated.has_value())
        {
            keys.emplace_back(KEY_LAST_UPDATED);
            values.push_back(JSONValue::CreateNumber(static_cast<double>(*m_lastUpdated)));
        }

        m_fileStore.Save(WriteJSONText(JSONValue::CreateObject(std::move(keys), std::move(values))));
    }
}
