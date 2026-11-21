module LostArkMultiTool.Core.CombatRegression;

import std;
import LostArkMultiTool.Core.CombatExtraComponent;
import LostArkMultiTool.Models.CharacterArmory;
import LostArkMultiTool.Models.CombatExtraState;
import LostArkMultiTool.Models.CombatState;
import LostArkMultiTool.Services.ArmoryParser;
import LostArkMultiTool.Services.JSONValue;

namespace LostArkMultiTool::Core::CombatRegression
{
    namespace
    {
        using Services::JSONType;    // JSON 값 종류
        using Services::JSONValue;   // JSON 값

        constexpr std::string_view VERIFY_FOLDER{ "Verify" };                    // 측정 데이터 최상위 폴더 (gitignore)
        constexpr std::string_view RESULTS_FOLDER{ "lopec_results" };            // lopec CSV 폴더
        constexpr std::string_view REGRESSION_FOLDER{ "regression" };            // 회귀 검사 보조 폴더
        constexpr std::string_view ARMORY_FOLDER{ "armory" };                    // 캐릭터 무기고 응답 (tools/Fetch-RegressionArmory.py가 저장)
        constexpr std::string_view DETAIL_FILE{ "last_result.csv" };            // 줄마다 예측 · lopec · 오차 (원인 분석용, 실행마다 덮어씀)
        constexpr std::string_view DETAIL_HEADER{ "캐릭터,랭크,컨트롤,변경 전,변경 후,lopec 배율,lopec 기준 보정,엔진 배율,오차(%),허용(%)\n" };   // 상세 파일 머리글
        constexpr std::string_view EXCEPTIONS_FILE{ "exceptions.json" };         // lopec 특이점 제외 목록 {"이름" 또는 "*": ["컨트롤", ...]}
        constexpr std::string_view ALL_RESULTS_FILE{ "lopec_all.csv" };          // 모든 lopec 측정을 모은 한 파일 (tools/Crawl-LopecSimulator.py · Merge-LopecResults.py)
        constexpr std::string_view JSON_EXTENSION{ ".json" };                    // JSON 확장자
        constexpr std::string_view UTF8_BOM{ "\xEF\xBB\xBF" };                    // UTF-8 BOM
        constexpr std::string_view ALL_CHARACTERS{ "*" };                         // 제외 목록에서 모든 캐릭터

        constexpr std::string_view COLUMN_CHARACTER{ "캐릭터" };                  // CSV 열: 캐릭터 이름
        constexpr std::string_view COLUMN_GROUP{ "계통" };                        // CSV 열: 계통
        constexpr std::string_view COLUMN_CONTROL{ "컨트롤" };                    // CSV 열: lopec 컨트롤 id
        constexpr std::string_view COLUMN_BEFORE{ "변경 전" };                    // CSV 열: 변경 전 값
        constexpr std::string_view COLUMN_AFTER{ "변경 후" };                     // CSV 열: 변경 후 값 (조합이면 "키=값 | 키=값")
        constexpr std::string_view COLUMN_MEASURE{ "측정" };                      // CSV 열: 측정 회차 (YYYY-MM-DD_HHMMSS, 글자 순 = 시간 순)
        constexpr std::string_view COLUMN_BASE_COMBAT{ "기준 전투력" };          // CSV 열: 측정 기준 전투력 (lopec 예상 전투력, API와 비교)
        constexpr double BASELINE_TOLERANCE{ 0.01 };                              // 측정 기준 전투력이 API와 이 비율 넘게 다르면 lopec 상태가 꼬인 회차로 봄
        constexpr std::string_view COLUMN_RATIO{ "전투력 배율" };                 // CSV 열: lopec 예상 전투력 ÷ 기준 전투력

        constexpr std::string_view GROUP_PASSIVE{ "ARK_PASSIVE" };                // 아크 패시브 단일 변경
        constexpr std::string_view GROUP_PASSIVE_COMBO{ "COMBO_ARK_PASSIVE" };    // 아크 패시브 조합 변경
        constexpr std::string_view COMBO_CONTROL{ "COMBO" };                      // 조합 줄의 컨트롤 이름 (제외 목록 · 보고용)
        constexpr std::string_view CONTROL_ENLIGHTENMENT{ "enlightenment-point" };   // 깨달음 포인트 구성
        constexpr std::string_view CONTROL_RANK{ "rankControl" };                    // 랭크 (세 트리 모두)
        constexpr std::string_view CONTROL_LEVEL{ "levelControl" };                  // 레벨 (세 트리 모두)
        constexpr std::string_view CONTROL_TREE_LEVEL{ "ark-passive-level-" };       // 트리 레벨 (뒤에 0 진화 · 1 깨달음 · 2 도약)
        constexpr std::string_view COMBO_SEPARATOR{ " | " };                         // 조합 항목 구분
        constexpr char COMBO_ASSIGN{ '=' };                                          // 조합 항목 키 = 값
        constexpr char RANK_SEPARATOR{ '/' };                                        // 상세 파일의 트리별 랭크 구분 (진화/깨달음/도약)
        constexpr char CSV_DELIMITER{ ',' };                                      // CSV 구분자
        constexpr char CSV_QUOTE{ '"' };                                             // CSV 따옴표
        constexpr char LINE_FEED{ '\n' };                                            // 줄바꿈
        constexpr char CARRIAGE_RETURN{ '\r' };                                      // 줄바꿈 (윈도우)

        constexpr double FULL_RANK_MAX_ERROR_PERCENT{ 0.1 };      // 6랭크 캐릭터 허용 최대 오차 (%)
        constexpr double PARTIAL_RANK_MAX_ERROR_PERCENT{ 0.8 };   // 랭크가 덜 찬 캐릭터 허용 최대 오차 (%) (랭크 0 부캐 약 0.77%)
        constexpr double SUPPORTER_MAX_ERROR_PERCENT{ 0.4 };      // 서포터 허용 최대 오차 (%) (전투력 9천 이상 서포터 0.14% 이내, 6천 ~ 7천대 서포터 최대 0.37% · 장비 수준별 차이는 아직 식에 없음)
        constexpr double PERCENT{ 100.0 };                        // 비율 → %
        constexpr std::size_t EVOLUTION_TREE{ 0 };                // 아크 패시브 트리 번호: 0 진화 (딜러는 레벨 효과 없음, lopec 기준 어긋남 측정용)
        constexpr std::size_t LEAP_TREE{ 2 };                     // 아크 패시브 트리 번호: 2 도약 (서포터는 레벨 효과 없음, 서포터 기준 어긋남 측정용)
        constexpr double ROLE_FLAT_SPREAD{ 1e-4 };                // 도약 레벨 슬라이더 줄의 lopec 배율 폭이 이 값 이하면 측정 당시 서포터 (서포터는 도약 레벨 효과 없음)
        constexpr std::size_t MIN_ROLE_ROWS{ 3 };                 // 측정 당시 역할을 판단할 최소 도약 슬라이더 줄 수
        constexpr double COMBO_OFFSET_SPREAD_PERCENT{ 0.3 };      // 조합 줄 lopec ÷ 엔진 비가 이 폭(%) 안에서 일정해야 조합 기준 어긋남으로 봄
        constexpr std::size_t MIN_OFFSET_ROWS{ 3 };               // lopec 슬라이더 기준 어긋남을 판단할 최소 줄 수
        constexpr double OFFSET_SPREAD_PERCENT{ 0.05 };           // 이 폭(%) 안에서 일정해야 기준 어긋남으로 봄 (엔진 기울기 오차는 값에 따라 달라짐, 6랭크 허용치의 절반)
        constexpr double OFFSET_MIN_PERCENT{ 0.05 };              // 이보다 작은 어긋남은 반올림 차이로 보고 보정하지 않음 (%)
        constexpr std::string_view NOTE_SEPARATOR{ ", " };         // 안내 문구 항목 구분

        using CsvRow = std::vector<std::string>;   // CSV 한 줄의 칸들

        // 열 이름 → 번호
        struct CsvColumns
        {
            std::size_t m_character{};   // 캐릭터
            std::size_t m_group{};       // 계통
            std::size_t m_control{};     // 컨트롤
            std::size_t m_before{};      // 변경 전
            std::size_t m_after{};       // 변경 후
            std::size_t m_ratio{};       // 전투력 배율
            std::size_t m_measure{};     // 측정 회차
            std::optional<std::size_t> m_baseCombat;   // 기준 전투력 (예전 형식에는 없음)
        };

        // 한 캐릭터의 가장 최근 측정 회차
        struct CharacterMeasure
        {
            std::string m_measure;         // 측정 회차 (YYYY-MM-DD_HHMMSS, 글자 순 = 시간 순)
            std::vector<CsvRow> m_rows;    // 아크 패시브 측정 줄
            CsvColumns m_columns;          // 열 번호
        };

        // UTF-8 std::string → 경로 (한글 이름이 ANSI 코드 페이지로 깨지지 않게)
        [[nodiscard]] std::filesystem::path ToPath(std::string_view text)
        {
            return std::filesystem::path{ std::u8string_view{ reinterpret_cast<const char8_t*>(text.data()), text.size() } };   // char8_t로 해석
        }

        // 경로 → UTF-8 std::string
        [[nodiscard]] std::string ToText(const std::filesystem::path& path)
        {
            const std::u8string text{ path.u8string() };                                          // UTF-8 바이트
            return std::string{ reinterpret_cast<const char*>(text.data()), text.size() };         // char로 복사
        }

        // 파일 전체를 읽고 앞의 BOM을 뗌 (못 열면 nullopt)
        [[nodiscard]] std::optional<std::string> ReadText(const std::filesystem::path& path)
        {
            std::ifstream file{ path, std::ios::binary };                                          // 바이너리 그대로
            if (!file) return std::nullopt;                                                        // 열기 실패
            std::string text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };   // 전체 읽기
            if (text.starts_with(UTF8_BOM)) text.erase(0, UTF8_BOM.size());                        // BOM 제거
            return text;                                                                           // 본문
        }

        // CSV 본문 → 줄 목록 (따옴표 안의 쉼표 · 줄바꿈 · 겹따옴표 처리)
        [[nodiscard]] std::vector<CsvRow> ParseCsv(std::string_view text)
        {
            std::vector<CsvRow> rows;        // 결과
            CsvRow row;                      // 만드는 중인 줄
            std::string field;               // 만드는 중인 칸
            bool isQuoted{};                 // 따옴표 안인지
            for (std::size_t index{}; index < text.size(); ++index)
            {
                const char letter{ text[index] };                                                  // 현재 글자
                if (isQuoted)
                {
                    if (letter != CSV_QUOTE) { field += letter; continue; }                        // 따옴표 안의 일반 글자
                    const bool isEscaped{ index + 1 < text.size() && text[index + 1] == CSV_QUOTE };   // "" = 따옴표 한 개
                    if (isEscaped) { field += CSV_QUOTE; ++index; }                                // 겹따옴표
                    else isQuoted = false;                                                         // 따옴표 끝
                    continue;
                }
                if (letter == CSV_QUOTE) isQuoted = true;                                          // 따옴표 시작
                else if (letter == CSV_DELIMITER) { row.push_back(std::move(field)); field.clear(); }   // 칸 끝
                else if (letter == LINE_FEED)                                                      // 줄 끝
                {
                    row.push_back(std::move(field)); field.clear();                                // 마지막 칸
                    rows.push_back(std::move(row)); row.clear();                                   // 줄 추가
                }
                else if (letter != CARRIAGE_RETURN) field += letter;                               // 일반 글자 (CR은 버림)
            }
            if (!field.empty() || !row.empty()) { row.push_back(std::move(field)); rows.push_back(std::move(row)); }   // 줄바꿈 없이 끝난 마지막 줄
            return rows;                                                                           // 결과
        }

        // 머리글에서 열 번호 찾기 (필요한 열이 하나라도 없으면 nullopt → 예전 형식 파일)
        [[nodiscard]] std::optional<CsvColumns> FindColumns(const CsvRow& header)
        {
            const auto find{ [&header](std::string_view name) -> std::optional<std::size_t>
            {
                const auto found{ std::ranges::find(header, name) };                               // 열 이름 검색
                if (found == header.end()) return std::nullopt;                                    // 없음
                return static_cast<std::size_t>(std::distance(header.begin(), found));             // 번호
            } };
            const std::array columns{ find(COLUMN_CHARACTER), find(COLUMN_GROUP), find(COLUMN_CONTROL), find(COLUMN_BEFORE), find(COLUMN_AFTER), find(COLUMN_RATIO), find(COLUMN_MEASURE) };   // 모든 열
            if (std::ranges::any_of(columns, [](const std::optional<std::size_t>& column) { return !column.has_value(); })) return std::nullopt;   // 빠진 열
            return CsvColumns{ *columns[0], *columns[1], *columns[2], *columns[3], *columns[4], *columns[5], *columns[6], find(COLUMN_BASE_COMBAT) };   // 순서대로 (기준 전투력은 있으면)
        }

        // 칸 값 (줄이 짧으면 빈 글자)
        [[nodiscard]] std::string_view GetField(const CsvRow& row, std::size_t column) noexcept
        {
            return column < row.size() ? std::string_view{ row[column] } : std::string_view{};      // 경계 검사
        }

        // CSV 칸 하나 (쉼표 · 따옴표가 있으면 따옴표로 감쌈)
        [[nodiscard]] std::string QuoteCsv(std::string_view text)
        {
            if (text.find_first_of(",\"\n") == std::string_view::npos) return std::string{ text };   // 그대로
            std::string quoted{ CSV_QUOTE };                                                       // 여는 따옴표
            for (const char letter : text) { if (letter == CSV_QUOTE) quoted += CSV_QUOTE; quoted += letter; }   // 따옴표는 두 번
            return quoted + CSV_QUOTE;                                                             // 닫는 따옴표
        }

        // 글자 → 수 (전체가 수가 아니면 nullopt)
        template <typename T>
        [[nodiscard]] std::optional<T> ParseNumber(std::string_view text) noexcept
        {
            T value{};                                                                             // 결과
            const auto [end, error]{ std::from_chars(text.data(), text.data() + text.size(), value) };   // 변환
            if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;     // 실패 · 남은 글자
            return value;                                                                          // 수
        }

        // 측정 한 파일(lopec_all.csv) → 캐릭터별 가장 최근 회차의 아크 패시브 측정
        [[nodiscard]] std::map<std::string, std::vector<CharacterMeasure>> LoadMeasures(const std::filesystem::path& dataDirectory)
        {
            std::map<std::string, std::vector<CharacterMeasure>> measures;                         // 캐릭터 → 측정 회차들 (오래된 것부터)
            const std::optional<std::string> text{ ReadText(dataDirectory / ToPath(ALL_RESULTS_FILE)) };   // 본문
            if (!text.has_value()) return measures;                                                // 파일 없음
            std::vector<CsvRow> rows{ ParseCsv(*text) };                                           // 줄들
            if (rows.empty()) return measures;                                                     // 빈 파일
            const std::optional<CsvColumns> columns{ FindColumns(rows.front()) };                  // 머리글
            if (!columns.has_value()) return measures;                                             // 예전 형식

            std::map<std::pair<std::string, std::string>, std::vector<CsvRow>> passiveRows;        // (캐릭터, 회차) → 아크 패시브 줄
            for (CsvRow& row : rows | std::views::drop(1))                                         // 머리글 다음부터
            {
                const std::string_view group{ GetField(row, columns->m_group) };                   // 계통
                if (group != GROUP_PASSIVE && group != GROUP_PASSIVE_COMBO) continue;              // 아크 패시브만
                std::pair<std::string, std::string> key{ std::string{ GetField(row, columns->m_character) },
                    std::string{ GetField(row, columns->m_measure) } };                            // 캐릭터 · 회차
                passiveRows[std::move(key)].push_back(std::move(row));                             // 회차별로 모음
            }
            for (auto& [key, characterRows] : passiveRows)                                         // 회차 글자 순 = 시간 순으로 돌므로
            {
                measures[key.first].push_back(CharacterMeasure{ key.second, std::move(characterRows), *columns });   // 시간 순으로 쌓음
            }
            return measures;                                                                       // 결과
        }

        // 제외 목록 읽기 (없으면 빈 목록)
        [[nodiscard]] std::map<std::string, std::set<std::string>> LoadExceptions(const std::filesystem::path& path)
        {
            std::map<std::string, std::set<std::string>> exceptions;                               // 캐릭터 → 컨트롤들
            const std::optional<std::string> text{ ReadText(path) };                               // 본문
            if (!text.has_value()) return exceptions;                                              // 파일 없음
            const JSONValue root{ Services::ParseJSONText(*text) };                                // 형식이 잘못되면 예외 → 자체 검사 실패로 보고
            for (const auto& [character, controls] : std::views::zip(root.GetKeys(), root.GetValues()))
            {
                for (const JSONValue& control : controls.GetItems()) exceptions[character].insert(control.GetString());   // 컨트롤 기록
            }
            return exceptions;                                                                     // 결과
        }

        // 제외 대상인지 (캐릭터 · 모든 캐릭터 공통)
        [[nodiscard]] bool IsException(const std::map<std::string, std::set<std::string>>& exceptions, const std::string& character, std::string_view control)
        {
            const auto contains{ [&exceptions, control](const std::string& key)
            {
                const auto found{ exceptions.find(key) };                                          // 캐릭터 항목
                return found != exceptions.end() && found->second.contains(std::string{ control });   // 컨트롤 포함 여부
            } };
            return contains(character) || contains(std::string{ ALL_CHARACTERS });                 // 캐릭터 또는 공통
        }

        // lopec 컨트롤 하나를 목표 상태에 반영 (모르는 컨트롤 · 잘못된 값이면 false)
        [[nodiscard]] bool ApplyControl(Models::ArkPassiveState& passive, std::string_view control, std::string_view value)
        {
            if (control == CONTROL_ENLIGHTENMENT) { passive.m_enlightenment = std::string{ value }; return true; }   // 깨달음 구성
            const std::optional<int> number{ ParseNumber<int>(value) };                            // 랭크 · 레벨 값
            if (!number.has_value()) return false;                                                 // 수가 아님
            if (control == CONTROL_RANK)                                                           // 랭크: 앱 편집기처럼 세 트리를 같은 값으로
            {
                passive.m_rank = *number;                                                          // 표시 랭크
                passive.m_treeRanks.fill(*number);                                                 // 트리별 랭크
                return true;
            }
            if (control == CONTROL_LEVEL) { passive.m_levels.fill(*number); return true; }         // 세 트리 레벨
            if (!control.starts_with(CONTROL_TREE_LEVEL)) return false;                            // 모르는 컨트롤
            const std::optional<std::size_t> tree{ ParseNumber<std::size_t>(control.substr(CONTROL_TREE_LEVEL.size())) };   // 트리 번호
            if (!tree.has_value() || *tree >= passive.m_levels.size()) return false;               // 범위 밖
            passive.m_levels[*tree] = *number;                                                     // 트리 레벨
            return true;
        }

        // 측정 한 줄을 목표 상태에 반영 (단일 컨트롤 또는 "키=값 | 키=값" 조합)
        [[nodiscard]] bool ApplyRow(Models::ArkPassiveState& passive, const CsvRow& row, const CsvColumns& columns)
        {
            const std::string_view after{ GetField(row, columns.m_after) };                        // 변경 후
            if (GetField(row, columns.m_group) == GROUP_PASSIVE) return ApplyControl(passive, GetField(row, columns.m_control), after);   // 단일 변경
            for (const auto part : std::views::split(after, COMBO_SEPARATOR))                      // 조합 항목 순서대로 (levelControl 뒤의 트리 레벨이 덮어씀)
            {
                const std::string_view item{ part.begin(), part.end() };                           // 키=값
                const std::size_t assign{ item.find(COMBO_ASSIGN) };                               // = 위치
                if (assign == std::string_view::npos || !ApplyControl(passive, item.substr(0, assign), item.substr(assign + 1))) return false;   // 잘못된 항목
            }
            return true;
        }

        // CSV 기준 상태(변경 전 값)의 트리 레벨이 지금 무기고와 같은지 (다르면 측정 뒤 캐릭터가 바뀐 것)
        [[nodiscard]] std::optional<std::string> FindStaleLevel(const CharacterMeasure& measure, const Models::ArkPassiveState& passive)
        {
            for (const CsvRow& row : measure.m_rows)
            {
                const std::string_view control{ GetField(row, measure.m_columns.m_control) };      // 컨트롤
                if (!control.starts_with(CONTROL_TREE_LEVEL)) continue;                            // 트리 레벨 줄만
                const std::optional<std::size_t> tree{ ParseNumber<std::size_t>(control.substr(CONTROL_TREE_LEVEL.size())) };   // 트리 번호
                const std::optional<int> before{ ParseNumber<int>(GetField(row, measure.m_columns.m_before)) };   // 측정 당시 레벨
                if (!tree.has_value() || !before.has_value() || *tree >= passive.m_levels.size()) continue;   // 읽을 수 없는 줄
                if (passive.m_levels[*tree] != *before) return std::format("트리 {} 레벨 {} → 지금 {}", *tree, *before, passive.m_levels[*tree]);   // 바뀐 레벨
            }
            return std::nullopt;                                                                   // 같음
        }

        // 측정 당시 깨달음 구성 (enlightenment-point 줄의 변경 전, 없으면 최대 구성)
        [[nodiscard]] std::string FindBaselineEnlightenment(const CharacterMeasure& measure)
        {
            for (const CsvRow& row : measure.m_rows)
            {
                if (GetField(row, measure.m_columns.m_control) == CONTROL_ENLIGHTENMENT) return std::string{ GetField(row, measure.m_columns.m_before) };   // 변경 전 구성
            }
            return std::string{ Models::FULL_ENLIGHTENMENT };                                      // 측정 줄이 없으면 최대
        }

        // 무기고 파일 → 앱 해석기로 만든 현재 상태 (실패하면 이유)
        // 무기고 응답의 API 전투력 (없거나 읽을 수 없으면 nullopt)
        [[nodiscard]] std::optional<double> LoadApiPower(const std::filesystem::path& path)
        {
            const std::optional<std::string> text{ ReadText(path) };                               // 무기고 응답
            if (!text.has_value()) return std::nullopt;                                            // 아직 안 받음
            try
            {
                return Services::ParseArmory(Services::ParseJSONText(*text)).m_combatPower;       // 실제 앱 해석기로 전투력
            }
            catch (const std::exception&)
            {
                return std::nullopt;                                                               // 해석 실패
            }
        }

        // 측정 회차의 기준 전투력 (열이 없거나 비면 nullopt)
        [[nodiscard]] std::optional<double> GetMeasureBaseline(const CharacterMeasure& measure)
        {
            if (!measure.m_columns.m_baseCombat.has_value()) return std::nullopt;                  // 예전 형식
            for (const CsvRow& row : measure.m_rows)                                               // 첫 숫자 줄
            {
                if (const std::optional<double> value{ ParseNumber<double>(GetField(row, *measure.m_columns.m_baseCombat)) }) return value;   // 기준 전투력
            }
            return std::nullopt;                                                                   // 없음
        }

        // API 전투력과 기준이 맞는 가장 최근 회차 (API · 기준을 모르거나 맞는 회차가 없으면 가장 최근 회차 = 측정 뒤 성장한 캐릭터)
        [[nodiscard]] const CharacterMeasure* SelectMeasure(const std::vector<CharacterMeasure>& measures, std::optional<double> apiPower)
        {
            for (const CharacterMeasure& measure : measures | std::views::reverse)                 // 최근 회차부터
            {
                const std::optional<double> baseline{ GetMeasureBaseline(measure) };               // 측정 기준 전투력
                if (!apiPower.has_value() || !baseline.has_value() || *apiPower <= 0.0) return &measure;   // 비교할 수 없으면 최근 회차
                if (std::abs(*baseline / *apiPower - 1.0) <= BASELINE_TOLERANCE) return &measure;  // 기준이 API와 맞는 회차
            }
            return measures.empty() ? nullptr : &measures.back();                                  // 맞는 회차가 없으면 가장 최근 회차
        }

        [[nodiscard]] std::expected<Models::CombatState, std::string> LoadCurrentState(const std::filesystem::path& path)
        {
            const std::optional<std::string> text{ ReadText(path) };                               // 무기고 응답
            if (!text.has_value()) return std::unexpected{ std::string{ "무기고 파일 없음 (tools/Fetch-RegressionArmory.py 실행 필요)" } };   // 아직 안 받음
            try
            {
                const Models::CharacterArmory armory{ Services::ParseArmory(Services::ParseJSONText(*text)) };   // 실제 앱 해석기
                Models::CombatState state;                                                         // 전투력 상태
                state.m_extra = armory.m_extra;                                                    // 시뮬레이터와 같은 방식으로 복사
                return state;                                                                      // 현재 상태
            }
            catch (const std::exception& exception)
            {
                return std::unexpected{ std::string{ exception.what() } };                         // 해석 실패
            }
        }

        // 다시 계산한 측정 한 줄
        struct RowResult
        {
            std::string m_control;                 // 컨트롤 (조합이면 COMBO)
            std::vector<std::string> m_sliders;    // 건드린 레벨 슬라이더 (levelControl · ark-passive-level-N)
            std::string m_before;                  // 변경 전
            std::string m_after;                   // 변경 후
            double m_lopecRatio{};                 // lopec 배율
            double m_predicted{};                  // 엔진 예측 배율
        };

        // 레벨 슬라이더 컨트롤인지 (levelControl · ark-passive-level-N)
        [[nodiscard]] bool IsLevelSlider(std::string_view control) noexcept
        {
            return control == CONTROL_LEVEL || control.starts_with(CONTROL_TREE_LEVEL);            // 레벨 컨트롤들
        }

        // 측정 한 줄이 건드린 레벨 슬라이더 (조합이면 "키=값"의 키 중 슬라이더)
        [[nodiscard]] std::vector<std::string> FindSliders(const CsvRow& row, const CsvColumns& columns)
        {
            std::vector<std::string> sliders;                                                      // 결과
            if (GetField(row, columns.m_group) == GROUP_PASSIVE)                                   // 단일 변경
            {
                const std::string_view control{ GetField(row, columns.m_control) };                // 컨트롤
                if (IsLevelSlider(control)) sliders.emplace_back(control);                         // 슬라이더면 기록
                return sliders;
            }
            for (const auto part : std::views::split(GetField(row, columns.m_after), COMBO_SEPARATOR))   // 조합 항목
            {
                const std::string_view item{ part.begin(), part.end() };                           // 키=값
                const std::string_view key{ item.substr(0, item.find(COMBO_ASSIGN)) };             // 키
                if (IsLevelSlider(key)) sliders.emplace_back(key);                                 // 슬라이더면 기록
            }
            return sliders;
        }

        // 오프셋 하나를 적용했을 때 이 슬라이더 줄들의 최대 오차 (%)
        [[nodiscard]] double GetWorstError(const std::vector<const RowResult*>& rows, double offset) noexcept
        {
            double worst{};                                                                        // 최대 오차
            for (const RowResult* row : rows) worst = std::max(worst, std::abs(row->m_predicted * offset / row->m_lopecRatio - 1.0) * PERCENT);   // 보정한 lopec 대비 오차
            return worst;
        }

        // lopec 레벨 슬라이더 기준 어긋남 (슬라이더 → 배수)
        // 1) 진화 레벨은 엔진 · lopec 모두 효과가 없어야 하는데, 진화 슬라이더를 건드린 lopec 값이 값과 상관없이 일정하게 1이 아니면 lopec 쪽 기준 차이 → 그 배수를 잼
        // 2) 다른 슬라이더는 그 배수를 적용할지 말지 두 가지 중 오차가 작은 쪽을 고름 (lopec이 슬라이더마다 기준 차이를 넣거나 안 넣음, 배수를 새로 맞추지는 않음)
        [[nodiscard]] std::map<std::string, double> FindSliderOffsets(const std::vector<RowResult>& rows, std::size_t referenceTree)
        {
            std::map<std::string, std::vector<const RowResult*>> sliderRows;                      // 슬라이더 하나만 바꾼 줄들
            for (const RowResult& row : rows)
            {
                if (row.m_control != COMBO_CONTROL && row.m_sliders.size() == 1 && row.m_predicted > 0.0) sliderRows[row.m_control].push_back(&row);   // 단일 슬라이더 줄
            }
            const std::string evolutionSlider{ std::format("{}{}", CONTROL_TREE_LEVEL, referenceTree) };   // 기준 슬라이더 (딜러 진화 · 서포터 도약: 엔진 효과가 없는 트리)
            const auto evolution{ sliderRows.find(evolutionSlider) };                              // 진화 슬라이더 줄
            if (evolution == sliderRows.end() || evolution->second.size() < MIN_OFFSET_ROWS) return {};   // 판단할 줄이 부족

            std::vector<double> ratios{ evolution->second | std::views::transform([](const RowResult* row) { return row->m_lopecRatio / row->m_predicted; }) | std::ranges::to<std::vector>() };   // lopec ÷ 엔진
            std::ranges::sort(ratios);                                                             // 중앙값 · 폭 계산용
            const double offset{ ratios[ratios.size() / 2] };                                      // 중앙값 = 기준 어긋남 배수
            const double spread{ (ratios.back() - ratios.front()) / offset * PERCENT };           // 값에 따른 폭 (%)
            if (spread > OFFSET_SPREAD_PERCENT || std::abs(offset - 1.0) * PERCENT <= OFFSET_MIN_PERCENT) return {};   // 일정하지 않거나 반올림 수준이면 보정 없음

            std::map<std::string, double> offsets;                                                 // 결과
            for (const auto& [slider, controlRows] : sliderRows)
            {
                if (GetWorstError(controlRows, offset) < GetWorstError(controlRows, 1.0)) offsets[slider] = offset;   // 배수를 적용하는 쪽이 더 맞으면 적용
            }
            return offsets;
        }

        // lopec 조합 줄 기준 어긋남 (조합 줄 모두 lopec ÷ 엔진이 일정하게 1이 아니면 lopec 조합 기준 차이 → 그 배수, 없으면 1)
        [[nodiscard]] double FindComboOffset(const std::vector<RowResult>& rows)
        {
            std::vector<const RowResult*> comboRows;                                               // 조합 줄들
            for (const RowResult& row : rows)                                                      // 모든 줄 순회
            {
                if (row.m_control == COMBO_CONTROL && row.m_predicted > 0.0) comboRows.push_back(&row);   // 엔진 예측이 있는 조합 줄만
            }
            if (comboRows.size() < MIN_OFFSET_ROWS) return 1.0;                                    // 판단할 줄이 부족
            std::vector<double> ratios{ comboRows | std::views::transform([](const RowResult* row) { return row->m_lopecRatio / row->m_predicted; }) | std::ranges::to<std::vector>() };   // lopec ÷ 엔진
            std::ranges::sort(ratios);                                                             // 중앙값 · 폭 계산용
            const double offset{ ratios[ratios.size() / 2] };                                      // 중앙값 = 조합 기준 어긋남 배수
            const double spread{ (ratios.back() - ratios.front()) / offset * PERCENT };           // 줄마다의 폭 (%)
            if (spread > COMBO_OFFSET_SPREAD_PERCENT || std::abs(offset - 1.0) * PERCENT <= OFFSET_MIN_PERCENT) return 1.0;   // 일정하지 않거나 반올림 수준이면 보정 없음
            return GetWorstError(comboRows, offset) < GetWorstError(comboRows, 1.0) ? offset : 1.0;   // 배수를 적용하는 쪽이 더 맞을 때만
        }

        // 측정 당시 역할 (도약 레벨 슬라이더만 바꾼 줄의 lopec 배율이 일정하면 서포터, 달라지면 딜러, 줄이 부족하면 nullopt)
        [[nodiscard]] std::optional<Models::CombatRole> FindMeasuredRole(const CharacterMeasure& measure)
        {
            const std::string leapSlider{ std::format("{}{}", CONTROL_TREE_LEVEL, LEAP_TREE) };    // 도약 트리 레벨 슬라이더
            std::vector<double> ratios;                                                            // 도약 슬라이더 줄의 lopec 배율
            for (const CsvRow& row : measure.m_rows)                                               // 측정 줄 순회
            {
                if (GetField(row, measure.m_columns.m_group) != GROUP_PASSIVE || GetField(row, measure.m_columns.m_control) != leapSlider) continue;   // 도약 단일 줄만
                if (const std::optional<double> ratio{ ParseNumber<double>(GetField(row, measure.m_columns.m_ratio)) }) ratios.push_back(*ratio);   // 배율 기록
            }
            if (ratios.size() < MIN_ROLE_ROWS) return std::nullopt;                                // 판단할 줄이 부족
            const auto [low, high]{ std::ranges::minmax(ratios) };                                 // 최소 · 최대 배율
            return high - low <= ROLE_FLAT_SPREAD ? Models::CombatRole::ROLE_SUPPORTER : Models::CombatRole::ROLE_DEALER;   // 일정하면 서포터
        }

        // 비정상 캐릭터 (장신구를 하나도 끼지 않음 → lopec 기준이 불안정) 이유, 정상이면 nullopt
        [[nodiscard]] std::optional<std::string> FindAbnormalReason(const Models::CombatState& state)
        {
            const bool hasAccessory{ std::ranges::any_of(state.m_extra.m_accessories, [](const Models::AccessoryState& accessory)   // 연마 옵션이 하나라도 있는 장신구
            {
                return std::ranges::any_of(accessory.m_options, [](const std::string& option) { return !Models::IsNone(option); });   // 빈 옵션이 아닌 줄
            }) };
            if (!hasAccessory) return std::string{ "장신구가 없음" };                             // 장신구 미착용
            return std::nullopt;                                                                   // 정상
        }

        // 측정 한 줄에 적용할 어긋남 배수 (건드린 슬라이더 중 가장 크게 어긋난 값, 없으면 1)
        [[nodiscard]] double FindRowOffset(const RowResult& row, const std::map<std::string, double>& offsets)
        {
            double offset{ 1.0 };                                                                  // 기본 = 보정 없음
            for (const std::string& slider : row.m_sliders)
            {
                const auto found{ offsets.find(slider) };                                          // 이 슬라이더의 어긋남
                if (found != offsets.end() && std::abs(found->second - 1.0) > std::abs(offset - 1.0)) offset = found->second;   // 더 큰 쪽
            }
            return offset;
        }

        // 한 캐릭터의 측정 줄을 모두 다시 계산해 결과에 누적
        void CheckCharacter(const std::string& character, const CharacterMeasure& measure, const std::filesystem::path& armoryDirectory,
            const std::map<std::string, std::set<std::string>>& exceptions, RegressionResult& result, std::string& detail)
        {
            std::expected<Models::CombatState, std::string> loaded{ LoadCurrentState(armoryDirectory / ToPath(character + std::string{ JSON_EXTENSION })) };   // 현재 상태
            if (!loaded.has_value()) { result.m_notes.push_back(std::format("{} 건너뜀: {}", character, loaded.error())); return; }   // 무기고 없음
            Models::CombatState& current{ *loaded };                                               // 현재 상태
            if (const std::optional<std::string> abnormal{ FindAbnormalReason(current) })         // 비정상 캐릭터
            {
                result.m_notes.push_back(std::format("{} 건너뜀: 비정상 캐릭터 ({})", character, *abnormal));   // 안내
                ++result.m_abnormalCount;                                                          // 건너뛴 수
                return;
            }
            const bool isSupporter{ current.m_extra.m_role == Models::CombatRole::ROLE_SUPPORTER };   // 서포터 계수로 계산하는지
            if (const std::optional<Models::CombatRole> measuredRole{ FindMeasuredRole(measure) }; measuredRole.has_value() && *measuredRole != current.m_extra.m_role)   // 측정 뒤 딜러 ↔ 서포터 전환
            {
                result.m_notes.push_back(std::format("{} 건너뜀: 측정 뒤 역할이 바뀜 ({} → {}) → 다시 측정 필요", character,
                    *measuredRole == Models::CombatRole::ROLE_SUPPORTER ? "서포터" : "딜러", isSupporter ? "서포터" : "딜러"));   // 안내
                ++result.m_abnormalCount;                                                          // 건너뛴 수 (비정상으로 셈)
                return;
            }
            if (const std::optional<std::string> stale{ FindStaleLevel(measure, current.m_extra.m_passive) })   // 측정 뒤 성장
            {
                result.m_notes.push_back(std::format("{} 건너뜀: 측정 뒤 아크 패시브가 바뀜 ({}) → 다시 측정 필요", character, *stale));
                return;
            }
            current.m_extra.m_passive.m_enlightenment = FindBaselineEnlightenment(measure);        // 무기고에 없는 깨달음 구성은 측정 기준값으로

            // 1단계: 모든 줄을 실제 앱 엔진으로 다시 계산
            const ArkPassiveFactorComponent component;                                             // 실제 앱 아크 패시브 배율 요소
            std::vector<RowResult> rows;                                                           // 계산 결과
            for (const CsvRow& row : measure.m_rows)
            {
                const bool isCombo{ GetField(row, measure.m_columns.m_group) == GROUP_PASSIVE_COMBO };   // 조합 줄
                const std::string_view control{ isCombo ? COMBO_CONTROL : GetField(row, measure.m_columns.m_control) };   // 컨트롤 이름
                const std::optional<double> lopecRatio{ ParseNumber<double>(GetField(row, measure.m_columns.m_ratio)) };   // lopec 배율
                Models::CombatState target{ current };                                             // 목표 = 현재 + 변경
                if (!lopecRatio.has_value() || *lopecRatio <= 0.0 || !ApplyRow(target.m_extra.m_passive, row, measure.m_columns)) continue;   // 읽을 수 없는 줄
                if (IsException(exceptions, character, control)) { ++result.m_exceptionCount; continue; }   // lopec 특이점 (손으로 적은 제외)
                target.m_extra = Models::CombatExtra::Clamp(target.m_extra);                       // 앱 편집과 같은 범위 보정
                rows.push_back(RowResult{ std::string{ control }, FindSliders(row, measure.m_columns), std::string{ GetField(row, measure.m_columns.m_before) },
                    std::string{ GetField(row, measure.m_columns.m_after) }, *lopecRatio, component.GetFactor(current, target) });   // 엔진 예측 기록
            }

            // 2단계: lopec 레벨 슬라이더 기준 어긋남을 재서 보정 (보정한 슬라이더는 안내에 남김)
            const std::map<std::string, double> offsets{ FindSliderOffsets(rows, isSupporter ? LEAP_TREE : EVOLUTION_TREE) };   // 슬라이더별 어긋남 (역할별 기준 트리)
            const double comboOffset{ FindComboOffset(rows) };                                     // 조합 줄 기준 어긋남
            if (comboOffset != 1.0)                                                                // 조합 기준을 보정했으면
            {
                result.m_notes.push_back(std::format("{} : lopec 조합 기준 어긋남 보정 (×{:.6f})", character, comboOffset));   // 안내
                ++result.m_comboOffsetCharacterCount;                                              // 보정한 캐릭터 수
            }
            if (!offsets.empty())
            {
                const std::string text{ offsets | std::views::transform([](const auto& entry) { return std::format("{} ×{:.6f}", entry.first, entry.second); })
                    | std::views::join_with(std::string_view{ NOTE_SEPARATOR }) | std::ranges::to<std::string>() };   // "슬라이더 ×배수, ..."
                result.m_notes.push_back(std::format("{} : lopec 레벨 슬라이더 기준 어긋남 보정 ({})", character, text));   // 안내
                ++result.m_offsetCharacterCount;                                                   // 보정한 캐릭터 수
            }

            // 3단계: 허용치 비교
            const bool isFullRank{ std::ranges::all_of(current.m_extra.m_passive.m_treeRanks, [](int rank) { return rank == Models::MAX_PASSIVE_RANK; }) };   // 세 트리 모두 6랭크
            const double limit{ isSupporter ? std::max(SUPPORTER_MAX_ERROR_PERCENT, isFullRank ? FULL_RANK_MAX_ERROR_PERCENT : PARTIAL_RANK_MAX_ERROR_PERCENT)
                : (isFullRank ? FULL_RANK_MAX_ERROR_PERCENT : PARTIAL_RANK_MAX_ERROR_PERCENT) };   // 허용치 (서포터는 서포터 허용치)
            const std::string ranks{ current.m_extra.m_passive.m_treeRanks | std::views::transform([](int rank) { return std::to_string(rank); })
                | std::views::join_with(RANK_SEPARATOR) | std::ranges::to<std::string>() };       // 진화/깨달음/도약 랭크 ("6/6/6")
            ++result.m_characterCount;                                                             // 검사 캐릭터 수
            if (isSupporter) ++result.m_supporterCount;                                            // 서포터 수
            for (const RowResult& row : rows)
            {
                const double offset{ row.m_control == COMBO_CONTROL && comboOffset != 1.0 ? comboOffset : FindRowOffset(row, offsets) };   // 이 줄의 lopec 기준 어긋남 (조합 기준을 잰 경우 조합 줄은 그 값, 아니면 슬라이더 기준)
                const double lopecRatio{ row.m_lopecRatio / offset };                              // 보정한 lopec 배율
                const double error{ std::abs(row.m_predicted / lopecRatio - 1.0) * PERCENT };      // 오차 (%)
                double& worst{ isFullRank ? result.m_worstFullRankPercent : result.m_worstPartialRankPercent };   // 최대 오차 칸
                worst = std::max(worst, error);                                                    // 최대 갱신
                ++(isFullRank ? result.m_fullRankRowCount : result.m_partialRankRowCount);        // 검사 줄 수
                detail += std::format("{},{},{},{},{},{:.6f},{:.6f},{:.6f},{:.4f},{}\n", QuoteCsv(character), ranks, QuoteCsv(row.m_control), QuoteCsv(row.m_before),
                    QuoteCsv(row.m_after), row.m_lopecRatio, offset, row.m_predicted, error, limit);   // 상세 한 줄
                if (error > limit || !std::isfinite(error))                                        // 허용치 초과 (NaN 포함)
                {
                    result.m_failures.push_back(RegressionFailure{ character, row.m_control, std::format("{} → {}", row.m_before, row.m_after), error, limit });   // 초과 기록
                }
            }
        }
    }

    std::optional<std::filesystem::path> FindDataDirectory(const std::filesystem::path& start)
    {
        std::error_code error;                                                                     // 예외 없이 검사
        for (std::filesystem::path folder{ std::filesystem::absolute(start, error) }; !folder.empty(); folder = folder.parent_path())
        {
            const std::filesystem::path candidate{ folder / VERIFY_FOLDER / RESULTS_FOLDER };      // 후보
            if (std::filesystem::is_directory(candidate, error)) return candidate;                 // 찾음
            if (folder == folder.parent_path()) break;                                             // 드라이브 루트
        }
        return std::nullopt;                                                                       // 없음
    }

    RegressionResult Run(const std::filesystem::path& dataDirectory)
    {
        RegressionResult result;                                                                   // 결과
        const std::filesystem::path regressionDirectory{ dataDirectory / REGRESSION_FOLDER };      // 보조 폴더
        const std::filesystem::path armoryDirectory{ regressionDirectory / ARMORY_FOLDER };        // 무기고 폴더
        if (!std::filesystem::is_directory(armoryDirectory))                                       // 무기고를 아직 안 받음
        {
            result.m_isSkipped = true;
            result.m_skipReason = "무기고 폴더 없음 (python tools/Fetch-RegressionArmory.py 실행 필요)";
            return result;
        }

        const std::map<std::string, std::vector<CharacterMeasure>> measures{ LoadMeasures(dataDirectory) };   // 캐릭터별 측정 회차들
        const std::map<std::string, std::set<std::string>> exceptions{ LoadExceptions(regressionDirectory / EXCEPTIONS_FILE) };   // 제외 목록
        std::string detail{ UTF8_BOM };                                                            // 상세 결과 (엑셀에서 한글이 보이게 BOM)
        detail += DETAIL_HEADER;                                                                   // 머리글
        for (const auto& [character, characterMeasures] : measures)                                // 캐릭터마다
        {
            const std::optional<double> apiPower{ LoadApiPower(armoryDirectory / ToPath(character + std::string{ JSON_EXTENSION })) };   // API 전투력
            const CharacterMeasure* measure{ SelectMeasure(characterMeasures, apiPower) };         // 기준이 맞는 최근 회차
            if (measure == nullptr)                                                                // 모든 회차가 API와 다름
            {
                result.m_notes.push_back(std::format("{} 건너뜀: 측정 회차 없음", character));   // 안내
                continue;
            }
            CheckCharacter(character, *measure, armoryDirectory, exceptions, result, detail);     // 다시 계산
        }
        std::ofstream{ regressionDirectory / DETAIL_FILE, std::ios::binary | std::ios::trunc } << detail;   // 상세 파일 (gitignore 폴더)
        std::ranges::sort(result.m_failures, std::greater{}, &RegressionFailure::m_errorPercent);  // 오차 큰 순
        if (result.m_characterCount == 0) { result.m_isSkipped = true; result.m_skipReason = "검사할 측정이 없음"; }   // 데이터 없음
        return result;                                                                             // 결과
    }
}
