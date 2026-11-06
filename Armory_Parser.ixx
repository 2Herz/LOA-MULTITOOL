export module LostArkMultiTool.Services.ArmoryParser;

import std;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Services.JSONValue;

export namespace LostArkMultiTool::Services
{
    // 무기고 응답 해석 (API /armories/characters/{이름}?filters=profiles+equipment+gems)
    // ArmoryProfile · ArmoryEquipment · ArmoryGem 만 읽음, 장비 툴팁(JSON 문자열)은 태그를 지운 글자에서 값을 찾음
    // 없는 캐릭터(응답이 null · 프로필 없음)면 std::runtime_error ("캐릭터를 찾을 수 없어요")
    [[nodiscard]] Models::CharacterArmory ParseArmory(const JSONValue& root);

    // 장비 툴팁 하나에서 재련 · 품질 · 능력치 읽기 (isWeapon이면 무기 공격력도, item.m_type이 "팔찌"면 툴팁 전체의 주스탯 합)
    void ParseGearTooltip(std::string_view tooltip, bool isWeapon, Models::GearItem& item);
}
