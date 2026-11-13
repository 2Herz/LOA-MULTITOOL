export module LostArkMultiTool.Views.EngravingFinderView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
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
    // 시세 표 정렬 기준 (정렬 콤보 상자 항목 순서와 같음)
    enum class EngravingSortOrder
    {
        SORT_PRICE_DESC,    // 현재 최저가 높은 순 (기본)
        SORT_PRICE_ASC,     // 현재 최저가 낮은 순
        SORT_CHANGE_DESC,   // 전일 대비 상승률 높은 순
        SORT_CHANGE_ASC,    // 전일 대비 하락률 높은 순
        SORT_NAME           // 이름 가나다 순
    };

    // 시세 표 한 행 (값이 0이면 "-"로 표시)
    struct EngravingRow
    {
        std::wstring m_name;
        double m_currentMinPrice{};
        std::optional<double> m_diff;             // 직전 갱신 대비 등락 (없으면 등락 표시 없음)
        double m_yesterdayAvgPrice{};
        double m_recentPrice{};
        std::optional<double> m_dayChangeRate;    // 전일 평균가 대비 변동률(%)
        std::string m_key;                        // 일별 기록을 찾는 키 (UTF-8 각인서 이름, 화면에는 표시하지 않음)
    };

    // 유각 시세 검색기 화면 구성
    // 부품 배치 · 표시 형식만 알고, 조회 · 검색 · 정렬 · 자동 갱신 일정은 EngravingFinderManager가 맡음
    class EngravingFinderView
    {
    public:
        explicit EngravingFinderView(Core::Platform& platform);
        ~EngravingFinderView();

        EngravingFinderView(const EngravingFinderView&) = delete;
        EngravingFinderView& operator=(const EngravingFinderView&) = delete;

        // ===== 사용자 동작 알림 (EngravingFinderManager가 연결) =====
        void SetOnRefresh(std::function<void()> onRefresh);
        void SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged);
        void SetOnFilterChanged(std::function<void()> onFilterChanged);   // 검색어 · 정렬이 바뀜
        void SetOnRowSelected(std::function<void()> onRowSelected);       // 시세 표에서 행을 골랐음 (그래프 대상)

        // ===== 상단 바 =====
        void SetAutoRefreshChecked(bool isChecked) noexcept;   // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsAutoRefreshChecked() const noexcept;
        void SetRefreshEnabled(bool isEnabled) noexcept;       // 조회 중에는 [시세 갱신] 비활성
        void SetStatusText(std::wstring text);
        [[nodiscard]] const std::wstring& GetSearchText() const noexcept;
        [[nodiscard]] EngravingSortOrder GetSortOrder() const noexcept;

        // ===== 시세 표 =====
        void ShowRows(const std::vector<EngravingRow>& rows);
        [[nodiscard]] std::optional<std::size_t> GetSelectedRow() const noexcept;   // 고른 행 (없으면 nullopt)
        void SelectRow(std::optional<std::size_t> row);                              // 코드에서 바꿈 (알림 없음)

        // ===== 일별 최저가 그래프 (오른쪽) =====
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
        static void RenderPanel(Core::Renderer& renderer, const Core::RectF& bounds, std::wstring_view title);   // 묶음 상자 하나
        void NotifyFilterChanged() const;

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Button& m_refreshButton;
        UI::CheckBox& m_autoRefreshBox;
        UI::Label& m_searchLabel;
        UI::TextBox& m_searchBox;
        UI::Label& m_sortLabel;
        UI::ComboBox& m_sortBox;
        UI::Label& m_statusLabel;
        UI::Grid& m_priceGrid;
        UI::LineChart& m_chart;      // 고른 각인서의 일별 최저가 그래프

        Core::RectF m_panelBounds;        // 시세 표 묶음 상자 (테두리 + 제목)
        Core::RectF m_chartPanelBounds;   // 그래프 묶음 상자
        Core::RectF m_layoutArea;
        bool m_hasLayout{};

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
        std::function<void()> m_onFilterChanged;
        std::function<void()> m_onRowSelected;
    };
}
