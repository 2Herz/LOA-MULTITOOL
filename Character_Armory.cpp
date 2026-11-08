module LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기

namespace LostArkMultiTool::Models  // 모델 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 영역 시작
    {
        // 부위 순서(GearPart)와 같은 순서의 API Type 글자 · 계수 표 부위 이름
        constexpr std::array<std::string_view, GEAR_PART_COUNT> PART_TYPES{ "무기", "투구", "어깨", "상의", "하의", "장갑" };  // API Type 글자 표 (GearPart 순서와 동일)
        constexpr std::array<std::string_view, GEAR_PART_COUNT> TABLE_SUFFIXES{ "무기", "머리", "어깨", "상의", "하의", "장갑" };  // 계수 표 부위 이름 표 (투구는 "머리")
    }

    std::optional<GearPart> FindGearPart(std::string_view type) noexcept  // API Type 글자로 부위 찾기
    {
        const auto found{ std::ranges::find(PART_TYPES, type) };  // 표에서 글자와 같은 항목 검색
        if (found == PART_TYPES.end()) return std::nullopt;  // 못 찾으면 전투력 시뮬레이터 대상이 아니므로 빈 값 반환
        return static_cast<GearPart>(std::distance(PART_TYPES.begin(), found));  // 찾은 위치를 부위 열거형으로 변환
    }

    std::string_view GetGearPartType(GearPart part)  // 부위의 API Type 글자 반환
    {
        return PART_TYPES.at(ToGearIndex(part));  // 경계 검사를 포함한 at()으로 글자 조회
    }

    std::string_view GetGearTableSuffix(GearPart part)  // 부위의 계수 표 부위 이름 반환
    {
        return TABLE_SUFFIXES.at(ToGearIndex(part));  // 경계 검사를 포함한 at()으로 이름 조회
    }

    std::size_t ToGearIndex(GearPart part)  // 부위를 배열 번호로 변환
    {
        const std::size_t index{ static_cast<std::size_t>(part) };  // 열거형 값을 정수 번호로 변환
        if (index >= GEAR_PART_COUNT) throw std::out_of_range{ "장비 부위 번호가 범위를 벗어났습니다." };  // 범위를 벗어나면 예외로 알림 (경계 검사)
        return index;  // 검증된 번호 반환
    }

    GearPart ToGearPart(std::size_t index)  // 배열 번호를 부위로 변환
    {
        if (index >= GEAR_PART_COUNT) throw std::out_of_range{ "장비 부위 번호가 범위를 벗어났습니다." };  // 범위를 벗어나면 예외로 알림 (경계 검사)
        return static_cast<GearPart>(index);  // 검증된 번호를 열거형으로 변환
    }
}
