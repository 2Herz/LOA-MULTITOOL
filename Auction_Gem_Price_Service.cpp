module LostArkMultiTool.Services.AuctionGemPriceService;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.APPExceptions;
import LostArkMultiTool.Core.GemDataRepository;
import LostArkMultiTool.Core.GemPriceService;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.GemData;
import LostArkMultiTool.Models.GemQuote;
import LostArkMultiTool.Models.PriceAlarm;
import LostArkMultiTool.Models.RefreshSettings;

namespace LostArkMultiTool::Services
{
    AuctionGemPriceService::AuctionGemPriceService(Core::GemDataRepository& repository)
        : m_repository{ repository }
    {
        // 조회 대상(종류 × 레벨)마다 시세 칸을 만들고, 지난 실행의 전체 갱신가를 등락 기준으로 넣어 둠
        const std::map<std::string, std::int64_t>& lastPrices{ m_repository.GetData().GetLastPrices() };
        for (Models::GemSelection& gem : Models::GemCatalog::GetAllGems())
        {
            std::string gemKey{ Models::GemCatalog::MakeKey(gem) };
            Models::GemQuote quote{ std::move(gem) };
            if (const auto saved{ lastPrices.find(gemKey) }; saved != lastPrices.end()) quote.SetPreviousPrice(saved->second);
            m_quotes.emplace(std::move(gemKey), std::move(quote));
        }
    }

    const std::map<std::string, Models::GemQuote>& AuctionGemPriceService::GetQuotes() const noexcept
    {
        return m_quotes;
    }

    const std::vector<Models::PriceAlarm>& AuctionGemPriceService::GetAlarms() const noexcept
    {
        return m_repository.GetData().GetAlarms();
    }

    const std::optional<std::int64_t>& AuctionGemPriceService::GetLastFullUpdated() const noexcept
    {
        return m_lastFullUpdated;
    }

    const std::optional<std::int64_t>& AuctionGemPriceService::GetLastWatchUpdated() const noexcept
    {
        return m_lastWatchUpdated;
    }

    std::vector<Models::GemSelection> AuctionGemPriceService::GetAllTargets() const
    {
        return Models::GemCatalog::GetAllGems();
    }

    // 알림이 걸린 보석만 (조회 범위 밖이거나 알 수 없는 종류는 제외, 중복 제거)
    std::vector<Models::GemSelection> AuctionGemPriceService::GetWatchedTargets() const
    {
        std::vector<Models::GemSelection> targets;
        std::set<std::string> seenKeys;
        for (const Models::PriceAlarm& alarm : m_repository.GetData().GetAlarms())
        {
            std::string gemKey{ alarm.GetGemKey() };
            if (!m_quotes.contains(gemKey)) continue;              // 조회 범위 밖
            if (!seenKeys.insert(std::move(gemKey)).second) continue;   // 이미 추가한 보석

            targets.push_back(alarm.GetGem());
        }
        return targets;
    }

    // 동시 요청 수를 제한해서 모두 조회 (원본 SemaphoreSlim + Task.WhenAll → 작업자 스레드 3개가 차례로 하나씩 가져감)
    std::vector<Core::FetchedPrice> AuctionGemPriceService::FetchPrices(Core::MarketAPI& api, const std::vector<Models::GemSelection>& targets) const
    {
        std::vector<Core::FetchedPrice> results(targets.size());
        std::stop_source stopSource;            // 하나가 실패하면 나머지 중단 (원본 CancellationTokenSource)
        std::atomic<std::size_t> nextIndex{};   // 다음에 조회할 보석 번호
        std::mutex errorMutex;
        std::exception_ptr firstError;          // 처음 실패한 진짜 원인 (중단으로 생긴 예외는 제외)

        const auto worker{ [&]
        {
            while (!stopSource.stop_requested())
            {
                const std::size_t index{ nextIndex.fetch_add(1) };
                if (index >= targets.size()) return;

                const Models::GemSelection& gem{ targets[index] };
                results[index].m_gemKey = Models::GemCatalog::MakeKey(gem);

                const Models::GemType* type{ Models::GemCatalog::FindType(gem.m_typeName) };
                if (type == nullptr) continue;   // 알 수 없는 종류는 매물 없음으로 (GetWatchedTargets에서 이미 걸러짐)

                try
                {
                    results[index].m_price = api.GetLowestBuyPrice(Models::GemCatalog::MakeItemName(*type, gem.m_level), type->m_tier,
                        stopSource.get_token());
                }
                catch (const Core::RequestCanceledException&)
                {
                    return;   // 다른 요청의 실패로 중단됨 → 원인으로 보지 않음
                }
                catch (...)
                {
                    {
                        std::scoped_lock lock{ errorMutex };
                        if (!firstError) firstError = std::current_exception();
                    }
                    stopSource.request_stop();   // 이번 회차는 반영되지 않으므로 나머지 요청 중단
                    return;
                }
            }
        } };

        {
            // 작업자는 이 범위를 벗어날 때 모두 끝날 때까지 기다림 (jthread 소멸자 = join → 결과 · 예외를 안전하게 읽음)
            const std::size_t workerCount{ std::min(targets.size(), static_cast<std::size_t>(Core::APPConstants::MAX_CONCURRENT_REQUESTS)) };
            std::vector<std::jthread> workers;
            workers.reserve(workerCount);
            for (std::size_t count{}; count < workerCount; ++count) workers.emplace_back(worker);
        }

        if (firstError) std::rethrow_exception(firstError);
        return results;
    }

    // 조회 결과 반영 → 알림 판정 → (바뀐 게 있으면) 저장 (원본 RefreshTargetsAsync 뒷부분)
    Core::RefreshResult AuctionGemPriceService::ApplyPrices(const std::vector<Core::FetchedPrice>& prices, bool isFull)
    {
        Core::RefreshResult result;
        // 일별 최저가의 하루는 오전 6시(일일 컨텐츠 초기화)부터 → 06시 전 조회는 전날 기록에 반영
        const std::string today{ Core::LocalTime::MakeDateKey(Core::LocalTime::GetGameToday()) };

        bool isChanged{};
        for (const Core::FetchedPrice& fetched : prices) isChanged |= ApplyPrice(fetched, isFull, today);
        isChanged |= EvaluateAlarms(result.m_reachedAlarms);

        // 전체 · 감시 갱신 시각을 따로 기록
        (isFull ? m_lastFullUpdated : m_lastWatchUpdated) = Core::LocalTime::GetUnixSeconds();

        if (!isChanged) return result;   // 저장할 변경이 있을 때만 파일에 씀
        try
        {
            m_repository.Save();
        }
        catch (const std::exception& error)
        {
            result.m_saveError = error.what();   // 저장 실패는 "시세 갱신 실패"와 구분해서 전달
        }
        return result;
    }

    void AuctionGemPriceService::SetAlarm(const Models::GemSelection& gem, std::int64_t targetPrice)
    {
        std::vector<Models::PriceAlarm>& alarms{ m_repository.GetData().GetAlarms() };
        const std::string gemKey{ Models::GemCatalog::MakeKey(gem) };

        // 같은 보석에 알림이 이미 있으면 목표가만 변경 (되돌리기용으로 이전 상태를 복사해 둠)
        const auto existing{ std::find_if(alarms.begin(), alarms.end(), [&gemKey](const Models::PriceAlarm& alarm) { return alarm.GetGemKey() == gemKey; }) };
        const bool isNew{ existing == alarms.end() };
        const std::optional<Models::PriceAlarm> previous{ isNew ? std::nullopt : std::optional<Models::PriceAlarm>{ *existing } };
        const auto index{ static_cast<std::size_t>(std::distance(alarms.begin(), existing)) };

        if (isNew) alarms.emplace_back(gem, targetPrice);

        Models::PriceAlarm& alarm{ alarms[index] };   // push_back 뒤라도 번호로 다시 찾으므로 안전
        alarm.SetTargetPrice(targetPrice);
        alarm.SetTriggered(false);
        alarm.SetNotified(false);
        static_cast<void>(UpdateAlarmState(alarm, false));   // 이미 목표가 이하라면 표시만 하고 풍선 알림은 생략

        try
        {
            m_repository.Save();
        }
        catch (...)
        {
            // 저장에 실패하면 화면과 파일이 어긋나지 않게 메모리 상태를 원래대로
            if (isNew) alarms.pop_back();
            else alarms[index] = *previous;
            throw;
        }
    }

    void AuctionGemPriceService::RemoveAlarm(std::string_view gemKey)
    {
        std::vector<Models::PriceAlarm>& alarms{ m_repository.GetData().GetAlarms() };
        const auto found{ std::find_if(alarms.begin(), alarms.end(), [gemKey](const Models::PriceAlarm& alarm) { return alarm.GetGemKey() == gemKey; }) };
        if (found == alarms.end()) return;

        const auto index{ std::distance(alarms.begin(), found) };
        Models::PriceAlarm removed{ std::move(*found) };
        alarms.erase(found);

        try
        {
            m_repository.Save();
        }
        catch (...)
        {
            alarms.insert(alarms.begin() + index, std::move(removed));   // 원래 위치에 되돌림
            throw;
        }
    }

    Models::RefreshSettings AuctionGemPriceService::GetSettings() const
    {
        return m_repository.GetData().GetSettings();
    }

    void AuctionGemPriceService::UpdateSettings(const Models::RefreshSettings& settings)
    {
        Models::GemData& data{ m_repository.GetData() };
        const Models::RefreshSettings previous{ data.GetSettings() };

        Models::RefreshSettings next{ settings };
        next.Normalize();
        data.SetSettings(next);

        try
        {
            m_repository.Save();
        }
        catch (...)
        {
            data.SetSettings(previous);
            throw;
        }
    }

    const std::map<std::string, std::int64_t>& AuctionGemPriceService::GetHistory(std::string_view gemKey) const
    {
        static const std::map<std::string, std::int64_t> EMPTY_HISTORY;   // 기록이 없는 보석에 돌려줄 빈 목록 (매번 만들지 않도록 공유)

        const auto& history{ m_repository.GetData().GetHistory() };
        const auto found{ history.find(std::string{ gemKey }) };
        return found != history.end() ? found->second : EMPTY_HISTORY;
    }

    bool AuctionGemPriceService::ApplyPrice(const Core::FetchedPrice& fetched, bool isFull, const std::string& today)
    {
        const auto found{ m_quotes.find(fetched.m_gemKey) };
        if (found == m_quotes.end()) return false;

        Models::GemQuote& quote{ found->second };
        quote.SetPrice(fetched.m_price);
        quote.SetQueried(true);

        // 등락 기준은 "전체 갱신"에서만 넘김 → 감시 갱신이 기준을 흔들지 않음
        if (isFull)
        {
            if (quote.GetLastFullPrice().has_value()) quote.SetPreviousPrice(quote.GetLastFullPrice());   // 첫 전체 갱신이면 저장값 유지
            if (fetched.m_price.has_value()) quote.SetLastFullPrice(fetched.m_price);                     // 매물 없음이면 기준을 지우지 않음
        }

        if (!fetched.m_price.has_value()) return false;   // 매물 없음 → 기록하지 않음
        const std::int64_t price{ *fetched.m_price };

        // 재시작 후 비교용 가격도 전체 갱신 기준으로만 저장
        bool isChanged{};
        std::map<std::string, std::int64_t>& lastPrices{ m_repository.GetData().GetLastPrices() };
        if (isFull)
        {
            const auto saved{ lastPrices.find(fetched.m_gemKey) };
            if (saved == lastPrices.end() || saved->second != price)
            {
                lastPrices.insert_or_assign(fetched.m_gemKey, price);
                isChanged = true;
            }
        }

        return RecordDailyLowest(fetched.m_gemKey, today, price) || isChanged;
    }

    bool AuctionGemPriceService::RecordDailyLowest(const std::string& gemKey, const std::string& day, std::int64_t price)
    {
        std::map<std::string, std::int64_t>& days{ m_repository.GetData().GetHistory()[gemKey] };

        if (const auto recorded{ days.find(day) }; recorded != days.end() && price >= recorded->second) return false;
        days.insert_or_assign(day, price);
        return true;
    }

    bool AuctionGemPriceService::EvaluateAlarms(std::vector<Core::ReachedAlarm>& reached)
    {
        bool isChanged{};
        for (Models::PriceAlarm& alarm : m_repository.GetData().GetAlarms())
        {
            const bool wasTriggered{ alarm.IsTriggered() };
            if (UpdateAlarmState(alarm, true))
            {
                reached.push_back(Core::ReachedAlarm{ alarm.GetDisplayName(), *m_quotes.at(alarm.GetGemKey()).GetPrice(), alarm.GetTargetPrice() });
            }
            if (wasTriggered != alarm.IsTriggered()) isChanged = true;
        }
        return isChanged;
    }

    bool AuctionGemPriceService::UpdateAlarmState(Models::PriceAlarm& alarm, bool canNotify)
    {
        // 시세가 있고 가격도 있을 때만 판정
        const auto quote{ m_quotes.find(alarm.GetGemKey()) };
        if (quote == m_quotes.end() || !quote->second.GetPrice().has_value()) return false;

        const bool isReached{ *quote->second.GetPrice() <= alarm.GetTargetPrice() };
        alarm.SetTriggered(isReached);

        if (!isReached)
        {
            alarm.SetNotified(false);
            return false;
        }

        if (alarm.IsNotified()) return false;   // 이미 알림
        alarm.SetNotified(true);
        return canNotify;
    }
}