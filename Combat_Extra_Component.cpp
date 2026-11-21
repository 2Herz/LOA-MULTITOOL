module LostArkMultiTool.Core.CombatExtraComponent;  // 추가 장착 배율 요소 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.CombatFactorComponent;  // 배율 요소 기반 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모듈 가져오기
import LostArkMultiTool.Models.CombatState;  // 전투력 상태 모듈 가져오기
import LostArkMultiTool.Models.CombatSupporterData;  // 서포터 계수 모듈 가져오기

namespace LostArkMultiTool::Core  // 코어 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        namespace Data = Models::CombatCoefficientData;  // 계수 데이터 네임스페이스 별칭
        namespace Supporter = Models::CombatSupporterData;  // 서포터 계수 네임스페이스 별칭

        // ===== 문구 · 상수 =====
        constexpr std::string_view WEAPON_ATTACK_PREFIX{ "무기 공격력 +" };   // 악세서리 무기 공격력 옵션 (식으로 계산)
        constexpr char PLUS_SIGN{ '+' };  // 옵션 문구에서 수치 앞의 덧셈 기호
        constexpr char PERCENT_SIGN{ '%' };  // 옵션 수치 뒤의 퍼센트 기호
        constexpr double PERCENT_SCALE{ 100.0 };  // 퍼센트를 비율로 바꾸는 배수
        constexpr double WEAPON_EXPONENT{ 0.5 };              // 무기 공격력 → 전투력은 제곱근
        constexpr double ATTACK_DIVISOR{ 6.0 };               // 공격력 = sqrt(주스탯 × 무기 공격력 / 6)
        constexpr double MIN_RATIO{ 1e-6 };                   // 나눗셈 보호
        constexpr double NO_CHANGE_FACTOR{ 1.0 };             // 바뀐 것이 없거나 계산할 수 없음
        constexpr int NO_STONE_LEVEL{ 0 };                    // 스톤 줄이 없거나 이어진 각인서가 없을 때 레벨
        constexpr double NO_STONE_RATIO{ 0.0 };               // 스톤 효과 · 보너스가 없을 때 값
        constexpr std::string_view AVATAR_LEGENDARY_KEY{ "전설" };  // 아바타 전설 등급 표 키
        constexpr std::string_view AVATAR_EPIC_KEY{ "영웅" };  // 아바타 영웅 등급 표 키
        constexpr std::string_view AVATAR_NONE_KEY{ "없음" };  // 아바타 없음 표 키
        constexpr std::size_t PASSIVE_ENLIGHTENMENT_TREE{ 1 };   // 아크 패시브 트리 번호: 0 진화(효과 없음), 1 깨달음, 2 도약
        constexpr std::size_t PASSIVE_LEAP_TREE{ 2 };  // 아크 패시브 도약 트리 번호
        constexpr double FULL_PASSIVE_RATIO{ 1.0 };           // 최대 랭크 · 최대 깨달음 구성의 표 배율 (이보다 작으면 최대가 아님)
        constexpr double NO_INTERACTION_LN{ 0.0 };            // 랭크 · 깨달음 상호작용이 없을 때 ln
        constexpr std::size_t PASSIVE_EVOLUTION_TREE{ 0 };    // 아크 패시브 진화 트리 번호 (서포터만 레벨 효과 있음)

        // 서포터 계수를 써야 하는 상태인지 (역할은 현재 · 목표가 같음)
        [[nodiscard]] bool IsSupporter(const Models::ExtraState& state) noexcept
        {
            return state.m_role == Models::CombatRole::ROLE_SUPPORTER;  // 역할이 서포터이면 참
        }

        // 무기 공격력 기준 W0 (모르면 0 → 무기 공격력 식은 계산하지 않음)
        [[nodiscard]] double GetWeaponBase(const Models::CombatState& state) noexcept  // 무기 공격력 기준값 조회
        {
            return Data::GetWeaponBase(state.m_weaponAttack);  // 상태의 무기 공격력으로 W0 계산
        }

        // 문구 끝의 "+숫자" 또는 "+숫자%" 읽기 (없으면 nullopt)
        struct OptionNumber  // 옵션 문구에서 읽은 수치 구조체
        {
            double m_value{};  // 수치
            bool m_isPercent{};  // 퍼센트 표기 여부
        };

        [[nodiscard]] std::optional<OptionNumber> ParseOptionNumber(std::string_view text) noexcept  // 문구 끝의 수치 읽기
        {
            const std::size_t plus{ text.rfind(PLUS_SIGN) };  // 마지막 덧셈 기호 위치
            if (plus == std::string_view::npos) return std::nullopt;  // 기호가 없으면 값 없음

            const std::string_view digits{ text.substr(plus + 1) };  // 기호 뒤 글자가 수치
            double value{};  // 수치를 담을 변수
            const auto [end, error]{ std::from_chars(digits.data(), digits.data() + digits.size(), value) };  // 숫자 변환 (끝 위치와 오류 코드 반환)
            if (error != std::errc{}) return std::nullopt;  // 변환에 실패하면 값 없음
            const bool isPercent{ end != digits.data() + digits.size() && *end == PERCENT_SIGN };  // 수치 뒤에 퍼센트 기호가 있는지 확인
            return OptionNumber{ value, isPercent };  // 수치와 퍼센트 여부 반환
        }

        // 악세서리 옵션 한 줄의 ln (표에 없으면 0)
        [[nodiscard]] double GetAccessoryLn(std::string_view option, double weaponBase, bool isEsther, bool isSupporter)  // 악세서리 옵션 한 줄의 ln 계산
        {
            if (Models::IsNone(option)) return 0.0;  // 옵션이 비어 있으면 0
            if (isSupporter)  // 서포터는 측정 표만 사용 (무기 공격력 옵션도 표 값)
            {
                const Data::KeyedValue* supporterFound{ Data::FindKeyed(Supporter::GetAccessoryOptions(), option) };  // 서포터 문구 표 조회
                return supporterFound != nullptr ? supporterFound->m_value : 0.0;  // 표에 없으면 효과 없음
            }

            if (option.starts_with(WEAPON_ATTACK_PREFIX))  // 무기 공격력 옵션이면 식으로 계산
            {
                const std::optional<OptionNumber> number{ ParseOptionNumber(option) };  // 옵션 문구의 수치 읽기
                if (!number.has_value()) return 0.0;  // 수치를 못 읽으면 0
                if (number->m_isPercent)  // 퍼센트 표기이면
                {
                    const Data::AccessoryFormula& formula{ Data::GetAccessoryFormula() };  // 악세서리 무기 공격력 식 계수
                    const double ratio{ isEsther ? formula.m_weaponRatioEsther : formula.m_weaponRatioReforge };  // 에스더 무기 여부에 따른 비율 선택
                    return WEAPON_EXPONENT * std::log1p(number->m_value / PERCENT_SCALE * ratio);  // 제곱근 영향을 반영한 ln(1 + 증가 비율 × 계수)
                }
                return weaponBase > 0.0 ? WEAPON_EXPONENT * std::log1p(number->m_value / weaponBase) : 0.0;  // 고정 수치이면 무기 기준값 대비 증가분으로 계산
            }

            const Data::KeyedValue* found{ Data::FindKeyed(Data::GetAccessoryOptions(), option) };  // 그 밖의 옵션은 문구 표에서 조회
            return found != nullptr ? found->m_value : 0.0;  // 표에 없으면 0
        }

        // 팔찌 옵션 한 줄의 ln (공백 없는 문구, 표에 없으면 0)
        [[nodiscard]] double GetBraceletLn(std::string_view option, double weaponBase, bool isSupporter)  // 팔찌 옵션 한 줄의 ln 계산
        {
            if (Models::IsNone(option)) return 0.0;  // 옵션이 비어 있으면 0
            if (isSupporter)  // 서포터는 측정 표만 사용
            {
                const Data::KeyedValue* supporterFound{ Data::FindKeyed(Supporter::GetBraceletOptions(), option) };  // 서포터 문구 표 조회
                return supporterFound != nullptr ? supporterFound->m_value : 0.0;  // 표에 없으면 효과 없음
            }

            for (const Data::BraceletWeaponLine& line : Data::GetBraceletWeaponLines())  // 무기 공격력 포함 줄 순회
            {
                if (line.m_key != option) continue;  // 문구가 다르면 건너뜀
                const double weaponPart{ weaponBase > 0.0 ? WEAPON_EXPONENT * std::log1p(line.m_weaponFlat / weaponBase) : 0.0 };  // 무기 공격력 고정값이 주는 ln
                return line.m_logAdd + weaponPart;  // 고정 ln 가산분 + 무기 공격력 ln 반환
            }
            const Data::KeyedValue* found{ Data::FindKeyed(Data::GetBraceletOptions(), option) };  // 그 밖의 옵션은 문구 표에서 조회
            return found != nullptr ? found->m_value : 0.0;  // 표에 없으면 0
        }

        // 치명 · 특화 · 신속 줄의 수치 합
        [[nodiscard]] double SumEffectiveStats(const Models::ExtraState& state)  // 치명 · 특화 · 신속 수치 합 계산
        {
            double sum{};  // 합계
            const std::span<const std::string_view> effectiveTypes{ IsSupporter(state) ? Supporter::GetBraceletEffectiveTypes() : Data::GetBraceletEffectiveTypes() };  // 역할별 유효 스탯 (서포터: 특화 · 신속)
            for (const Models::BraceletStat& line : state.m_braceletStats)  // 팔찌 스탯 줄 순회
            {
                if (std::ranges::find(effectiveTypes, line.m_type) != effectiveTypes.end()) sum += line.m_value;  // 유효 스탯 종류이면 합산
            }
            return sum;  // 합계 반환
        }

        // 스톤: 각인 이름이 비면 레벨 0
        [[nodiscard]] int GetStoneLevel(const Models::StoneLine& line) noexcept  // 스톤 레벨 (각인 이름이 비면 0)
        {
            return Models::IsNone(line.m_name) ? 0 : line.m_level;  // 이름이 없으면 0, 있으면 저장된 레벨
        }

        // 각인서 한 권의 기본 ln (유물: 레벨 0 ~ 4 보간, 전설: 유물 레벨 0에서 남은 단계만큼 배율을 직선으로 뺌)
        [[nodiscard]] double GetBookBaseLn(const Data::EngravingBook& entry, const Models::BookState& book)  // 각인서 한 권의 기본 ln 계산
        {
            const int level{ std::clamp(book.m_level, Models::MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL) };  // 0 ~ 최대로 제한한 레벨
            const double fraction{ static_cast<double>(level) / Data::ENGRAVING_BOOK_MAX_LEVEL };  // 레벨 비율 (0 ~ 1)
            if (!book.m_isLegendary) return entry.m_base + (entry.m_top - entry.m_base) * fraction;  // 유물이면 기본값과 최대값 사이를 레벨 비율로 보간

            const std::span<const std::string_view> flatNames{ Data::GetLegendFlatNames() };  // 전설이어도 유물 레벨 0과 같은 각인
            if (std::ranges::find(flatNames, entry.m_name) != flatNames.end()) return entry.m_base;  // 그런 각인이면 유물 레벨 0 값

            const double stepRatio{ std::expm1(entry.m_top - entry.m_base) / Data::ENGRAVING_BOOK_MAX_LEVEL };  // 유물 한 단계가 올리는 배율 (유물 레벨 0 대비)
            const int stepsBelow{ std::max(Data::GetLegendLevelOffset() - level, Models::MIN_VALUE) };  // 유물 레벨 0보다 낮은 단계 수 (전설 X4 = 0)
            return entry.m_base + std::log1p(-stepRatio * stepsBelow);  // 유물 레벨 0 배율에서 단계만큼 직선으로 빼서 ln 으로
        }

        // 각인 이름과 같은 스톤 줄의 가장 높은 레벨 (같은 이름 줄이 없으면 0)
        [[nodiscard]] int GetLinkedStoneLevel(const Models::ExtraState& state, std::string_view name)  // 각인서와 이어진 스톤 레벨 조회
        {
            int level{ NO_STONE_LEVEL };  // 이어진 스톤 레벨 (기본 0)
            for (const Models::StoneLine& line : state.m_stone)  // 스톤 두 줄 순회
            {
                if (line.m_name == name) level = std::max(level, GetStoneLevel(line));  // 이름이 같으면 더 높은 레벨을 유지
            }
            return std::clamp(level, NO_STONE_LEVEL, Data::STONE_MAX_LEVEL);  // 0 ~ 최대 레벨로 제한해 반환
        }

        // 스톤 레벨 → 레벨 1 효과 대비 배수 (표 범위 밖이면 0)
        [[nodiscard]] double GetStoneScale(int level) noexcept  // 스톤 레벨 배수 조회
        {
            const std::span<const double> scale{ Data::GetStoneLevelScale() };  // 레벨별 배수 표
            const std::size_t index{ static_cast<std::size_t>(std::max(level, NO_STONE_LEVEL)) };  // 표 위치 (음수 보호)
            return index < scale.size() ? scale[index] : NO_STONE_RATIO;  // 범위 안이면 배수, 밖이면 0
        }

        // 스톤 두 줄 레벨 합 보너스 ln (합이 기준 이상이면 보너스)
        [[nodiscard]] double GetStoneSumLn(const Models::ExtraState& state) noexcept  // 스톤 레벨 합 보너스 계산
        {
            int total{ NO_STONE_LEVEL };  // 두 줄 레벨 합
            for (const Models::StoneLine& line : state.m_stone) total += GetStoneLevel(line);  // 각 줄 레벨을 합산
            const Data::StoneSumBonus& bonus{ Data::GetStoneSumBonus() };  // 레벨 합 보너스 기준과 값
            return total >= bonus.m_minLevel ? bonus.m_ln : NO_STONE_RATIO;  // 기준 이상이면 보너스 ln, 아니면 0
        }

        // 서포터 각인서 한 권의 기본 ln (유물: 레벨 0 ~ 4 보간, 전설: 전설 레벨 4에서 유물과 같은 폭으로 단계만큼 뺌)
        [[nodiscard]] double GetSupporterBookLn(const Supporter::SupporterBook& entry, const Models::BookState& book) noexcept  // 서포터 각인서 기본 ln 계산
        {
            const int level{ std::clamp(book.m_level, Models::MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL) };  // 0 ~ 최대로 제한한 레벨
            const double fraction{ static_cast<double>(level) / Data::ENGRAVING_BOOK_MAX_LEVEL };  // 레벨 비율 (0 ~ 1)
            const double span{ entry.m_top - entry.m_base };  // 유물 레벨 0 → 최대 ln 폭
            if (!book.m_isLegendary) return entry.m_base + span * fraction;  // 유물이면 기본값과 최대값 사이를 보간
            return entry.m_legendTop - span * (1.0 - fraction);  // 전설이면 전설 최대값에서 남은 단계만큼 뺌
        }

        // 서포터 스톤 레벨 합 보너스
        [[nodiscard]] double GetSupporterStoneSumLn(const Models::ExtraState& state) noexcept  // 서포터 스톤 레벨 합 보너스 계산
        {
            int total{ NO_STONE_LEVEL };  // 두 줄 레벨 합
            for (const Models::StoneLine& line : state.m_stone) total += GetStoneLevel(line);  // 각 줄 레벨을 합산
            const Data::StoneSumBonus& bonus{ Supporter::GetStoneSumBonus() };  // 서포터 레벨 합 보너스
            return total >= bonus.m_minLevel ? bonus.m_ln : NO_STONE_RATIO;  // 기준 이상이면 보너스 ln, 아니면 0
        }

        // 서포터 각인서 5개 + 스톤 ln 합 (딜러와 같은 구조, 서포터 표 사용)
        [[nodiscard]] double GetSupporterEngravingLn(const Models::ExtraState& state)  // 서포터 각인서 · 스톤 ln 합 계산
        {
            double sum{ GetSupporterStoneSumLn(state) };  // 합계 (스톤 레벨 합 보너스부터 시작)
            for (std::size_t index{}; index < Models::BOOK_COUNT; ++index)  // 각인서 5개 순회
            {
                const Models::BookState& book{ state.m_books[index] };  // 현재 각인서
                if (Models::IsNone(book.m_name)) continue;  // 이름이 없으면 건너뜀
                const auto isSameName{ [&book](const Models::BookState& other) { return other.m_name == book.m_name; } };  // 앞 칸과 같은 이름인지 검사하는 람다
                if (std::any_of(state.m_books.begin(), state.m_books.begin() + static_cast<std::ptrdiff_t>(index), isSameName)) continue;  // 중복 이름은 건너뜀
                const std::span<const Supporter::SupporterBook> books{ Supporter::GetBooks() };  // 서포터 각인서 표
                const auto found{ std::ranges::find(books, book.m_name, &Supporter::SupporterBook::m_name) };  // 이름으로 표 항목 찾기
                if (found == books.end()) continue;  // 표에 없는 각인은 효과 없음
                const Data::KeyedValue* levelOne{ Data::FindKeyed(Supporter::GetStoneLevelOne(), book.m_name) };  // 같은 이름 스톤의 레벨 1 효과
                const double stoneRatio{ levelOne != nullptr ? levelOne->m_value * GetStoneScale(GetLinkedStoneLevel(state, book.m_name)) : NO_STONE_RATIO };  // 이어진 스톤 효과 비율
                sum += std::log(std::exp(GetSupporterBookLn(*found, book)) + stoneRatio);  // 각인서 배율 + 스톤 비율의 ln 합산
            }
            return sum;  // 합계 반환
        }

        // 각인서 5개 + 스톤의 ln 합 (각인서 효과 비율에 같은 이름 스톤 효과를 더한 뒤 ln, 없음 · 앞 칸과 같은 이름 · 효과 없는 각인은 0)
        [[nodiscard]] double GetEngravingLn(const Models::ExtraState& state)  // 각인서 · 스톤 ln 합 계산
        {
            if (IsSupporter(state)) return GetSupporterEngravingLn(state);  // 서포터는 서포터 표로 계산
            double sum{ GetStoneSumLn(state) };  // 합계 (스톤 레벨 합 보너스부터 시작)
            for (std::size_t index{}; index < Models::BOOK_COUNT; ++index)  // 각인서 5개 순회
            {
                const Models::BookState& book{ state.m_books[index] };  // 현재 각인서
                if (Models::IsNone(book.m_name)) continue;  // 이름이 없으면 건너뜀

                const auto isSameName{ [&book](const Models::BookState& other) { return other.m_name == book.m_name; } };  // 앞 칸과 같은 이름인지 검사하는 람다
                if (std::any_of(state.m_books.begin(), state.m_books.begin() + static_cast<std::ptrdiff_t>(index), isSameName)) continue;  // 앞 칸에 같은 이름이 있으면 중복이므로 건너뜀

                const std::span<const Data::EngravingBook> books{ Data::GetEngravingBooks() };  // 각인서 효과 표
                const auto found{ std::ranges::find(books, book.m_name, &Data::EngravingBook::m_name) };  // 이름으로 표 항목 찾기
                if (found == books.end() || (found->m_base == 0.0 && found->m_top == 0.0)) continue;  // 표에 없거나 효과가 없는 각인은 건너뜀

                const Data::KeyedValue* levelOne{ Data::FindKeyed(Data::GetStoneLevelOne(), book.m_name) };  // 같은 이름 스톤의 레벨 1 효과
                const double stoneRatio{ levelOne != nullptr ? levelOne->m_value * GetStoneScale(GetLinkedStoneLevel(state, book.m_name)) : NO_STONE_RATIO };  // 이어진 스톤 효과 비율
                sum += std::log(std::exp(GetBookBaseLn(*found, book)) + stoneRatio);  // 각인서 배율에 스톤 비율을 더한 값의 ln 을 합산
            }
            return sum;  // 합계 반환
        }

        // 아바타 등급 → 표 키
        [[nodiscard]] std::string_view GetAvatarKey(Models::AvatarGrade grade) noexcept  // 아바타 등급 → 표 키 변환
        {
            switch (grade)  // 등급별 분기
            {
                case Models::AvatarGrade::AVATAR_LEGENDARY:
                    return AVATAR_LEGENDARY_KEY;  // 전설 키 반환
                case Models::AvatarGrade::AVATAR_EPIC:
                    return AVATAR_EPIC_KEY;  // 영웅 키 반환
                case Models::AvatarGrade::AVATAR_NONE:
                    break;  // 없음은 아래 기본 반환으로
            }
            return AVATAR_NONE_KEY;  // 없음 키 반환
        }

        // 이름을 모르는 혼돈 코어의 평균 ln (표에 이름이 아예 없을 때만, 의지력 포인트가 같은 이름 있는 칸의 평균: 같은 코어 번호 우선, 없으면 모든 혼돈 코어)
        [[nodiscard]] std::optional<double> GetChaosFallbackLn(std::size_t core, const Models::ArkCoreState& state)  // 모르는 혼돈 코어 이름의 평균 ln 계산
        {
            if (state.m_name.empty()) return std::nullopt;  // 이름이 없으면 모르는 이름이 아니므로 대체하지 않음
            const std::span<const Data::ArkCorePoint> entries{ Data::GetArkCorePoints() };  // 의지력 포인트 표
            if (std::ranges::any_of(entries, [&state](const Data::ArkCorePoint& entry) { return entry.m_name == state.m_name; })) return std::nullopt;  // 표에 있는 이름이면 대체하지 않음 (포인트가 칸에 없으면 효과 0 유지)

            const bool isAncient{ state.m_grade == Models::ArkCoreGrade::CORE_ANCIENT };  // 고대 등급 여부
            double slotSum{};  // 같은 코어 번호 칸의 ln 합
            double allSum{};  // 모든 혼돈 코어 칸의 ln 합
            std::size_t slotCount{};  // 같은 코어 번호 칸 개수
            std::size_t allCount{};  // 모든 혼돈 코어 칸 개수
            for (const Data::ArkCorePoint& entry : entries)  // 의지력 포인트 표 순회
            {
                if (entry.m_name.empty() || entry.m_points != state.m_points) continue;  // 질서 코어 칸이거나 포인트가 다르면 건너뜀
                const double ln{ isAncient ? entry.m_ancientLn : entry.m_ln };  // 등급에 맞는 ln
                allSum += ln;  // 전체 합에 더함
                ++allCount;  // 전체 개수 증가
                if (entry.m_core != static_cast<int>(core)) continue;  // 다른 코어 번호는 같은 번호 합에서 제외
                slotSum += ln;  // 같은 번호 합에 더함
                ++slotCount;  // 같은 번호 개수 증가
            }
            if (slotCount > 0) return slotSum / static_cast<double>(slotCount);  // 같은 코어 번호 평균 우선
            if (allCount > 0) return allSum / static_cast<double>(allCount);  // 없으면 모든 혼돈 코어 평균
            return std::nullopt;  // 평균을 낼 칸이 없으면 대체하지 않음
        }

        // 코어 하나의 ln: 의지력 포인트가 표의 칸(10 / 14 / 17 / 18 / 19 / 20)과 정확히 같을 때만 효과가 켜지고 그 밖에는 0
        // 질서 코어는 이름과 무관, 혼돈 코어는 이름이 표에 있어야 함 (표에 없는 처음 보는 이름은 같은 종류 혼돈 코어의 평균값으로 대체)
        [[nodiscard]] double GetCoreLn(std::size_t core, const Models::ArkCoreState& state)  // 코어 하나의 ln 계산
        {
            const bool isAncient{ state.m_grade == Models::ArkCoreGrade::CORE_ANCIENT };  // 고대 등급 여부
            for (const Data::ArkCorePoint& entry : Data::GetArkCorePoints())  // 의지력 포인트 표 순회
            {
                if (entry.m_core != static_cast<int>(core) || entry.m_points != state.m_points) continue;  // 코어 번호나 포인트가 다르면 건너뜀
                if (!entry.m_name.empty() && entry.m_name != state.m_name) continue;  // 코어 이름 조건이 있고 이름이 다르면 건너뜀
                return isAncient ? entry.m_ancientLn : entry.m_ln;  // 고대면 고대 ln, 아니면 일반 ln 반환
            }
            return GetChaosFallbackLn(core, state).value_or(0.0);  // 처음 보는 혼돈 코어 이름은 평균 ln, 대체할 수 없으면 효과 0
        }

        // 코어 6개의 ln 합
        [[nodiscard]] double GetCoresLn(const Models::ExtraState& state)  // 코어 6개의 ln 합 계산
        {
            double sum{};  // 합계
            if (IsSupporter(state))  // 서포터는 코어 이름과 무관한 서포터 표
            {
                for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core)  // 코어 6개 순회
                {
                    const Models::ArkCoreState& entry{ state.m_cores[core] };  // 코어 상태
                    const bool isAncient{ entry.m_grade == Models::ArkCoreGrade::CORE_ANCIENT };  // 고대 등급 여부
                    for (const Data::ArkCorePoint& row : Supporter::GetArkCorePoints())  // 서포터 포인트 표 순회
                    {
                        if (row.m_core != static_cast<int>(core) || row.m_points != entry.m_points) continue;  // 코어 번호 · 포인트가 다르면 건너뜀
                        sum += isAncient ? row.m_ancientLn : row.m_ln;  // 등급에 맞는 ln 합산
                        break;  // 한 칸만 해당
                    }
                }
                return sum;  // 서포터 합계 반환
            }
            for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core) sum += GetCoreLn(core, state.m_cores[core]);  // 코어별 ln을 합산
            return sum;  // 합계 반환
        }

        // 아크 패시브 트리 레벨 비율 1 - 기울기 × (최대 - 레벨)
        [[nodiscard]] double GetLevelRatio(double slope, int level) noexcept  // 아크 패시브 트리 레벨 비율 계산
        {
            const int clamped{ std::clamp(level, Models::MIN_VALUE, Models::MAX_PASSIVE_LEVEL) };  // 레벨을 0 ~ 최대 레벨로 제한
            return std::max(1.0 - slope * static_cast<double>(Models::MAX_PASSIVE_LEVEL - clamped), MIN_RATIO);  // 1 - 기울기 × (최대 - 레벨), 0 나눗셈 방지 최소값 적용
        }

        // 랭크 · 깨달음 포인트가 둘 다 최대가 아닐 때 더하는 ln (lopec 조합 측정: 상수 + 교차항 × 두 감소폭의 곱)
        [[nodiscard]] double GetPassiveInteractionLn(double rankRatio, double enlightenmentRatio, bool isSupporter) noexcept  // 랭크 · 깨달음 상호작용 ln 계산
        {
            if (rankRatio >= FULL_PASSIVE_RATIO || enlightenmentRatio >= FULL_PASSIVE_RATIO) return NO_INTERACTION_LN;  // 둘 중 하나라도 최대면 상호작용 없음
            const Data::PassiveInteraction& interaction{ isSupporter ? Supporter::GetPassiveInteraction() : Data::GetPassiveInteraction() };  // 역할별 상호작용 계수
            return interaction.m_constantLn + interaction.m_crossLn * (FULL_PASSIVE_RATIO - rankRatio) * (FULL_PASSIVE_RATIO - enlightenmentRatio);  // 상수 + 교차항 × 랭크 감소폭 × 깨달음 감소폭
        }
    }

    // 처음 보는 혼돈 코어 이름이 있어 평균값으로 계산했는지 (질서 코어 · 표에 있는 이름 · 이름 없음은 해당 없음)
    bool HasUnknownChaosCore(const Models::ExtraState& state)  // 모르는 혼돈 코어 이름 사용 여부 검사
    {
        if (IsSupporter(state)) return false;  // 서포터 표는 코어 이름과 무관하므로 대체 없음
        for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core)  // 코어 6개 순회
        {
            const Models::ArkCoreState& entry{ state.m_cores[core] };  // 현재 코어 상태
            if (!GetChaosFallbackLn(core, entry).has_value()) continue;  // 평균 대체 대상이 아니면 건너뜀
            const bool isNamedMatch{ std::ranges::any_of(Data::GetArkCorePoints(), [core, &entry](const Data::ArkCorePoint& row) { return row.m_core == static_cast<int>(core) && row.m_points == entry.m_points && (row.m_name.empty() || row.m_name == entry.m_name); }) };  // 표에서 그대로 찾아지는 코어인지
            if (!isNamedMatch) return true;  // 표에 없어 평균으로 대체된 코어가 있으면 참
        }
        return false;  // 대체된 코어가 없으면 거짓
    }

    // ===================== 악세서리 =====================

    CombatFactorKind AccessoryFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_ACCESSORY;  // 악세서리 종류 반환
    }

    double AccessoryFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 악세서리 배율 계산
    {
        const double weaponBase{ GetWeaponBase(current) };  // 무기 공격력 기준값
        const bool isEsther{ current.m_extra.m_isEstherWeapon };  // 에스더 무기 여부
        const bool isSupporter{ IsSupporter(current.m_extra) };  // 서포터 표를 쓸지

        double lnChange{};  // ln 변화량 합계
        for (std::size_t slot{}; slot < Models::ACCESSORY_SLOT_COUNT; ++slot)  // 악세서리 슬롯 순회
        {
            for (std::size_t option{}; option < Models::ACCESSORY_OPTION_COUNT; ++option)  // 연마 옵션 줄 순회
            {
                lnChange += GetAccessoryLn(target.m_extra.m_accessories[slot].m_options[option], weaponBase, isEsther, isSupporter)  // 목표 옵션 ln에서
                    - GetAccessoryLn(current.m_extra.m_accessories[slot].m_options[option], weaponBase, isEsther, isSupporter);  // 현재 옵션 ln을 뺀 값을 누적
            }
        }
        return std::exp(lnChange);  // ln 변화량을 지수로 되돌려 배율로 반환
    }

    // ===================== 완갑 =====================

    CombatFactorKind VambraceFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_VAMBRACE;  // 완갑 종류 반환
    }

    double VambraceFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 완갑 배율 계산
    {
        if (IsSupporter(current.m_extra))  // 서포터는 단계별 측정 표 (등급 변화 포함)
        {
            const std::span<const double> levels{ Supporter::GetVambraceLevelLn() };  // 단계 → ln 표
            const std::optional<int>& from{ current.m_extra.m_vambraceLevel };  // 현재 단계
            const std::optional<int>& to{ target.m_extra.m_vambraceLevel };  // 목표 단계
            if (!from.has_value() || !to.has_value() || levels.empty()) return NO_CHANGE_FACTOR;  // 완갑이 없으면 영향 없음
            const auto toIndex{ [&levels](int level) { return static_cast<std::size_t>(std::clamp(level, Models::MIN_VALUE, static_cast<int>(levels.size()) - 1)); } };  // 단계를 표 범위로
            return std::exp(levels[toIndex(*to)] - levels[toIndex(*from)]);  // ln 차이를 배율로
        }
        const Models::CombatExtra::VambraceGain gain{ Models::CombatExtra::GetVambraceGain(current.m_extra, target.m_extra) };  // 완갑 재련 변화량

        // 기본 공격력 % 합 = 보석 기본 공격력 % + 완갑 한계 해방 추가 효과 % (같은 칸에서 더해짐)
        const double currentRate{ NO_CHANGE_FACTOR + current.m_gemBaseAttackRate + gain.m_gradeRateFrom };  // 현재 기본 공격력 배수 (1 + 보석 % + 완갑 등급 %)
        const double targetRate{ NO_CHANGE_FACTOR + current.m_gemBaseAttackRate + gain.m_gradeRateTo };  // 목표 기본 공격력 배수
        const double gradeFactor{ currentRate > 0.0 ? targetRate / currentRate : NO_CHANGE_FACTOR };  // 한계 해방 등급 배율

        // 기본 공격력 A = sqrt(S0 × W0 / 6) × (1 + 보석 % + 완갑 등급 %) × 보정 → 배율 (A + ΔB) / A (S0 · W0를 모르면 ΔB는 계산하지 않음)
        const double weaponBase{ GetWeaponBase(current) };  // 무기 공격력 기준값
        double baseAttackFactor{ NO_CHANGE_FACTOR };  // 기본 공격력 배율 초기값
        if (current.m_totalMainStat > 0.0 && weaponBase > 0.0)  // 주스탯과 무기 기준값을 알 때만 계산
        {
            const double baseAttack{ std::sqrt(current.m_totalMainStat * weaponBase / ATTACK_DIVISOR) * currentRate * Data::GetBaseAttackCalibration() };  // 현재 기본 공격력 A 추정
            if (baseAttack > 0.0) baseAttackFactor = std::max(baseAttack + gain.m_baseAttack, 0.0) / baseAttack;  // (A + ΔB) ÷ A 로 배율 계산
        }
        return baseAttackFactor * gradeFactor;  // 기본 공격력 배율 × 한계 해방 등급 배율 반환
    }

    // ===================== 팔찌 =====================

    CombatFactorKind BraceletFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_BRACELET;  // 팔찌 종류 반환
    }

    double BraceletFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 팔찌 배율 계산
    {
        // 치명 · 특화 · 신속: 포인트 1당 같은 비율로 증가
        const bool isSupporter{ IsSupporter(current.m_extra) };  // 서포터 표를 쓸지
        const double statRatio{ isSupporter ? Supporter::GetBraceletStatRatio() : Data::GetBraceletStatRatio() };  // 역할별 스탯 1포인트당 배율
        const double statFactor{ std::max(1.0 + statRatio * (SumEffectiveStats(target.m_extra) - SumEffectiveStats(current.m_extra)), MIN_RATIO) };  // 치명 · 특화 · 신속 포인트 차이에 따른 선형 배율

        const double weaponBase{ GetWeaponBase(current) };  // 무기 공격력 기준값
        double lnChange{};  // ln 변화량 합계
        for (std::size_t option{}; option < Models::BRACELET_OPTION_COUNT; ++option)  // 팔찌 옵션 줄 순회
        {
            lnChange += GetBraceletLn(target.m_extra.m_braceletOptions[option], weaponBase, isSupporter) - GetBraceletLn(current.m_extra.m_braceletOptions[option], weaponBase, isSupporter);  // 목표 옵션 ln에서 현재 옵션 ln을 뺀 값을 누적
        }
        return statFactor * std::exp(lnChange);  // 스탯 배율 × 옵션 배율 반환
    }

    // ===================== 스톤 · 각인서 =====================

    CombatFactorKind EngravingFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_ENGRAVING;  // 스톤 · 각인서 종류 반환
    }

    double EngravingFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 스톤 · 각인서 배율 계산
    {
        const double lnChange{ GetEngravingLn(target.m_extra) - GetEngravingLn(current.m_extra) };  // 각인서 · 스톤 ln 변화량
        return std::exp(lnChange);  // ln 변화량을 지수로 되돌려 배율로 반환
    }

    // ===================== 아바타 =====================

    CombatFactorKind AvatarFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_AVATAR;  // 아바타 종류 반환
    }

    double AvatarFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 아바타 배율 계산
    {
        const auto getPercent{ [](Models::AvatarGrade grade)  // 등급의 한 부위 주스탯 %를 구하는 람다
        {
            const Data::KeyedValue* found{ Data::FindKeyed(Data::GetAvatarPercent(), GetAvatarKey(grade)) };  // 등급 키로 아바타 주스탯 % 표 조회
            return found != nullptr ? found->m_value : 0.0;  // 표에 없으면 0
        } };

        double currentPercent{};  // 현재 네 부위 주스탯 % 합
        double targetPercent{};   // 목표 네 부위 주스탯 % 합
        for (std::size_t slot{}; slot < Models::AVATAR_SLOT_COUNT; ++slot)  // 아바타 부위 슬롯 순회
        {
            currentPercent += getPercent(current.m_extra.m_avatars[slot]);  // 현재 등급의 주스탯 % 누적
            targetPercent += getPercent(target.m_extra.m_avatars[slot]);    // 목표 등급의 주스탯 % 누적
        }
        const double scale{ IsSupporter(current.m_extra) ? Supporter::GetAvatarMainStatScale() : Data::GetAvatarMainStatScale() };  // 역할별 주스탯 % 합 배율 계수
        return std::sqrt((1.0 + scale * targetPercent) / (1.0 + scale * currentPercent));  // 주스탯 변화가 sqrt(S) 경로로 들어간 배율 반환
    }

    // ===================== 아크 그리드 코어 =====================

    CombatFactorKind ArkCoreFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_ARK_CORE;  // 아크 코어 종류 반환
    }

    double ArkCoreFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 아크 코어 배율 계산
    {
        return std::exp(GetCoresLn(target.m_extra) - GetCoresLn(current.m_extra));  // 코어 ln 변화량을 지수로 되돌려 배율로 반환
    }

    // ===================== 아크 그리드 젬 =====================

    CombatFactorKind ArkGemFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_ARK_GEM;  // 아크 젬 종류 반환
    }

    double ArkGemFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 아크 젬 배율 계산
    {
        double lnChange{};  // ln 변화량 합계
        const std::span<const Data::KeyedValue> perLevelTable{ IsSupporter(current.m_extra) ? Supporter::GetArkGemPerLevel() : Data::GetArkGemPerLevel() };  // 역할별 레벨당 ln 표
        for (std::size_t effect{}; effect < Models::ARK_GEM_EFFECT_COUNT; ++effect)  // 젬 효과 종류 순회
        {
            const Data::KeyedValue* perLevel{ Data::FindKeyed(perLevelTable, Models::ARK_GEM_EFFECT_NAMES[effect]) };  // 효과별 레벨 1당 ln 조회
            if (perLevel == nullptr) continue;  // 표에 없으면 건너뜀
            lnChange += perLevel->m_value * static_cast<double>(target.m_extra.m_arkGemLevels[effect] - current.m_extra.m_arkGemLevels[effect]);  // 레벨 합 차이 × 레벨당 ln 누적
        }
        return std::exp(lnChange);  // ln 변화량을 지수로 되돌려 배율로 반환
    }

    // ===================== 아크 패시브 =====================

    CombatFactorKind ArkPassiveFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_ARK_PASSIVE;  // 아크 패시브 종류 반환
    }

    double ArkPassiveFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 아크 패시브 배율 계산
    {
        const Models::ArkPassiveState& from{ current.m_extra.m_passive };  // 현재 패시브 상태
        const Models::ArkPassiveState& to{ target.m_extra.m_passive };  // 목표 패시브 상태

        // 랭크 (0 ~ 6 표, 트리별 랭크의 ln을 몫으로 가중합 · lopec 랭크 조절은 세 트리를 같은 값으로)
        double factor{ NO_CHANGE_FACTOR };  // 배율 초기값 (변화 없음)
        const bool isSupporter{ IsSupporter(current.m_extra) };  // 서포터 표를 쓸지
        const std::span<const double> ranks{ isSupporter ? Supporter::GetPassiveRankRatios() : Data::GetPassiveRankRatios() };  // 역할별 랭크 배율 표
        const std::span<const double> shares{ Data::GetPassiveRankTreeShares() };  // 트리별 몫 (진화 · 깨달음 · 도약)
        const auto rankRatio{ [&ranks](int rank) { return ranks.empty() ? FULL_PASSIVE_RATIO : ranks[static_cast<std::size_t>(std::clamp(rank, Models::MIN_VALUE, static_cast<int>(ranks.size()) - 1))]; } };  // 랭크를 표 범위 안으로 제한해 배율 조회 (표가 없으면 최대 취급)
        const auto treeRankRatio{ [&rankRatio, &shares](const Models::ArkPassiveState& state)  // 트리별 랭크 → 효과 랭크 배율
        {
            double rankLn{ NO_INTERACTION_LN };  // 가중 ln 합
            for (std::size_t tree{}; tree < state.m_treeRanks.size() && tree < shares.size(); ++tree)  // 몫이 있는 트리만 순회 (경계 검사)
            {
                const double ratio{ rankRatio(state.m_treeRanks[tree]) };  // 이 트리 랭크의 표 배율
                if (ratio > 0.0) rankLn += shares[tree] * std::log(ratio);  // 양수일 때만 몫 × ln 누적
            }
            return std::exp(rankLn);  // 효과 랭크 배율
        } };
        const double fromRank{ treeRankRatio(from) };  // 현재 랭크 배율 (트리별 가중)
        const double toRank{ treeRankRatio(to) };  // 목표 랭크 배율 (트리별 가중)
        if (fromRank > 0.0) factor *= toRank / fromRank;  // 현재 랭크 값이 양수이면 랭크 배율 반영
        const double fromEnlightenmentRank{ rankRatio(from.m_treeRanks[PASSIVE_ENLIGHTENMENT_TREE]) };  // 현재 깨달음 트리 랭크 배율 (상호작용용)
        const double toEnlightenmentRank{ rankRatio(to.m_treeRanks[PASSIVE_ENLIGHTENMENT_TREE]) };  // 목표 깨달음 트리 랭크 배율 (상호작용용)

        // 깨달음 포인트 구성 (+ 랭크와 둘 다 최대가 아닐 때 상호작용)
        const std::span<const Data::KeyedValue> enlightenments{ isSupporter ? Supporter::GetEnlightenmentRatios() : Data::GetEnlightenmentRatios() };  // 역할별 깨달음 구성 배율 표
        const Data::KeyedValue* fromEnlightenment{ Data::FindKeyed(enlightenments, from.m_enlightenment) };  // 현재 구성 표 값
        const Data::KeyedValue* toEnlightenment{ Data::FindKeyed(enlightenments, to.m_enlightenment) };  // 목표 구성 표 값
        if (fromEnlightenment != nullptr && toEnlightenment != nullptr && fromEnlightenment->m_value > 0.0 && fromRank > 0.0)  // 두 구성이 모두 표에 있고 값이 양수일 때만
        {
            factor *= toEnlightenment->m_value / fromEnlightenment->m_value;  // 깨달음 구성 비율 반영
            factor *= std::exp(GetPassiveInteractionLn(toEnlightenmentRank, toEnlightenment->m_value, isSupporter) - GetPassiveInteractionLn(fromEnlightenmentRank, fromEnlightenment->m_value, isSupporter));  // 상호작용 ln 차이 반영 (역할별, 깨달음 트리 랭크 기준, lopec 측정)
        }

        // 서포터: 진화 · 깨달음 레벨 (도약 레벨은 효과 없음)
        if (isSupporter)
        {
            const Supporter::SupporterPassiveSlopes& supporterSlopes{ Supporter::GetPassiveSlopes() };  // 서포터 트리 레벨 기울기
            factor *= GetLevelRatio(supporterSlopes.m_evolution, to.m_levels[PASSIVE_EVOLUTION_TREE]) / GetLevelRatio(supporterSlopes.m_evolution, from.m_levels[PASSIVE_EVOLUTION_TREE]);  // 진화 트리 레벨 비율
            factor *= GetLevelRatio(supporterSlopes.m_enlightenment, to.m_levels[PASSIVE_ENLIGHTENMENT_TREE]) / GetLevelRatio(supporterSlopes.m_enlightenment, from.m_levels[PASSIVE_ENLIGHTENMENT_TREE]);  // 깨달음 트리 레벨 비율
            factor *= GetLevelRatio(supporterSlopes.m_leap, to.m_levels[PASSIVE_LEAP_TREE]) / GetLevelRatio(supporterSlopes.m_leap, from.m_levels[PASSIVE_LEAP_TREE]);  // 도약 트리 레벨 비율 (기울기 0 = 1)
            return factor;  // 서포터 배율 반환
        }

        // 깨달음 · 도약 레벨 (진화 레벨은 효과 없음)
        const Data::PassiveSlopes& slopes{ Data::GetPassiveSlopes() };  // 트리 레벨 기울기 표
        factor *= GetLevelRatio(slopes.m_level1, to.m_levels[PASSIVE_ENLIGHTENMENT_TREE]) / GetLevelRatio(slopes.m_level1, from.m_levels[PASSIVE_ENLIGHTENMENT_TREE]);  // 깨달음 트리 레벨 비율 반영
        factor *= GetLevelRatio(slopes.m_level2, to.m_levels[PASSIVE_LEAP_TREE]) / GetLevelRatio(slopes.m_level2, from.m_levels[PASSIVE_LEAP_TREE]);  // 도약 트리 레벨 비율 반영
        return factor;  // 누적한 배율 반환
    }
}
