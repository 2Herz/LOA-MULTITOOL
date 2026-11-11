module LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기

namespace LostArkMultiTool::Models  // 모델 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        namespace Data = CombatCoefficientData;  // 계수 데이터 네임스페이스 별칭

        constexpr char SPACE_CHARACTER{ ' ' };  // 공백 글자 상수
        constexpr std::string_view WHITESPACE_CHARACTERS{ " \t\r\n" };  // 공백으로 취급하는 글자들

        [[nodiscard]] bool IsSpace(char character) noexcept  // 공백 글자 여부 판별
        {
            return WHITESPACE_CHARACTERS.contains(character);  // 공백 목록에 글자가 포함되는지 검사
        }

        // 팔찌 스탯 줄의 종류가 목록에 있는지 ("힘" 등)
        [[nodiscard]] bool IsIn(std::span<const std::string_view> list, std::string_view value) noexcept  // 목록에 값이 들어 있는지 검사
        {
            return std::ranges::find(list, value) != list.end();  // 값을 찾으면 true
        }

        // 완갑 재련 단계의 능력치 (범위를 벗어나면 가장 가까운 끝 단계, 표가 없으면 0)
        [[nodiscard]] const Data::VambraceStep& GetVambraceStats(int level) noexcept  // 완갑 단계 능력치 조회
        {
            static constexpr Data::VambraceStep EMPTY_STEP{};  // 표가 없을 때 쓰는 빈 단계 (모두 0)
            const int clamped{ std::clamp(level, MIN_VALUE, MAX_VAMBRACE_LEVEL) };  // 단계를 0 ~ 최대 완갑 단계로 제한
            const Data::VambraceStep* step{ Data::FindVambraceStep(clamped) };  // 표에서 단계 찾기
            return step != nullptr ? *step : EMPTY_STEP;  // 찾았으면 그 단계, 아니면 빈 단계 반환
        }

        // 완갑 등급의 한계 해방 추가 효과 기본 공격력 비율 (계수 표 순서 = 등급 번호, 표가 모자라면 0)
        [[nodiscard]] double GetVambraceGradeRate(VambraceGrade grade) noexcept  // 완갑 등급 추가 효과 비율 조회
        {
            const std::span<const Data::VambraceGradeBonus> bonuses{ Data::GetVambraceGradeBonuses() };  // 등급별 추가 효과 표
            const std::size_t index{ static_cast<std::size_t>(grade) };  // 등급 번호
            return index < bonuses.size() ? bonuses[index].m_rate : 0.0;  // 범위 안이면 그 등급 비율, 아니면 0
        }

        // 상태의 완갑 등급 (사용자가 고른 등급이 단계와 맞으면 그 등급, 아니면 단계의 기본 등급)
        [[nodiscard]] VambraceGrade ResolveVambraceGrade(const ExtraState& state) noexcept  // 상태의 완갑 등급 결정
        {
            const int level{ state.m_vambraceLevel.value_or(MIN_VALUE) };  // 완갑 재련 단계 (없으면 0)
            if (state.m_vambraceGrade.has_value() && CombatExtra::IsVambraceGradeValid(*state.m_vambraceGrade, level)) return *state.m_vambraceGrade;  // 고른 등급이 단계와 맞으면 그대로
            return CombatExtra::GetVambraceGradeForLevel(level);  // 아니면 단계의 기본 등급
        }
    }

    std::string NormalizeSpaces(std::string_view text)  // 연속 공백을 하나로 줄이고 양 끝 공백을 지움
    {
        std::string result;  // 결과 문자열
        result.reserve(text.size());  // 입력 길이만큼 메모리 미리 확보
        bool isPendingSpace{};  // 공백이 대기 중인지 표시
        for (const char character : text)  // 글자를 하나씩 순회
        {
            if (IsSpace(character))  // 공백이면
            {
                isPendingSpace = !result.empty();   // 맨 앞 공백은 버림, 연속 공백은 하나로
                continue;  // 공백 대기 상태 갱신
            }  // 다음 글자로 넘어감
            if (isPendingSpace) result.push_back(SPACE_CHARACTER);
            isPendingSpace = false;  // 대기 중인 공백이 있으면 하나만 추가
            result.push_back(character);  // 공백 대기 해제
        }  // 글자 추가
        return result;
    }  // 정리한 글자 반환

    std::string RemoveSpaces(std::string_view text)
    {  // 모든 공백을 지움
        std::string result;
        result.reserve(text.size());  // 결과 문자열
        for (const char character : text)  // 입력 길이만큼 메모리 미리 확보
        {  // 글자를 하나씩 순회
            if (!IsSpace(character)) result.push_back(character);
        }  // 공백이 아니면 추가
        return result;
    }  // 공백을 모두 지운 글자 반환

    bool IsNone(std::string_view text) noexcept
    {  // 옵션이 비어 있는지 판별
        return text.empty() || text == NONE_TEXT;
    }  // 빈 글자이거나 "없음"이면 true

    namespace CombatExtra  // 추가 시스템 규칙 네임스페이스 시작
    {
        double GetMainStatGain(const ExtraState& current, const ExtraState& target)  // 주스탯 증가량 계산
        {
            double gain{};  // 증가량 합계

            // 악세서리 기본 효과 주스탯
            for (std::size_t index{}; index < ACCESSORY_SLOT_COUNT; ++index)  // 악세서리 슬롯 순회
            {
                gain += target.m_accessories[index].m_mainStat - current.m_accessories[index].m_mainStat;  // 목표 - 현재 주스탯 차이를 더함
            }

            // 팔찌 스탯 줄 중 힘 · 민첩 · 지능
            const auto sumMainStat{ [](const ExtraState& state)  // 상태의 팔찌 스탯 중 주스탯 합을 구하는 람다
            {
                double sum{};  // 주스탯 합계
                for (const BraceletStat& line : state.m_braceletStats)  // 팔찌 스탯 줄 순회
                {
                    if (IsIn(Data::GetBraceletMainStatTypes(), line.m_type)) sum += line.m_value;  // 힘 · 민첩 · 지능 종류이면 더함
                }
                return sum;  // 합계 반환
            } };
            gain += sumMainStat(target) - sumMainStat(current);  // 목표와 현재의 팔찌 주스탯 차이를 더함

            return gain;  // 전체 증가량 반환
        }

        VambraceGain GetVambraceGain(const ExtraState& current, const ExtraState& target)  // 완갑 재련 변화량 계산
        {
            VambraceGain gain;  // 변화량 (기본값은 변화 없음)
            if (!current.m_vambraceLevel.has_value() || !target.m_vambraceLevel.has_value()) return gain;  // 현재나 목표 완갑이 없으면 변화 없음

            const Data::VambraceStep& from{ GetVambraceStats(*current.m_vambraceLevel) };  // 현재 단계 능력치
            const Data::VambraceStep& to{ GetVambraceStats(*target.m_vambraceLevel) };  // 목표 단계 능력치
            gain.m_weaponAttack = to.m_weaponAttack - from.m_weaponAttack;  // 무기 공격력 차이
            gain.m_mainStat = to.m_mainStat - from.m_mainStat;  // 주스탯 차이
            gain.m_baseAttack = to.m_baseAttack - from.m_baseAttack;  // 기본 공격력 차이

            // 한계 해방 추가 효과 (현재는 API 툴팁 값을 우선, 목표 등급이 현재와 같으면 현재 비율 그대로)
            const VambraceGrade fromGrade{ ResolveVambraceGrade(current) };  // 현재 완갑 등급
            const VambraceGrade toGrade{ ResolveVambraceGrade(target) };  // 목표 완갑 등급
            gain.m_gradeRateFrom = current.m_vambraceGradeRate.value_or(GetVambraceGradeRate(fromGrade));  // 현재 추가 효과 비율
            gain.m_gradeRateTo = toGrade == fromGrade ? gain.m_gradeRateFrom : GetVambraceGradeRate(toGrade);  // 목표 추가 효과 비율
            return gain;  // 변화량 반환
        }

        VambraceGrade GetVambraceGradeForLevel(int level) noexcept  // 재련 단계 → 기본 등급 (해방 전 등급)
        {
            for (std::size_t index{}; index < VAMBRACE_GRADE_COUNT; ++index)  // 낮은 등급부터 확인
            {
                if (level <= VAMBRACE_GRADE_MAX_LEVELS[index]) return static_cast<VambraceGrade>(index);  // 그 등급의 최고 단계 이하이면 그 등급
            }
            return VambraceGrade::VAMBRACE_ANCIENT;  // 범위를 넘으면 가장 높은 등급
        }

        bool IsVambraceGradeValid(VambraceGrade grade, int level) noexcept  // 등급 · 단계 조합 검사
        {
            const std::size_t index{ static_cast<std::size_t>(grade) };  // 등급 번호
            if (index >= VAMBRACE_GRADE_COUNT) return false;  // 모르는 등급이면 맞지 않음
            return level >= VAMBRACE_GRADE_MIN_LEVELS[index] && level <= VAMBRACE_GRADE_MAX_LEVELS[index];  // 등급의 재련 가능 범위 안인지 확인
        }

        std::optional<VambraceGrade> ParseVambraceGrade(std::string_view name) noexcept  // 등급 이름 해석
        {
            const std::span<const Data::VambraceGradeBonus> bonuses{ Data::GetVambraceGradeBonuses() };  // 등급 표 (이름 · 순서)
            for (std::size_t index{}; index < bonuses.size() && index < VAMBRACE_GRADE_COUNT; ++index)  // 등급 표 순회
            {
                if (bonuses[index].m_name == name) return static_cast<VambraceGrade>(index);  // 이름이 같으면 그 등급
            }
            return std::nullopt;  // 모르는 이름이면 없음
        }

        ExtraState Clamp(const ExtraState& state)  // 입력 범위 안으로 맞추기
        {
            ExtraState clamped{ state };  // 복사본을 만들어 값을 보정
            for (AccessoryState& accessory : clamped.m_accessories) accessory.m_mainStat = std::clamp(accessory.m_mainStat, 0.0, MAX_ACCESSORY_MAIN_STAT);  // 악세서리 주스탯 범위 제한
            for (BraceletStat& line : clamped.m_braceletStats) line.m_value = std::clamp(line.m_value, MIN_VALUE, MAX_BRACELET_STAT);  // 팔찌 스탯 수치 범위 제한
            for (StoneLine& line : clamped.m_stone) line.m_level = std::clamp(line.m_level, MIN_VALUE, Data::STONE_MAX_LEVEL);  // 스톤 레벨 범위 제한
            for (BookState& book : clamped.m_books) book.m_level = std::clamp(book.m_level, MIN_VALUE, Data::ENGRAVING_BOOK_MAX_LEVEL);  // 각인서 레벨 범위 제한
            for (ArkCoreState& core : clamped.m_cores) core.m_points = std::clamp(core.m_points, MIN_VALUE, MAX_ARK_CORE_POINTS);  // 코어 의지력 포인트 범위 제한
            for (int& level : clamped.m_arkGemLevels) level = std::clamp(level, MIN_VALUE, MAX_ARK_GEM_LEVEL_SUM);  // 아크 젬 레벨 합 범위 제한
            clamped.m_passive.m_rank = std::clamp(clamped.m_passive.m_rank, MIN_VALUE, MAX_PASSIVE_RANK);  // 아크 패시브 랭크 범위 제한
            for (int& level : clamped.m_passive.m_levels) level = std::clamp(level, MIN_VALUE, MAX_PASSIVE_LEVEL);  // 아크 패시브 트리 레벨 범위 제한
            if (clamped.m_vambraceLevel.has_value())  // 완갑이 있으면
            {
                clamped.m_vambraceLevel = std::clamp(*clamped.m_vambraceLevel, MIN_VALUE, MAX_VAMBRACE_LEVEL);  // 완갑 재련 단계 범위 제한
                clamped.m_vambraceGrade = ResolveVambraceGrade(clamped);  // 단계와 맞지 않는 등급은 단계의 기본 등급으로 맞춤
            }
            else clamped.m_vambraceGrade.reset();  // 완갑이 없으면 등급도 없음
            return clamped;  // 보정한 상태 반환
        }
    }
}
