module LostArkMultiTool.Core.CombatPowerCalculator;  // 전투력 계산기 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.CombatExtraComponent;  // 추가 장착 요소(악세 · 팔찌 등) 모듈 가져오기
import LostArkMultiTool.Core.CombatFactorComponent;  // 배율 요소 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 모듈 가져오기
import LostArkMultiTool.Models.CombatState;  // 전투력 상태 모듈 가져오기

namespace LostArkMultiTool::Core  // 코어 네임스페이스 시작
{
    CombatPowerCalculator::CombatPowerCalculator()  // 생성자: 기본 배율 요소들을 순서대로 등록
    {
        AddComponent(std::make_unique<GearFactorComponent>());  // 장비 재련 요소
        AddComponent(std::make_unique<WeaponQualityComponent>());  // 무기 품질 요소
        AddComponent(std::make_unique<GemFactorComponent>());  // 보석 요소
        AddComponent(std::make_unique<VambraceFactorComponent>());  // 완갑 요소
        AddComponent(std::make_unique<AccessoryFactorComponent>());  // 악세서리 요소
        AddComponent(std::make_unique<BraceletFactorComponent>());  // 팔찌 요소
        AddComponent(std::make_unique<EngravingFactorComponent>());  // 스톤 · 각인서 요소
        AddComponent(std::make_unique<AvatarFactorComponent>());  // 아바타 요소
        AddComponent(std::make_unique<ArkCoreFactorComponent>());  // 아크 코어 요소
        AddComponent(std::make_unique<ArkGemFactorComponent>());  // 아크 젬 요소
        AddComponent(std::make_unique<ArkPassiveFactorComponent>());  // 아크 패시브 요소
    }

    CombatPowerCalculator::~CombatPowerCalculator() = default;  // 소멸자 기본 구현

    void CombatPowerCalculator::AddComponent(std::unique_ptr<CombatFactorComponent> component)  // 배율 요소 추가
    {
        if (component) m_components.push_back(std::move(component));  // 빈 포인터가 아니면 소유권을 이동해 보관
    }

    // 요소별 배율을 곱해서 예상 전투력 + 6부위 평균 아이템 레벨 변화
    CombatPowerResult CombatPowerCalculator::Calculate(double currentPower, const Models::CombatState& current, const Models::CombatState& target) const
    {
        CombatPowerResult result;  // 결과 구조체 준비
        result.m_currentPower = currentPower;  // 현재 전투력 기록
        result.m_factors.reserve(m_components.size());  // 요소 개수만큼 메모리 미리 확보
        for (const std::unique_ptr<CombatFactorComponent>& component : m_components)  // 모든 배율 요소를 순회
        {
            const double factor{ component->GetFactor(current, target) };  // 현재 → 목표 상태의 요소 배율 계산
            result.m_factors.push_back(CombatFactorResult{ component->GetKind(), factor });  // 요소 종류와 배율을 결과에 기록
            result.m_totalFactor *= factor;  // 전체 배율에 곱함
        }
        result.m_predictedPower = currentPower * result.m_totalFactor;  // 예상 전투력 = 현재 × 전체 배율
        result.m_delta = result.m_predictedPower - currentPower;  // 증감량 = 예상 - 현재

        // 아이템 레벨: 표가 있는 부위만 (일반 × 5 + 상급 × 1) 차이를 6부위 평균에 더함
        int levelGain{};  // 6부위 레벨 증감 합계
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 모든 장비 부위를 순회
        {
            const Models::GearState& currentGear{ current.m_gears[index] };  // 현재 부위 장비 상태
            const Models::GearState& targetGear{ target.m_gears[index] };  // 목표 부위 장비 상태
            if (!currentGear.IsKnown() || currentGear.m_tableKey != targetGear.m_tableKey) continue;  // 표를 모르거나 표가 바뀐 부위는 건너뜀
            levelGain += Models::CombatGear::GetItemLevelGain(targetGear) - Models::CombatGear::GetItemLevelGain(currentGear);  // 목표와 현재의 아이템 레벨 증가분 차이를 누적
        }
        result.m_currentItemLevel = current.m_itemLevel;  // 현재 아이템 레벨 기록
        result.m_predictedItemLevel = current.m_itemLevel + static_cast<double>(levelGain) / static_cast<double>(Models::GEAR_PART_COUNT);  // 예상 아이템 레벨 = 현재 + 증감 ÷ 6부위
        return result;  // 계산 결과 반환
    }
}
