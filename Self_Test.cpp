module;

#define NOMINMAX
#include <Windows.h>									// GetModuleFileNameW, GetFileVersionInfoW, VerQueryValueW

#pragma comment(lib, "version.lib")					// 버전 정보 API

module LostArkMultiTool.Core.SelfTest;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.CombatRegression;
import LostArkMultiTool.Models.CombatCoefficientData;
import LostArkMultiTool.Models.RefineData;

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
            reporter.Check(result.m_failures.empty(), "lopec 회귀 · 아크 패시브", std::format("{}명 · 6랭크 {}줄 최대 {:.3f}% · 덜 찬 랭크 {}줄 최대 {:.3f}% · 슬라이더 기준 보정 {}명 · 제외 {}줄 · 초과 {}줄",
                result.m_characterCount, result.m_fullRankRowCount, result.m_worstFullRankPercent, result.m_partialRankRowCount, result.m_worstPartialRankPercent,
                result.m_offsetCharacterCount, result.m_exceptionCount, result.m_failures.size()));   // 요약 한 줄
            for (const CombatRegression::RegressionFailure& failure : result.m_failures | std::views::take(MAX_REPORTED_FAILURES))   // 오차 큰 줄부터
            {
                reporter.Note(std::format("초과 {} {} ({}) : {:.3f}% (허용 {}%)", failure.m_character, failure.m_control, failure.m_change, failure.m_errorPercent, failure.m_limitPercent));
            }
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
            CheckLopecRegression(reporter, reportPath);          // lopec 회귀
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
