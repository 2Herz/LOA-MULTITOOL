module LostArkMultiTool.Models.PriceHistory;

import std;

namespace LostArkMultiTool::Models
{
    const PriceHistory::DailyPrices& PriceHistory::GetDays(std::string_view itemKey) const
    {
        static const DailyPrices EMPTY_DAYS;   // 기록이 없는 품목에 돌려줄 빈 목록 (매번 만들지 않도록 공유)

        const auto found{ m_history.find(itemKey) };   // 투명 비교라 string_view로 바로 찾음
        return found != m_history.end() ? found->second : EMPTY_DAYS;   // 없으면 빈 목록
    }

    PriceHistory::HistoryMap& PriceHistory::GetAll() noexcept
    {
        return m_history;   // 저장소가 읽은 값을 채울 때 사용
    }

    const PriceHistory::HistoryMap& PriceHistory::GetAll() const noexcept
    {
        return m_history;   // 저장소가 저장할 때 사용
    }

    bool PriceHistory::RecordLowest(const std::string& itemKey, const std::string& dayKey, std::int64_t price)
    {
        DailyPrices& days{ m_history[itemKey] };   // 품목 기록 (없으면 새로 만듦)

        // 이미 기록된 값이 더 싸거나 같으면 그대로 둠
        if (const auto recorded{ days.find(dayKey) }; recorded != days.end() && price >= recorded->second) return false;

        days.insert_or_assign(dayKey, price);   // 그날 최저가로 기록
        return true;
    }

    bool PriceHistory::Prune(std::string_view oldestKeptKey)
    {
        bool isChanged{};   // 지운 기록이 있는지

        for (auto item{ m_history.begin() }; item != m_history.end();)
        {
            DailyPrices& days{ item->second };   // 이 품목의 일별 기록

            // 날짜 키는 "yyyy-MM-dd"라 글자 순서 = 날짜 순서 → 가장 오래 남길 날짜보다 앞선 기록을 모두 지움
            const auto firstKept{ days.lower_bound(std::string{ oldestKeptKey }) };
            if (firstKept != days.begin())
            {
                days.erase(days.begin(), firstKept);   // 보관 기간이 지난 기록 삭제
                isChanged = true;
            }

            // 남은 기록이 없는 품목은 통째로 지움
            if (days.empty())
            {
                item = m_history.erase(item);
                isChanged = true;
            }
            else
            {
                ++item;
            }
        }
        return isChanged;
    }
}
