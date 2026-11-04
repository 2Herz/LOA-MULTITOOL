export module LostArkMultiTool.Views.TrackerView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.LineChart;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // 주차 하나의 통계 칸 (총합 · 평균 · 전주대비 · 첫주대비, 첫 주차는 증감률 없음)
    struct WeekStats
    {
        double m_total{};
        double m_average{};
        std::optional<double> m_previousRate;   // 전주대비(%)
        std::optional<double> m_firstRate;      // 첫주대비(%)
    };

    // 낙원력 트래커 화면 구성 (원본 Form1의 화면 부분)
    // 열 구성: 주차 | 현재 원정대(입력) | 원정대 제외(읽기 전용) | 총합 · 평균 · 전주대비 · 첫주대비
    // 부품 배치 · 표시 형식만 알고, 계산 · 저장 · API 요청 · 시즌 관리는 TrackerManager가 맡음
    // 캐릭터 번호(characterIndex) = 캐릭터 열 순서 (0부터, 앞쪽 editableCount개가 입력 가능한 현재 원정대)
    class TrackerView
    {
    public:
        explicit TrackerView(Core::Platform& platform);
        ~TrackerView();

        TrackerView(const TrackerView&) = delete;
        TrackerView& operator=(const TrackerView&) = delete;

        // ===== 사용자 동작 알림 (TrackerManager가 연결) =====
        void SetOnSyncRoster(std::function<void(const std::wstring& characterName)> onSyncRoster);
        void SetOnAddWeek(std::function<void()> onAddWeek);
        void SetOnAddSeason(std::function<void()> onAddSeason);
        void SetOnEditPeriod(std::function<void()> onEditPeriod);   // [시즌 기간] (기간이 있는 시즌의 시작일 · 종료일 바꾸기)
        void SetOnUnlockWeek(std::function<void()> onUnlockWeek);   // [지난 주차 수정] (고른 지난 주차를 확인 후 잠깐 풀기)
        void SetOnSeasonSelected(std::function<void()> onSeasonSelected);
        // 낙원력 칸 편집 확정 (score = nullopt면 숫자가 아님 → 받는 쪽이 안내하고 원래 값을 다시 씀, 빈칸 = 0 = 미기록)
        void SetOnScoreEdited(std::function<void(std::size_t weekIndex, std::size_t characterIndex, std::optional<double> score)> onScoreEdited);

        // ===== 시즌 선택 =====
        void SetSeasonItems(std::vector<std::wstring> items, std::optional<std::size_t> selectedIndex);
        [[nodiscard]] std::optional<std::size_t> GetSelectedSeasonIndex() const noexcept;

        // ===== 표 =====
        // 캐릭터 열 다시 만들기 (headers = 캐릭터 열 머리글, 앞쪽 editableCount개는 입력 가능 · 나머지는 원정대 제외 열)
        void SetCharacterColumns(std::vector<std::wstring> headers, std::size_t editableCount);
        void SetWeekCount(std::size_t weekCount);   // 행 수 = 주차 수 (칸 색 · 편집 가능 여부도 다시 정함)
        void ShowWeekLabel(std::size_t weekIndex, std::wstring label);
        void ShowScore(std::size_t weekIndex, std::size_t characterIndex, double score);
        void ShowStats(std::size_t weekIndex, const WeekStats& stats);
        void CommitEdit();   // 편집 중인 칸 확정 (동기화 · 주차 추가 · 시즌 전환 전)

        // ===== 시즌 기간 (주차 잠금) =====
        // editableWeeks: 주차마다 입력 가능 여부 (nullopt = 기간 없는 시즌 → 모든 주차 입력 가능), currentWeek = 이번 주차 (주차 열 강조)
        void SetWeekLock(std::optional<std::vector<bool>> editableWeeks, std::optional<std::size_t> currentWeek);
        void FocusWeek(std::size_t weekIndex);                         // 그 주차의 첫 캐릭터 칸을 고르고 보이게 스크롤
        [[nodiscard]] std::optional<std::size_t> GetSelectedWeek() const noexcept;
        void SetPeriodMode(bool hasPeriod);                            // 기간 있음: [주차 추가] 끔 · [시즌 기간] · [지난 주차 수정] 켬
        void SetScheduleText(std::wstring text);                       // 도구 막대 아래 안내 줄 (빈 글자면 줄을 숨김)

        // ===== 그래프 · 버튼 상태 =====
        void SetChart(std::wstring title, std::vector<UI::ChartPoint> points);
        void SetSyncBusy(bool isBusy);

        // ===== 프레임 =====
        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때 (편집 확정 · IME 끔)
        void RestoreFocus();

    private:
        void Layout(const Core::RectF& contentArea);
        void ApplyRowStyle(std::size_t row);   // 한 행의 칸 색 · 편집 가능 여부
        void HandleCellEdited(std::size_t row, std::size_t column, const std::wstring& text);

        [[nodiscard]] std::size_t GetStatColumn(std::size_t statOffset) const noexcept;   // 통계 열 번호 (캐릭터 열 바로 다음부터)
        [[nodiscard]] bool IsEditableColumn(std::size_t column) const noexcept;
        [[nodiscard]] bool IsEditableWeek(std::size_t row) const noexcept;   // 주차 잠금 (기간 없는 시즌이면 항상 true)

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::TextBox& m_nameBox;
        UI::Button& m_syncButton;
        UI::Button& m_addWeekButton;
        UI::Button& m_addSeasonButton;
        UI::ComboBox& m_seasonBox;
        UI::Button& m_periodButton;      // [시즌 기간]
        UI::Button& m_unlockButton;      // [지난 주차 수정]
        UI::Label& m_scheduleLabel;      // 이번 주 입력 마감 안내
        UI::Grid& m_grid;
        UI::LineChart& m_chart;

        std::size_t m_characterCount{};   // 캐릭터 열 수 (현재 원정대 + 원정대 제외)
        std::size_t m_editableCount{};    // 그중 입력 가능한 현재 원정대 열 수
        std::optional<std::vector<bool>> m_editableWeeks;   // 주차 잠금 (nullopt = 잠금 없음)
        std::optional<std::size_t> m_currentWeek;           // 이번 주차 (강조)

        Core::RectF m_layoutArea;
        bool m_hasLayout{};

        std::function<void(const std::wstring&)> m_onSyncRoster;
        std::function<void()> m_onAddWeek;
        std::function<void()> m_onAddSeason;
        std::function<void()> m_onEditPeriod;
        std::function<void()> m_onUnlockWeek;
        std::function<void()> m_onSeasonSelected;
        std::function<void(std::size_t, std::size_t, std::optional<double>)> m_onScoreEdited;
    };
}
