export module LostArkMultiTool.Models.GemData;

import std;
import LostArkMultiTool.Models.PriceAlarm;
import LostArkMultiTool.Models.RefreshSettings;

export namespace LostArkMultiTool::Models
{
    // 보석 검색기 저장 파일 전체 (알림 · 마지막 전체 갱신가 · 일별 최저가 기록 · 설정, 원본 GemData)
    class GemData
    {
    public:
        // 목표가 알림 목록 (보석 하나당 알림 하나)
        [[nodiscard]] std::vector<PriceAlarm>& GetAlarms() noexcept;
        [[nodiscard]] const std::vector<PriceAlarm>& GetAlarms() const noexcept;

        // 보석 키 → 마지막 "전체 갱신" 최저가 (재시작 후 첫 등락 비교용)
        [[nodiscard]] std::map<std::string, std::int64_t>& GetLastPrices() noexcept;
        [[nodiscard]] const std::map<std::string, std::int64_t>& GetLastPrices() const noexcept;

        // 보석 키 → (날짜 키 "yyyy-MM-dd" → 그날 관측된 최저가), 정렬 맵이라 날짜순
        [[nodiscard]] std::map<std::string, std::map<std::string, std::int64_t>>& GetHistory() noexcept;
        [[nodiscard]] const std::map<std::string, std::map<std::string, std::int64_t>>& GetHistory() const noexcept;

        [[nodiscard]] const RefreshSettings& GetSettings() const noexcept;
        void SetSettings(const RefreshSettings& settings) noexcept;

        // 읽은 직후 잘못된 값 보정 (설정 범위 · 이름이 빠진 알림 · 같은 보석에 겹친 알림)
        void Normalize();

        // 조회 대상이 아닌 보석의 기록과 보관 기간이 지난 기록 정리 (바뀐 게 있으면 true)
        // oldestKeptKey: 남길 가장 오래된 날짜 키 (이보다 앞선 날짜는 지움)
        // 알림은 사용자가 직접 지우도록 남겨 둠 (화면에 "조회 범위 밖"으로 표시)
        bool Prune(std::string_view oldestKeptKey);

    private:
        std::vector<PriceAlarm> m_alarms;
        std::map<std::string, std::int64_t> m_lastPrices;
        std::map<std::string, std::map<std::string, std::int64_t>> m_history;
        RefreshSettings m_settings;
    };
}