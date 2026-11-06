module LostArkMultiTool.Core.CombatFactorComponent;

import std;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Models.CombatState;

namespace LostArkMultiTool::Core
{
    namespace
    {
        namespace Data = Models::CombatCoefficientData;

        constexpr double NO_CHANGE_FACTOR{ 1.0 };   // 바뀐 것이 없거나 계산할 수 없음

        // 현재 · 목표 모두 같은 표로 조회한 능력치 차이 (분류가 다르거나 모르는 칸이면 0)
        [[nodiscard]] double GetStatGain(const Models::GearState& current, const Models::GearState& target)
        {
            if (!current.IsKnown() || current.m_tableKey != target.m_tableKey) return 0.0;

            const std::optional<double> currentStat{ Models::CombatGear::GetStat(current) };
            const std::optional<double> targetStat{ Models::CombatGear::GetStat(target) };
            if (!currentStat.has_value() || !targetStat.has_value()) return 0.0;
            return *targetStat - *currentStat;
        }

        // 기준값에 차이를 더한 비율 (기준이 없으면 1)
        [[nodiscard]] double GetRatio(double baseValue, double gain) noexcept
        {
            if (baseValue <= 0.0) return NO_CHANGE_FACTOR;
            return std::max(baseValue + gain, 0.0) / baseValue;
        }

        // 품질 곡선 F(q) (0 ~ 최대로 맞춤)
        [[nodiscard]] double GetQualityValue(int quality) noexcept
        {
            const Data::QualityCurve& curve{ Data::GetWeaponQualityCurve() };
            const double ratio{ std::clamp(static_cast<double>(quality), 0.0, curve.m_maxQuality) / curve.m_maxQuality };
            return curve.m_base + curve.m_scale * ratio * ratio;
        }

        // 보석 레벨 계수 F(level) (표 범위(6 ~ 10) 밖의 레벨 = nullopt)
        [[nodiscard]] std::optional<double> GetGemValue(int level) noexcept
        {
            const std::span<const double> factors{ Data::GetGemFactors() };
            const int index{ level - Data::GetGemFirstLevel() };   // 표 번호 = 레벨 - 첫 레벨
            if (index < 0 || static_cast<std::size_t>(index) >= factors.size()) return std::nullopt;
            return factors[static_cast<std::size_t>(index)];
        }
    }

    CombatFactorComponent::~CombatFactorComponent() = default;

    // ===================== 장비 재련 =====================

    CombatFactorKind GearFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_GEAR;
    }

    double GearFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        double mainStatGain{};
        double weaponAttackGain{};
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)
        {
            const double gain{ GetStatGain(current.m_gears[index], target.m_gears[index]) };
            if (Models::ToGearPart(index) == Models::GearPart::PART_WEAPON) weaponAttackGain += gain;
            else mainStatGain += gain;
        }

        // 악세서리 · 팔찌 · 완갑이 바꾸는 주스탯도 같은 S에 더함 (함께 올라야 제곱근이 맞게 합쳐짐)
        mainStatGain += Models::CombatExtra::GetMainStatGain(current.m_extra, target.m_extra);
        const Models::CombatExtra::VambraceGain vambrace{ Models::CombatExtra::GetVambraceGain(current.m_extra, target.m_extra) };
        mainStatGain += vambrace.m_mainStat;
        weaponAttackGain += vambrace.m_weaponAttack;

        // 무기 툴팁 공격력을 모르면 무기 쪽은 계산하지 않음 (보정값만으로는 기준이 맞지 않음)
        const double weaponBase{ Data::GetWeaponBase(current.m_weaponAttack) };
        const double product{ GetRatio(current.m_totalMainStat, mainStatGain) * GetRatio(weaponBase, weaponAttackGain) };
        return std::sqrt(product);
    }

    // ===================== 무기 품질 =====================

    CombatFactorKind WeaponQualityComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_WEAPON_QUALITY;
    }

    double WeaponQualityComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        const std::size_t weaponIndex{ Models::ToGearIndex(Models::GearPart::PART_WEAPON) };
        if (!current.m_weaponQuality.has_value() || !target.m_weaponQuality.has_value()) return NO_CHANGE_FACTOR;
        if (Models::CombatGear::IsEsther(current.m_gears[weaponIndex].m_tableKey)) return NO_CHANGE_FACTOR;

        const double currentValue{ GetQualityValue(*current.m_weaponQuality) };
        if (currentValue <= 0.0) return NO_CHANGE_FACTOR;
        return GetQualityValue(*target.m_weaponQuality) / currentValue;
    }

    // ===================== 보석 =====================

    CombatFactorKind GemFactorComponent::GetKind() const noexcept
    {
        return CombatFactorKind::FACTOR_GEM;
    }

    double GemFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const
    {
        // 같은 번호의 보석끼리 비교 (목표 목록이 짧으면 그만큼만)
        // 현재 · 목표 레벨 중 하나라도 표(6 ~ 10)에 없으면 그 보석은 배율 1 (6레벨 미만 보석은 계산하지 않음)
        double factor{ NO_CHANGE_FACTOR };
        const std::size_t count{ std::min(current.m_gemLevels.size(), target.m_gemLevels.size()) };
        for (std::size_t index{}; index < count; ++index)
        {
            const std::optional<double> currentValue{ GetGemValue(current.m_gemLevels[index]) };
            const std::optional<double> targetValue{ GetGemValue(target.m_gemLevels[index]) };
            if (!currentValue.has_value() || !targetValue.has_value() || *currentValue <= 0.0) continue;
            factor *= *targetValue / *currentValue;
        }
        return factor;
    }
}
