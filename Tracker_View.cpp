module LostArkMultiTool.Views.TrackerView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.LineChart;
import LostArkMultiTool.Views.UI.TextBox;

namespace LostArkMultiTool::Views
{
    namespace
    {
        // ===== 표 열 (원본 Form1과 같은 순서) =====
        constexpr std::size_t COL_WEEK{ 0 };
        constexpr std::size_t FIRST_CHAR_COL{ 1 };

        // 통계 열 위치 (마지막 캐릭터 열 바로 다음부터의 순서, STAT_HEADERS 순서와 같아야 함)
        constexpr std::size_t STAT_TOTAL_OFFSET{ 0 };
        constexpr std::size_t STAT_AVERAGE_OFFSET{ 1 };
        constexpr std::size_t STAT_PREV_RATE_OFFSET{ 2 };
        constexpr std::size_t STAT_FIRST_RATE_OFFSET{ 3 };
        constexpr std::size_t STAT_COLUMN_COUNT{ 4 };
        constexpr std::array<std::wstring_view, STAT_COLUMN_COUNT> STAT_HEADERS{ L"총합", L"평균", L"전주대비(%)", L"첫주대비(%)" };

        constexpr float WEEK_COLUMN_WEIGHT{ 0.8f };
        constexpr float CHARACTER_COLUMN_WEIGHT{ 1.0f };
        constexpr float STAT_COLUMN_WEIGHT{ 1.0f };

        constexpr int SCORE_DECIMALS{ 0 };   // 원본 "N0"
        constexpr int RATE_DECIMALS{ 1 };    // 원본 증감률 소수 첫째 자리
        constexpr double UNRECORDED_SCORE{ 0.0 };

        // ===== 글자 =====
        constexpr std::wstring_view WEEK_HEADER{ L"주차" };
        constexpr std::wstring_view NO_RATE_TEXT{ L"-" };     // 첫 주차는 비교 대상 없음
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };
        constexpr std::wstring_view NAME_PLACEHOLDER{ L"캐릭터 닉네임" };
        constexpr std::wstring_view SYNC_TEXT{ L"원정대 동기화" };
        constexpr std::wstring_view SYNC_BUSY_TEXT{ L"동기화 중..." };
        constexpr std::wstring_view ADD_WEEK_TEXT{ L"주차 추가" };
        constexpr std::wstring_view ADD_SEASON_TEXT{ L"시즌 추가" };
        constexpr std::wstring_view PERIOD_TEXT{ L"시즌 기간" };
        constexpr std::wstring_view UNLOCK_WEEK_TEXT{ L"지난 주차 수정" };
        constexpr std::wstring_view CHART_VALUE_PREFIX{ L"평균 " };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 16.0f };            // 도구 막대 · 표 · 그래프 사이
        constexpr float CONTROL_GAP{ 8.0f };
        constexpr float GROUP_GAP{ 16.0f };              // 원정대 동기화 묶음과 주차 · 시즌 묶음 사이
        constexpr float NAME_BOX_WIDTH{ 180.0f };
        constexpr float SYNC_BUTTON_WIDTH{ 120.0f };
        constexpr float ADD_BUTTON_WIDTH{ 96.0f };
        constexpr float SEASON_BOX_WIDTH{ 120.0f };
        constexpr float PERIOD_BUTTON_WIDTH{ 96.0f };
        constexpr float UNLOCK_BUTTON_WIDTH{ 120.0f };
        constexpr float SCHEDULE_LINE_HEIGHT{ 20.0f };   // 이번 주 입력 마감 안내 줄
        constexpr float SCHEDULE_LINE_GAP{ 8.0f };
        constexpr Core::TextStyle SCHEDULE_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };
        constexpr float HEADER_HEIGHT{ 44.0f };          // 두 줄 머리글 "직업\n(Lv.1740)"
        constexpr float GRID_AREA_RATIO{ 0.45f };        // 표가 차지할 수 있는 최대 높이 비율 (나머지는 그래프)
        constexpr std::size_t GRID_MIN_VISIBLE_ROWS{ 3 };

        // 낙원력 칸에 받을 글자 (숫자 · 콤마 · 소수점)
        [[nodiscard]] bool IsScoreCharacter(wchar_t character) noexcept
        {
            return (character >= L'0' && character <= L'9') || character == L',' || character == L'.';
        }

        // 낙원력 글자 해석: 빈칸 = 0(미기록), 천 단위 콤마 허용, 숫자가 아니면 nullopt (원본 TryParseScore)
        [[nodiscard]] std::optional<double> ParseScore(std::wstring_view text)
        {
            std::string digits;
            for (const wchar_t character : text)
            {
                if (character == L',' || character == L' ') continue;
                if (!IsScoreCharacter(character)) return std::nullopt;
                digits.push_back(static_cast<char>(character));   // 숫자 · 소수점만 남으므로 ASCII
            }
            if (digits.empty()) return UNRECORDED_SCORE;

            double score{};
            const char* first{ digits.data() };
            const char* last{ digits.data() + digits.size() };
            const auto [ptr, ec] { std::from_chars(first, last, score) };
            if (ec != std::errc{} || ptr != last || score < UNRECORDED_SCORE) return std::nullopt;
            return score;
        }

        [[nodiscard]] std::wstring FormatRate(const std::optional<double>& rate)
        {
            if (!rate.has_value()) return std::wstring{ NO_RATE_TEXT };
            return Core::NumberFormat::FormatNumber(*rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX };
        }
    }

    TrackerView::TrackerView(Core::Platform& platform)
        : m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }
        , m_syncButton{ m_host.Emplace<UI::Button>(std::wstring{ SYNC_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_addWeekButton{ m_host.Emplace<UI::Button>(std::wstring{ ADD_WEEK_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_addSeasonButton{ m_host.Emplace<UI::Button>(std::wstring{ ADD_SEASON_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_seasonBox{ m_host.Emplace<UI::ComboBox>() }
        , m_periodButton{ m_host.Emplace<UI::Button>(std::wstring{ PERIOD_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_unlockButton{ m_host.Emplace<UI::Button>(std::wstring{ UNLOCK_WEEK_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_scheduleLabel{ m_host.Emplace<UI::Label>(std::wstring{}, SCHEDULE_TEXT_STYLE, Theme::HIGHLIGHT_TEXT_COLOR) }
        , m_grid{ m_host.Emplace<UI::Grid>(platform, std::vector<UI::GridColumn>{}, std::size_t{}) }
        , m_chart{ m_host.Emplace<UI::LineChart>() }
    {
        // 캐릭터 이름은 한글 → IME 켜 둠, Enter = 원정대 동기화
        m_nameBox.SetOnSubmit([this]
        {
            if (const auto onSyncRoster{ m_onSyncRoster }) onSyncRoster(m_nameBox.GetText());
        });
        m_syncButton.SetOnClick([this]
        {
            if (const auto onSyncRoster{ m_onSyncRoster }) onSyncRoster(m_nameBox.GetText());
        });
        m_addWeekButton.SetOnClick([this]
        {
            if (const auto onAddWeek{ m_onAddWeek }) onAddWeek();
        });
        m_addSeasonButton.SetOnClick([this]
        {
            if (const auto onAddSeason{ m_onAddSeason }) onAddSeason();
        });
        m_seasonBox.SetOnSelectionChanged([this]
        {
            if (const auto onSeasonSelected{ m_onSeasonSelected }) onSeasonSelected();
        });
        m_periodButton.SetOnClick([this]
        {
            if (const auto onEditPeriod{ m_onEditPeriod }) onEditPeriod();
        });
        m_unlockButton.SetOnClick([this]
        {
            if (const auto onUnlockWeek{ m_onUnlockWeek }) onUnlockWeek();
        });
        SetPeriodMode(false);   // 처음엔 기간 없는 시즌 기준 (매니저가 시즌을 고르면 다시 정함)

        // 표: 두 줄 머리글 + 숫자 칸 (IME 끔 → 한/영 상태와 상관없이 숫자 입력)
        m_grid.SetHeaderHeight(HEADER_HEIGHT);
        m_grid.SetEditInput(false, IsScoreCharacter);
        m_grid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text) { HandleCellEdited(row, column, text); });

        // 그래프: 원본 낙원력 그래프 (주황 곡선 + "평균 N" + 직전 기록 대비 + 최고 기록선)
        m_chart.SetSeriesColor(Theme::PARADISE_CHART_LINE_COLOR);
        m_chart.SetValuePrefix(std::wstring{ CHART_VALUE_PREFIX });
        m_chart.SetChangeShown(true);
        m_chart.SetPeakLineShown(true);

        SetCharacterColumns({}, 0);
    }

    TrackerView::~TrackerView() = default;

    void TrackerView::SetOnSyncRoster(std::function<void(const std::wstring& characterName)> onSyncRoster)
    {
        m_onSyncRoster = std::move(onSyncRoster);
    }

    void TrackerView::SetOnAddWeek(std::function<void()> onAddWeek)
    {
        m_onAddWeek = std::move(onAddWeek);
    }

    void TrackerView::SetOnAddSeason(std::function<void()> onAddSeason)
    {
        m_onAddSeason = std::move(onAddSeason);
    }

    void TrackerView::SetOnEditPeriod(std::function<void()> onEditPeriod)
    {
        m_onEditPeriod = std::move(onEditPeriod);
    }

    void TrackerView::SetOnUnlockWeek(std::function<void()> onUnlockWeek)
    {
        m_onUnlockWeek = std::move(onUnlockWeek);
    }

    void TrackerView::SetOnSeasonSelected(std::function<void()> onSeasonSelected)
    {
        m_onSeasonSelected = std::move(onSeasonSelected);
    }

    void TrackerView::SetOnScoreEdited(std::function<void(std::size_t weekIndex, std::size_t characterIndex, std::optional<double> score)> onScoreEdited)
    {
        m_onScoreEdited = std::move(onScoreEdited);
    }

    void TrackerView::SetSeasonItems(std::vector<std::wstring> items, std::optional<std::size_t> selectedIndex)
    {
        m_seasonBox.SetItems(std::move(items));
        m_seasonBox.SetSelectedIndex(selectedIndex);
    }

    std::optional<std::size_t> TrackerView::GetSelectedSeasonIndex() const noexcept
    {
        return m_seasonBox.GetSelectedIndex();
    }

    // 주차 | 캐릭터들 | 통계 4열 (원본 RefreshGridStructure)
    void TrackerView::SetCharacterColumns(std::vector<std::wstring> headers, std::size_t editableCount)
    {
        m_characterCount = headers.size();
        m_editableCount = std::min(editableCount, m_characterCount);

        std::vector<UI::GridColumn> columns;
        columns.reserve(FIRST_CHAR_COL + m_characterCount + STAT_COLUMN_COUNT);
        columns.push_back(UI::GridColumn{ std::wstring{ WEEK_HEADER }, WEEK_COLUMN_WEIGHT, Core::TextAlign::CENTER });
        for (std::wstring& header : headers) columns.push_back(UI::GridColumn{ std::move(header), CHARACTER_COLUMN_WEIGHT, Core::TextAlign::TRAILING });
        for (const std::wstring_view header : STAT_HEADERS) columns.push_back(UI::GridColumn{ std::wstring{ header }, STAT_COLUMN_WEIGHT, Core::TextAlign::TRAILING });

        m_grid.SetColumns(std::move(columns));
        for (std::size_t row{}; row < m_grid.GetRowCount(); ++row) ApplyRowStyle(row);
    }

    void TrackerView::SetWeekCount(std::size_t weekCount)
    {
        m_grid.SetRowCount(weekCount);
        for (std::size_t row{}; row < weekCount; ++row) ApplyRowStyle(row);
        m_hasLayout = false;   // 행 수에 따라 표 높이가 바뀜 → 다음 그리기에서 다시 배치
    }

    void TrackerView::ShowWeekLabel(std::size_t weekIndex, std::wstring label)
    {
        m_grid.SetCellText(weekIndex, COL_WEEK, std::move(label));
    }

    void TrackerView::ShowScore(std::size_t weekIndex, std::size_t characterIndex, double score)
    {
        if (characterIndex >= m_characterCount) throw std::out_of_range{ "캐릭터 번호가 범위를 벗어났어요!" };   // 경계 검사
        m_grid.SetCellText(weekIndex, FIRST_CHAR_COL + characterIndex, Core::NumberFormat::FormatNumber(score, SCORE_DECIMALS));
    }

    void TrackerView::ShowStats(std::size_t weekIndex, const WeekStats& stats)
    {
        m_grid.SetCellText(weekIndex, GetStatColumn(STAT_TOTAL_OFFSET), Core::NumberFormat::FormatNumber(stats.m_total, SCORE_DECIMALS));
        m_grid.SetCellText(weekIndex, GetStatColumn(STAT_AVERAGE_OFFSET), Core::NumberFormat::FormatNumber(stats.m_average, SCORE_DECIMALS));
        m_grid.SetCellText(weekIndex, GetStatColumn(STAT_PREV_RATE_OFFSET), FormatRate(stats.m_previousRate));
        m_grid.SetCellText(weekIndex, GetStatColumn(STAT_FIRST_RATE_OFFSET), FormatRate(stats.m_firstRate));
    }

    void TrackerView::CommitEdit()
    {
        m_grid.CommitEdit();
    }

    void TrackerView::SetWeekLock(std::optional<std::vector<bool>> editableWeeks, std::optional<std::size_t> currentWeek)
    {
        m_editableWeeks = std::move(editableWeeks);
        m_currentWeek = currentWeek;
        for (std::size_t row{}; row < m_grid.GetRowCount(); ++row) ApplyRowStyle(row);
    }

    // 현재 원정대 첫 캐릭터 칸 (캐릭터가 없으면 주차 칸)
    void TrackerView::FocusWeek(std::size_t weekIndex)
    {
        m_grid.SelectCell(weekIndex, m_editableCount > 0 ? FIRST_CHAR_COL : COL_WEEK);
    }

    std::optional<std::size_t> TrackerView::GetSelectedWeek() const noexcept
    {
        return m_grid.GetSelectedRow();
    }

    void TrackerView::SetPeriodMode(bool hasPeriod)
    {
        m_addWeekButton.SetEnabled(!hasPeriod);   // 기간 있는 시즌은 주차가 날짜에 맞춰 자동으로 늘어남
        m_periodButton.SetEnabled(hasPeriod);
        m_unlockButton.SetEnabled(hasPeriod);
    }

    void TrackerView::SetScheduleText(std::wstring text)
    {
        if (text.empty() != m_scheduleLabel.GetText().empty()) m_hasLayout = false;   // 안내 줄이 생기거나 사라지면 다시 배치
        m_scheduleLabel.SetText(std::move(text));
    }

    void TrackerView::SetChart(std::wstring title, std::vector<UI::ChartPoint> points)
    {
        m_chart.SetTitle(std::move(title));
        m_chart.SetPoints(std::move(points));
    }

    void TrackerView::SetSyncBusy(bool isBusy)
    {
        m_syncButton.SetEnabled(!isBusy);
        m_syncButton.SetText(std::wstring{ isBusy ? SYNC_BUSY_TEXT : SYNC_TEXT });
    }

    void TrackerView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void TrackerView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        m_host.Render(renderer);
    }

    bool TrackerView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape TrackerView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void TrackerView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void TrackerView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 도구 막대(위) → 표(행 수만큼, 최대 높이 비율까지) → 그래프(남은 높이 전부)
    void TrackerView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ contentArea.m_right - PAGE_PADDING };
        const float toolbarTop{ contentArea.m_top + PAGE_PADDING };
        const float toolbarBottom{ toolbarTop + Theme::CONTROL_HEIGHT };

        // 왼쪽: 닉네임 · 원정대 동기화 | 주차 추가 · 시즌 추가 · 시즌 선택 · 시즌 기간 · 지난 주차 수정
        float x{ left };
        m_nameBox.SetBounds(Core::RectF{ x, toolbarTop, x + NAME_BOX_WIDTH, toolbarBottom });
        x += NAME_BOX_WIDTH + CONTROL_GAP;
        m_syncButton.SetBounds(Core::RectF{ x, toolbarTop, x + SYNC_BUTTON_WIDTH, toolbarBottom });
        x += SYNC_BUTTON_WIDTH + GROUP_GAP;
        m_addWeekButton.SetBounds(Core::RectF{ x, toolbarTop, x + ADD_BUTTON_WIDTH, toolbarBottom });
        x += ADD_BUTTON_WIDTH + CONTROL_GAP;
        m_addSeasonButton.SetBounds(Core::RectF{ x, toolbarTop, x + ADD_BUTTON_WIDTH, toolbarBottom });
        x += ADD_BUTTON_WIDTH + CONTROL_GAP;
        m_seasonBox.SetBounds(Core::RectF{ x, toolbarTop, x + SEASON_BOX_WIDTH, toolbarBottom });
        x += SEASON_BOX_WIDTH + CONTROL_GAP;
        m_periodButton.SetBounds(Core::RectF{ x, toolbarTop, x + PERIOD_BUTTON_WIDTH, toolbarBottom });
        x += PERIOD_BUTTON_WIDTH + CONTROL_GAP;
        m_unlockButton.SetBounds(Core::RectF{ x, toolbarTop, x + UNLOCK_BUTTON_WIDTH, toolbarBottom });

        // 도구 막대 아래: 이번 주 입력 마감 안내 (기간 있는 시즌만, 없으면 줄을 비움)
        float sectionTop{ toolbarBottom };
        if (!m_scheduleLabel.GetText().empty())
        {
            const float lineTop{ toolbarBottom + SCHEDULE_LINE_GAP };
            m_scheduleLabel.SetBounds(Core::RectF{ left, lineTop, right, lineTop + SCHEDULE_LINE_HEIGHT });
            sectionTop = lineTop + SCHEDULE_LINE_HEIGHT;
        }
        else
        {
            m_scheduleLabel.SetBounds(Core::RectF{ left, toolbarBottom, left, toolbarBottom });
        }

        // 표: 모든 행이 보이는 높이 (너무 많으면 비율까지만 → 휠 · 방향키로 스크롤)
        const float gridTop{ sectionTop + SECTION_GAP };
        const float availableHeight{ std::max(0.0f, contentArea.m_bottom - PAGE_PADDING - gridTop) };
        const float minGridHeight{ HEADER_HEIGHT + Theme::GRID_ROW_HEIGHT * static_cast<float>(GRID_MIN_VISIBLE_ROWS) };
        const float maxGridHeight{ std::max(minGridHeight, availableHeight * GRID_AREA_RATIO) };
        const float gridBottom{ gridTop + std::clamp(m_grid.GetPreferredHeight(), minGridHeight, maxGridHeight) };
        m_grid.SetBounds(Core::RectF{ left, gridTop, right, gridBottom });

        const float chartTop{ gridBottom + SECTION_GAP };
        const float chartBottom{ std::max(chartTop, contentArea.m_bottom - PAGE_PADDING) };
        m_chart.SetBounds(Core::RectF{ left, chartTop, right, chartBottom });
    }

    // 주차 열: 머리글 톤 / 현재 원정대: 입력 가능 / 원정대 제외: 읽기 전용 회색 / 통계: 다크 청록 (원본 열 스타일)
    void TrackerView::ApplyRowStyle(std::size_t row)
    {
        // 이번 주차는 주차 칸을 강조색으로
        const bool isCurrentWeek{ m_currentWeek == row };
        m_grid.SetCellStyle(row, COL_WEEK, isCurrentWeek ? UI::GridCellStyle{ Theme::HIGHLIGHT_BACK_COLOR, Theme::HIGHLIGHT_TEXT_COLOR, Core::FontWeight::BOLD }
            : UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR });

        const bool isWeekEditable{ IsEditableWeek(row) };
        for (std::size_t index{}; index < m_characterCount; ++index)
        {
            const std::size_t column{ FIRST_CHAR_COL + index };
            const bool isEditable{ isWeekEditable && IsEditableColumn(column) };
            m_grid.SetCellEditable(row, column, isEditable);
            m_grid.SetCellStyle(row, column, isEditable ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR }
                : UI::GridCellStyle{ Theme::READONLY_BACK_COLOR, Theme::MUTED_TEXT_COLOR });
        }

        for (std::size_t offset{}; offset < STAT_COLUMN_COUNT; ++offset)
        {
            m_grid.SetCellStyle(row, GetStatColumn(offset), UI::GridCellStyle{ Theme::TOTAL_BACK_COLOR, Theme::TOTAL_TEXT_COLOR });
        }
    }

    // 낙원력 칸 편집 확정 → 해석해서 알림 (보기 좋은 형식으로 다시 쓰는 일은 받는 쪽이 ShowScore로)
    void TrackerView::HandleCellEdited(std::size_t row, std::size_t column, const std::wstring& text)
    {
        if (!IsEditableColumn(column) || !IsEditableWeek(row)) return;
        if (const auto onScoreEdited{ m_onScoreEdited }) onScoreEdited(row, column - FIRST_CHAR_COL, ParseScore(text));
    }

    std::size_t TrackerView::GetStatColumn(std::size_t statOffset) const noexcept
    {
        return FIRST_CHAR_COL + m_characterCount + statOffset;
    }

    bool TrackerView::IsEditableColumn(std::size_t column) const noexcept
    {
        return column >= FIRST_CHAR_COL && column < FIRST_CHAR_COL + m_editableCount;
    }

    bool TrackerView::IsEditableWeek(std::size_t row) const noexcept
    {
        if (!m_editableWeeks.has_value()) return true;                     // 기간 없는 시즌
        return row < m_editableWeeks->size() && (*m_editableWeeks)[row];  // 경계 검사 (목록 밖 주차는 잠금)
    }
}
