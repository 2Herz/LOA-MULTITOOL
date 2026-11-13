module LostArkMultiTool.Views.CombatSimulatorManager;  // 전투력 시뮬레이터 매니저 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.APPExceptions;  // 앱 예외 모듈 가져오기
import LostArkMultiTool.Core.BackgroundTask;  // 백그라운드 작업 모듈 가져오기
import LostArkMultiTool.Core.CombatExtraComponent;  // 추가 장착 배율 요소 모듈 가져오기 (모르는 혼돈 코어 검사)
import LostArkMultiTool.Core.CombatFactorComponent;  // 배율 요소 모듈 가져오기
import LostArkMultiTool.Core.CombatPowerCalculator;  // 전투력 계산기 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Core.LocalTime;  // 현지 시간 모듈 가져오기
import LostArkMultiTool.Core.MarketAPI;  // 시장 API 모듈 가져오기
import LostArkMultiTool.Core.Notifier;  // 알림 모듈 가져오기
import LostArkMultiTool.Core.NumberFormat;  // 숫자 서식 모듈 가져오기
import LostArkMultiTool.Core.Platform;  // 플랫폼 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Core.TextEncoding;  // 글자 인코딩 모듈 가져오기
import LostArkMultiTool.Core.ToolContext;  // 도구 문맥 모듈 가져오기
import LostArkMultiTool.Models.CharacterArmory;  // 캐릭터 무기고 모델 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기
import LostArkMultiTool.Models.CombatExtraState;  // 추가 장착 상태 모델 가져오기
import LostArkMultiTool.Models.CombatState;  // 전투력 상태 모델 가져오기
import LostArkMultiTool.Models.RateLimitState;  // 호출 한도 상태 모델 가져오기
import LostArkMultiTool.Services.CombatSimulatorStore;  // 전투력 시뮬레이터 저장소 가져오기
import LostArkMultiTool.Views.CombatExtraEditor;  // 추가 장착 편집기 가져오기
import LostArkMultiTool.Views.CombatSimulatorView;  // 전투력 시뮬레이터 화면 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        constexpr std::wstring_view TAB_TITLE{ L"전투력 시뮬레이터" };  // 탭 제목

        // ===== 입력 범위 =====
        constexpr int MIN_QUALITY{ 0 };  // 무기 품질 최소값
        constexpr int MAX_QUALITY{ 100 };  // 무기 품질 최대값
        constexpr std::size_t GEM_KEEP_INDEX{ 0 };   // 보석 목표 0번 = 현재 유지, 1번부터 = 6레벨 ..., 마지막 = 개별 설정
        constexpr std::size_t FIRST_GEM_LEVEL_INDEX{ 1 };   // 선택지 1번 = MIN_GEM_LEVEL
        constexpr int MIN_GEM_LEVEL{ 6 };            // 보석 목표 레벨 범위 (계수 표와 같은 6 ~ 10, 낮은 레벨은 쓰지 않음)
        constexpr int MAX_GEM_LEVEL{ 10 };  // 보석 최대 레벨
        constexpr std::string_view ESTHER_GRADE_NAME{ "에스더" };  // 에스더 등급 이름 (API 등급 글자)

        // ===== 숫자 표시 =====
        constexpr int POWER_DECIMALS{ 2 };  // 전투력 표시 소수 자릿수
        constexpr int LEVEL_DECIMALS{ 2 };  // 아이템 레벨 표시 소수 자릿수
        constexpr double PERCENT_SCALE{ 100.0 };     // 비율 → 퍼센트
        constexpr double NO_CHANGE_FACTOR{ 1.0 };  // 변화 없음 배율
        constexpr double CHANGE_EPSILON{ 1e-9 };     // 이보다 작은 차이는 변화 없음
        constexpr double FACTOR_DISPLAY_EPSILON{ 5e-5 };   // 배율 내역에서 ×1.0000으로 보이는 요소는 숨김
        constexpr int MAX_VAMBRACE_TARGET{ Models::MAX_VAMBRACE_LEVEL };  // 완갑 목표 최대 단계
        constexpr std::string_view VAMBRACE_GRADE_NAME{ "유물" };  // 완갑을 장착하는 등급 이름

        // ===== 문구 =====
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };  // 빈 칸 표시 글자
        constexpr std::wstring_view ENHANCEMENT_FORMAT{ L"+{}" };  // 일반 재련 표시 서식
        constexpr std::wstring_view GEM_KEEP_TEXT{ L"현재 유지" };  // 보석 현재 유지 선택지 글자
        constexpr std::wstring_view GEM_INDIVIDUAL_TEXT{ L"개별 설정" };   // 보석 표에서 하나씩 바꾼 상태
        constexpr std::wstring_view GEM_SLOT_FORMAT{ L"{}" };  // 보석 칸 번호 서식
        constexpr std::wstring_view GEM_LEVEL_FORMAT{ L"{}레벨" };  // 보석 레벨 서식
        constexpr std::wstring_view GEM_SUMMARY_FORMAT{ L"보석 {}개" };  // 보석 요약 서식
        constexpr std::wstring_view GEM_COUNT_FORMAT{ L" · {}레벨 {}개" };  // 레벨별 보석 개수 서식
        constexpr std::wstring_view NO_GEM_TEXT{ L"장착한 보석이 없어요." };  // 보석 없음 안내 글자
        constexpr std::wstring_view GEM_NOT_LOADED_TEXT{ L"보석: 조회 후 표시돼요." };  // 조회 전 보석 안내 글자
        constexpr std::wstring_view QUALITY_FORMAT{ L"무기 품질   현재 {}   →   목표" };  // 무기 품질 안내 서식
        constexpr std::wstring_view QUALITY_ESTHER_TEXT{ L"무기 품질   에스더 무기는 품질이 없어요" };  // 에스더 무기 품질 안내 글자
        constexpr std::wstring_view QUALITY_UNKNOWN_TEXT{ L"무기 품질   정보 없음" };  // 무기 품질 정보 없음 안내 글자

        constexpr std::wstring_view STATUS_FORMAT{ L"{} · {} · 아이템 레벨 {} · API 전투력 {}" };  // 상태 줄 서식
        constexpr std::wstring_view NO_POWER_TEXT{ L"정보 없음" };  // 전투력 정보 없음 글자
        constexpr std::wstring_view SEARCHING_FORMAT{ L"'{}' 무기고 조회 중..." };  // 조회 중 서식
        constexpr std::wstring_view NOT_SEARCHED_TEXT{ L"캐릭터 이름을 넣고 [조회]를 누르면 무기고를 불러와요." };  // 조회 전 안내 글자
        constexpr std::wstring_view SEARCH_ERROR_FORMAT{ L"조회하지 못했어요: {}" };  // 조회 실패 서식

        constexpr std::wstring_view CURRENT_POWER_FORMAT{ L"현재 전투력 (API)   {}" };  // 현재 전투력 서식
        constexpr std::wstring_view PREDICTED_POWER_FORMAT{ L"예상 전투력   {}" };  // 예상 전투력 서식
        constexpr std::wstring_view PREDICTED_FACTOR_FORMAT{ L"예상 배율   ×{:.4f}" };  // 예상 배율 서식
        constexpr std::wstring_view CHANGE_FORMAT{ L"증감   {}{} ({}{:.2f}%)" };  // 증감 서식
        constexpr std::wstring_view NO_POWER_CHANGE_TEXT{ L"API 전투력 정보가 없어서 배율만 보여 줘요." };  // 전투력 정보 없음 증감 안내
        constexpr std::wstring_view ITEM_LEVEL_FORMAT{ L"예상 아이템 레벨   {} → {}" };  // 예상 아이템 레벨 서식
        constexpr std::wstring_view FACTOR_PREFIX{ L"배율  " };  // 배율 내역 머리글
        constexpr std::wstring_view FACTOR_ITEM_FORMAT{ L"{} ×{:.4f}" };  // 배율 항목 서식
        constexpr std::wstring_view FACTOR_SEPARATOR{ L" · " };  // 배율 항목 구분 글자
        constexpr std::wstring_view PLUS_TEXT{ L"+" };  // 양수 부호 글자
        constexpr std::wstring_view NO_SIGN_TEXT{ L"" };  // 부호 없음 글자
        constexpr std::wstring_view FACTOR_GEAR_TEXT{ L"장비" };  // 장비 배율 이름
        constexpr std::wstring_view FACTOR_QUALITY_TEXT{ L"무기 품질" };  // 무기 품질 배율 이름
        constexpr std::wstring_view FACTOR_GEM_TEXT{ L"보석" };  // 보석 배율 이름
        constexpr std::wstring_view FACTOR_VAMBRACE_TEXT{ L"완갑" };  // 완갑 배율 이름
        constexpr std::wstring_view FACTOR_ACCESSORY_TEXT{ L"악세서리" };  // 악세서리 배율 이름
        constexpr std::wstring_view FACTOR_BRACELET_TEXT{ L"팔찌" };  // 팔찌 배율 이름
        constexpr std::wstring_view FACTOR_ENGRAVING_TEXT{ L"스톤 · 각인서" };  // 스톤 · 각인서 배율 이름
        constexpr std::wstring_view FACTOR_AVATAR_TEXT{ L"아바타" };  // 아바타 배율 이름
        constexpr std::wstring_view FACTOR_ARK_CORE_TEXT{ L"아크 코어" };  // 아크 코어 배율 이름
        constexpr std::wstring_view FACTOR_ARK_GEM_TEXT{ L"아크 젬" };  // 아크 젬 배율 이름
        constexpr std::wstring_view FACTOR_ARK_PASSIVE_TEXT{ L"아크 패시브" };  // 아크 패시브 배율 이름
        constexpr std::wstring_view NO_FACTOR_CHANGE_TEXT{ L"변화 없음" };  // 배율 변화 없음 글자
        constexpr std::wstring_view ERROR_RANGE_FORMAT{ L"   오차 범위 약 ±{:.2f}%" };  // 오차 범위 서식 (퍼센트)
        constexpr std::wstring_view ERROR_RANGE_POWER_FORMAT{ L" (±{})" };  // 오차 범위 전투력 서식 (전투력 정보가 있을 때만 덧붙임)
        constexpr std::wstring_view CHAOS_FALLBACK_NOTE{ L"처음 보는 혼돈 코어 이름은 같은 종류 혼돈 코어의 평균값으로 계산해요." };  // 모르는 혼돈 코어 평균 대체 안내
        constexpr std::string_view UNCERTAINTY_GEAR{ "GEAR" };  // 오차 표 키: 장비 재련
        constexpr std::string_view UNCERTAINTY_WEAPON_QUALITY{ "WEAPON_QUALITY" };  // 오차 표 키: 무기 품질 (장비 재련에 가장 가까운 값)
        constexpr std::string_view UNCERTAINTY_GEM{ "GEM" };  // 오차 표 키: 보석
        constexpr std::string_view UNCERTAINTY_VAMBRACE{ "VAMBRACE" };  // 오차 표 키: 완갑
        constexpr std::string_view UNCERTAINTY_ACCESSORY{ "ACCESSORY" };  // 오차 표 키: 악세서리
        constexpr std::string_view UNCERTAINTY_BRACELET{ "BRACELET" };  // 오차 표 키: 팔찌
        constexpr std::string_view UNCERTAINTY_BOOK{ "ENGRAVING_BOOK" };  // 오차 표 키: 각인서
        constexpr std::string_view UNCERTAINTY_STONE{ "STONE" };  // 오차 표 키: 스톤
        constexpr std::string_view UNCERTAINTY_AVATAR{ "AVATAR" };  // 오차 표 키: 아바타
        constexpr std::string_view UNCERTAINTY_ARK_CORE{ "ARK_CORE" };  // 오차 표 키: 아크 그리드 코어
        constexpr std::string_view UNCERTAINTY_ARK_GEM{ "ARK_GEM" };  // 오차 표 키: 아크 그리드 젬
        constexpr std::string_view UNCERTAINTY_ARK_PASSIVE{ "ARK_PASSIVE" };  // 오차 표 키: 아크 패시브
        constexpr std::string_view UNCERTAINTY_ARK_PASSIVE_PARTIAL{ "ARK_PASSIVE_PARTIAL" };  // 오차 표 키: 랭크가 모두 6이 아닌 아크 패시브
        constexpr std::string_view UNCERTAINTY_COMBO{ "COMBO_SURCHARGE" };  // 오차 표 키: 각인서 · 스톤 동시 변경 가산
        constexpr std::size_t COMBO_MIN_CHANGES{ 2 };  // 각인서 · 스톤이 이만큼 이상 동시에 바뀌면 가산 오차 적용
        constexpr std::wstring_view VAMBRACE_PART_TEXT{ L"완갑" };  // 장비 표 완갑 부위 이름
        constexpr std::wstring_view VAMBRACE_ITEM_FORMAT{ L"완갑 ({})" };  // 장비 표 완갑 아이템 이름 서식 (등급)
        constexpr std::wstring_view VAMBRACE_ITEM_CHANGE_FORMAT{ L"완갑 ({} → {})" };  // 목표 등급이 다를 때 서식 (현재 → 목표)

        constexpr std::wstring_view NOTICE_TITLE{ L"알림" };  // 알림 제목
        constexpr std::wstring_view ERROR_TITLE{ L"오류" };  // 오류 제목
        constexpr std::wstring_view RATE_LIMIT_TITLE{ L"호출 한도" };  // 호출 한도 제목
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"API 서버 점검" };  // API 서버 점검 제목
        constexpr std::wstring_view RATE_SHORTAGE_FORMAT{ L"남은 호출이 부족해서 이번 조회는 건너뛰었어요.\n{} 이후 다시 시도해 주세요." };  // 호출 한도 부족 안내 서식
        constexpr std::wstring_view API_KEY_ERROR_TITLE{ L"API 키 오류" };  // API 키 오류 제목
        constexpr std::wstring_view API_KEY_RETRY_TEXT{ L"\n\nAPI 키를 다시 입력할까요?" };  // API 키 재입력 질문
        constexpr std::wstring_view EMPTY_NAME_TEXT{ L"캐릭터 이름을 입력해 주세요." };  // 이름 입력 요청 안내
        constexpr std::wstring_view LOAD_WARNING_TITLE{ L"저장 파일 안내" };  // 저장 파일 안내 제목
        constexpr std::wstring_view SAVE_ERROR_TITLE{ L"저장 오류" };  // 저장 오류 제목
        constexpr std::wstring_view SAVE_ERROR_TEXT{ L"전투력 시뮬레이터 설정을 저장하지 못했어요.\n" };  // 저장 오류 안내
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };  // 알 수 없는 오류 글자
        constexpr std::wstring_view TRIM_CHARACTERS{ L" 　" };   // 일반 공백 · 전각 공백

        // 예외 메시지(UTF-8) → 화면 문구 (변환할 수 없으면 기본 문구)
        [[nodiscard]] std::wstring ToWideMessage(std::string_view message)  // UTF-8 메시지를 화면 글자로 변환
        {
            try  // 변환 시도
            {
                return Core::TextEncoding::UTF8ToWide(message);  // 화면 글자로 변환해 반환
            }
            catch (const std::exception&)  // 변환 실패 시
            {
                return std::wstring{ UNKNOWN_ERROR_TEXT };  // 알 수 없는 오류 글자 반환
            }
        }

        // 상태의 완갑 등급 이름 (계수 표 이름, 완갑이 없거나 모르면 빈 글자)
        [[nodiscard]] std::string_view GetVambraceGradeName(const Models::ExtraState& extra) noexcept  // 완갑 등급 이름 조회
        {
            if (!extra.m_vambraceLevel.has_value()) return {};  // 완갑이 없으면 빈 글자
            const Models::VambraceGrade grade{ extra.m_vambraceGrade.value_or(Models::CombatExtra::GetVambraceGradeForLevel(*extra.m_vambraceLevel)) };  // 등급 (없으면 단계의 기본 등급)
            const std::span<const Models::CombatCoefficientData::VambraceGradeBonus> bonuses{ Models::CombatCoefficientData::GetVambraceGradeBonuses() };  // 등급 표 (이름)
            const std::size_t index{ static_cast<std::size_t>(grade) };  // 등급 번호
            return index < bonuses.size() ? bonuses[index].m_name : std::string_view{};  // 범위 안이면 이름, 아니면 빈 글자
        }

        [[nodiscard]] std::wstring_view TrimSpaces(std::wstring_view text) noexcept  // 양 끝 공백 제거
        {
            const std::size_t first{ text.find_first_not_of(TRIM_CHARACTERS) };  // 첫 번째 공백 아닌 글자 위치
            if (first == std::wstring_view::npos) return {};  // 전부 공백이면 빈 글자 반환

            const std::size_t last{ text.find_last_not_of(TRIM_CHARACTERS) };  // 마지막 공백 아닌 글자 위치
            return text.substr(first, last - first + 1);  // 공백을 뺀 부분 반환
        }

        // 입력 글자 → 정수 (비었으면 nullopt, 쉼표 · 공백은 NumberFormat이 무시)
        [[nodiscard]] std::optional<int> ParseInteger(const std::wstring& text)  // 입력 글자를 정수로 변환
        {
            if (TrimSpaces(text).empty()) return std::nullopt;  // 비어 있으면 값 없음
            return static_cast<int>(Core::NumberFormat::ParseNumber(text));  // 숫자로 변환해 반환
        }

        // 선택지 번호 ↔ 보석 레벨 (1번 = 6레벨 ... 5번 = 10레벨)
        [[nodiscard]] int ToGemLevel(std::size_t index) noexcept  // 선택지 번호 → 보석 레벨
        {
            return MIN_GEM_LEVEL + static_cast<int>(index - FIRST_GEM_LEVEL_INDEX);  // 최소 레벨에 번호 차이를 더함
        }

        [[nodiscard]] std::size_t ToGemIndex(int level) noexcept  // 보석 레벨 → 선택지 번호
        {
            return FIRST_GEM_LEVEL_INDEX + static_cast<std::size_t>(level - MIN_GEM_LEVEL);  // 첫 레벨 번호에 레벨 차이를 더함
        }

        [[nodiscard]] bool IsGemLevelInRange(int level) noexcept  // 보석 레벨이 허용 범위 안인지 검사
        {
            return level >= MIN_GEM_LEVEL && level <= MAX_GEM_LEVEL;  // 최소 ~ 최대 레벨 안이면 true
        }

        // 개별 설정 선택지 번호 (현재 유지 + 6 ~ 10레벨 다음)
        [[nodiscard]] std::size_t GetGemIndividualIndex() noexcept  // "개별 설정" 선택지 번호
        {
            return ToGemIndex(MAX_GEM_LEVEL) + 1;  // 최대 레벨 선택지 바로 다음
        }

        // 계통 오차 표에서 키로 오차(%) 조회 (표에 없으면 0)
        [[nodiscard]] double GetUncertainty(std::string_view key) noexcept  // 계통 오차(%) 조회
        {
            const Models::CombatCoefficientData::KeyedValue* found{ Models::CombatCoefficientData::FindKeyed(Models::CombatCoefficientData::GetUncertainties(), key) };  // 오차 표에서 키 검색
            return found != nullptr ? found->m_value : 0.0;  // 찾았으면 값, 없으면 0
        }

        // 바뀐 각인서 · 스톤 줄 개수 (같은 칸끼리 비교)
        [[nodiscard]] std::size_t CountEngravingChanges(const Models::ExtraState& current, const Models::ExtraState& target) noexcept  // 바뀐 각인서 · 스톤 줄 수 계산
        {
            std::size_t count{};  // 바뀐 줄 수
            for (std::size_t index{}; index < Models::BOOK_COUNT; ++index) count += current.m_books[index] != target.m_books[index] ? 1U : 0U;  // 각인서 칸 비교
            for (std::size_t index{}; index < Models::STONE_LINE_COUNT; ++index) count += current.m_stone[index] != target.m_stone[index] ? 1U : 0U;  // 스톤 줄 비교
            return count;  // 바뀐 줄 수 반환
        }

        // 아크 패시브 세 트리의 랭크가 모두 최대(6)인지 (아니면 일부만 랭크가 찬 부캐)
        [[nodiscard]] bool IsPassiveFullRank(const Models::ExtraState& state) noexcept  // 패시브 트리 랭크 만렙 여부
        {
            return std::ranges::all_of(state.m_passive.m_treeRanks, [](int rank) { return rank == Models::MAX_PASSIVE_RANK; });  // 모든 트리 랭크가 최대인지
        }

        // 바뀐 계통의 오차(%) 선형 합 (변화 없는 계통은 더하지 않음, 하나도 안 바뀌면 0)
        [[nodiscard]] double EstimateErrorPercent(const Core::CombatPowerResult& result, const Models::CombatState& current, const Models::CombatState& target)  // 예상 오차(%) 계산
        {
            const Models::ExtraState& before{ current.m_extra };  // 현재 추가 장착 상태
            const Models::ExtraState& after{ target.m_extra };  // 목표 추가 장착 상태
            const bool bookChanged{ !std::ranges::equal(before.m_books, after.m_books) };  // 각인서가 바뀌었는지
            const bool stoneChanged{ !std::ranges::equal(before.m_stone, after.m_stone) };  // 스톤이 바뀌었는지
            double total{};  // 오차 합계 (%)
            for (const Core::CombatFactorResult& factor : result.m_factors)  // 요소별 배율 순회
            {
                if (std::abs(factor.m_factor - NO_CHANGE_FACTOR) <= CHANGE_EPSILON) continue;  // 바뀌지 않은 계통은 건너뜀
                switch (factor.m_kind)  // 계통별 오차 더하기
                {
                    case Core::CombatFactorKind::FACTOR_GEAR:
                        total += GetUncertainty(UNCERTAINTY_GEAR);  // 장비 재련
                        break;
                    case Core::CombatFactorKind::FACTOR_WEAPON_QUALITY:
                        total += GetUncertainty(UNCERTAINTY_WEAPON_QUALITY);  // 무기 품질
                        break;
                    case Core::CombatFactorKind::FACTOR_GEM:
                        total += GetUncertainty(UNCERTAINTY_GEM);  // 보석
                        break;
                    case Core::CombatFactorKind::FACTOR_VAMBRACE:
                        total += GetUncertainty(UNCERTAINTY_VAMBRACE);  // 완갑
                        break;
                    case Core::CombatFactorKind::FACTOR_ACCESSORY:
                        total += GetUncertainty(UNCERTAINTY_ACCESSORY);  // 악세서리
                        break;
                    case Core::CombatFactorKind::FACTOR_BRACELET:
                        total += GetUncertainty(UNCERTAINTY_BRACELET);  // 팔찌
                        break;
                    case Core::CombatFactorKind::FACTOR_ENGRAVING:
                        if (stoneChanged) total += GetUncertainty(UNCERTAINTY_STONE);  // 스톤이 바뀌었으면 스톤 오차
                        if (bookChanged || !stoneChanged) total += GetUncertainty(UNCERTAINTY_BOOK);  // 각인서가 바뀌었거나 원인을 모르면 각인서 오차
                        break;
                    case Core::CombatFactorKind::FACTOR_AVATAR:
                        total += GetUncertainty(UNCERTAINTY_AVATAR);  // 아바타
                        break;
                    case Core::CombatFactorKind::FACTOR_ARK_CORE:
                        total += GetUncertainty(UNCERTAINTY_ARK_CORE);  // 아크 그리드 코어
                        break;
                    case Core::CombatFactorKind::FACTOR_ARK_GEM:
                        total += GetUncertainty(UNCERTAINTY_ARK_GEM);  // 아크 그리드 젬 (아크 그리드 오차 사용)
                        break;
                    case Core::CombatFactorKind::FACTOR_ARK_PASSIVE:
                        total += GetUncertainty(IsPassiveFullRank(before) && IsPassiveFullRank(after) ? UNCERTAINTY_ARK_PASSIVE : UNCERTAINTY_ARK_PASSIVE_PARTIAL);  // 랭크가 모두 6이 아니면 큰 오차
                        break;
                }
            }
            if (CountEngravingChanges(before, after) >= COMBO_MIN_CHANGES) total += GetUncertainty(UNCERTAINTY_COMBO);  // 각인서 · 스톤을 두 줄 이상 함께 바꾸면 가산 오차
            return total;  // 오차 합계 반환
        }

        [[nodiscard]] std::wstring_view GetFactorName(Core::CombatFactorKind kind) noexcept  // 배율 요소 종류 → 화면 이름
        {
            switch (kind)  // 종류별 분기
            {
                case Core::CombatFactorKind::FACTOR_GEAR:
                    return FACTOR_GEAR_TEXT;  // 장비 이름 반환
                case Core::CombatFactorKind::FACTOR_WEAPON_QUALITY:
                    return FACTOR_QUALITY_TEXT;  // 무기 품질 이름 반환
                case Core::CombatFactorKind::FACTOR_GEM:
                    return FACTOR_GEM_TEXT;  // 보석 이름 반환
                case Core::CombatFactorKind::FACTOR_VAMBRACE:
                    return FACTOR_VAMBRACE_TEXT;  // 완갑 이름 반환
                case Core::CombatFactorKind::FACTOR_ACCESSORY:
                    return FACTOR_ACCESSORY_TEXT;  // 악세서리 이름 반환
                case Core::CombatFactorKind::FACTOR_BRACELET:
                    return FACTOR_BRACELET_TEXT;  // 팔찌 이름 반환
                case Core::CombatFactorKind::FACTOR_ENGRAVING:
                    return FACTOR_ENGRAVING_TEXT;  // 스톤 · 각인서 이름 반환
                case Core::CombatFactorKind::FACTOR_AVATAR:
                    return FACTOR_AVATAR_TEXT;  // 아바타 이름 반환
                case Core::CombatFactorKind::FACTOR_ARK_CORE:
                    return FACTOR_ARK_CORE_TEXT;  // 아크 코어 이름 반환
                case Core::CombatFactorKind::FACTOR_ARK_GEM:
                    return FACTOR_ARK_GEM_TEXT;  // 아크 젬 이름 반환
                case Core::CombatFactorKind::FACTOR_ARK_PASSIVE:
                    return FACTOR_ARK_PASSIVE_TEXT;  // 아크 패시브 이름 반환
            }
            return EMPTY_TEXT;  // 알 수 없는 종류는 빈 칸 글자 반환
        }

        // 부호를 붙인 글자 앞부분 (양수 = "+", 그 밖 = 없음 → 음수는 숫자에 '-'가 붙어 있음)
        [[nodiscard]] std::wstring_view GetSignText(double value) noexcept  // 값의 부호 글자
        {
            return value > CHANGE_EPSILON ? PLUS_TEXT : NO_SIGN_TEXT;  // 양수이면 "+", 아니면 빈 글자
        }

        [[nodiscard]] std::wstring FormatEnhancement(int enhancement)  // 일반 재련 단계 글자
        {
            return std::format(ENHANCEMENT_FORMAT, enhancement);  // "+단계" 서식으로 변환
        }

        // 상급 칸 글자 (상급 재련이 없는 분류 · 단계면 "-")
        [[nodiscard]] std::wstring FormatAdvanced(const Models::GearState& gear)  // 상급 재련 단계 글자
        {
            if (!gear.IsKnown() || (gear.m_advancedEnhancement <= 0 && !Models::CombatGear::CanAdvance(gear))) return std::wstring{ EMPTY_TEXT };  // 계수 표가 없거나 상급 불가 장비면 빈 칸 글자
            return std::to_wstring(gear.m_advancedEnhancement);  // 상급 단계를 문자열로 변환
        }

        // 무기고의 장비 하나 → 현재 상태 (계수 표에 없거나 지금 단계를 표에서 찾을 수 없으면 모르는 장비)
        [[nodiscard]] Models::GearState MakeGearState(Models::GearPart part, const Models::GearItem& item)  // API 장비를 재련 상태로 변환
        {
            Models::GearState gear{ Models::CombatGear::DetectTableKey(part, item.m_name, item.m_grade), item.m_enhancement, item.m_advancedEnhancement };  // 분류 키 · 일반 단계 · 상급 단계로 상태 생성
            if (gear.IsKnown() && !Models::CombatGear::GetStat(gear).has_value()) gear.m_tableKey.clear();  // 능력치를 조회할 수 없는 표면 계산 대상에서 제외
            return gear;  // 재련 상태 반환
        }
    }

    CombatSimulatorManager::CombatSimulatorManager() = default;  // 생성자 기본 구현 (싱글톤)

    CombatSimulatorManager::~CombatSimulatorManager() = default;  // 소멸자 기본 구현

    std::wstring_view CombatSimulatorManager::GetTabTitle() const noexcept  // 탭 제목 조회
    {
        return TAB_TITLE;  // 탭 제목 반환
    }

    void CombatSimulatorManager::Initialize(Core::ToolContext& context)  // 초기화
    {
        m_context = &context;  // 도구 문맥 보관

        // 모델: 마지막 캐릭터 이름 · 목표 (CombatSimulatorData.json) + 배율 계산기
        m_store = std::make_unique<Services::CombatSimulatorStore>();  // 저장소 생성 (저장 파일 불러오기)
        if (const std::optional<std::string>& warning{ m_store->GetLoadWarning() }; warning.has_value())  // 불러올 때 경고가 있으면
        {
            context.m_notifier.ShowMessage(std::wstring{ LOAD_WARNING_TITLE }, ToWideMessage(*warning), Core::NoticeKind::NOTICE_WARNING, nullptr);  // 경고 알림 표시
        }
        m_calculator = std::make_unique<Core::CombatPowerCalculator>();  // 계산기 생성

        // 화면: 사용자 동작을 매니저 함수로 연결
        m_editor = std::make_unique<CombatExtraEditor>();  // 편집기 생성
        m_view = std::make_unique<CombatSimulatorView>(context.m_platform);  // 화면 생성
        m_view->SetOnSearch([this](const std::wstring& name) { RequestSearch(name); });  // 조회 알림 연결
        m_view->SetOnGearCellEdited([this](std::size_t row, CombatGearColumn column, const std::wstring& text) { OnGearCellEdited(row, column, text); });  // 장비 칸 편집 알림 연결
        m_view->SetOnQualityChanged([this](const std::wstring& text) { OnQualityChanged(text); });  // 무기 품질 변경 알림 연결
        m_view->SetOnGemTargetSelected([this](std::size_t index) { OnGemTargetSelected(index); });  // 보석 목표 선택 알림 연결
        m_view->SetOnGemCellEdited([this](std::size_t row, const std::wstring& text) { OnGemCellEdited(row, text); });  // 보석 칸 편집 알림 연결
        m_view->SetOnCardEdited([this](CombatCard card, std::size_t row, const std::wstring& text) { OnCardEdited(card, row, text); });  // 카드 편집 알림 연결

        // 보석 목표 선택지: 현재 유지 + 6 ~ 10레벨 + 개별 설정 (보석 표에서 하나씩 바꾸면 이 항목이 표시됨)
        std::vector<std::wstring> gemItems;  // 보석 전체 목표 선택지 목록
        gemItems.emplace_back(GEM_KEEP_TEXT);  // "현재 유지" 선택지
        for (int level{ MIN_GEM_LEVEL }; level <= MAX_GEM_LEVEL; ++level) gemItems.push_back(std::format(GEM_LEVEL_FORMAT, level));  // 최소 ~ 최대 레벨 선택지 추가
        gemItems.emplace_back(GEM_INDIVIDUAL_TEXT);  // "개별 설정" 선택지
        m_view->SetGemTargetItems(std::move(gemItems));  // 화면에 선택지 전달

        // 마지막으로 조회한 이름을 넣어 둠 (조회는 사용자가 누를 때만 → 호출 한도 절약)
        m_view->SetCharacterName(ToWideMessage(m_store->GetSettings().m_characterName));  // 저장된 캐릭터 이름 표시
        RefreshAll();  // 전체 화면 갱신
    }

    void CombatSimulatorManager::Shutdown()  // 종료
    {
        // 진행 중인 요청이 끝날 때까지 기다린 뒤 정리 (Application이 공용 API를 지우기 전에)
        m_armoryTask.Wait();  // 진행 중인 백그라운드 조회가 끝날 때까지 대기

        m_view.reset();  // 화면 해제
        m_editor.reset();  // 편집기 해제
        m_calculator.reset();  // 계산기 해제
        m_store.reset();  // 저장소 해제
        m_context = nullptr;  // 도구 문맥 해제
    }

    void CombatSimulatorManager::OnActivated()  // 탭이 선택되었을 때
    {
        if (m_view) m_view->RestoreFocus();  // 입력 포커스 복원
    }

    void CombatSimulatorManager::OnDeactivated()  // 탭이 선택 해제되었을 때
    {
        if (m_view) m_view->SuspendFocus();  // 입력 포커스 일시 해제
    }

    void CombatSimulatorManager::Update(double deltaSeconds)  // 프레임 갱신
    {
        if (!m_view) return;  // 화면이 없으면 중단

        m_view->Update(deltaSeconds);  // 화면 갱신
        if (m_armoryTask.IsReady()) FinishSearch();  // 백그라운드 조회가 끝났으면 결과 처리
    }

    void CombatSimulatorManager::Render(Core::Renderer& renderer, const Core::RectF& contentArea)  // 화면 그리기
    {
        if (m_view) m_view->Render(renderer, contentArea);  // 화면이 있으면 그리기
    }

    bool CombatSimulatorManager::HandleInput(const Core::InputEvent& event)  // 입력 처리
    {
        return m_view && m_view->HandleInput(event);  // 화면이 있으면 입력 전달
    }

    Core::CursorShape CombatSimulatorManager::GetCursor() const noexcept
    {
        return m_view ? m_view->GetCursor() : Core::CursorShape::CURSOR_ARROW;  // 화면이 있으면 커서 모양 반환, 없으면 화살표
    }

    // ===================== 조회 =====================

    void CombatSimulatorManager::RequestSearch(const std::wstring& characterName)
    {
        const std::wstring_view trimmedName{ TrimSpaces(characterName) };  // 양 끝 공백을 뺀 이름
        if (trimmedName.empty())  // 이름이 비어 있으면
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE }, std::wstring{ EMPTY_NAME_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);  // 이름 입력 안내 알림 표시
            return;  // 조회하지 않고 종료
        }
        if (m_armoryTask.IsRunning()) return;   // 이미 조회 중이면 겹치지 않게 건너뜀

        // 남은 호출이 안전 여유분 이하면 건너뜀 (다른 툴과 키를 함께 쓰므로)
        const std::int64_t now{ Core::LocalTime::GetUnixSeconds() };  // 현재 시각(초)
        const Models::RateLimitState rate{ m_context->m_marketAPI.GetRateLimit() };  // API 호출 한도 상태
        if (rate.IsExhausted(now))  // 남은 호출이 부족하면
        {
            m_context->m_notifier.ShowMessage(std::wstring{ NOTICE_TITLE },  // 부족 안내 알림 (초기화 시각 포함)
                std::format(RATE_SHORTAGE_FORMAT, Core::LocalTime::FormatClock(rate.GetResetAtLocal().value_or(now))), Core::NoticeKind::NOTICE_INFO, nullptr);
            return;  // 조회하지 않고 종료
        }

        m_searchError.reset();  // 이전 조회 오류 지움
        m_view->SetSearchBusy(true);  // 조회 중 표시
        m_view->SetStatusText(std::format(SEARCHING_FORMAT, trimmedName), false);  // 조회 중 상태 글자 표시

        Core::MarketAPI* marketAPI{ &m_context->m_marketAPI };  // 시장 API 포인터 (백그라운드에서 사용)
        m_armoryTask.Start([marketAPI, name = Core::TextEncoding::WideToUTF8(trimmedName)] { return marketAPI->GetArmory(name); });  // 백그라운드로 무기고 조회 시작 (이름은 UTF-8로 변환)
    }

    // 조회가 끝남 → 무기고 반영 · 오류 처리
    void CombatSimulatorManager::FinishSearch()
    {
        m_view->SetSearchBusy(false);  // 조회 중 표시 해제
        std::optional<std::string> keyError;  // API 키 오류 메시지 보관

        try  // 조회 결과 적용 시도
        {
            ApplyArmory(m_armoryTask.TakeResult());   // 백그라운드에서 난 예외는 여기서 다시 던져짐
        }
        catch (const Core::APIKeyInvalidException& error)  // API 키 오류
        {
            keyError = error.what();  // 키 오류 메시지 보관
            m_searchError = ToWideMessage(error.what());  // 상태 줄에 표시할 오류 보관
        }
        catch (const Core::APIRateLimitException& error)  // 호출 한도 초과 오류
        {
            m_searchError = ToWideMessage(error.what());  // 상태 줄에 표시할 오류 보관
            m_context->m_notifier.ShowMessage(std::wstring{ RATE_LIMIT_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_WARNING, nullptr);  // 경고 알림 표시
        }
        catch (const Core::APIMaintenanceException& error)  // 서버 점검 오류
        {
            m_searchError = ToWideMessage(error.what());  // 상태 줄에 표시할 오류 보관
            m_context->m_notifier.ShowMessage(std::wstring{ MAINTENANCE_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_INFO, nullptr);  // 안내 알림 표시
        }
        catch (const std::exception& error)  // 그 밖의 오류
        {
            m_searchError = ToWideMessage(error.what());   // 없는 캐릭터 · 통신 오류 등
            m_context->m_notifier.ShowMessage(std::wstring{ ERROR_TITLE }, *m_searchError, Core::NoticeKind::NOTICE_ERROR, nullptr);  // 오류 알림 표시
        }

        RefreshAll();  // 화면 전체 갱신

        // 조회가 끝난 뒤에 키 재입력 안내
        if (keyError.has_value()) HandleAPIKeyError(*keyError);  // 키 오류였으면 화면 갱신 뒤에 재입력 질문
    }

    // API 키 오류(401): 창이 보이면 다시 입력할지 묻고, 숨어 있으면 풍선 알림 (다른 탭과 같음)
    void CombatSimulatorManager::HandleAPIKeyError(const std::string& message)
    {
        const std::wstring reason{ ToWideMessage(message) };  // 오류 이유 글자
        if (!m_context->m_platform.IsWindowShown())  // 창이 숨겨져 있으면
        {
            m_context->m_notifier.ShowToast(reason, Core::NoticeKind::NOTICE_ERROR);  // 토스트 알림으로 오류 표시
            return;  // 질문하지 않고 종료
        }

        m_context->m_notifier.AskYesNo(std::wstring{ API_KEY_ERROR_TITLE }, reason + std::wstring{ API_KEY_RETRY_TEXT },  // API 키를 다시 입력할지 질문
            [openAPIKeyDialog = m_context->m_openAPIKeyDialog](bool isYes)  // 예를 선택하면 API 키 입력창을 열 함수
        {
            if (isYes && openAPIKeyDialog) openAPIKeyDialog();  // 예 선택 시 입력창 열기
        });
    }

    // 무기고 → 현재 상태, 목표 = 현재 (같은 캐릭터를 다시 조회했으면 저장된 목표를 범위 안으로 맞춰서 복원)
    void CombatSimulatorManager::ApplyArmory(Models::CharacterArmory armory)
    {
        Models::CombatState current;  // 현재 상태
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 부위 개수만큼 순회
        {
            if (const std::optional<Models::GearItem>& item{ armory.m_gears[index] }; item.has_value())  // 장비가 있는 부위만
            {
                current.m_gears[index] = MakeGearState(Models::ToGearPart(index), *item);  // API 장비를 재련 상태로 변환
            }
        }

        // 무기 품질: 에스더 무기는 품질이 없음
        const std::optional<Models::GearItem>& weapon{ armory.m_gears[Models::ToGearIndex(Models::GearPart::PART_WEAPON)] };  // 무기 장비
        const bool isEstherWeapon{ weapon.has_value() && (weapon->m_grade == ESTHER_GRADE_NAME  // 에스더 무기 여부 (등급이나 분류 키로 판별)
            || Models::CombatGear::IsEsther(current.m_gears[Models::ToGearIndex(Models::GearPart::PART_WEAPON)].m_tableKey)) };
        if (weapon.has_value() && !isEstherWeapon) current.m_weaponQuality = weapon->m_quality;  // 에스더가 아닌 무기는 품질 기록

        current.m_gemLevels = armory.m_gemLevels;  // 보석 레벨 복사
        current.m_gemKinds = armory.m_gemKinds;  // 보석 종류 복사
        current.m_totalMainStat = armory.m_totalMainStat * Models::CombatCoefficientData::GetMainStatCalibration();   // 로아랩 실측 보정
        current.m_gemBaseAttackRate = armory.m_gemBaseAttackRate;  // 보석 기본 공격력 합 복사
        current.m_weaponAttack = weapon.has_value() ? weapon->m_weaponAttack : 0.0;  // 무기 툴팁 공격력 기록
        current.m_itemLevel = armory.m_itemAvgLevel;  // 평균 아이템 레벨 기록
        current.m_extra = armory.m_extra;  // 추가 장착 상태 복사
        current.m_extra.m_isEstherWeapon = isEstherWeapon;   // 완갑 표 · 무기 공격력 옵션 계수 선택

        Models::CombatState target{ current };  // 목표 상태는 현재 상태에서 시작
        Services::CombatSimulatorSettings& settings{ m_store->GetSettings() };  // 저장 설정
        if (settings.m_characterName == armory.m_characterName)  // 같은 캐릭터이면 저장한 목표 복원
        {
            for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 부위 순회
            {
                Models::GearState& gear{ target.m_gears[index] };  // 목표 부위 상태
                const std::optional<Services::CombatGearTarget>& saved{ settings.m_gearTargets[index] };  // 저장된 부위 목표
                if (!gear.IsKnown() || !saved.has_value()) continue;  // 표가 없거나 저장 값이 없으면 건너뜀
                gear.m_enhancement = saved->m_enhancement;  // 저장한 일반 재련 단계 반영
                gear.m_advancedEnhancement = saved->m_advancedEnhancement;  // 저장한 상급 재련 단계 반영
                gear = Models::CombatGear::Clamp(gear);  // 입력 범위로 보정
            }
            if (target.m_weaponQuality.has_value() && settings.m_weaponQualityTarget.has_value())  // 무기 품질 목표가 저장되어 있으면
            {
                target.m_weaponQuality = std::clamp(*settings.m_weaponQualityTarget, MIN_QUALITY, MAX_QUALITY);  // 품질 범위로 보정해 반영
            }

            // 보석: 저장된 개수가 지금 장착 개수와 같을 때만 복원 (보석을 바꿔 끼웠으면 현재 유지)
            if (settings.m_gemTargets.size() == target.m_gemLevels.size())  // 보석 개수가 같을 때만 복원
            {
                for (std::size_t index{}; index < target.m_gemLevels.size(); ++index)  // 보석마다
                {
                    // 6 ~ 10만 목표로 복원 (그 밖의 값 = 현재 그대로 둔 보석 → 현재 레벨 유지)
                    if (IsGemLevelInRange(settings.m_gemTargets[index])) target.m_gemLevels[index] = settings.m_gemTargets[index];  // 허용 범위의 저장 레벨만 반영
                }
            }

            // 악세서리 · 팔찌 · 스톤 · 각인서 · 아바타 · 아크 · 완갑: 저장된 목표 (완갑이 없어졌거나 새로 생겼으면 현재 기준)
            if (settings.m_extraTarget.has_value())  // 추가 시스템 목표가 저장되어 있으면
            {
                Models::ExtraState saved{ *settings.m_extraTarget };  // 저장된 목표 복사
                saved.m_isEstherWeapon = current.m_extra.m_isEstherWeapon;  // 에스더 여부는 현재 값 사용
                for (std::size_t core{}; core < Models::ARK_CORE_COUNT; ++core) saved.m_cores[core].m_name = current.m_extra.m_cores[core].m_name;   // 코어 이름은 저장하지 않음 (현재 기준)
                if (!current.m_extra.m_vambraceLevel.has_value()) saved.m_vambraceLevel.reset();  // 현재 완갑이 없으면 목표도 없음
                else if (!saved.m_vambraceLevel.has_value()) saved.m_vambraceLevel = current.m_extra.m_vambraceLevel;  // 저장 목표에 완갑이 없으면 현재 값으로 채움
                if (!saved.m_vambraceGrade.has_value()) saved.m_vambraceGrade = current.m_extra.m_vambraceGrade;  // 저장 목표에 완갑 등급이 없으면 현재 등급으로 채움 (단계와 안 맞으면 Clamp가 맞춤)
                saved.m_vambraceGradeRate = current.m_extra.m_vambraceGradeRate;  // 추가 효과 툴팁 값은 저장하지 않음 (현재 기준)
                target.m_extra = Models::CombatExtra::Clamp(saved);  // 범위 보정 후 목표에 반영
            }
        }
        else
        {  // 다른 캐릭터이면
            // 다른 캐릭터: 목표 초기화
            settings = Services::CombatSimulatorSettings{};  // 저장 설정 초기화
            settings.m_characterName = armory.m_characterName;  // 새 캐릭터 이름 기록
        }

        m_current = std::move(current);  // 현재 상태 확정
        m_target = std::move(target);  // 목표 상태 확정
        m_armory = std::move(armory);  // 무기고 보관
        StoreTargets();  // 목표를 저장 설정에 기록
        SaveData();  // 파일에 저장
    }

    // ===================== 사용자 입력 =====================

    // 목표 일반 · 상급 숫자 입력 → 표 범위 안으로 맞춰서 반영
    void CombatSimulatorManager::OnGearCellEdited(std::size_t row, CombatGearColumn column, const std::wstring& text)
    {
        if (row == Models::GEAR_PART_COUNT)  // 완갑 행이면
        {
            OnVambraceEdited(column, text);   // 마지막 행 = 완갑
            return;  // 처리 끝
        }
        if (row > Models::GEAR_PART_COUNT) return;  // 부위 범위 밖 행이면 무시
        Models::GearState& gear{ m_target.m_gears[row] };  // 목표 부위 상태
        const std::optional<int> value{ ParseInteger(text) };  // 입력 글자를 정수로 변환
        if (!gear.IsKnown() || !value.has_value())  // 표가 없거나 숫자가 아니면
        {
            RefreshGears();   // 숫자가 아니면 원래 값으로 되돌림
            return;  // 처리 끝
        }

        Models::GearState edited{ gear };  // 편집 중인 복사본
        if (column == CombatGearColumn::COLUMN_TARGET_ENHANCEMENT) edited.m_enhancement = *value;  // 목표 일반 재련 열이면 일반 단계 반영
        else if (column == CombatGearColumn::COLUMN_TARGET_ADVANCED) edited.m_advancedEnhancement = *value;  // 목표 상급 재련 열이면 상급 단계 반영
        else return;  // 그 밖의 열은 무시

        gear = Models::CombatGear::Clamp(edited);   // 일반 단계가 내려가 상급을 못 넣게 되면 상급은 0
        StoreTargets();  // 목표 기록
        SaveData();  // 파일에 저장
        RefreshGears();  // 장비 표 갱신
        RefreshResult();  // 결과 갱신
    }

    // 무기 품질 목표: 숫자일 때만 반영 (0 ~ 100으로 맞추고, 넘으면 칸도 고침)
    void CombatSimulatorManager::OnQualityChanged(const std::wstring& text)
    {
        if (!m_target.m_weaponQuality.has_value()) return;  // 무기 품질이 없으면 무시
        const std::optional<int> value{ ParseInteger(text) };  // 입력 글자를 정수로 변환
        if (!value.has_value()) return;  // 숫자가 아니면 무시

        const int quality{ std::clamp(*value, MIN_QUALITY, MAX_QUALITY) };  // 품질 범위로 보정
        if (quality != *value) m_view->SetQualityTarget(std::to_wstring(quality));  // 범위를 벗어났으면 입력 칸을 보정한 값으로 되돌림
        if (quality == *m_target.m_weaponQuality) return;  // 값이 그대로면 종료

        m_target.m_weaponQuality = quality;  // 목표 품질 기록
        StoreTargets();  // 목표를 저장 설정에 기록
        SaveData();  // 파일에 저장
        RefreshResult();  // 결과 갱신
    }

    // 모든 보석 목표: 현재 유지 = 현재 레벨로 되돌림, N레벨 = 모두 N, 개별 설정 = 그대로 (표에서 고친 값 유지)
    void CombatSimulatorManager::OnGemTargetSelected(std::size_t index)
    {
        if (index >= GetGemIndividualIndex())  // "개별 설정" 선택이면
        {
            RefreshGems();  // 보석 표만 되돌림
            return;  // 처리 끝
        }

        m_target.m_gemLevels = m_current.m_gemLevels;  // 목표 보석을 현재 레벨로 되돌림
        if (index != GEM_KEEP_INDEX) std::ranges::fill(m_target.m_gemLevels, ToGemLevel(index));   // 6레벨 미만 보석도 고른 레벨로 (그 보석 배율은 1)
        StoreTargets();  // 목표를 저장 설정에 기록
        SaveData();  // 파일에 저장
        RefreshGems();  // 보석 표 갱신
        RefreshResult();  // 결과 갱신
    }

    // 보석 하나 목표 레벨 입력 → 1 ~ 10으로 맞춰서 그 보석만 덮어씀
    void CombatSimulatorManager::OnGemCellEdited(std::size_t row, const std::wstring& text)
    {
        const std::optional<int> value{ ParseInteger(text) };  // 입력 글자를 정수로 변환
        if (row >= m_target.m_gemLevels.size() || !value.has_value())  // 행 번호가 범위 밖이거나 숫자가 아니면
        {
            RefreshGems();   // 숫자가 아니면 원래 값으로 되돌림
            return;  // 처리 끝
        }

        m_target.m_gemLevels[row] = std::clamp(*value, MIN_GEM_LEVEL, MAX_GEM_LEVEL);   // 6 ~ 10 (현재가 6 미만인 보석은 배율 1)
        StoreTargets();  // 목표를 저장 설정에 기록
        SaveData();  // 파일에 저장
        RefreshGems();  // 보석 표 갱신
        RefreshResult();  // 결과 갱신
    }

    // 완갑 목표 재련 입력 (0 ~ 25로 맞춤, 완갑이 없거나 숫자가 아니면 원래 값으로)
    void CombatSimulatorManager::OnVambraceEdited(CombatGearColumn column, const std::wstring& text)
    {
        const std::optional<int> value{ ParseInteger(text) };  // 입력 글자를 정수로 변환
        if (column != CombatGearColumn::COLUMN_TARGET_ENHANCEMENT || !m_target.m_extra.m_vambraceLevel.has_value() || !value.has_value())  // 목표 일반 재련 열이 아니거나 완갑이 없거나 숫자가 아니면
        {
            RefreshGears();  // 장비 표를 원래 값으로 되돌림
            return;  // 처리 끝
        }

        m_target.m_extra.m_vambraceLevel = std::clamp(*value, Models::MIN_VALUE, MAX_VAMBRACE_TARGET);  // 완갑 목표 단계를 범위 안으로 보정해 기록
        m_target.m_extra = Models::CombatExtra::Clamp(m_target.m_extra);  // 고른 등급과 맞지 않는 단계면 등급을 단계의 기본 등급으로 맞춤
        StoreTargets();  // 목표를 저장 설정에 기록
        SaveData();  // 파일에 저장
        RefreshGears();  // 장비 표 갱신
        RefreshCards();  // 완갑 등급 행이 있는 카드도 갱신
        RefreshResult();  // 결과 갱신
    }

    // 목록형 카드 입력 (편집기가 목표 상태에 반영 → 범위 맞춤 → 저장 · 화면 갱신)
    void CombatSimulatorManager::OnCardEdited(CombatCard card, std::size_t row, const std::wstring& text)
    {
        if (!m_editor) return;  // 편집기가 없으면 무시
        Models::ExtraState edited{ m_target.m_extra };  // 편집 중인 복사본
        if (m_editor->Apply(card, row, text, edited)) m_target.m_extra = Models::CombatExtra::Clamp(edited);  // 편집을 반영하고 범위 보정

        StoreTargets();  // 목표를 저장 설정에 기록
        SaveData();  // 파일에 저장
        RefreshGears();   // 완갑 표시는 장비 표에 있어서 함께 갱신
        RefreshCards();  // 카드 갱신
        RefreshResult();  // 결과 갱신
    }

    // ===================== 화면 갱신 · 저장 =====================

    void CombatSimulatorManager::RefreshAll()
    {
        RefreshStatus();  // 상태 줄 갱신
        RefreshGears();  // 장비 표 갱신
        RefreshOptions();  // 보석 · 품질 옵션 갱신
        RefreshCards();  // 목록형 카드 갱신
        RefreshResult();  // 결과 갱신
    }

    // 상태 줄: 조회 중이면 그대로, 오류 · 조회 전 · 캐릭터 요약
    void CombatSimulatorManager::RefreshStatus()
    {
        if (!m_view || m_armoryTask.IsRunning()) return;  // 화면이 없거나 조회 중이면 건너뜀

        if (m_searchError.has_value())  // 조회 오류가 있으면
        {
            m_view->SetStatusText(std::format(SEARCH_ERROR_FORMAT, *m_searchError), true);  // 오류 상태 글자 표시
            return;  // 처리 끝
        }
        if (!m_armory.has_value())  // 조회한 무기고가 없으면
        {
            m_view->SetStatusText(std::wstring{ NOT_SEARCHED_TEXT }, false);  // 조회 안내 글자 표시
            return;  // 처리 끝
        }

        const std::wstring power{ m_armory->m_combatPower.has_value()  // API 전투력 글자 (없으면 정보 없음)
            ? Core::NumberFormat::FormatNumber(*m_armory->m_combatPower, POWER_DECIMALS) : std::wstring{ NO_POWER_TEXT } };
        m_view->SetStatusText(std::format(STATUS_FORMAT, ToWideMessage(m_armory->m_characterName), ToWideMessage(m_armory->m_className),  // 캐릭터 · 직업 · 아이템 레벨 · 전투력 상태 글자 표시
            Core::NumberFormat::FormatNumber(m_armory->m_itemAvgLevel, LEVEL_DECIMALS), power), false);
    }

    // 장비 표: 부위마다 한 행 (조회 전 · 안 낀 부위는 "-", 계수 표가 없는 장비는 목표 입력 잠금)
    void CombatSimulatorManager::RefreshGears()
    {
        if (!m_view) return;  // 화면이 없으면 중단

        std::vector<CombatGearRow> rows;  // 장비 표 행 목록
        rows.reserve(Models::GEAR_PART_COUNT);  // 부위 개수만큼 메모리 미리 확보
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 부위마다
        {
            CombatGearRow& row{ rows.emplace_back() };  // 새 행 추가
            row.m_partName = ToWideMessage(Models::GetGearPartType(Models::ToGearPart(index)));  // 부위 이름 글자
            row.m_itemName = std::wstring{ EMPTY_TEXT };  // 아이템 이름 기본값은 빈 칸 글자
            row.m_currentEnhancement = row.m_currentAdvanced = row.m_targetEnhancement = row.m_targetAdvanced = std::wstring{ EMPTY_TEXT };  // 현재 · 목표 재련 칸 기본값은 빈 칸 글자

            const std::optional<Models::GearItem>* item{ m_armory.has_value() ? &m_armory->m_gears[index] : nullptr };  // 이 부위의 API 장비 (조회 전이면 없음)
            if (item == nullptr || !item->has_value()) continue;  // 장비가 없으면 다음 부위로

            row.m_itemName = ToWideMessage((*item)->m_name);  // 아이템 이름 글자
            row.m_gradeName = (*item)->m_grade;  // 등급 이름 (글자색용)
            row.m_currentEnhancement = FormatEnhancement((*item)->m_enhancement);  // 현재 일반 재련 글자
            row.m_currentAdvanced = (*item)->m_advancedEnhancement > 0 ? std::to_wstring((*item)->m_advancedEnhancement) : std::wstring{ EMPTY_TEXT };  // 현재 상급 재련 글자 (없으면 빈 칸 글자)

            const Models::GearState& current{ m_current.m_gears[index] };  // 현재 부위 상태
            const Models::GearState& target{ m_target.m_gears[index] };  // 목표 부위 상태
            if (!target.IsKnown()) continue;   // 계수 표가 없는 장비: 목표 "-"

            row.m_currentAdvanced = FormatAdvanced(current);  // 상급 재련 표시를 규칙에 맞게 갱신
            row.m_targetEnhancement = FormatEnhancement(target.m_enhancement);  // 목표 일반 재련 글자
            row.m_targetAdvanced = FormatAdvanced(target);  // 목표 상급 재련 글자
            row.m_isTargetEditable = true;  // 표가 있으므로 목표 편집 가능
            row.m_isTargetAdvancedEditable = Models::CombatGear::CanAdvance(target);  // 상급 재련 편집 가능 여부
            row.m_hasChange = target != current;  // 목표가 현재와 다르면 강조
        }

        // 마지막 행: 완갑 재련 (주스탯이 장비 재련과 함께 계산됨, 상급 재련 없음)
        CombatGearRow& vambrace{ rows.emplace_back() };  // 완갑 행 추가
        vambrace.m_partName = std::wstring{ VAMBRACE_PART_TEXT };  // 부위 이름 글자
        vambrace.m_itemName = std::wstring{ EMPTY_TEXT };  // 아이템 이름 기본값은 빈 칸 글자
        vambrace.m_currentEnhancement = vambrace.m_currentAdvanced = vambrace.m_targetEnhancement = vambrace.m_targetAdvanced = std::wstring{ EMPTY_TEXT };  // 현재 · 목표 재련 칸 기본값은 빈 칸 글자
        if (m_current.m_extra.m_vambraceLevel.has_value() && m_target.m_extra.m_vambraceLevel.has_value())  // 현재와 목표에 완갑이 모두 있으면
        {
            const std::string_view currentGrade{ GetVambraceGradeName(m_current.m_extra) };  // 현재 완갑 등급 이름
            const std::string_view targetGrade{ GetVambraceGradeName(m_target.m_extra) };  // 목표 완갑 등급 이름
            vambrace.m_itemName = currentGrade == targetGrade  // 아이템 이름 글자 ("완갑 (유물)" 또는 "완갑 (유물 → 고대)")
                ? std::format(VAMBRACE_ITEM_FORMAT, ToWideMessage(currentGrade))
                : std::format(VAMBRACE_ITEM_CHANGE_FORMAT, ToWideMessage(currentGrade), ToWideMessage(targetGrade));
            vambrace.m_gradeName = std::string{ currentGrade.empty() ? VAMBRACE_GRADE_NAME : currentGrade };  // 글자색용 등급 이름 (모르면 유물색)
            vambrace.m_currentEnhancement = FormatEnhancement(*m_current.m_extra.m_vambraceLevel);  // 현재 완갑 단계 글자
            vambrace.m_targetEnhancement = FormatEnhancement(*m_target.m_extra.m_vambraceLevel);  // 목표 완갑 단계 글자
            vambrace.m_isTargetEditable = true;  // 목표 편집 가능
            vambrace.m_hasChange = *m_target.m_extra.m_vambraceLevel != *m_current.m_extra.m_vambraceLevel || currentGrade != targetGrade;  // 단계나 등급이 현재와 다르면 강조
        }
        m_view->ShowGears(rows);  // 장비 표 갱신
    }

    // 악세서리 · 아크 그리드 · 팔찌/스톤/각인서 · 아크 패시브/아바타 카드 (행 글자는 편집기가 만듦)
    void CombatSimulatorManager::RefreshCards()
    {
        if (!m_view || !m_editor) return;  // 화면이나 편집기가 없으면 중단

        for (std::size_t index{}; index < static_cast<std::size_t>(CombatCard::CARD_COUNT); ++index)  // 카드마다
        {
            const CombatCard card{ static_cast<CombatCard>(index) };  // 카드 종류
            m_view->ShowCard(card, m_editor->BuildRows(card, m_current.m_extra, m_target.m_extra));  // 편집기가 만든 행으로 카드 표 갱신
        }
    }

    // 무기 품질 · 보석 상자
    void CombatSimulatorManager::RefreshOptions()
    {
        if (!m_view) return;  // 화면이 없으면 중단

        // 품질: 에스더 · 모름 · 조회 전이면 입력 잠금
        const bool hasQuality{ m_current.m_weaponQuality.has_value() && m_target.m_weaponQuality.has_value() };  // 무기 품질이 현재와 목표 모두 있는지
        const std::optional<Models::GearItem>* weapon{ m_armory.has_value() ? &m_armory->m_gears[Models::ToGearIndex(Models::GearPart::PART_WEAPON)] : nullptr };  // 무기 장비 (조회 전이면 없음)
        const bool isEsther{ weapon != nullptr && weapon->has_value() && (*weapon)->m_grade == ESTHER_GRADE_NAME };  // 에스더 무기 여부
        if (hasQuality) m_view->SetQualityText(std::format(QUALITY_FORMAT, *m_current.m_weaponQuality));  // 품질이 있으면 현재 품질 안내
        else m_view->SetQualityText(std::wstring{ isEsther ? QUALITY_ESTHER_TEXT : QUALITY_UNKNOWN_TEXT });  // 없으면 에스더 / 정보 없음 안내
        m_view->SetQualityTarget(hasQuality ? std::to_wstring(*m_target.m_weaponQuality) : std::wstring{});  // 품질 목표 칸 값 설정
        m_view->SetQualityEnabled(hasQuality);  // 품질 입력 칸 활성 여부 설정

        // 보석: "보석 11개 · 10레벨 5개 · 9레벨 6개" (높은 레벨부터)
        if (!m_armory.has_value()) m_view->SetGemText(std::wstring{ GEM_NOT_LOADED_TEXT });  // 조회 전이면 안내 글자
        else if (m_current.m_gemLevels.empty()) m_view->SetGemText(std::wstring{ NO_GEM_TEXT });  // 보석이 없으면 안내 글자
        else
        {
            std::map<int, std::size_t, std::greater<>> counts;  // 레벨별 보석 개수 (높은 레벨부터)
            for (const int level : m_current.m_gemLevels) ++counts[level];  // 보석 레벨별 개수 집계

            std::wstring text{ std::format(GEM_SUMMARY_FORMAT, m_current.m_gemLevels.size()) };  // "보석 N개" 요약 글자
            for (const auto& [level, count] : counts) text += std::format(GEM_COUNT_FORMAT, level, count);  // 레벨별 개수 글자 이어 붙임
            m_view->SetGemText(std::move(text));  // 요약 글자 반영
        }
        m_view->SetGemTargetEnabled(m_armory.has_value() && !m_current.m_gemLevels.empty());  // 보석이 있을 때만 전체 목표 선택 상자 활성
        RefreshGems();  // 보석 표 갱신
    }

    // 보석 표: 장착 순서대로 슬롯 · 종류 · 현재 · 목표 (조회 전 · 보석 없음이면 빈 표)
    void CombatSimulatorManager::RefreshGems()
    {
        if (!m_view) return;  // 화면이 없으면 중단

        std::vector<CombatGemRow> rows;  // 보석 표 행 목록
        const std::size_t count{ m_armory.has_value() ? std::min(m_current.m_gemLevels.size(), m_target.m_gemLevels.size()) : 0 };  // 현재와 목표 중 짧은 쪽 개수 (조회 전이면 0)
        rows.reserve(count);  // 개수만큼 메모리 미리 확보
        for (std::size_t index{}; index < count; ++index)  // 보석마다
        {
            const std::string_view kind{ index < m_armory->m_gemKinds.size() ? std::string_view{ m_armory->m_gemKinds[index] } : std::string_view{} };  // 보석 종류 (모르면 빈 글자)
            rows.push_back(CombatGemRow{ std::format(GEM_SLOT_FORMAT, index + 1), kind.empty() ? std::wstring{ EMPTY_TEXT } : ToWideMessage(kind),  // 슬롯 · 종류 · 현재 레벨 · 목표 레벨 · 변경 여부로 행 추가
                std::to_wstring(m_current.m_gemLevels[index]), std::to_wstring(m_target.m_gemLevels[index]),
                m_target.m_gemLevels[index] != m_current.m_gemLevels[index] });
        }
        m_view->ShowGems(rows);  // 보석 표 갱신
        m_view->SetGemTargetIndex(GetGemTargetIndex());  // 전체 목표 선택 표시 갱신
    }

    // 목표가 현재와 같으면 현재 유지, 모두 같은 레벨이면 그 레벨, 그 밖은 개별 설정
    std::size_t CombatSimulatorManager::GetGemTargetIndex() const
    {
        const std::vector<int>& targets{ m_target.m_gemLevels };  // 목표 보석 레벨 목록
        if (targets.empty() || targets == m_current.m_gemLevels) return GEM_KEEP_INDEX;  // 목표가 없거나 현재와 같으면 "현재 유지"
        if (IsGemLevelInRange(targets.front()) && std::ranges::all_of(targets, [first = targets.front()](int level) { return level == first; }))  // 모든 목표가 같은 허용 레벨이면
        {
            return ToGemIndex(targets.front());  // 해당 레벨 선택지 번호 반환
        }
        return GetGemIndividualIndex();  // 그 밖에는 "개별 설정"
    }

    // 결과 상자: 현재 · 예상 전투력, 증감, 예상 아이템 레벨, 요소별 배율
    void CombatSimulatorManager::RefreshResult()
    {
        if (!m_view) return;  // 화면이 없으면 중단

        CombatResultText text;  // 결과 글자 모음
        if (!m_armory.has_value())  // 조회 전이면
        {
            text.m_currentPower = std::format(CURRENT_POWER_FORMAT, EMPTY_TEXT);  // 현재 전투력 빈 칸 표시
            text.m_predictedPower = std::format(PREDICTED_POWER_FORMAT, EMPTY_TEXT);  // 예상 전투력 빈 칸 표시
            m_view->ShowResult(text);  // 결과 반영
            return;  // 처리 끝
        }

        const double currentPower{ m_armory->m_combatPower.value_or(0.0) };  // API 전투력 (없으면 0)
        const Core::CombatPowerResult result{ m_calculator->Calculate(currentPower, m_current, m_target) };  // 예상 전투력 계산

        if (m_armory->m_combatPower.has_value())  // API 전투력이 있으면
        {
            text.m_currentPower = std::format(CURRENT_POWER_FORMAT, Core::NumberFormat::FormatNumber(currentPower, POWER_DECIMALS));  // 현재 전투력 글자
            text.m_predictedPower = std::format(PREDICTED_POWER_FORMAT, Core::NumberFormat::FormatNumber(result.m_predictedPower, POWER_DECIMALS));  // 예상 전투력 글자
            const double rate{ (result.m_totalFactor - NO_CHANGE_FACTOR) * PERCENT_SCALE };  // 증감 비율 (%)
            text.m_change = std::format(CHANGE_FORMAT, GetSignText(result.m_delta), Core::NumberFormat::FormatNumber(result.m_delta, POWER_DECIMALS), GetSignText(rate), rate);  // 증감 글자 (부호 · 값 · 비율)
        }
        else  // API 전투력이 없으면
        {
            text.m_currentPower = std::format(CURRENT_POWER_FORMAT, NO_POWER_TEXT);  // 현재 전투력 "정보 없음" 글자
            text.m_predictedPower = std::format(PREDICTED_FACTOR_FORMAT, result.m_totalFactor);  // 예상 배율만 표시
            text.m_change = std::wstring{ NO_POWER_CHANGE_TEXT };  // 배율만 보여 준다는 안내
        }
        text.m_isUp = result.m_totalFactor > NO_CHANGE_FACTOR + CHANGE_EPSILON;  // 전체 배율이 올랐는지
        text.m_isDown = result.m_totalFactor < NO_CHANGE_FACTOR - CHANGE_EPSILON;  // 전체 배율이 내렸는지
        text.m_itemLevel = std::format(ITEM_LEVEL_FORMAT, Core::NumberFormat::FormatNumber(result.m_currentItemLevel, LEVEL_DECIMALS),  // 예상 아이템 레벨 글자
            Core::NumberFormat::FormatNumber(result.m_predictedItemLevel, LEVEL_DECIMALS));

        // 배율 내역: 1이 아닌 요소만 (모두 1이면 변화 없음)
        text.m_factors = std::wstring{ FACTOR_PREFIX };  // 배율 내역 머리글
        bool hasFactor{};  // 배율 항목이 하나라도 있는지
        for (const Core::CombatFactorResult& factor : result.m_factors)  // 요소별 배율 순회
        {
            if (std::abs(factor.m_factor - NO_CHANGE_FACTOR) <= FACTOR_DISPLAY_EPSILON) continue;  // 변화가 거의 없는 요소는 건너뜀
            if (hasFactor) text.m_factors += FACTOR_SEPARATOR;  // 이미 항목이 있으면 구분 글자 추가
            text.m_factors += std::format(FACTOR_ITEM_FORMAT, GetFactorName(factor.m_kind), factor.m_factor);  // "이름 ×배율" 항목 추가
            hasFactor = true;  // 항목 있음 표시
        }
        if (!hasFactor) text.m_factors += NO_FACTOR_CHANGE_TEXT;  // 항목이 없으면 "변화 없음" 표시

        // 오차 범위: 바뀐 계통의 오차를 더한 값 (하나도 안 바뀌었으면 숨김)
        const double errorPercent{ EstimateErrorPercent(result, m_current, m_target) };  // 예상 오차 (%)
        if (errorPercent > 0.0)  // 오차가 있을 때만 표시
        {
            text.m_factors += std::format(ERROR_RANGE_FORMAT, errorPercent);  // "오차 범위 약 ±N%" 덧붙임
            if (m_armory->m_combatPower.has_value())  // API 전투력이 있으면 전투력 단위 오차도 표시
            {
                text.m_factors += std::format(ERROR_RANGE_POWER_FORMAT, Core::NumberFormat::FormatNumber(currentPower * errorPercent / PERCENT_SCALE, POWER_DECIMALS));  // "(±N)" 덧붙임
            }
        }

        // 모르는 혼돈 코어 이름을 평균값으로 계산했으면 안내 (현재 또는 목표에 있을 때만)
        if (Core::HasUnknownChaosCore(m_current.m_extra) || Core::HasUnknownChaosCore(m_target.m_extra)) text.m_note = std::wstring{ CHAOS_FALLBACK_NOTE };  // 평균 대체 안내 문구
        m_view->ShowResult(text);  // 결과 반영
    }

    // 목표 → 저장 설정 (계수 표가 있는 부위만)
    void CombatSimulatorManager::StoreTargets()
    {
        Services::CombatSimulatorSettings& settings{ m_store->GetSettings() };  // 저장 설정
        for (std::size_t index{}; index < Models::GEAR_PART_COUNT; ++index)  // 부위마다
        {
            const Models::GearState& gear{ m_target.m_gears[index] };  // 목표 부위 상태
            settings.m_gearTargets[index] = gear.IsKnown()  // 표가 있는 부위만 목표 기록, 없으면 비움
                ? std::optional<Services::CombatGearTarget>{ Services::CombatGearTarget{ gear.m_enhancement, gear.m_advancedEnhancement } } : std::nullopt;
        }
        settings.m_weaponQualityTarget = m_target.m_weaponQuality;  // 무기 품질 목표 기록
        settings.m_gemTargets = m_target.m_gemLevels != m_current.m_gemLevels ? m_target.m_gemLevels : std::vector<int>{};   // 현재와 같으면 저장 안 함
        settings.m_extraTarget = m_target.m_extra != m_current.m_extra ? std::optional<Models::ExtraState>{ m_target.m_extra } : std::nullopt;   // 현재와 같으면 저장 안 함
    }

    // 전투력 시뮬레이터 설정(마지막 캐릭터 · 목표값)을 사용자 데이터 폴더에 저장
    void CombatSimulatorManager::SaveData()
    {
        try  // 저장 시도
        {
            m_store->Save();  // 파일에 저장
        }
        catch (const std::exception& error)  // 저장 실패 시
        {
            m_context->m_notifier.ShowMessage(std::wstring{ SAVE_ERROR_TITLE }, std::wstring{ SAVE_ERROR_TEXT } + ToWideMessage(error.what()),  // 저장 오류 알림 표시
                Core::NoticeKind::NOTICE_ERROR, nullptr);
        }
    }
}
