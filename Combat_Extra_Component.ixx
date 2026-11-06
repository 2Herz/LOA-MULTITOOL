export module LostArkMultiTool.Core.CombatExtraComponent;

import std;
import LostArkMultiTool.Core.CombatFactorComponent;
import LostArkMultiTool.Models.CombatState;

export namespace LostArkMultiTool::Core
{
    // 악세서리 연마 옵션: 슬롯마다 옵션 3줄의 ln을 합산 → 배율 = exp(목표 합 - 현재 합)
    // 문구 표(적에게 주는 피해 · 추가 피해 · 공격력 · 치명) + 무기 공격력 옵션은 식 (W0 · 장비 종류에 따라 달라짐)
    // 악세서리 기본 효과 주스탯은 장비 재련(GearFactorComponent)의 S에 더해짐
    class AccessoryFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 완갑: 기본 공격력 고정값 차이는 (A + ΔB) / A, 유물 → 고대 등급 업은 한 번 배율 (무기 공격력 · 주스탯은 장비 재련에 합산)
    class VambraceFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 팔찌: 치명 · 특화 · 신속 포인트 합은 선형 (1 + K × 포인트 차이), 옵션 줄은 표 (무기 공격력 포함 줄은 식)
    // 힘 · 민첩 · 지능 줄은 장비 재련의 S에 더해짐
    class BraceletFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 어빌리티 스톤(두 줄 조합 표) + 각인서 5개(이름별 레벨 선형, 전설 감점)
    class EngravingFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 아바타: 부위마다 전설 / 영웅 ln을 합산
    class AvatarFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 아크 그리드 코어: 코어마다 고대 등급 추가 ln + 의지력 포인트 표
    class ArkCoreFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 아크 그리드 젬 효과: 효과 종류별 레벨 합 × 레벨 1당 ln (공격력 · 추가 피해 · 보스 피해)
    class ArkGemFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };

    // 아크 패시브: 랭크 표 × 깨달음 포인트 구성 표 × 깨달음 · 도약 레벨 선형
    class ArkPassiveFactorComponent final : public CombatFactorComponent
    {
    public:
        [[nodiscard]] CombatFactorKind GetKind() const noexcept override;
        [[nodiscard]] double GetFactor(const Models::CombatState& current, const Models::CombatState& target) const override;
    };
}
