export module LostArkMultiTool.Core.CombatPowerCalculator;  // 전투력 계산기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.CombatFactorComponent;  // 배율 요소 모듈 가져오기
import LostArkMultiTool.Models.CombatState;  // 전투력 상태 모듈 가져오기

export namespace LostArkMultiTool::Core  // 코어 네임스페이스 내보내기 시작
{
    // 배율 요소 하나의 결과
    struct CombatFactorResult
    {
        CombatFactorKind m_kind{};  // 배율 요소 종류
        double m_factor{ 1.0 };  // 배율 값 (기본 1.0 = 변화 없음)
    };

    // 예상 전투력 계산 결과
    struct CombatPowerResult
    {
        double m_currentPower{};             // API 전투력 (없으면 0)
        double m_predictedPower{};           // 현재 × 모든 배율
        double m_delta{};                    // 예상 - 현재
        double m_totalFactor{ 1.0 };         // 모든 배율의 곱
        double m_currentItemLevel{};  // 현재 아이템 레벨
        double m_predictedItemLevel{};       // 6부위 평균 (현재 평균 + 바뀐 레벨 ÷ 6)
        std::vector<CombatFactorResult> m_factors;   // 요소별 배율 (추가한 순서)
    };

    // 예상 전투력 계산기 (배율 요소들을 소유하고 모두 곱함, 화면 · API를 모르는 순수 계산)
    class CombatPowerCalculator  // 예상 전투력 계산기 클래스
    {
    public:  // 공개 멤버 시작
        CombatPowerCalculator();   // 기본 요소: 장비 재련 · 무기 품질 · 보석 · 악세서리 · 팔찌 · 스톤/각인서 · 아바타 · 아크 코어 · 아크 젬 · 아크 패시브
        ~CombatPowerCalculator();  // 소멸자 (unique_ptr 요소 해제를 위해 cpp에서 정의)

        CombatPowerCalculator(const CombatPowerCalculator&) = delete;  // 복사 생성 금지 (요소 소유권 유지)
        CombatPowerCalculator& operator=(const CombatPowerCalculator&) = delete;  // 복사 대입 금지

        // 요소 추가 (팔찌 · 악세 같은 요소를 나중에 붙일 수 있게)
        void AddComponent(std::unique_ptr<CombatFactorComponent> component);

        [[nodiscard]] CombatPowerResult Calculate(double currentPower, const Models::CombatState& current, const Models::CombatState& target) const;  // 현재 상태와 목표 상태로 예상 전투력 계산

    private:
        std::vector<std::unique_ptr<CombatFactorComponent>> m_components;  // 배율 요소 목록 (추가한 순서대로 소유)
    };
}
