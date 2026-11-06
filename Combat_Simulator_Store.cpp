module LostArkMultiTool.Services.CombatSimulatorStore;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Services.JSONFileStore;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== JSON 키 =====
        constexpr std::string_view KEY_CHARACTER_NAME{ "CharacterName" };
        constexpr std::string_view KEY_TARGETS{ "Targets" };
        constexpr std::string_view KEY_PART{ "Part" };
        constexpr std::string_view KEY_ENHANCEMENT{ "Enhancement" };
        constexpr std::string_view KEY_ADVANCED{ "Advanced" };
        constexpr std::string_view KEY_WEAPON_QUALITY{ "WeaponQuality" };
        constexpr std::string_view KEY_GEM_TARGETS{ "GemTargets" };

        // ===== 범위 (손으로 고친 파일에서 이상한 값이 와도 안전하게) =====
        constexpr int MIN_VALUE{ 0 };
        constexpr int MAX_ENHANCEMENT{ 25 };
        constexpr int MAX_ADVANCED_ENHANCEMENT{ 40 };
        constexpr int MAX_QUALITY{ 100 };
        constexpr int MAX_GEM_LEVEL{ 10 };

        // 숫자 칸 → 범위 안 정수 (숫자가 아니면 nullopt)
        [[nodiscard]] std::optional<int> ReadInt(const JSONValue& value, int maxValue)
        {
            if (value.GetType() != JSONType::NUMBER_VALUE) return std::nullopt;
            return std::clamp(static_cast<int>(value.GetNumber()), MIN_VALUE, maxValue);
        }

        [[nodiscard]] JSONValue MakeNumber(int value)
        {
            return JSONValue::CreateNumber(static_cast<double>(value));
        }

        // ===== 추가 시스템(Extra) JSON 키 =====
        constexpr std::string_view KEY_EXTRA{ "Extra" };
        constexpr std::string_view KEY_ACCESSORIES{ "Accessories" };
        constexpr std::string_view KEY_OPTIONS{ "Options" };
        constexpr std::string_view KEY_MAIN_STAT{ "MainStat" };
        constexpr std::string_view KEY_BRACELET_STATS{ "BraceletStats" };
        constexpr std::string_view KEY_BRACELET_OPTIONS{ "BraceletOptions" };
        constexpr std::string_view KEY_TYPE{ "Type" };
        constexpr std::string_view KEY_VALUE{ "Value" };
        constexpr std::string_view KEY_STONE{ "Stone" };
        constexpr std::string_view KEY_BOOKS{ "Books" };
        constexpr std::string_view KEY_NAME{ "Name" };
        constexpr std::string_view KEY_LEGENDARY{ "Legendary" };
        constexpr std::string_view KEY_LEVEL{ "Level" };
        constexpr std::string_view KEY_AVATARS{ "Avatars" };
        constexpr std::string_view KEY_CORES{ "Cores" };
        constexpr std::string_view KEY_GRADE{ "Grade" };
        constexpr std::string_view KEY_POINTS{ "Points" };
        constexpr std::string_view KEY_ARK_GEM_LEVELS{ "ArkGemLevels" };
        constexpr std::string_view KEY_PASSIVE{ "Passive" };
        constexpr std::string_view KEY_RANK{ "Rank" };
        constexpr std::string_view KEY_ENLIGHTENMENT{ "Enlightenment" };
        constexpr std::string_view KEY_LEVELS{ "Levels" };
        constexpr std::string_view KEY_VAMBRACE{ "Vambrace" };
        constexpr int MAX_STORED_VALUE{ 100000 };                 // 손으로 고친 파일의 이상한 값을 막는 상한
        constexpr int MAX_AVATAR_GRADE{ static_cast<int>(Models::AvatarGrade::AVATAR_LEGENDARY) };
        constexpr int MAX_CORE_GRADE{ static_cast<int>(Models::ArkCoreGrade::CORE_ANCIENT) };

        [[nodiscard]] JSONValue MakeString(const std::string& value)
        {
            return JSONValue::CreateString(value);
        }

        [[nodiscard]] JSONValue MakeStrings(std::span<const std::string> values)
        {
            std::vector<JSONValue> items;
            items.reserve(values.size());
            for (const std::string& value : values) items.push_back(MakeString(value));
            return JSONValue::CreateArray(std::move(items));
        }

        [[nodiscard]] JSONValue MakeNumbers(std::span<const int> values)
        {
            std::vector<JSONValue> items;
            items.reserve(values.size());
            for (const int value : values) items.push_back(MakeNumber(value));
            return JSONValue::CreateArray(std::move(items));
        }

        // 객체 만들기 (키와 값을 같은 순서로)
        [[nodiscard]] JSONValue MakeObject(std::initializer_list<std::pair<std::string_view, JSONValue>> members)
        {
            std::vector<std::string> keys;
            std::vector<JSONValue> values;
            for (const auto& [key, value] : members)
            {
                keys.emplace_back(key);
                values.push_back(value);
            }
            return JSONValue::CreateObject(std::move(keys), std::move(values));
        }

        // 읽기: 있는 값만 덮어씀 (타입이 다르면 그대로)
        void ReadString(const JSONValue& value, std::string& target)
        {
            if (value.GetType() == JSONType::STRING_VALUE) target = value.GetString();
        }

        void ReadStored(const JSONValue& value, int& target)
        {
            if (value.GetType() == JSONType::NUMBER_VALUE) target = std::clamp(static_cast<int>(value.GetNumber()), MIN_VALUE, MAX_STORED_VALUE);
        }

        // 배열의 각 칸을 앞에서부터 reader로 읽음 (저장된 칸이 모자라면 나머지는 그대로)
        template <typename Element, std::size_t Count, typename Reader>
        void ReadArray(const JSONValue& array, std::array<Element, Count>& target, Reader reader)
        {
            const std::vector<JSONValue>& items{ array.GetItems() };
            for (std::size_t index{}; index < Count && index < items.size(); ++index) reader(items[index], target[index]);
        }

        [[nodiscard]] JSONValue WriteExtra(const Models::ExtraState& extra)
        {
            std::vector<JSONValue> accessories;
            for (const Models::AccessoryState& accessory : extra.m_accessories)
            {
                accessories.push_back(MakeObject({ { KEY_OPTIONS, MakeStrings(accessory.m_options) }, { KEY_MAIN_STAT, JSONValue::CreateNumber(accessory.m_mainStat) } }));
            }
            std::vector<JSONValue> braceletStats;
            for (const Models::BraceletStat& stat : extra.m_braceletStats)
            {
                braceletStats.push_back(MakeObject({ { KEY_TYPE, MakeString(stat.m_type) }, { KEY_VALUE, MakeNumber(stat.m_value) } }));
            }
            std::vector<JSONValue> stone;
            for (const Models::StoneLine& line : extra.m_stone)
            {
                stone.push_back(MakeObject({ { KEY_NAME, MakeString(line.m_name) }, { KEY_LEVEL, MakeNumber(line.m_level) } }));
            }
            std::vector<JSONValue> books;
            for (const Models::BookState& book : extra.m_books)
            {
                books.push_back(MakeObject({ { KEY_NAME, MakeString(book.m_name) }, { KEY_LEGENDARY, JSONValue::CreateBool(book.m_isLegendary) }, { KEY_LEVEL, MakeNumber(book.m_level) } }));
            }
            std::array<int, Models::AVATAR_SLOT_COUNT> avatars{};
            for (std::size_t slot{}; slot < avatars.size(); ++slot) avatars[slot] = static_cast<int>(extra.m_avatars[slot]);
            std::vector<JSONValue> cores;
            for (const Models::ArkCoreState& core : extra.m_cores)
            {
                cores.push_back(MakeObject({ { KEY_GRADE, MakeNumber(static_cast<int>(core.m_grade)) }, { KEY_POINTS, MakeNumber(core.m_points) } }));
            }

            std::vector<std::string> keys;
            std::vector<JSONValue> values;
            const auto add{ [&](std::string_view key, JSONValue value)
            {
                keys.emplace_back(key);
                values.push_back(std::move(value));
            } };
            add(KEY_ACCESSORIES, JSONValue::CreateArray(std::move(accessories)));
            add(KEY_BRACELET_STATS, JSONValue::CreateArray(std::move(braceletStats)));
            add(KEY_BRACELET_OPTIONS, MakeStrings(extra.m_braceletOptions));
            add(KEY_STONE, JSONValue::CreateArray(std::move(stone)));
            add(KEY_BOOKS, JSONValue::CreateArray(std::move(books)));
            add(KEY_AVATARS, MakeNumbers(avatars));
            add(KEY_CORES, JSONValue::CreateArray(std::move(cores)));
            add(KEY_ARK_GEM_LEVELS, MakeNumbers(extra.m_arkGemLevels));
            add(KEY_PASSIVE, MakeObject({ { KEY_RANK, MakeNumber(extra.m_passive.m_rank) }, { KEY_ENLIGHTENMENT, MakeString(extra.m_passive.m_enlightenment) },
                { KEY_LEVELS, MakeNumbers(extra.m_passive.m_levels) } }));
            if (extra.m_vambraceLevel.has_value()) add(KEY_VAMBRACE, MakeNumber(*extra.m_vambraceLevel));
            return JSONValue::CreateObject(std::move(keys), std::move(values));
        }

        // 저장된 목표를 현재 값(base) 위에 덮어 읽음 (빠진 칸은 현재 값 그대로, 범위는 CombatExtra::Clamp가 맞춤)
        [[nodiscard]] Models::ExtraState ReadExtra(const JSONValue& root, Models::ExtraState extra)
        {
            ReadArray(root.FindMember(KEY_ACCESSORIES), extra.m_accessories, [](const JSONValue& item, Models::AccessoryState& accessory)
            {
                ReadArray(item.FindMember(KEY_OPTIONS), accessory.m_options, [](const JSONValue& option, std::string& text) { ReadString(option, text); });
                if (item.FindMember(KEY_MAIN_STAT).GetType() == JSONType::NUMBER_VALUE) accessory.m_mainStat = item.FindMember(KEY_MAIN_STAT).GetNumber();
            });
            ReadArray(root.FindMember(KEY_BRACELET_STATS), extra.m_braceletStats, [](const JSONValue& item, Models::BraceletStat& stat)
            {
                ReadString(item.FindMember(KEY_TYPE), stat.m_type);
                ReadStored(item.FindMember(KEY_VALUE), stat.m_value);
            });
            ReadArray(root.FindMember(KEY_BRACELET_OPTIONS), extra.m_braceletOptions, [](const JSONValue& option, std::string& text) { ReadString(option, text); });
            ReadArray(root.FindMember(KEY_STONE), extra.m_stone, [](const JSONValue& item, Models::StoneLine& line)
            {
                ReadString(item.FindMember(KEY_NAME), line.m_name);
                ReadStored(item.FindMember(KEY_LEVEL), line.m_level);
            });
            ReadArray(root.FindMember(KEY_BOOKS), extra.m_books, [](const JSONValue& item, Models::BookState& book)
            {
                ReadString(item.FindMember(KEY_NAME), book.m_name);
                if (item.FindMember(KEY_LEGENDARY).GetType() == JSONType::BOOL_VALUE) book.m_isLegendary = item.FindMember(KEY_LEGENDARY).GetBool();
                ReadStored(item.FindMember(KEY_LEVEL), book.m_level);
            });
            ReadArray(root.FindMember(KEY_AVATARS), extra.m_avatars, [](const JSONValue& item, Models::AvatarGrade& grade)
            {
                int value{ static_cast<int>(grade) };
                ReadStored(item, value);
                grade = static_cast<Models::AvatarGrade>(std::min(value, MAX_AVATAR_GRADE));
            });
            ReadArray(root.FindMember(KEY_CORES), extra.m_cores, [](const JSONValue& item, Models::ArkCoreState& core)
            {
                int grade{ static_cast<int>(core.m_grade) };
                ReadStored(item.FindMember(KEY_GRADE), grade);
                core.m_grade = static_cast<Models::ArkCoreGrade>(std::min(grade, MAX_CORE_GRADE));
                ReadStored(item.FindMember(KEY_POINTS), core.m_points);
            });
            ReadArray(root.FindMember(KEY_ARK_GEM_LEVELS), extra.m_arkGemLevels, [](const JSONValue& item, int& level) { ReadStored(item, level); });

            const JSONValue& passive{ root.FindMember(KEY_PASSIVE) };
            ReadStored(passive.FindMember(KEY_RANK), extra.m_passive.m_rank);
            ReadString(passive.FindMember(KEY_ENLIGHTENMENT), extra.m_passive.m_enlightenment);
            ReadArray(passive.FindMember(KEY_LEVELS), extra.m_passive.m_levels, [](const JSONValue& item, int& level) { ReadStored(item, level); });

            if (root.FindMember(KEY_VAMBRACE).GetType() == JSONType::NUMBER_VALUE)
            {
                int level{};
                ReadStored(root.FindMember(KEY_VAMBRACE), level);
                extra.m_vambraceLevel = level;
            }
            return Models::CombatExtra::Clamp(extra);
        }
    }

    CombatSimulatorStore::CombatSimulatorStore()
        : m_fileStore{ Core::APPConstants::COMBAT_SIMULATOR_DATA_FILE_NAME }
    {
        Load();
    }

    void CombatSimulatorStore::Load()
    {
        // 변환이 끝까지 성공한 파일만 반영 (실패하면 백업으로 다시 시도, 둘 다 없으면 기본값)
        std::optional<CombatSimulatorSettings> loaded;
        JSONLoadResult result{ m_fileStore.Load([&loaded](const JSONValue& root)
        {
            if (root.GetType() != JSONType::OBJECT_VALUE) throw std::runtime_error{ "전투력 시뮬레이터 저장 파일 형식이 올바르지 않습니다." };

            CombatSimulatorSettings settings;
            settings.m_characterName = root.FindMember(KEY_CHARACTER_NAME).GetString();
            for (const JSONValue& entry : root.FindMember(KEY_TARGETS).GetItems())
            {
                const std::optional<Models::GearPart> part{ Models::FindGearPart(entry.FindMember(KEY_PART).GetString()) };
                const std::optional<int> enhancement{ ReadInt(entry.FindMember(KEY_ENHANCEMENT), MAX_ENHANCEMENT) };
                if (!part.has_value() || !enhancement.has_value()) continue;   // 모르는 부위 · 값은 건너뜀

                settings.m_gearTargets[Models::ToGearIndex(*part)] = CombatGearTarget{ *enhancement,
                    ReadInt(entry.FindMember(KEY_ADVANCED), MAX_ADVANCED_ENHANCEMENT).value_or(MIN_VALUE) };
            }
            settings.m_weaponQualityTarget = ReadInt(root.FindMember(KEY_WEAPON_QUALITY), MAX_QUALITY);
            for (const JSONValue& level : root.FindMember(KEY_GEM_TARGETS).GetItems())
            {
                settings.m_gemTargets.push_back(ReadInt(level, MAX_GEM_LEVEL).value_or(MIN_VALUE));   // 0 ~ 10 (6 미만 = 현재 그대로 둔 보석, 매니저가 거름)
            }
            if (const JSONValue& extra{ root.FindMember(KEY_EXTRA) }; extra.GetType() == JSONType::OBJECT_VALUE) settings.m_extraTarget = ReadExtra(extra, Models::ExtraState{});
            loaded = std::move(settings);
        }) };

        m_loadWarning = std::move(result.m_warning);
        if (loaded.has_value()) m_settings = std::move(*loaded);
    }

    const CombatSimulatorSettings& CombatSimulatorStore::GetSettings() const noexcept
    {
        return m_settings;
    }

    CombatSimulatorSettings& CombatSimulatorStore::GetSettings() noexcept
    {
        return m_settings;
    }

    const std::optional<std::string>& CombatSimulatorStore::GetLoadWarning() const noexcept
    {
        return m_loadWarning;
    }

    void CombatSimulatorStore::Save()
    {
        std::vector<JSONValue> targets;
        targets.reserve(Models::GEAR_PART_COUNT);
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)
        {
            const std::optional<CombatGearTarget>& target{ m_settings.m_gearTargets[index] };
            if (!target.has_value()) continue;

            targets.push_back(JSONValue::CreateObject(
                { std::string{ KEY_PART }, std::string{ KEY_ENHANCEMENT }, std::string{ KEY_ADVANCED } },
                { JSONValue::CreateString(std::string{ Models::GetGearPartType(Models::ToGearPart(index)) }),
                  MakeNumber(target->m_enhancement), MakeNumber(target->m_advancedEnhancement) }));
        }

        std::vector<std::string> keys{ std::string{ KEY_CHARACTER_NAME }, std::string{ KEY_TARGETS } };
        std::vector<JSONValue> values{ JSONValue::CreateString(m_settings.m_characterName), JSONValue::CreateArray(std::move(targets)) };
        if (m_settings.m_weaponQualityTarget.has_value())
        {
            keys.emplace_back(KEY_WEAPON_QUALITY);
            values.push_back(MakeNumber(*m_settings.m_weaponQualityTarget));
        }
        if (!m_settings.m_gemTargets.empty())
        {
            std::vector<JSONValue> gemTargets;
            gemTargets.reserve(m_settings.m_gemTargets.size());
            for (const int level : m_settings.m_gemTargets) gemTargets.push_back(MakeNumber(level));
            keys.emplace_back(KEY_GEM_TARGETS);
            values.push_back(JSONValue::CreateArray(std::move(gemTargets)));
        }

        if (m_settings.m_extraTarget.has_value())
        {
            keys.emplace_back(KEY_EXTRA);
            values.push_back(WriteExtra(*m_settings.m_extraTarget));
        }

        m_fileStore.Save(WriteJSONText(JSONValue::CreateObject(std::move(keys), std::move(values))));
    }
}
