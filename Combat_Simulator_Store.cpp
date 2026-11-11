module LostArkMultiTool.Services.CombatSimulatorStore;  // 전투력 시뮬레이터 저장소 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPConstants;  // 앱 상수 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모듈 가져오기
import LostArkMultiTool.Services.JSONFileStore;  // JSON 파일 저장소 모듈 가져오기
import LostArkMultiTool.Services.JSONValue;  // JSON 값 모듈 가져오기

namespace LostArkMultiTool::Services  // 서비스 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        // ===== JSON 키 =====
        constexpr std::string_view KEY_CHARACTER_NAME{ "CharacterName" };  // 캐릭터 이름 키
        constexpr std::string_view KEY_TARGETS{ "Targets" };  // 부위별 목표 배열 키
        constexpr std::string_view KEY_PART{ "Part" };  // 부위 이름 키
        constexpr std::string_view KEY_ENHANCEMENT{ "Enhancement" };  // 일반 재련 단계 키
        constexpr std::string_view KEY_ADVANCED{ "Advanced" };  // 상급 재련 단계 키
        constexpr std::string_view KEY_WEAPON_QUALITY{ "WeaponQuality" };  // 무기 품질 목표 키
        constexpr std::string_view KEY_GEM_TARGETS{ "GemTargets" };  // 보석 목표 레벨 배열 키

        // ===== 범위 (손으로 고친 파일에서 이상한 값이 와도 안전하게) =====
        constexpr int MIN_VALUE{ 0 };  // 값의 최소 허용 범위
        constexpr int MAX_ENHANCEMENT{ 25 };  // 일반 재련 최대 단계
        constexpr int MAX_ADVANCED_ENHANCEMENT{ 40 };  // 상급 재련 최대 단계
        constexpr int MAX_QUALITY{ 100 };  // 무기 품질 최대값
        constexpr int MAX_GEM_LEVEL{ 10 };  // 보석 최대 레벨

        // 숫자 칸 → 범위 안 정수 (숫자가 아니면 nullopt)
        [[nodiscard]] std::optional<int> ReadInt(const JSONValue& value, int maxValue)  // 숫자 칸 읽기 (범위 안 정수)
        {
            if (value.GetType() != JSONType::NUMBER_VALUE) return std::nullopt;  // 숫자 형식이 아니면 값 없음
            return std::clamp(static_cast<int>(value.GetNumber()), MIN_VALUE, maxValue);  // 0 ~ 최대값으로 제한해 반환
        }

        [[nodiscard]] JSONValue MakeNumber(int value)  // 정수를 JSON 숫자로 만들기
        {
            return JSONValue::CreateNumber(static_cast<double>(value));  // JSON 숫자 값 생성
        }

        // ===== 추가 시스템(Extra) JSON 키 =====
        constexpr std::string_view KEY_EXTRA{ "Extra" };  // 추가 시스템 루트 키
        constexpr std::string_view KEY_ACCESSORIES{ "Accessories" };  // 악세서리 배열 키
        constexpr std::string_view KEY_OPTIONS{ "Options" };  // 옵션 배열 키
        constexpr std::string_view KEY_MAIN_STAT{ "MainStat" };  // 주스탯 키
        constexpr std::string_view KEY_BRACELET_STATS{ "BraceletStats" };  // 팔찌 스탯 배열 키
        constexpr std::string_view KEY_BRACELET_OPTIONS{ "BraceletOptions" };  // 팔찌 옵션 배열 키
        constexpr std::string_view KEY_TYPE{ "Type" };  // 스탯 종류 키
        constexpr std::string_view KEY_VALUE{ "Value" };  // 스탯 값 키
        constexpr std::string_view KEY_STONE{ "Stone" };  // 스톤 배열 키
        constexpr std::string_view KEY_BOOKS{ "Books" };  // 각인서 배열 키
        constexpr std::string_view KEY_NAME{ "Name" };  // 이름 키
        constexpr std::string_view KEY_LEGENDARY{ "Legendary" };  // 전설 여부 키
        constexpr std::string_view KEY_LEVEL{ "Level" };  // 레벨 키
        constexpr std::string_view KEY_AVATARS{ "Avatars" };  // 아바타 배열 키
        constexpr std::string_view KEY_CORES{ "Cores" };  // 아크 코어 배열 키
        constexpr std::string_view KEY_GRADE{ "Grade" };  // 등급 키
        constexpr std::string_view KEY_POINTS{ "Points" };  // 포인트 키
        constexpr std::string_view KEY_ARK_GEM_LEVELS{ "ArkGemLevels" };  // 아크 젬 레벨 배열 키
        constexpr std::string_view KEY_PASSIVE{ "Passive" };  // 아크 패시브 객체 키
        constexpr std::string_view KEY_RANK{ "Rank" };  // 랭크 키
        constexpr std::string_view KEY_ENLIGHTENMENT{ "Enlightenment" };  // 깨달음 키
        constexpr std::string_view KEY_LEVELS{ "Levels" };  // 레벨 배열 키
        constexpr std::string_view KEY_VAMBRACE{ "Vambrace" };  // 완갑 레벨 키
        constexpr std::string_view KEY_VAMBRACE_GRADE{ "VambraceGrade" };  // 완갑 등급 키 (0 영웅 ~ 3 고대)
        constexpr int MAX_VAMBRACE_GRADE{ static_cast<int>(Models::VambraceGrade::VAMBRACE_ANCIENT) };  // 완갑 등급 최대값 (고대)
        constexpr int MAX_STORED_VALUE{ 100000 };                 // 손으로 고친 파일의 이상한 값을 막는 상한
        constexpr int MAX_AVATAR_GRADE{ static_cast<int>(Models::AvatarGrade::AVATAR_LEGENDARY) };  // 아바타 등급 최대값 (전설)
        constexpr int MAX_CORE_GRADE{ static_cast<int>(Models::ArkCoreGrade::CORE_ANCIENT) };  // 아크 코어 등급 최대값 (고대)

        [[nodiscard]] JSONValue MakeString(const std::string& value)  // 문자열을 JSON 문자열로 만들기
        {
            return JSONValue::CreateString(value);  // JSON 문자열 값 생성
        }

        [[nodiscard]] JSONValue MakeStrings(std::span<const std::string> values)  // 문자열 목록을 JSON 배열로 만들기
        {
            std::vector<JSONValue> items;  // 배열 원소 목록
            items.reserve(values.size());  // 원소 개수만큼 메모리 미리 확보
            for (const std::string& value : values) items.push_back(MakeString(value));  // 각 문자열을 JSON 문자열로 바꿔 추가
            return JSONValue::CreateArray(std::move(items));  // 배열 값 생성
        }

        [[nodiscard]] JSONValue MakeNumbers(std::span<const int> values)  // 정수 목록을 JSON 배열로 만들기
        {
            std::vector<JSONValue> items;  // 배열 원소 목록
            items.reserve(values.size());  // 원소 개수만큼 메모리 미리 확보
            for (const int value : values) items.push_back(MakeNumber(value));  // 각 정수를 JSON 숫자로 바꿔 추가
            return JSONValue::CreateArray(std::move(items));  // 배열 값 생성
        }

        // 객체 만들기 (키와 값을 같은 순서로)
        [[nodiscard]] JSONValue MakeObject(std::initializer_list<std::pair<std::string_view, JSONValue>> members)  // JSON 객체 만들기
        {
            std::vector<std::string> keys;  // 키 목록
            std::vector<JSONValue> values;  // 값 목록
            for (const auto& [key, value] : members)  // 멤버를 순서대로 순회
            {
                keys.emplace_back(key);  // 키 추가
                values.push_back(value);  // 값 추가
            }
            return JSONValue::CreateObject(std::move(keys), std::move(values));  // 객체 값 생성
        }

        // 읽기: 있는 값만 덮어씀 (타입이 다르면 그대로)
        void ReadString(const JSONValue& value, std::string& target)  // 문자열 읽기
        {
            if (value.GetType() == JSONType::STRING_VALUE) target = value.GetString();  // 문자열 형식일 때만 덮어씀
        }

        void ReadStored(const JSONValue& value, int& target)  // 저장된 정수 읽기
        {
            if (value.GetType() == JSONType::NUMBER_VALUE) target = std::clamp(static_cast<int>(value.GetNumber()), MIN_VALUE, MAX_STORED_VALUE);  // 숫자 형식일 때만 0 ~ 상한으로 제한해 덮어씀
        }

        // 배열의 각 칸을 앞에서부터 reader로 읽음 (저장된 칸이 모자라면 나머지는 그대로)
        template <typename Element, std::size_t Count, typename Reader>
        void ReadArray(const JSONValue& array, std::array<Element, Count>& target, Reader reader)  // 배열 읽기 보조 함수
        {
            const std::vector<JSONValue>& items{ array.GetItems() };  // JSON 배열의 원소 목록
            for (std::size_t index{}; index < Count && index < items.size(); ++index) reader(items[index], target[index]);  // 저장된 칸과 대상 칸 중 적은 쪽까지만 읽음
        }

        [[nodiscard]] JSONValue WriteExtra(const Models::ExtraState& extra)  // 추가 시스템 상태를 JSON으로 쓰기
        {
            std::vector<JSONValue> accessories;  // 악세서리 JSON 목록
            for (const Models::AccessoryState& accessory : extra.m_accessories)  // 악세서리 상태 순회
            {
                accessories.push_back(MakeObject({ { KEY_OPTIONS, MakeStrings(accessory.m_options) }, { KEY_MAIN_STAT, JSONValue::CreateNumber(accessory.m_mainStat) } }));  // 옵션 목록과 주스탯을 객체로 저장
            }
            std::vector<JSONValue> braceletStats;  // 팔찌 스탯 JSON 목록
            for (const Models::BraceletStat& stat : extra.m_braceletStats)  // 팔찌 스탯 순회
            {
                braceletStats.push_back(MakeObject({ { KEY_TYPE, MakeString(stat.m_type) }, { KEY_VALUE, MakeNumber(stat.m_value) } }));  // 종류와 값을 객체로 저장
            }
            std::vector<JSONValue> stone;  // 스톤 JSON 목록
            for (const Models::StoneLine& line : extra.m_stone)  // 스톤 줄 순회
            {
                stone.push_back(MakeObject({ { KEY_NAME, MakeString(line.m_name) }, { KEY_LEVEL, MakeNumber(line.m_level) } }));  // 이름과 레벨을 객체로 저장
            }
            std::vector<JSONValue> books;  // 각인서 JSON 목록
            for (const Models::BookState& book : extra.m_books)  // 각인서 순회
            {
                books.push_back(MakeObject({ { KEY_NAME, MakeString(book.m_name) }, { KEY_LEGENDARY, JSONValue::CreateBool(book.m_isLegendary) }, { KEY_LEVEL, MakeNumber(book.m_level) } }));  // 이름 · 전설 여부 · 레벨을 객체로 저장
            }
            std::array<int, Models::AVATAR_SLOT_COUNT> avatars{};  // 아바타 등급을 정수 배열로 변환
            for (std::size_t slot{}; slot < avatars.size(); ++slot) avatars[slot] = static_cast<int>(extra.m_avatars[slot]);  // 슬롯별 등급을 정수로 기록
            std::vector<JSONValue> cores;  // 아크 코어 JSON 목록
            for (const Models::ArkCoreState& core : extra.m_cores)  // 아크 코어 상태 순회
            {
                cores.push_back(MakeObject({ { KEY_GRADE, MakeNumber(static_cast<int>(core.m_grade)) }, { KEY_POINTS, MakeNumber(core.m_points) } }));  // 등급과 포인트를 객체로 저장
            }

            std::vector<std::string> keys;  // 최종 객체의 키 목록
            std::vector<JSONValue> values;  // 최종 객체의 값 목록
            const auto add{ [&](std::string_view key, JSONValue value)  // 키와 값을 함께 추가하는 람다
            {
                keys.emplace_back(key);  // 키 추가
                values.push_back(std::move(value));  // 값 추가
            } };
            add(KEY_ACCESSORIES, JSONValue::CreateArray(std::move(accessories)));  // 악세서리 저장
            add(KEY_BRACELET_STATS, JSONValue::CreateArray(std::move(braceletStats)));  // 팔찌 스탯 저장
            add(KEY_BRACELET_OPTIONS, MakeStrings(extra.m_braceletOptions));  // 팔찌 옵션 저장
            add(KEY_STONE, JSONValue::CreateArray(std::move(stone)));  // 스톤 저장
            add(KEY_BOOKS, JSONValue::CreateArray(std::move(books)));  // 각인서 저장
            add(KEY_AVATARS, MakeNumbers(avatars));  // 아바타 저장
            add(KEY_CORES, JSONValue::CreateArray(std::move(cores)));  // 아크 코어 저장
            add(KEY_ARK_GEM_LEVELS, MakeNumbers(extra.m_arkGemLevels));  // 아크 젬 레벨 저장
            add(KEY_PASSIVE, MakeObject({ { KEY_RANK, MakeNumber(extra.m_passive.m_rank) }, { KEY_ENLIGHTENMENT, MakeString(extra.m_passive.m_enlightenment) },  // 아크 패시브 저장 (랭크 · 깨달음 · 레벨)
                { KEY_LEVELS, MakeNumbers(extra.m_passive.m_levels) } }));
            if (extra.m_vambraceLevel.has_value()) add(KEY_VAMBRACE, MakeNumber(*extra.m_vambraceLevel));  // 완갑 레벨이 있을 때만 저장
            if (extra.m_vambraceGrade.has_value()) add(KEY_VAMBRACE_GRADE, MakeNumber(static_cast<int>(*extra.m_vambraceGrade)));  // 완갑 등급이 있을 때만 저장
            return JSONValue::CreateObject(std::move(keys), std::move(values));  // 전체 객체 생성 후 반환
        }

        // 저장된 목표를 현재 값(base) 위에 덮어 읽음 (빠진 칸은 현재 값 그대로, 범위는 CombatExtra::Clamp가 맞춤)
        [[nodiscard]] Models::ExtraState ReadExtra(const JSONValue& root, Models::ExtraState extra)  // JSON에서 추가 시스템 목표 읽기
        {
            ReadArray(root.FindMember(KEY_ACCESSORIES), extra.m_accessories, [](const JSONValue& item, Models::AccessoryState& accessory)  // 악세서리 배열 읽기
            {
                ReadArray(item.FindMember(KEY_OPTIONS), accessory.m_options, [](const JSONValue& option, std::string& text) { ReadString(option, text); });  // 옵션 배열 읽기
                if (item.FindMember(KEY_MAIN_STAT).GetType() == JSONType::NUMBER_VALUE) accessory.m_mainStat = item.FindMember(KEY_MAIN_STAT).GetNumber();  // 주스탯이 숫자일 때만 반영
            });
            ReadArray(root.FindMember(KEY_BRACELET_STATS), extra.m_braceletStats, [](const JSONValue& item, Models::BraceletStat& stat)  // 팔찌 스탯 배열 읽기
            {
                ReadString(item.FindMember(KEY_TYPE), stat.m_type);  // 종류 읽기
                ReadStored(item.FindMember(KEY_VALUE), stat.m_value);  // 값 읽기
            });
            ReadArray(root.FindMember(KEY_BRACELET_OPTIONS), extra.m_braceletOptions, [](const JSONValue& option, std::string& text) { ReadString(option, text); });  // 팔찌 옵션 배열 읽기
            ReadArray(root.FindMember(KEY_STONE), extra.m_stone, [](const JSONValue& item, Models::StoneLine& line)  // 스톤 배열 읽기
            {
                ReadString(item.FindMember(KEY_NAME), line.m_name);  // 이름 읽기
                ReadStored(item.FindMember(KEY_LEVEL), line.m_level);  // 레벨 읽기
            });
            ReadArray(root.FindMember(KEY_BOOKS), extra.m_books, [](const JSONValue& item, Models::BookState& book)  // 각인서 배열 읽기
            {
                ReadString(item.FindMember(KEY_NAME), book.m_name);  // 이름 읽기
                if (item.FindMember(KEY_LEGENDARY).GetType() == JSONType::BOOL_VALUE) book.m_isLegendary = item.FindMember(KEY_LEGENDARY).GetBool();  // 전설 여부가 불리언일 때만 반영
                ReadStored(item.FindMember(KEY_LEVEL), book.m_level);  // 레벨 읽기
            });
            ReadArray(root.FindMember(KEY_AVATARS), extra.m_avatars, [](const JSONValue& item, Models::AvatarGrade& grade)  // 아바타 배열 읽기
            {
                int value{ static_cast<int>(grade) };  // 현재 등급을 정수로 시작값 삼음
                ReadStored(item, value);  // 저장된 값으로 덮어씀
                grade = static_cast<Models::AvatarGrade>(std::min(value, MAX_AVATAR_GRADE));  // 최대 등급을 넘지 않게 열거형으로 되돌림
            });
            ReadArray(root.FindMember(KEY_CORES), extra.m_cores, [](const JSONValue& item, Models::ArkCoreState& core)  // 아크 코어 배열 읽기
            {
                int grade{ static_cast<int>(core.m_grade) };  // 현재 등급을 정수로 시작값 삼음
                ReadStored(item.FindMember(KEY_GRADE), grade);  // 저장된 등급 읽기
                core.m_grade = static_cast<Models::ArkCoreGrade>(std::min(grade, MAX_CORE_GRADE));  // 최대 등급을 넘지 않게 열거형으로 되돌림
                ReadStored(item.FindMember(KEY_POINTS), core.m_points);  // 포인트 읽기
            });
            ReadArray(root.FindMember(KEY_ARK_GEM_LEVELS), extra.m_arkGemLevels, [](const JSONValue& item, int& level) { ReadStored(item, level); });  // 아크 젬 레벨 배열 읽기

            const JSONValue& passive{ root.FindMember(KEY_PASSIVE) };  // 아크 패시브 객체
            ReadStored(passive.FindMember(KEY_RANK), extra.m_passive.m_rank);  // 랭크 읽기
            ReadString(passive.FindMember(KEY_ENLIGHTENMENT), extra.m_passive.m_enlightenment);  // 깨달음 읽기
            ReadArray(passive.FindMember(KEY_LEVELS), extra.m_passive.m_levels, [](const JSONValue& item, int& level) { ReadStored(item, level); });  // 레벨 배열 읽기

            if (root.FindMember(KEY_VAMBRACE).GetType() == JSONType::NUMBER_VALUE)  // 완갑 레벨이 숫자일 때만 반영
            {
                int level{};  // 완갑 레벨 임시 변수
                ReadStored(root.FindMember(KEY_VAMBRACE), level);  // 값 읽기
                extra.m_vambraceLevel = level;  // 완갑 레벨 반영
            }
            if (root.FindMember(KEY_VAMBRACE_GRADE).GetType() == JSONType::NUMBER_VALUE)  // 완갑 등급이 숫자일 때만 반영
            {
                int grade{};  // 완갑 등급 임시 변수
                ReadStored(root.FindMember(KEY_VAMBRACE_GRADE), grade);  // 값 읽기
                extra.m_vambraceGrade = static_cast<Models::VambraceGrade>(std::min(grade, MAX_VAMBRACE_GRADE));  // 최대 등급을 넘지 않게 열거형으로 반영
            }
            return Models::CombatExtra::Clamp(extra);  // 입력 범위에 맞춰 보정하고 반환
        }
    }

    CombatSimulatorStore::CombatSimulatorStore()  // 생성자: 저장 파일 이름으로 파일 저장소 초기화
        : m_fileStore{ Core::APPConstants::COMBAT_SIMULATOR_DATA_FILE_NAME }  // 사용자 데이터 폴더의 전투력 시뮬레이터 파일 이름
    {
        Load();  // 생성과 동시에 불러오기
    }

    void CombatSimulatorStore::Load()  // 저장 파일에서 설정 불러오기
    {
        // 변환이 끝까지 성공한 파일만 반영 (실패하면 백업으로 다시 시도, 둘 다 없으면 기본값)
        std::optional<CombatSimulatorSettings> loaded;  // 변환에 성공한 설정을 담을 임시 값
        JSONLoadResult result{ m_fileStore.Load([&loaded](const JSONValue& root)  // 파일 저장소가 읽어 준 루트를 변환
        {
            if (root.GetType() != JSONType::OBJECT_VALUE) throw std::runtime_error{ "전투력 시뮬레이터 저장 파일 형식이 올바르지 않습니다." };  // 루트가 객체가 아니면 예외 (백업으로 재시도)

            CombatSimulatorSettings settings;  // 새 설정 객체
            settings.m_characterName = root.FindMember(KEY_CHARACTER_NAME).GetString();  // 캐릭터 이름 읽기
            for (const JSONValue& entry : root.FindMember(KEY_TARGETS).GetItems())  // 부위별 목표 순회
            {
                const std::optional<Models::GearPart> part{ Models::FindGearPart(entry.FindMember(KEY_PART).GetString()) };  // 부위 이름으로 부위 찾기
                const std::optional<int> enhancement{ ReadInt(entry.FindMember(KEY_ENHANCEMENT), MAX_ENHANCEMENT) };  // 일반 재련 단계 읽기
                if (!part.has_value() || !enhancement.has_value()) continue;   // 모르는 부위 · 값은 건너뜀

                settings.m_gearTargets[Models::ToGearIndex(*part)] = CombatGearTarget{ *enhancement,  // 부위 번호 칸에 목표 저장
                    ReadInt(entry.FindMember(KEY_ADVANCED), MAX_ADVANCED_ENHANCEMENT).value_or(MIN_VALUE) };  // 상급 단계는 없으면 0
            }
            settings.m_weaponQualityTarget = ReadInt(root.FindMember(KEY_WEAPON_QUALITY), MAX_QUALITY);  // 무기 품질 목표 읽기
            for (const JSONValue& level : root.FindMember(KEY_GEM_TARGETS).GetItems())  // 보석 목표 레벨 순회
            {
                settings.m_gemTargets.push_back(ReadInt(level, MAX_GEM_LEVEL).value_or(MIN_VALUE));   // 0 ~ 10 (6 미만 = 현재 그대로 둔 보석, 매니저가 거름)
            }
            if (const JSONValue& extra{ root.FindMember(KEY_EXTRA) }; extra.GetType() == JSONType::OBJECT_VALUE) settings.m_extraTarget = ReadExtra(extra, Models::ExtraState{});  // 추가 시스템 목표가 객체이면 읽기
            loaded = std::move(settings);  // 변환 성공 시 임시 값에 보관
        }) };

        m_loadWarning = std::move(result.m_warning);  // 경고 메시지 보관
        if (loaded.has_value()) m_settings = std::move(*loaded);  // 성공했으면 설정 반영
    }

    const CombatSimulatorSettings& CombatSimulatorStore::GetSettings() const noexcept  // 설정 읽기 (const)
    {
        return m_settings;  // 현재 설정 반환
    }

    CombatSimulatorSettings& CombatSimulatorStore::GetSettings() noexcept  // 설정 읽기 (수정용)
    {
        return m_settings;  // 현재 설정 반환
    }

    const std::optional<std::string>& CombatSimulatorStore::GetLoadWarning() const noexcept  // 불러오기 경고 읽기
    {
        return m_loadWarning;  // 경고 반환
    }

    void CombatSimulatorStore::Save()  // 설정을 파일로 저장
    {
        std::vector<JSONValue> targets;  // 부위별 목표 JSON 목록
        targets.reserve(Models::GEAR_PART_COUNT);  // 부위 개수만큼 메모리 미리 확보
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 모든 부위를 순회
        {
            const std::optional<CombatGearTarget>& target{ m_settings.m_gearTargets[index] };  // 이 부위의 목표
            if (!target.has_value()) continue;  // 목표가 없는 부위는 건너뜀

            targets.push_back(JSONValue::CreateObject(  // 부위 이름 · 일반 단계 · 상급 단계를 객체로 저장
                { std::string{ KEY_PART }, std::string{ KEY_ENHANCEMENT }, std::string{ KEY_ADVANCED } },
                { JSONValue::CreateString(std::string{ Models::GetGearPartType(Models::ToGearPart(index)) }),
                  MakeNumber(target->m_enhancement), MakeNumber(target->m_advancedEnhancement) }));
        }

        std::vector<std::string> keys{ std::string{ KEY_CHARACTER_NAME }, std::string{ KEY_TARGETS } };  // 최종 객체의 키 목록 (캐릭터 이름 · 목표)
        std::vector<JSONValue> values{ JSONValue::CreateString(m_settings.m_characterName), JSONValue::CreateArray(std::move(targets)) };  // 최종 객체의 값 목록
        if (m_settings.m_weaponQualityTarget.has_value())  // 무기 품질 목표가 있으면 저장
        {
            keys.emplace_back(KEY_WEAPON_QUALITY);  // 키 추가
            values.push_back(MakeNumber(*m_settings.m_weaponQualityTarget));  // 값 추가
        }
        if (!m_settings.m_gemTargets.empty())  // 보석 목표가 있으면 저장
        {
            std::vector<JSONValue> gemTargets;  // 보석 목표 JSON 목록
            gemTargets.reserve(m_settings.m_gemTargets.size());  // 개수만큼 메모리 미리 확보
            for (const int level : m_settings.m_gemTargets) gemTargets.push_back(MakeNumber(level));  // 레벨을 JSON 숫자로 바꿔 추가
            keys.emplace_back(KEY_GEM_TARGETS);  // 키 추가
            values.push_back(JSONValue::CreateArray(std::move(gemTargets)));  // 값 추가
        }

        if (m_settings.m_extraTarget.has_value())  // 추가 시스템 목표가 있으면 저장
        {
            keys.emplace_back(KEY_EXTRA);  // 키 추가
            values.push_back(WriteExtra(*m_settings.m_extraTarget));  // 값 추가
        }

        m_fileStore.Save(WriteJSONText(JSONValue::CreateObject(std::move(keys), std::move(values))));  // JSON 텍스트로 만들어 파일 저장소에 저장
    }
}
