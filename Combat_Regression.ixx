export module LostArkMultiTool.Core.CombatRegression;

import std;

// lopec 측정값 대비 전투력 엔진 회귀 검사 (자체 검사 --self-test에서 호출)
// Verify/lopec_results/lopec_all.csv(캐릭터별 가장 최근 측정 회차)와 regression/armory의 무기고 응답을 읽어, 실제 앱 엔진(무기고 해석기 → 아크 패시브 배율 요소)으로
// 같은 변경을 다시 계산하고 lopec 배율과의 오차가 허용치 안인지 확인 (측정 데이터는 gitignore → 데이터가 없는 PC면 건너뜀)
export namespace LostArkMultiTool::Core::CombatRegression
{
    // 허용치를 넘은 측정 한 줄
    struct RegressionFailure
    {
        std::string m_character;      // 캐릭터 이름
        std::string m_control;        // lopec 컨트롤 (rankControl 등, 조합이면 COMBO)
        std::string m_change;         // 변경 내용 (변경 전 → 변경 후)
        double m_errorPercent{};      // 엔진 예측과 lopec 배율의 오차 (%)
        double m_limitPercent{};      // 이 줄에 적용한 허용치 (%)
    };

    // 회귀 검사 결과
    struct RegressionResult
    {
        bool m_isSkipped{};                        // 측정 데이터가 없어 건너뜀
        std::string m_skipReason;                  // 건너뛴 이유
        int m_characterCount{};                    // 검사한 캐릭터 수
        int m_fullRankRowCount{};                  // 6랭크 캐릭터 검사 줄 수
        int m_partialRankRowCount{};               // 랭크가 덜 찬 캐릭터 검사 줄 수
        int m_exceptionCount{};                    // lopec 특이점으로 제외한 줄 수
        int m_offsetCharacterCount{};              // lopec 레벨 슬라이더 기준 어긋남을 보정한 캐릭터 수
        double m_worstFullRankPercent{};           // 6랭크 최대 오차 (%)
        double m_worstPartialRankPercent{};        // 덜 찬 랭크 최대 오차 (%)
        std::vector<RegressionFailure> m_failures; // 허용치 초과 줄 (오차 큰 순)
        std::vector<std::string> m_notes;          // 건너뛴 캐릭터 등 안내
    };

    // start에서 위로 올라가며 측정 데이터 폴더(Verify/lopec_results)를 찾음 (없으면 nullopt)
    [[nodiscard]] std::optional<std::filesystem::path> FindDataDirectory(const std::filesystem::path& start);

    // 측정 데이터 폴더의 lopec_all.csv 전체를 다시 계산해 결과 반환
    [[nodiscard]] RegressionResult Run(const std::filesystem::path& dataDirectory);
}
