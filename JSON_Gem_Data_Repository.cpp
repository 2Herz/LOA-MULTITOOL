module LostArkMultiTool.Services.JSONGemDataRepository;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Models.GemData;
import LostArkMultiTool.Models.PriceAlarm;
import LostArkMultiTool.Models.RefreshSettings;
import LostArkMultiTool.Services.JSONFileStore;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 저장 파일 키 이름 (원본 GemData.json과 같은 구조) =====
        constexpr std::string_view KEY_ALARMS{ "Alarms" };
        constexpr std::string_view KEY_TYPE_NAME{ "TypeName" };
        constexpr std::string_view KEY_LEVEL{ "Level" };
        constexpr std::string_view KEY_TARGET_PRICE{ "TargetPrice" };
        constexpr std::string_view KEY_IS_TRIGGERED{ "IsTriggered" };
        constexpr std::string_view KEY_LAST_PRICES{ "LastPrices" };
        constexpr std::string_view KEY_HISTORY{ "History" };
        constexpr std::string_view KEY_SETTINGS{ "Settings" };
        constexpr std::string_view KEY_WATCH_INTERVAL{ "WatchIntervalSeconds" };
        constexpr std::string_view KEY_FULL_INTERVAL{ "FullIntervalMinutes" };
        constexpr std::string_view KEY_AUTO_REFRESH{ "IsAutoRefreshEnabled" };

        constexpr std::string_view STRUCTURE_ERROR_MESSAGE{ "보석 저장 파일 구조가 올바르지 않습니다." };

        // ===== 읽기: JSONValue 트리 → 모델 (구조가 어긋나면 예외 → JSONFileStore가 손상으로 처리) =====
        [[noreturn]] void ThrowStructureError()
        {
            throw std::runtime_error{ std::string{ STRUCTURE_ERROR_MESSAGE } };
        }

        bool IsNullOrMissing(const JSONValue& value) noexcept
        {
            return value.GetType() == JSONType::NULL_VALUE;
        }

        void RequireType(const JSONValue& value, JSONType expected)
        {
            if (value.GetType() != expected) ThrowStructureError();
        }

        // 숫자 멤버 (없거나 null이면 기본값, 숫자가 아니면 구조 오류)
        double ReadNumber(const JSONValue& object, std::string_view key, double fallback)
        {
            const JSONValue& value{ object.FindMember(key) };
            if (IsNullOrMissing(value)) return fallback;

            RequireType(value, JSONType::NUMBER_VALUE);
            return value.GetNumber();
        }

        bool ReadBool(const JSONValue& object, std::string_view key, bool fallback)
        {
            const JSONValue& value{ object.FindMember(key) };
            if (IsNullOrMissing(value)) return fallback;

            RequireType(value, JSONType::BOOL_VALUE);
            return value.GetBool();
        }

        std::string ReadString(const JSONValue& object, std::string_view key)
        {
            const JSONValue& value{ object.FindMember(key) };
            if (IsNullOrMissing(value)) return {};

            RequireType(value, JSONType::STRING_VALUE);
            return value.GetString();
        }

        std::int64_t ToPrice(const JSONValue& value)
        {
            RequireType(value, JSONType::NUMBER_VALUE);
            return std::llround(value.GetNumber());
        }

        void ReadAlarms(const JSONValue& alarmsValue, std::vector<Models::PriceAlarm>& alarms)
        {
            if (IsNullOrMissing(alarmsValue)) return;
            RequireType(alarmsValue, JSONType::ARRAY_VALUE);

            for (const JSONValue& alarmValue : alarmsValue.GetItems())
            {
                if (IsNullOrMissing(alarmValue)) continue;   // null 알림은 건너뜀 (원본 RemoveAll(a is null))
                RequireType(alarmValue, JSONType::OBJECT_VALUE);

                Models::PriceAlarm alarm{
                    Models::GemSelection{ ReadString(alarmValue, KEY_TYPE_NAME), static_cast<int>(ReadNumber(alarmValue, KEY_LEVEL, 0.0)) },
                    std::llround(ReadNumber(alarmValue, KEY_TARGET_PRICE, 0.0)) };
                alarm.SetTriggered(ReadBool(alarmValue, KEY_IS_TRIGGERED, false));
                alarms.push_back(std::move(alarm));
            }
        }

        void ReadLastPrices(const JSONValue& pricesValue, std::map<std::string, std::int64_t>& lastPrices)
        {
            if (IsNullOrMissing(pricesValue)) return;
            RequireType(pricesValue, JSONType::OBJECT_VALUE);

            for (const auto& [gemKey, price] : std::views::zip(pricesValue.GetKeys(), pricesValue.GetValues()))
            {
                lastPrices.insert_or_assign(gemKey, ToPrice(price));
            }
        }

        void ReadHistory(const JSONValue& historyValue, std::map<std::string, std::map<std::string, std::int64_t>>& history)
        {
            if (IsNullOrMissing(historyValue)) return;
            RequireType(historyValue, JSONType::OBJECT_VALUE);

            for (const auto& [gemKey, daysValue] : std::views::zip(historyValue.GetKeys(), historyValue.GetValues()))
            {
                std::map<std::string, std::int64_t>& days{ history[gemKey] };   // null이면 빈 기록 (원본과 같음)
                if (IsNullOrMissing(daysValue)) continue;

                RequireType(daysValue, JSONType::OBJECT_VALUE);
                for (const auto& [dateKey, price] : std::views::zip(daysValue.GetKeys(), daysValue.GetValues()))
                {
                    days.insert_or_assign(dateKey, ToPrice(price));
                }
            }
        }

        Models::RefreshSettings ReadSettings(const JSONValue& settingsValue)
        {
            Models::RefreshSettings settings;
            if (IsNullOrMissing(settingsValue)) return settings;
            RequireType(settingsValue, JSONType::OBJECT_VALUE);

            settings.SetWatchIntervalSeconds(static_cast<int>(ReadNumber(settingsValue, KEY_WATCH_INTERVAL, settings.GetWatchIntervalSeconds())));
            settings.SetFullIntervalMinutes(static_cast<int>(ReadNumber(settingsValue, KEY_FULL_INTERVAL, settings.GetFullIntervalMinutes())));
            settings.SetAutoRefreshEnabled(ReadBool(settingsValue, KEY_AUTO_REFRESH, settings.IsAutoRefreshEnabled()));
            return settings;
        }

        Models::GemData ReadGemData(const JSONValue& root)
        {
            RequireType(root, JSONType::OBJECT_VALUE);

            Models::GemData data;
            ReadAlarms(root.FindMember(KEY_ALARMS), data.GetAlarms());
            ReadLastPrices(root.FindMember(KEY_LAST_PRICES), data.GetLastPrices());
            ReadHistory(root.FindMember(KEY_HISTORY), data.GetHistory());
            data.SetSettings(ReadSettings(root.FindMember(KEY_SETTINGS)));
            return data;
        }

        // ===== 쓰기: 모델 → JSONValue 트리 (키 순서 = 원본 C# 속성 순서) =====
        JSONValue MakeNumber(std::int64_t value)
        {
            return JSONValue::CreateNumber(static_cast<double>(value));
        }

        JSONValue WriteAlarms(const std::vector<Models::PriceAlarm>& alarms)
        {
            std::vector<JSONValue> items;
            items.reserve(alarms.size());
            for (const Models::PriceAlarm& alarm : alarms)
            {
                items.push_back(JSONValue::CreateObject(
                    { std::string{ KEY_TYPE_NAME }, std::string{ KEY_LEVEL }, std::string{ KEY_TARGET_PRICE }, std::string{ KEY_IS_TRIGGERED } },
                    { JSONValue::CreateString(alarm.GetGem().m_typeName), MakeNumber(alarm.GetGem().m_level),
                      MakeNumber(alarm.GetTargetPrice()), JSONValue::CreateBool(alarm.IsTriggered()) }));
            }
            return JSONValue::CreateArray(std::move(items));
        }

        JSONValue WritePriceMap(const std::map<std::string, std::int64_t>& prices)
        {
            std::vector<std::string> keys;
            std::vector<JSONValue> values;
            keys.reserve(prices.size());
            values.reserve(prices.size());
            for (const auto& [key, price] : prices)   // 정렬 맵이라 저장 순서가 고정됨 (날짜는 날짜순)
            {
                keys.push_back(key);
                values.push_back(MakeNumber(price));
            }
            return JSONValue::CreateObject(std::move(keys), std::move(values));
        }

        JSONValue WriteHistory(const std::map<std::string, std::map<std::string, std::int64_t>>& history)
        {
            std::vector<std::string> keys;
            std::vector<JSONValue> values;
            for (const auto& [gemKey, days] : history)
            {
                keys.push_back(gemKey);
                values.push_back(WritePriceMap(days));
            }
            return JSONValue::CreateObject(std::move(keys), std::move(values));
        }

        JSONValue WriteSettings(const Models::RefreshSettings& settings)
        {
            return JSONValue::CreateObject(
                { std::string{ KEY_WATCH_INTERVAL }, std::string{ KEY_FULL_INTERVAL }, std::string{ KEY_AUTO_REFRESH } },
                { MakeNumber(settings.GetWatchIntervalSeconds()), MakeNumber(settings.GetFullIntervalMinutes()),
                  JSONValue::CreateBool(settings.IsAutoRefreshEnabled()) });
        }

        std::string BuildJSONText(const Models::GemData& data)
        {
            return WriteJSONText(JSONValue::CreateObject(
                { std::string{ KEY_ALARMS }, std::string{ KEY_LAST_PRICES }, std::string{ KEY_HISTORY }, std::string{ KEY_SETTINGS } },
                { WriteAlarms(data.GetAlarms()), WritePriceMap(data.GetLastPrices()), WriteHistory(data.GetHistory()),
                  WriteSettings(data.GetSettings()) }));
        }
    }

    JSONGemDataRepository::JSONGemDataRepository()
        // 예전 버전은 실행 파일 폴더에 저장했음 → 사용자 데이터 폴더에 파일이 없으면 그 파일을 한 번 가져옴
        : m_fileStore{ Core::APPConstants::GEM_DATA_FILE_NAME, { Core::APPConstants::GetDataFilePath(Core::APPConstants::GEM_DATA_FILE_NAME) } }
    {
        // 변환이 끝까지 성공한 파일만 반영 (중간에 실패하면 loaded는 그대로 → 백업 파일로 다시 시도)
        std::optional<Models::GemData> loaded;
        JSONLoadResult result{ m_fileStore.Load([&loaded](const JSONValue& root) { loaded = ReadGemData(root); }) };
        m_loadWarning = std::move(result.m_warning);

        m_data = loaded.has_value() ? std::move(*loaded) : Models::GemData{};   // 첫 실행 또는 복구 실패
        m_data.Normalize();

        // 오래된 기록 · 조회 범위 밖 기록 정리 (다음 저장 때 반영, 원본과 같음 / 날짜는 오전 6시 기준 하루)
        const auto oldestKept{ Core::LocalTime::AddDays(Core::LocalTime::GetGameToday(), -Core::APPConstants::HISTORY_KEEP_DAYS) };
        static_cast<void>(m_data.Prune(Core::LocalTime::MakeDateKey(oldestKept)));

        // 가져온 기록은 바로 새 위치에 저장 (실패해도 메모리의 값은 유지 → 다음 저장 때 다시 시도, 다음 실행 때 다시 가져옴)
        if (!result.m_isImported) return;
        try
        {
            Save();
        }
        catch (const std::exception&)
        {
            // 안내는 가져오기 문구로 이미 표시됨 (저장 실패는 다음 저장에서 사용자에게 알림)
        }
    }

    Models::GemData& JSONGemDataRepository::GetData()
    {
        return m_data;
    }

    const Models::GemData& JSONGemDataRepository::GetData() const
    {
        return m_data;
    }

    const std::optional<std::string>& JSONGemDataRepository::GetLoadWarning() const noexcept
    {
        return m_loadWarning;
    }

    void JSONGemDataRepository::Save()
    {
        m_fileStore.Save(BuildJSONText(m_data));
    }
}