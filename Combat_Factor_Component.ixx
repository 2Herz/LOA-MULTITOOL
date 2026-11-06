export module LostArkMultiTool.Core.CombatFactorComponent;

import std;
import LostArkMultiTool.Models.CombatState;

export namespace LostArkMultiTool::Core
{
    // 전투력 배율 요소 종류 (화면의 배율 내역 순서)
    enum class CombatFactorKind
    {
        FACTOR_GEAR,             // 장비 재련 (주스탯 · 무기 공격력)
        FACTOR_WEAPON_QUALITY,   // 무기 품질
        FACTOR_GEM,              // 보석 레벨
        FACTOR_VAMBRACE,         // 완갑 기본 공격력 (무기 공격력 · 주스탯은 장비 재련에 합산)
        FACTOR_ACCESSORY,        // 악세서리 연마 옵션
        FACTOR_BRACELET,         // 팔찌 스탯 · 옵션
        FACTOR_ENGRAVING,        // 어빌리티 스톤 · 각인서
        FACTOR_AVATAR,           // 아바타
        FACTOR_ARK_CORE,         // 아크 그리드 코어 (등급 · 포인트)
        FACTOR_ARK_GEM,          // 아크 그리드 젬 효과
        FACTOR_ARK_PASSIVE       // 아크 패시브
    };

    // 전투력 배율 요소 하나 (전략 패턴 · 컴포넌트: 계산기는 이 인터페이스만 보고 곱함)
    // 현재 → 목표로 바뀔 때 전투력이 몇 배가 되는지만 계산 (화면 · API를 모름, 순수 계산)
    class CombatFactorComponent
    {
    public:
        virtual ~CombatFactorComponent();

        [[nodiscard]] virtual CombatFactorKind GetKind() const noexcept = 0;

        // 목표 전투력 ÷ 현재 전투력 (계산할 수 없으면 1 = 영향 없음)
        [[nodiscard]] virtual double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const = 0;
    };

    // 장비 재련: CP ∝ √(S × W)
    // S' = S0 + Σ방어구 (목표 능력치 - 현재 능력치), W' = W0 + (목표 무기 공격력 - 현재), W0 = 무기 툴팁 공격력 + 보정
    // 배율 = √(S'/S0 × W'/W0) (현재 능력치도 계수 표 값을 써서 두 값의 기준을 맞춤)
    // S'에는 악세서리 주스탯 · 팔찌 힘/민첩/지능 · 완갑 주스탯 차이를, W'에는 완갑 무기 공격력 차이도 함께 더함
    class GearFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 무기 품질: F(q) = A + B × (q/100)² → 배율 = F(목표) / F(현재) (에스더 무기는 품질이 없어서 1)
    class WeaponQualityComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 보석: 보석마다 F(목표 레벨) / F(현재 레벨)을 모두 곱함 (레벨 0 = 보석 없음)
    class GemFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };
}
