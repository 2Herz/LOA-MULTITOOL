module LostArkMultiTool.Services.JSONPriceHistoryRepository;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Models.PriceHistory;
import LostArkMultiTool.Services.JSONFileStore;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 저장 파일 키 이름 (보석 검색기 저장 파일과 같은 구조) =====
        constexpr std::string_view KEY_HISTORY{ "History" };
        constexpr std::string_view STRUCTURE_ERROR_MESSAGE{ "시세 기록 저장 파일 구조가 올바르지 않습니다." };

        // 구조가 어긋나면 예외 → JSONFileStore가 손상으로 처리 (백업 복구)
        [[noreturn]] void ThrowStructureError()
        {
            throw std::runtime_error{ std::string{ STRUCTURE_ERROR_MESSAGE } };   // 손상 안내 예외
        }

        // 값 형식이 기대와 다르면 구조 오류
        void RequireType(const JSONValue& value, JSONType expected)
        {
            if (value.GetType() != expected) ThrowStructureError();
        }

        // 없거나 null인 값인지
        bool IsNullOrMissing(const JSONValue& value) noexcept
        {
            return value.GetType() == JSONType::NULL_VALUE;
        }

        // ===== 읽기: JSONValue 트리 → 모델 =====
        Models::PriceHistory ReadHistory(const JSONValue& root)
        {
            RequireType(root, JSONType::OBJECT_VALUE);   // 최상위는 객체여야 함

            Models::PriceHistory history;   // 읽어서 채울 기록
            const JSONValue& historyValue{ root.FindMember(KEY_HISTORY) };   // "History" 멤버
            if (IsNullOrMissing(historyValue)) return history;   // 기록이 없으면 빈 상태
            RequireType(historyValue, JSONType::OBJECT_VALUE);

            // 품목마다 날짜별 최저가 읽기
            for (const auto& [itemKey, daysValue] : std::views::zip(historyValue.GetKeys(), historyValue.GetValues()))
            {
                Models::PriceHistory::DailyPrices& days{ history.GetAll()[itemKey] };   // 이 품목의 일별 기록
                if (IsNullOrMissing(daysValue)) continue;   // null이면 빈 기록
                RequireType(daysValue, JSONType::OBJECT_VALUE);

                for (const auto& [dateKey, price] : std::views::zip(daysValue.GetKeys(), daysValue.GetValues()))
                {
                    RequireType(price, JSONType::NUMBER_VALUE);   // 가격은 숫자여야 함
                    days.insert_or_assign(dateKey, std::llround(price.GetNumber()));   // 정수 골드로 보관
                }
            }
            return history;
        }

        // ===== 쓰기: 모델 → JSON 글자 =====
        std::string BuildJSONText(const Models::PriceHistory& history)
        {
            std::vector<std::string> itemKeys;      // 품목 키 목록
            std::vector<JSONValue> itemValues;      // 품목별 일별 기록 객체
            for (const auto& [itemKey, days] : history.GetAll())
            {
                std::vector<std::string> dateKeys;     // 날짜 키 목록 (정렬 맵이라 날짜순)
                std::vector<JSONValue> prices;         // 날짜별 최저가
                for (const auto& [dateKey, price] : days)
                {
                    dateKeys.push_back(dateKey);
                    prices.push_back(JSONValue::CreateNumber(static_cast<double>(price)));
                }
                itemKeys.push_back(itemKey);
                itemValues.push_back(JSONValue::CreateObject(std::move(dateKeys), std::move(prices)));
            }

            return WriteJSONText(JSONValue::CreateObject(
                { std::string{ KEY_HISTORY } },
                { JSONValue::CreateObject(std::move(itemKeys), std::move(itemValues)) }));
        }
    }

    JSONPriceHistoryRepository::JSONPriceHistoryRepository(std::string_view fileName)
        : m_fileStore{ fileName }   // 사용자 데이터 폴더의 새 파일 (가져올 이전 파일 없음)
    {
        // 변환이 끝까지 성공한 파일만 반영 (중간에 실패하면 loaded는 그대로 → 백업 파일로 다시 시도)
        std::optional<Models::PriceHistory> loaded;
        JSONLoadResult result{ m_fileStore.Load([&loaded](const JSONValue& root) { loaded = ReadHistory(root); }) };
        m_loadWarning = std::move(result.m_warning);   // 손상 복구 안내 보관

        m_history = loaded.has_value() ? std::move(*loaded) : Models::PriceHistory{};   // 첫 실행 또는 복구 실패는 빈 기록

        // 보관 기간(365일)이 지난 기록 정리 (다음 저장 때 반영, 날짜는 오전 6시 기준 하루)
        const auto oldestKept{ Core::LocalTime::AddDays(Core::LocalTime::GetGameToday(), -Core::APPConstants::HISTORY_KEEP_DAYS) };
        static_cast<void>(m_history.Prune(Core::LocalTime::MakeDateKey(oldestKept)));
    }

    Models::PriceHistory& JSONPriceHistoryRepository::GetHistory()
    {
        return m_history;
    }

    const Models::PriceHistory& JSONPriceHistoryRepository::GetHistory() const
    {
        return m_history;
    }

    const std::optional<std::string>& JSONPriceHistoryRepository::GetLoadWarning() const noexcept
    {
        return m_loadWarning;
    }

    void JSONPriceHistoryRepository::Save()
    {
        m_fileStore.Save(BuildJSONText(m_history));   // 임시 파일 → 교체 방식으로 저장 (기존 파일은 .bak으로 보관)
    }
}
