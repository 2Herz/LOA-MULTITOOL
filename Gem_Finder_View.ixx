export module LostArkMultiTool.Views.GemFinderView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.GemQuote;
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

    // 보석 최저가 검색기 화면 구성 (원본 MainForm의 화면 부분)
    // 부품 배치 · 표시 형식만 알고, 조회 · 알림 판정 · 저장 · 자동 갱신 일정은 GemFinderManager가 맡음
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
        void SetOnPriceCellClicked(std::function<void(const Models::GemSelection& gem)> onPriceCellClicked);
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
        void SetRefreshEnabled(bool isEnabled) noexcept;        // 조회 중에는 [시세 갱신] 비활성
        void SetStatusText(std::wstring text);
        void SetRateLimitText(std::wstring text, RateLimitLevel level);

        // ===== 최저가 표 =====
        void ShowQuote(const Models::GemQuote& quote);   // 조회 범위 밖 보석이면 무시

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
        // 묶음 상자 (원본 GroupBox: 테두리 + 제목)
        struct Panel
        {
            Core::RectF m_bounds;
            std::wstring m_title;
        };

        void InitializePriceGrid();
        void Layout(const Core::RectF& contentArea);
        [[nodiscard]] Core::RectF LayoutPanel(Panel& panel, const Core::RectF& bounds) const;   // 테두리 · 제목 자리를 뺀 안쪽 영역
        void RenderPanels(Core::Renderer& renderer) const;
        [[nodiscard]] std::optional<std::size_t> FindGemIndex(const Models::GemSelection& gem) const;

        UI::ControlHost m_host;
        std::vector<Models::GemSelection> m_gems;   // 보석 콤보 상자 항목 순서 (겁화 10 ~ 7, 작열 10 ~ 7)

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Button& m_refreshButton;
        UI::CheckBox& m_autoRefreshBox;
        UI::Label& m_watchLabel;
        UI::ComboBox& m_watchIntervalBox;
        UI::Label& m_fullLabel;
        UI::ComboBox& m_fullIntervalBox;
        UI::Label& m_statusLabel;
        UI::Label& m_rateLimitLabel;
        UI::Grid& m_priceGrid;
        UI::ComboBox& m_alarmGemBox;
        UI::Label& m_targetLabel;
        UI::TextBox& m_targetPriceBox;
        UI::Button& m_addAlarmButton;
        UI::Button& m_removeAlarmButton;
        UI::Grid& m_alarmGrid;
        UI::ComboBox& m_chartGemBox;
        UI::LineChart& m_chart;

        Panel m_pricePanel;
        Panel m_alarmPanel;
        Panel m_chartPanel;
        Core::RectF m_layoutArea;
        bool m_hasLayout{};

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
        std::function<void()> m_onIntervalChanged;
        std::function<void(const Models::GemSelection&)> m_onPriceCellClicked;
        std::function<void(std::size_t)> m_onAlarmSelected;
        std::function<void()> m_onAddAlarm;
        std::function<void()> m_onRemoveAlarm;
        std::function<void()> m_onChartGemChanged;
    };
}