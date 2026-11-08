export module LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태(ExtraState) 모듈 가져오기

export namespace LostArkMultiTool::Models  // 모델 네임스페이스 내보내기 시작
{
    // 전투력 시뮬레이터가 다루는 장비 부위 (표 행 순서 = 이 순서)
    enum class GearPart : std::size_t
    {
        PART_WEAPON,  // 무기 부위
        PART_HELMET,  // 투구 부위
        PART_SHOULDER,  // 어깨 부위
        PART_CHEST,  // 상의 부위
        PART_PANTS,  // 하의 부위
        PART_GLOVES,  // 장갑 부위
        PART_COUNT  // 부위 개수 (배열 크기 계산용 끝 표식)
    };

    inline constexpr std::size_t GEAR_PART_COUNT{ static_cast<std::size_t>(GearPart::PART_COUNT) };  // 부위 개수 상수 (열거형 끝 표식에서 계산)

    // 장비 하나 (API ArmoryEquipment 한 항목 + 툴팁에서 뽑은 값)
    struct GearItem
    {
        std::string m_type;                 // "무기" · "투구" ... (UTF-8)
        std::string m_name;                 // "+18 운명의 업화 견갑"
        std::string m_grade;                // "유물" · "고대" · "에스더"
        int m_enhancement{};                // 일반 재련 단계 (이름 앞 +N)
        int m_advancedEnhancement{};        // 상급 재련 단계 ("[상급 재련] 31단계", 없으면 0)
        std::optional<int> m_quality;       // 품질 (툴팁 qualityValue, 없거나 -1이면 nullopt)
        double m_mainStat{};                // 기본 효과의 힘 · 민첩 · 지능 (없으면 0)
        double m_weaponAttack{};            // 기본 효과의 무기 공격력 (무기만, 없으면 0)
    };

    // 캐릭터 무기고 (API /armories/characters/{이름} → profiles + equipment + gems)
    struct CharacterArmory
    {
        std::string m_characterName;  // 캐릭터 이름
        std::string m_className;  // 직업 이름
        double m_itemAvgLevel{};                                     // "1,764.17" → 1764.17
        std::optional<double> m_combatPower;                         // API 전투력 (없으면 nullopt)
        std::array<std::optional<GearItem>, GEAR_PART_COUNT> m_gears; // 부위별 장비 (안 낀 부위는 nullopt)
        double m_totalMainStat{};                                    // 모든 장비(방어구 · 악세 등) 기본 효과 주스탯 합 (S0)
        std::vector<int> m_gemLevels;                                // 장착한 보석 레벨들
        double m_gemBaseAttackRate{};                                // 보석 기본 공격력 총합 ("기본 공격력 총합 : 6.60%" → 0.066, 없으면 0)
        std::vector<std::string> m_gemKinds;                         // 보석 종류 ("겁화" · "작열" ..., 모르면 빈 문자열, m_gemLevels와 같은 순서)
        ExtraState m_extra;                                          // 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 그리드 · 아크 패시브 · 완갑 (API 현재 값)
    };

    // API 장비 Type 글자("무기" · "투구" ...) → 부위 (전투력 시뮬레이터 대상이 아니면 nullopt)
    [[nodiscard]] std::optional<GearPart> FindGearPart(std::string_view type) noexcept;

    // 부위 → API Type 글자 (저장 파일 키로도 사용)
    [[nodiscard]] std::string_view GetGearPartType(GearPart part);

    // 부위 → 계수 표 분류 키의 부위 이름 ("투구" → "머리")
    [[nodiscard]] std::string_view GetGearTableSuffix(GearPart part);

    // 부위 ↔ 배열 번호 (범위를 벗어나면 std::out_of_range → 경계 검사)
    [[nodiscard]] std::size_t ToGearIndex(GearPart part);
    [[nodiscard]] GearPart ToGearPart(std::size_t index);
}
