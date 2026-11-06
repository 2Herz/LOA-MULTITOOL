module LostArkMultiTool.Services.ArmoryParser;

import std;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Services
{
    namespace
    {
        // ===== 응답 JSON 키 =====
        constexpr std::string_view KEY_PROFILE{ "ArmoryProfile" };
        constexpr std::string_view KEY_EQUIPMENT{ "ArmoryEquipment" };
        constexpr std::string_view KEY_GEM{ "ArmoryGem" };
        constexpr std::string_view KEY_GEMS{ "Gems" };
        constexpr std::string_view KEY_EFFECTS{ "Effects" };   // ArmoryGem.Effects (보석 효과 합계)
        constexpr std::string_view GEM_BASE_ATTACK_MARKER{ "기본 공격력 총합" };   // "기본 공격력 총합 : 6.60%"
        constexpr double PERCENT_DIVISOR{ 100.0 };
        constexpr std::string_view KEY_LEVEL{ "Level" };
        constexpr std::string_view KEY_CHARACTER_NAME{ "CharacterName" };
        constexpr std::string_view KEY_CHARACTER_CLASS_NAME{ "CharacterClassName" };
        constexpr std::string_view KEY_ITEM_AVG_LEVEL{ "ItemAvgLevel" };
        constexpr std::string_view KEY_COMBAT_POWER{ "CombatPower" };
        constexpr std::string_view KEY_TYPE{ "Type" };
        constexpr std::string_view KEY_NAME{ "Name" };
        constexpr std::string_view KEY_GRADE{ "Grade" };
        constexpr std::string_view KEY_TOOLTIP{ "Tooltip" };
        constexpr std::string_view KEY_QUALITY_VALUE{ "qualityValue" };

        // ===== 툴팁 글자 =====
        constexpr std::string_view ADVANCED_MARKER{ "[상급 재련]" };
        constexpr std::string_view BASIC_EFFECT_MARKER{ "기본 효과" };
        constexpr std::string_view EFFECT_WORD{ "효과" };                        // 다음 묶음 제목 ("추가 효과" 등) = 기본 효과 구간 끝
        constexpr std::array<std::string_view, 3> MAIN_STAT_MARKERS{ "힘 +", "민첩 +", "지능 +" };
        constexpr std::string_view WEAPON_ATTACK_MARKER{ "무기 공격력 +" };
        constexpr std::string_view BRACELET_TYPE{ "팔찌" };
        constexpr std::array<std::string_view, 4> GEM_KINDS{ "겁화", "작열", "멸화", "홍염" };   // 보석 이름에서 찾는 종류 글자                      // 주스탯이 기본 효과 밖에 있는 장비 Type
        constexpr std::string_view QUALITY_TEXT_PATTERN{ R"("qualityValue"\s*:\s*(-?\d+))" };   // 툴팁을 JSON으로 읽지 못했을 때
        constexpr char TAG_OPEN{ '<' };
        constexpr char TAG_CLOSE{ '>' };
        constexpr char TEXT_SEPARATOR{ '\n' };
        constexpr char PLUS_SIGN{ '+' };
        constexpr char DIGIT_SEPARATOR{ ',' };
        constexpr std::string_view SKIPPED_BEFORE_NUMBER{ " \t+" };
        constexpr int NO_QUALITY{ -1 };                                          // API: 품질이 없는 장비
        constexpr int DECIMAL_BASE{ 10 };
        constexpr std::size_t QUALITY_GROUP{ 1 };                                // 정규식의 품질 숫자 묶음

        constexpr std::string_view NOT_FOUND_MESSAGE{ "캐릭터를 찾을 수 없어요. 이름을 다시 확인해 주세요." };

        // "1,764.17" · "1764.17" → 숫자 (쉼표는 무시, 읽을 수 없으면 nullopt)
        [[nodiscard]] std::optional<double> ParseDecimal(std::string_view text)
        {
            std::string cleaned;
            cleaned.reserve(text.size());
            for (const char character : text)
            {
                if (character != DIGIT_SEPARATOR && character != ' ') cleaned.push_back(character);
            }
            if (cleaned.empty()) return std::nullopt;

            double value{};
            const auto [end, error]{ std::from_chars(cleaned.data(), cleaned.data() + cleaned.size(), value) };
            if (error != std::errc{} || end != cleaned.data() + cleaned.size()) return std::nullopt;
            return value;
        }

        // 문자열 또는 숫자로 오는 값 → 숫자 (없거나 읽을 수 없으면 nullopt)
        [[nodiscard]] std::optional<double> ReadFlexibleNumber(const JSONValue& value)
        {
            switch (value.GetType())
            {
                case JSONType::NUMBER_VALUE:
                    return value.GetNumber();
                case JSONType::STRING_VALUE:
                    return ParseDecimal(value.GetString());
                default:
                    return std::nullopt;
            }
        }

        // <...> 태그를 지운 글자 (태그 자리는 공백 하나 → "+123<BR>힘" 같은 글자가 붙지 않게)
        [[nodiscard]] std::string StripTags(std::string_view text)
        {
            std::string stripped;
            stripped.reserve(text.size());
            bool isInTag{};
            for (const char character : text)
            {
                if (character == TAG_OPEN)
                {
                    isInTag = true;
                    continue;
                }
                if (character == TAG_CLOSE && isInTag)
                {
                    isInTag = false;
                    stripped.push_back(' ');
                    continue;
                }
                if (!isInTag) stripped.push_back(character);
            }
            return stripped;
        }

        // 툴팁 JSON 트리의 글자를 순서대로 이어 붙임 (키 순서 = 툴팁 표시 순서) + qualityValue 찾기
        void CollectTooltip(const JSONValue& node, std::string& text, std::optional<int>& quality)
        {
            switch (node.GetType())
            {
                case JSONType::STRING_VALUE:
                    text += node.GetString();
                    text.push_back(TEXT_SEPARATOR);
                    break;
                case JSONType::ARRAY_VALUE:
                    for (const JSONValue& item : node.GetItems()) CollectTooltip(item, text, quality);
                    break;
                case JSONType::OBJECT_VALUE:
                    for (const auto& [key, value] : std::views::zip(node.GetKeys(), node.GetValues()))
                    {
                        if (key == KEY_QUALITY_VALUE && value.GetType() == JSONType::NUMBER_VALUE && !quality.has_value())
                        {
                            quality = static_cast<int>(value.GetNumber());
                            continue;
                        }
                        CollectTooltip(value, text, quality);
                    }
                    break;
                default:
                    break;
            }
        }

        // 표시 위치 뒤의 첫 정수 (공백 · '+'는 건너뜀, 쉼표는 무시, 숫자가 없으면 nullopt)
        [[nodiscard]] std::optional<long long> ReadNumberAt(std::string_view text, std::size_t position)
        {
            while (position < text.size() && SKIPPED_BEFORE_NUMBER.contains(text[position])) ++position;

            long long value{};
            bool hasDigit{};
            for (; position < text.size(); ++position)
            {
                const char character{ text[position] };
                if (character == DIGIT_SEPARATOR && hasDigit) continue;
                if (character < '0' || character > '9') break;
                value = value * DECIMAL_BASE + (character - '0');
                hasDigit = true;
            }
            if (!hasDigit) return std::nullopt;
            return value;
        }

        // 표시 글자 뒤의 숫자 (표시가 없으면 nullopt)
        [[nodiscard]] std::optional<long long> ReadNumberAfter(std::string_view text, std::string_view marker, std::size_t from, std::size_t until)
        {
            const std::size_t found{ text.find(marker, from) };
            if (found == std::string_view::npos || found >= until) return std::nullopt;
            return ReadNumberAt(text, found + marker.size());
        }

        // 기본 효과 구간 [시작, 끝) (없으면 nullopt)
        [[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>> FindBasicEffect(std::string_view text)
        {
            const std::size_t begin{ text.find(BASIC_EFFECT_MARKER) };
            if (begin == std::string_view::npos) return std::nullopt;

            const std::size_t contentBegin{ begin + BASIC_EFFECT_MARKER.size() };
            const std::size_t end{ text.find(EFFECT_WORD, contentBegin) };
            return std::pair{ contentBegin, end == std::string_view::npos ? text.size() : end };
        }

        // 이름 앞 "+N" → 일반 재련 단계 (없으면 0)
        [[nodiscard]] int ReadEnhancement(std::string_view name)
        {
            if (!name.starts_with(PLUS_SIGN)) return 0;
            return static_cast<int>(ReadNumberAt(name, 0).value_or(0));   // 앞의 '+'는 ReadNumberAt이 건너뜀
        }

        // ===== 악세서리 슬롯 번호 (목걸이 1 · 귀걸이 2 · 반지 2 순서) =====
        constexpr std::size_t ACCESSORY_NECKLACE_SLOT{ 0 };
        constexpr std::size_t ACCESSORY_FIRST_EARRING_SLOT{ 1 };
        constexpr std::size_t ACCESSORY_EARRING_COUNT{ 2 };
        constexpr std::size_t ACCESSORY_FIRST_RING_SLOT{ 3 };
        constexpr std::size_t ACCESSORY_RING_COUNT{ 2 };

        // ===== 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 그리드 · 아크 패시브 =====
        constexpr std::string_view KEY_AVATARS{ "ArmoryAvatars" };
        constexpr std::string_view KEY_ENGRAVING{ "ArmoryEngraving" };
        constexpr std::string_view KEY_ARK_PASSIVE_EFFECTS{ "ArkPassiveEffects" };
        constexpr std::string_view KEY_ARK_PASSIVE{ "ArkPassive" };
        constexpr std::string_view KEY_ARK_GRID{ "ArkGrid" };
        constexpr std::string_view KEY_SLOTS{ "Slots" };
        constexpr std::string_view KEY_POINT{ "Point" };
        constexpr std::string_view KEY_POINTS{ "Points" };
        constexpr std::string_view KEY_INDEX{ "Index" };
        constexpr std::string_view KEY_DESCRIPTION{ "Description" };
        constexpr std::string_view KEY_IS_INNER{ "IsInner" };
        constexpr std::string_view KEY_ELEMENT_HEADER{ "Element_000" };      // 툴팁 상자의 제목
        constexpr std::string_view KEY_ELEMENT_BODY{ "Element_001" };        // 툴팁 상자의 본문

        constexpr std::string_view ACCESSORY_TYPE_NECKLACE{ "목걸이" };
        constexpr std::string_view ACCESSORY_TYPE_EARRING{ "귀걸이" };
        constexpr std::string_view ACCESSORY_TYPE_RING{ "반지" };
        constexpr std::string_view STONE_TYPE{ "어빌리티 스톤" };
        constexpr std::string_view VAMBRACE_TYPE{ "완갑" };
        constexpr std::string_view POLISH_HEADER{ "연마 효과" };              // 악세서리 옵션 상자 제목
        constexpr std::string_view BRACELET_HEADER{ "팔찌 효과" };            // 팔찌 스탯 · 옵션 상자 제목
        constexpr std::string_view BRACELET_LOCKED_MARK{ "bracelet_locked" };       // 고정 줄 (스탯 · 고정 옵션)
        constexpr std::string_view BRACELET_CHANGEABLE_MARK{ "bracelet_changeable" };   // 바꿀 수 있는 옵션 줄의 시작
        constexpr std::string_view LEVEL_MARK{ "Lv." };
        constexpr char BRACKET_OPEN{ '[' };
        constexpr char BRACKET_CLOSE{ ']' };
        constexpr char BREAK_TAG_FIRST{ 'b' };
        constexpr char BREAK_TAG_SECOND{ 'r' };
        constexpr char SPACE_CHARACTER{ ' ' };
        constexpr std::string_view STAT_NUMBER_PREFIX{ "+" };

        constexpr std::array<std::string_view, Models::AVATAR_SLOT_COUNT> AVATAR_SLOT_NAMES{ "무기 아바타", "머리 아바타", "상의 아바타", "하의 아바타" };
        constexpr std::string_view AVATAR_LEGENDARY{ "전설" };
        constexpr std::string_view AVATAR_EPIC{ "영웅" };
        constexpr std::string_view ARK_CORE_ANCIENT{ "고대" };
        constexpr std::string_view ARK_CORE_RELIC{ "유물" };
        constexpr std::string_view ARK_CORE_LEGENDARY{ "전설" };
        constexpr std::array<std::string_view, Models::PASSIVE_TREE_COUNT> PASSIVE_TREE_NAMES{ "진화", "깨달음", "도약" };
        constexpr std::size_t PASSIVE_RANK_INDEX{ 0 };      // "6랭크 27레벨"에서 첫 번째 숫자
        constexpr std::size_t PASSIVE_LEVEL_INDEX{ 1 };     // 두 번째 숫자

        // ASCII 대소문자를 가리지 않고 같은 글자인지 (태그 이름 비교용)
        [[nodiscard]] bool EqualsIgnoreCase(char first, char second) noexcept
        {
            return std::tolower(static_cast<unsigned char>(first)) == std::tolower(static_cast<unsigned char>(second));
        }

        // HTML 글자를 <br> 기준으로 줄 나누기 (대소문자 무시, 태그는 그대로 남김)
        [[nodiscard]] std::vector<std::string> SplitBreaks(std::string_view html)
        {
            std::vector<std::string> lines;
            std::string current;
            for (std::size_t position{}; position < html.size();)
            {
                const bool isBreak{ html[position] == TAG_OPEN && position + 2 < html.size()
                    && EqualsIgnoreCase(html[position + 1], BREAK_TAG_FIRST) && EqualsIgnoreCase(html[position + 2], BREAK_TAG_SECOND) };
                const std::size_t close{ isBreak ? html.find(TAG_CLOSE, position) : std::string_view::npos };
                if (close != std::string_view::npos)
                {
                    lines.push_back(std::move(current));
                    current.clear();
                    position = close + 1;
                    continue;
                }
                current.push_back(html[position++]);
            }
            lines.push_back(std::move(current));
            return lines;
        }

        // 툴팁 JSON 트리에서 제목에 header가 들어 있는 상자의 본문 글자 찾기 (없으면 nullptr)
        [[nodiscard]] const std::string* FindPartBody(const JSONValue& node, std::string_view header)
        {
            if (node.GetType() == JSONType::OBJECT_VALUE)
            {
                const JSONValue& title{ node.FindMember(KEY_ELEMENT_HEADER) };
                const JSONValue& body{ node.FindMember(KEY_ELEMENT_BODY) };
                if (title.GetType() == JSONType::STRING_VALUE && body.GetType() == JSONType::STRING_VALUE && title.GetString().contains(header)) return &body.GetString();
                for (const JSONValue& value : node.GetValues())
                {
                    if (const std::string* found{ FindPartBody(value, header) }; found != nullptr) return found;
                }
            }
            else if (node.GetType() == JSONType::ARRAY_VALUE)
            {
                for (const JSONValue& item : node.GetItems())
                {
                    if (const std::string* found{ FindPartBody(item, header) }; found != nullptr) return found;
                }
            }
            return nullptr;
        }

        // 한 줄 HTML → 태그를 지우고 공백을 정리한 글자
        [[nodiscard]] std::string CleanLine(std::string_view htmlLine)
        {
            return Models::NormalizeSpaces(StripTags(htmlLine));
        }

        // 글자 속 "[이름] ... Lv.N" 모음 (스톤 각인 · 아크 그리드 젬 효과), Lv.이 따라오지 않는 대괄호는 건너뜀
        [[nodiscard]] std::vector<std::pair<std::string, int>> FindLevelTags(std::string_view text)
        {
            std::vector<std::pair<std::string, int>> tags;
            for (std::size_t open{ text.find(BRACKET_OPEN) }; open != std::string_view::npos; open = text.find(BRACKET_OPEN, open + 1))
            {
                const std::size_t close{ text.find(BRACKET_CLOSE, open) };
                if (close == std::string_view::npos) break;

                std::size_t position{ close + 1 };
                while (position < text.size() && (text[position] == SPACE_CHARACTER || text[position] == TEXT_SEPARATOR)) ++position;
                if (!text.substr(position).starts_with(LEVEL_MARK)) continue;

                const std::optional<long long> level{ ReadNumberAt(text, position + LEVEL_MARK.size()) };
                if (!level.has_value()) continue;
                tags.emplace_back(Models::NormalizeSpaces(text.substr(open + 1, close - open - 1)), static_cast<int>(*level));
            }
            return tags;
        }

        // 글자 속 정수들을 순서대로 ("6랭크 27레벨" → 6, 27)
        [[nodiscard]] std::vector<int> ExtractIntegers(std::string_view text)
        {
            std::vector<int> numbers;
            for (std::size_t position{}; position < text.size();)
            {
                if (text[position] < '0' || text[position] > '9')
                {
                    ++position;
                    continue;
                }
                int value{};
                for (; position < text.size() && text[position] >= '0' && text[position] <= '9'; ++position) value = value * DECIMAL_BASE + (text[position] - '0');
                numbers.push_back(value);
            }
            return numbers;
        }

        // 팔찌 스탯 줄 읽기 ("신속 +119" → 신속, 119, 스탯 종류가 아니면 nullopt)
        [[nodiscard]] std::optional<Models::BraceletStat> ParseBraceletStat(std::string_view text)
        {
            const std::size_t space{ text.find(SPACE_CHARACTER) };
            if (space == std::string_view::npos) return std::nullopt;

            const std::string_view type{ text.substr(0, space) };
            const std::span<const std::string_view> types{ Models::CombatCoefficientData::GetBraceletStatTypes() };
            if (Models::IsNone(type) || std::ranges::find(types, type) == types.end()) return std::nullopt;

            const std::string_view rest{ text.substr(space + 1) };
            if (!rest.starts_with(STAT_NUMBER_PREFIX)) return std::nullopt;
            const std::optional<long long> value{ ReadNumberAt(rest, 0) };
            if (!value.has_value()) return std::nullopt;
            return Models::BraceletStat{ std::string{ type }, static_cast<int>(*value) };
        }

        // 악세서리: 연마 효과 상자의 줄들 → 옵션 문구
        void ParseAccessoryOptions(const JSONValue& tooltip, Models::AccessoryState& accessory)
        {
            const std::string* body{ FindPartBody(tooltip, POLISH_HEADER) };
            if (body == nullptr) return;

            std::size_t index{};
            for (const std::string& line : SplitBreaks(*body))
            {
                const std::string text{ CleanLine(line) };
                if (text.empty()) continue;
                if (index >= Models::ACCESSORY_OPTION_COUNT) break;
                accessory.m_options[index++] = text;
            }
        }

        // 팔찌: 고정 줄(스탯 · 고정 옵션)과 바꿀 수 있는 옵션 줄 → 스탯 4개 · 옵션 4개
        // 옵션 하나가 문장 여러 줄이면 아이콘이 없는 다음 줄들을 앞 옵션에 이어 붙임
        void ParseBracelet(const JSONValue& tooltip, Models::ExtraState& extra)
        {
            const std::string* body{ FindPartBody(tooltip, BRACELET_HEADER) };
            if (body == nullptr) return;

            const bool hasMarks{ body->contains(BRACELET_LOCKED_MARK) || body->contains(BRACELET_CHANGEABLE_MARK) };
            std::size_t statIndex{};
            std::size_t optionIndex{};
            std::string option;
            bool isInOption{};

            const auto flushOption{ [&]
            {
                if (isInOption && optionIndex < Models::BRACELET_OPTION_COUNT) extra.m_braceletOptions[optionIndex++] = Models::RemoveSpaces(option);
                option.clear();
                isInOption = false;
            } };

            for (const std::string& line : SplitBreaks(*body))
            {
                const std::string text{ CleanLine(line) };
                if (text.empty()) continue;

                const bool isLocked{ line.contains(BRACELET_LOCKED_MARK) };
                const bool isChangeable{ line.contains(BRACELET_CHANGEABLE_MARK) };
                if (hasMarks && !isLocked && !isChangeable)
                {
                    if (isInOption) option += SPACE_CHARACTER + text;   // 앞 옵션의 다음 문장
                    continue;
                }

                flushOption();
                if (!isChangeable)
                {
                    if (const std::optional<Models::BraceletStat> stat{ ParseBraceletStat(text) }; stat.has_value())
                    {
                        if (statIndex < Models::BRACELET_STAT_COUNT) extra.m_braceletStats[statIndex++] = *stat;
                        continue;
                    }
                }
                option = text;
                isInOption = true;
            }
            flushOption();
        }

        // 스톤: 툴팁 글자의 "[각인] Lv.N" 앞 두 줄 (세 번째는 감소 각인)
        void ParseStone(std::string_view strippedText, Models::ExtraState& extra)
        {
            const std::vector<std::pair<std::string, int>> tags{ FindLevelTags(strippedText) };
            for (std::size_t index{}; index < Models::STONE_LINE_COUNT && index < tags.size(); ++index)
            {
                extra.m_stone[index] = Models::StoneLine{ tags[index].first, tags[index].second };
            }
        }

        // 아크 그리드 코어 등급 글자 → 열거형 (모르면 영웅)
        [[nodiscard]] Models::ArkCoreGrade ToCoreGrade(std::string_view grade) noexcept
        {
            if (grade == ARK_CORE_ANCIENT) return Models::ArkCoreGrade::CORE_ANCIENT;
            if (grade == ARK_CORE_LEGENDARY) return Models::ArkCoreGrade::CORE_LEGENDARY;
            if (grade == ARK_CORE_RELIC) return Models::ArkCoreGrade::CORE_RELIC;
            return Models::ArkCoreGrade::CORE_EPIC;
        }

        // 툴팁 JSON 글자를 읽어 트리로 (읽지 못하면 null 값)
        [[nodiscard]] JSONValue ReadTooltipTree(std::string_view tooltip)
        {
            try
            {
                return ParseJSONText(tooltip);
            }
            catch (const std::exception&)
            {
                return JSONValue{};
            }
        }

        // 툴팁 트리의 글자를 이어 붙이고 태그를 지움
        [[nodiscard]] std::string ReadTooltipText(std::string_view tooltip)
        {
            std::string joined;
            std::optional<int> unusedQuality;
            CollectTooltip(ReadTooltipTree(tooltip), joined, unusedQuality);
            return StripTags(joined);
        }

        // 장비 한 개의 악세서리 · 팔찌 · 스톤 · 완갑 정보를 extra에 반영 (earringCount · ringCount = 지금까지 읽은 귀걸이 · 반지 수)
        void ParseEquipmentExtra(const Models::GearItem& item, std::string_view tooltip, Models::ExtraState& extra, std::size_t& earringCount, std::size_t& ringCount)
        {
            if (item.m_type == VAMBRACE_TYPE)
            {
                extra.m_vambraceLevel = item.m_enhancement;
                return;
            }

            std::optional<std::size_t> accessorySlot;
            if (item.m_type == ACCESSORY_TYPE_NECKLACE) accessorySlot = ACCESSORY_NECKLACE_SLOT;
            else if (item.m_type == ACCESSORY_TYPE_EARRING && earringCount < ACCESSORY_EARRING_COUNT) accessorySlot = ACCESSORY_FIRST_EARRING_SLOT + earringCount++;
            else if (item.m_type == ACCESSORY_TYPE_RING && ringCount < ACCESSORY_RING_COUNT) accessorySlot = ACCESSORY_FIRST_RING_SLOT + ringCount++;

            if (accessorySlot.has_value())
            {
                Models::AccessoryState& accessory{ extra.m_accessories[*accessorySlot] };
                accessory.m_mainStat = item.m_mainStat;
                ParseAccessoryOptions(ReadTooltipTree(tooltip), accessory);
            }
            else if (item.m_type == BRACELET_TYPE)
            {
                ParseBracelet(ReadTooltipTree(tooltip), extra);
            }
            else if (item.m_type == STONE_TYPE)
            {
                ParseStone(ReadTooltipText(tooltip), extra);
            }
        }

        // 아바타: 장착 중인 아바타(IsInner) 중 부위별 가장 높은 등급 (덧입기 이벤트 아바타는 능력치가 없어서 제외)
        void ParseAvatars(const JSONValue& avatars, Models::ExtraState& extra)
        {
            for (const JSONValue& avatar : avatars.GetItems())
            {
                const JSONValue& inner{ avatar.FindMember(KEY_IS_INNER) };
                if (inner.GetType() == JSONType::BOOL_VALUE && !inner.GetBool()) continue;

                const auto slot{ std::ranges::find(AVATAR_SLOT_NAMES, std::string_view{ avatar.FindMember(KEY_TYPE).GetString() }) };
                if (slot == AVATAR_SLOT_NAMES.end()) continue;

                const std::string_view grade{ avatar.FindMember(KEY_GRADE).GetString() };
                Models::AvatarGrade parsed{ Models::AvatarGrade::AVATAR_NONE };
                if (grade == AVATAR_LEGENDARY) parsed = Models::AvatarGrade::AVATAR_LEGENDARY;
                else if (grade == AVATAR_EPIC) parsed = Models::AvatarGrade::AVATAR_EPIC;

                Models::AvatarGrade& saved{ extra.m_avatars[static_cast<std::size_t>(std::distance(AVATAR_SLOT_NAMES.begin(), slot))] };
                saved = std::max(saved, parsed);
            }
        }

        // 각인서 5개: ArkPassiveEffects의 이름 · 등급 · 레벨(각인서 레벨)
        void ParseBooks(const JSONValue& engraving, Models::ExtraState& extra)
        {
            std::size_t index{};
            for (const JSONValue& effect : engraving.FindMember(KEY_ARK_PASSIVE_EFFECTS).GetItems())
            {
                if (index >= Models::BOOK_COUNT) break;
                extra.m_books[index++] = Models::BookState{ effect.FindMember(KEY_NAME).GetString(), effect.FindMember(KEY_GRADE).GetString() == AVATAR_LEGENDARY,
                    static_cast<int>(effect.FindMember(KEY_LEVEL).GetNumber()) };
            }
        }

        // 아크 패시브: 진화 · 깨달음 · 도약의 "N랭크 M레벨" (랭크는 세 트리 중 가장 낮은 값)
        void ParseArkPassive(const JSONValue& arkPassive, Models::ExtraState& extra)
        {
            std::optional<int> lowestRank;
            for (const JSONValue& point : arkPassive.FindMember(KEY_POINTS).GetItems())
            {
                const auto tree{ std::ranges::find(PASSIVE_TREE_NAMES, std::string_view{ point.FindMember(KEY_NAME).GetString() }) };
                if (tree == PASSIVE_TREE_NAMES.end()) continue;

                const std::vector<int> numbers{ ExtractIntegers(point.FindMember(KEY_DESCRIPTION).GetString()) };
                if (numbers.size() <= PASSIVE_LEVEL_INDEX) continue;
                extra.m_passive.m_levels[static_cast<std::size_t>(std::distance(PASSIVE_TREE_NAMES.begin(), tree))] = numbers[PASSIVE_LEVEL_INDEX];
                lowestRank = std::min(lowestRank.value_or(numbers[PASSIVE_RANK_INDEX]), numbers[PASSIVE_RANK_INDEX]);
            }
            if (lowestRank.has_value()) extra.m_passive.m_rank = *lowestRank;
        }

        // 아크 그리드: 코어 6개(등급 · 포인트) + 젬 효과 레벨 합
        void ParseArkGrid(const JSONValue& arkGrid, Models::ExtraState& extra)
        {
            for (const JSONValue& slot : arkGrid.FindMember(KEY_SLOTS).GetItems())
            {
                const auto index{ static_cast<std::size_t>(slot.FindMember(KEY_INDEX).GetNumber()) };
                if (index >= Models::ARK_CORE_COUNT) continue;
                extra.m_cores[index] = Models::ArkCoreState{ ToCoreGrade(slot.FindMember(KEY_GRADE).GetString()), static_cast<int>(slot.FindMember(KEY_POINT).GetNumber()) };

                for (const JSONValue& gem : slot.FindMember(KEY_GEMS).GetItems())
                {
                    for (const auto& [name, level] : FindLevelTags(ReadTooltipText(gem.FindMember(KEY_TOOLTIP).GetString())))
                    {
                        const auto effect{ std::ranges::find(Models::ARK_GEM_EFFECT_NAMES, std::string_view{ name }) };
                        if (effect != Models::ARK_GEM_EFFECT_NAMES.end()) extra.m_arkGemLevels[static_cast<std::size_t>(std::distance(Models::ARK_GEM_EFFECT_NAMES.begin(), effect))] += level;
                    }
                }
            }
        }
    }

    void ParseGearTooltip(std::string_view tooltip, bool isWeapon, Models::GearItem& item)
    {
        // 툴팁은 JSON 문자열 → 트리로 읽어 글자를 이어 붙임 (읽지 못하면 원문 그대로 + 정규식으로 품질)
        std::string joined;
        std::optional<int> quality;
        try
        {
            CollectTooltip(ParseJSONText(tooltip), joined, quality);
        }
        catch (const std::exception&)
        {
            joined = std::string{ tooltip };
            std::match_results<std::string_view::const_iterator> match;
            if (std::regex_search(tooltip.begin(), tooltip.end(), match, std::regex{ QUALITY_TEXT_PATTERN.begin(), QUALITY_TEXT_PATTERN.end() }))
            {
                quality = ParseDecimal(match[QUALITY_GROUP].str()).transform([](double value) { return static_cast<int>(value); });
            }
        }
        if (quality.has_value() && *quality <= NO_QUALITY) quality.reset();
        item.m_quality = quality;

        const std::string text{ StripTags(joined) };
        item.m_advancedEnhancement = static_cast<int>(ReadNumberAfter(text, ADVANCED_MARKER, 0, text.size()).value_or(0));

        // 주스탯: 기본 효과 구간에서 힘 · 민첩 · 지능 중 처음 나오는 것
        if (const auto basic{ FindBasicEffect(text) }; basic.has_value())
        {
            std::size_t firstPosition{ std::string::npos };
            for (const std::string_view marker : MAIN_STAT_MARKERS)
            {
                const std::size_t found{ text.find(marker, basic->first) };
                if (found < basic->second && found < firstPosition)
                {
                    firstPosition = found;
                    item.m_mainStat = static_cast<double>(ReadNumberAt(text, found + marker.size()).value_or(0));
                }
            }
            if (isWeapon) item.m_weaponAttack = static_cast<double>(ReadNumberAfter(text, WEAPON_ATTACK_MARKER, basic->first, basic->second).value_or(0));
        }

        // 팔찌: 주스탯이 기본 효과가 아니라 팔찌 효과 묶음에 "힘 +12345"처럼 나옴 → 툴팁 전체의 힘 · 민첩 · 지능 줄을 모두 더함
        // ⚠️ 실제 API 응답으로 확인하지 않은 규칙 (팔찌 툴팁 형식이 다르면 S0가 조금 달라짐)
        if (item.m_type == BRACELET_TYPE)
        {
            double braceletStat{};
            for (const std::string_view marker : MAIN_STAT_MARKERS)
            {
                for (std::size_t found{ text.find(marker) }; found != std::string::npos; found = text.find(marker, found + marker.size()))
                {
                    braceletStat += static_cast<double>(ReadNumberAt(text, found + marker.size()).value_or(0));
                }
            }
            item.m_mainStat = braceletStat;
        }

        // 기본 효과 구간을 못 찾은 무기는 툴팁 전체에서 무기 공격력 찾기
        if (isWeapon && item.m_weaponAttack <= 0.0)
        {
            item.m_weaponAttack = static_cast<double>(ReadNumberAfter(text, WEAPON_ATTACK_MARKER, 0, text.size()).value_or(0));
        }
    }

    Models::CharacterArmory ParseArmory(const JSONValue& root)
    {
        // 없는 이름은 200 + null (또는 프로필 null)
        const JSONValue& profile{ root.FindMember(KEY_PROFILE) };
        if (root.GetType() != JSONType::OBJECT_VALUE || profile.GetType() != JSONType::OBJECT_VALUE)
        {
            throw std::runtime_error{ std::string{ NOT_FOUND_MESSAGE } };
        }

        Models::CharacterArmory armory;
        armory.m_characterName = profile.FindMember(KEY_CHARACTER_NAME).GetString();
        armory.m_className = profile.FindMember(KEY_CHARACTER_CLASS_NAME).GetString();
        armory.m_itemAvgLevel = ReadFlexibleNumber(profile.FindMember(KEY_ITEM_AVG_LEVEL)).value_or(0.0);
        armory.m_combatPower = ReadFlexibleNumber(profile.FindMember(KEY_COMBAT_POWER));
        if (armory.m_combatPower.has_value() && *armory.m_combatPower <= 0.0) armory.m_combatPower.reset();

        // 장비: 모든 항목의 주스탯을 더하고(S0), 시뮬레이터 부위만 따로 보관
        std::size_t earringCount{};
        std::size_t ringCount{};
        for (const JSONValue& entry : root.FindMember(KEY_EQUIPMENT).GetItems())
        {
            Models::GearItem item;
            item.m_type = entry.FindMember(KEY_TYPE).GetString();
            item.m_name = entry.FindMember(KEY_NAME).GetString();
            item.m_grade = entry.FindMember(KEY_GRADE).GetString();
            item.m_enhancement = ReadEnhancement(item.m_name);

            const std::optional<Models::GearPart> part{ Models::FindGearPart(item.m_type) };
            ParseGearTooltip(entry.FindMember(KEY_TOOLTIP).GetString(), part == Models::GearPart::PART_WEAPON, item);
            armory.m_totalMainStat += item.m_mainStat;

            // 악세서리 · 팔찌 · 스톤 · 완갑은 따로 정리
            ParseEquipmentExtra(item, entry.FindMember(KEY_TOOLTIP).GetString(), armory.m_extra, earringCount, ringCount);

            if (!part.has_value()) continue;
            std::optional<Models::GearItem>& slot{ armory.m_gears[Models::ToGearIndex(*part)] };
            if (!slot.has_value()) slot = std::move(item);   // 같은 부위가 두 번 오면 첫 항목
        }

        // 보석 (없으면 빈 목록)
        for (const JSONValue& gem : root.FindMember(KEY_GEM).FindMember(KEY_GEMS).GetItems())
        {
            armory.m_gemLevels.push_back(static_cast<int>(gem.FindMember(KEY_LEVEL).GetNumber()));

            // 종류: 이름("10레벨 겁화의 보석", 태그 포함 가능)에 들어 있는 종류 글자 (없으면 빈 문자열)
            const std::string gemName{ StripTags(gem.FindMember(KEY_NAME).GetString()) };
            const auto kind{ std::ranges::find_if(GEM_KINDS, [&gemName](std::string_view candidate) { return gemName.contains(candidate); }) };
            armory.m_gemKinds.emplace_back(kind != GEM_KINDS.end() ? *kind : std::string_view{});
        }

        // 보석 기본 공격력 총합 (퍼센트 → 비율, 없으면 0)
        const std::string gemEffectText{ StripTags(root.FindMember(KEY_GEM).FindMember(KEY_EFFECTS).FindMember(KEY_DESCRIPTION).GetString()) };
        if (const std::size_t marker{ gemEffectText.find(GEM_BASE_ATTACK_MARKER) }; marker != std::string::npos)
        {
            const std::size_t colon{ gemEffectText.find(':', marker) };
            if (colon != std::string::npos) armory.m_gemBaseAttackRate = ParseDecimal(std::string_view{ gemEffectText }.substr(colon + 1, gemEffectText.find('%', colon) - colon - 1)).value_or(0.0) / PERCENT_DIVISOR;
        }

        // 아바타 · 각인서 · 아크 패시브 · 아크 그리드 (응답에 없으면 기본값 그대로)
        ParseAvatars(root.FindMember(KEY_AVATARS), armory.m_extra);
        ParseBooks(root.FindMember(KEY_ENGRAVING), armory.m_extra);
        ParseArkPassive(root.FindMember(KEY_ARK_PASSIVE), armory.m_extra);
        ParseArkGrid(root.FindMember(KEY_ARK_GRID), armory.m_extra);
        return armory;
    }
}
