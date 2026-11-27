module;

#define NOMINMAX
#include <Windows.h>									// GetModuleFileNameW, GetFileVersionInfoW, VerQueryValueW

#pragma comment(lib, "version.lib")					// 버전 정보 API
#pragma comment(lib, "gdi32.lib")					// 숫자 읽기 검사용 글자 그리기 (GDI)

module LostArkMultiTool.Core.SelfTest;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.CombatRegression;
import LostArkMultiTool.Models.BoundGemData;  // 젬 수량 종류별 합계
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.RefineData;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.TransferData;
import LostArkMultiTool.Services.ArmoryParser;
import LostArkMultiTool.Services.JSONValue;
import LostArkMultiTool.Services.TransferCalculator;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Vision.BossHealthDetector;
import LostArkMultiTool.Views.BossPanel;
import LostArkMultiTool.Vision.CombatAnalyzerDetector;  // 전투 분석기 인식기
import LostArkMultiTool.Views.DamagePanel;  // 딜 지분 계산
import LostArkMultiTool.Vision.MaterialSlotDetector;  // 창고 · 소지품 슬롯 인식기
import LostArkMultiTool.Vision.GemTooltipDetector;
import LostArkMultiTool.Vision.CharacterDetector;
import LostArkMultiTool.Vision.DigitReader;
import LostArkMultiTool.Vision.NameplateMemory;
import LostArkMultiTool.Vision.FrameImage;
import LostArkMultiTool.Vision.ImageFile;
import LostArkMultiTool.Vision.OCRTextParser;
import LostArkMultiTool.Vision.ParadiseDetector;
import LostArkMultiTool.Vision.ScreenCaptureService;
import LostArkMultiTool.Vision.ShardBarDetector;
import LostArkMultiTool.Vision.TooltipDetector;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.DX11Renderer;
import LostArkMultiTool.Views.OverlayWindow;
import LostArkMultiTool.Views.MiniToolPanel;
import LostArkMultiTool.Views.CharacterPanel;
import LostArkMultiTool.Views.MainWindow;
import LostArkMultiTool.Views.UI.Slider;
import LostArkMultiTool.Views.OverlayPlaceholderTool;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.ToolManager;
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

            // 5) OCR 글자 해석: 숫자 묶음 · 재료 이름 (OCR이 흔히 틀리는 모양 포함)
            namespace Parser = Vision::OCRTextParser;                                    // 해석 함수 묶음
            constexpr std::int64_t EXPECTED_DEST{ 163439 };                              // 파괴석 결정 수량
            constexpr std::int64_t EXPECTED_PARADISE{ 29492422 };                        // 낙원력
            const bool isNumberParsed = Parser::ParseGroupedNumber(L"163.439개") == EXPECTED_DEST     // 쉼표 → 마침표
                && Parser::ParseGroupedNumber(L"29,492 422") == EXPECTED_PARADISE                       // 쉼표 → 띄어쓰기
                && Parser::ParseGroupedNumber(L"29,492, 422") == EXPECTED_PARADISE                      // 구분자 두 글자
                && !Parser::ParseGroupedNumber(L"29,4922").has_value();                                 // 묶음이 이상하면 버림
            reporter.Check(isNumberParsed, "화면 인식 · 숫자 묶음 해석 (163.439 · 29,492 422 · 29,4922)");
            const auto matchKey = [](std::wstring_view line) -> std::string                             // 줄 → "키/구분"
                {
                    const auto match = Parser::MatchMaterial(line);                                     // 재료 맞추기
                    return match ? Parser::MakeMaterialValueKey(match->m_material->m_key, match->m_material->m_tier) : std::string{ "-" };
                };
            const bool isNameMatched = matchKey(L"운명의 피괴석 결정 (귀속) [X 1634391") == "Dest/top"      // 한 글자 오인식 + 결정
                && matchKey(L"운명의 파괴석 (귀속)") == "Dest/bottom"                                    // 하위
                && matchKey(L"위대한 운명의 돌파석 (귀속)") == "Leap/top"                                // 위대한
                && matchKey(L"아비도스 융화 재료 (귀속)") == "Fusion/bottom"                              // 상급 아님
                && matchKey(L"용암의 숨결 (귀속)") == "Yong/level"                                       // 레벨로 정함
                && matchKey(L"캐릭터 귀속됨") == "-";                                                   // 재료 아님
            reporter.Check(isNameMatched, "화면 인식 · 재료 이름 맞추기 (결정 · 위대한 · 상급 구분, 한 글자 오인식)");

            // 6) 운영체제 기능 (안내만: 실행하는 PC 환경에 따라 다름)
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

    namespace  // 실제 게임 화면 샘플 검사 (Verify/screens, 개발 PC에만 있음)
    {
        constexpr std::wstring_view SAMPLE_FOLDER{ L"Verify/screens" };   // 샘플 폴더 (실행 파일 위쪽에서 찾음)
        constexpr int MAX_PARENT_SEARCH{ 6 };                             // 위로 올라가며 찾을 최대 단계

        // 샘플 하나: 파일 · 커서 위치(이미지 기준 비율) · 기대 결과
        struct VisionSample
        {
            std::wstring_view m_file;        // 파일 이름
            double m_cursorX;                // 커서 가로 비율 (0 ~ 1)
            double m_cursorY;                // 커서 세로 비율
            std::string_view m_valueKey;     // 기대 값 이름
            std::int64_t m_expected;         // 기대 값
        };

        constexpr std::array VISION_SAMPLES{
            VisionSample{ L"tooltip_dest_top.png", 0.5, 0.5, "Dest/top", 163439 },      // 파괴석 결정 툴팁
            VisionSample{ L"tooltip_guard_top.png", 0.5, 0.5, "Guard/top", 363321 },    // 수호석 결정 툴팁
            VisionSample{ L"tooltip_guard_top_bound.png", 0.5, 0.5, "Guard/top", 17482 },   // 수호석 결정 (귀속) 툴팁 ("1 7 482개"로 띄어 읽음)
            VisionSample{ L"tooltip_leap_top.png", 0.5, 0.5, "Leap/top", 12501 },       // 위대한 돌파석 툴팁
            VisionSample{ L"paradise.webp", 0.85, 0.1, "paradise", 29492422 },          // 낙원 UI (현재 낙원력 숫자 근처)
            VisionSample{ L"topbar_shard.webp", 0.5, 0.9, "Shard/level", 24615414 } };  // 상단바 운명의 파편 (커서가 화면 아래쪽이어도 읽어야 함)

        // 실행 파일 위쪽 폴더에서 샘플 폴더 찾기
        [[nodiscard]] std::optional<std::filesystem::path> FindSampleFolder(const std::filesystem::path& start)
        {
            std::filesystem::path folder = start;                                 // 지금 폴더
            for (int step = 0; step < MAX_PARENT_SEARCH; ++step)                  // 위로 올라가며
            {
                std::error_code error;                                            // 예외 없는 검사용
                if (std::filesystem::is_directory(folder / SAMPLE_FOLDER, error)) return folder / SAMPLE_FOLDER;   // 찾음
                if (!folder.has_parent_path() || folder.parent_path() == folder) break;   // 맨 위
                folder = folder.parent_path();                                    // 한 단계 위
            }
            return std::nullopt;   // 없음
        }

        // 샘플 하나를 인식기들로 읽어 결과 값 찾기 (OCR은 이 함수를 부르는 스레드에서)
        [[nodiscard]] std::optional<std::int64_t> ReadSample(const Vision::FrameImage& frame, const VisionSample& sample,
            const Vision::TextRecognizer& recognizer, const Vision::ScreenLayoutStore& layouts)
        {
            const Vision::PixelPoint cursor{ static_cast<int>(frame.GetWidth() * sample.m_cursorX), static_cast<int>(frame.GetHeight() * sample.m_cursorY) };   // 커서
            const Vision::PixelRect hoverArea = Vision::ScreenWatcher::GetHoverArea(cursor, frame.GetWidth(), frame.GetHeight());   // 커서 주변
            const auto text = recognizer.Recognize(frame.Crop(hoverArea));                // OCR
            if (!text) return std::nullopt;                                                // OCR 실패
            const Vision::DetectorContext context{ layouts, recognizer, *text, hoverArea, cursor };   // 인식기 공용 도구
            Vision::TooltipDetector tooltip;                                               // 툴팁 인식기
            Vision::ParadiseDetector paradise;                                             // 낙원 인식기
            Vision::ShardBarDetector shard;                                                // 상단바 인식기
            for (Vision::ScreenDetector* detector : std::array<Vision::ScreenDetector*, 3>{ &tooltip, &paradise, &shard })   // 인식기마다
            {
                const auto result = detector->Detect(frame, context);                      // 읽기
                if (!result) continue;                                                     // 이 화면이 아님
                for (const Vision::ScanValue& value : result->m_values)                    // 값마다
                    if (value.m_key == sample.m_valueKey) return value.m_value;            // 기대한 값 이름
            }
            return std::nullopt;   // 못 읽음
        }

        // 실제 화면 샘플 검사 (샘플이 없으면 건너뜀)
        void CheckVisionSamples(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            const auto folder = FindSampleFolder(reportPath.parent_path());               // 샘플 폴더
            if (!folder) { reporter.Note("화면 인식 샘플 검사 건너뜀 (Verify/screens 없음)"); return; }   // 개발 PC가 아님

            std::vector<std::pair<std::string, bool>> outcomes;                           // (이름, 통과) 목록
            std::thread worker{ [&]                                                       // OCR 일꾼 스레드
                {
                    const Vision::VisionApartment apartment;                              // WinRT · DPI 준비
                    const Vision::TextRecognizer recognizer;                              // OCR 엔진
                    const Vision::ScreenLayoutStore layouts;                              // 화면 배치 (지금은 안 씀)
                    for (const VisionSample& sample : VISION_SAMPLES)                     // 샘플마다
                    {
                        const auto frame = Vision::ImageFile::Load(*folder / sample.m_file);   // 이미지 읽기
                        const std::string name = std::string(sample.m_file.begin(), sample.m_file.end());   // 파일 이름 (영문)
                        if (!frame) { outcomes.emplace_back(name + " : 파일을 못 읽음", false); continue; }   // 실패
                        const auto value = ReadSample(*frame, sample, recognizer, layouts);   // 읽기
                        outcomes.emplace_back(std::format("{} : {} = {} (기대 {})", name, sample.m_valueKey,
                            value ? std::to_string(*value) : std::string{ "못 읽음" }, sample.m_expected), value == sample.m_expected);   // 결과
                    }
                } };
            worker.join();                                                                // 끝날 때까지 기다림
            for (const auto& [text, isPassed] : outcomes) reporter.Check(isPassed, "화면 인식 샘플", text);   // 기록
        }
    }

    namespace  // 현재 캐릭터 인식 검사
    {
        constexpr double ITEM_LEVEL_EPSILON{ 0.001 };   // 아이템 레벨 비교 오차

        // 원정대 이름 맞추기 (자모 거리 · 애매함 · 별칭) + 아이템 레벨 읽기 (OCR 없이)
        void CheckCharacterMatching(Reporter& reporter)
        {
            constexpr std::size_t ONE_JAMO{ 1 };                                                // 자모 하나 차이
            const bool isJamo = Vision::CharacterDetector::FindJamoDistance(L"외숡모", L"외숙모") == ONE_JAMO      // 겹받침 → 홑받침
                && Vision::CharacterDetector::FindJamoDistance(L"읾모", L"읽모") == ONE_JAMO                       // 받침 하나 다름
                && Vision::CharacterDetector::FindJamoDistance(L"할머니", L"할머니") == 0;                         // 같음
            reporter.Check(isJamo, "현재 캐릭터 · 자모 거리 (외숡모↔외숙모 1 · 읾모↔읽모 1)");

            const Vision::CharacterDetector detector{ { L"외숡모", L"당숡모", L"읾모", L"할머니" } };   // 검사 원정대
            const auto fuzzy = detector.MatchName(L"외숙모");                                    // 한 자모 오인식
            const auto exact = detector.MatchName(L"할머니");                                    // 정확히 같음
            const auto other = detector.MatchName(L"정찻");                                      // 다른 사람
            const auto title = detector.MatchName(L"죽음의");                                    // 칭호
            const bool isMatched = fuzzy && fuzzy->first == L"외숡모" && fuzzy->second == ONE_JAMO
                && exact && exact->first == L"할머니" && exact->second == 0 && !other && !title;
            reporter.Check(isMatched, "현재 캐릭터 · 원정대 이름 맞추기 (OCR 한 자모 오인식 · 다른 사람 · 칭호 제외)");

            const Vision::CharacterDetector ambiguous{ { L"가나", L"가다" } };                   // 거리가 같은 두 이름
            reporter.Check(!ambiguous.MatchName(L"가라").has_value(), "현재 캐릭터 · 애매하면 고르지 않음 (가라 → 가나 · 가다)");

            const Vision::CharacterDetector aliased{ { L"외숡모" }, { Vision::CharacterAlias{ L"외숨 모", L"외숡모" } } };   // 별칭 (띄어쓰기 무시)
            const auto alias = aliased.MatchName(L"외숨모");                                     // 별칭 글자
            reporter.Check(alias && alias->first == L"외숡모" && alias->second == 0, "현재 캐릭터 · 직접 고른 캐릭터(별칭) 기억");

            const auto close = [](std::optional<double> value, double expected) { return value && std::abs(*value - expected) < ITEM_LEVEL_EPSILON; };
            constexpr double EXPECTED_LEVEL{ 1775.0 };                                           // 기대 레벨
            const bool isLevel = close(Vision::CharacterDetector::ParseItemLevel(L"1775.00"), EXPECTED_LEVEL)
                && close(Vision::CharacterDetector::ParseItemLevel(L"1,775.00"), EXPECTED_LEVEL)
                && close(Vision::CharacterDetector::ParseItemLevel(L"I775.0O"), EXPECTED_LEVEL)
                && !Vision::CharacterDetector::ParseItemLevel(L"Lv.70").has_value()
                && !Vision::CharacterDetector::ParseItemLevel(L"전투력").has_value();
            reporter.Check(isLevel, "현재 캐릭터 · 아이템 레벨 읽기 (1775.00 · 1,775.00 · I775.0O, Lv.70 제외)");
        }

        // 실제 화면 샘플: 이름표 · 캐릭터 정보 창 · 로딩 화면 (샘플이 없으면 건너뜀)
        void CheckCharacterSamples(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            const auto folder = FindSampleFolder(reportPath.parent_path());               // 샘플 폴더
            if (!folder) { reporter.Note("현재 캐릭터 샘플 검사 건너뜀 (Verify/screens 없음)"); return; }   // 개발 PC가 아님

            std::vector<std::pair<std::string, bool>> outcomes;                           // (내용, 통과)
            std::thread worker{ [&]                                                       // OCR 일꾼 스레드
                {
                    const Vision::VisionApartment apartment;                              // WinRT · DPI 준비
                    const Vision::TextRecognizer recognizer;                              // OCR 엔진
                    const std::vector<std::wstring> roster{ L"외숡모", L"당숡모", L"읾모", L"핢머니", L"정찻" };   // 원정대 (다른 사람 '정찻'도 넣어 노란 이름 규칙 검사)
                    const auto memory = std::make_shared<Vision::NameplateMemory>(Vision::MemoryStorage::STORAGE_NONE);   // 파일을 건드리지 않는 모양 기억
                    const Vision::CharacterDetector detector{ roster, {}, memory };       // 인식기
                    const auto load = [&](std::wstring_view file) { return Vision::ImageFile::Load(*folder / file); };   // 이미지 읽기
                    const auto read = [&](std::wstring_view file) -> std::optional<Vision::CharacterReading>   // 샘플 하나 읽기
                        {
                            const auto frame = load(file);                                // 이미지
                            if (!frame) return std::nullopt;                              // 파일 없음
                            return detector.Detect(*frame, recognizer);                   // 인식
                        };
                    const auto describe = [](const std::optional<Vision::CharacterReading>& reading)   // 결과 글자
                        {
                            if (!reading) return std::string{ "파일을 못 읽음" };
                            return std::format("신호 {} · 곳 {} · 이름 '{}' · 레벨 {}", static_cast<int>(reading->m_signal), static_cast<int>(reading->m_source),
                                Core::TextEncoding::WideToUTF8(reading->m_name), reading->m_itemLevel ? std::format("{:.2f}", *reading->m_itemLevel) : std::string{ "-" });
                        };

                    // 내 이름표 찾기 (연한 노란 글자만, 칭호 · 길드 · 다른 사람 제외)
                    const auto plateFrame = load(L"character_nameplate.webp");           // 머리 위 이름표 화면
                    const auto blob = plateFrame ? Vision::NameplateMemory::FindOwnNameplate(*plateFrame) : std::nullopt;   // 내 이름표
                    constexpr int EXPECTED_LEFT{ 985 };                                   // 기대 위치 (샘플 기준 "핢머니" 글자)
                    constexpr int EXPECTED_RIGHT{ 1022 };
                    constexpr int POSITION_TOLERANCE{ 3 };                                // 허용 오차 (픽셀)
                    outcomes.emplace_back(blob ? std::format("내 이름표 위치 x {} ~ {} · y {} · {}×{}", blob->m_bounds.m_x, blob->m_bounds.GetRight() - 1, blob->m_bounds.m_y,
                        blob->m_bounds.m_width, blob->m_bounds.m_height) : std::string{ "내 이름표 위치: 못 찾음" },
                        blob && std::abs(blob->m_bounds.m_x - EXPECTED_LEFT) <= POSITION_TOLERANCE && std::abs(blob->m_bounds.GetRight() - 1 - EXPECTED_RIGHT) <= POSITION_TOLERANCE);

                    const auto plate = read(L"character_nameplate.webp");                // 처음: 이름표 OCR로 맞추고 모양 기억
                    outcomes.emplace_back("이름표 → 핢머니 (이름표만 3배 OCR · 자모 비교) : " + describe(plate),
                        plate && plate->m_signal == Vision::CharacterSignal::SIGNAL_NAME && plate->m_name == L"핢머니");
                    const bool isLearned = blob && memory->Match(blob->m_shape) == std::optional<std::string>{ Core::TextEncoding::WideToUTF8(L"핢머니") };   // 모양 기억
                    outcomes.emplace_back("이름표 모양 기억 → 다음부터 OCR 없이 핢머니", isLearned);

                    const auto manual = std::make_shared<Vision::NameplateMemory>(Vision::MemoryStorage::STORAGE_NONE);   // [변경]으로 고를 때 흐름
                    if (blob) manual->SetPending(blob->m_shape);                          // 못 알아본 모양 대기
                    constexpr std::chrono::seconds PENDING_AGE{ 60 };                     // 대기 시간
                    const bool isPicked = manual->LearnPending(Core::TextEncoding::WideToUTF8(L"외숡모"), PENDING_AGE)
                        && manual->Match(blob->m_shape) == std::optional<std::string>{ Core::TextEncoding::WideToUTF8(L"외숡모") };   // 고른 캐릭터로 기억
                    outcomes.emplace_back("[변경]으로 고르면 그 이름표 모양을 기억", isPicked);

                    const auto info = read(L"character_info.webp");                      // 캐릭터 정보 창 ("Lv." 줄 위 이름)
                    const bool isInfoRead = info && (info->m_signal == Vision::CharacterSignal::SIGNAL_NAME ? info->m_name == L"핢머니"
                        : (!info->m_unknownName.empty() && info->m_unknownName != L"예리"));       // 길드 이름("Lv.38 Family" 위)을 이름으로 잡으면 안 됨
                    outcomes.emplace_back("정보 창 → 핢머니 또는 별칭 후보 : " + describe(info) + " · 못 맞춘 글자 '" + (info ? Core::TextEncoding::WideToUTF8(info->m_unknownName) : std::string{}) + "'", isInfoRead);
                    if (info && info->m_signal != Vision::CharacterSignal::SIGNAL_NAME && !info->m_unknownName.empty())   // 별칭으로 기억하면 다음엔 맞춤
                    {
                        const Vision::CharacterDetector aliased{ roster, { Vision::CharacterAlias{ info->m_unknownName, L"핢머니" } } };   // 별칭 인식기
                        const auto infoFrame = load(L"character_info.webp");             // 다시 읽기
                        const auto again = infoFrame ? std::optional{ aliased.Detect(*infoFrame, recognizer) } : std::nullopt;
                        outcomes.emplace_back("정보 창 별칭 기억 → 핢머니 : " + describe(again), again && again->m_signal == Vision::CharacterSignal::SIGNAL_NAME && again->m_name == L"핢머니");
                    }

                    const auto loading = read(L"loading.webp");                          // 로딩 화면 (픽셀)
                    outcomes.emplace_back("로딩 화면 → 다시 확인 : " + describe(loading),
                        loading && loading->m_signal == Vision::CharacterSignal::SIGNAL_LOADING);
                    const bool isNotLoading = plateFrame && !Vision::CharacterDetector::IsLoadingScreen(*plateFrame);   // 게임 화면은 로딩 아님
                    outcomes.emplace_back("게임 화면은 로딩 화면이 아님", isNotLoading);
                } };
            worker.join();                                                                // 끝날 때까지 기다림
            for (const auto& [text, isPassed] : outcomes) reporter.Check(isPassed, "현재 캐릭터 샘플", text);   // 기록
        }
    }

    namespace  // 인게임 오버레이 검사
    {
        constexpr Core::Color OVERLAY_TEST_CLEAR{ 0.0f, 0.0f, 0.0f, 0.0f };   // 투명 지우기 색

        // 숨은 오버레이 창 + 투명 렌더러를 만들어 한 프레임 그려 봄 (DirectComposition · 미리 곱한 알파 스왑체인)
        void CheckOverlay(Reporter& reporter)
        {
            try
            {
                Views::OverlayWindow window;                                                   // 숨은 투명 창
                Views::DX11Renderer renderer{ window.GetNativeHandle(), window.GetDPI(), Views::SurfaceMode::SURFACE_TRANSPARENT };   // 투명 렌더러
                const bool isBegun = renderer.BeginFrame();                                    // 그리기 시작
                constexpr int TEST_IMAGE_SIZE{ 4 };                                            // 검사 그림 크기
                constexpr std::uint8_t TEST_CHANNEL{ 200 };                                    // 검사 그림 색
                Core::ImageData image;                                                         // 검사 그림 (설정 탭 미리 보기 · 로고와 같은 경로)
                image.m_width = TEST_IMAGE_SIZE;
                image.m_height = TEST_IMAGE_SIZE;
                image.m_version = 1;
                image.m_pixels.assign(static_cast<std::size_t>(TEST_IMAGE_SIZE) * TEST_IMAGE_SIZE * 4, TEST_CHANNEL);   // 불투명한 회색
                if (isBegun)
                {
                    renderer.Clear(OVERLAY_TEST_CLEAR);                                        // 투명으로
                    renderer.DrawImage(image, Core::RectF{ 0.0f, 0.0f, 32.0f, 32.0f }, 1.0f);  // 그림 그리기 (비트맵 만들기)
                    renderer.DrawImage(image, Core::RectF{ 0.0f, 0.0f, 32.0f, 32.0f }, 0.5f);  // 같은 그림 다시 (캐시)
                    renderer.EndFrame();                                                       // 내보내기
                }
                reporter.Check(isBegun, "인게임 오버레이 · 투명 창 · 렌더러 만들기 · 그림 그리기");   // 결과
            }
            catch (const std::exception& error)
            {
                reporter.Check(false, "인게임 오버레이 · 투명 창 · 렌더러 만들기", error.what());   // 실패 이유
            }
        }

        // 보스 체력바 읽기 (v1.10.0): 글자 해석 · 단위 글자 · 실제 화면 샘플
        void CheckBossHealth(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            constexpr std::int64_t SAMPLE_HEALTH{ 83'119'120'458 };                    // 샘플 체력 (루멘칼리고)
            constexpr int SAMPLE_LINES{ 100 };                                         // 샘플 줄 수
            const auto parse = [](std::initializer_list<std::wstring_view> lines)      // 줄들 → 해석
                {
                    Vision::TextResult text;                                           // 가짜 OCR 결과
                    for (std::wstring_view line : lines) text.m_lines.push_back(Vision::TextLine{ std::wstring{ line }, {}, {} });
                    return Vision::BossHealthDetector::Parse(text);
                };
            const auto spaced = parse({ L"83.1 19.1 20.458/83.119.120.458", L"X 100" });   // 쉼표를 마침표 · 띄어쓰기로 읽음
            const auto bracket = parse({ L"41.559.560.229/83,119,120,458 )(100" });     // X를 ")("로 읽음, 한 줄
            const auto broken = parse({ L"83.119.120.458i83111의12다458" });            // '/'를 못 읽음 → 버림
            const auto reversed = parse({ L"90,000/80,000" });                          // 현재 > 최대 → 버림
            const bool isParsed = spaced && spaced->m_current == SAMPLE_HEALTH && spaced->m_maximum == SAMPLE_HEALTH && spaced->m_lines == SAMPLE_LINES
                && bracket && bracket->m_current == 41'559'560'229 && bracket->m_lines == SAMPLE_LINES && !broken && !reversed;
            reporter.Check(isParsed, "보스 체력 · 글자 해석 (쉼표 오인식 · X 오인식 · '/' 없음 · 현재 > 최대 제외)");

            const bool isFormatted = Views::BossPanel::FormatKoreanNumber(SAMPLE_HEALTH) == L"831억 1912만 458"
                && Views::BossPanel::FormatKoreanNumber(1'234'056'789'012) == L"1조 2340억 5678만 9012"
                && Views::BossPanel::FormatKoreanNumber(100'000'050'003) == L"1000억 5만 3"
                && Views::BossPanel::FormatKoreanNumber(50'000) == L"5만";
            reporter.Check(isFormatted, "보스 체력 · 1의 자리까지 (831억 1912만 458 · 1조 2340억 5678만 9012)");

            const auto folder = FindSampleFolder(reportPath.parent_path());           // 샘플 폴더
            if (!folder) { reporter.Note("보스 체력 샘플 검사 건너뜀 (Verify/screens 없음)"); return; }
            struct BossSample
            {
                std::wstring_view m_file;      // 파일
                std::int64_t m_current;        // 기대 현재 체력
                int m_lines;                   // 기대 줄 수
                std::string_view m_name;       // 검사 이름
            };
            constexpr std::array BOSS_SAMPLES{
                BossSample{ L"boss_hp.webp", SAMPLE_HEALTH, SAMPLE_LINES, "가득 · 주황 막대" },
                BossSample{ L"boss_hp_half.webp", 37'019'552'389, 45, "절반 · 노란 막대" },
                BossSample{ L"boss_hp_low.webp", 4'816'559'070, 6, "조금 · 파란 막대" } };   // 체력 · 막대 색이 다른 세 장
            std::vector<std::optional<Vision::BossHealthReading>> readings(BOSS_SAMPLES.size());   // 읽은 값
            std::thread worker{ [&]                                                    // OCR 일꾼 스레드
                {
                    const Vision::VisionApartment apartment;                           // WinRT 준비
                    const Vision::TextRecognizer recognizer;                           // OCR
                    for (std::size_t index = 0; index < BOSS_SAMPLES.size(); ++index)  // 샘플마다
                        if (const auto frame = Vision::ImageFile::Load(*folder / BOSS_SAMPLES[index].m_file)) readings[index] = Vision::BossHealthDetector::Detect(*frame, recognizer);
                } };
            worker.join();                                                             // 기다림
            for (std::size_t index = 0; index < BOSS_SAMPLES.size(); ++index)          // 한 장씩: 현재 체력 · 줄 수 (최대 체력은 아래 투표에서)
            {
                const BossSample& sample = BOSS_SAMPLES[index];                        // 기대
                const auto& reading = readings[index];                                 // 읽은 값
                reporter.Check(reading && reading->m_current == sample.m_current && reading->m_lines == sample.m_lines,
                    std::format("보스 체력 샘플 ({})", sample.m_name),
                    reading ? std::format("{} / {} · {}줄", reading->m_current, reading->m_maximum, reading->m_lines.value_or(0)) : std::string{ "못 읽음" });
            }
            Vision::BossHealthTracker tracker;                                         // 여러 장 투표
            std::optional<Vision::BossHealthReading> voted;                            // 마지막 결과
            for (const std::size_t index : { std::size_t{ 2 }, std::size_t{ 1 }, std::size_t{ 0 }, std::size_t{ 1 } })   // 조금 → 절반(최대 오인식) → 가득 → 절반
                if (readings[index]) voted = tracker.Feed(*readings[index]);
            reporter.Check(voted && voted->m_maximum == SAMPLE_HEALTH && voted->m_current == BOSS_SAMPLES[1].m_current,
                "보스 체력 · 최대 체력 투표 (한 장 오인식을 고침)",
                voted ? std::format("{} / {}", voted->m_current, voted->m_maximum) : std::string{ "결과 없음" });
        }

        // 전투 분석기 · 딜 지분 (v1.10.2): 단위 글자 해석 · 보스 샘플 세 장 · 지분 계산
        void CheckCombatAnalyzer(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            using Vision::CombatAnalyzerDetector;
            const bool isAmount = CombatAnalyzerDetector::ParseAmount(L"460,99익") == 46'099'000'000
                && CombatAnalyzerDetector::ParseAmount(L"3.51역") == 351'000'000
                && CombatAnalyzerDetector::ParseAmount(L"4,827.14만") == 48'271'400
                && CombatAnalyzerDetector::ParseAmount(L"12,345") == 12'345
                && !CombatAnalyzerDetector::ParseAmount(L"-").has_value();
            reporter.Check(isAmount, "전투 분석기 · 억 · 만 단위 글자 (460,99익 · 3.51역 · 4,827.14만 · 12,345 · -)");

            constexpr std::int64_t SAMPLE_HEALTH{ 83'119'120'458 };                    // 샘플 최대 체력
            constexpr std::int64_t HALF_CURRENT{ 37'019'552'389 };                     // 절반 샘플 현재 체력
            constexpr double SHARE_TOLERANCE{ 0.0001 };                                // 지분 비교 허용 오차
            const auto solo = Views::DamagePanel::Calculate(46'099'000'000, HALF_CURRENT, SAMPLE_HEALTH, Views::DamagePanel::RAID_SIZE_EIGHT);   // 혼자 깎은 절반
            const auto quarter = Views::DamagePanel::Calculate(25, 0, 100, Views::DamagePanel::RAID_SIZE_FOUR);                                  // 4인 25%
            const auto strong = Views::DamagePanel::Calculate(17, 0, 100, Views::DamagePanel::RAID_SIZE_EIGHT);                                  // 8인 17%
            const bool isShare = solo && std::abs(solo->m_depletedShare - 1.0) < SHARE_TOLERANCE && solo->m_stage == 2
                && quarter && std::abs(quarter->m_portions - 0.75) < SHARE_TOLERANCE && quarter->m_stage == 0
                && strong && strong->m_stage == 1 && std::abs(strong->m_portions - 1.02) < SHARE_TOLERANCE
                && !Views::DamagePanel::Calculate(10, 100, 100, Views::DamagePanel::RAID_SIZE_EIGHT).has_value();
            reporter.Check(isShare, "딜 지분 · 깎인 체력 대비 · 몇 인분 · 단계 (8인 15% · 20%, 4인 30% · 40%)");

            const auto folder = FindSampleFolder(reportPath.parent_path());           // 샘플 폴더
            if (!folder) { reporter.Note("전투 분석기 샘플 검사 건너뜀 (Verify/screens 없음)"); return; }
            struct CombatSampleFile
            {
                std::wstring_view m_file;                    // 파일
                std::optional<std::int64_t> m_damage;        // 기대 피해량
                std::optional<std::int64_t> m_dps;           // 기대 DPS
            };
            const std::array samples{
                CombatSampleFile{ L"boss_hp.webp", std::nullopt, std::nullopt },                       // 전투 시작 전 ("-")
                CombatSampleFile{ L"boss_hp_half.webp", 46'099'000'000, 351'000'000 },                 // 460.99억 · 3.51억
                CombatSampleFile{ L"boss_hp_low.webp", 78'298'000'000, 343'000'000 } };                // 782.98억 · 3.43억
            std::vector<std::optional<Vision::CombatReading>> readings(samples.size());   // 읽은 값
            std::thread worker{ [&]                                                    // OCR 일꾼 스레드
                {
                    const Vision::VisionApartment apartment;                           // WinRT 준비
                    const Vision::TextRecognizer recognizer;                           // OCR
                    for (std::size_t index = 0; index < samples.size(); ++index)       // 샘플마다
                        if (const auto frame = Vision::ImageFile::Load(*folder / samples[index].m_file)) readings[index] = CombatAnalyzerDetector::Detect(*frame, recognizer);
                } };
            worker.join();                                                             // 기다림
            for (std::size_t index = 0; index < samples.size(); ++index)
            {
                const auto& reading = readings[index];                                 // 읽은 값
                const bool isPassed = samples[index].m_damage ? (reading && reading->m_damage == samples[index].m_damage && reading->m_dps == samples[index].m_dps) : !reading;
                const std::string outcome = reading ? std::format("피해량 {} · DPS {}", reading->m_damage.value_or(-1), reading->m_dps.value_or(-1)) : std::string{ "값 없음" };
                reporter.Check(isPassed, "전투 분석기 샘플", std::format("{} : {}", std::string(samples[index].m_file.begin(), samples[index].m_file.end()), outcome));
            }
        }

        // 창고 · 소지품 슬롯 (v1.10.3): 마우스 없이 융화 재료 · 숨결 수량 + 캐릭터 확인 전 값 붙잡기
        void CheckMaterialSlots(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            Vision::CharacterValueGate gate;                                           // 값 붙잡기 문
            const auto start = Vision::CharacterValueGate::Clock::now();               // 기준 시각
            Vision::ScanResult sample;                                                 // 아무 결과
            sample.m_screenName = "tooltip";
            gate.Hold(sample, start);                                                  // 하나
            gate.Hold(sample, start + std::chrono::seconds{ 1 });                      // 둘
            const std::size_t early = gate.ReleaseExpired(start + std::chrono::seconds{ 2 }).size();   // 아직 기다림
            const std::size_t expired = gate.ReleaseExpired(start + Vision::CharacterValueGate::MAX_HOLD).size();   // 첫째만
            const std::size_t released = gate.Release().size();                        // 남은 하나
            gate.Hold(sample, start);
            gate.Discard();                                                            // 로딩 → 버림
            reporter.Check(early == 0 && expired == 1 && released == 1 && !gate.HasPending(), "캐릭터 확인 전 값 붙잡기 (같은 캐릭터 → 내보냄 · 오래 기다림 → 내보냄 · 로딩 → 버림)");

            const auto folder = FindSampleFolder(reportPath.parent_path());           // 샘플 폴더
            if (!folder) { reporter.Note("슬롯 샘플 검사 건너뜀 (Verify/screens 없음)"); return; }
            struct SlotSample
            {
                std::wstring_view m_file;                                              // 파일
                std::string_view m_expected;                                           // 기대 값 ("키=값 ...")
            };
            constexpr std::array samples{
                SlotSample{ L"storage.png", "Bing/level=3024 Fusion/level=1178 Yong/level=2720" },             // 창고 (파괴석 · 수호석 9999+는 건너뜀)
                SlotSample{ L"tooltip_guard_top_bound.png", "Fusion/level=207 Yong/level=1423" },             // 툴팁이 일부 가린 창고
                SlotSample{ L"boss_hp.webp", "없음" },                                                          // 1080p 전투 화면 (창고 없음 → 오탐 없어야 함)
                SlotSample{ L"character_info.webp", "없음" } };                                                 // 1080p 캐릭터 정보 창
            std::vector<std::string> outcomes(samples.size());                         // 결과 글자
            std::vector<std::string> timings(samples.size());                          // 크기 · 처리 시간
            std::thread worker{ [&]                                                    // 같은 방식으로 일꾼 스레드
                {
                    for (std::size_t index = 0; index < samples.size(); ++index)
                    {
                        const auto frame = Vision::ImageFile::Load(*folder / samples[index].m_file);   // 이미지
                        if (!frame) { outcomes[index] = "파일을 못 읽음"; continue; }
                        const auto scanStart = std::chrono::steady_clock::now();      // 처리 시간 재기
                        const auto result = Vision::MaterialSlotDetector::DetectFrame(*frame);   // 읽기
                        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - scanStart).count();   // 밀리초
                        std::string text;                                              // "키=값 ..."
                        if (result) for (const Vision::ScanValue& value : result->m_values) text += std::format("{}{}={}", text.empty() ? "" : " ", value.m_key, value.m_value);
                        outcomes[index] = text.empty() ? std::string{ "없음" } : text;
                        timings[index] = std::format("{}x{} · {}ms", frame->GetWidth(), frame->GetHeight(), elapsedMs);   // 크기 · 시간
                    }
                } };
            worker.join();                                                             // 기다림
            for (std::size_t index = 0; index < samples.size(); ++index)
                reporter.Check(outcomes[index] == samples[index].m_expected, "슬롯 샘플",
                    std::format("{} : {} ({})", std::string(samples[index].m_file.begin(), samples[index].m_file.end()), outcomes[index], timings[index]));
        }

        // 젬 툴팁 (v1.10.1~): 실제 화면 샘플 다섯 장 (가공 중인 영웅 젬은 v1.10.2부터 한 개로 셈)
        void CheckGemTooltips(Reporter& reporter, const std::filesystem::path& reportPath)
        {
            using Models::GemStockKind;
            const auto processedKey = Vision::GemTooltipDetector::ParseValueKey(Vision::GemTooltipDetector::MakeValueKey(2, 1, GemStockKind::STOCK_PROCESSED, "4.99.1"));
            const bool isKey = Vision::GemTooltipDetector::ParseValueKey(Vision::GemTooltipDetector::MakeValueKey(1, 2)) == std::optional{ Vision::GemValueKey{ 1, 2 } }
                && Vision::GemTooltipDetector::ParseValueKey(Vision::GemTooltipDetector::MakeValueKey(1, 2, GemStockKind::STOCK_TRADABLE)) == std::optional{ Vision::GemValueKey{ 1, 2, GemStockKind::STOCK_TRADABLE } }
                && processedKey == std::optional{ Vision::GemValueKey{ 2, 1, GemStockKind::STOCK_PROCESSED, "4.99.1" } }
                && Vision::GemTooltipDetector::ParseValueKey("Gem/1/2") == std::optional{ Vision::GemValueKey{ 1, 2 } }
                && !Vision::GemTooltipDetector::ParseValueKey("Dest/top").has_value() && !Vision::GemTooltipDetector::ParseValueKey("Gem/9/0").has_value()
                && !Vision::GemTooltipDetector::ParseValueKey("Gem/1/2/X").has_value();
            reporter.Check(isKey, "젬 · 값 이름 (귀속 · 거래 가능 · 가공 · 예전 모양 · 범위 밖 · 다른 값 제외)");
            Models::BoundGemData stock;                                                // 종류별 합계
            stock.SetQuantity(0, 0, 5);                                                // 예전에 손으로 넣은 5개 (내역 없음)
            const int afterBound = stock.ApplyScan(0, 0, GemStockKind::STOCK_BOUND, 3, {}, Core::APPConstants::MAX_BOUND_GEM_COUNT);       // 귀속 툴팁 전체 3
            const int afterTradable = stock.ApplyScan(0, 0, GemStockKind::STOCK_TRADABLE, 5, {}, Core::APPConstants::MAX_BOUND_GEM_COUNT); // 거래 가능 툴팁 전체 5 (같은 이름 합계라 더하지 않음)
            const int afterProcessed = stock.ApplyScan(0, 0, GemStockKind::STOCK_PROCESSED, 1, "4.99", Core::APPConstants::MAX_BOUND_GEM_COUNT);   // + 가공 1
            const int afterSame = stock.ApplyScan(0, 0, GemStockKind::STOCK_PROCESSED, 1, "4.99", Core::APPConstants::MAX_BOUND_GEM_COUNT);        // 같은 가공 젬 다시
            const int afterBoundAgain = stock.ApplyScan(0, 0, GemStockKind::STOCK_BOUND, 6, {}, Core::APPConstants::MAX_BOUND_GEM_COUNT);  // 전체 6으로 바뀜
            reporter.Check(afterBound == 3 && afterTradable == 5 && afterProcessed == 6 && afterSame == 6 && afterBoundAgain == 7 && stock.GetQuantity(0, 0) == 7,
                "젬 수량 · 전체 보유 수량(거래 타입 구분 없음) + 가공 젬 (같은 가공 젬은 한 번만)",
                std::format("{} → {} → {} → {} → {}", afterBound, afterTradable, afterProcessed, afterSame, afterBoundAgain));
            const bool isQuantity = Vision::OCRTextParser::ParseQuantity(L": 417개") == 417 && Vision::OCRTextParser::ParseQuantity(L": 41774") == 417
                && Vision::OCRTextParser::ParseQuantity(L": 1774") == 17 && Vision::OCRTextParser::ParseQuantity(L": 1 77h") == 17
                && Vision::OCRTextParser::ParseQuantity(L": 163,439개") == 163439 && Vision::OCRTextParser::ParseQuantity(L": 74개") == 74
                && Vision::OCRTextParser::ParseQuantity(L": 1 7 482개") == 17482 && !Vision::OCRTextParser::ParseQuantity(L": 1 7 482").has_value();
            reporter.Check(isQuantity, "수량 읽기 · '개'를 74 · 7h로 읽은 경우 (417개 → 41774 · 17개 → 1 77h) · 쉼표를 놓친 경우 (1 7 482개)");

            const auto folder = FindSampleFolder(reportPath.parent_path());           // 샘플 폴더
            if (!folder) { reporter.Note("귀속 젬 샘플 검사 건너뜀 (Verify/screens 없음)"); return; }
            struct GemSample
            {
                std::wstring_view m_file;                  // 파일
                std::optional<std::string> m_valueKey;     // 기대 값 이름 (없으면 건너뛰어야 함, "Gem/0/1/P/"처럼 /로 끝나면 앞부분만 비교)
                std::int64_t m_quantity;                   // 기대 수량
            };
            const std::array samples{
                GemSample{ L"gem_order_stable_common_2.png", Vision::GemTooltipDetector::MakeValueKey(0, 0), 2 },
                GemSample{ L"gem_order_solid_common_1.png", Vision::GemTooltipDetector::MakeValueKey(1, 0), 1 },
                GemSample{ L"gem_order_stable_common_417.png", Vision::GemTooltipDetector::MakeValueKey(0, 0), 417 },
                GemSample{ L"gem_order_stable_rare_17.png", Vision::GemTooltipDetector::MakeValueKey(0, 1), 17 },
                GemSample{ L"gem_order_solid_hero_processed.png", Vision::GemTooltipDetector::MakeValueKey(1, 2, Models::GemStockKind::STOCK_PROCESSED, {}), 1 } };   // 질서 안정 · 불변, 고급 · 희귀, 가공 중인 영웅 (한 개)
            std::vector<std::string> outcomes(samples.size());                         // 결과 글자
            std::vector<bool> passes(samples.size());                                  // 통과
            std::thread worker{ [&]                                                    // OCR 일꾼 스레드
                {
                    const Vision::VisionApartment apartment;                           // WinRT 준비
                    const Vision::TextRecognizer recognizer;                           // OCR
                    const Vision::ScreenLayoutStore layouts;                           // 배치 (안 씀)
                    Vision::GemTooltipDetector detector;                               // 젬 인식기
                    for (std::size_t index = 0; index < samples.size(); ++index)       // 샘플마다
                    {
                        const auto frame = Vision::ImageFile::Load(*folder / samples[index].m_file);   // 이미지
                        if (!frame) { outcomes[index] = "파일을 못 읽음"; continue; }
                        const Vision::PixelRect area{ 0, 0, frame->GetWidth(), frame->GetHeight() };   // 툴팁만 있는 그림 전체
                        const auto text = recognizer.Recognize(*frame);                // OCR
                        const Vision::TextResult empty;                                // 실패 때
                        const Vision::DetectorContext context{ layouts, recognizer, text ? *text : empty, area, Vision::PixelPoint{ frame->GetWidth() / 2, frame->GetHeight() / 2 } };
                        const auto result = detector.Detect(*frame, context);          // 읽기
                        const std::optional<Vision::ScanValue> value = result && !result->m_values.empty() ? std::optional{ result->m_values.front() } : std::nullopt;
                        outcomes[index] = value ? std::format("{} = {}", value->m_key, value->m_value) : std::string{ "건너뜀" };
                        const auto isKeyMatched = [&](const std::string& expected)    // 값 이름 비교 (가공 젬은 구분 글자 앞까지)
                            {
                                if (!value) return false;
                                if (expected.ends_with('/')) return value->m_key.starts_with(expected) && value->m_key.size() > expected.size();
                                return value->m_key == expected;
                            };
                        passes[index] = samples[index].m_valueKey ? (isKeyMatched(*samples[index].m_valueKey) && value->m_value == samples[index].m_quantity) : !value;
                    }
                } };
            worker.join();                                                             // 기다림
            for (std::size_t index = 0; index < samples.size(); ++index)
                reporter.Check(passes[index], "귀속 젬 샘플", std::format("{} : {}", std::string(samples[index].m_file.begin(), samples[index].m_file.end()), outcomes[index]));
        }

        // 설정 탭 부품 · 요소 투명도 · 프로그램 아이콘 (v1.9.3)
        void CheckOverlaySetup(Reporter& reporter)
        {
            // 값 막대: 10 ~ 100 · 5 단위, 가로 100 + 손잡이 여백
            constexpr double SLIDER_MIN{ 10.0 };                                           // 최솟값
            constexpr double SLIDER_MAX{ 100.0 };                                          // 최댓값
            constexpr double SLIDER_STEP{ 5.0 };                                           // 단위
            constexpr float SLIDER_LEFT{ 0.0f };                                           // 부품 왼쪽
            constexpr float SLIDER_RIGHT{ 114.0f };                                        // 부품 오른쪽 (손잡이 반지름 7 × 2 + 막대 100)
            double committed = 0.0;                                                        // 놓을 때 받은 값
            Views::UI::Slider slider{ SLIDER_MIN, SLIDER_MAX, SLIDER_STEP, nullptr, [&committed](double value) { committed = value; } };
            slider.SetBounds(Core::RectF{ SLIDER_LEFT, 0.0f, SLIDER_RIGHT, 32.0f });       // 위치
            const bool isEnds = slider.ValueFromX(-50.0f) == SLIDER_MIN && slider.ValueFromX(500.0f) == SLIDER_MAX;   // 범위 밖은 끝 값
            const bool isMiddle = slider.ValueFromX(7.0f + 50.0f) == 55.0;                 // 가운데 = 55
            const bool isStep = slider.ValueFromX(7.0f + 51.0f) == 55.0 && slider.ValueFromX(7.0f + 54.0f) == 60.0;   // 5 단위 반올림
            slider.SetValue(83.0);                                                         // 코드로 바꾸기 (85로 맞춤)
            const bool isNormalized = slider.GetValue() == 85.0;
            const bool isKey = slider.OnKeyDown(Core::KeyEvent{ Core::Key::KEY_RIGHT }) && slider.GetValue() == 90.0 && committed == 90.0;   // → 키 = 한 칸 + 저장
            reporter.Check(isEnds && isMiddle && isStep && isNormalized && isKey, "설정 탭 · 값 막대 (범위 · 5 단위 · 키 · 놓을 때 저장)");

            // 요소 투명도 (패널 전체, 10% ~ 100%)
            Views::CharacterPanel panel{ nullptr, nullptr, nullptr };                      // 아무 패널
            panel.SetAlpha(0.0f);                                                          // 너무 작음
            const bool isLow = std::abs(panel.GetAlpha() - 0.1f) < 0.001f;                 // 10%로
            panel.SetAlpha(2.0f);                                                          // 너무 큼
            const bool isHigh = std::abs(panel.GetAlpha() - 1.0f) < 0.001f;                // 100%로
            reporter.Check(isLow && isHigh, "설정 탭 · 요소 투명도 범위 (10% ~ 100%)");

            // 프로그램 아이콘 (exe 리소스 → 메뉴 로고 그림)
            constexpr int ICON_PIXELS{ 64 };                                               // 읽을 크기
            const auto icon = Views::MainWindow::LoadProgramIconImage(ICON_PIXELS);        // 아이콘 그림
            const bool hasOpaque = icon && std::ranges::any_of(std::views::iota(std::size_t{ 0 }, icon->m_pixels.size() / 4),
                [&icon](std::size_t index) { return icon->m_pixels[index * 4 + 3] != 0; }); // 보이는 픽셀이 있음
            reporter.Check(icon && icon->m_width == ICON_PIXELS && icon->m_height == ICON_PIXELS && hasOpaque, "프로그램 아이콘 (exe 리소스 64×64)");
        }

        // 미니 모드 패널: ◀ 처음 → 끝, ▶ 끝 → 처음 순환 + 안내 탭은 입력을 받지 않음 (창 없이 패널만)
        void CheckMiniToolPanel(Reporter& reporter)
        {
            constexpr float PANEL_WIDTH{ 1000.0f };          // 검사 패널 가로
            constexpr float PANEL_HEIGHT{ 640.0f };          // 검사 패널 세로
            constexpr float PREVIOUS_X{ 15.0f };             // ◀ 단추 안 X (안쪽 여백 10 + 5)
            constexpr float NEXT_X{ PANEL_WIDTH - 15.0f };   // ▶ 단추 안 X
            constexpr float SWITCHER_Y{ 40.0f };             // 탭 고르기 줄 안 Y (머리말 28 + 여백 5 + 7)
            constexpr std::size_t TOOL_COUNT{ 3 };           // 검사 탭 수
            constexpr std::size_t LAST_INDEX{ TOOL_COUNT - 1 };   // 마지막 탭

            std::array<Views::OverlayPlaceholderTool, TOOL_COUNT> tools;                   // 가짜 탭들 (안내 탭 재사용)
            const std::array<std::reference_wrapper<Core::ToolManager>, TOOL_COUNT> toolRefs{ tools[0], tools[1], tools[2] };   // 탭 목록
            std::vector<std::size_t> changes;                                              // 바뀐 탭 번호 기록
            Views::MiniToolPanel panel{ toolRefs, 0, [&changes](std::size_t index) { changes.push_back(index); } };   // 첫 탭부터
            panel.SetBounds(Core::RectF{ 0.0f, 0.0f, PANEL_WIDTH, PANEL_HEIGHT });         // 위치 · 크기

            const auto click = [&panel](float x, float y)                                  // 왼쪽 누르고 떼기
                {
                    (void)panel.HandleInput(Core::MouseButtonEvent{ x, y, Core::MouseButton::MOUSE_BUTTON_LEFT, true, 1 }, true);
                    (void)panel.HandleInput(Core::MouseButtonEvent{ x, y, Core::MouseButton::MOUSE_BUTTON_LEFT, false, 1 }, true);
                };
            click(PREVIOUS_X, SWITCHER_Y);                                                 // ◀ (처음 → 끝)
            const bool isWrappedBack = panel.GetToolIndex() == LAST_INDEX;                 // 끝으로 갔는지
            click(NEXT_X, SWITCHER_Y);                                                     // ▶ (끝 → 처음)
            const bool isWrappedForward = panel.GetToolIndex() == 0;                       // 처음으로 갔는지
            const bool isNotified = changes == std::vector<std::size_t>{ LAST_INDEX, 0 }; // 바꿀 때마다 알림
            const bool isPlaceholderPassive = !tools[0].HandleInput(Core::KeyEvent{ Core::Key::KEY_ENTER });   // 안내 탭은 입력을 안 받음
            reporter.Check(isWrappedBack && isWrappedForward && isNotified && isPlaceholderPassive, "인게임 오버레이 · 미니 모드 탭 순환",
                std::format("◀ {} · ▶ {} · 알림 {}개", isWrappedBack, isWrappedForward, changes.size()));
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
            CheckVisionSamples(reporter, reportPath);            // 화면 인식 실제 화면 샘플
            CheckCharacterMatching(reporter);                    // 현재 캐릭터 이름 맞추기
            CheckCharacterSamples(reporter, reportPath);         // 현재 캐릭터 실제 화면 샘플
            CheckOverlay(reporter);                              // 인게임 오버레이 투명 창
            CheckMiniToolPanel(reporter);                        // 인게임 오버레이 미니 모드
            CheckOverlaySetup(reporter);                         // 설정 탭 부품 · 요소 투명도 · 아이콘
            CheckBossHealth(reporter, reportPath);               // 보스 체력바
            CheckGemTooltips(reporter, reportPath);              // 귀속 젬 툴팁
            CheckCombatAnalyzer(reporter, reportPath);           // 전투 분석기 · 딜 지분
            CheckMaterialSlots(reporter, reportPath);            // 창고 · 소지품 슬롯 · 값 붙잡기
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
