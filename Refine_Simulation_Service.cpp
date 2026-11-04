module LostArkMultiTool.Core.RefineSimulationService;

import LostArkMultiTool.Models.RefineData;

namespace LostArkMultiTool::Core
{
    namespace
    {
        using Models::RefineData::EquipmentState;
        using Models::RefineData::EquipmentTier;

        // 프리셋 상태 (로아업 통합 재련과 같음)
        constexpr EquipmentState PRESET_1700_STATE{ EquipmentTier::TIER_AEGIR, 18, 20 };
        constexpr EquipmentState PRESET_1730_STATE{ EquipmentTier::TIER_SERCA, 11, 0 };
        constexpr EquipmentState PRESET_1750_STATE{ EquipmentTier::TIER_SERCA, 15, 0 };
    }

    EquipmentState GetStartPresetState(RefineStartPreset preset) noexcept
    {
        switch (preset)
        {
        case RefineStartPreset::PRESET_1700: return PRESET_1700_STATE;
        case RefineStartPreset::PRESET_1750: return PRESET_1750_STATE;
        case RefineStartPreset::PRESET_1730: break;
        }
        return PRESET_1730_STATE;
    }

    RefineSimulationService::~RefineSimulationService() = default;
}
