module LostArkMultiTool.Services.ArmoryParser;  // 무기고 해석기 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APIFormatMonitor;  // API 형식 변경 신고 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모델 가져오기
import LostArkMultiTool.Services.JSONValue;  // JSON 값 모듈 가져오기

namespace LostArkMultiTool::Services  // 서비스 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        // ===== 응답 JSON 키 =====
        constexpr std::string_view KEY_PROFILE{ "ArmoryProfile" };  // 응답의 프로필 키
        constexpr std::string_view KEY_EQUIPMENT{ "ArmoryEquipment" };  // 응답의 장비 목록 키
        constexpr std::string_view KEY_GEM{ "ArmoryGem" };  // 응답의 보석 키
        constexpr std::string_view KEY_GEMS{ "Gems" };  // 보석 목록 키
        constexpr std::string_view KEY_EFFECTS{ "Effects" };   // ArmoryGem.Effects (보석 효과 합계)
        constexpr std::string_view GEM_BASE_ATTACK_MARKER{ "기본 공격력 총합" };   // "기본 공격력 총합 : 6.60%"
        constexpr double PERCENT_DIVISOR{ 100.0 };  // 퍼센트를 비율로 바꾸는 나눔 값
        constexpr std::string_view KEY_SKILLS{ "Skills" };            // ArmoryGem.Effects.Skills[]
        constexpr std::string_view KEY_GEM_SLOT{ "GemSlot" };  // 보석 슬롯 번호 키
        constexpr std::string_view GEM_DAMAGE_TEXT{ "피해" };          // 설명에 "피해 N% 증가" → 겁화 계열
        constexpr std::string_view GEM_COOLDOWN_TEXT{ "재사용" };      // 설명에 "재사용 대기시간 N% 감소" → 작열 계열
        constexpr std::string_view CORE_NAME_SEPARATOR{ " : " };          // 코어 제목 "질서의 해 코어 : 샷건 오버로드"
        constexpr std::string_view GEM_DAMAGE_KIND{ "겁화" };  // 겁화 보석 종류 이름
        constexpr std::string_view GEM_COOLDOWN_KIND{ "작열" };  // 작열 보석 종류 이름
        constexpr std::string_view KEY_LEVEL{ "Level" };  // 레벨 키
        constexpr std::string_view KEY_CHARACTER_NAME{ "CharacterName" };  // 캐릭터 이름 키
        constexpr std::string_view KEY_CHARACTER_CLASS_NAME{ "CharacterClassName" };  // 직업 이름 키
        constexpr std::string_view KEY_ITEM_AVG_LEVEL{ "ItemAvgLevel" };  // 평균 아이템 레벨 키
        constexpr std::string_view KEY_COMBAT_POWER{ "CombatPower" };  // 전투력 키
        constexpr std::string_view KEY_TYPE{ "Type" };  // 장비 종류 키
        constexpr std::string_view KEY_NAME{ "Name" };  // 장비 이름 키
        constexpr std::string_view KEY_GRADE{ "Grade" };  // 장비 등급 키
        constexpr std::string_view KEY_TOOLTIP{ "Tooltip" };  // 장비 툴팁 키
        constexpr std::string_view KEY_QUALITY_VALUE{ "qualityValue" };  // 툴팁 안 품질 키

        // ===== 툴팁 글자 =====
        constexpr std::string_view ADVANCED_MARKER{ "[상급 재련]" };  // 상급 재련 표시 글자
        constexpr std::string_view BASIC_EFFECT_MARKER{ "기본 효과" };  // 기본 효과 구간 표시 글자
        constexpr std::string_view EFFECT_WORD{ "효과" };                        // 다음 묶음 제목 ("추가 효과" 등) = 기본 효과 구간 끝
        constexpr std::array<std::string_view, 3> MAIN_STAT_MARKERS{ "힘 +", "민첩 +", "지능 +" };  // 주스탯 표시 글자 (힘 · 민첩 · 지능)
        constexpr std::string_view WEAPON_ATTACK_MARKER{ "무기 공격력 +" };  // 무기 공격력 표시 글자
        constexpr std::string_view BRACELET_TYPE{ "팔찌" };  // 팔찌 장비 종류 이름
        constexpr std::array<std::string_view, 4> GEM_KINDS{ "겁화", "작열", "멸화", "홍염" };   // 보석 이름에서 찾는 종류 글자                      // 주스탯이 기본 효과 밖에 있는 장비 Type
        constexpr std::string_view QUALITY_TEXT_PATTERN{ R"("qualityValue"\s*:\s*(-?\d+))" };   // 툴팁을 JSON으로 읽지 못했을 때
        constexpr char TAG_OPEN{ '<' };  // 태그 시작 글자
        constexpr char TAG_CLOSE{ '>' };  // 태그 끝 글자
        constexpr char TEXT_SEPARATOR{ '\n' };  // 줄 구분 글자
        constexpr char PLUS_SIGN{ '+' };  // 덧셈 기호
        constexpr char DIGIT_SEPARATOR{ ',' };  // 숫자 천 단위 구분 글자
        constexpr std::string_view SKIPPED_BEFORE_NUMBER{ " \t+" };  // 숫자 앞에서 건너뛸 글자들
        constexpr int NO_QUALITY{ -1 };                                          // API: 품질이 없는 장비
        constexpr int DECIMAL_BASE{ 10 };  // 숫자 변환 기준 (10진수)
        constexpr std::size_t QUALITY_GROUP{ 1 };                                // 정규식의 품질 숫자 묶음

        constexpr std::string_view NOT_FOUND_MESSAGE{ "캐릭터를 찾을 수 없어요. 이름을 다시 확인해 주세요." };  // 캐릭터를 찾지 못했을 때 안내 문구

        // "1,764.17" · "1764.17" → 숫자 (쉼표는 무시, 읽을 수 없으면 nullopt)
        [[nodiscard]] std::optional<double> ParseDecimal(std::string_view text)
        {
            std::string cleaned;  // 쉼표 · 공백을 지운 글자
            cleaned.reserve(text.size());  // 입력 길이만큼 메모리 미리 확보
            for (const char character : text)  // 글자를 하나씩 순회
            {
                if (character != DIGIT_SEPARATOR && character != ' ') cleaned.push_back(character);  // 구분 글자가 아니면 보관
            }
            if (cleaned.empty()) return std::nullopt;  // 비었으면 값 없음

            double value{};  // 변환 결과
            const auto [end, error]{ std::from_chars(cleaned.data(), cleaned.data() + cleaned.size(), value) };  // 숫자로 변환 (끝 위치와 오류 코드 반환)
            if (error != std::errc{} || end != cleaned.data() + cleaned.size()) return std::nullopt;  // 오류가 있거나 끝까지 읽지 못했으면 값 없음
            return value;  // 변환한 값 반환
        }

        // 문자열 또는 숫자로 오는 값 → 숫자 (없거나 읽을 수 없으면 nullopt)
        [[nodiscard]] std::optional<double> ReadFlexibleNumber(const JSONValue& value)
        {
            switch (value.GetType())  // JSON 값 종류에 따라 분기
            {
                case JSONType::NUMBER_VALUE:  // 숫자 형식
                    return value.GetNumber();  // 숫자 그대로 반환
                case JSONType::STRING_VALUE:  // 문자열 형식
                    return ParseDecimal(value.GetString());  // 문자열을 숫자로 변환
                default:  // 그 밖의 형식
                    return std::nullopt;  // 값 없음
            }
        }

        // <...> 태그를 지운 글자 (태그 자리는 공백 하나 → "+123<BR>힘" 같은 글자가 붙지 않게)
        [[nodiscard]] std::string StripTags(std::string_view text)
        {
            std::string stripped;  // 태그를 지운 글자
            stripped.reserve(text.size());  // 입력 길이만큼 메모리 미리 확보
            bool isInTag{};  // 태그 안에 있는지 표시
            for (const char character : text)  // 글자를 하나씩 순회
            {
                if (character == TAG_OPEN)  // 태그 시작이면
                {
                    isInTag = true;  // 태그 안으로 들어감
                    continue;  // 글자는 버리고 다음으로
                }
                if (character == TAG_CLOSE && isInTag)  // 태그 끝이고 태그 안이면
                {
                    isInTag = false;  // 태그 밖으로 나옴
                    stripped.push_back(' ');  // 태그 자리는 공백 하나로 대체
                    continue;  // 다음으로
                }
                if (!isInTag) stripped.push_back(character);  // 태그 밖의 글자만 보관
            }
            return stripped;  // 태그를 지운 글자 반환
        }

        // 툴팁 JSON 트리의 글자를 순서대로 이어 붙임 (키 순서 = 툴팁 표시 순서) + qualityValue 찾기
        void CollectTooltip(const JSONValue& node, std::string& text, std::optional<int>& quality)
        {
            switch (node.GetType())  // 노드 형식에 따라 분기
            {
                case JSONType::STRING_VALUE:  // 문자열 형식
                    text += node.GetString();  // 툴팁 글자에 이어 붙임
                    text.push_back(TEXT_SEPARATOR);  // 줄 구분 글자 추가
                    break;  // 분기 끝
                case JSONType::ARRAY_VALUE:  // 배열 형식
                    for (const JSONValue& item : node.GetItems()) CollectTooltip(item, text, quality);  // 각 원소를 재귀로 수집
                    break;  // 분기 끝
                case JSONType::OBJECT_VALUE:  // 객체 형식
                    for (const auto& [key, value] : std::views::zip(node.GetKeys(), node.GetValues()))  // 키와 값 쌍을 순회
                    {
                        if (key == KEY_QUALITY_VALUE && value.GetType() == JSONType::NUMBER_VALUE && !quality.has_value())  // 품질 키이면서 숫자이고 아직 품질을 못 찾았으면
                        {
                            quality = static_cast<int>(value.GetNumber());  // 품질 기록
                            continue;  // 이 항목은 글자 수집에서 제외
                        }
                        CollectTooltip(value, text, quality);  // 그 밖의 값은 재귀로 수집
                    }
                    break;  // 분기 끝
                default:  // 그 밖의 형식
                    break;  // 무시
            }
        }

        // 표시 위치 뒤의 첫 정수 (공백 · '+'는 건너뜀, 쉼표는 무시, 숫자가 없으면 nullopt)
        [[nodiscard]] std::optional<long long> ReadNumberAt(std::string_view text, std::size_t position)
        {
            while (position < text.size() && SKIPPED_BEFORE_NUMBER.contains(text[position])) ++position;  // 숫자 앞의 공백 · 덧셈 기호 건너뜀

            long long value{};  // 읽은 값
            bool hasDigit{};  // 숫자를 하나라도 읽었는지
            for (; position < text.size(); ++position)  // 위치부터 끝까지 순회
            {
                const char character{ text[position] };  // 현재 글자
                if (character == DIGIT_SEPARATOR && hasDigit) continue;  // 천 단위 구분 글자는 건너뜀
                if (character < '0' || character > '9') break;  // 숫자가 아니면 읽기 끝
                value = value * DECIMAL_BASE + (character - '0');  // 자리 올림하며 값 누적
                hasDigit = true;  // 숫자 읽음 표시
            }
            if (!hasDigit) return std::nullopt;  // 숫자가 없으면 값 없음
            return value;  // 읽은 값 반환
        }

        // 표시 글자 뒤의 숫자 (표시가 없으면 nullopt)
        [[nodiscard]] std::optional<long long> ReadNumberAfter(std::string_view text, std::string_view marker, std::size_t from, std::size_t until)
        {
            const std::size_t found{ text.find(marker, from) };  // 표식 위치 찾기
            if (found == std::string_view::npos || found >= until) return std::nullopt;  // 표식이 없거나 범위 밖이면 값 없음
            return ReadNumberAt(text, found + marker.size());  // 표식 뒤의 숫자 읽기
        }

        // 기본 효과 구간 [시작, 끝) (없으면 nullopt)
        [[nodiscard]] std::optional<std::pair<std::size_t, std::size_t>> FindBasicEffect(std::string_view text)
        {
            const std::size_t begin{ text.find(BASIC_EFFECT_MARKER) };  // 기본 효과 시작 위치 찾기
            if (begin == std::string_view::npos) return std::nullopt;  // 없으면 값 없음

            const std::size_t contentBegin{ begin + BASIC_EFFECT_MARKER.size() };  // 내용 시작 위치
            const std::size_t end{ text.find(EFFECT_WORD, contentBegin) };  // 내용 끝 위치 (다음 "효과" 글자)
            return std::pair{ contentBegin, end == std::string_view::npos ? text.size() : end };  // 끝이 없으면 글자 끝까지로 범위 반환
        }

        // 이름 앞 "+N" → 일반 재련 단계 (없으면 0)
        [[nodiscard]] int ReadEnhancement(std::string_view name)
        {
            if (!name.starts_with(PLUS_SIGN)) return 0;  // 이름이 "+"로 시작하지 않으면 재련 0
            return static_cast<int>(ReadNumberAt(name, 0).value_or(0));   // 앞의 '+'는 ReadNumberAt이 건너뜀
        }

        // ===== 악세서리 슬롯 번호 (목걸이 1 · 귀걸이 2 · 반지 2 순서) =====
        constexpr std::size_t ACCESSORY_NECKLACE_SLOT{ 0 };  // 목걸이 슬롯 번호
        constexpr std::size_t ACCESSORY_FIRST_EARRING_SLOT{ 1 };  // 첫 번째 귀걸이 슬롯 번호
        constexpr std::size_t ACCESSORY_EARRING_COUNT{ 2 };  // 귀걸이 개수
        constexpr std::size_t ACCESSORY_FIRST_RING_SLOT{ 3 };  // 첫 번째 반지 슬롯 번호
        constexpr std::size_t ACCESSORY_RING_COUNT{ 2 };  // 반지 개수

        // ===== 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 그리드 · 아크 패시브 =====
        constexpr std::string_view KEY_AVATARS{ "ArmoryAvatars" };  // 아바타 키
        constexpr std::string_view KEY_ENGRAVING{ "ArmoryEngraving" };  // 각인 키
        constexpr std::string_view KEY_ARK_PASSIVE_EFFECTS{ "ArkPassiveEffects" };  // 아크 패시브 효과 키
        constexpr std::string_view KEY_ARK_PASSIVE{ "ArkPassive" };  // 아크 패시브 키
        constexpr std::string_view KEY_ARK_GRID{ "ArkGrid" };  // 아크 그리드 키
        constexpr std::string_view KEY_SLOTS{ "Slots" };  // 슬롯 목록 키
        constexpr std::string_view KEY_POINT{ "Point" };  // 포인트 키
        constexpr std::string_view KEY_POINTS{ "Points" };  // 포인트 합 키
        constexpr std::string_view KEY_INDEX{ "Index" };  // 번호 키
        constexpr std::string_view KEY_DESCRIPTION{ "Description" };  // 설명 키
        constexpr std::string_view KEY_IS_INNER{ "IsInner" };  // 안쪽 슬롯 여부 키
        constexpr std::string_view KEY_ELEMENT_HEADER{ "Element_000" };      // 툴팁 상자의 제목
        constexpr std::string_view KEY_ELEMENT_BODY{ "Element_001" };        // 툴팁 상자의 본문

        constexpr std::string_view ACCESSORY_TYPE_NECKLACE{ "목걸이" };  // 목걸이 종류 이름
        constexpr std::string_view ACCESSORY_TYPE_EARRING{ "귀걸이" };  // 귀걸이 종류 이름
        constexpr std::string_view ACCESSORY_TYPE_RING{ "반지" };  // 반지 종류 이름
        constexpr std::string_view STONE_TYPE{ "어빌리티 스톤" };  // 어빌리티 스톤 종류 이름
        constexpr std::string_view VAMBRACE_TYPE{ "완갑" };  // 완갑 종류 이름
        constexpr std::string_view VAMBRACE_EXTRA_HEADER{ "추가 효과" };      // 완갑 한계 해방 추가 효과 상자 제목
        constexpr std::string_view VAMBRACE_BASE_ATTACK_TEXT{ "기본 공격력" };   // 추가 효과 줄 "기본 공격력 +2.00%"
        constexpr char PLUS_CHARACTER{ '+' };                                  // 수치 앞 덧셈 기호
        constexpr char PERCENT_CHARACTER{ '%' };                               // 수치 뒤 퍼센트 기호
        constexpr std::string_view POLISH_HEADER{ "연마 효과" };              // 악세서리 옵션 상자 제목
        constexpr std::string_view BRACELET_HEADER{ "팔찌 효과" };            // 팔찌 스탯 · 옵션 상자 제목
        constexpr std::string_view BRACELET_LOCKED_MARK{ "bracelet_locked" };       // 고정 줄 (스탯 · 고정 옵션)
        constexpr std::string_view BRACELET_CHANGEABLE_MARK{ "bracelet_changeable" };   // 바꿀 수 있는 옵션 줄의 시작
        constexpr std::string_view LEVEL_MARK{ "Lv." };  // 레벨 표시 글자
        constexpr char BRACKET_OPEN{ '[' };  // 대괄호 시작 글자
        constexpr char BRACKET_CLOSE{ ']' };  // 대괄호 끝 글자
        constexpr char BREAK_TAG_FIRST{ 'b' };  // 줄바꿈 태그 첫 글자
        constexpr char BREAK_TAG_SECOND{ 'r' };  // 줄바꿈 태그 둘째 글자
        constexpr char SPACE_CHARACTER{ ' ' };  // 공백 글자
        constexpr std::string_view STAT_NUMBER_PREFIX{ "+" };  // 스탯 수치 앞 기호

        constexpr std::array<std::string_view, Models::AVATAR_SLOT_COUNT> AVATAR_SLOT_NAMES{ "무기 아바타", "머리 아바타", "상의 아바타", "하의 아바타" };  // 아바타 부위 이름 표
        constexpr std::string_view AVATAR_LEGENDARY{ "전설" };  // 전설 아바타 등급 이름
        constexpr std::string_view AVATAR_EPIC{ "영웅" };  // 영웅 아바타 등급 이름
        constexpr std::string_view ARK_CORE_ANCIENT{ "고대" };  // 고대 코어 등급 이름
        constexpr std::string_view ARK_CORE_RELIC{ "유물" };  // 유물 코어 등급 이름
        constexpr std::string_view ARK_CORE_LEGENDARY{ "전설" };  // 전설 코어 등급 이름
        constexpr std::array<std::string_view, Models::PASSIVE_TREE_COUNT> PASSIVE_TREE_NAMES{ "진화", "깨달음", "도약" };  // 아크 패시브 트리 이름 표
        constexpr std::size_t PASSIVE_RANK_INDEX{ 0 };      // "6랭크 27레벨"에서 첫 번째 숫자
        constexpr std::size_t PASSIVE_LEVEL_INDEX{ 1 };     // 두 번째 숫자
        constexpr int EMPTY_TREE_RANK{ 0 };                 // 설명이 빈 트리(랭크 미달성)의 랭크 (lopec 해석)
        constexpr int EMPTY_TREE_LEVEL{ 1 };                // 설명이 빈 트리의 레벨 (lopec 해석)

        // ASCII 대소문자를 가리지 않고 같은 글자인지 (태그 이름 비교용)
        [[nodiscard]] bool EqualsIgnoreCase(char first, char second) noexcept
        {  // 대소문자 구분 없이 글자 비교
            return std::tolower(static_cast<unsigned char>(first)) == std::tolower(static_cast<unsigned char>(second));  // 소문자로 바꿔 비교
        }

        // HTML 글자를 <br> 기준으로 줄 나누기 (대소문자 무시, 태그는 그대로 남김)
        [[nodiscard]] std::vector<std::string> SplitBreaks(std::string_view html)
        {  // 줄바꿈 태그로 나눈 줄 목록
            std::vector<std::string> lines;  // 나눈 줄 목록
            std::string current;  // 현재 줄
            for (std::size_t position{}; position < html.size();)  // 글자를 하나씩 순회 (위치는 안에서 증가)
            {
                const bool isBreak{ html[position] == TAG_OPEN && position + 2 < html.size()  // 줄바꿈 태그 시작인지 확인
                    && EqualsIgnoreCase(html[position + 1], BREAK_TAG_FIRST) && EqualsIgnoreCase(html[position + 2], BREAK_TAG_SECOND) };
                const std::size_t close{ isBreak ? html.find(TAG_CLOSE, position) : std::string_view::npos };  // 태그 끝 위치 (줄바꿈 태그일 때만)
                if (close != std::string_view::npos)  // 줄바꿈 태그이면
                {
                    lines.push_back(std::move(current));  // 현재 줄 확정
                    current.clear();  // 현재 줄 비움
                    position = close + 1;  // 태그 뒤로 위치 이동
                    continue;  // 다음으로
                }
                current.push_back(html[position++]);  // 일반 글자는 현재 줄에 추가
            }
            lines.push_back(std::move(current));  // 마지막 줄 확정
            return lines;  // 줄 목록 반환
        }

        // 툴팁 JSON 트리에서 제목에 header가 들어 있는 상자의 본문 글자 찾기 (없으면 nullptr)
        [[nodiscard]] const std::string* FindPartBody(const JSONValue& node, std::string_view header)
        {  // 객체 노드이면
            if (node.GetType() == JSONType::OBJECT_VALUE)
            {
                const JSONValue& title{ node.FindMember(KEY_ELEMENT_HEADER) };  // 부위 제목 글자
                const JSONValue& body{ node.FindMember(KEY_ELEMENT_BODY) };  // 부위 내용 글자
                if (title.GetType() == JSONType::STRING_VALUE && body.GetType() == JSONType::STRING_VALUE && title.GetString().contains(header)) return &body.GetString();  // 제목이 맞으면 내용 반환
                for (const JSONValue& value : node.GetValues())  // 하위 값을 순회
                {
                    if (const std::string* found{ FindPartBody(value, header) }; found != nullptr) return found;  // 하위에서 찾았으면 반환
                }
            }
            else if (node.GetType() == JSONType::ARRAY_VALUE)  // 배열 노드이면
            {
                for (const JSONValue& item : node.GetItems())  // 원소를 순회
                {
                    if (const std::string* found{ FindPartBody(item, header) }; found != nullptr) return found;  // 원소에서 찾았으면 반환
                }
            }
            return nullptr;  // 찾지 못함
        }

        // 한 줄 HTML → 태그를 지우고 공백을 정리한 글자
        [[nodiscard]] std::string CleanLine(std::string_view htmlLine)
        {  // 태그를 지우고 공백을 정리한 줄 반환
            return Models::NormalizeSpaces(StripTags(htmlLine));
        }

        // 글자 속 "[이름] ... Lv.N" 모음 (스톤 각인 · 아크 그리드 젬 효과), Lv.이 따라오지 않는 대괄호는 건너뜀
        [[nodiscard]] std::vector<std::pair<std::string, int>> FindLevelTags(std::string_view text)
        {  // 태그 목록
            std::vector<std::pair<std::string, int>> tags;  // 대괄호 시작을 차례로 찾음
            for (std::size_t open{ text.find(BRACKET_OPEN) }; open != std::string_view::npos; open = text.find(BRACKET_OPEN, open + 1))
            {
                const std::size_t close{ text.find(BRACKET_CLOSE, open) };  // 짝이 되는 대괄호 끝
                if (close == std::string_view::npos) break;  // 짝이 없으면 끝

                std::size_t position{ close + 1 };  // 대괄호 뒤 위치
                while (position < text.size() && (text[position] == SPACE_CHARACTER || text[position] == TEXT_SEPARATOR)) ++position;  // 공백 · 줄 구분 글자 건너뜀
                if (!text.substr(position).starts_with(LEVEL_MARK)) continue;  // "Lv."이 이어지지 않으면 건너뜀

                const std::optional<long long> level{ ReadNumberAt(text, position + LEVEL_MARK.size()) };  // 레벨 숫자 읽기
                if (!level.has_value()) continue;  // 숫자가 없으면 건너뜀
                tags.emplace_back(Models::NormalizeSpaces(text.substr(open + 1, close - open - 1)), static_cast<int>(*level));  // 대괄호 안 이름과 레벨을 목록에 추가
            }
            return tags;  // 태그 목록 반환
        }

        // 글자 속 정수들을 순서대로 ("6랭크 27레벨" → 6, 27)
        [[nodiscard]] std::vector<int> ExtractIntegers(std::string_view text)
        {  // 숫자 목록
            std::vector<int> numbers;  // 글자를 순회
            for (std::size_t position{}; position < text.size();)
            {
                if (text[position] < '0' || text[position] > '9')  // 숫자가 아니면
                {
                    ++position;  // 다음 글자로
                    continue;
                }
                int value{};  // 현재 숫자
                for (; position < text.size() && text[position] >= '0' && text[position] <= '9'; ++position) value = value * DECIMAL_BASE + (text[position] - '0');  // 연속된 숫자를 읽어 값 누적
                numbers.push_back(value);  // 숫자 목록에 추가
            }
            return numbers;  // 숫자 목록 반환
        }

        // 팔찌 스탯 줄 읽기 ("신속 +119" → 신속, 119, 스탯 종류가 아니면 nullopt)
        [[nodiscard]] std::optional<Models::BraceletStat> ParseBraceletStat(std::string_view text)
        {
            const std::size_t space{ text.find(SPACE_CHARACTER) };  // 첫 공백 위치
            if (space == std::string_view::npos) return std::nullopt;  // 공백이 없으면 값 없음

            const std::string_view type{ text.substr(0, space) };  // 스탯 종류 글자
            const std::span<const std::string_view> types{ Models::CombatCoefficientData::GetBraceletStatTypes() };  // 허용되는 팔찌 스탯 종류 목록
            if (Models::IsNone(type) || std::ranges::find(types, type) == types.end()) return std::nullopt;  // 종류가 없음이거나 목록에 없으면 값 없음

            const std::string_view rest{ text.substr(space + 1) };  // 종류 뒤 나머지 글자
            if (!rest.starts_with(STAT_NUMBER_PREFIX)) return std::nullopt;  // "+" 기호가 없으면 값 없음
            const std::optional<long long> value{ ReadNumberAt(rest, 0) };  // 수치 읽기
            if (!value.has_value()) return std::nullopt;  // 수치가 없으면 값 없음
            return Models::BraceletStat{ std::string{ type }, static_cast<int>(*value) };  // 종류와 수치로 팔찌 스탯 반환
        }

        // 악세서리: 연마 효과 상자의 줄들 → 옵션 문구
        void ParseAccessoryOptions(const JSONValue& tooltip, Models::AccessoryState& accessory)
        {
            const std::string* body{ FindPartBody(tooltip, POLISH_HEADER) };  // 연마 효과 부위 내용 찾기
            if (body == nullptr) return;  // 없으면 중단

            std::size_t index{};  // 옵션 칸 번호
            for (const std::string& line : SplitBreaks(*body))  // 줄바꿈 태그로 나눈 줄을 순회
            {
                const std::string text{ CleanLine(line) };  // 태그를 지운 줄
                if (text.empty()) continue;  // 빈 줄이면 건너뜀
                if (index >= Models::ACCESSORY_OPTION_COUNT) break;  // 옵션 칸이 가득 차면 중단
                accessory.m_options[index++] = text;  // 줄 글자를 옵션으로 기록
            }
        }

        // 팔찌: 고정 줄(스탯 · 고정 옵션)과 바꿀 수 있는 옵션 줄 → 스탯 4개 · 옵션 4개
        // 옵션 하나가 문장 여러 줄이면 아이콘이 없는 다음 줄들을 앞 옵션에 이어 붙임
        void ParseBracelet(const JSONValue& tooltip, Models::ExtraState& extra)  // 팔찌 효과 해석
        {
            const std::string* body{ FindPartBody(tooltip, BRACELET_HEADER) };  // 팔찌 효과 부위 내용 찾기
            if (body == nullptr) return;  // 없으면 중단

            const bool hasMarks{ body->contains(BRACELET_LOCKED_MARK) || body->contains(BRACELET_CHANGEABLE_MARK) };  // 잠금 · 변경 가능 표식이 있는지
            std::size_t statIndex{};  // 스탯 줄 번호
            std::size_t optionIndex{};  // 옵션 줄 번호
            std::string option;  // 이어 붙이는 중인 옵션 글자
            bool isInOption{};  // 옵션을 쓰는 중인지

            const auto flushOption{ [&]  // 쌓인 옵션을 확정하는 람다
            {
                if (isInOption && optionIndex < Models::BRACELET_OPTION_COUNT) extra.m_braceletOptions[optionIndex++] = Models::RemoveSpaces(option);  // 옵션 중이고 칸이 남아 있으면 공백을 지워 기록
                option.clear();  // 옵션 글자 비움
                isInOption = false;  // 옵션 중 표시 해제
            } };

            for (const std::string& line : SplitBreaks(*body))  // 줄을 순회
            {
                const std::string text{ CleanLine(line) };  // 태그를 지운 줄
                if (text.empty()) continue;  // 빈 줄이면 건너뜀

                const bool isLocked{ line.contains(BRACELET_LOCKED_MARK) };  // 잠금 표식 줄인지
                const bool isChangeable{ line.contains(BRACELET_CHANGEABLE_MARK) };  // 변경 가능 표식 줄인지
                if (hasMarks && !isLocked && !isChangeable)  // 표식이 있는 팔찌에서 표식 없는 줄이면
                {
                    if (isInOption) option += SPACE_CHARACTER + text;   // 앞 옵션의 다음 문장
                    continue;  // 이어지는 줄이므로 다음으로
                }

                flushOption();  // 이전 옵션 확정
                if (!isChangeable)  // 변경 가능 줄이 아니면
                {
                    if (const std::optional<Models::BraceletStat> stat{ ParseBraceletStat(text) }; stat.has_value())  // 스탯 줄로 해석 시도
                    {
                        if (statIndex < Models::BRACELET_STAT_COUNT) extra.m_braceletStats[statIndex++] = *stat;  // 스탯 칸이 남아 있으면 기록
                        continue;  // 다음 줄로
                    }
                }
                option = text;  // 새 옵션 시작
                isInOption = true;  // 옵션 중 표시
            }
            flushOption();  // 마지막 옵션 확정
        }

        // 스톤: 툴팁 글자의 "[각인] Lv.N" 앞 두 줄 (세 번째는 감소 각인)
        void ParseStone(std::string_view strippedText, Models::ExtraState& extra)  // 어빌리티 스톤 해석
        {
            const std::vector<std::pair<std::string, int>> tags{ FindLevelTags(strippedText) };  // "[이름] Lv.N" 태그 목록
            for (std::size_t index{}; index < Models::STONE_LINE_COUNT && index < tags.size(); ++index)  // 스톤 줄 수만큼 순회
            {
                extra.m_stone[index] = Models::StoneLine{ tags[index].first, tags[index].second };  // 이름과 레벨로 스톤 줄 기록
            }
        }

        // 아크 그리드 코어 등급 글자 → 열거형 (모르면 영웅)
        [[nodiscard]] Models::ArkCoreGrade ToCoreGrade(std::string_view grade) noexcept  // 코어 등급 글자를 열거형으로 변환
        {
            if (grade == ARK_CORE_ANCIENT) return Models::ArkCoreGrade::CORE_ANCIENT;  // 고대 등급
            if (grade == ARK_CORE_LEGENDARY) return Models::ArkCoreGrade::CORE_LEGENDARY;  // 전설 등급
            if (grade == ARK_CORE_RELIC) return Models::ArkCoreGrade::CORE_RELIC;  // 유물 등급
            return Models::ArkCoreGrade::CORE_EPIC;  // 그 밖에는 영웅 등급
        }

        // 툴팁 JSON 글자를 읽어 트리로 (읽지 못하면 null 값)
        [[nodiscard]] JSONValue ReadTooltipTree(std::string_view tooltip)  // 툴팁 글자를 JSON 트리로 변환
        {
            try
            {
                return ParseJSONText(tooltip);  // JSON으로 해석해 반환
            }
            catch (const std::exception&)  // 해석 실패 시
            {
                return JSONValue{};  // 빈 JSON 반환
            }
        }

        // 툴팁 트리의 글자를 이어 붙이고 태그를 지움
        [[nodiscard]] std::string ReadTooltipText(std::string_view tooltip)  // 툴팁 글자만 뽑아냄
        {
            std::string joined;  // 이어 붙인 글자
            std::optional<int> unusedQuality;  // 품질 (사용 안 함)
            CollectTooltip(ReadTooltipTree(tooltip), joined, unusedQuality);  // 툴팁 트리에서 글자 수집
            return StripTags(joined);  // 태그를 지워 반환
        }

        // 장비 한 개의 악세서리 · 팔찌 · 스톤 · 완갑 정보를 extra에 반영 (earringCount · ringCount = 지금까지 읽은 귀걸이 · 반지 수)
        void ParseEquipmentExtra(const Models::GearItem& item, std::string_view tooltip, Models::ExtraState& extra, std::size_t& earringCount, std::size_t& ringCount)  // 장비 하나에서 추가 장착 정보 읽기
        {
            if (item.m_type == VAMBRACE_TYPE)  // 완갑이면
            {
                extra.m_vambraceLevel = item.m_enhancement;  // 완갑 재련 단계 기록
                extra.m_vambraceGrade = Models::CombatExtra::ParseVambraceGrade(item.m_grade);  // 완갑 등급 기록 (영웅 · 전설 · 유물 · 고대, 모르면 없음)

                // 한계 해방 추가 효과 "추가 효과 … 기본 공격력 +2.00%" (없으면 등급 표로 추정하도록 비워 둠)
                const std::string text{ ReadTooltipText(tooltip) };  // 태그를 지운 툴팁 글자
                const std::size_t header{ text.find(VAMBRACE_EXTRA_HEADER) };  // 추가 효과 상자 위치
                if (header == std::string::npos) return;  // 추가 효과가 없으면 처리 끝
                const std::size_t label{ text.find(VAMBRACE_BASE_ATTACK_TEXT, header) };  // 추가 효과 안의 기본 공격력 줄 위치
                if (label == std::string::npos) return;  // 기본 공격력 줄이 없으면 처리 끝
                const std::size_t plus{ text.find(PLUS_CHARACTER, label) };  // 수치 앞 덧셈 기호 위치
                const std::size_t percent{ plus == std::string::npos ? std::string::npos : text.find(PERCENT_CHARACTER, plus) };  // 수치 뒤 퍼센트 기호 위치
                if (percent == std::string::npos) return;  // 퍼센트 수치가 아니면 처리 끝
                const std::optional<double> rate{ ParseDecimal(std::string_view{ text }.substr(plus + 1, percent - plus - 1)) };  // 퍼센트 수치 읽기
                if (rate.has_value()) extra.m_vambraceGradeRate = *rate / PERCENT_DIVISOR;  // 퍼센트를 비율로 바꿔 기록
                return;  // 처리 끝
            }

            std::optional<std::size_t> accessorySlot;  // 악세서리 슬롯 번호 (악세서리가 아니면 없음)
            if (item.m_type == ACCESSORY_TYPE_NECKLACE) accessorySlot = ACCESSORY_NECKLACE_SLOT;  // 목걸이는 고정 슬롯
            else if (item.m_type == ACCESSORY_TYPE_EARRING && earringCount < ACCESSORY_EARRING_COUNT) accessorySlot = ACCESSORY_FIRST_EARRING_SLOT + earringCount++;  // 귀걸이는 순서대로 슬롯 배정
            else if (item.m_type == ACCESSORY_TYPE_RING && ringCount < ACCESSORY_RING_COUNT) accessorySlot = ACCESSORY_FIRST_RING_SLOT + ringCount++;  // 반지는 순서대로 슬롯 배정

            if (accessorySlot.has_value())  // 악세서리이면
            {
                Models::AccessoryState& accessory{ extra.m_accessories[*accessorySlot] };  // 해당 슬롯 참조
                accessory.m_mainStat = item.m_mainStat;  // 주스탯 기록
                ParseAccessoryOptions(ReadTooltipTree(tooltip), accessory);  // 연마 효과 옵션 해석
            }
            else if (item.m_type == BRACELET_TYPE)  // 팔찌이면
            {
                ParseBracelet(ReadTooltipTree(tooltip), extra);  // 팔찌 효과 해석
            }
            else if (item.m_type == STONE_TYPE)  // 어빌리티 스톤이면
            {
                ParseStone(ReadTooltipText(tooltip), extra);  // 스톤 각인 해석
            }
        }

        // 아바타: 장착 중인 아바타(IsInner) 중 부위별 가장 높은 등급 (덧입기 이벤트 아바타는 능력치가 없어서 제외)
        void ParseAvatars(const JSONValue& avatars, Models::ExtraState& extra)
        {  // 아바타 목록을 순회
            for (const JSONValue& avatar : avatars.GetItems())
            {
                const JSONValue& inner{ avatar.FindMember(KEY_IS_INNER) };  // 안쪽 슬롯 여부
                if (inner.GetType() == JSONType::BOOL_VALUE && !inner.GetBool()) continue;  // 겉모습 아바타면 건너뜀

                const auto slot{ std::ranges::find(AVATAR_SLOT_NAMES, std::string_view{ avatar.FindMember(KEY_TYPE).GetString() }) };  // 아바타 부위 찾기
                if (slot == AVATAR_SLOT_NAMES.end()) continue;  // 모르는 부위면 건너뜀

                const std::string_view grade{ avatar.FindMember(KEY_GRADE).GetString() };  // 아바타 등급 글자
                Models::AvatarGrade parsed{ Models::AvatarGrade::AVATAR_NONE };  // 등급 기본값은 없음
                if (grade == AVATAR_LEGENDARY) parsed = Models::AvatarGrade::AVATAR_LEGENDARY;  // 전설 등급
                else if (grade == AVATAR_EPIC) parsed = Models::AvatarGrade::AVATAR_EPIC;  // 영웅 등급

                Models::AvatarGrade& saved{ extra.m_avatars[static_cast<std::size_t>(std::distance(AVATAR_SLOT_NAMES.begin(), slot))] };  // 해당 부위 저장 칸
                saved = std::max(saved, parsed);  // 더 높은 등급을 유지
            }
        }

        // 각인서 5개: ArkPassiveEffects의 이름 · 등급 · 레벨(각인서 레벨)
        void ParseBooks(const JSONValue& engraving, Models::ExtraState& extra)
        {
            std::size_t index{};  // 각인서 번호
            for (const JSONValue& effect : engraving.FindMember(KEY_ARK_PASSIVE_EFFECTS).GetItems())  // 아크 패시브 각인 효과를 순회
            {
                if (index >= Models::BOOK_COUNT) break;  // 칸이 가득 차면 중단
                extra.m_books[index++] = Models::BookState{ effect.FindMember(KEY_NAME).GetString(), effect.FindMember(KEY_GRADE).GetString() == AVATAR_LEGENDARY,  // 이름 · 전설 여부 · 레벨로 각인서 기록
                    static_cast<int>(effect.FindMember(KEY_LEVEL).GetNumber()) };
            }
        }

        // 아크 패시브: 진화 · 깨달음 · 도약의 "N랭크 M레벨" → 트리별 랭크 · 레벨 (설명이 비면 랭크 0 · 레벨 1, 표시 랭크는 세 트리 중 가장 낮은 값)
        void ParseArkPassive(const JSONValue& arkPassive, Models::ExtraState& extra)
        {
            std::optional<int> lowestRank;  // 가장 낮은 랭크
            for (const JSONValue& point : arkPassive.FindMember(KEY_POINTS).GetItems())  // 패시브 포인트를 순회
            {
                const auto tree{ std::ranges::find(PASSIVE_TREE_NAMES, std::string_view{ point.FindMember(KEY_NAME).GetString() }) };  // 트리 이름 찾기
                if (tree == PASSIVE_TREE_NAMES.end()) continue;  // 모르는 트리면 건너뜀

                const auto treeIndex{ static_cast<std::size_t>(std::distance(PASSIVE_TREE_NAMES.begin(), tree)) };  // 트리 번호
                const std::vector<int> numbers{ ExtractIntegers(point.FindMember(KEY_DESCRIPTION).GetString()) };  // 설명에서 숫자 추출
                const bool hasRank{ numbers.size() > PASSIVE_LEVEL_INDEX };  // "N랭크 M레벨"이 모두 있는지
                const int rank{ hasRank ? numbers[PASSIVE_RANK_INDEX] : EMPTY_TREE_RANK };  // 트리 랭크 (없으면 랭크 미달성)
                extra.m_passive.m_levels[treeIndex] = hasRank ? numbers[PASSIVE_LEVEL_INDEX] : EMPTY_TREE_LEVEL;  // 트리 레벨 기록 (없으면 최소 레벨)
                extra.m_passive.m_treeRanks[treeIndex] = rank;  // 트리 랭크 기록
                lowestRank = std::min(lowestRank.value_or(rank), rank);  // 가장 낮은 랭크 갱신
            }
            if (lowestRank.has_value()) extra.m_passive.m_rank = *lowestRank;  // 랭크가 있으면 기록
        }

        // 아크 그리드: 코어 6개(등급 · 포인트) + 젬 효과 레벨 합
        void ParseArkGrid(const JSONValue& arkGrid, Models::ExtraState& extra)
        {
            for (const JSONValue& slot : arkGrid.FindMember(KEY_SLOTS).GetItems())  // 코어 슬롯을 순회
            {
                const auto index{ static_cast<std::size_t>(slot.FindMember(KEY_INDEX).GetNumber()) };  // 코어 번호
                if (index >= Models::ARK_CORE_COUNT) continue;  // 범위 밖이면 건너뜀
                // 코어 이름: "혼돈의 해 코어 : 재빠른 공격" → ":" 뒤 글자 (없으면 빈 글자)
                const std::string coreTitle{ slot.FindMember(KEY_NAME).GetString() };  // 코어 제목 글자
                const std::size_t nameBegin{ coreTitle.find(CORE_NAME_SEPARATOR) };  // 이름 구분 글자 위치
                const std::string coreName{ nameBegin == std::string::npos ? std::string{} : Models::NormalizeSpaces(std::string_view{ coreTitle }.substr(nameBegin + CORE_NAME_SEPARATOR.size())) };  // 구분 글자 뒤 이름 (없으면 빈 글자)
                extra.m_cores[index] = Models::ArkCoreState{ ToCoreGrade(slot.FindMember(KEY_GRADE).GetString()), static_cast<int>(slot.FindMember(KEY_POINT).GetNumber()), coreName };  // 등급 · 포인트 · 이름으로 코어 기록

                for (const JSONValue& gem : slot.FindMember(KEY_GEMS).GetItems())  // 코어에 장착된 젬을 순회
                {
                    for (const auto& [name, level] : FindLevelTags(ReadTooltipText(gem.FindMember(KEY_TOOLTIP).GetString())))  // 젬 툴팁의 효과 이름과 레벨 태그를 순회
                    {
                        const auto effect{ std::ranges::find(Models::ARK_GEM_EFFECT_NAMES, std::string_view{ name }) };  // 효과 종류 찾기
                        if (effect != Models::ARK_GEM_EFFECT_NAMES.end()) extra.m_arkGemLevels[static_cast<std::size_t>(std::distance(Models::ARK_GEM_EFFECT_NAMES.begin(), effect))] += level;  // 알려진 효과면 레벨 합산
                    }
                }
            }
        }
    }

    void ParseGearTooltip(std::string_view tooltip, bool isWeapon, Models::GearItem& item)
    {
        // 툴팁은 JSON 문자열 → 트리로 읽어 글자를 이어 붙임 (읽지 못하면 원문 그대로 + 정규식으로 품질)
        std::string joined;  // 툴팁 글자 수집용 변수
        std::optional<int> quality;  // 품질 변수
        try  // JSON 해석 시도
        {
            CollectTooltip(ParseJSONText(tooltip), joined, quality);  // 글자와 품질 수집
        }
        catch (const std::exception&)  // 해석 실패 시
        {
            joined = std::string{ tooltip };  // 원본 글자를 그대로 사용
            std::match_results<std::string_view::const_iterator> match;  // 정규식 결과
            if (std::regex_search(tooltip.begin(), tooltip.end(), match, std::regex{ QUALITY_TEXT_PATTERN.begin(), QUALITY_TEXT_PATTERN.end() }))  // 정규식으로 품질 찾기
            {
                quality = ParseDecimal(match[QUALITY_GROUP].str()).transform([](double value) { return static_cast<int>(value); });  // 찾은 품질 기록
            }
        }
        if (quality.has_value() && *quality <= NO_QUALITY) quality.reset();  // 품질이 -1 이하면 없음 처리
        item.m_quality = quality;  // 품질 기록

        const std::string text{ StripTags(joined) };  // 태그를 지운 글자
        item.m_advancedEnhancement = static_cast<int>(ReadNumberAfter(text, ADVANCED_MARKER, 0, text.size()).value_or(0));  // 상급 재련 단계 읽기

        // 주스탯: 기본 효과 구간에서 힘 · 민첩 · 지능 중 처음 나오는 것
        if (const auto basic{ FindBasicEffect(text) }; basic.has_value())  // 기본 효과 구간을 찾으면
        {
            std::size_t firstPosition{ std::string::npos };  // 가장 앞선 표식 위치
            for (const std::string_view marker : MAIN_STAT_MARKERS)  // 주스탯 표식(힘 · 민첩 · 지능)을 순회
            {
                const std::size_t found{ text.find(marker, basic->first) };  // 구간 안에서 표식 찾기
                if (found < basic->second && found < firstPosition)  // 더 앞선 표식이면
                {
                    firstPosition = found;  // 위치 갱신
                    item.m_mainStat = static_cast<double>(ReadNumberAt(text, found + marker.size()).value_or(0));  // 주스탯 기록
                }
            }
            if (isWeapon) item.m_weaponAttack = static_cast<double>(ReadNumberAfter(text, WEAPON_ATTACK_MARKER, basic->first, basic->second).value_or(0));  // 무기이면 무기 공격력 기록
        }

        // 팔찌: 주스탯이 기본 효과가 아니라 팔찌 효과 묶음에 "힘 +12345"처럼 나옴 → 툴팁 전체의 힘 · 민첩 · 지능 줄을 모두 더함
        // ⚠️ 실제 API 응답으로 확인하지 않은 규칙 (팔찌 툴팁 형식이 다르면 S0가 조금 달라짐)
        if (item.m_type == BRACELET_TYPE)  // 팔찌이면
        {
            double braceletStat{};  // 팔찌 주스탯 합계
            for (const std::string_view marker : MAIN_STAT_MARKERS)  // 주스탯 표식을 순회
            {
                for (std::size_t found{ text.find(marker) }; found != std::string::npos; found = text.find(marker, found + marker.size()))  // 툴팁 전체에서 표식이 나오는 곳마다
                {
                    braceletStat += static_cast<double>(ReadNumberAt(text, found + marker.size()).value_or(0));  // 수치를 합산
                }
            }
            item.m_mainStat = braceletStat;  // 합계 기록
        }

        // 기본 효과 구간을 못 찾은 무기는 툴팁 전체에서 무기 공격력 찾기
        if (isWeapon && item.m_weaponAttack <= 0.0)  // 무기 공격력을 못 읽었으면
        {
            item.m_weaponAttack = static_cast<double>(ReadNumberAfter(text, WEAPON_ATTACK_MARKER, 0, text.size()).value_or(0));  // 툴팁 전체에서 무기 공격력 다시 찾기
        }
    }

    Models::CharacterArmory ParseArmory(const JSONValue& root)  // 무기고 응답 해석
    {
        // 없는 이름은 200 + null (또는 프로필 null)
        const JSONValue& profile{ root.FindMember(KEY_PROFILE) };  // 프로필 노드
        if (root.GetType() != JSONType::OBJECT_VALUE || profile.GetType() != JSONType::OBJECT_VALUE)  // 응답이나 프로필이 객체가 아니면
        {
            throw std::runtime_error{ std::string{ NOT_FOUND_MESSAGE } };  // 캐릭터를 찾을 수 없다는 오류
        }
        // 캐릭터를 찾은 응답인데 꼭 있어야 할 묶음이 없으면 형식 변경으로 신고 (값이 조용히 0이 되지 않게)
        if (profile.FindMember(KEY_COMBAT_POWER).GetType() == JSONType::NULL_VALUE)  // 전투력 필드가 사라짐
            Core::APIFormatMonitor::Report("무기고", "ArmoryProfile.CombatPower 없음");  // 신고 (실행 중 한 번)
        if (root.FindMember(KEY_EQUIPMENT).GetType() != JSONType::ARRAY_VALUE)  // 장비 목록이 배열이 아님
            Core::APIFormatMonitor::Report("무기고", "ArmoryEquipment 배열 없음");  // 신고

        Models::CharacterArmory armory;  // 해석 결과
        armory.m_characterName = profile.FindMember(KEY_CHARACTER_NAME).GetString();  // 캐릭터 이름
        armory.m_className = profile.FindMember(KEY_CHARACTER_CLASS_NAME).GetString();  // 직업 이름
        armory.m_itemAvgLevel = ReadFlexibleNumber(profile.FindMember(KEY_ITEM_AVG_LEVEL)).value_or(0.0);  // 평균 아이템 레벨
        armory.m_combatPower = ReadFlexibleNumber(profile.FindMember(KEY_COMBAT_POWER));  // API 전투력
        if (armory.m_combatPower.has_value() && *armory.m_combatPower <= 0.0) armory.m_combatPower.reset();  // 0 이하면 없음 처리

        // 장비: 모든 항목의 주스탯을 더하고(S0), 시뮬레이터 부위만 따로 보관
        std::size_t earringCount{};  // 귀걸이 개수
        std::size_t ringCount{};  // 반지 개수
        for (const JSONValue& entry : root.FindMember(KEY_EQUIPMENT).GetItems())  // 장비 목록을 순회
        {
            Models::GearItem item;  // 장비 하나
            item.m_type = entry.FindMember(KEY_TYPE).GetString();  // 종류
            item.m_name = entry.FindMember(KEY_NAME).GetString();  // 이름
            item.m_grade = entry.FindMember(KEY_GRADE).GetString();  // 등급
            item.m_enhancement = ReadEnhancement(item.m_name);  // 재련 단계

            const std::optional<Models::GearPart> part{ Models::FindGearPart(item.m_type) };  // 전투력 시뮬레이터 대상 부위
            ParseGearTooltip(entry.FindMember(KEY_TOOLTIP).GetString(), part == Models::GearPart::PART_WEAPON, item);  // 툴팁에서 재련 · 품질 · 능력치 읽기
            armory.m_totalMainStat += item.m_mainStat;  // 주스탯 합계에 더함

            // 악세서리 · 팔찌 · 스톤 · 완갑은 따로 정리
            ParseEquipmentExtra(item, entry.FindMember(KEY_TOOLTIP).GetString(), armory.m_extra, earringCount, ringCount);  // 악세서리 · 팔찌 · 스톤 · 완갑 정보 읽기

            if (!part.has_value()) continue;  // 대상 부위가 아니면 다음으로
            std::optional<Models::GearItem>& slot{ armory.m_gears[Models::ToGearIndex(*part)] };  // 부위 칸 참조
            if (!slot.has_value()) slot = std::move(item);   // 같은 부위가 두 번 오면 첫 항목
        }

        // 보석 (없으면 빈 목록)
        for (const JSONValue& gem : root.FindMember(KEY_GEM).FindMember(KEY_GEMS).GetItems())  // 보석 목록을 순회
        {
            armory.m_gemLevels.push_back(static_cast<int>(gem.FindMember(KEY_LEVEL).GetNumber()));  // 보석 레벨 기록

            // 종류: 이름("10레벨 겁화의 보석", 태그 포함 가능)에 들어 있는 종류 글자 (없으면 빈 문자열)
            const std::string gemName{ StripTags(gem.FindMember(KEY_NAME).GetString()) };  // 보석 이름 (태그 제거)
            const auto kind{ std::ranges::find_if(GEM_KINDS, [&gemName](std::string_view candidate) { return gemName.contains(candidate); }) };  // 이름에 포함된 보석 종류 찾기
            armory.m_gemKinds.emplace_back(kind != GEM_KINDS.end() ? *kind : std::string_view{});  // 종류 기록 (모르면 빈 글자)
        }

        // 보석 이름에 종류가 없으면(예: "광휘의 보석") 스킬 효과 설명으로 구분 (피해 증가 = 겁화, 재사용 대기시간 감소 = 작열)
        for (const JSONValue& skill : root.FindMember(KEY_GEM).FindMember(KEY_EFFECTS).FindMember(KEY_SKILLS).GetItems())  // 보석 효과의 스킬 목록을 순회
        {
            const auto slot{ static_cast<std::size_t>(skill.FindMember(KEY_GEM_SLOT).GetNumber()) };  // 보석 칸 번호
            if (slot >= armory.m_gemKinds.size() || !armory.m_gemKinds[slot].empty()) continue;  // 범위 밖이거나 종류를 이미 알면 건너뜀

            for (const JSONValue& description : skill.FindMember(KEY_DESCRIPTION).GetItems())  // 스킬 설명을 순회
            {
                const std::string& text{ description.GetString() };  // 설명 글자
                if (text.contains(GEM_COOLDOWN_TEXT)) armory.m_gemKinds[slot] = GEM_COOLDOWN_KIND;  // 재사용 효과 설명이면 작열
                else if (text.contains(GEM_DAMAGE_TEXT)) armory.m_gemKinds[slot] = GEM_DAMAGE_KIND;  // 피해 효과 설명이면 겁화
            }
        }

        // 보석 기본 공격력 총합 (퍼센트 → 비율, 없으면 0)
        const std::string gemEffectText{ StripTags(root.FindMember(KEY_GEM).FindMember(KEY_EFFECTS).FindMember(KEY_DESCRIPTION).GetString()) };  // 보석 효과 설명 글자
        if (const std::size_t marker{ gemEffectText.find(GEM_BASE_ATTACK_MARKER) }; marker != std::string::npos)  // "기본 공격력 총합" 표식을 찾으면
        {
            const std::size_t colon{ gemEffectText.find(':', marker) };  // 콜론 위치
            if (colon != std::string::npos) armory.m_gemBaseAttackRate = ParseDecimal(std::string_view{ gemEffectText }.substr(colon + 1, gemEffectText.find('%', colon) - colon - 1)).value_or(0.0) / PERCENT_DIVISOR;  // 퍼센트 값을 비율로 변환해 기록
        }

        // 아바타 · 각인서 · 아크 패시브 · 아크 그리드 (응답에 없으면 기본값 그대로)
        ParseAvatars(root.FindMember(KEY_AVATARS), armory.m_extra);  // 아바타 해석
        ParseBooks(root.FindMember(KEY_ENGRAVING), armory.m_extra);  // 각인서 해석
        ParseArkPassive(root.FindMember(KEY_ARK_PASSIVE), armory.m_extra);  // 아크 패시브 해석
        ParseArkGrid(root.FindMember(KEY_ARK_GRID), armory.m_extra);  // 아크 그리드 해석
        return armory;  // 해석 결과 반환
    }
}
