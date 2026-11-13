module LostArkMultiTool.Views.TrackerView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.PageLayout;
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
        constexpr std::wstring_view SEARCH_PLACEHOLDER{ L"캐릭터 이름 · 직업 검색" };
        constexpr std::wstring_view CLEAR_TEXT{ L"✕" };                                   // 검색어 지우기
        constexpr std::wstring_view FILTER_COUNT_FORMAT{ L"{}개의 캐릭터" };              // 검색 줄 오른쪽
        constexpr std::wstring_view FILTER_COUNT_FILTERED_FORMAT{ L"{}개 / 전체 {}개의 캐릭터" };   // 검색어로 걸러졌을 때
        constexpr std::wstring_view ROSTER_EYEBROW{ L"CHARACTER ROSTER" };
        constexpr std::wstring_view ROSTER_TITLE{ L"캐릭터별 낙원력" };
        constexpr std::wstring_view GROWTH_EYEBROW{ L"GROWTH HISTORY" };
        constexpr std::wstring_view GROWTH_TITLE{ L"원정대 평균 낙원력 추이" };
        constexpr std::wstring_view RECORD_EYEBROW{ L"WEEKLY RECORD" };
        constexpr std::wstring_view RECORD_TITLE{ L"주차별 낙원력 입력" };
        constexpr std::wstring_view RECORD_HINT{ L"칸을 눌러 입력 · 자동 저장" };
        constexpr std::wstring_view EMPTY_ROSTER_TITLE{ L"표시할 캐릭터가 없어요" };
        constexpr std::wstring_view EMPTY_ROSTER_SEARCH_HINT{ L"검색어를 지우거나 다른 이름 · 직업으로 찾아보세요." };
        constexpr std::wstring_view EMPTY_ROSTER_DATA_HINT{ L"원정대 동기화 후 아래 표에 낙원력을 입력하면 여기에 보여요." };
        constexpr std::array<std::wstring_view, 4> RANGE_ITEMS{ L"전체 주차", L"최근 4주", L"최근 8주", L"최근 12주" };
        constexpr std::array<std::size_t, 4> RANGE_WEEKS{ 0, 4, 8, 12 };                   // 선택 번호 → 주차 수 (0 = 전체)
        constexpr std::array<std::wstring_view, 6> ROSTER_HEADERS{ L"#", L"캐릭터", L"직업 · 레벨", L"낙원력", L"전주 대비", L"추이" };
        constexpr std::array<float, 6> ROSTER_WEIGHTS{ 0.4f, 1.5f, 1.5f, 1.1f, 1.0f, 1.2f };   // 목록 표 열 너비 비율
        constexpr std::size_t ROSTER_COL_RANK{ 0 };
        constexpr std::size_t ROSTER_COL_NAME{ 1 };
        constexpr std::size_t ROSTER_COL_DETAIL{ 2 };
        constexpr std::size_t ROSTER_COL_SCORE{ 3 };
        constexpr std::size_t ROSTER_COL_RATE{ 4 };
        constexpr std::size_t ROSTER_COL_TREND{ 5 };
        constexpr std::wstring_view RANK_FORMAT{ L"{:02}" };
        constexpr std::wstring_view SIGNED_RATE_FORMAT{ L"{}{}%" };
        constexpr std::wstring_view PLUS_SIGN{ L"+" };
        constexpr std::array<Core::Color, 5> BADGE_COLORS{ Theme::ACCENT_COLOR, Theme::GEM_CHART_LINE_COLOR, Theme::HIGHLIGHT_TEXT_COLOR,
            Theme::DOWN_TEXT_COLOR, Theme::SPARKLINE_DOWN_COLOR };   // 순위마다 돌아가며 쓰는 이니셜 배지 색

        // ===== 배치 (DIP) =====
        constexpr float CONTROL_GAP{ 8.0f };
        constexpr float GROUP_GAP{ 16.0f };              // 원정대 동기화 묶음과 주차 · 시즌 묶음 사이
        constexpr float NAME_BOX_WIDTH{ 180.0f };
        constexpr float SYNC_BUTTON_WIDTH{ 120.0f };
        constexpr float ADD_BUTTON_WIDTH{ 96.0f };
        constexpr float SEASON_BOX_WIDTH{ 120.0f };
        constexpr float PERIOD_BUTTON_WIDTH{ 96.0f };
        constexpr float UNLOCK_BUTTON_WIDTH{ 120.0f };
        constexpr float SCHEDULE_LINE_HEIGHT{ 20.0f };   // 이번 주 입력 마감 안내 줄
        constexpr float SCHEDULE_LINE_GAP{ 4.0f };       // 도구 줄과 안내 줄 사이
        constexpr float SCHEDULE_BOTTOM_PAD{ 8.0f };     // 안내 줄 아래 카드 여백
        constexpr Core::TextStyle SCHEDULE_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };
        constexpr Core::TextStyle FILTER_COUNT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };   // "N개의 캐릭터"
        constexpr Core::TextStyle RECORD_HINT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::TRAILING };  // 입력 카드 안내
        constexpr float HEADER_HEIGHT{ 44.0f };          // 두 줄 머리글 "직업\n(Lv.1740)"
        constexpr float PAGE_VERTICAL_PADDING{ 8.0f };   // 본문 위 · 아래 여백
        constexpr float SEARCH_BOX_WIDTH{ 300.0f };
        constexpr float CLEAR_BUTTON_WIDTH{ 32.0f };
        constexpr float RANGE_BOX_WIDTH{ 110.0f };
        constexpr float ROSTER_ROW_HEIGHT{ 320.0f };     // 목록 · 추이 카드 높이
        constexpr float ROSTER_COLUMN_WEIGHT{ 1.0f };    // 목록 카드와 추이 카드의 너비 비율
        constexpr float GROWTH_COLUMN_WEIGHT{ 1.15f };
        constexpr float ROSTER_HEADER_HEIGHT{ 28.0f };   // 목록 표 머리글 행
        constexpr float ROSTER_GRID_ROW_HEIGHT{ 44.0f };  // 목록 표 행 (배지 · 추이 선이 들어가는 높이)
        constexpr float BODY_INSET{ 12.0f };             // 카드 머리말 아래 · 안쪽 여백
        constexpr float RECORD_MIN_HEIGHT{ 190.0f };     // 입력 카드 최소 높이
        constexpr std::size_t GRID_MIN_VISIBLE_ROWS{ 3 };
        constexpr float HALF{ 0.5f };                    // 가운데 계산
        constexpr std::size_t FIRST_BADGE_CHARACTER{ 0 };   // 이니셜 배지에 쓸 글자 위치

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

        // 대소문자를 가리지 않고 글자가 들어 있는지 (검색어가 비어 있으면 항상 true)
        [[nodiscard]] bool ContainsIgnoreCase(std::wstring_view text, std::wstring_view keyword)
        {
            if (keyword.empty()) return true;
            const auto isSame{ [](wchar_t left, wchar_t right) { return std::towlower(left) == std::towlower(right); } };
            return !std::ranges::search(text, keyword, isSame).empty();
        }

        // 증감률 글자 "+12.8%" / "-1.3%" (없으면 "-")
        [[nodiscard]] std::wstring FormatSignedRate(const std::optional<double>& rate)
        {
            if (!rate.has_value()) return std::wstring{ NO_RATE_TEXT };
            return std::format(SIGNED_RATE_FORMAT, *rate > 0.0 ? PLUS_SIGN : std::wstring_view{}, Core::NumberFormat::FormatNumber(*rate, RATE_DECIMALS));
        }

        // 목록 표 열 → 비율 열 목록 (머리글 · 너비 비율 · 정렬)
        [[nodiscard]] std::vector<UI::GridColumn> MakeRosterColumns()
        {
            std::vector<UI::GridColumn> columns;
            for (std::size_t column{}; column < ROSTER_HEADERS.size(); ++column)
            {
                const bool isLeading{ column == ROSTER_COL_NAME || column == ROSTER_COL_DETAIL || column == ROSTER_COL_TREND };
                columns.push_back(UI::GridColumn{ std::wstring{ ROSTER_HEADERS[column] }, ROSTER_WEIGHTS[column],
                    column == ROSTER_COL_RANK ? Core::TextAlign::CENTER : isLeading ? Core::TextAlign::LEADING : Core::TextAlign::TRAILING });
            }
            return columns;
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
        , m_searchBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ SEARCH_PLACEHOLDER }) }
        , m_clearButton{ m_host.Emplace<UI::Button>(std::wstring{ CLEAR_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_rosterGrid{ m_host.Emplace<UI::Grid>(platform, MakeRosterColumns(), std::size_t{}) }
        , m_rangeBox{ m_host.Emplace<UI::ComboBox>() }
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

        // 검색: 글자가 바뀔 때마다 목록을 바로 걸러 냄 · ✕ = 한 번에 지우고 다시 입력 상태로
        m_searchBox.SetOnTextChanged([this] { RebuildRoster(); });
        m_clearButton.SetOnClick([this]
        {
            m_searchBox.SetText(std::wstring{});
            m_host.SetFocus(&m_searchBox);
            RebuildRoster();
        });

        // 캐릭터별 목록 표: 읽기 전용, 행 전체 선택 · 배지 · 추이 선이 들어가는 행 높이
        m_rosterGrid.SetHeaderHeight(ROSTER_HEADER_HEIGHT);
        m_rosterGrid.SetRowHeight(ROSTER_GRID_ROW_HEIGHT);
        m_rosterGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);

        // 그래프 기간: 전체 · 최근 4 · 8 · 12주
        std::vector<std::wstring> rangeItems;
        for (const std::wstring_view item : RANGE_ITEMS) rangeItems.emplace_back(item);
        m_rangeBox.SetItems(std::move(rangeItems));
        m_rangeBox.SetSelectedIndex(std::size_t{ 0 });
        m_rangeBox.SetOnSelectionChanged([this]
        {
            if (const auto onChartRangeChanged{ m_onChartRangeChanged }) onChartRangeChanged();
        });

        // 표: 두 줄 머리글 + 숫자 칸 (IME 끔 → 한/영 상태와 상관없이 숫자 입력)
        m_grid.SetShowVerticalLines(true);   // 입력 표는 칸 경계를 보이게
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

    void TrackerView::SetOnChartRangeChanged(std::function<void()> onChartRangeChanged)
    {
        m_onChartRangeChanged = std::move(onChartRangeChanged);
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

    // 선택한 기간의 최근 주차 수 (전체면 nullopt)
    std::optional<std::size_t> TrackerView::GetChartWeekLimit() const noexcept
    {
        const std::optional<std::size_t> selected{ m_rangeBox.GetSelectedIndex() };
        if (!selected.has_value() || *selected >= RANGE_WEEKS.size() || RANGE_WEEKS[*selected] == 0) return std::nullopt;   // 경계 검사
        return RANGE_WEEKS[*selected];
    }

    void TrackerView::SetStatCards(std::vector<PageLayout::StatCard> cards)
    {
        m_statCards = std::move(cards);
    }

    void TrackerView::SetRoster(std::vector<RosterEntry> entries)
    {
        m_rosterEntries = std::move(entries);
        RebuildRoster();
    }

    // 검색어(이름 · 직업)에 맞는 캐릭터만 목록 표에 채움 (순위 번호는 전체 기준 그대로)
    void TrackerView::RebuildRoster()
    {
        const std::wstring_view keyword{ m_searchBox.GetText() };

        std::vector<std::size_t> shown;   // 보일 캐릭터의 전체 목록 번호
        for (std::size_t index{}; index < m_rosterEntries.size(); ++index)
        {
            const RosterEntry& entry{ m_rosterEntries[index] };
            if (ContainsIgnoreCase(entry.m_name, keyword) || ContainsIgnoreCase(entry.m_detail, keyword)) shown.push_back(index);
        }
        m_shownRosterCount = shown.size();

        m_rosterGrid.SetRowCount(shown.size());
        const UI::GridCellStyle baseStyle{ Theme::CARD_BACK_COLOR, Theme::TEXT_COLOR };   // 카드와 같은 바탕
        for (std::size_t row{}; row < shown.size(); ++row)
        {
            const std::size_t rank{ shown[row] };
            const RosterEntry& entry{ m_rosterEntries[rank] };

            m_rosterGrid.SetCellText(row, ROSTER_COL_RANK, std::format(RANK_FORMAT, rank + 1));
            m_rosterGrid.SetCellStyle(row, ROSTER_COL_RANK, UI::GridCellStyle{ Theme::CARD_BACK_COLOR, Theme::MUTED_TEXT_COLOR });

            const std::wstring initial{ entry.m_name.empty() ? std::wstring{} : entry.m_name.substr(FIRST_BADGE_CHARACTER, 1) };   // 이니셜 배지
            m_rosterGrid.SetCellText(row, ROSTER_COL_NAME, entry.m_name);
            m_rosterGrid.SetCellStyle(row, ROSTER_COL_NAME, UI::GridCellStyle{ Theme::CARD_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD,
                UI::GridDecoration::DECORATION_BADGE, initial, BADGE_COLORS[rank % BADGE_COLORS.size()] });

            m_rosterGrid.SetCellText(row, ROSTER_COL_DETAIL, entry.m_detail);
            m_rosterGrid.SetCellStyle(row, ROSTER_COL_DETAIL, UI::GridCellStyle{ Theme::CARD_BACK_COLOR, Theme::MUTED_TEXT_COLOR });

            m_rosterGrid.SetCellText(row, ROSTER_COL_SCORE, Core::NumberFormat::FormatNumber(entry.m_score, SCORE_DECIMALS));
            m_rosterGrid.SetCellStyle(row, ROSTER_COL_SCORE, UI::GridCellStyle{ Theme::CARD_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD });

            Core::Color rateColor{ Theme::MUTED_TEXT_COLOR };   // 등락 색 (변동 없음 · 비교 대상 없음 = 회색)
            if (entry.m_changeRate.has_value() && *entry.m_changeRate > 0.0) rateColor = Theme::SPARKLINE_UP_COLOR;
            else if (entry.m_changeRate.has_value() && *entry.m_changeRate < 0.0) rateColor = Theme::SPARKLINE_DOWN_COLOR;
            m_rosterGrid.SetCellText(row, ROSTER_COL_RATE, FormatSignedRate(entry.m_changeRate));
            m_rosterGrid.SetCellStyle(row, ROSTER_COL_RATE, UI::GridCellStyle{ Theme::CARD_BACK_COLOR, rateColor, Core::FontWeight::BOLD });

            m_rosterGrid.SetCellText(row, ROSTER_COL_TREND, std::wstring{});
            m_rosterGrid.SetCellStyle(row, ROSTER_COL_TREND, baseStyle);
            m_rosterGrid.SetCellSparkline(row, ROSTER_COL_TREND, entry.m_history);   // 기록이 2개 이상일 때만 그려짐
        }
        m_rosterGrid.SetVisible(!shown.empty());   // 비어 있으면 표 대신 안내 문구를 그림
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

    // 카드 바탕 · 머리말 · 통계 카드를 먼저 그리고 그 위에 부품(입력칸 · 표 · 그래프)을 그림
    void TrackerView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);

        PageLayout::DrawCard(renderer, m_toolbarRect);   // 시즌 · 동기화 도구 카드

        for (std::size_t index{}; index < m_statCards.size() && index < m_statRects.size(); ++index)   // 통계 카드 (실제 기록으로 계산한 값)
        {
            PageLayout::DrawStatCard(renderer, m_statRects[index], m_statCards[index]);
        }

        PageLayout::DrawCard(renderer, m_filterRect);   // 검색 줄
        const std::wstring countText{ m_shownRosterCount == m_rosterEntries.size() ? std::format(FILTER_COUNT_FORMAT, m_shownRosterCount)
            : std::format(FILTER_COUNT_FILTERED_FORMAT, m_shownRosterCount, m_rosterEntries.size()) };
        renderer.DrawString(countText, m_filterCountRect, Theme::ACCENT_COLOR, FILTER_COUNT_STYLE);

        const Core::RectF rosterBody{ PageLayout::DrawCardWithHeader(renderer, m_rosterRect, ROSTER_EYEBROW, ROSTER_TITLE) };   // 캐릭터별 낙원력 카드
        if (m_shownRosterCount == 0)   // 보여 줄 캐릭터가 없으면 해결 방법 안내
        {
            PageLayout::DrawEmptyState(renderer, rosterBody, EMPTY_ROSTER_TITLE,
                m_searchBox.GetText().empty() ? EMPTY_ROSTER_DATA_HINT : EMPTY_ROSTER_SEARCH_HINT);
        }
        PageLayout::DrawCardWithHeader(renderer, m_growthRect, GROWTH_EYEBROW, GROWTH_TITLE);   // 추이 카드

        PageLayout::DrawCardWithHeader(renderer, m_recordRect, RECORD_EYEBROW, RECORD_TITLE, RECORD_HINT);   // 주차별 입력 카드

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

    // 도구 카드 → 통계 카드 4장 → 검색 줄 → [캐릭터별 목록 | 추이 그래프] → 주차별 입력 표(남은 높이)
    void TrackerView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const Core::RectF column{ PageLayout::GetPageColumn(contentArea) };   // 가운데 정렬된 본문 열
        const float pageLeft{ column.m_left };
        const float pageRight{ column.m_right };
        float y{ column.m_top + PAGE_VERTICAL_PADDING };

        // ===== 도구 카드 (닉네임 · 동기화 · 주차 · 시즌 · 안내 줄) =====
        const bool hasSchedule{ !m_scheduleLabel.GetText().empty() };
        const float toolbarCardHeight{ PageLayout::FILTER_BAR_HEIGHT + (hasSchedule ? SCHEDULE_LINE_HEIGHT + SCHEDULE_BOTTOM_PAD - SCHEDULE_LINE_GAP : 0.0f) };
        m_toolbarRect = Core::RectF{ pageLeft, y, pageRight, y + toolbarCardHeight };
        const float left{ pageLeft + PageLayout::CARD_PADDING };
        const float right{ pageRight - PageLayout::CARD_PADDING };
        const float toolbarTop{ y + (PageLayout::FILTER_BAR_HEIGHT - PageLayout::CONTROL_HEIGHT) * HALF };
        const float toolbarBottom{ toolbarTop + PageLayout::CONTROL_HEIGHT };

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

        // 도구 줄 아래: 이번 주 입력 마감 안내 (기간 있는 시즌만, 없으면 줄을 비움)
        if (hasSchedule)
        {
            const float lineTop{ y + PageLayout::FILTER_BAR_HEIGHT - SCHEDULE_LINE_GAP };
            m_scheduleLabel.SetBounds(Core::RectF{ left, lineTop, right, lineTop + SCHEDULE_LINE_HEIGHT });
        }
        else
        {
            m_scheduleLabel.SetBounds(Core::RectF{ left, toolbarBottom, left, toolbarBottom });
        }
        y += toolbarCardHeight + PageLayout::CARD_GAP;

        // ===== 통계 카드 4장 =====
        const std::vector<Core::RectF> statColumns{ PageLayout::SplitColumns(Core::RectF{ pageLeft, y, pageRight, y + PageLayout::STAT_CARD_HEIGHT },
            m_statRects.size(), PageLayout::CARD_GAP) };
        for (std::size_t index{}; index < m_statRects.size() && index < statColumns.size(); ++index) m_statRects[index] = statColumns[index];
        y += PageLayout::STAT_CARD_HEIGHT + PageLayout::CARD_GAP;

        // ===== 검색 줄: [검색칸][✕] ........ N개의 캐릭터 =====
        m_filterRect = Core::RectF{ pageLeft, y, pageRight, y + PageLayout::FILTER_BAR_HEIGHT };
        const float filterTop{ y + (PageLayout::FILTER_BAR_HEIGHT - PageLayout::CONTROL_HEIGHT) * HALF };
        const float filterBottom{ filterTop + PageLayout::CONTROL_HEIGHT };
        m_searchBox.SetBounds(Core::RectF{ left, filterTop, left + SEARCH_BOX_WIDTH, filterBottom });
        m_clearButton.SetBounds(Core::RectF{ left + SEARCH_BOX_WIDTH + CONTROL_GAP, filterTop, left + SEARCH_BOX_WIDTH + CONTROL_GAP + CLEAR_BUTTON_WIDTH, filterBottom });
        m_filterCountRect = Core::RectF{ right - SEARCH_BOX_WIDTH, y, right, y + PageLayout::FILTER_BAR_HEIGHT };
        y += PageLayout::FILTER_BAR_HEIGHT + PageLayout::CARD_GAP;

        // ===== 캐릭터별 목록 | 추이 그래프 =====
        const std::array<float, 2> rowWeights{ ROSTER_COLUMN_WEIGHT, GROWTH_COLUMN_WEIGHT };
        const std::vector<Core::RectF> rowColumns{ PageLayout::SplitColumns(Core::RectF{ pageLeft, y, pageRight, y + ROSTER_ROW_HEIGHT },
            rowWeights.size(), PageLayout::CARD_GAP, rowWeights) };
        m_rosterRect = rowColumns[0];
        m_growthRect = rowColumns[1];
        m_rosterGrid.SetBounds(Core::RectF{ m_rosterRect.m_left + PageLayout::CARD_PADDING, m_rosterRect.m_top + PageLayout::CARD_HEADER_HEIGHT + BODY_INSET,
            m_rosterRect.m_right - PageLayout::CARD_PADDING, m_rosterRect.m_bottom - BODY_INSET });
        const float rangeMiddle{ m_growthRect.m_top + PageLayout::CARD_HEADER_HEIGHT * HALF };   // 머리말 오른쪽 기간 선택
        m_rangeBox.SetBounds(Core::RectF{ m_growthRect.m_right - PageLayout::CARD_PADDING - RANGE_BOX_WIDTH, rangeMiddle - PageLayout::CONTROL_HEIGHT * HALF,
            m_growthRect.m_right - PageLayout::CARD_PADDING, rangeMiddle + PageLayout::CONTROL_HEIGHT * HALF });
        m_chart.SetBounds(Core::RectF{ m_growthRect.m_left + BODY_INSET, m_growthRect.m_top + PageLayout::CARD_HEADER_HEIGHT + BODY_INSET,
            m_growthRect.m_right - BODY_INSET, m_growthRect.m_bottom - BODY_INSET });
        y += ROSTER_ROW_HEIGHT + PageLayout::CARD_GAP;

        // ===== 주차별 낙원력 입력 표: 모든 행이 보이는 높이 (남은 높이까지, 넘치면 휠 · 방향키로 스크롤) =====
        const float recordChrome{ PageLayout::CARD_HEADER_HEIGHT + BODY_INSET + BODY_INSET };   // 머리말 + 표 위 · 아래 여백
        const float availableGrid{ std::max(0.0f, column.m_bottom - PAGE_VERTICAL_PADDING - y - recordChrome) };
        const float minGridHeight{ HEADER_HEIGHT + Theme::GRID_ROW_HEIGHT * static_cast<float>(GRID_MIN_VISIBLE_ROWS) };
        const float gridHeight{ std::clamp(m_grid.GetPreferredHeight(), minGridHeight, std::max(minGridHeight, availableGrid)) };
        const float recordHeight{ std::max(RECORD_MIN_HEIGHT, recordChrome + gridHeight) };
        m_recordRect = Core::RectF{ pageLeft, y, pageRight, y + recordHeight };
        const float gridTop{ y + PageLayout::CARD_HEADER_HEIGHT + BODY_INSET };
        m_grid.SetBounds(Core::RectF{ left, gridTop, right, gridTop + gridHeight });
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
