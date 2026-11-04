module LostArkMultiTool.Services.JSONGrowthDataRepository;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.LocalTime;
import LostArkMultiTool.Models.CharacterInfo;
import LostArkMultiTool.Models.GrowthData;
import LostArkMultiTool.Models.Season;
import LostArkMultiTool.Models.WeeklyRecord;
import LostArkMultiTool.Services.JSONFileStore;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 저장 파일 키 이름 (원본 RosterGrowthData_v2.json과 같은 구조 → 원본 파일을 그대로 읽음) =====
        constexpr std::string_view KEY_DATA_VERSION{ "DataVersion" };
        constexpr std::string_view KEY_CHARACTERS{ "Characters" };
        constexpr std::string_view KEY_SEASONS{ "Seasons" };
        constexpr std::string_view KEY_SELECTED_SEASON{ "SelectedSeasonNumber" };
        constexpr std::string_view KEY_LEGACY_RECORDS{ "Records" };   // v1: 루트의 주차 목록 (읽기만)
        constexpr std::string_view KEY_SERVER_NAME{ "ServerName" };
        constexpr std::string_view KEY_CHARACTER_NAME{ "CharacterName" };
        constexpr std::string_view KEY_CLASS_NAME{ "CharacterClassName" };
        constexpr std::string_view KEY_ITEM_MAX_LEVEL{ "ItemMaxLevel" };
        constexpr std::string_view KEY_ITEM_AVG_LEVEL{ "ItemAvgLevel" };
        constexpr std::string_view KEY_SEASON_NUMBER{ "SeasonNumber" };
        constexpr std::string_view KEY_RECORDS{ "Records" };
        constexpr std::string_view KEY_WEEK_NUMBER{ "WeekNumber" };
        constexpr std::string_view KEY_SCORES{ "Scores" };
        constexpr std::string_view KEY_START_DATE{ "StartDate" };   // 시즌 기간 (시즌 5부터, "yyyy-MM-dd")
        constexpr std::string_view KEY_END_DATE{ "EndDate" };

        constexpr std::string_view STRUCTURE_ERROR_MESSAGE{ "낙원력 저장 파일 구조가 올바르지 않습니다." };

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

        int ReadInteger(const JSONValue& object, std::string_view key, int fallback)
        {
            return static_cast<int>(std::lround(ReadNumber(object, key, static_cast<double>(fallback))));
        }

        // 문자열 또는 숫자로 온 값을 글자로 (원본 object 속성 → ToString과 같음, 없으면 nullopt)
        std::optional<std::string> ReadOptionalText(const JSONValue& object, std::string_view key)
        {
            const JSONValue& value{ object.FindMember(key) };
            switch (value.GetType())
            {
                case JSONType::STRING_VALUE:
                    return value.GetString();
                case JSONType::NUMBER_VALUE:
                {
                    // 편집기의 consteval 형식 검사를 거치지 않도록 런타임 포맷 사용 (결과는 std::format과 같음)
                    const double number{ value.GetNumber() };
                    return std::vformat("{}", std::make_format_args(number));
                }
                case JSONType::NULL_VALUE:
                    return std::nullopt;
                default:
                    ThrowStructureError();
            }
        }

        std::vector<Models::CharacterInfo> ReadCharacters(const JSONValue& charactersValue)
        {
            std::vector<Models::CharacterInfo> characters;
            if (IsNullOrMissing(charactersValue)) return characters;
            RequireType(charactersValue, JSONType::ARRAY_VALUE);

            for (const JSONValue& characterValue : charactersValue.GetItems())
            {
                if (IsNullOrMissing(characterValue)) continue;   // null 캐릭터는 건너뜀 (원본 RemoveAll(c is null))
                RequireType(characterValue, JSONType::OBJECT_VALUE);

                characters.emplace_back(
                    ReadOptionalText(characterValue, KEY_CHARACTER_NAME).value_or(std::string{}),
                    ReadOptionalText(characterValue, KEY_CLASS_NAME),
                    ReadOptionalText(characterValue, KEY_ITEM_MAX_LEVEL),
                    ReadOptionalText(characterValue, KEY_ITEM_AVG_LEVEL),
                    ReadOptionalText(characterValue, KEY_SERVER_NAME));
            }
            return characters;
        }

        Models::WeeklyRecord ReadRecord(const JSONValue& recordValue)
        {
            RequireType(recordValue, JSONType::OBJECT_VALUE);
            Models::WeeklyRecord record{ ReadInteger(recordValue, KEY_WEEK_NUMBER, 0) };

            const JSONValue& scoresValue{ recordValue.FindMember(KEY_SCORES) };
            if (IsNullOrMissing(scoresValue)) return record;   // "Scores": null → 빈 기록 (원본 field ?? [])
            RequireType(scoresValue, JSONType::OBJECT_VALUE);

            for (const auto& [name, score] : std::views::zip(scoresValue.GetKeys(), scoresValue.GetValues()))
            {
                RequireType(score, JSONType::NUMBER_VALUE);
                record.SetScore(name, score.GetNumber());   // 0 이하는 미기록으로 걸러짐
            }
            return record;
        }

        std::vector<Models::WeeklyRecord> ReadRecords(const JSONValue& recordsValue)
        {
            std::vector<Models::WeeklyRecord> records;
            if (IsNullOrMissing(recordsValue)) return records;
            RequireType(recordsValue, JSONType::ARRAY_VALUE);

            for (const JSONValue& recordValue : recordsValue.GetItems())
            {
                if (IsNullOrMissing(recordValue)) continue;   // 빈 항목 방어 (원본 Records.RemoveAll(r is null))
                records.push_back(ReadRecord(recordValue));
            }
            return records;
        }

        std::vector<Models::Season> ReadSeasons(const JSONValue& seasonsValue)
        {
            std::vector<Models::Season> seasons;
            if (IsNullOrMissing(seasonsValue)) return seasons;
            RequireType(seasonsValue, JSONType::ARRAY_VALUE);

            for (const JSONValue& seasonValue : seasonsValue.GetItems())
            {
                if (IsNullOrMissing(seasonValue)) continue;
                RequireType(seasonValue, JSONType::OBJECT_VALUE);

                Models::Season season{ ReadInteger(seasonValue, KEY_SEASON_NUMBER, 0) };
                season.SetRecords(ReadRecords(seasonValue.FindMember(KEY_RECORDS)));

                // 시즌 기간 (시즌 5부터, 둘 다 올바른 날짜일 때만 → 예전 파일은 기간 없음 = 잠금 없이 자유 입력)
                const std::optional<std::chrono::year_month_day> startDate{ Core::LocalTime::ParseDateKey(seasonValue.FindMember(KEY_START_DATE).GetString()) };
                const std::optional<std::chrono::year_month_day> endDate{ Core::LocalTime::ParseDateKey(seasonValue.FindMember(KEY_END_DATE).GetString()) };
                if (startDate.has_value() && endDate.has_value()) season.SetPeriod(*startDate, *endDate);
                seasons.push_back(std::move(season));
            }
            return seasons;
        }

        // v1(루트에 주차 목록) · v2(시즌별) 모두 읽음 → EnsureValid가 v1을 첫 시즌(시즌 4)으로 옮김
        Models::GrowthData ReadGrowthData(const JSONValue& root)
        {
            RequireType(root, JSONType::OBJECT_VALUE);

            Models::GrowthData data;
            data.SetCharacters(ReadCharacters(root.FindMember(KEY_CHARACTERS)));
            data.SetSeasons(ReadSeasons(root.FindMember(KEY_SEASONS)));
            data.SetSelectedSeasonNumber(ReadInteger(root, KEY_SELECTED_SEASON, Models::GrowthData::FIRST_SEASON_NUMBER));
            data.EnsureValid(ReadRecords(root.FindMember(KEY_LEGACY_RECORDS)));
            return data;
        }

        // ===== 쓰기: 모델 → JSONValue 트리 (키 순서 = 원본 C# 속성 순서) =====
        JSONValue MakeNumber(double value)
        {
            return JSONValue::CreateNumber(value);
        }

        JSONValue MakeNullableString(const std::optional<std::string>& text)
        {
            return text.has_value() ? JSONValue::CreateString(*text) : JSONValue{};
        }

        JSONValue WriteCharacters(const std::vector<Models::CharacterInfo>& characters)
        {
            std::vector<JSONValue> items;
            items.reserve(characters.size());
            for (const Models::CharacterInfo& character : characters)
            {
                items.push_back(JSONValue::CreateObject(
                    { std::string{ KEY_SERVER_NAME }, std::string{ KEY_CHARACTER_NAME }, std::string{ KEY_CLASS_NAME },
                      std::string{ KEY_ITEM_MAX_LEVEL }, std::string{ KEY_ITEM_AVG_LEVEL } },
                    { MakeNullableString(character.GetServerName()), JSONValue::CreateString(character.GetCharacterName()),
                      MakeNullableString(character.GetCharacterClassName()), MakeNullableString(character.GetItemMaxLevel()),
                      MakeNullableString(character.GetItemAvgLevel()) }));
            }
            return JSONValue::CreateArray(std::move(items));
        }

        JSONValue WriteScores(const std::map<std::string, double>& scores)
        {
            std::vector<std::string> keys;
            std::vector<JSONValue> values;
            keys.reserve(scores.size());
            values.reserve(scores.size());
            for (const auto& [name, score] : scores)   // 정렬 맵이라 저장 순서가 고정됨
            {
                keys.push_back(name);
                values.push_back(MakeNumber(score));
            }
            return JSONValue::CreateObject(std::move(keys), std::move(values));
        }

        JSONValue WriteRecords(const std::vector<Models::WeeklyRecord>& records)
        {
            std::vector<JSONValue> items;
            items.reserve(records.size());
            for (const Models::WeeklyRecord& record : records)
            {
                items.push_back(JSONValue::CreateObject(
                    { std::string{ KEY_WEEK_NUMBER }, std::string{ KEY_SCORES } },
                    { MakeNumber(record.GetWeekNumber()), WriteScores(record.GetScores()) }));
            }
            return JSONValue::CreateArray(std::move(items));
        }

        JSONValue WriteSeasons(const std::vector<Models::Season>& seasons)
        {
            std::vector<JSONValue> items;
            items.reserve(seasons.size());
            for (const Models::Season& season : seasons)
            {
                std::vector<std::string> keys{ std::string{ KEY_SEASON_NUMBER }, std::string{ KEY_RECORDS } };
                std::vector<JSONValue> values{ MakeNumber(season.GetSeasonNumber()), WriteRecords(season.GetRecords()) };

                // 기간이 있는 시즌만 "StartDate" · "EndDate" ("yyyy-MM-dd")
                if (season.HasPeriod())
                {
                    keys.emplace_back(KEY_START_DATE);
                    values.push_back(JSONValue::CreateString(Core::LocalTime::MakeDateKey(*season.GetStartDate())));
                    keys.emplace_back(KEY_END_DATE);
                    values.push_back(JSONValue::CreateString(Core::LocalTime::MakeDateKey(*season.GetEndDate())));
                }
                items.push_back(JSONValue::CreateObject(std::move(keys), std::move(values)));
            }
            return JSONValue::CreateArray(std::move(items));
        }

        std::string BuildJSONText(const Models::GrowthData& data)
        {
            return WriteJSONText(JSONValue::CreateObject(
                { std::string{ KEY_DATA_VERSION }, std::string{ KEY_CHARACTERS }, std::string{ KEY_SEASONS }, std::string{ KEY_SELECTED_SEASON } },
                { MakeNumber(Models::GrowthData::CURRENT_DATA_VERSION), WriteCharacters(data.GetCharacters()), WriteSeasons(data.GetSeasons()),
                  MakeNumber(data.GetSelectedSeasonNumber()) }));
        }

        // 원본 낙원력 트래커의 기록 위치 (본 파일 → 이전 버전 파일 순서, 각 파일의 .bak은 JSONFileStore가 이어서 시도)
        std::vector<std::filesystem::path> MakeImportPaths()
        {
            return {
                Core::APPConstants::GetLocalAppDataFilePath(Core::APPConstants::PARADISE_ORIGINAL_FOLDER_NAME, Core::APPConstants::PARADISE_ORIGINAL_FILE_NAME),
                Core::APPConstants::GetLocalAppDataFilePath(Core::APPConstants::PARADISE_ORIGINAL_FOLDER_NAME, Core::APPConstants::PARADISE_LEGACY_FILE_NAME)
            };
        }
    }

    JSONGrowthDataRepository::JSONGrowthDataRepository()
        : m_fileStore{ Core::APPConstants::PARADISE_DATA_FILE_NAME, MakeImportPaths() }
    {
        // 변환이 끝까지 성공한 파일만 반영 (중간에 실패하면 loaded는 그대로 → 백업 · 다음 후보 파일로 다시 시도)
        std::optional<Models::GrowthData> loaded;
        JSONLoadResult result{ m_fileStore.Load([&loaded](const JSONValue& root) { loaded = ReadGrowthData(root); }) };
        m_loadWarning = std::move(result.m_warning);

        m_data = loaded.has_value() ? std::move(*loaded) : Models::GrowthData{};   // 첫 실행 또는 복구 실패
        m_data.EnsureValid();

        // 가져온 기록은 바로 멀티툴 전용 파일에 저장 (원본 파일은 건드리지 않음, 실패하면 다음 실행 때 다시 가져옴)
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

    Models::GrowthData& JSONGrowthDataRepository::GetData()
    {
        return m_data;
    }

    const Models::GrowthData& JSONGrowthDataRepository::GetData() const
    {
        return m_data;
    }

    const std::optional<std::string>& JSONGrowthDataRepository::GetLoadWarning() const noexcept
    {
        return m_loadWarning;
    }

    void JSONGrowthDataRepository::Save()
    {
        m_fileStore.Save(BuildJSONText(m_data));
    }
}
