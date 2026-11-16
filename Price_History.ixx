export module LostArkMultiTool.Models.PriceHistory;

import std;

export namespace LostArkMultiTool::Models
{
    // 품목별 일별 최저가 기록 (유각 · 젬 시세 검색기 공용, 보석 검색기의 GemData 기록과 같은 형태)
    // 키(품목 이름) → (날짜 키 "yyyy-MM-dd" → 그날 관측된 최저가), 정렬 맵이라 날짜순
    class PriceHistory
    {
    public:
        using DailyPrices = std::map<std::string, std::int64_t>;                    // 날짜 키 → 최저가
        using HistoryMap = std::map<std::string, DailyPrices, std::less<>>;         // 품목 키 → 일별 기록

        // 품목 하나의 일별 기록 (기록이 없으면 빈 목록)
        [[nodiscard]] const DailyPrices& GetDays(std::string_view itemKey) const;

        // 전체 기록 (저장소가 읽고 쓸 때 사용)
        [[nodiscard]] HistoryMap& GetAll() noexcept;
        [[nodiscard]] const HistoryMap& GetAll() const noexcept;

        // 그날 최저가 기록 (그날 처음이거나 더 싸면 갱신, 바뀐 게 있으면 true)
        bool RecordLowest(const std::string& itemKey, const std::string& dayKey, std::int64_t price);

        // 보관 기간이 지난 기록 정리 (바뀐 게 있으면 true, oldestKeptKey = 남길 가장 오래된 날짜 키)
        bool Prune(std::string_view oldestKeptKey);

        // ===== 즐겨찾기 (유각 시세 검색기에서 사용, 기록과 같은 파일에 저장) =====
        using FavoriteSet = std::set<std::string, std::less<>>;                     // 즐겨찾기 품목 키 모음

        [[nodiscard]] bool IsFavorite(std::string_view itemKey) const;               // 즐겨찾기인지
        void ToggleFavorite(const std::string& itemKey);                             // 즐겨찾기 추가 ↔ 해제
        [[nodiscard]] FavoriteSet& GetFavorites() noexcept;                          // 전체 즐겨찾기 (저장소가 읽고 쓸 때 사용)
        [[nodiscard]] const FavoriteSet& GetFavorites() const noexcept;              // 전체 즐겨찾기 (읽기 전용)

    private:
        HistoryMap m_history;     // 품목별 일별 기록
        FavoriteSet m_favorites;  // 즐겨찾기 품목 키
    };
}
