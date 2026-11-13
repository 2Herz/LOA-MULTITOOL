export module LostArkMultiTool.Views.GemFinderView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.GemQuote;
import LostArkMultiTool.Views.PageLayout;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.LineChart;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // 알림 목록 한 행의 상태 (위에서부터 순서대로 판정, 원본 FillAlarmRow)
    enum class AlarmState
    {
        ALARM_OUT_OF_RANGE,   // 조회 범위 밖 (삭제 권장)
        ALARM_NOT_CHECKED,    // 이번 실행에서 아직 확인 전
        ALARM_REACHED,        // 목표가 도달
        ALARM_WAITING         // 대기
    };

    // 알림 목록 한 행
    struct AlarmRow
    {
        std::wstring m_name;                          // "10레벨 겁화"
        std::int64_t m_targetPrice{};
        std::optional<std::int64_t> m_currentPrice;   // 없으면 "-"
        AlarmState m_state{ AlarmState::ALARM_NOT_CHECKED };
    };

    // 상단 바 호출 한도 글자의 강조 정도
    enum class RateLimitLevel
    {
        RATE_NORMAL,    // 기본 글자색
        RATE_CAUTION,   // 예상 호출량이 한도에 가까움 (주황)
        RATE_ALERT      // 일시정지 · 한도 보호 중 (빨강)
    };

    // 최저가 표 정렬 기준 (정렬 콤보 상자 항목 순서와 같음)
    enum class GemSortOrder
    {
        SORT_PRICE_DESC,    // 현재 최저가 높은 순 (기본)
        SORT_PRICE_ASC,     // 현재 최저가 낮은 순
        SORT_CHANGE_DESC,   // 전일 대비 변동률 높은 순
        SORT_CHANGE_ASC     // 전일 대비 변동률 낮은 순
    };

    // 최저가 표 한 행 = 보석 하나 (레벨 × 종류, 값이 없으면 "-")
    struct GemRow
    {
        Models::GemSelection m_gem;                    // 종류 + 레벨 (행을 누르면 그래프 · 알림 대상)
        std::wstring m_name;                           // "10레벨 겁화의 보석"
        std::wstring m_typeText;                       // 분류 칩 글자 "겁화"
        std::optional<std::int64_t> m_price;           // 현재 최저가 (없으면 조회 전이거나 매물 없음)
        bool m_isQueried{};                            // 이번 실행에서 조회했는지 ("-" / "매물 없음" 구분)
        std::optional<std::int64_t> m_diff;            // 직전 전체 갱신 대비 등락
        std::optional<std::int64_t> m_yesterdayPrice;  // 전일 최저가 (일별 기록, 없으면 "-")
        std::optional<double> m_changeRate;            // 전일 최저가 대비 변동률(%)
        std::vector<double> m_trend;                   // 추이 선 값 (일별 최저가 기록 중 최근 날짜들)
    };

    // 위쪽 통계 카드 4장의 원본 값 (화면이 형식 · 색을 정함)
    struct GemStats
    {
        std::size_t m_queriedCount{};          // 이번 실행에서 조회한 보석 수
        std::size_t m_listedCount{};           // 그중 매물이 있는 수
        std::optional<double> m_averageRate;   // 전일 최저가 대비 평균 변동률(%) (계산할 수 있는 보석이 없으면 nullopt)
        std::wstring m_topName;                // 현재 최저가가 가장 비싼 보석 이름 (없으면 빈 글자)
        std::int64_t m_topPrice{};             // 그 보석의 현재 최저가
        std::wstring m_updatedText;            // 마지막 갱신 시각 글자 (아직 조회 전이면 빈 글자)
    };

    // 보석 최저가 검색기 화면 구성 (원본 MainForm의 화면 부분)
    // 부품 배치 · 표시 형식만 알고, 조회 · 알림 판정 · 저장 · 검색 · 정렬 · 자동 갱신 일정은 GemFinderManager가 맡음
    class GemFinderView
    {
    public:
        explicit GemFinderView(Core::Platform& platform);
        ~GemFinderView();

        GemFinderView(const GemFinderView&) = delete;
        GemFinderView& operator=(const GemFinderView&) = delete;

        // ===== 사용자 동작 알림 (GemFinderManager가 연결) =====
        void SetOnRefresh(std::function<void()> onRefresh);
        void SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged);
        void SetOnIntervalChanged(std::function<void()> onIntervalChanged);
        void SetOnFilterChanged(std::function<void()> onFilterChanged);   // 검색어 · 분류 · 정렬이 바뀜
        void SetOnPriceCellClicked(std::function<void(const Models::GemSelection& gem)> onPriceCellClicked);   // 표의 행을 누름
        void SetOnAlarmSelected(std::function<void(std::size_t alarmIndex)> onAlarmSelected);
        void SetOnAddAlarm(std::function<void()> onAddAlarm);
        void SetOnRemoveAlarm(std::function<void()> onRemoveAlarm);
        void SetOnChartGemChanged(std::function<void()> onChartGemChanged);

        // ===== 상단 바 =====
        void SetIntervals(int watchSeconds, int fullMinutes);   // 코드에서 바꿈 (알림 없음, 선택지에 없으면 첫 항목)
        [[nodiscard]] int GetWatchIntervalSeconds() const noexcept;
        [[nodiscard]] int GetFullIntervalMinutes() const noexcept;
        void SetAutoRefreshChecked(bool isChecked) noexcept;    // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsAutoRefreshChecked() const noexcept;
        void SetRefreshEnabled(bool isEnabled) noexcept;        // 조회 중에는 [시세 갱신] 비활성 + "조회 중…"
        void SetStatusText(std::wstring text);
        void SetRateLimitText(std::wstring text, RateLimitLevel level);

        // ===== 검색 · 분류 · 정렬 =====
        [[nodiscard]] const std::wstring& GetSearchText() const noexcept;
        [[nodiscard]] std::optional<std::string> GetCategoryTypeName() const;   // 고른 분류의 보석 종류 이름 (전체 분류면 nullopt)
        [[nodiscard]] GemSortOrder GetSortOrder() const noexcept;

        // ===== 최저가 표 =====
        void ShowRows(const std::vector<GemRow>& rows, std::size_t totalCount);   // totalCount = 거르기 전 전체 보석 수 (빈 안내 문구 선택용)
        void SetStats(GemStats stats);                                             // 위쪽 통계 카드 값

        // ===== 알림 =====
        void ShowAlarms(const std::vector<AlarmRow>& rows);
        [[nodiscard]] std::optional<std::size_t> GetSelectedAlarmIndex() const noexcept;
        void SelectAlarm(std::optional<std::size_t> alarmIndex);   // 코드에서 바꿈 (알림 없음)

        [[nodiscard]] std::optional<Models::GemSelection> GetAlarmGem() const;
        void SelectAlarmGem(const Models::GemSelection& gem);
        [[nodiscard]] std::int64_t GetTargetPrice() const noexcept;   // 비었거나 읽을 수 없으면 0 (최대값으로 제한)
        void SetTargetPrice(std::int64_t targetPrice);

        // ===== 그래프 =====
        [[nodiscard]] std::optional<Models::GemSelection> GetChartGem() const;
        void SelectChartGem(const Models::GemSelection& gem);
        void SetChart(std::wstring title, std::vector<UI::ChartPoint> points);

        // ===== 프레임 =====
        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때 (조합 확정 · IME 끔)
        void RestoreFocus();

    private:
        void Layout(const Core::RectF& contentArea);
        void NotifyFilterChanged() const;
        void SyncRowSelection();   // 표에서 그래프 보석의 행을 선택 상태로 (정렬 · 검색으로 순서가 바뀌어도 따라감)
        [[nodiscard]] std::vector<PageLayout::StatCard> BuildStatCards() const;   // 통계 값 → 카드 4장
        void RenderTableExtras(Core::Renderer& renderer) const;                    // 표 아래 "총 N개" 줄 · 빈 안내
        [[nodiscard]] std::optional<std::size_t> FindGemIndex(const Models::GemSelection& gem) const;

        UI::ControlHost m_host;
        std::vector<Models::GemSelection> m_gems;      // 보석 콤보 상자 항목 순서 (겁화 10 ~ 7, 작열 10 ~ 7)
        std::vector<Models::GemSelection> m_rowGems;   // 표 행 순서대로의 보석 (검색 · 정렬 결과와 같은 순서)

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Button& m_refreshButton;
        UI::CheckBox& m_autoRefreshBox;
        UI::Label& m_watchLabel;
        UI::ComboBox& m_watchIntervalBox;
        UI::Label& m_fullLabel;
        UI::ComboBox& m_fullIntervalBox;
        UI::TextBox& m_searchBox;
        UI::Button& m_clearButton;      // 검색어 한 번에 지우기 (✕)
        UI::ComboBox& m_categoryBox;    // 분류 (전체 분류 / 겁화 / 작열)
        UI::ComboBox& m_sortBox;
        UI::Label& m_statusLabel;
        UI::Label& m_rateLimitLabel;
        UI::Grid& m_priceGrid;
        UI::ComboBox& m_alarmGemBox;
        UI::TextBox& m_targetPriceBox;
        UI::Button& m_addAlarmButton;
        UI::Button& m_removeAlarmButton;
        UI::Grid& m_alarmGrid;
        UI::ComboBox& m_chartGemBox;
        UI::LineChart& m_chart;

        std::vector<Core::RectF> m_statBounds;   // 통계 카드 4장 자리
        Core::RectF m_filterBounds;              // 조회 · 검색 줄 카드
        Core::RectF m_priceCardBounds;           // 최저가 표 카드
        Core::RectF m_alarmCardBounds;           // 목표가 알림 카드
        Core::RectF m_chartCardBounds;           // 시세 기록 그래프 카드
        Core::RectF m_footerBounds;              // 표 아래 "총 N개" 줄
        Core::RectF m_emptyBounds;               // 표가 비었을 때 안내를 그릴 자리
        GemStats m_stats;                        // 통계 카드 값
        std::size_t m_rowCount{};                // 지금 표에 보이는 행 수
        std::size_t m_totalCount{};              // 거르기 전 전체 보석 수
        Core::RectF m_layoutArea;
        bool m_hasLayout{};

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
        std::function<void()> m_onIntervalChanged;
        std::function<void()> m_onFilterChanged;
        std::function<void(const Models::GemSelection&)> m_onPriceCellClicked;
        std::function<void(std::size_t)> m_onAlarmSelected;
        std::function<void()> m_onAddAlarm;
        std::function<void()> m_onRemoveAlarm;
        std::function<void()> m_onChartGemChanged;
    };
}
