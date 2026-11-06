module LostArkMultiTool.Core.CombatExtraComponent;

import std;
import LostArkMultiTool.Core.CombatFactorComponent;
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Models.CombatState;

namespace LostArkMultiTool::Core
{
    namespace
    {
        namespace Data = Models::CombatCoefficientData;

        // ===== 문구 · 상수 =====
        constexpr std::string_view WEAPON_ATTACK_PREFIX{ "무기 공격력 +" };   // 악세서리 무기 공격력 옵션 (식으로 계산)
        constexpr char PLUS_SIGN{ '+' };
        constexpr char PERCENT_SIGN{ '%' };
        constexpr double PERCENT_SCALE{ 100.0 };
        constexpr double WEAPON_EXPONENT{ 0.5 };              // 무기 공격력 → 전투력은 제곱근
        constexpr double ATTACK_DIVISOR{ 6.0 };               // 공격력 = sqrt(주스탯 × 무기 공격력 / 6)
        constexpr double MIN_RATIO{ 1e-6 };                   // 나눗셈 보호
        constexpr double NO_CHANGE_FACTOR{ 1.0 };             // 바뀐 것이 없거나 계산할 수 없음
        constexpr std::string_view KEY_SAME{ "S" };           // 스톤 두 줄이 같은 각인
        constexpr std::string_view KEY_DIFFERENT{ "D" };      // 스톤 두 줄이 다른 각인
        constexpr std::string_view AVATAR_LEGENDARY_KEY{ "전설" };
        constexpr std::string_view AVATAR_EPIC_KEY{ "영웅" };
        constexpr std::string_view AVATAR_NONE_KEY{ "없음" };
        constexpr std::size_t PASSIVE_ENLIGHTENMENT_TREE{ 1 };   // 아크 패시브 트리 번호: 0 진화(효과 없음), 1 깨달음, 2 도약
        constexpr std::size_t PASSIVE_LEAP_TREE{ 2 };

        // 무기 공격력 기준 W0 (모르면 0 → 무기 공격력 식은 계산하지 않음)
        [[nodiscard]] double GetWeaponBase(const Models::CombatState& state) noexcept
        {
            return Data::GetWeaponBase(state.m_weaponAttack);
        }

        // 문구 끝의 "+숫자" 또는 "+숫자%" 읽기 (없으면 nullopt)
        struct OptionNumber
        {
            double m_value{};
            bool m_isPercent{};
        };

        [[nodiscard]] std::optional<OptionNumber> ParseOptionNumber(std::string_view text) noexcept
        {
            const std::size_t plus{ text.rfind(PLUS_SIGN) };
            if (plus == std::string_view::npos) return std::nullopt;

            const std::string_view digits{ text.substr(plus + 1) };
            double value{};
            const auto [end, error]{ std::from_chars(digits.data(), digits.data() + digits.size(), value) };
            if (error != std::errc{}) return std::nullopt;
            const bool isPercent{ end != digits.data() + digits.size() && *end == PERCENT_SIGN };
            return OptionNumber{ value, isPercent };
        }

        // 악세서리 옵션 한 줄의 ln (표에 없으면 0)
        [[nodiscard]] double GetAccessoryLn(std::string_view option, double weaponBase, bool isEsther)
        {
            if (Models::IsNone(option)) return 0.0;

            if (option.starts_with(WEAPON_ATTACK_PREFIX))
            {
                const std::optional<OptionNumber> number{ ParseOptionNumber(option) };
                if (!number.has_value()) return 0.0;
                if (number->m_isPercent)
                {
                    const Data::AccessoryFormula& formula{ Data::GetAccessoryFormula() };
                    const double ratio{ isEsther ? formula.m_weaponRatioEsther : formula.m_weaponRatioReforge };
                    return WEAPON_EXPONENT * std::log1p(number->m_value / PERCENT_SCALE * ratio);
                }
                return weaponBase > 0.0 ? WEAPON_EXPONENT * std::log1p(number->m_value / weaponBase) : 0.0;
            }

            const Data::KeyedValue* found{ Data::FindKeyed(Data::GetAccessoryOptions(), option) };
            return found != nullptr ? found->m_value : 0.0;
        }

        // 팔찌 옵션 한 줄의 ln (공백 없는 문구, 표에 없으면 0)
        [[nodiscard]] double GetBraceletLn(std::string_view option, double weaponBase)
        {
            if (Models::IsNone(option)) return 0.0;

            for (const Data::BraceletWeaponLine& line : Data::GetBraceletWeaponLines())
            {
                if (line.m_key != option) continue;
                const double weaponPart{ weaponBase > 0.0 ? WEAPON_EXPONENT * std::log1p(line.m_weaponFlat / weaponBase) : 0.0 };
                return line.m_logAdd + weaponPart;
            }
            const Data::KeyedValue* found{ Data::FindKeyed(Data::GetBraceletOptions(), option) };
            return found != nullptr ? found->m_value : 0.0;
        }

        // 치명 · 특화 · 신속 줄의 수치 합
        [[nodiscard]] double SumEffectiveStats(const Models::ExtraState& state)
        {
            double sum{};
            for (const Models::BraceletStat& line : state.m_braceletStats)
            {
                if (std::ranges::find(Data::GetBraceletEffectiveTypes(), line.m_type) != Data::GetBraceletEffectiveTypes().end()) sum += line.m_value;
            }
            return sum;
        }

        // 스톤: 각인 이름이 비면 레벨 0
        [[nodiscard]] int GetStoneLevel(const Models::StoneLine& line) noexcept
        {
            return Models::IsNone(line.m_name) ? 0 : line.m_level;
        }

        // 스톤 두 줄 조합의 ln (표에 없으면 nullopt, 1번 줄이 없을 때는 2번 줄 레벨별 평균)
        [[nodiscard]] std::optional<double> GetStoneLn(const Models::ExtraState& state)
        {
            const Models::StoneLine& first{ state.m_stone[0] };
            const Models::StoneLine& second{ state.m_stone[1] };
            const int firstLevel{ GetStoneLevel(first) };
            const int secondLevel{ GetStoneLevel(second) };
            const bool isSame{ !Models::IsNone(first.m_name) && first.m_name == second.m_name };

            const std::string key{ std::string{ isSame ? KEY_SAME : KEY_DIFFERENT } + std::to_string(firstLevel) + std::to_string(secondLevel) };
            if (const Data::KeyedValue* found{ Data::FindKeyed(Data::GetStoneCells(), key) }; found != nullptr) return found->m_value;

            if (firstLevel == 0 && secondLevel > 0)
            {
                if (const Data::KeyedValue* unstable{ Data::FindKeyed(Data::GetStoneUnstable(), std::to_string(secondLevel)) }; unstable != nullptr) return unstable->m_value;
            }
            return std::nullopt;
        }

        // 각인서 5개의 ln 합 (없음 · 앞 칸과 같은 이름 · 효과 없는 각인은 0)
        [[nodiscard]] double GetBooksLn(const Models::ExtraState& state)
        {
            double sum{};
            for (std::size_t index{}; index < Models::BOOK_COUNT; ++index)
            {
                const Models::BookState& book{ state.m_books[index] };
                if (Models::IsNone(book.m_name)) continue;

                const auto isSameName{ [&book](const Models::BookState& other) { return other.m_name == book.m_name; } };
                if (std::any_of(state.m_books.begin(), state.m_books.begin() + static_cast<std::ptrdiff_t>(index), isSameName)) continue;

                const std::span<const Data::EngravingBook> books{ Data::GetEngravingBooks() };
                const auto found{ std::ranges::find(books, book.m_name, &Data::EngravingBook::m_name) };
                if (found == books.end() || (found->m_base == 0.0 && found->m_top == 0.0)) continue;

                const double fraction{ static_cast<double>(std::clamp(book.m_level, Models::MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL)) / Data::ENGRAVING_BOOK_MAX_LEVEL };
                sum += found->m_base + (found->m_top - found->m_base) * fraction;
                if (book.m_isLegendary)
                {
                    const Data::LevelRange& penalty{ Data::GetLegendPenalty() };
                    sum += penalty.m_atMin + (penalty.m_atMax - penalty.m_atMin) * fraction;
                }
            }
            return sum;
        }

        // 아바타 등급 → 표 키
        [[nodiscard]] std::string_view GetAvatarKey(Models::AvatarGrade grade) noexcept
        {
            switch (grade)
            {
                case Models::AvatarGrade::AVATAR_LEGENDARY:
                    return AVATAR_LEGENDARY_KEY;
                case Models::AvatarGrade::AVATAR_EPIC:
                    return AVATAR_EPIC_KEY;
                case Models::AvatarGrade::AVATAR_NONE:
                    break;
            }
            return AVATAR_NONE_KEY;
        }

        // 코어 의지력 포인트의 ln: 같은 포인트가 표에 있으면 그 값, 없으면 바로 아래 포인트 (표보다 낮으면 가장 낮은 칸)
        [[nodiscard]] double GetCorePointLn(std::size_t core, int points)
        {
            const Data::ArkCorePoint* lowest{};
            const Data::ArkCorePoint* floorEntry{};
            for (const Data::ArkCorePoint& entry : Data::GetArkCorePoints())
            {
                if (entry.m_core != static_cast<int>(core)) continue;
                if (lowest == nullptr || entry.m_points < lowest->m_points) lowest = &entry;
                if (entry.m_points <= points && (floorEntry == nullptr || entry.m_points > floorEntry->m_points)) floorEntry = &entry;
            }
            if (floorEntry != nullptr) return floorEntry->m_ln;
            return lowest != nullptr ? lowest->m_ln : 0.0;
        }

        // 코어 6개의 ln 합
        [[nodiscard]] double GetCoresLn(const Models::ExtraState& state)
        {
            const std::span<const double> ancientGains{ Data::GetArkCoreAncientGains() };
            double sum{};
            for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core)
            {
                const Models::ArkCoreState& coreState{ state.m_cores[core] };
                if (coreState.m_grade == Models::ArkCoreGrade::CORE_ANCIENT && core < ancientGains.size()) sum += ancientGains[core];
                sum += GetCorePointLn(core, coreState.m_points);
            }
            return sum;
        }

        // 아크 패시브 트리 레벨 비율 1 - 기울기 × (최대 - 레벨)
        [[nodiscard]] double GetLevelRatio(double slope, int level) noexcept
        {
            const int clamped{ std::clamp(level, Models::MIN_VALUE, Models::MAX_PASSIVE_LEVEL) };
            return std::max(1.0 - slope * static_cast<double>(Models::MAX_PASSIVE_LEVEL - clamped), MIN_RATIO);
        }

        // 표 배율 두 개의 비 (둘 중 하나라도 없으면 1)
        [[nodiscard]] double GetKeyedRatio(std::span<const Data::KeyedValue> table, std::string_view currentKey, std::string_view targetKey)
        {
            const Data::KeyedValue* currentValue{ Data::FindKeyed(table, currentKey) };
            const Data::KeyedValue* targetValue{ Data::FindKeyed(table, targetKey) };
            if (currentValue == nullptr || targetValue == nullptr || currentValue->m_value <= 0.0) return NO_CHANGE_FACTOR;
            return targetValue->m_value / currentValue->m_value;
        }
    }

    // ===================== 악세서리 =====================

    CombatFactorKind AccessoryFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_ACCESSORY;
    }

    double AccessoryFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        const double weaponBase{ GetWeaponBase(current) };
        const bool isEsther{ current.m_extra.m_isEstherWeapon };

        double lnChange{};
        for (std::size_t slot{}; slot < Models::ACCESSORY_SLOT_COUNT; ++slot)
        {
            for (std::size_t option{}; option < Models::ACCESSORY_OPTION_COUNT; ++option)
            {
                lnChange += GetAccessoryLn(target.m_extra.m_accessories[slot].m_options[option], weaponBase, isEsther)
                    - GetAccessoryLn(current.m_extra.m_accessories[slot].m_options[option], weaponBase, isEsther);
            }
        }
        return std::exp(lnChange);
    }

    // ===================== 완갑 =====================

    CombatFactorKind VambraceFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_VAMBRACE;
    }

    double VambraceFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        const Models::CombatExtra::VambraceGain gain{ Models::CombatExtra::GetVambraceGain(current.m_extra, target.m_extra) };

        // 기본 공격력 A = sqrt(S0 × W0 / 6) × (1 + 보석 기본 공격력 %) × 보정 → 배율 (A + ΔB) / A (S0 · W0를 모르면 ΔB는 계산하지 않음)
        const double weaponBase{ GetWeaponBase(current) };
        double baseAttackFactor{ NO_CHANGE_FACTOR };
        if (current.m_totalMainStat > 0.0 && weaponBase > 0.0)
        {
            const double baseAttack{ std::sqrt(current.m_totalMainStat * weaponBase / ATTACK_DIVISOR) * (1.0 + current.m_gemBaseAttackRate) * Data::GetBaseAttackCalibration() };
            if (baseAttack > 0.0) baseAttackFactor = std::max(baseAttack + gain.m_baseAttack, 0.0) / baseAttack;
        }
        return baseAttackFactor * gain.m_gradeFactor;
    }

    // ===================== 팔찌 =====================

    CombatFactorKind BraceletFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_BRACELET;
    }

    double BraceletFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        // 치명 · 특화 · 신속: 포인트 1당 같은 비율로 증가
        const double statFactor{ std::max(1.0 + Data::GetBraceletStatRatio() * (SumEffectiveStats(target.m_extra) - SumEffectiveStats(current.m_extra)), MIN_RATIO) };

        const double weaponBase{ GetWeaponBase(current) };
        double lnChange{};
        for (std::size_t option{}; option < Models::BRACELET_OPTION_COUNT; ++option)
        {
            lnChange += GetBraceletLn(target.m_extra.m_braceletOptions[option], weaponBase) - GetBraceletLn(current.m_extra.m_braceletOptions[option], weaponBase);
        }
        return statFactor * std::exp(lnChange);
    }

    // ===================== 스톤 · 각인서 =====================

    CombatFactorKind EngravingFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_ENGRAVING;
    }

    double EngravingFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        double lnChange{ GetBooksLn(target.m_extra) - GetBooksLn(current.m_extra) };

        // 스톤: 현재나 목표 조합이 표에 없으면 스톤은 계산하지 않음
        const std::optional<double> currentStone{ GetStoneLn(current.m_extra) };
        const std::optional<double> targetStone{ GetStoneLn(target.m_extra) };
        if (currentStone.has_value() && targetStone.has_value()) lnChange += *targetStone - *currentStone;
        return std::exp(lnChange);
    }

    // ===================== 아바타 =====================

    CombatFactorKind AvatarFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_AVATAR;
    }

    double AvatarFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        const auto getLn{ [](Models::AvatarGrade grade)
        {
            const Data::KeyedValue* found{ Data::FindKeyed(Data::GetAvatarLn(), GetAvatarKey(grade)) };
            return found != nullptr ? found->m_value : 0.0;
        } };

        double lnChange{};
        for (std::size_t slot{}; slot < Models::AVATAR_SLOT_COUNT; ++slot)
        {
            lnChange += getLn(target.m_extra.m_avatars[slot]) - getLn(current.m_extra.m_avatars[slot]);
        }
        return std::exp(lnChange);
    }

    // ===================== 아크 그리드 코어 =====================

    CombatFactorKind ArkCoreFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_ARK_CORE;
    }

    double ArkCoreFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        return std::exp(GetCoresLn(target.m_extra) - GetCoresLn(current.m_extra));
    }

    // ===================== 아크 그리드 젬 =====================

    CombatFactorKind ArkGemFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_ARK_GEM;
    }

    double ArkGemFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        double lnChange{};
        for (std::size_t effect{}; effect < Models::ARK_GEM_EFFECT_COUNT; ++effect)
        {
            const Data::KeyedValue* perLevel{ Data::FindKeyed(Data::GetArkGemPerLevel(), Models::ARK_GEM_EFFECT_NAMES[effect]) };
            if (perLevel == nullptr) continue;
            lnChange += perLevel->m_value * static_cast<double>(target.m_extra.m_arkGemLevels[effect] - current.m_extra.m_arkGemLevels[effect]);
        }
        return std::exp(lnChange);
    }

    // ===================== 아크 패시브 =====================

    CombatFactorKind ArkPassiveFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_ARK_PASSIVE;
    }

    double ArkPassiveFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        const Models::ArkPassiveState& from{ current.m_extra.m_passive };
        const Models::ArkPassiveState& to{ target.m_extra.m_passive };

        // 랭크 (0 ~ 6 표)
        double factor{ NO_CHANGE_FACTOR };
        const std::span<const double> ranks{ Data::GetPassiveRankRatios() };
        const auto rankIndex{ [&ranks](int rank) { return static_cast<std::size_t>(std::clamp(rank, Models::MIN_VALUE, static_cast<int>(ranks.size()) - 1)); } };
        if (!ranks.empty() && ranks[rankIndex(from.m_rank)] > 0.0) factor *= ranks[rankIndex(to.m_rank)] / ranks[rankIndex(from.m_rank)];

        // 깨달음 포인트 구성
        factor *= GetKeyedRatio(Data::GetEnlightenmentRatios(), from.m_enlightenment, to.m_enlightenment);

        // 깨달음 · 도약 레벨 (진화 레벨은 효과 없음)
        const Data::PassiveSlopes& slopes{ Data::GetPassiveSlopes() };
        factor *= GetLevelRatio(slopes.m_level1, to.m_levels[PASSIVE_ENLIGHTENMENT_TREE]) / GetLevelRatio(slopes.m_level1, from.m_levels[PASSIVE_ENLIGHTENMENT_TREE]);
        factor *= GetLevelRatio(slopes.m_level2, to.m_levels[PASSIVE_LEAP_TREE]) / GetLevelRatio(slopes.m_level2, from.m_levels[PASSIVE_LEAP_TREE]);
        return factor;
    }
}
