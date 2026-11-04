module LostArkMultiTool.Services.AssetCalculationService;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.DataRepository;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.Character;
import LostArkMultiTool.Models.ItemData;
import LostArkMultiTool.Models.PriceData;
import LostArkMultiTool.Models.UserData;

namespace LostArkMultiTool::Services
{
    namespace
    {
        using PriceTable = std::unordered_map<std::string, Models::PriceData>;

        // 조회할 재료 정의 (C# MaterialQuery 대응)
        struct MaterialQuery
        {
            std::string_view m_name;
            std::size_t m_col;      // 표의 열 번호 = Core::APPConstants::MAT_KEYS 인덱스
            bool m_isShard;         // 파편 주머니 (3000개 단위 시세)
            bool m_isBundle;        // 묶음 판매 (묶음 개수로 나눈 시세)
        };

        // 표의 열 번호 (MAT_KEYS 인덱스와 같음)
        constexpr std::size_t COL_SHARD{ 1 };
        constexpr std::size_t COL_DEST{ 2 };
        constexpr std::size_t COL_GUARD{ 3 };
        constexpr std::size_t COL_LEAP{ 4 };
        constexpr std::size_t COL_FUSION{ 5 };
        constexpr std::size_t COL_YONG{ 6 };
        constexpr std::size_t COL_BING{ 7 };

        constexpr double ZERO_PRICE{ 0.0 };

        // 상위 재료 (1730 이상)
        constexpr std::array TOP_MATERIALS{
            MaterialQuery{ .m_name = "운명의 파편 주머니(대)", .m_col = COL_SHARD, .m_isShard = true, .m_isBundle = false },
            MaterialQuery{ .m_name = "운명의 파괴석 결정", .m_col = COL_DEST, .m_isShard = false, .m_isBundle = true },
            MaterialQuery{ .m_name = "운명의 수호석 결정", .m_col = COL_GUARD, .m_isShard = false, .m_isBundle = true },
            MaterialQuery{ .m_name = "위대한 운명의 돌파석", .m_col = COL_LEAP, .m_isShard = false, .m_isBundle = false },
            MaterialQuery{ .m_name = "상급 아비도스 융화 재료", .m_col = COL_FUSION, .m_isShard = false, .m_isBundle = false },
            MaterialQuery{ .m_name = "용암의 숨결", .m_col = COL_YONG, .m_isShard = false, .m_isBundle = false },
            MaterialQuery{ .m_name = "빙하의 숨결", .m_col = COL_BING, .m_isShard = false, .m_isBundle = false }
        };

        // 하위 재료 (1730 미만)
        constexpr std::array BOT_MATERIALS{
            MaterialQuery{ .m_name = "운명의 파괴석", .m_col = COL_DEST, .m_isShard = false, .m_isBundle = true },
            MaterialQuery{ .m_name = "운명의 수호석", .m_col = COL_GUARD, .m_isShard = false, .m_isBundle = true },
            MaterialQuery{ .m_name = "운명의 돌파석", .m_col = COL_LEAP, .m_isShard = false, .m_isBundle = false },
            MaterialQuery{ .m_name = "아비도스 융화 재료", .m_col = COL_FUSION, .m_isShard = false, .m_isBundle = false }
        };

        // 상위·하위 단가가 같은 재료 (운파, 용숨, 빙숨)
        constexpr std::array SHARED_MATERIAL_COLS{ COL_SHARD, COL_YONG, COL_BING };

        // 단가 갱신 (처음이면 직전값 = 현재값 → 등락 0)
        void UpdatePriceDict(PriceTable& dict, const std::string& key, double newPrice)
        {
            Models::PriceData& data{ dict[key] };
            const double currentPrice{ data.GetCurrent() };
            data.SetPrevious(currentPrice == ZERO_PRICE ? newPrice : currentPrice);
            data.SetCurrent(newPrice);
        }

        // 현재 단가 조회 (없으면 0)
        double GetCurrentPrice(const PriceTable& dict, std::string_view key)
        {
            const auto found{ dict.find(std::string{ key }) };
            return found != dict.end() ? found->second.GetCurrent() : ZERO_PRICE;
        }

        // 캐릭터 한 명의 재료 가치 계산 → 합계 반환
        double ApplyMaterialValues(Models::Character& charData, const PriceTable& topPrices, const PriceTable& botPrices)
        {
            double total{};
            for (const std::string_view key : Core::APPConstants::MAT_KEYS)
            {
                if (key.empty()) continue;

                Models::ItemData& item{ charData.GetMaterial(key) };
                item.SetCalculatedTopValue(std::floor(item.GetTopQty() * GetCurrentPrice(topPrices, key)));
                item.SetCalculatedBotValue(std::floor(item.GetBottomQty() * GetCurrentPrice(botPrices, key)));
                total += item.GetCalculatedTopValue() + item.GetCalculatedBotValue();
            }
            return total;
        }
    }

    AssetCalculationService::AssetCalculationService(Core::DataRepository& repository)
        : m_repository{ repository }
    {}

    void AssetCalculationService::AddAssetCalculatedListener(std::function<void()> listener)
    {
        if (!listener) return;   // 빈 함수는 등록하지 않음 (C# event 구독 없음과 같음)
        m_assetCalculatedListeners.push_back(std::move(listener));
    }

    std::unordered_map<std::string, Models::PriceData> AssetCalculationService::GetTopPrices() const
    {
        std::scoped_lock lock{ m_pricesMutex };
        return m_topPrices;   // 복사본 반환 → 호출 쪽은 잠금 없이 안전하게 읽음
    }

    std::unordered_map<std::string, Models::PriceData> AssetCalculationService::GetBotPrices() const
    {
        std::scoped_lock lock{ m_pricesMutex };
        return m_botPrices;
    }

    bool AssetCalculationService::HasPrices() const
    {
        std::scoped_lock lock{ m_pricesMutex };
        return !m_topPrices.empty() && !m_botPrices.empty();
    }

    void AssetCalculationService::FetchMarketPrices(Core::MarketAPI& api)
    {
        // 네트워크 호출은 잠금 밖에서 수행 (화면 읽기를 오래 막지 않도록)
        // 임시 맵에 전부 모은 뒤, 모두 성공했을 때만 반영 (부분 갱신 방지)
        std::unordered_map<std::string, double> newTop;
        std::unordered_map<std::string, double> newBot;

        for (const MaterialQuery& material : TOP_MATERIALS)
        {
            const std::string key{ Core::APPConstants::MAT_KEYS.at(material.m_col) };
            newTop.insert_or_assign(key, api.GetPrice(material.m_name, material.m_isShard, material.m_isBundle));
        }

        for (const MaterialQuery& material : BOT_MATERIALS)
        {
            const std::string key{ Core::APPConstants::MAT_KEYS.at(material.m_col) };
            newBot.insert_or_assign(key, api.GetPrice(material.m_name, material.m_isShard, material.m_isBundle));
        }

        // 상위·하위 단가가 같은 재료는 상위 시세를 그대로 사용
        for (const std::size_t col : SHARED_MATERIAL_COLS)
        {
            const std::string key{ Core::APPConstants::MAT_KEYS.at(col) };
            newBot.insert_or_assign(key, newTop.at(key));
        }

        // 여기서만 잠금: 맵 반영 구간
        std::scoped_lock lock{ m_pricesMutex };
        for (const auto& [key, price] : newTop) UpdatePriceDict(m_topPrices, key, price);
        for (const auto& [key, price] : newBot) UpdatePriceDict(m_botPrices, key, price);
    }

    void AssetCalculationService::CalculateCharacterAssets(std::string_view charName)
    {
        Models::Character* charData{ m_repository.GetData().FindCharacter(charName) };
        if (charData == nullptr) return;   // 없는 캐릭터는 조용히 무시 (원본과 같음)

        {
            std::scoped_lock lock{ m_pricesMutex };
            ApplyMaterialValues(*charData, m_topPrices, m_botPrices);
        }
        NotifyAssetCalculated();   // 리스너는 잠금을 푼 뒤 호출 (데드락 방지)
    }

    void AssetCalculationService::CalculateAllCharacters()
    {
        ApplyAllCharacterValues();
        NotifyAssetCalculated();
    }

    // 모든 캐릭터의 계산값까지 갱신하므로 호출 측에서 CalculateCharacterAssets를 따로 부를 필요 없음 (알림 1회)
    void AssetCalculationService::CalculateRosterHistory()
    {
        if (!HasPrices()) return;   // 시세 없이 0원이 기록되는 것 방지

        const double totalRosterAsset{ ApplyAllCharacterValues() };

        Models::UserData& data{ m_repository.GetData() };
        data.RecordHistory(Models::UserData::MakeHistoryKey(Core::LocalTime::GetLocalToday()), totalRosterAsset);

        // 원본 C# finally 대응: 저장이 실패해도 화면 갱신은 보장하고, 예외는 그대로 다시 던짐
        try
        {
            m_repository.Save();
        }
        catch (...)
        {
            NotifyAssetCalculated();
            throw;
        }
        NotifyAssetCalculated();
    }

    double AssetCalculationService::ApplyAllCharacterValues()
    {
        Models::UserData& data{ m_repository.GetData() };

        // 맵 구조는 바뀌지 않으므로 const로 순회하고, 값 수정은 FindCharacter가 준 포인터로 함
        std::scoped_lock lock{ m_pricesMutex };
        double totalRosterAsset{};
        for (const auto& entry : std::as_const(data).GetCharacters())
        {
            if (Models::Character* charData{ data.FindCharacter(entry.first) }; charData != nullptr)
            {
                totalRosterAsset += ApplyMaterialValues(*charData, m_topPrices, m_botPrices);
            }
        }
        return totalRosterAsset;
    }

    void AssetCalculationService::NotifyAssetCalculated()
    {
        for (const auto& listener : m_assetCalculatedListeners)
        {
            listener();
        }
    }
}