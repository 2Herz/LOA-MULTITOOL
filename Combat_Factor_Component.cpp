module LostArkMultiTool.Core.CombatFactorComponent;  // 전투력 배율 요소 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모듈 가져오기
import LostArkMultiTool.Models.CombatState;  // 전투력 상태 모듈 가져오기
import LostArkMultiTool.Models.CombatSupporterData;  // 서포터 계수 모듈 가져오기

namespace LostArkMultiTool::Core  // 코어 네임스페이스 시작
{
    namespace  // 파일 내부 전용 보조 함수 영역 시작
    {
        namespace Data = Models::CombatCoefficientData;  // 계수 데이터 네임스페이스 별칭
        namespace Supporter = Models::CombatSupporterData;  // 서포터 계수 네임스페이스 별칭

        constexpr double NO_CHANGE_FACTOR{ 1.0 };   // 바뀐 것이 없거나 계산할 수 없음

        // 서포터 계수를 써야 하는 상태인지 (역할은 현재 · 목표가 같음)
        [[nodiscard]] bool IsSupporter(const Models::CombatState& state) noexcept
        {
            return state.m_extra.m_role == Models::CombatRole::ROLE_SUPPORTER;  // 역할이 서포터이면 참
        }

        // 현재 · 목표 모두 같은 표로 조회한 능력치 차이 (분류가 다르거나 모르는 칸이면 0)
        [[nodiscard]] double GetStatGain(const Models::GearState& current, const Models::GearState& target)
        {
            if (!current.IsKnown() || current.m_tableKey != target.m_tableKey) return 0.0;  // 분류가 다르거나 모르는 부위는 능력치 차이 0

            const std::optional<double> currentStat{ Models::CombatGear::GetStat(current) };  // 현재 장비의 표 능력치
            const std::optional<double> targetStat{ Models::CombatGear::GetStat(target) };  // 목표 장비의 표 능력치
            if (!currentStat.has_value() || !targetStat.has_value()) return 0.0;  // 둘 중 하나라도 표에 없으면 차이 0
            return *targetStat - *currentStat;  // 목표 - 현재 = 능력치 증가량
        }

        // 기준값에 차이를 더한 비율 (기준이 없으면 1)
        [[nodiscard]] double GetRatio(double baseValue, double gain) noexcept
        {
            if (baseValue <= 0.0) return NO_CHANGE_FACTOR;  // 기준값이 없으면 영향 없음
            return std::max(baseValue + gain, 0.0) / baseValue;  // (기준 + 증가량) ÷ 기준, 음수가 되지 않게 0으로 제한
        }

        // 품질 곡선 F(q) (0 ~ 최대로 맞춤)
        [[nodiscard]] double GetQualityValue(int quality) noexcept
        {
            const Data::QualityCurve& curve{ Data::GetWeaponQualityCurve() };  // 무기 품질 곡선 조회
            const double ratio{ std::clamp(static_cast<double>(quality), 0.0, curve.m_maxQuality) / curve.m_maxQuality };  // 품질을 0 ~ 최대로 제한하고 비율로 변환
            return curve.m_base + curve.m_scale * ratio * ratio;  // F(q) = A + B × 비율²
        }

        // 보석 계수 F(level) × 종류 계수 (레벨 0 = 보석 없음 → 없을 때의 계수, 표 범위 밖의 레벨 = nullopt)
        [[nodiscard]] std::optional<double> GetGemValue(int level, std::string_view kind, bool isSupporter) noexcept
        {
            if (isSupporter)  // 서포터는 서포터 표 (보석 종류는 영향 없음)
            {
                if (level <= 0) return Supporter::GetNoGemValue();  // 레벨 0이면 보석 없음 계수
                const std::span<const double> supporterFactors{ Supporter::GetGemFactors() };  // 서포터 레벨별 계수 표
                const int supporterIndex{ level - Supporter::GetGemFirstLevel() };  // 표 번호 = 레벨 - 첫 레벨
                if (supporterIndex < 0 || static_cast<std::size_t>(supporterIndex) >= supporterFactors.size()) return std::nullopt;  // 표 범위 밖이면 값 없음
                return supporterFactors[static_cast<std::size_t>(supporterIndex)];  // 레벨 계수 반환
            }
            if (level <= 0) return Data::GetNoGemValue();  // 레벨 0이면 보석 없음 계수 반환
            const std::span<const double> factors{ Data::GetGemFactors() };  // 레벨별 보석 계수 표
            const int index{ level - Data::GetGemFirstLevel() };   // 표 번호 = 레벨 - 첫 레벨
            if (index < 0 || static_cast<std::size_t>(index) >= factors.size()) return std::nullopt;  // 표 범위 밖의 레벨이면 값 없음
            return factors[static_cast<std::size_t>(index)] * Data::GetGemTypeFactor(kind);  // 레벨 계수에 종류 계수를 곱해 반환
        }
    }

    CombatFactorComponent::~CombatFactorComponent() = default;  // 기반 클래스 소멸자 기본 구현

    // ===================== 장비 재련 =====================

    CombatFactorKind GearFactorComponent::GetKind() const noexcept  // 이 요소의 종류: 장비 재련
    {
        return CombatFactorKind::FACTOR_GEAR;  // 장비 재련 종류 반환
    }

    double GearFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 장비 재련 배율 계산
    {
        double mainStatGain{};  // 방어구 주스탯 증가량 합계
        double weaponAttackGain{};  // 무기 공격력 증가량 합계
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 모든 장비 부위를 순회
        {
            const double gain{ GetStatGain(current.m_gears[index], target.m_gears[index]) };  // 부위별 능력치 증가량
            if (Models::ToGearPart(index) == Models::GearPart::PART_WEAPON) weaponAttackGain += gain;  // 무기 부위는 무기 공격력에 합산
            else mainStatGain += gain;  // 나머지 부위는 주스탯에 합산
        }

        // 악세서리 · 팔찌 · 완갑이 바꾸는 주스탯도 같은 S에 더함 (함께 올라야 제곱근이 맞게 합쳐짐)
        mainStatGain += Models::CombatExtra::GetMainStatGain(current.m_extra, target.m_extra);  // 악세서리 · 팔찌 주스탯 증가량을 합산
        const Models::CombatExtra::VambraceGain vambrace{ Models::CombatExtra::GetVambraceGain(current.m_extra, target.m_extra) };  // 완갑이 바꾸는 주스탯 · 무기 공격력 증가량
        mainStatGain += vambrace.m_mainStat;  // 완갑 주스탯 증가분 합산
        weaponAttackGain += vambrace.m_weaponAttack;  // 완갑 무기 공격력 증가분 합산

        // 무기 툴팁 공격력을 모르면 무기 쪽은 계산하지 않음 (보정값만으로는 기준이 맞지 않음)
        const bool isSupporter{ IsSupporter(current) };  // 서포터는 S0 · W0 보정값이 다름
        const double weaponBase{ isSupporter ? Supporter::GetWeaponBase(current.m_weaponAttack) : Data::GetWeaponBase(current.m_weaponAttack) };  // 무기 기준 공격력 (보정 포함)
        const double mainStatBase{ isSupporter && current.m_totalMainStat > 0.0 ? current.m_totalMainStat + Supporter::GetMainStatExtra() : current.m_totalMainStat };  // 주스탯 기준값 (서포터 보정 포함)
        const double product{ GetRatio(mainStatBase, mainStatGain) * GetRatio(weaponBase, weaponAttackGain) };  // 주스탯 비율 × 무기 공격력 비율
        return std::sqrt(product);  // CP ∝ √(S × W) 이므로 제곱근 반환
    }

    // ===================== 무기 품질 =====================

    CombatFactorKind WeaponQualityComponent::GetKind() const noexcept  // 이 요소의 종류: 무기 품질
    {
        return CombatFactorKind::FACTOR_WEAPON_QUALITY;  // 무기 품질 종류 반환
    }

    double WeaponQualityComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 무기 품질 배율 계산
    {
        const std::size_t weaponIndex{ Models::ToGearIndex(Models::GearPart::PART_WEAPON) };  // 무기 부위 번호
        if (IsSupporter(current))  // 서포터는 무기 품질 영향 없음, 방어구 5부위 품질만 (9명 측정)
        {
            const double amplitude{ Supporter::GetArmorQualityAmplitude() };  // 품질 0일 때 부위 하나의 감소 비율
            const double maxQuality{ Supporter::GetMaxQuality() };  // 최대 품질
            const auto armorValue{ [amplitude, maxQuality](int quality)  // F(q) = 1 - a + a × (q / 최대)²
            {
                const double ratio{ std::clamp(static_cast<double>(quality), 0.0, maxQuality) / maxQuality };  // 품질 비율 (0 ~ 1)
                return 1.0 - amplitude + amplitude * ratio * ratio;  // 곡선 값
            } };
            double factor{ NO_CHANGE_FACTOR };  // 누적 배율
            for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 부위 순회
            {
                if (Models::ToGearPart(index) == Models::GearPart::PART_WEAPON) continue;  // 무기 칸은 제외
                const std::optional<int>& from{ current.m_armorQualities[index] };  // 현재 품질
                const std::optional<int>& to{ target.m_armorQualities[index] };  // 목표 품질
                if (!from.has_value() || !to.has_value()) continue;  // 모르는 부위는 건너뜀
                factor *= armorValue(*to) / armorValue(*from);  // 부위 배율 누적
            }
            return factor;  // 방어구 품질 배율 반환
        }
        if (!current.m_weaponQuality.has_value() || !target.m_weaponQuality.has_value()) return NO_CHANGE_FACTOR;  // 현재나 목표 품질을 모르면 영향 없음
        if (Models::CombatGear::IsEsther(current.m_gears[weaponIndex].m_tableKey)) return NO_CHANGE_FACTOR;  // 에스더 무기는 품질이 없으므로 영향 없음

        const double currentValue{ GetQualityValue(*current.m_weaponQuality) };  // 현재 품질 값 F(현재)
        if (currentValue <= 0.0) return NO_CHANGE_FACTOR;  // 0 이하면 나눌 수 없으므로 영향 없음
        return GetQualityValue(*target.m_weaponQuality) / currentValue;  // F(목표) ÷ F(현재)
    }

    // ===================== 보석 =====================

    CombatFactorKind GemFactorComponent::GetKind() const noexcept  // 이 요소의 종류: 보석
    {
        return CombatFactorKind::FACTOR_GEM;  // 보석 종류 반환
    }

    double GemFactorComponent::GetFactor(const Models::CombatState& current, const Models::CombatState& target) const  // 보석 배율 계산
    {
        // 같은 번호의 보석끼리 비교 (목표 목록이 짧으면 그만큼만)
        // 현재 · 목표 레벨 중 하나라도 표(1 ~ 10)에 없으면 그 보석은 배율 1
        double factor{ NO_CHANGE_FACTOR };  // 전체 배율 초기값 (변화 없음)
        const bool isSupporter{ IsSupporter(current) };  // 서포터 표를 쓸지
        const std::size_t count{ std::min(current.m_gemLevels.size(), target.m_gemLevels.size()) };  // 비교할 보석 개수 (두 목록 중 짧은 쪽)
        for (std::size_t index{}; index < count; ++index)  // 같은 번호의 보석끼리 순회
        {
            const auto getKind{ [index](const Models::CombatState& state) -> std::string_view  // 상태에서 해당 번호 보석 종류를 꺼내는 람다
            {
                return index < state.m_gemKinds.size() ? std::string_view{ state.m_gemKinds[index] } : std::string_view{};  // 종류 목록이 짧으면 빈 글자 반환
            } };
            const std::optional<double> currentValue{ GetGemValue(current.m_gemLevels[index], getKind(current), isSupporter) };  // 현재 보석 값
            const std::optional<double> targetValue{ GetGemValue(target.m_gemLevels[index], getKind(target), isSupporter) };  // 목표 보석 값
            if (!currentValue.has_value() || !targetValue.has_value() || *currentValue <= 0.0) continue;  // 표에 없거나 기준이 0 이하면 이 보석은 건너뜀
            factor *= *targetValue / *currentValue;  // 보석별 배율을 누적해 곱함
        }
        return factor;  // 누적한 보석 배율 반환
    }
}
