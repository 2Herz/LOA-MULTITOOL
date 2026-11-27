module LostArkMultiTool.Services.JSONBoundGemRepository;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.BoundGemRepository;
import LostArkMultiTool.Models.BoundGemData;
import LostArkMultiTool.Models.BoundGemRosterData;
import LostArkMultiTool.Services.JSONFileStore;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== JSON 키 =====
        constexpr std::string_view KEY_CHARACTERS{ "Characters" };
        constexpr std::string_view KEY_CLASS_NAME{ "ClassName" };
        constexpr std::string_view KEY_ITEM_LEVEL{ "ItemLevel" };
        constexpr std::string_view KEY_QUANTITIES{ "Quantities" };
        constexpr std::string_view KEY_NAME{ "Name" };
        constexpr std::string_view KEY_GRADE{ "Grade" };
        constexpr std::string_view KEY_COUNT{ "Count" };
        constexpr std::string_view KEY_HISTORY{ "History" };   // { "yyyy-MM-dd": 원정대 총 가치 }
        constexpr std::string_view KEY_SCAN_BOUND{ "ScanBound" };         // 화면 인식 귀속 묶음 수량
        constexpr std::string_view KEY_SCAN_TRADABLE{ "ScanTradable" };   // 화면 인식 거래 가능 묶음 수량
        constexpr std::string_view KEY_SCAN_PROCESSED{ "ScanProcessed" }; // 화면 인식 가공 젬 구분 글자 목록

        // 숫자 항목 → 수량 (없으면 nullopt)
        [[nodiscard]] std::optional<int> ReadScanCount(const JSONValue& value)
        {
            if (value.GetType() != JSONType::NUMBER_VALUE) return std::nullopt;   // 없음
            return static_cast<int>(std::clamp(value.GetNumber(), 0.0, static_cast<double>(Core::APPConstants::MAX_BOUND_GEM_COUNT)));   // 범위
        }

        // 이름 목록에서 번호 찾기 (없으면 nullopt → 모르는 항목은 건너뜀)
        template <std::size_t Count>
        [[nodiscard]] std::optional<std::size_t> FindIndex(const std::array<std::string_view, Count>& names, std::string_view name) noexcept
        {
            const auto found{ std::ranges::find(names, name) };
            if (found == names.end()) return std::nullopt;
            return static_cast<std::size_t>(std::distance(names.begin(), found));
        }

        // 수량 목록 JSON → 모델 (없거나 배열이 아니면 빈 수량)
        [[nodiscard]] Models::BoundGemData ReadQuantities(const JSONValue& quantities)
        {
            Models::BoundGemData data;
            for (const JSONValue& entry : quantities.GetItems())
            {
                const std::optional<std::size_t> gemIndex{ FindIndex(Models::BoundGemData::GEM_NAMES, entry.FindMember(KEY_NAME).GetString()) };
                const std::optional<std::size_t> gradeIndex{ FindIndex(Models::BoundGemData::GRADE_NAMES, entry.FindMember(KEY_GRADE).GetString()) };
                if (!gemIndex.has_value() || !gradeIndex.has_value()) continue;

                const double count{ std::clamp(entry.FindMember(KEY_COUNT).GetNumber(), 0.0, static_cast<double>(Core::APPConstants::MAX_BOUND_GEM_COUNT)) };
                data.SetQuantity(*gemIndex, *gradeIndex, static_cast<int>(count));
                Models::GemScanSlot slot;                                                            // 화면 인식 내역 (예전 파일은 없음)
                slot.m_bound = ReadScanCount(entry.FindMember(KEY_SCAN_BOUND));
                slot.m_tradable = ReadScanCount(entry.FindMember(KEY_SCAN_TRADABLE));
                for (const JSONValue& key : entry.FindMember(KEY_SCAN_PROCESSED).GetItems())
                    if (key.GetType() == JSONType::STRING_VALUE && !key.GetString().empty()) slot.m_processed.push_back(key.GetString());
                data.SetScanSlot(*gemIndex, *gradeIndex, std::move(slot));
            }
            return data;
        }

        // JSON → 모델 (최상위가 객체가 아니면 예외 → JSONFileStore가 손상으로 처리)
        [[nodiscard]] Models::BoundGemRosterData ReadData(const JSONValue& root)
        {
            if (root.GetType() != JSONType::OBJECT_VALUE) throw std::runtime_error{ "귀속 젬 저장 파일 형식이 올바르지 않습니다." };

            Models::BoundGemRosterData data;
            for (const JSONValue& entry : root.FindMember(KEY_CHARACTERS).GetItems())
            {
                const std::string& name{ entry.FindMember(KEY_NAME).GetString() };
                if (name.empty()) continue;   // 이름 없는 캐릭터는 건너뜀

                const JSONValue& className{ entry.FindMember(KEY_CLASS_NAME) };
                data.AddCharacter(Models::BoundGemCharacter{ name,
                    className.GetType() == JSONType::STRING_VALUE ? std::optional<std::string>{ className.GetString() } : std::nullopt,
                    entry.FindMember(KEY_ITEM_LEVEL).GetNumber(),
                    ReadQuantities(entry.FindMember(KEY_QUANTITIES)) });
            }

            // 캐릭터별 저장 이전 파일 (최상위 "Quantities") → 공용 수량
            data.GetUnassigned() = ReadQuantities(root.FindMember(KEY_QUANTITIES));

            // 날짜별 원정대 총 가치 (없으면 빈 기록)
            const JSONValue& history{ root.FindMember(KEY_HISTORY) };
            const std::vector<std::string>& dateKeys{ history.GetKeys() };
            const std::vector<JSONValue>& values{ history.GetValues() };
            for (std::size_t index{}; index < std::min(dateKeys.size(), values.size()); ++index)
            {
                if (values[index].GetType() == JSONType::NUMBER_VALUE) data.RecordHistory(dateKeys[index], values[index].GetNumber());
            }
            return data;
        }

        // 수량 모델 → JSON 배열 (0개는 빼서 파일을 짧게)
        [[nodiscard]] JSONValue BuildQuantities(const Models::BoundGemData& data)
        {
            std::vector<JSONValue> entries;
            for (std::size_t gemIndex{}; gemIndex < Models::BoundGemData::GEM_COUNT; ++gemIndex)
            {
                for (std::size_t gradeIndex{}; gradeIndex < Models::BoundGemData::GRADE_COUNT; ++gradeIndex)
                {
                    const int quantity{ data.GetQuantity(gemIndex, gradeIndex) };
                    if (quantity <= 0) continue;

                    std::vector<std::string> keys{ std::string{ KEY_NAME }, std::string{ KEY_GRADE }, std::string{ KEY_COUNT } };
                    std::vector<JSONValue> values{ JSONValue::CreateString(std::string{ Models::BoundGemData::GEM_NAMES[gemIndex] }),
                        JSONValue::CreateString(std::string{ Models::BoundGemData::GRADE_NAMES[gradeIndex] }),
                        JSONValue::CreateNumber(static_cast<double>(quantity)) };
                    const Models::GemScanSlot& slot{ data.GetScanSlot(gemIndex, gradeIndex) };   // 화면 인식 내역 (있을 때만)
                    if (slot.m_bound) { keys.emplace_back(KEY_SCAN_BOUND); values.push_back(JSONValue::CreateNumber(static_cast<double>(*slot.m_bound))); }
                    if (slot.m_tradable) { keys.emplace_back(KEY_SCAN_TRADABLE); values.push_back(JSONValue::CreateNumber(static_cast<double>(*slot.m_tradable))); }
                    if (!slot.m_processed.empty())
                    {
                        std::vector<JSONValue> processed;
                        for (const std::string& key : slot.m_processed) processed.push_back(JSONValue::CreateString(key));
                        keys.emplace_back(KEY_SCAN_PROCESSED);
                        values.push_back(JSONValue::CreateArray(std::move(processed)));
                    }
                    entries.push_back(JSONValue::CreateObject(std::move(keys), std::move(values)));
                }
            }
            return JSONValue::CreateArray(std::move(entries));
        }

        // 모델 → JSON 텍스트 (직업이 없으면 키 생략, 공용 수량은 남아 있을 때만)
        [[nodiscard]] std::string BuildJSONText(const Models::BoundGemRosterData& data)
        {
            std::vector<JSONValue> characters;
            characters.reserve(data.GetCharacters().size());
            for (const Models::BoundGemCharacter& character : data.GetCharacters())
            {
                std::vector<std::string> keys{ std::string{ KEY_NAME } };
                std::vector<JSONValue> values{ JSONValue::CreateString(character.m_name) };
                if (character.m_className.has_value())
                {
                    keys.emplace_back(KEY_CLASS_NAME);
                    values.push_back(JSONValue::CreateString(*character.m_className));
                }
                keys.emplace_back(KEY_ITEM_LEVEL);
                values.push_back(JSONValue::CreateNumber(character.m_itemLevel));
                keys.emplace_back(KEY_QUANTITIES);
                values.push_back(BuildQuantities(character.m_quantities));
                characters.push_back(JSONValue::CreateObject(std::move(keys), std::move(values)));
            }

            std::vector<std::string> rootKeys{ std::string{ KEY_CHARACTERS } };
            std::vector<JSONValue> rootValues{ JSONValue::CreateArray(std::move(characters)) };
            if (data.HasUnassigned())
            {
                rootKeys.emplace_back(KEY_QUANTITIES);
                rootValues.push_back(BuildQuantities(data.GetUnassigned()));
            }

            // 날짜별 원정대 총 가치
            std::vector<std::string> historyKeys;
            std::vector<JSONValue> historyValues;
            for (const auto& [dateKey, value] : data.GetHistory())
            {
                historyKeys.push_back(dateKey);
                historyValues.push_back(JSONValue::CreateNumber(value));
            }
            rootKeys.emplace_back(KEY_HISTORY);
            rootValues.push_back(JSONValue::CreateObject(std::move(historyKeys), std::move(historyValues)));
            return WriteJSONText(JSONValue::CreateObject(std::move(rootKeys), std::move(rootValues)));
        }
    }

    JSONBoundGemRepository::JSONBoundGemRepository()
        : m_fileStore{ Core::APPConstants::BOUND_GEM_DATA_FILE_NAME }
    {
        // 변환이 끝까지 성공한 파일만 반영 (실패하면 백업으로 다시 시도, 둘 다 없으면 빈 데이터)
        std::optional<Models::BoundGemRosterData> loaded;
        JSONLoadResult result{ m_fileStore.Load([&loaded](const JSONValue& root) { loaded = ReadData(root); }) };
        m_loadWarning = std::move(result.m_warning);
        if (loaded.has_value()) m_data = std::move(*loaded);
    }

    Models::BoundGemRosterData& JSONBoundGemRepository::GetData()
    {
        return m_data;
    }

    const Models::BoundGemRosterData& JSONBoundGemRepository::GetData() const
    {
        return m_data;
    }

    const std::optional<std::string>& JSONBoundGemRepository::GetLoadWarning() const noexcept
    {
        return m_loadWarning;
    }

    void JSONBoundGemRepository::Save()
    {
        m_fileStore.Save(BuildJSONText(m_data));
    }
}
