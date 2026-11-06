module LostArkMultiTool.Core.CombatPowerCalculator;

import std;
import LostArkMultiTool.Core.CombatExtraComponent;
import LostArkMultiTool.Core.CombatFactorComponent;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatState;

namespace LostArkMultiTool::Core
{
    CombatPowerCalculator::CombatPowerCalculator()
    {
        AddComponent(std::make_unique<GearFactorComponent>());
        AddComponent(std::make_unique<WeaponQualityComponent>());
        AddComponent(std::make_unique<GemFactorComponent>());
        AddComponent(std::make_unique<VambraceFactorComponent>());
        AddComponent(std::make_unique<AccessoryFactorComponent>());
        AddComponent(std::make_unique<BraceletFactorComponent>());
        AddComponent(std::make_unique<EngravingFactorComponent>());
        AddComponent(std::make_unique<AvatarFactorComponent>());
        AddComponent(std::make_unique<ArkCoreFactorComponent>());
        AddComponent(std::make_unique<ArkGemFactorComponent>());
        AddComponent(std::make_unique<ArkPassiveFactorComponent>());
    }

    CombatPowerCalculator::~CombatPowerCalculator() = default;

    void CombatPowerCalculator::AddComponent(std::unique_ptr<CombatFactorComponent> component)
    {
        if (component) m_components.push_back(std::move(component));
    }

    // 요소별 배율을 곱해서 예상 전투력 + 6부위 평균 아이템 레벨 변화
    CombatPowerResult CombatPowerCalculator::Calculate(double currentPower, const Models::CombatState& current, const Models::CombatState& target) const
    {
        CombatPowerResult result;
        result.m_currentPower = currentPower;
        result.m_factors.reserve(m_components.size());
        for (const std::unique_ptr<CombatFactorComponent>& component : m_components)
        {
            const double factor{ component->GetFactor(current, target) };
            result.m_factors.push_back(CombatFactorResult{ component->GetKind(), factor });
            result.m_totalFactor *= factor;
        }
        result.m_predictedPower = currentPower * result.m_totalFactor;
        result.m_delta = result.m_predictedPower - currentPower;

        // 아이템 레벨: 표가 있는 부위만 (일반 × 5 + 상급 × 1) 차이를 6부위 평균에 더함
        int levelGain{};
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)
        {
            const Models::GearState& currentGear{ current.m_gears[index] };
            const Models::GearState& targetGear{ target.m_gears[index] };
            if (!currentGear.IsKnown() || currentGear.m_tableKey != targetGear.m_tableKey) continue;
            levelGain += Models::CombatGear::GetItemLevelGain(targetGear) - Models::CombatGear::GetItemLevelGain(currentGear);
        }
        result.m_currentItemLevel = current.m_itemLevel;
        result.m_predictedItemLevel = current.m_itemLevel + static_cast<double>(levelGain) / static_cast<double>(Models::GEAR_PART_COUNT);
        return result;
    }
}
