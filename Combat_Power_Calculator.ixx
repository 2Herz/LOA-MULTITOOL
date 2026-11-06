export module LostArkMultiTool.Core.CombatPowerCalculator;

import std;
import LostArkMultiTool.Core.CombatFactorComponent;
import LostArkMultiTool.Models.CombatState;

export namespace LostArkMultiTool::Core
{
    // 배율 요소 하나의 결과
    struct CombatFactorResult
    {
        CombatFactorKind m_kind{};
        double m_factor{ 1.0 };
    };

    // 예상 전투력 계산 결과
    struct CombatPowerResult
    {
        double m_currentPower{};             // API 전투력 (없으면 0)
        double m_predictedPower{};           // 현재 × 모든 배율
        double m_delta{};                    // 예상 - 현재
        double m_totalFactor{ 1.0 };         // 모든 배율의 곱
        double m_currentItemLevel{};
        double m_predictedItemLevel{};       // 6부위 평균 (현재 평균 + 바뀐 레벨 ÷ 6)
        std::vector<CombatFactorResult> m_factors;   // 요소별 배율 (추가한 순서)
    };

    // 예상 전투력 계산기 (배율 요소들을 소유하고 모두 곱함, 화면 · API를 모르는 순수 계산)
    class CombatPowerCalculator
    {
    public:
        CombatPowerCalculator();   // 기본 요소: 장비 재련 · 무기 품질 · 보석 · 악세서리 · 팔찌 · 스톤/각인서 · 아바타 · 아크 코어 · 아크 젬 · 아크 패시브
        ~CombatPowerCalculator();

        CombatPowerCalculator(const CombatPowerCalculator&) = delete;
        CombatPowerCalculator& operator=(const CombatPowerCalculator&) = delete;

        // 요소 추가 (팔찌 · 악세 같은 요소를 나중에 붙일 수 있게)
        void AddComponent(std::unique_ptr<CombatFactorComponent> component);

        [[nodiscard]] CombatPowerResult Calculate(double currentPower, const Models::CombatState& current, const Models::CombatState& target) const;

    private:
        std::vector<std::unique_ptr<CombatFactorComponent>> m_components;
    };
}
