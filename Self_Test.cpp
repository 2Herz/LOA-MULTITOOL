module;

#define NOMINMAX
#include <Windows.h>									// GetModuleFileNameW, GetFileVersionInfoW, VerQueryValueW

#pragma comment(lib, "version.lib")					// 버전 정보 API
#pragma comment(lib, "gdi32.lib")					// 숫자 읽기 검사용 글자 그리기 (GDI)

module LostArkMultiTool.Core.SelfTest;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.CombatRegression;
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.RefineData;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.TransferData;
import LostArkMultiTool.Services.ArmoryParser;
import LostArkMultiTool.Services.JSONValue;
import LostArkMultiTool.Services.TransferCalculator;
import LostArkMultiTool.Vision.DigitReader;
import LostArkMultiTool.Vision.FrameImage;
import LostArkMultiTool.Vision.ScreenCaptureService;
import LostArkMultiTool.Vision.ScreenLayout;
import LostArkMultiTool.Vision.ScreenWatcher;
import LostArkMultiTool.Vision.TemplateMatcher;
import LostArkMultiTool.Vision.TextRecognizer;

namespace LostArkMultiTool::Core::SelfTest
{
    namespace
    {
        namespace Coefficients = Models::CombatCoefficientData;   // 전투력 계수 표
        namespace Refine = Models::RefineData;                    // 재련 표

        constexpr std::wstring_view SELF_TEST_ARGUMENT{ L"--self-test" };   // 명령줄 인자
        constexpr double SHARE_SUM_TOLERANCE{ 1e-4 };                        // 트리별 랭크 몫 합 허용 오차 (소수 5자리 반올림)
        constexpr double EXPECTED_SHARE_SUM{ 1.0 };                          // 몫 합은 1
        constexpr double TOP_GEM_FACTOR{ 1.0 };                              // 10레벨 보석 계수 = 1 (기준)
        constexpr double GEM_FACTOR_TOLERANCE{ 1e-9 };                       // 계수 비교 허용 오차
        constexpr std::size_t PASSIVE_RANK_COUNT{ 7 };                       // 랭크 0 ~ 6
        constexpr double MIN_PROBABILITY{ 0.0 };                             // 확률 하한 (초과해야 함)
        constexpr double MAX_PROBABILITY{ 1.0 };                             // 확률 상한
        constexpr double MAX_UNCERTAINTY_PERCENT{ 5.0 };                     // 계통 오차 추정이 이보다 크면 이상한 값
        constexpr std::string_view PASS_TEXT{ "PASS" };                      // 통과 표시
        constexpr std::string_view FAIL_TEXT{ "FAIL" };                      // 실패 표시
        constexpr std::string_view NOTE_TEXT{ "NOTE" };                      // 안내 표시 (실패 수에 안 셈)
        constexpr std::size_t MAX_REPORTED_FAILURES{ 20 };                   // 보고서에 적을 회귀 초과 줄 수 (오차 큰 순)
        constexpr std::size_t EXE_PATH_BUFFER_SIZE{ 32767 };                 // 윈도우가 지원하는 가장 긴 경로 길이
        constexpr std::wstring_view PRODUCT_VERSION_KEY{ L"\\StringFileInfo\\041204b0\\ProductVersion" };   // App_Version.rc 한국어 · 유니코드 블록의 제품 버전

        // 검사 결과를 모아 보고서로 만드는 도구
        class Reporter
        {
        public:
            // 한 항목 결과 기록 (실패면 이유도 함께)
            void Check(bool isPassed, std::string_view name, std::string_view detail = {})
            {
                if (!isPassed) ++m_failCount;                                                    // 실패 수
                m_lines += std::format("[{}] {}{}{}\n", isPassed ? PASS_TEXT : FAIL_TEXT, name,
                    detail.empty() ? "" : " : ", detail);                                         // 한 줄 기록
            }

            // 안내 한 줄 기록 (통과 · 실패에 영향 없음)
            void Note(std::string_view text)
            {
                m_lines += std::format("[{}] {}\n", NOTE_TEXT, text);   // 한 줄 기록
            }

            [[nodiscard]] int GetFailCount() const noexcept { return m_failCount; }   // 실패 항목 수
            [[nodiscard]] const std::string& GetText() const noexcept { return m_lines; }   // 보고서 본문

        private:
            int m_failCount{};     // 실패 수
            std::string m_lines;   // 누적 보고서
        };

        // 유한한 수인지 (NaN · 무한대 걸러냄)
        [[nodiscard]] bool IsFiniteValue(double value) noexcept
        {
            return std::isfinite(value);   // 표준 검사
        }

        // 키 · 값 표가 비어 있지 않고 모든 값이 유한한지
        void CheckKeyedTable(Reporter& reporter, std::string_view name, std::span<const Coefficients::KeyedValue> table)
        {
            const bool isFinite{ std::ranges::all_of(table, [](const Coefficients::KeyedValue& row) { return IsFiniteValue(row.m_value); }) };   // 값 검사
            const bool hasEmptyKey{ std::ranges::any_of(table, [](const Coefficients::KeyedValue& row) { return row.m_key.empty(); }) };       // 빈 키
            reporter.Check(!table.empty() && isFinite && !hasEmptyKey, name, std::format("{}줄", table.size()));   // 결과
        }

        // ===== 전투력 계수 =====
        void CheckCombatCoefficients(Reporter& reporter)
        {
            // 아크 패시브 트리별 랭크 몫 합 = 1
            const std::span<const double> shares{ Coefficients::GetPassiveRankTreeShares() };                    // 진화 · 깨달음 · 도약
            const double shareSum{ std::accumulate(shares.begin(), shares.end(), 0.0) };                        // 합
            reporter.Check(std::abs(shareSum - EXPECTED_SHARE_SUM) < SHARE_SUM_TOLERANCE, "아크 패시브 트리 몫 합 = 1", std::format("{:.6f}", shareSum));

            // 랭크 배율: 7칸 · 양수 · 랭크가 오를수록 줄지 않음
            const std::span<const double> rankRatios{ Coefficients::GetPassiveRankRatios() };                   // 랭크 0 ~ 6
            const bool isRankPositive{ std::ranges::all_of(rankRatios, [](double ratio) { return IsFiniteValue(ratio) && ratio > 0.0; }) };   // 양수
            const bool isRankSorted{ std::ranges::is_sorted(rankRatios) };                                      // 단조 증가
            reporter.Check(rankRatios.size() == PASSIVE_RANK_COUNT && isRankPositive && isRankSorted, "아크 패시브 랭크 배율 (7칸 · 양수 · 단조)");

            // 보석 계수: 레벨이 오를수록 커지고 마지막(10레벨) = 1
            const std::span<const double> gemFactors{ Coefficients::GetGemFactors() };                          // 첫 레벨부터
            const bool isGemSorted{ std::ranges::is_sorted(gemFactors) };                                       // 단조 증가
            const bool isGemTopOne{ !gemFactors.empty() && std::abs(gemFactors.back() - TOP_GEM_FACTOR) < GEM_FACTOR_TOLERANCE };   // 기준
            reporter.Check(isGemSorted && isGemTopOne, "보석 레벨 계수 (단조 · 10레벨 = 1)");
            reporter.Check(Coefficients::GetNoGemValue() <= gemFactors.front(), "보석 없음 계수 ≤ 첫 레벨 계수");

            // 장비 · 완갑 · 아크 코어 표가 비어 있지 않음
            reporter.Check(!Coefficients::GetEquipmentTables().empty(), "장비 능력치 표", std::format("{}개", Coefficients::GetEquipmentTables().size()));
            reporter.Check(!Coefficients::GetVambraceGrades().empty(), "완갑 등급 표", std::format("{}개", Coefficients::GetVambraceGrades().size()));
            reporter.Check(!Coefficients::GetArkCorePoints().empty(), "아크 코어 포인트 표", std::format("{}줄", Coefficients::GetArkCorePoints().size()));
            reporter.Check(!Coefficients::GetEngravingBooks().empty(), "각인서 효과 표", std::format("{}개", Coefficients::GetEngravingBooks().size()));

            // 키 · 값 표 (빈 키 · NaN 없음)
            CheckKeyedTable(reporter, "악세 연마 옵션", Coefficients::GetAccessoryOptions());
            CheckKeyedTable(reporter, "팔찌 옵션", Coefficients::GetBraceletOptions());
            CheckKeyedTable(reporter, "스톤 레벨 1 효과", Coefficients::GetStoneLevelOne());
            CheckKeyedTable(reporter, "아크 그리드 젬 레벨당 효과", Coefficients::GetArkGemPerLevel());
            CheckKeyedTable(reporter, "아바타 등급별 주스탯 %", Coefficients::GetAvatarPercent());
            CheckKeyedTable(reporter, "깨달음 포인트 배율", Coefficients::GetEnlightenmentRatios());
            CheckKeyedTable(reporter, "계통별 예상 오차", Coefficients::GetUncertainties());

            // 예상 오차는 0 이상 · 상식적인 크기
            const bool isUncertaintyRange{ std::ranges::all_of(Coefficients::GetUncertainties(),
                [](const Coefficients::KeyedValue& row) { return row.m_value >= 0.0 && row.m_value <= MAX_UNCERTAINTY_PERCENT; }) };   // 0 ~ 5%
            reporter.Check(isUncertaintyRange, "계통별 예상 오차 범위 (0 ~ 5%)");

            // 계수 측정 날짜가 적혀 있음 (앱 안내 문구에 표시)
            reporter.Check(!Coefficients::GetCoefficientMeasuredAt().empty(), "계수 측정 날짜 기록");
        }

        // ===== 재련 표 =====
        // 일반 재련 한 단계가 상식 범위인지 (확률 0 초과 1 이하 · 재료 수량 0 이상 · 보조 재료 확률 0 ~ 1)
        [[nodiscard]] bool IsValidNormalStep(const Refine::NormalStep& step) noexcept
        {
            if (!(step.m_baseProbability > MIN_PROBABILITY && step.m_baseProbability <= MAX_PROBABILITY)) return false;   // 기본 확률
            const bool isAmountValid{ std::ranges::all_of(step.m_materials,
                [](const Refine::MaterialAmount& amount) { return IsFiniteValue(amount.m_amount) && amount.m_amount >= 0.0; }) };   // 재료 수량
            const auto isSupportValid{ [](const std::optional<Refine::SupportMaterial>& support)
            {
                return !support.has_value() || (support->m_probability >= MIN_PROBABILITY && support->m_probability <= MAX_PROBABILITY && support->m_maxCount >= 0);   // 숨결 · 책
            } };
            return isAmountValid && isSupportValid(step.m_breath) && isSupportValid(step.m_book);   // 모두 통과
        }

        void CheckRefineTables(Reporter& reporter)
        {
            constexpr std::array TIERS{ Refine::EquipmentTier::TIER_AEGIR, Refine::EquipmentTier::TIER_SERCA };   // 에기르 · 세르카
            constexpr std::array KINDS{ Refine::EquipmentKind::KIND_ARMOR, Refine::EquipmentKind::KIND_WEAPON };   // 방어구 · 무기

            int stepCount{};          // 확인한 단계 수
            int invalidCount{};       // 이상한 단계 수
            for (const Refine::EquipmentTier tier : TIERS)
            {
                const int firstLevel{ tier == Refine::EquipmentTier::TIER_AEGIR ? Refine::MIN_AEGIR_ENHANCEMENT : Refine::MIN_SERCA_ENHANCEMENT };   // 표 시작
                for (const Refine::EquipmentKind kind : KINDS)
                {
                    for (int level{ firstLevel }; level < Refine::MAX_ENHANCEMENT; ++level)
                    {
                        const std::optional<Refine::NormalStep> step{ Refine::FindNormalStep(tier, kind, level) };   // level → level + 1
                        if (!step.has_value()) continue;                                                            // 표에 없는 단계
                        ++stepCount;                                                                                // 확인 수
                        if (!IsValidNormalStep(*step)) ++invalidCount;                                              // 이상한 값
                    }
                }
            }
            reporter.Check(stepCount > 0 && invalidCount == 0, "일반 재련 단계 (확률 · 재료 수량)", std::format("{}단계 중 이상 {}개", stepCount, invalidCount));

            int blockCount{};         // 확인한 상급 재련 묶음 수
            int invalidBlockCount{};  // 이상한 묶음 수
            for (const Refine::EquipmentKind kind : KINDS)
            {
                for (int blockIndex{}; blockIndex < Refine::ADVANCED_BLOCK_COUNT; ++blockIndex)
                {
                    const std::optional<Refine::AdvancedBlock> block{ Refine::FindAdvancedBlock(kind, blockIndex) };   // 10단계 묶음
                    if (!block.has_value()) continue;                                                                // 표에 없음
                    ++blockCount;                                                                                    // 확인 수
                    const bool isTryValid{ !block->m_tryRows.empty() && std::ranges::all_of(block->m_tryRows, [](const Refine::AdvancedTryRow& row)
                    {
                        return row.m_paidNormalTry >= 0.0 && row.m_freeNormalTry >= 0.0 && row.m_bonusTry >= 0.0 && row.m_enhancedBonusTry >= 0.0;   // 기대 시도 수 0 이상
                    }) };
                    if (!isTryValid) ++invalidBlockCount;                                                            // 이상한 묶음
                }
            }
            reporter.Check(blockCount > 0 && invalidBlockCount == 0, "상급 재련 묶음 (기대 시도 수)", std::format("{}묶음 중 이상 {}개", blockCount, invalidBlockCount));
        }
        // 실행 파일 버전 정보(App_Version.rc)의 제품 버전이 APP_VERSION과 같은지 (버전을 올릴 때 rc를 빠뜨리면 실패)
        void CheckVersionResource(Reporter& reporter)
        {
            std::wstring exePath(EXE_PATH_BUFFER_SIZE, L'\0');                                                   // 실행 파일 경로 버퍼
            const DWORD pathLength{ ::GetModuleFileNameW(nullptr, exePath.data(), static_cast<DWORD>(exePath.size())) };   // 경로 읽기
            exePath.resize(pathLength);                                                                          // 실제 길이로 자름

            DWORD handle{};                                                                                      // 쓰지 않는 값 (API 형식)
            const DWORD infoSize{ ::GetFileVersionInfoSizeW(exePath.c_str(), &handle) };                         // 버전 정보 크기
            std::vector<std::byte> info(infoSize);                                                               // 버전 정보 버퍼
            const bool hasInfo{ infoSize > 0 && ::GetFileVersionInfoW(exePath.c_str(), 0, infoSize, info.data()) != FALSE };   // 읽기

            wchar_t* text{ nullptr };                                                                            // 버퍼 안을 가리키기만 함 (해제 불필요)
            UINT textLength{};                                                                                   // 글자 수 (끝 0 포함)
            const bool hasVersion{ hasInfo && ::VerQueryValueW(info.data(), PRODUCT_VERSION_KEY.data(), reinterpret_cast<void**>(&text), &textLength) != FALSE
                && text != nullptr && textLength > 0 };                                                          // 제품 버전 찾기
            const std::wstring resourceVersion{ hasVersion ? std::wstring{ text } : std::wstring{} };            // 리소스 버전 글자
            const std::wstring appVersion{ APPConstants::APP_VERSION.begin(), APPConstants::APP_VERSION.end() }; // 코드 버전 (ASCII)

            const std::string resourceText{ resourceVersion.begin(), resourceVersion.end() };                    // 보고서용 (버전은 ASCII)
            reporter.Check(resourceVersion == appVersion, "실행 파일 버전 정보",
                std::format("rc {} / APP_VERSION {}", hasVersion ? resourceText : std::string{ "없음" }, APPConstants::APP_VERSION));   // 비교 결과
        }


        // ===== 2부 이관 규칙 검사용 방송 예시 =====
        constexpr int TRANSFER_EXAMPLE_LEVEL{ 1720 };          // 방송 예시 장비 레벨 (→ 희귀 4연마 1720)
        constexpr int TRANSFER_EXAMPLE_POLISH{ 4 };            // 방송 예시 연마 단계
        constexpr std::string_view TRANSFER_EXAMPLE_GRADE{ "희귀" };   // 방송 예시 등급
        constexpr int TRANSFER_TOP_SOURCE_LEVEL{ 1800 };       // 25강 무기 레벨
        constexpr int TRANSFER_TOP_TARGET_LEVEL{ 1815 };       // 25강 무기 이관 레벨
        constexpr int ESTHER_EXAMPLE_LEVEL{ 1830 };            // 10단계 엘라3 에스더 무기 레벨
        constexpr int ESTHER_EXAMPLE_STAGE{ 4 };               // 이관 엘라 단계
        constexpr int ESTHER_EXAMPLE_POLISH{ 5 };              // 이관 연마 단계
        constexpr int ESTHER_EXAMPLE_TARGET_LEVEL{ 1865 };     // 이관 레벨
        constexpr int ESTHER_REFINE_EVOLUTION{ 8 };            // 공지 예시: 8진화
        constexpr int ESTHER_REFINE_ELA{ 2 };                  // 공지 예시: 엘라 2단계
        constexpr int ESTHER_REFINE_LEVEL{ 1715 };             // 8진화 엘라2 레벨
        constexpr int ESTHER_REFINE_STAGE{ 18 };               // → 무기 슬롯 재련 18단계
        constexpr int ESTHER_REFINE_ENERGY{ 50 };              // → 환급 에스더의 기운 50
        constexpr int ESTHER_REFINE_KIT{ 17000 };              // → 환급 로맨틱 재료 키트 17,000
        constexpr int STONE_EXAMPLE_HIGH{ 3 };                 // 9/7돌 높은 쪽 활성화 포인트
        constexpr int STONE_EXAMPLE_LOW{ 2 };                  // 9/7돌 낮은 쪽 활성화 포인트
        constexpr int STONE_FULL_POINT{ 4 };                   // 4+4 활성화 포인트

        // ===== 2부 이관 규칙 (방송 슬라이드 예시와 같은 답이 나오는지) =====
        void CheckTransferRules(Reporter& reporter)
        {
            namespace Transfer = Models::TransferData;   // 이관 표
            using Calculator = Services::TransferCalculator;   // 이관 계산기

            const std::span<const Transfer::GearTransferRow> gearTable{ Transfer::GetGearTable() };   // 장비 이관 표
            const bool isGearSorted{ std::ranges::is_sorted(gearTable, {}, &Transfer::GearTransferRow::m_maxLevel) };   // 레벨 오름차순
            const std::optional<Transfer::GearTransferRow> example{ Calculator::FindGearRow(TRANSFER_EXAMPLE_LEVEL) };   // 방송 예시: 1720 → 희귀 4연마 1720
            const std::optional<Transfer::GearTransferRow> top{ Calculator::FindGearRow(TRANSFER_TOP_SOURCE_LEVEL) };       // 25강 무기: 1800 → 고대 3연마 1815
            const bool isExample{ example.has_value() && example->m_grade == TRANSFER_EXAMPLE_GRADE && example->m_polish == TRANSFER_EXAMPLE_POLISH && example->m_level == TRANSFER_EXAMPLE_LEVEL };   // 예시 일치
            const bool isTop{ top.has_value() && top->m_level == TRANSFER_TOP_TARGET_LEVEL };   // 최고 구간 일치
            reporter.Check(isGearSorted && isExample && isTop && !Calculator::FindGearRow(Transfer::GEAR_MIN_EXCLUSIVE_LEVEL).has_value(),
                "2부 장비 이관 표 (1720 → 희귀 4연마 · 1800 → 1815 · 1600 이하 없음)", std::format("{}줄", gearTable.size()));

            const std::optional<Transfer::EstherTransferRow> esther{ Calculator::FindEstherRow(ESTHER_EXAMPLE_LEVEL) };   // 10단계 엘라3
            reporter.Check(esther.has_value() && esther->m_elaStage == ESTHER_EXAMPLE_STAGE && esther->m_polish == ESTHER_EXAMPLE_POLISH && esther->m_level == ESTHER_EXAMPLE_TARGET_LEVEL, "2부 에스더 이관 (1830 → 엘라 4단계 5연마 1865)");

            const std::optional<Transfer::EstherTransferRow> estherRefine{ Calculator::FindEstherRow(ESTHER_REFINE_EVOLUTION, ESTHER_REFINE_ELA, ESTHER_REFINE_LEVEL) };   // 8진화 엘라2
            reporter.Check(estherRefine.has_value() && estherRefine->m_slotRefine == ESTHER_REFINE_STAGE && estherRefine->m_refundEnergy == ESTHER_REFINE_ENERGY && estherRefine->m_refundKit == ESTHER_REFINE_KIT,
                "2부 에스더 재련 · 환급 (8진화 엘라2 → 재련 18 · 기운 50 · 키트 17,000)");

            const auto supporterRules{ Transfer::GetSupporterAccessoryRules() };   // 서포터 장신구 규칙
            const bool isSupporterFiller{ std::ranges::all_of(supporterRules, [](const Transfer::AccessoryRule& rule)
            {
                return rule.m_partType == Transfer::SUPPORTER_EARRING_TYPE || (std::ranges::contains(rule.m_fillers, std::string_view{ "최대생명력" }) && !std::ranges::contains(rule.m_fillers, std::string_view{ "공격력+" }));   // 목걸이 · 반지 = 무공+ · 최생
            }) };
            reporter.Check(isSupporterFiller && Transfer::IsDealerEngraving("각성") && Transfer::IsSupporterEngraving("각성") && !Transfer::IsSupporterEngraving("원한") && Transfer::IsSupporterClass("바드"),
                "2부 서포터 3옵션 · 유효 각인 · 서포터 직업 (11/25 공지)");

            std::string key;   // 연마 표 키
            const bool isHigh{ Calculator::RankPolishLine("적에게 주는 피해 +2.00%", key) == Transfer::PolishRank::RANK_HIGH && key == "적에게주는피해" };   // 상
            const bool isMiddle{ Calculator::RankPolishLine("공격력 +195", key) == Transfer::PolishRank::RANK_MIDDLE && key == "공격력+" };                 // 중 (고정값)
            const bool isLow{ Calculator::RankPolishLine("무기 공격력 +0.80%", key) == Transfer::PolishRank::RANK_LOW && key == "무기공격력%" };          // 하 (퍼센트)
            reporter.Check(isHigh && isMiddle && isLow, "2부 장신구 연마 상 · 중 · 하 판정");

            reporter.Check(Calculator::FindStoneResult(STONE_EXAMPLE_HIGH, STONE_EXAMPLE_LOW) == "슬롯 2개 7단계" && Calculator::FindStoneResult(STONE_FULL_POINT, STONE_FULL_POINT) == "슬롯 5개 7단계 +@", "2부 스톤 이관 표 (9/7돌 → 슬롯 2개 7단계)");
        }

        // ===== 2부 이관 계산 (저장된 무기고 응답 전부, 개발 PC에만 있음) =====
        void CheckTransferArmories(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            const std::optional<std::filesystem::path> dataDirectory{ CombatRegression::FindDataDirectory(reportPath.parent_path()) };   // Verify/lopec_results
            const std::filesystem::path armoryDirectory{ dataDirectory.value_or(std::filesystem::path{}) / "regression" / "armory" };   // 무기고 폴더
            if (!dataDirectory.has_value() || !std::filesystem::is_directory(armoryDirectory))
            {
                reporter.Check(true, "2부 이관 계산 (건너뜀)", "저장된 무기고 응답 없음");   // 데이터 없는 PC
                return;
            }

            const Services::TransferCalculator calculator;   // 이관 계산기
            std::size_t characterCount{};   // 계산한 캐릭터 수
            std::size_t missingSlotCount{}; // 1600 초과인데 이관 장비를 못 찾은 슬롯 수
            std::size_t estherCount{};      // 에스더 무기 캐릭터 수
            std::size_t scrollCount{};      // 주문서 대상 장신구 수
            std::size_t stoneCount{};       // 스톤 표 안에 든 캐릭터 수
            std::set<std::string> unknownBracelet;   // 티어를 못 정한 팔찌 옵션 (체력 · 제압 등 점수 없는 옵션)
            std::map<int, std::size_t> gradeCounts;  // 주문서 등급별 장신구 수
            for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator{ armoryDirectory })   // 무기고 파일마다
            {
                if (entry.path().extension() != ".json") continue;   // JSON만
                std::ifstream file{ entry.path(), std::ios::binary };   // 파일
                const std::string text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };   // 내용
                const Models::CharacterArmory armory{ Services::ParseArmory(Services::ParseJSONText(text)) };   // 실제 앱 해석기
                const Services::TransferResult result{ calculator.Calculate(armory) };   // 이관 계산
                ++characterCount;   // 캐릭터 수
                for (const Services::TransferSlotResult& slot : result.m_armorSlots)   // 장비 쪽 슬롯
                {
                    if (slot.m_sourceLevel > Models::TransferData::GEAR_MIN_EXCLUSIVE_LEVEL && slot.GetTargetLevel() <= 0) ++missingSlotCount;   // 표가 비었음
                }
                if (result.m_isEsther) ++estherCount;   // 에스더
                for (const Services::ScrollResult& scroll : result.m_scrolls)   // 장신구마다
                {
                    if (!scroll.m_scrollGrade.has_value()) continue;   // 대상 아님
                    ++scrollCount;                                     // 주문서 대상
                    ++gradeCounts[*scroll.m_scrollGrade];              // 등급별 개수
                }
                if (!result.m_stone.m_result.empty()) ++stoneCount;   // 스톤 표 안
                for (const Services::BraceletLineResult& line : result.m_braceletLines)   // 팔찌 줄
                {
                    if (line.m_label.empty()) unknownBracelet.insert(line.m_text);   // 분류 못 한 옵션
                }
            }
            std::string gradeText;   // "1등급 12 · 2등급 30 ..."
            for (const auto& [grade, count] : gradeCounts) gradeText += std::format("{}{}등급 {}", gradeText.empty() ? "" : " · ", grade, count);   // 등급 분포
            reporter.Note(std::format("2부 장신구 주문서 등급 분포 : {}", gradeText));   // 안내
            reporter.Check(characterCount > 0 && missingSlotCount == 0, "2부 이관 계산 · 저장된 무기고",
                std::format("{}명 · 에스더 {}명 · 주문서 대상 장신구 {}개 · 스톤 표 안 {}명 · 빈 슬롯 {}개 · 분류 못 한 팔찌 옵션 {}종",
                    characterCount, estherCount, scrollCount, stoneCount, missingSlotCount, unknownBracelet.size()));   // 요약
        }

        // ===== lopec 회귀 (실제 엔진 예측 vs lopec 측정, 측정 데이터가 없는 PC면 건너뜀) =====
        void CheckLopecRegression(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            const std::optional<std::filesystem::path> dataDirectory{ CombatRegression::FindDataDirectory(reportPath.parent_path()) };   // 실행 파일 위쪽의 Verify/lopec_results
            if (!dataDirectory.has_value())
            {
                reporter.Check(true, "lopec 회귀 (건너뜀)", "Verify/lopec_results 없음 (측정 데이터는 개발 PC에만 있음)");   // 데이터 없는 PC
                return;
            }

            const CombatRegression::RegressionResult result{ CombatRegression::Run(*dataDirectory) };   // 다시 계산
            if (result.m_isSkipped)
            {
                reporter.Check(true, "lopec 회귀 (건너뜀)", result.m_skipReason);   // 무기고를 아직 안 받음 등
                return;
            }
            for (const std::string& note : result.m_notes) reporter.Note(note);   // 건너뛴 캐릭터 안내
            reporter.Check(result.m_failures.empty(), "lopec 회귀 · 아크 패시브", std::format("{}명 (서포터 {}명) · 6랭크 {}줄 최대 {:.3f}% · 덜 찬 랭크 {}줄 최대 {:.3f}% · 슬라이더 기준 보정 {}명 · 조합 기준 보정 {}명 · 비정상 제외 {}명 · 제외 {}줄 · 초과 {}줄",
                result.m_characterCount, result.m_supporterCount, result.m_fullRankRowCount, result.m_worstFullRankPercent, result.m_partialRankRowCount, result.m_worstPartialRankPercent,
                result.m_offsetCharacterCount, result.m_comboOffsetCharacterCount, result.m_abnormalCount, result.m_exceptionCount, result.m_failures.size()));   // 요약 한 줄
            for (const CombatRegression::RegressionFailure& failure : result.m_failures | std::views::take(MAX_REPORTED_FAILURES))   // 오차 큰 줄부터
            {
                reporter.Note(std::format("초과 {} {} ({}) : {:.3f}% (허용 {}%)", failure.m_character, failure.m_control, failure.m_change, failure.m_errorPercent, failure.m_limitPercent));
            }
        }
    }

    namespace  // 화면 인식 검사 전용 상수 · 도우미
    {
        constexpr int VISION_CANVAS_WIDTH{ 320 };          // 글자를 그릴 캔버스 가로
        constexpr int VISION_CANVAS_HEIGHT{ 48 };          // 글자를 그릴 캔버스 세로
        constexpr int VISION_FONT_HEIGHT{ 28 };            // 글자 크기 (픽셀)
        constexpr int VISION_TEXT_MARGIN{ 6 };             // 글자 시작 여백
        constexpr COLORREF VISION_TEXT_COLOR{ RGB(255, 255, 255) };   // 흰 글자 (게임 수량 숫자처럼 밝은 글자)
        constexpr std::wstring_view VISION_FONT_NAME{ L"Arial" };     // 검사용 글꼴
        constexpr std::wstring_view TRAINING_LINE{ L"0123456789,+" }; // 템플릿을 만들 줄
        constexpr std::string_view TRAINING_SYMBOLS{ "0123456789,+" };// 그 줄의 답
        constexpr double MATCH_PASS_SCORE{ 0.99 };         // 같은 그림을 찾았을 때 점수 하한
        constexpr int SMALL_PATTERN_SIZE{ 12 };            // 작은 템플릿 크기 (바로 훑기)
        constexpr int LARGE_PATTERN_SIZE{ 40 };            // 큰 템플릿 크기 (축소 단계 사용)
        constexpr int SCENE_WIDTH{ 400 };                  // 검사 장면 가로
        constexpr int SCENE_HEIGHT{ 300 };                 // 검사 장면 세로
        constexpr int SMALL_PATTERN_X{ 37 };               // 작은 템플릿을 둔 X
        constexpr int SMALL_PATTERN_Y{ 21 };               // 작은 템플릿을 둔 Y
        constexpr int LARGE_PATTERN_X{ 251 };              // 큰 템플릿을 둔 X
        constexpr int LARGE_PATTERN_Y{ 173 };              // 큰 템플릿을 둔 Y
        constexpr int NOISE_MODULUS{ 251 };                // 바탕 무늬를 만들 나머지 값 (소수)
        constexpr int NOISE_X_FACTOR{ 31 };                // 바탕 무늬 가로 계수
        constexpr int NOISE_Y_FACTOR{ 17 };                // 바탕 무늬 세로 계수
        constexpr std::int64_t EXPECTED_SHARD_VALUE{ 12345 };   // "12,345" 기대값
        constexpr std::int64_t EXPECTED_OVERFLOW_VALUE{ 9999 }; // "9999+" 기대값
        constexpr int SMALL_CHECKER_CELL{ 3 };             // 작은 체크무늬 칸 크기
        constexpr int LARGE_CHECKER_CELL{ 5 };             // 큰 체크무늬 칸 크기
        constexpr int SAMPLE_SCREEN_WIDTH{ 1920 };         // 비율 변환 검사 화면 가로
        constexpr int SAMPLE_SCREEN_HEIGHT{ 1080 };        // 비율 변환 검사 화면 세로
        constexpr Vision::PixelRect EXPECTED_REGION_PIXELS{ 480, 540, 96, 22 };   // (0.25, 0.5, 0.05, 0.02) → 1920x1080 픽셀

        // GDI로 흰 글자를 검은 바탕에 그려 BGRA 프레임으로 (안티앨리어싱 없이 → 게임 비트맵 글꼴과 비슷)
        [[nodiscard]] Vision::FrameImage RenderText(std::wstring_view text)
        {
            BITMAPINFO info{};                                                // 비트맵 설명
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);                 // 머리말 크기
            info.bmiHeader.biWidth = VISION_CANVAS_WIDTH;                     // 가로
            info.bmiHeader.biHeight = -VISION_CANVAS_HEIGHT;                  // 음수 = 위에서 아래로
            info.bmiHeader.biPlanes = 1;                                      // 평면 1장
            info.bmiHeader.biBitCount = 32;                                   // BGRA 32비트
            info.bmiHeader.biCompression = BI_RGB;                            // 압축 없음
            void* bits = nullptr;                                             // 픽셀 메모리
            HDC screen = ::GetDC(nullptr);                                    // 화면 DC (호환 DC를 만들 기준)
            HDC memory = ::CreateCompatibleDC(screen);                        // 메모리 DC
            ::ReleaseDC(nullptr, screen);                                     // 화면 DC는 바로 돌려줌
            HBITMAP bitmap = ::CreateDIBSection(memory, &info, DIB_RGB_COLORS, &bits, nullptr, 0);   // 픽셀 직접 접근 비트맵
            HGDIOBJ oldBitmap = ::SelectObject(memory, bitmap);               // 비트맵 연결
            HFONT font = ::CreateFontW(VISION_FONT_HEIGHT, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, DEFAULT_PITCH, VISION_FONT_NAME.data());   // 검사용 글꼴
            HGDIOBJ oldFont = ::SelectObject(memory, font);                   // 글꼴 연결
            ::SetBkColor(memory, RGB(0, 0, 0));                               // 검은 바탕
            ::SetTextColor(memory, VISION_TEXT_COLOR);                        // 흰 글자
            RECT area{ 0, 0, VISION_CANVAS_WIDTH, VISION_CANVAS_HEIGHT };     // 그릴 영역
            ::ExtTextOutW(memory, VISION_TEXT_MARGIN, VISION_TEXT_MARGIN, ETO_OPAQUE, &area,
                text.data(), static_cast<UINT>(text.size()), nullptr);        // 바탕을 채우며 글자 그리기
            ::GdiFlush();                                                     // 그리기 끝내기 (픽셀 읽기 전)

            const std::size_t byteCount = static_cast<std::size_t>(VISION_CANVAS_WIDTH) * VISION_CANVAS_HEIGHT * Vision::FrameImage::BYTES_PER_PIXEL;   // 픽셀 바이트
            const std::span<const std::uint8_t> source{ static_cast<const std::uint8_t*>(bits), byteCount };   // 경계가 있는 span
            Vision::FrameImage frame = Vision::FrameImage::FromRows(VISION_CANVAS_WIDTH, VISION_CANVAS_HEIGHT, source,
                VISION_CANVAS_WIDTH * Vision::FrameImage::BYTES_PER_PIXEL);   // 프레임으로 복사

            ::SelectObject(memory, oldFont);      // 글꼴 원래대로
            ::DeleteObject(font);                 // 글꼴 해제
            ::SelectObject(memory, oldBitmap);    // 비트맵 원래대로
            ::DeleteObject(bitmap);               // 비트맵 해제
            ::DeleteDC(memory);                   // 메모리 DC 해제
            return frame;                         // 그린 프레임
        }

        // 무늬가 있는 바탕에 템플릿을 박은 장면 만들기
        [[nodiscard]] Vision::GrayImage MakeScene(const Vision::GrayImage& smallPattern, const Vision::GrayImage& largePattern)
        {
            Vision::GrayImage scene{ SCENE_WIDTH, SCENE_HEIGHT };                     // 장면
            for (int y = 0; y < SCENE_HEIGHT; ++y)                                    // 줄마다
                for (int x = 0; x < SCENE_WIDTH; ++x)                                 // 칸마다
                    scene.Set(x, y, static_cast<std::uint8_t>((x * NOISE_X_FACTOR + y * NOISE_Y_FACTOR) % NOISE_MODULUS));   // 반복 없는 바탕 무늬
            for (int y = 0; y < smallPattern.GetHeight(); ++y)                               // 작은 템플릿 박기
                for (int x = 0; x < smallPattern.GetWidth(); ++x)
                    scene.Set(SMALL_PATTERN_X + x, SMALL_PATTERN_Y + y, smallPattern.At(x, y));
            for (int y = 0; y < largePattern.GetHeight(); ++y)                               // 큰 템플릿 박기
                for (int x = 0; x < largePattern.GetWidth(); ++x)
                    scene.Set(LARGE_PATTERN_X + x, LARGE_PATTERN_Y + y, largePattern.At(x, y));
            return scene;                                                             // 장면
        }

        // 체크무늬 템플릿 (크기 · 칸 크기)
        [[nodiscard]] Vision::GrayImage MakeChecker(int size, int cell)
        {
            constexpr std::uint8_t DARK{ 20 };      // 어두운 칸
            constexpr std::uint8_t BRIGHT{ 235 };   // 밝은 칸
            Vision::GrayImage image{ size, size };  // 템플릿
            for (int y = 0; y < size; ++y)
                for (int x = 0; x < size; ++x)
                    image.Set(x, y, ((x / cell + y / cell + x * y / (cell * cell * 3)) % 2 == 0) ? BRIGHT : DARK);   // 불규칙한 체크무늬
            return image;   // 템플릿
        }

        // 화면 인식 기반 검사 (게임 없이: 매칭 · 숫자 읽기 · 확정 규칙 · 배치 저장 형식)
        void CheckVision(Reporter& reporter)
        {
            // 1) 템플릿 매칭: 작은 것 · 큰 것(축소 단계) 모두 정확한 자리를 찾는지
            const Vision::GrayImage smallPattern = MakeChecker(SMALL_PATTERN_SIZE, SMALL_CHECKER_CELL);   // 작은 템플릿
            const Vision::GrayImage largePattern = MakeChecker(LARGE_PATTERN_SIZE, LARGE_CHECKER_CELL);   // 큰 템플릿
            const Vision::GrayImage scene = MakeScene(smallPattern, largePattern);                      // 장면
            const Vision::PixelRect whole{ 0, 0, SCENE_WIDTH, SCENE_HEIGHT };             // 전체 검색
            const auto smallMatch = Vision::TemplateMatcher::FindBest(scene, smallPattern, whole, MATCH_PASS_SCORE);   // 작은 템플릿 찾기
            const auto largeMatch = Vision::TemplateMatcher::FindBest(scene, largePattern, whole, MATCH_PASS_SCORE);   // 큰 템플릿 찾기
            reporter.Check(smallMatch && smallMatch->m_x == SMALL_PATTERN_X && smallMatch->m_y == SMALL_PATTERN_Y,
                "화면 인식 · 템플릿 매칭 (작은 그림)", smallMatch ? std::format("({}, {}) {:.3f}", smallMatch->m_x, smallMatch->m_y, smallMatch->m_score) : "못 찾음");
            reporter.Check(largeMatch && largeMatch->m_x == LARGE_PATTERN_X && largeMatch->m_y == LARGE_PATTERN_Y,
                "화면 인식 · 템플릿 매칭 (큰 그림, 축소 단계)", largeMatch ? std::format("({}, {}) {:.3f}", largeMatch->m_x, largeMatch->m_y, largeMatch->m_score) : "못 찾음");

            // 2) 숫자 읽기: 글꼴 줄로 템플릿을 만들고 다른 줄을 읽음
            const Vision::DigitReadOptions options;                                       // 밝은 글자 · 오츠 자동
            const auto glyphs = Vision::DigitReader::MakeGlyphsFromLine(RenderText(TRAINING_LINE).ToGray(), TRAINING_SYMBOLS, options);   // 템플릿 만들기
            reporter.Check(glyphs && glyphs->size() == TRAINING_SYMBOLS.size(), "화면 인식 · 숫자 템플릿 만들기",
                glyphs ? std::format("{}글자", glyphs->size()) : "글자 수 불일치");
            if (glyphs)
            {
                const Vision::DigitReader reader{ *glyphs };                              // 숫자 읽기 도구
                const auto shard = reader.Read(RenderText(L"12,345").ToGray(), options);  // 쉼표 숫자
                reporter.Check(shard.m_value == EXPECTED_SHARD_VALUE && !shard.m_isOverflow, "화면 인식 · 숫자 읽기 (12,345)",
                    std::format("'{}' 최저 {:.3f}", shard.m_text, shard.m_lowestScore));
                const auto overflow = reader.Read(RenderText(L"9999+").ToGray(), options);// 넘침 표시
                reporter.Check(overflow.m_isOverflow && overflow.m_value == EXPECTED_OVERFLOW_VALUE, "화면 인식 · 넘침 표시 (9999+ → 툴팁 대기)",
                    std::format("'{}'", overflow.m_text));
                const auto empty = reader.Read(Vision::GrayImage{ VISION_CANVAS_WIDTH, VISION_CANVAS_HEIGHT }, options);   // 빈 칸
                reporter.Check(!empty.m_value, "화면 인식 · 빈 칸은 값 없음");
            }

            // 3) 확정 규칙: 같은 결과 2번 연속이어야 확정, 이미 낸 결과는 다시 안 냄
            Vision::ScanConfirmer confirmer;                                              // 기본 2프레임
            Vision::ScanResult first{ "storage", "본캐", { { "dest_top", 100 } } };         // 결과 A
            Vision::ScanResult second{ "storage", "본캐", { { "dest_top", 120 } } };        // 결과 B
            const bool isFirstHeld = !confirmer.Feed(first);                              // 1번째 A → 보류
            const bool isFirstConfirmed = confirmer.Feed(first).has_value();              // 2번째 A → 확정
            const bool isRepeatSkipped = !confirmer.Feed(first);                          // 3번째 A → 중복이라 안 냄
            const bool isChangeHeld = !confirmer.Feed(second);                            // B 1번째 → 보류
            const bool isChangeConfirmed = confirmer.Feed(second).has_value();            // B 2번째 → 확정
            reporter.Check(isFirstHeld && isFirstConfirmed && isRepeatSkipped && isChangeHeld && isChangeConfirmed,
                "화면 인식 · 2프레임 확정 규칙");

            // 4) 화면 배치 저장 형식: 쓰고 다시 읽으면 같은지 · 비율 → 픽셀
            Vision::ScreenLayout layout{ "storage", "storage_anchor.png", { 0.1, 0.1, 0.3, 0.05 }, 0.85,
                { { "dest_top", { 0.25, 0.5, 0.05, 0.02 } } } };                          // 예시 배치
            const auto readBack = Vision::ScreenLayoutStore::ReadText(Vision::ScreenLayoutStore::WriteText(std::span{ &layout, 1 }));   // 왕복
            const bool isSame = readBack.size() == 1 && readBack[0].m_screenName == layout.m_screenName
                && readBack[0].m_regions.size() == 1 && readBack[0].FindRegion("dest_top") != nullptr
                && readBack[0].m_anchorMinScore == layout.m_anchorMinScore;               // 내용 비교
            const Vision::PixelRect pixels = layout.m_regions.front().m_rect.ToPixels(SAMPLE_SCREEN_WIDTH, SAMPLE_SCREEN_HEIGHT);   // 1920x1080 기준 픽셀
            reporter.Check(isSame && pixels == EXPECTED_REGION_PIXELS, "화면 인식 · 배치 저장 형식 · 비율 변환",
                std::format("({}, {}, {}, {})", pixels.m_x, pixels.m_y, pixels.m_width, pixels.m_height));

            // 5) 운영체제 기능 (안내만: 실행하는 PC 환경에 따라 다름)
            reporter.Note(std::format("화면 인식 · 창 캡처 지원: {}", Vision::ScreenCaptureService::IsSupported() ? "예" : "아니오"));
            std::string ocrNote;                                                          // OCR 안내
            std::thread ocrWorker{ [&ocrNote]                                             // WinRT 아파트를 따로 쓰는 일꾼
                {
                    const Vision::VisionApartment apartment;                              // 이 스레드의 WinRT 준비
                    const Vision::TextRecognizer recognizer;                              // OCR 엔진
                    ocrNote = recognizer.IsAvailable()
                        ? std::format("화면 인식 · OCR 엔진: 사용 가능 (언어 {})", std::string(recognizer.GetLanguageTag().begin(), recognizer.GetLanguageTag().end()))
                        : std::string{ "화면 인식 · OCR 엔진: 없음 (윈도우 설정 → 언어에서 한국어 OCR 설치 필요)" };   // 결과
                } };
            ocrWorker.join();                                                             // 끝날 때까지 기다림
            reporter.Note(ocrNote);                                                       // 안내 기록
        }
    }

    bool IsRequested(std::wstring_view commandLine) noexcept
    {
        return commandLine.find(SELF_TEST_ARGUMENT) != std::wstring_view::npos;   // 인자 포함 여부
    }

    int Run(const std::filesystem::path& reportPath)
    {
        Reporter reporter;                                       // 결과 모음
        try
        {
            CheckCombatCoefficients(reporter);                   // 전투력 계수
            CheckRefineTables(reporter);                         // 재련 표
            CheckVersionResource(reporter);                      // 실행 파일 버전 정보
            CheckTransferRules(reporter);                        // 2부 이관 표 규칙
            CheckTransferArmories(reporter, reportPath);         // 2부 이관 계산 (저장된 무기고)
            CheckLopecRegression(reporter, reportPath);          // lopec 회귀
            CheckVision(reporter);                               // 화면 인식 기반
        }
        catch (const std::exception& exception)
        {
            reporter.Check(false, "검사 중 예외", exception.what());   // 예외도 실패로
        }

        std::ofstream file{ reportPath, std::ios::binary | std::ios::trunc };   // 보고서 (UTF-8)
        file << std::format("LostArkMultiTool {} self-test\n", APPConstants::APP_VERSION) << reporter.GetText()
             << std::format("FAILED {}\n", reporter.GetFailCount());           // 마지막 줄 = 실패 수
        return reporter.GetFailCount();                                       // 0이면 통과
    }
}
