module LostArkMultiTool.Core.CombatExtraComponent;  // 추가 장착 배율 요소 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.CombatFactorComponent;  // 배율 요소 기반 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모듈 가져오기
import LostArkMultiTool.Models.CombatState;  // 전투력 상태 모듈 가져오기

namespace LostArkMultiTool::Core  // 코어 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        namespace Data = Models::CombatCoefficientData;  // 계수 데이터 네임스페이스 별칭

        // ===== 문구 · 상수 =====
        constexpr std::string_view WEAPON_ATTACK_PREFIX{ "무기 공격력 +" };   // 악세서리 무기 공격력 옵션 (식으로 계산)
        constexpr char PLUS_SIGN{ '+' };  // 옵션 문구에서 수치 앞의 덧셈 기호
        constexpr char PERCENT_SIGN{ '%' };  // 옵션 수치 뒤의 퍼센트 기호
        constexpr double PERCENT_SCALE{ 100.0 };  // 퍼센트를 비율로 바꾸는 배수
        constexpr double WEAPON_EXPONENT{ 0.5 };              // 무기 공격력 → 전투력은 제곱근
        constexpr double ATTACK_DIVISOR{ 6.0 };               // 공격력 = sqrt(주스탯 × 무기 공격력 / 6)
        constexpr double MIN_RATIO{ 1e-6 };                   // 나눗셈 보호
        constexpr double NO_CHANGE_FACTOR{ 1.0 };             // 바뀐 것이 없거나 계산할 수 없음
        constexpr std::string_view KEY_SAME{ "S" };           // 스톤 두 줄이 같은 각인
        constexpr std::string_view KEY_DIFFERENT{ "D" };      // 스톤 두 줄이 다른 각인
        constexpr std::string_view AVATAR_LEGENDARY_KEY{ "전설" };  // 아바타 전설 등급 표 키
        constexpr std::string_view AVATAR_EPIC_KEY{ "영웅" };  // 아바타 영웅 등급 표 키
        constexpr std::string_view AVATAR_NONE_KEY{ "없음" };  // 아바타 없음 표 키
        constexpr std::size_t PASSIVE_ENLIGHTENMENT_TREE{ 1 };   // 아크 패시브 트리 번호: 0 진화(효과 없음), 1 깨달음, 2 도약
        constexpr std::size_t PASSIVE_LEAP_TREE{ 2 };  // 아크 패시브 도약 트리 번호

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
        [[nodiscard]] double GetAccessoryLn(std::string_view option, double weaponBase, bool isEsther)  // 악세서리 옵션 한 줄의 ln 계산
        {
            if (Models::IsNone(option)) return 0.0;  // 옵션이 비어 있으면 0

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
        [[nodiscard]] double GetBraceletLn(std::string_view option, double weaponBase)  // 팔찌 옵션 한 줄의 ln 계산
        {
            if (Models::IsNone(option)) return 0.0;  // 옵션이 비어 있으면 0

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
            for (const Models::BraceletStat& line : state.m_braceletStats)  // 팔찌 스탯 줄 순회
            {
                if (std::ranges::find(Data::GetBraceletEffectiveTypes(), line.m_type) != Data::GetBraceletEffectiveTypes().end()) sum += line.m_value;  // 유효 스탯 종류이면 합산
            }
            return sum;  // 합계 반환
        }

        // 스톤: 각인 이름이 비면 레벨 0
        [[nodiscard]] int GetStoneLevel(const Models::StoneLine& line) noexcept  // 스톤 레벨 (각인 이름이 비면 0)
        {
            return Models::IsNone(line.m_name) ? 0 : line.m_level;  // 이름이 없으면 0, 있으면 저장된 레벨
        }

        // 스톤 두 줄 조합의 ln (표에 없으면 nullopt, 1번 줄이 없을 때는 2번 줄 레벨별 평균)
        [[nodiscard]] std::optional<double> GetStoneLn(const Models::ExtraState& state)  // 스톤 두 줄 조합의 ln 계산
        {
            const Models::StoneLine& first{ state.m_stone[0] };  // 첫 번째 스톤 줄
            const Models::StoneLine& second{ state.m_stone[1] };  // 두 번째 스톤 줄
            const int firstLevel{ GetStoneLevel(first) };  // 첫 줄 레벨
            const int secondLevel{ GetStoneLevel(second) };  // 둘째 줄 레벨
            const bool isSame{ !Models::IsNone(first.m_name) && first.m_name == second.m_name };  // 두 줄이 같은 각인인지 여부

            // 레벨 1 이상인 줄의 각인 이름별 보정 (표에 없는 이름은 0)
            const auto getNameLn{ [](const Models::StoneLine& line, int level)  // 줄의 각인 이름별 보정 값을 구하는 람다
            {
                if (level <= 0) return 0.0;  // 레벨 0 이하면 보정 없음
                const Data::KeyedValue* offset{ Data::FindKeyed(Data::GetStoneNameLn(), line.m_name) };  // 이름 보정 표 조회
                return offset != nullptr ? offset->m_value : 0.0;  // 표에 없으면 0
            } };
            const double nameLn{ getNameLn(first, firstLevel) + getNameLn(second, secondLevel) };  // 두 줄의 이름 보정 합

            const std::string key{ std::string{ isSame ? KEY_SAME : KEY_DIFFERENT } + std::to_string(firstLevel) + std::to_string(secondLevel) };  // 조합 키 (같은/다른 + 두 레벨)
            if (const Data::KeyedValue* found{ Data::FindKeyed(Data::GetStoneCells(), key) }; found != nullptr) return found->m_value + nameLn;  // 조합 표에 있으면 값에 이름 보정을 더해 반환

            if (firstLevel == 0 && secondLevel > 0)  // 첫 줄이 없고 둘째 줄만 있으면
            {
                if (const Data::KeyedValue* unstable{ Data::FindKeyed(Data::GetStoneUnstable(), std::to_string(secondLevel)) }; unstable != nullptr) return unstable->m_value + nameLn;  // 불안정 평균 표에서 둘째 줄 레벨로 조회
            }
            return std::nullopt;  // 표에 없는 조합이면 값 없음
        }

        // 각인서 5개의 ln 합 (없음 · 앞 칸과 같은 이름 · 효과 없는 각인은 0)
        [[nodiscard]] double GetBooksLn(const Models::ExtraState& state)  // 각인서 ln 합 계산
        {
            double sum{};  // 합계
            for (std::size_t index{}; index < Models::BOOK_COUNT; ++index)  // 각인서 5개 순회
            {
                const Models::BookState& book{ state.m_books[index] };  // 현재 각인서
                if (Models::IsNone(book.m_name)) continue;  // 이름이 없으면 건너뜀

                const auto isSameName{ [&book](const Models::BookState& other) { return other.m_name == book.m_name; } };  // 앞 칸과 같은 이름인지 검사하는 람다
                if (std::any_of(state.m_books.begin(), state.m_books.begin() + static_cast<std::ptrdiff_t>(index), isSameName)) continue;  // 앞 칸에 같은 이름이 있으면 중복이므로 건너뜀

                const std::span<const Data::EngravingBook> books{ Data::GetEngravingBooks() };  // 각인서 효과 표
                const auto found{ std::ranges::find(books, book.m_name, &Data::EngravingBook::m_name) };  // 이름으로 표 항목 찾기
                if (found == books.end() || (found->m_base == 0.0 && found->m_top == 0.0)) continue;  // 표에 없거나 효과가 없는 각인은 건너뜀

                const double fraction{ static_cast<double>(std::clamp(book.m_level, Models::MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL)) / Data::ENGRAVING_BOOK_MAX_LEVEL };  // 레벨 비율 (0 ~ 최대 레벨 → 0 ~ 1)
                sum += found->m_base + (found->m_top - found->m_base) * fraction;  // 기본값과 최대값 사이를 레벨 비율로 보간해 합산
                if (book.m_isLegendary)  // 전설 등급 각인서이면
                {
                    const Data::LevelRange& penalty{ Data::GetLegendPenalty() };  // 전설 감점 범위
                    sum += penalty.m_atMin + (penalty.m_atMax - penalty.m_atMin) * fraction;  // 감점도 같은 비율로 보간해 합산
                }
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

        // 코어 하나의 ln: 의지력 포인트가 표의 칸(10 / 14 / 17 / 18 / 19 / 20)과 정확히 같을 때만 효과가 켜지고 그 밖에는 0
        // 질서 코어는 이름과 무관, 혼돈 코어는 이름이 표에 있어야 함 (없는 이름 = 효과 0)
        [[nodiscard]] double GetCoreLn(std::size_t core, const Models::ArkCoreState& state)  // 코어 하나의 ln 계산
        {
            const bool isAncient{ state.m_grade == Models::ArkCoreGrade::CORE_ANCIENT };  // 고대 등급 여부
            for (const Data::ArkCorePoint& entry : Data::GetArkCorePoints())  // 의지력 포인트 표 순회
            {
                if (entry.m_core != static_cast<int>(core) || entry.m_points != state.m_points) continue;  // 코어 번호나 포인트가 다르면 건너뜀
                if (!entry.m_name.empty() && entry.m_name != state.m_name) continue;  // 코어 이름 조건이 있고 이름이 다르면 건너뜀
                return isAncient ? entry.m_ancientLn : entry.m_ln;  // 고대면 고대 ln, 아니면 일반 ln 반환
            }
            return 0.0;  // 표에 없는 포인트는 효과 0
        }

        // 코어 6개의 ln 합
        [[nodiscard]] double GetCoresLn(const Models::ExtraState& state)  // 코어 6개의 ln 합 계산
        {
            double sum{};  // 합계
            for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core) sum += GetCoreLn(core, state.m_cores[core]);  // 코어별 ln을 합산
            return sum;  // 합계 반환
        }

        // 아크 패시브 트리 레벨 비율 1 - 기울기 × (최대 - 레벨)
        [[nodiscard]] double GetLevelRatio(double slope, int level) noexcept  // 아크 패시브 트리 레벨 비율 계산
        {
            const int clamped{ std::clamp(level, Models::MIN_VALUE, Models::MAX_PASSIVE_LEVEL) };  // 레벨을 0 ~ 최대 레벨로 제한
            return std::max(1.0 - slope * static_cast<double>(Models::MAX_PASSIVE_LEVEL - clamped), MIN_RATIO);  // 1 - 기울기 × (최대 - 레벨), 0 나눗셈 방지 최소값 적용
        }

        // 표 배율 두 개의 비 (둘 중 하나라도 없으면 1)
        [[nodiscard]] double GetKeyedRatio(std::span<const Data::KeyedValue> table, std::string_view currentKey, std::string_view targetKey)  // 표 배율 두 개의 비 계산
        {
            const Data::KeyedValue* currentValue{ Data::FindKeyed(table, currentKey) };  // 현재 키의 표 값
            const Data::KeyedValue* targetValue{ Data::FindKeyed(table, targetKey) };  // 목표 키의 표 값
            if (currentValue == nullptr || targetValue == nullptr || currentValue->m_value <= 0.0) return NO_CHANGE_FACTOR;  // 하나라도 없거나 현재 값이 0 이하면 영향 없음
            return targetValue->m_value / currentValue->m_value;  // 목표 ÷ 현재 반환
        }
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

        double lnChange{};  // ln 변화량 합계
        for (std::size_t slot{}; slot < Models::ACCESSORY_SLOT_COUNT; ++slot)  // 악세서리 슬롯 순회
        {
            for (std::size_t option{}; option < Models::ACCESSORY_OPTION_COUNT; ++option)  // 연마 옵션 줄 순회
            {
                lnChange += GetAccessoryLn(target.m_extra.m_accessories[slot].m_options[option], weaponBase, isEsther)  // 목표 옵션 ln에서
                    - GetAccessoryLn(current.m_extra.m_accessories[slot].m_options[option], weaponBase, isEsther);  // 현재 옵션 ln을 뺀 값을 누적
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
        const Models::CombatExtra::VambraceGain gain{ Models::CombatExtra::GetVambraceGain(current.m_extra, target.m_extra) };  // 완갑 재련 변화량

        // 기본 공격력 A = sqrt(S0 × W0 / 6) × (1 + 보석 기본 공격력 %) × 보정 → 배율 (A + ΔB) / A (S0 · W0를 모르면 ΔB는 계산하지 않음)
        const double weaponBase{ GetWeaponBase(current) };  // 무기 공격력 기준값
        double baseAttackFactor{ NO_CHANGE_FACTOR };  // 기본 공격력 배율 초기값
        if (current.m_totalMainStat > 0.0 && weaponBase > 0.0)  // 주스탯과 무기 기준값을 알 때만 계산
        {
            const double baseAttack{ std::sqrt(current.m_totalMainStat * weaponBase / ATTACK_DIVISOR) * (1.0 + current.m_gemBaseAttackRate) * Data::GetBaseAttackCalibration() };  // 현재 기본 공격력 A 추정
            if (baseAttack > 0.0) baseAttackFactor = std::max(baseAttack + gain.m_baseAttack, 0.0) / baseAttack;  // (A + ΔB) ÷ A 로 배율 계산
        }
        return baseAttackFactor * gain.m_gradeFactor;  // 기본 공격력 배율 × 등급 배율 반환
    }

    // ===================== 팔찌 =====================

    CombatFactorKind BraceletFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_BRACELET;  // 팔찌 종류 반환
    }

    double BraceletFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 팔찌 배율 계산
    {
        // 치명 · 특화 · 신속: 포인트 1당 같은 비율로 증가
        const double statFactor{ std::max(1.0 + Data::GetBraceletStatRatio() * (SumEffectiveStats(target.m_extra) - SumEffectiveStats(current.m_extra)), MIN_RATIO) };  // 치명 · 특화 · 신속 포인트 차이에 따른 선형 배율

        const double weaponBase{ GetWeaponBase(current) };  // 무기 공격력 기준값
        double lnChange{};  // ln 변화량 합계
        for (std::size_t option{}; option < Models::BRACELET_OPTION_COUNT; ++option)  // 팔찌 옵션 줄 순회
        {
            lnChange += GetBraceletLn(target.m_extra.m_braceletOptions[option], weaponBase) - GetBraceletLn(current.m_extra.m_braceletOptions[option], weaponBase);  // 목표 옵션 ln에서 현재 옵션 ln을 뺀 값을 누적
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
        double lnChange{ GetBooksLn(target.m_extra) - GetBooksLn(current.m_extra) };  // 각인서 ln 변화량

        // 스톤: 현재나 목표 조합이 표에 없으면 스톤은 계산하지 않음
        const std::optional<double> currentStone{ GetStoneLn(current.m_extra) };  // 현재 스톤 조합의 ln
        const std::optional<double> targetStone{ GetStoneLn(target.m_extra) };  // 목표 스톤 조합의 ln
        if (currentStone.has_value() && targetStone.has_value()) lnChange += *targetStone - *currentStone;  // 둘 다 표에 있을 때만 스톤 변화량 반영
        return std::exp(lnChange);  // ln 변화량을 지수로 되돌려 배율로 반환
    }

    // ===================== 아바타 =====================

    CombatFactorKind AvatarFactorComponent::GetKind() const noexcept  // 이 요소의 종류 조회
    {
        return CombatFactorKind::FACTOR_AVATAR;  // 아바타 종류 반환
    }

    double AvatarFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 아바타 배율 계산
    {
        const auto getLn{ [](Models::AvatarGrade grade)  // 등급의 ln을 구하는 람다
        {
            const Data::KeyedValue* found{ Data::FindKeyed(Data::GetAvatarLn(), GetAvatarKey(grade)) };  // 등급 키로 아바타 ln 표 조회
            return found != nullptr ? found->m_value : 0.0;  // 표에 없으면 0
        } };

        double lnChange{};  // ln 변화량 합계
        for (std::size_t slot{}; slot < Models::AVATAR_SLOT_COUNT; ++slot)  // 아바타 부위 슬롯 순회
        {
            lnChange += getLn(target.m_extra.m_avatars[slot]) - getLn(current.m_extra.m_avatars[slot]);  // 목표 등급 ln에서 현재 등급 ln을 뺀 값을 누적
        }
        return std::exp(lnChange);  // ln 변화량을 지수로 되돌려 배율로 반환
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
        for (std::size_t effect{}; effect < Models::ARK_GEM_EFFECT_COUNT; ++effect)  // 젬 효과 종류 순회
        {
            const Data::KeyedValue* perLevel{ Data::FindKeyed(Data::GetArkGemPerLevel(), Models::ARK_GEM_EFFECT_NAMES[effect]) };  // 효과별 레벨 1당 ln 조회
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

        // 랭크 (0 ~ 6 표)
        double factor{ NO_CHANGE_FACTOR };  // 배율 초기값 (변화 없음)
        const std::span<const double> ranks{ Data::GetPassiveRankRatios() };  // 랭크 배율 표
        const auto rankIndex{ [&ranks](int rank) { return static_cast<std::size_t>(std::clamp(rank, Models::MIN_VALUE, static_cast<int>(ranks.size()) - 1)); } };  // 랭크를 표 범위 안 번호로 바꾸는 람다
        if (!ranks.empty() && ranks[rankIndex(from.m_rank)] > 0.0) factor *= ranks[rankIndex(to.m_rank)] / ranks[rankIndex(from.m_rank)];  // 표가 있고 현재 랭크 값이 양수이면 랭크 배율 반영

        // 깨달음 포인트 구성
        factor *= GetKeyedRatio(Data::GetEnlightenmentRatios(), from.m_enlightenment, to.m_enlightenment);  // 깨달음 구성 비율 반영

        // 깨달음 · 도약 레벨 (진화 레벨은 효과 없음)
        const Data::PassiveSlopes& slopes{ Data::GetPassiveSlopes() };  // 트리 레벨 기울기 표
        factor *= GetLevelRatio(slopes.m_level1, to.m_levels[PASSIVE_ENLIGHTENMENT_TREE]) / GetLevelRatio(slopes.m_level1, from.m_levels[PASSIVE_ENLIGHTENMENT_TREE]);  // 깨달음 트리 레벨 비율 반영
        factor *= GetLevelRatio(slopes.m_level2, to.m_levels[PASSIVE_LEAP_TREE]) / GetLevelRatio(slopes.m_level2, from.m_levels[PASSIVE_LEAP_TREE]);  // 도약 트리 레벨 비율 반영
        return factor;  // 누적한 배율 반환
    }
}
