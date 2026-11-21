module LostArkMultiTool.Services.TransferCalculator;  // 2부 이관 계산기 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모델 가져오기
import LostArkMultiTool.Models.TransferData;  // 2부 이관 표 가져오기

namespace LostArkMultiTool::Services  // 서비스 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        namespace Data = Models::TransferData;   // 이관 표 네임스페이스 줄임

        using Data::PolishRank;   // 상 · 중 · 하 열거형

        constexpr std::string_view WEAPON_PART{ "무기" };            // 무기 부위 이름 (API)
        constexpr std::string_view VAMBRACE_SLOT{ "완갑" };          // 완갑 슬롯 이름
        constexpr char PLUS_CHARACTER{ '+' };                        // 옵션 이름과 수치 사이 기호
        constexpr char PERCENT_CHARACTER{ '%' };                     // 퍼센트 옵션 표식
        constexpr char THOUSANDS_SEPARATOR{ ',' };                   // 천 단위 구분 글자
        constexpr std::string_view PERCENT_KEY_SUFFIX{ "%" };        // 퍼센트 옵션 키 끝 글자 (공격력% 등)
        constexpr std::string_view FLAT_KEY_SUFFIX{ "+" };           // 고정값 옵션 키 끝 글자 (공격력+ 등)
        constexpr double AMOUNT_TOLERANCE{ 0.001 };                  // 수치 비교 오차 (소수 둘째 자리 표시)
        constexpr int MIN_GEM_LEVEL{ 1 };                            // 보석 최저 레벨

        constexpr std::array<std::string_view, Models::ACCESSORY_SLOT_COUNT> SCROLL_SLOT_NAMES{ "목걸이", "귀걸이1", "귀걸이2", "반지1", "반지2" };   // 장신구 슬롯 이름 (m_accessories 순서)

        // 대상이 아닌 이유
        constexpr std::string_view REASON_EMPTY{ "장착 안 함" };                    // 장신구 · 팔찌가 없음
        constexpr std::string_view REASON_TIER{ "4티어만 이관" };                    // 3티어 이하
        constexpr std::string_view REASON_QUALITY{ "품질 67 미만" };                // 품질 부족
        constexpr std::string_view REASON_GRADE{ "고대 (서포터는 유물 상상도) 만 대상" };   // 등급 부족
        constexpr std::string_view REASON_POLISH{ "유물은 서포터 상상만 대상" };    // 유물인데 서포터 상상이 아님

        // 띄어쓰기를 뺀 글자에서 수치 읽기 ("1,234" · "2.60%" → 수치, 못 읽으면 nullopt)
        [[nodiscard]] std::optional<double> ParseAmount(std::string_view text)
        {
            std::string digits;   // 쉼표 · % 를 뺀 숫자 글자
            for (const char character : text)   // 글자마다
            {
                if (character == THOUSANDS_SEPARATOR) continue;   // 천 단위 구분은 건너뜀
                if (character == PERCENT_CHARACTER) break;        // % 앞까지만
                digits.push_back(character);                      // 숫자 · 소수점 기록
            }
            double amount{};   // 읽은 수치
            const auto [end, error]{ std::from_chars(digits.data(), digits.data() + digits.size(), amount) };   // 수치 변환
            if (error != std::errc{} || digits.empty()) return std::nullopt;   // 변환 실패
            return amount;   // 수치 반환
        }

        // 상 · 중 · 하 순서로 비교할 번호 (작을수록 좋음)
        [[nodiscard]] constexpr int ToOrder(PolishRank rank) noexcept
        {
            return static_cast<int>(rank);   // 열거형 순서 = 상 · 중 · 하 · 없음
        }

        // 연마 효과 줄 목록 → 표 키별 가장 좋은 등급
        using RankMap = std::vector<std::pair<std::string, PolishRank>>;   // (키, 등급) 목록 (최대 3줄이라 선형 검색)

        [[nodiscard]] PolishRank FindRank(const RankMap& ranks, std::string_view key) noexcept
        {
            PolishRank best{ PolishRank::RANK_NONE };   // 가장 좋은 등급 (없으면 없음)
            for (const auto& [lineKey, rank] : ranks)   // 줄마다
            {
                if (lineKey == key && ToOrder(rank) < ToOrder(best)) best = rank;   // 같은 키에서 더 좋은 등급
            }
            return best;   // 결과 반환
        }

        // 부위 규칙 하나로 판정한 결과
        struct ScrollJudge
        {
            std::optional<int> m_grade;      // 주문서 등급 (대상이 아니면 nullopt)
            std::string m_polishText;        // "상상중" 같은 판정 글자
            std::string_view m_reason;       // 대상이 아닌 이유
        };

        // 규칙(딜러 · 서포터) 하나로 장신구 등급 판정
        [[nodiscard]] ScrollJudge JudgeScroll(const Data::AccessoryRule& rule, const RankMap& ranks, std::string_view grade, bool isSupporterRule)
        {
            PolishRank first{ FindRank(ranks, rule.m_valid[0]) };    // 유효 옵션 1
            PolishRank second{ FindRank(ranks, rule.m_valid[1]) };   // 유효 옵션 2
            if (ToOrder(second) < ToOrder(first)) std::swap(first, second);   // 좋은 등급이 앞으로 (상중 = 중상)

            // 세 번째 칸: 인정하는 옵션 중 가장 좋은 등급 (서포터 귀걸이는 최대 생명력인지 따로 기억)
            PolishRank filler{ PolishRank::RANK_NONE };   // 세 번째 칸 등급
            bool isHealthFiller{};                        // 세 번째 칸이 최대 생명력인지
            for (const std::string_view fillerKey : rule.m_fillers)   // 인정 옵션마다
            {
                const PolishRank rank{ FindRank(ranks, fillerKey) };   // 그 옵션의 등급
                if (ToOrder(rank) < ToOrder(filler) || (rank == filler && rank != PolishRank::RANK_NONE && fillerKey == Data::SUPPORT_EARRING_HEALTH_KEY))   // 더 좋은 등급 (같으면 최대 생명력 우선)
                {
                    filler = rank;   // 등급 기록
                    isHealthFiller = fillerKey == Data::SUPPORT_EARRING_HEALTH_KEY;   // 최대 생명력인지 기록
                }
            }

            ScrollJudge judge;   // 판정 결과
            judge.m_polishText = std::format("{}{}{}", Data::GetPolishRankName(first), Data::GetPolishRankName(second),
                filler == PolishRank::RANK_NONE ? std::string_view{} : Data::GetPolishRankName(filler));   // "상상" + 세 번째 칸

            const bool isHighHigh{ first == PolishRank::RANK_HIGH && second == PolishRank::RANK_HIGH };   // 유효 옵션 둘 다 상
            if (grade == Data::RELIC_GRADE)   // 유물: 서포터 목걸이 · 반지 상상만 5등급
            {
                if (isSupporterRule && rule.m_partType != Data::SUPPORTER_EARRING_TYPE && isHighHigh) judge.m_grade = Data::RELIC_SUPPORTER_HIGH_HIGH;   // 5등급
                else judge.m_reason = REASON_POLISH;   // 대상 아님
                return judge;   // 판정 끝
            }
            if (grade != Data::ANCIENT_GRADE)   // 고대 · 유물이 아니면 대상 아님
            {
                judge.m_reason = REASON_GRADE;   // 이유 기록
                return judge;   // 판정 끝
            }

            if (isSupporterRule && rule.m_partType == Data::SUPPORTER_EARRING_TYPE)   // 고대 서포터 귀걸이 (별도 표)
            {
                if (!isHighHigh) judge.m_grade = Data::SCROLL_GRADE_WORST;   // 상중 이하 6등급
                else if (filler == PolishRank::RANK_HIGH) judge.m_grade = isHealthFiller ? Data::SUPPORT_EARRING_HEALTH_GRADE : Data::SUPPORT_EARRING_SHIELD_GRADE;   // 상상상 (최생 2 · 보호막/회복 3)
                else if (filler == PolishRank::RANK_MIDDLE) judge.m_grade = Data::SUPPORT_EARRING_HIGH_HIGH_MIDDLE;   // 상상중 4
                else judge.m_grade = Data::SUPPORT_EARRING_HIGH_HIGH;   // 상상 5
                return judge;   // 판정 끝
            }

            // 고대 딜러 목 · 귀 · 반, 서포터 목 · 반
            if (isHighHigh)   // 상상
            {
                if (filler == PolishRank::RANK_HIGH) judge.m_grade = Data::SCROLL_GRADE_BEST;   // 상상상 1
                else if (filler == PolishRank::RANK_MIDDLE) judge.m_grade = Data::SCROLL_GRADE_HIGH_HIGH_MIDDLE;   // 상상중 2
                else judge.m_grade = Data::SCROLL_GRADE_HIGH_HIGH;   // 상상 3
            }
            else if (first == PolishRank::RANK_HIGH && second == PolishRank::RANK_MIDDLE) judge.m_grade = Data::SCROLL_GRADE_HIGH_MIDDLE;   // 상중 4
            else if (first == PolishRank::RANK_HIGH && second == PolishRank::RANK_LOW) judge.m_grade = Data::SCROLL_GRADE_HIGH_LOW;         // 상하 5
            else judge.m_grade = Data::SCROLL_GRADE_WORST;   // 중중 이하 6
            return judge;   // 판정 끝
        }

        // 부위 종류("목걸이")에 맞는 규칙 (없으면 nullptr)
        [[nodiscard]] const Data::AccessoryRule* FindRule(std::span<const Data::AccessoryRule> rules, std::string_view partType) noexcept
        {
            const auto found{ std::ranges::find(rules, partType, &Data::AccessoryRule::m_partType) };   // 부위로 찾기
            return found == rules.end() ? nullptr : &*found;   // 못 찾으면 nullptr
        }

        // 부위 이름 → 지금 장비 (없으면 nullptr)
        [[nodiscard]] const Models::GearItem* FindGear(const Models::CharacterArmory& armory, std::string_view partName)
        {
            const std::optional<Models::GearPart> part{ Models::FindGearPart(partName) };   // API 부위 이름 → 부위
            if (!part.has_value()) return nullptr;   // 모르는 부위
            const std::optional<Models::GearItem>& gear{ armory.m_gears[Models::ToGearIndex(*part)] };   // 부위 칸
            return gear.has_value() ? &*gear : nullptr;   // 안 끼었으면 nullptr
        }
    }

    // 이관 아이템 레벨 (에스더 우선, 표 밖이면 0)
    int TransferSlotResult::GetTargetLevel() const noexcept
    {
        if (m_esther.has_value()) return m_esther->m_level;   // 에스더 무기
        return m_target.has_value() ? m_target->m_level : 0;  // 일반 장비 (표 밖이면 0)
    }

    // 장비 레벨 → 이관 장비 (1600 이하 = nullopt, 1800 초과 = 마지막 행)
    std::optional<Data::GearTransferRow> TransferCalculator::FindGearRow(int level) noexcept
    {
        if (level <= Data::GEAR_MIN_EXCLUSIVE_LEVEL) return std::nullopt;   // 표 시작 이하
        const std::span<const Data::GearTransferRow> table{ Data::GetGearTable() };   // 장비 이관 표
        const auto found{ std::ranges::find_if(table, [level](const Data::GearTransferRow& row) { return level <= row.m_maxLevel; }) };   // 상한 이하인 첫 행
        return found == table.end() ? table.back() : *found;   // 표보다 높으면 마지막 행
    }

    // 에스더 무기 레벨 → 엘라 단계 이관
    std::optional<Data::EstherTransferRow> TransferCalculator::FindEstherRow(int level) noexcept
    {
        if (level <= 0) return std::nullopt;   // 레벨을 모름
        const std::span<const Data::EstherTransferRow> table{ Data::GetEstherTable() };   // 에스더 이관 표
        const auto found{ std::ranges::find_if(table, [level](const Data::EstherTransferRow& row) { return level <= row.m_maxLevel; }) };   // 상한 이하인 첫 행
        return found == table.end() ? table.back() : *found;   // 마지막 행은 상한이 최대값
    }

    // 연마 효과 한 줄 ("적에게 주는 피해 +1.20%") → 상 · 중 · 하, key = 표 키
    Data::PolishRank TransferCalculator::RankPolishLine(std::string_view line, std::string& key)
    {
        key.clear();   // 키 비움
        const std::string compact{ Models::RemoveSpaces(line) };   // 띄어쓰기를 뺀 줄
        const std::size_t plus{ compact.find(PLUS_CHARACTER) };     // 이름과 수치 사이 "+"
        if (plus == std::string::npos) return PolishRank::RANK_NONE;   // 수치가 없는 줄

        const std::string name{ compact.substr(0, plus) };   // 옵션 이름
        const std::string_view valueText{ std::string_view{ compact }.substr(plus + 1) };   // 수치 글자
        const bool isPercent{ valueText.contains(PERCENT_CHARACTER) };   // 퍼센트 옵션인지
        const std::optional<double> amount{ ParseAmount(valueText) };   // 수치
        if (!amount.has_value()) return PolishRank::RANK_NONE;   // 수치를 못 읽음

        // 표 키: "공격력%" · "무기공격력+"처럼 단위를 붙인 키를 먼저, 없으면 이름 그대로
        const std::span<const Data::PolishTier> tiers{ Data::GetPolishTiers() };   // 연마 효과 표
        const std::string suffixed{ name + std::string{ isPercent ? PERCENT_KEY_SUFFIX : FLAT_KEY_SUFFIX } };   // 단위를 붙인 키
        auto tier{ std::ranges::find(tiers, suffixed, &Data::PolishTier::m_key) };   // 단위 키로 찾기
        if (tier == tiers.end()) tier = std::ranges::find(tiers, name, &Data::PolishTier::m_key);   // 이름 키로 찾기
        if (tier == tiers.end()) return PolishRank::RANK_NONE;   // 표에 없는 옵션

        key = std::string{ tier->m_key };   // 찾은 키 기록
        for (std::size_t index{}; index < Data::POLISH_RANK_COUNT; ++index)   // 상 → 중 → 하 순서로
        {
            if (*amount + AMOUNT_TOLERANCE >= tier->m_amounts[index]) return static_cast<PolishRank>(index);   // 그 등급 수치 이상
        }
        return PolishRank::RANK_NONE;   // 하 미만
    }

    // 활성화 포인트 → 각인 업그레이드 결과 (표에 없으면 빈 글자)
    std::string_view TransferCalculator::FindStoneResult(int high, int low) noexcept
    {
        for (const Data::StoneTransferRow& row : Data::GetStoneTable())   // 표의 행마다
        {
            if (row.m_high == high && row.m_low == low) return row.m_result;   // 같은 조합
        }
        return {};   // 표 밖
    }

    // 무기고 → 2부 이관 결과
    TransferResult TransferCalculator::Calculate(const Models::CharacterArmory& armory) const
    {
        TransferResult result;   // 결과
        result.m_currentAverageLevel = armory.m_itemAvgLevel;   // 지금 평균 레벨
        result.m_isSupporter = armory.m_extra.m_role == Models::CombatRole::ROLE_SUPPORTER;   // 서포터 여부

        // ===== 무기 기준 레벨 (에스더 1800 이상이면 25강 무기 1800 기준) =====
        const Models::GearItem* weapon{ FindGear(armory, WEAPON_PART) };   // 무기
        result.m_isEsther = weapon != nullptr && weapon->m_grade == Data::ESTHER_GRADE;   // 에스더 무기 여부
        const int weaponLevel{ weapon != nullptr ? weapon->m_itemLevel : 0 };   // 무기 레벨
        result.m_weaponBasisLevel = result.m_isEsther && weaponLevel >= Data::ESTHER_FULL_LEVEL ? Data::ESTHER_FULL_LEVEL : weaponLevel;   // 무기 기준 부위에 쓸 레벨

        // 슬롯 하나 채우기 (기준이 무기면 무기 기준 레벨, 아니면 그 부위 레벨)
        const auto makeSlot{ [&](const Data::SlotPair& pair)
        {
            TransferSlotResult slot;   // 슬롯 결과
            slot.m_slotName = pair.m_target;     // 2부 슬롯 이름
            slot.m_sourceName = pair.m_source;   // 기준 부위 이름
            slot.m_isNewSlot = pair.m_isNewSlot; // 새 슬롯 여부
            const Models::GearItem* source{ FindGear(armory, pair.m_source) };   // 기준 장비
            if (source != nullptr) slot.m_sourceGrade = source->m_grade;          // 기준 장비 등급
            slot.m_sourceLevel = pair.m_source == WEAPON_PART ? result.m_weaponBasisLevel : (source != nullptr ? source->m_itemLevel : 0);   // 기준 레벨
            slot.m_target = FindGearRow(slot.m_sourceLevel);   // 이관 장비
            return slot;   // 슬롯 반환
        } };

        // ===== 장비 쪽 8칸 (무기 · 방어구 5 · 완갑 · 허리띠) + 슬롯 재련 =====
        for (const Data::SlotPair& pair : Data::GetArmorSidePairs())   // 장비 쪽 슬롯마다
        {
            TransferSlotResult slot{ makeSlot(pair) };   // 기본 이관
            const bool isOwnPart{ pair.m_target != VAMBRACE_SLOT && !pair.m_isNewSlot };   // 무기 · 방어구 5부위 (완갑 · 허리띠는 무기 기준이라 자기 장비가 아님)
            const Models::GearItem* own{ isOwnPart ? FindGear(armory, pair.m_source) : nullptr };   // 자기 부위 장비
            if (pair.m_target == WEAPON_PART && result.m_isEsther)   // 에스더 무기 슬롯
            {
                slot.m_sourceLevel = weaponLevel;          // 실제 무기 레벨 표시
                slot.m_esther = FindEstherRow(weaponLevel); // 엘라 단계 이관
                slot.m_target.reset();                     // 일반 장비 표는 쓰지 않음
                if (weaponLevel >= Data::ESTHER_FULL_LEVEL) slot.m_slotRefine = Data::ESTHER_FULL_SLOT_REFINE;   // 9 · 10단계 엘라3 → 25단계
            }
            else if (own != nullptr)   // 무기 · 방어구: 지금 재련 단계가 같은 슬롯으로
            {
                slot.m_slotRefine = own->m_enhancement;           // 슬롯 재련
                slot.m_advancedRefine = own->m_advancedEnhancement;   // 상급 재련 (처리 방식 공지 대기)
            }
            else if (pair.m_target == VAMBRACE_SLOT) slot.m_slotRefine = armory.m_extra.m_vambraceLevel;   // 완갑 재련 단계
            else if (pair.m_isNewSlot) slot.m_slotRefine = Data::NEW_SLOT_REFINE;   // 허리띠는 0부터
            result.m_armorSlots.push_back(std::move(slot));   // 결과에 추가
        }

        // ===== 장신구 쪽 8칸 =====
        for (const Data::SlotPair& pair : Data::GetAccessorySidePairs()) result.m_accessorySlots.push_back(makeSlot(pair));   // 짝 부위 레벨로 이관

        // ===== 16부위 평균 (표 밖 부위는 0으로 셈 → 실제보다 낮게 보일 수 있음) =====
        int levelSum{};   // 이관 레벨 합
        for (const TransferSlotResult& slot : result.m_armorSlots) levelSum += slot.GetTargetLevel();       // 장비 쪽
        for (const TransferSlotResult& slot : result.m_accessorySlots) levelSum += slot.GetTargetLevel();   // 장신구 쪽
        result.m_transferAverageLevel = static_cast<double>(levelSum) / static_cast<double>(Data::TRANSFER_SLOT_TOTAL);   // 평균

        // ===== 장신구 → 주문서 =====
        for (std::size_t index{}; index < Models::ACCESSORY_SLOT_COUNT; ++index)   // 장신구 슬롯마다
        {
            ScrollResult scroll;   // 주문서 결과
            scroll.m_slotName = SCROLL_SLOT_NAMES[index];   // 슬롯 이름
            const std::optional<Models::GearItem>& item{ armory.m_accessoryItems[index] };   // 장신구 장비
            if (!item.has_value())   // 안 낌
            {
                scroll.m_reason = REASON_EMPTY;   // 이유
                result.m_scrolls.push_back(std::move(scroll));   // 결과에 추가
                continue;   // 다음 슬롯
            }
            scroll.m_itemName = item->m_name;   // 이름
            scroll.m_grade = item->m_grade;     // 등급
            scroll.m_quality = item->m_quality; // 품질
            scroll.m_tier = item->m_tier;       // 티어

            // 연마 효과 줄마다 상 · 중 · 하
            RankMap ranks;   // (키, 등급) 목록
            for (const std::string& line : armory.m_extra.m_accessories[index].m_options)   // 옵션 줄마다
            {
                if (Models::IsNone(line)) continue;   // 빈 줄
                std::string key;   // 표 키
                const PolishRank rank{ RankPolishLine(line, key) };   // 등급
                if (!key.empty()) ranks.emplace_back(std::move(key), rank);   // 표에 있는 옵션만
            }

            const Data::AccessoryRule* dealerRule{ FindRule(Data::GetDealerAccessoryRules(), item->m_type) };       // 딜러 규칙
            const Data::AccessoryRule* supporterRule{ FindRule(Data::GetSupporterAccessoryRules(), item->m_type) }; // 서포터 규칙
            const ScrollJudge dealer{ dealerRule != nullptr ? JudgeScroll(*dealerRule, ranks, item->m_grade, false) : ScrollJudge{} };            // 딜러 판정
            const ScrollJudge supporter{ supporterRule != nullptr ? JudgeScroll(*supporterRule, ranks, item->m_grade, true) : ScrollJudge{} };    // 서포터 판정

            // 등급이 좋은 쪽 (같으면 캐릭터 역할 쪽)
            const int dealerGrade{ dealer.m_grade.value_or(std::numeric_limits<int>::max()) };          // 딜러 등급 (없으면 최대)
            const int supporterGrade{ supporter.m_grade.value_or(std::numeric_limits<int>::max()) };    // 서포터 등급
            const bool useSupporter{ supporterGrade < dealerGrade || (supporterGrade == dealerGrade && result.m_isSupporter) };   // 서포터 기준을 쓸지
            const ScrollJudge& chosen{ useSupporter ? supporter : dealer };   // 고른 판정
            scroll.m_polishText = chosen.m_polishText;   // 판정 글자
            scroll.m_isSupporterBasis = useSupporter;    // 기준
            scroll.m_scrollGrade = chosen.m_grade;       // 주문서 등급
            scroll.m_reason = chosen.m_reason;           // 이유 (대상이면 빈 글자)

            // 공통 대상 조건 (4티어 · 품질 67 이상)
            if (item->m_tier != Data::ACCESSORY_MIN_TIER) { scroll.m_scrollGrade.reset(); scroll.m_reason = REASON_TIER; }   // 4티어 아님
            else if (item->m_quality.value_or(0) < Data::ACCESSORY_MIN_QUALITY) { scroll.m_scrollGrade.reset(); scroll.m_reason = REASON_QUALITY; }   // 품질 부족

            if (scroll.m_scrollGrade.has_value() && (!result.m_bestScrollGrade.has_value() || *scroll.m_scrollGrade < *result.m_bestScrollGrade))
                result.m_bestScrollGrade = scroll.m_scrollGrade;   // 가장 좋은 등급 갱신
            result.m_scrolls.push_back(std::move(scroll));   // 결과에 추가
        }

        // ===== 팔찌 → 인장 주문서 (옵션 티어) =====
        if (!armory.m_braceletItem.has_value()) result.m_braceletReason = REASON_EMPTY;   // 팔찌 없음
        else
        {
            result.m_braceletGrade = armory.m_braceletItem->m_grade;   // 팔찌 등급
            if (armory.m_braceletItem->m_tier != Data::ACCESSORY_MIN_TIER) result.m_braceletReason = REASON_TIER;   // 4티어만

            const std::span<const Data::BraceletStatRule> statRules{ Data::GetBraceletStatRules() };   // 스탯 티어 표
            const auto addStat{ [&](std::string_view stat, std::string text)   // 스탯 줄 추가
            {
                const auto rule{ std::ranges::find(statRules, stat, &Data::BraceletStatRule::m_stat) };   // 스탯 규칙
                BraceletLineResult line{ std::move(text) };   // 줄 결과
                if (rule != statRules.end())   // 아는 스탯
                {
                    line.m_label = rule->m_stat;                 // 이름
                    line.m_dealerTier = rule->m_dealerTier;      // 딜러 티어
                    line.m_supporterTier = rule->m_supporterTier;   // 서포터 티어
                }
                result.m_braceletLines.push_back(std::move(line));   // 결과에 추가
            } };

            for (const Models::BraceletStat& stat : armory.m_extra.m_braceletStats)   // 고정 스탯 줄
            {
                if (stat.m_type.empty()) continue;   // 빈 줄
                addStat(stat.m_type, std::format("{} +{}", stat.m_type, stat.m_value));   // "특화 +93"
            }
            for (const std::string& option : armory.m_extra.m_braceletOptions)   // 옵션 줄
            {
                if (Models::IsNone(option)) continue;   // 빈 줄

                // 바꿀 수 있는 스탯 줄 ("특화+93")
                const auto statRule{ std::ranges::find_if(statRules, [&option](const Data::BraceletStatRule& rule)
                {
                    return option.starts_with(rule.m_stat) && option.size() > rule.m_stat.size() && option[rule.m_stat.size()] == PLUS_CHARACTER;   // 스탯 이름 바로 뒤가 "+"
                }) };
                if (statRule != statRules.end()) { addStat(statRule->m_stat, option); continue; }   // 스탯 줄로 처리

                // 옵션 규칙 (위에서부터 처음 맞는 것)
                BraceletLineResult line{ option };   // 줄 결과
                for (const Data::BraceletRule& rule : Data::GetBraceletRules())   // 규칙마다
                {
                    const bool isMatch{ std::ranges::all_of(rule.m_keywords, [&option](std::string_view keyword) { return keyword.empty() || option.contains(keyword); }) };   // 낱말이 모두 들어 있는지
                    if (!isMatch) continue;   // 다음 규칙
                    line.m_label = rule.m_label;                 // 분류 이름
                    line.m_dealerTier = rule.m_dealerTier;       // 딜러 티어
                    line.m_supporterTier = rule.m_supporterTier; // 서포터 티어
                    break;   // 처음 맞는 규칙만
                }
                result.m_braceletLines.push_back(std::move(line));   // 결과에 추가
            }
        }

        // ===== 보석 → 광휘 · 성휘 =====
        for (const int level : armory.m_gemLevels)   // 보석마다
        {
            const int clamped{ std::clamp(level, MIN_GEM_LEVEL, Data::MAX_GEM_LEVEL) };   // 1 ~ 10
            ++result.m_gems.m_levelCounts[static_cast<std::size_t>(clamped)];   // 레벨별 개수
            ++result.m_gems.m_totalCount;   // 전체 개수
        }
        const int tenCount{ result.m_gems.m_levelCounts[static_cast<std::size_t>(Data::MAX_GEM_LEVEL)] };   // 10레벨 개수
        const int starTwoCost{ Data::STAR_FIRST_COST * Data::STAR_NEXT_COST };   // 성휘 2레벨 하나에 드는 10레벨 수 (3 × 2 = 6)
        result.m_gems.m_starTwoCount = tenCount / starTwoCost;   // 성휘 2레벨 수
        const int remainTen{ tenCount % starTwoCost };            // 남은 10레벨
        result.m_gems.m_starOneCount = remainTen / Data::STAR_FIRST_COST;   // 성휘 1레벨 수
        result.m_gems.m_leftTenCount = remainTen % Data::STAR_FIRST_COST;   // 남는 10레벨 광휘

        // ===== 스톤 → 각인 업그레이드 (장착한 각인서와 같은 이름만 유효) =====
        result.m_stone.m_hasStone = armory.m_stoneItem.has_value();   // 스톤 장착 여부
        result.m_stone.m_lines = armory.m_extra.m_stone;              // 스톤 두 줄
        std::array<int, Models::STONE_LINE_COUNT> points{};           // 유효 활성화 포인트
        for (std::size_t index{}; index < Models::STONE_LINE_COUNT; ++index)   // 줄마다
        {
            const Models::StoneLine& line{ armory.m_extra.m_stone[index] };   // 스톤 줄
            const bool isValid{ !line.m_name.empty() && line.m_level > 0 &&
                std::ranges::any_of(armory.m_extra.m_books, [&line](const Models::BookState& book) { return book.m_name == line.m_name; }) };   // 장착 각인과 같은 이름
            result.m_stone.m_isValid[index] = isValid;   // 유효 여부
            points[index] = isValid ? line.m_level : 0;  // 유효하면 포인트
        }
        result.m_stone.m_high = std::ranges::max(points);   // 높은 쪽
        result.m_stone.m_low = std::ranges::min(points);    // 낮은 쪽
        result.m_stone.m_result = FindStoneResult(result.m_stone.m_high, result.m_stone.m_low);   // 이관 결과
        return result;   // 결과 반환
    }
}
