export module LostArkMultiTool.Services.CombatSimulatorStore;

import std;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Services.JSONFileStore;

export namespace LostArkMultiTool::Services
{
    // 저장된 부위 목표 하나
    struct CombatGearTarget
    {
        int m_enhancement{};
        int m_advancedEnhancement{};
    };

    // 전투력 시뮬레이터 설정 (마지막으로 조회한 캐릭터 + 그 캐릭터의 목표)
    struct CombatSimulatorSettings
    {
        std::string m_characterName;                                                    // UTF-8
        std::array<std::optional<CombatGearTarget>, Models::GEAR_PART_COUNT> m_gearTargets;   // 부위별 목표 (없으면 현재 유지)
        std::optional<int> m_weaponQualityTarget;                                       // 무기 품질 목표
        std::vector<int> m_gemTargets;                                                  // 보석별 목표 레벨 (장착 순서, 비어 있으면 현재 유지)
        std::optional<Models::ExtraState> m_extraTarget;                                // 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 그리드 · 아크 패시브 · 완갑 목표 (없으면 현재 유지)
    };

    // 전투력 시뮬레이터 저장소 (CombatSimulatorData.json, 사용자 데이터 폴더)
    // 형식: { "CharacterName": "이름", "Targets": [ { "Part": "무기", "Enhancement": 20, "Advanced": 40 }, ... ],
    //         "WeaponQuality": 100, "GemTargets": [ 10, 10, 9, ... ], "Extra": { "Accessories": [...], "BraceletStats": [...], ... } }
    //         (WeaponQuality · GemTargets · Extra는 없으면 현재 유지, Extra의 빠진 칸은 현재 값 그대로)
    class CombatSimulatorStore
    {
    public:
        CombatSimulatorStore();

        [[nodiscard]] const CombatSimulatorSettings& GetSettings() const noexcept;
        [[nodiscard]] CombatSimulatorSettings& GetSettings() noexcept;
        [[nodiscard]] const std::optional<std::string>& GetLoadWarning() const noexcept;

        void Save();   // 실패하면 예외 (받는 쪽이 안내)

    private:
        void Load();   // 저장 파일 → 설정 (없거나 손상이면 기본값)

        JSONFileStore m_fileStore;
        CombatSimulatorSettings m_settings;
        std::optional<std::string> m_loadWarning;
    };
}
