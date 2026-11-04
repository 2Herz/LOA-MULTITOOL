module LostArkMultiTool.Views.GemFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.GemCatalog;
import LostArkMultiTool.Models.GemQuote;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
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
        // ===== 최저가 표 (행 = 높은 레벨부터, 열 = 레벨 + 보석 종류) =====
        constexpr std::size_t PRICE_COL_LEVEL{ 0 };
        constexpr std::size_t PRICE_COL_FIRST_TYPE{ 1 };
        constexpr float LEVEL_COLUMN_WEIGHT{ 0.5f };     // 레벨 열은 좁게
        constexpr float PRICE_COLUMN_WEIGHT{ 1.0f };

        // ===== 알림 표 =====
        constexpr std::size_t ALARM_COL_GEM{ 0 };
        constexpr std::size_t ALARM_COL_TARGET{ 1 };
        constexpr std::size_t ALARM_COL_CURRENT{ 2 };
        constexpr std::size_t ALARM_COL_STATE{ 3 };
        constexpr std::size_t ALARM_COLUMN_COUNT{ 4 };
        constexpr float ALARM_COLUMN_WEIGHT{ 1.0f };

        constexpr int AMOUNT_DECIMALS{ 0 };              // 원본 "N0"
        constexpr std::size_t TARGET_PRICE_MAX_LENGTH{ 14 };   // "100,000,000" + 여유

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view AUTO_REFRESH_TEXT{ L"자동 갱신" };
        constexpr std::wstring_view WATCH_LABEL_TEXT{ L"감시" };
        constexpr std::wstring_view FULL_LABEL_TEXT{ L"전체" };
        constexpr std::wstring_view SECONDS_SUFFIX{ L"초" };
        constexpr std::wstring_view MINUTES_SUFFIX{ L"분" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view LEVEL_HEADER{ L"레벨" };
        constexpr std::wstring_view PRICE_HEADER_SUFFIX{ L" 최저가" };
        constexpr std::wstring_view LEVEL_SUFFIX{ L"레벨" };
        constexpr std::wstring_view NOT_QUERIED_TEXT{ L"-" };
        constexpr std::wstring_view NO_LISTING_TEXT{ L"매물 없음" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };
        constexpr std::wstring_view TARGET_LABEL_TEXT{ L"목표가" };
        constexpr std::wstring_view TARGET_PLACEHOLDER{ L"목표가 (골드)" };
        constexpr std::wstring_view ADD_ALARM_TEXT{ L"추가/변경" };
        constexpr std::wstring_view REMOVE_ALARM_TEXT{ L"삭제" };
        constexpr std::wstring_view CHART_VALUE_SUFFIX{ L" 골드" };

        constexpr std::array<std::wstring_view, ALARM_COLUMN_COUNT> ALARM_HEADERS{ L"보석", L"목표가", L"현재 최저가", L"상태" };
        constexpr std::wstring_view ALARM_STATE_OUT_OF_RANGE{ L"조회 범위 밖 (삭제 권장)" };
        constexpr std::wstring_view ALARM_STATE_NOT_CHECKED{ L"확인 전" };
        constexpr std::wstring_view ALARM_STATE_REACHED{ L"✔ 도달" };
        constexpr std::wstring_view ALARM_STATE_WAITING{ L"대기" };

        constexpr std::wstring_view PRICE_PANEL_TITLE{ L"레벨·종류별 최저가 (즉시 구매가, 등락은 직전 전체 갱신 대비)" };
        constexpr std::wstring_view ALARM_PANEL_TITLE{ L"목표가 알림 (감시 갱신 대상)" };
        constexpr std::wstring_view CHART_PANEL_TITLE{ L"시세 기록 (일별 최저가)" };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 16.0f };            // 상단 바 · 묶음 상자 사이
        constexpr float CONTROL_GAP{ 8.0f };
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float LABEL_GAP{ 4.0f };               // "감시" 글자와 콤보 상자 사이
        constexpr float REFRESH_BUTTON_WIDTH{ 110.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 96.0f };
        constexpr float SHORT_LABEL_WIDTH{ 30.0f };      // "감시" · "전체"
        constexpr float INTERVAL_BOX_WIDTH{ 80.0f };
        constexpr float STATUS_WIDTH{ 230.0f };
        constexpr float GEM_BOX_WIDTH{ 150.0f };
        constexpr float TARGET_LABEL_WIDTH{ 44.0f };
        constexpr float TARGET_BOX_WIDTH{ 130.0f };
        constexpr float ADD_BUTTON_WIDTH{ 90.0f };
        constexpr float REMOVE_BUTTON_WIDTH{ 64.0f };
        constexpr float PRICE_AREA_RATIO{ 0.45f };       // 왼쪽 표 영역 너비 비율 (원본 45%)
        constexpr float ALARM_AREA_RATIO{ 0.40f };       // 오른쪽 알림 영역 높이 비율 (원본 40%)
        constexpr float PANEL_PADDING{ 10.0f };          // 묶음 상자 테두리 안쪽 여백
        constexpr float PANEL_TITLE_LEFT{ 12.0f };

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle PANEL_TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };

        // 목표가 칸에 받을 글자 (숫자 · 콤마)
        [[nodiscard]] bool IsPriceCharacter(wchar_t character) noexcept
        {
            return (character >= L'0' && character <= L'9') || character == L',';
        }

        [[nodiscard]] std::wstring FormatAmount(std::int64_t value)
        {
            return Core::NumberFormat::FormatNumber(static_cast<double>(value), AMOUNT_DECIMALS);
        }

        // 주기 콤보 상자 항목 ("10초", "1분" …)
        template <std::size_t Count>
        [[nodiscard]] std::vector<std::wstring> MakeIntervalItems(const std::array<int, Count>& options, std::wstring_view suffix)
        {
            std::vector<std::wstring> items;
            items.reserve(Count);
            for (const int value : options) items.push_back(std::format(L"{}{}", value, suffix));
            return items;
        }

        // 선택지에서 값의 번호 (없으면 첫 항목)
        template <std::size_t Count>
        [[nodiscard]] std::size_t FindOptionIndex(const std::array<int, Count>& options, int value) noexcept
        {
            const auto found{ std::find(options.begin(), options.end(), value) };
            return found != options.end() ? static_cast<std::size_t>(std::distance(options.begin(), found)) : 0;
        }

        // 선택된 번호의 값 (선택이 없으면 기본값)
        template <std::size_t Count>
        [[nodiscard]] int GetOptionValue(const std::array<int, Count>& options, std::optional<std::size_t> index, int fallback) noexcept
        {
            return index.has_value() && *index < Count ? options[*index] : fallback;
        }

        [[nodiscard]] std::size_t GetPriceRow(int level) noexcept
        {
            return static_cast<std::size_t>(Core::APPConstants::MAX_GEM_LEVEL - level);   // 높은 레벨이 위로
        }

        [[nodiscard]] int GetRowLevel(std::size_t row) noexcept
        {
            return Core::APPConstants::MAX_GEM_LEVEL - static_cast<int>(row);
        }

        [[nodiscard]] std::wstring_view GetStateText(AlarmState state) noexcept
        {
            switch (state)
            {
                case AlarmState::ALARM_OUT_OF_RANGE: return ALARM_STATE_OUT_OF_RANGE;
                case AlarmState::ALARM_NOT_CHECKED:  return ALARM_STATE_NOT_CHECKED;
                case AlarmState::ALARM_REACHED:      return ALARM_STATE_REACHED;
                default:                             return ALARM_STATE_WAITING;
            }
        }

        // 도달: 다크 초록 + 밝은 초록 글자 / 조회 범위 밖: 흐린 글자
        [[nodiscard]] UI::GridCellStyle GetAlarmRowStyle(AlarmState state) noexcept
        {
            switch (state)
            {
                case AlarmState::ALARM_REACHED:      return UI::GridCellStyle{ Theme::REACHED_BACK_COLOR, Theme::REACHED_TEXT_COLOR };
                case AlarmState::ALARM_OUT_OF_RANGE: return UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };
                default:                             return UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
            }
        }

        [[nodiscard]] const Core::Color& GetRateLimitColor(RateLimitLevel level) noexcept
        {
            switch (level)
            {
                case RateLimitLevel::RATE_CAUTION: return Theme::CAUTION_TEXT_COLOR;
                case RateLimitLevel::RATE_ALERT:   return Theme::ALERT_TEXT_COLOR;
                default:                           return Theme::TEXT_COLOR;
            }
        }

        [[nodiscard]] std::vector<UI::GridColumn> MakePriceColumns()
        {
            std::vector<UI::GridColumn> columns{ UI::GridColumn{ std::wstring{ LEVEL_HEADER }, LEVEL_COLUMN_WEIGHT, Core::TextAlign::CENTER } };
            for (const Models::GemType& type : Models::GemCatalog::GEM_TYPES)
            {
                columns.push_back(UI::GridColumn{ Core::TextEncoding::UTF8ToWide(type.m_shortName) + std::wstring{ PRICE_HEADER_SUFFIX },
                    PRICE_COLUMN_WEIGHT, Core::TextAlign::TRAILING });
            }
            return columns;
        }

        [[nodiscard]] std::vector<UI::GridColumn> MakeAlarmColumns()
        {
            std::vector<UI::GridColumn> columns;
            for (std::size_t column{}; column < ALARM_HEADERS.size(); ++column)
            {
                columns.push_back(UI::GridColumn{ std::wstring{ ALARM_HEADERS[column] }, ALARM_COLUMN_WEIGHT,
                    column == ALARM_COL_GEM ? Core::TextAlign::LEADING : Core::TextAlign::TRAILING });
            }
            return columns;
        }

        [[nodiscard]] std::vector<std::wstring> MakeGemItems(const std::vector<Models::GemSelection>& gems)
        {
            std::vector<std::wstring> items;
            items.reserve(gems.size());
            for (const Models::GemSelection& gem : gems) items.push_back(Core::TextEncoding::UTF8ToWide(Models::GemCatalog::MakeDisplayName(gem)));
            return items;
        }
    }

    GemFinderView::GemFinderView(Core::Platform& platform)
        : m_gems{ Models::GemCatalog::GetAllGems() }
        , m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::wstring{ AUTO_REFRESH_TEXT },
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_watchLabel{ m_host.Emplace<UI::Label>(std::wstring{ WATCH_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_watchIntervalBox{ m_host.Emplace<UI::ComboBox>() }
        , m_fullLabel{ m_host.Emplace<UI::Label>(std::wstring{ FULL_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_fullIntervalBox{ m_host.Emplace<UI::ComboBox>() }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_rateLimitLabel{ m_host.Emplace<UI::Label>(std::wstring{}, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_priceGrid{ m_host.Emplace<UI::Grid>(platform, MakePriceColumns(), Models::GemCatalog::LEVEL_COUNT) }
        , m_alarmGemBox{ m_host.Emplace<UI::ComboBox>() }
        , m_targetLabel{ m_host.Emplace<UI::Label>(std::wstring{ TARGET_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_targetPriceBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ TARGET_PLACEHOLDER }) }
        , m_addAlarmButton{ m_host.Emplace<UI::Button>(std::wstring{ ADD_ALARM_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_removeAlarmButton{ m_host.Emplace<UI::Button>(std::wstring{ REMOVE_ALARM_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_alarmGrid{ m_host.Emplace<UI::Grid>(platform, MakeAlarmColumns(), 0) }
        , m_chartGemBox{ m_host.Emplace<UI::ComboBox>() }
        , m_chart{ m_host.Emplace<UI::LineChart>() }
        , m_pricePanel{ Core::RectF{}, std::wstring{ PRICE_PANEL_TITLE } }
        , m_alarmPanel{ Core::RectF{}, std::wstring{ ALARM_PANEL_TITLE } }
        , m_chartPanel{ Core::RectF{}, std::wstring{ CHART_PANEL_TITLE } }
    {
        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 주기 콤보 상자 (선택은 SetIntervals로 저장된 설정을 채움)
        m_watchIntervalBox.SetItems(MakeIntervalItems(Core::APPConstants::WATCH_INTERVAL_SECONDS_OPTIONS, SECONDS_SUFFIX));
        m_fullIntervalBox.SetItems(MakeIntervalItems(Core::APPConstants::FULL_INTERVAL_MINUTES_OPTIONS, MINUTES_SUFFIX));
        SetIntervals(Core::APPConstants::DEFAULT_WATCH_INTERVAL_SECONDS, Core::APPConstants::DEFAULT_FULL_INTERVAL_MINUTES);
        const auto notifyIntervalChanged{ [this]
        {
            if (const auto onIntervalChanged{ m_onIntervalChanged }) onIntervalChanged();
        } };
        m_watchIntervalBox.SetOnSelectionChanged(notifyIntervalChanged);
        m_fullIntervalBox.SetOnSelectionChanged(notifyIntervalChanged);

        InitializePriceGrid();

        // 알림 입력: 보석 선택 + 목표가 (숫자 칸이라 IME 끔, Enter = 추가/변경)
        const std::vector<std::wstring> gemItems{ MakeGemItems(m_gems) };
        m_alarmGemBox.SetItems(gemItems);
        m_alarmGemBox.SetSelectedIndex(0);
        m_targetPriceBox.SetIMEAllowed(false);
        m_targetPriceBox.SetInputFilter(IsPriceCharacter);
        m_targetPriceBox.SetMaxLength(TARGET_PRICE_MAX_LENGTH);
        m_targetPriceBox.SetOnSubmit([this]
        {
            if (const auto onAddAlarm{ m_onAddAlarm }) onAddAlarm();
        });
        m_addAlarmButton.SetOnClick([this]
        {
            if (const auto onAddAlarm{ m_onAddAlarm }) onAddAlarm();
        });
        m_removeAlarmButton.SetOnClick([this]
        {
            if (const auto onRemoveAlarm{ m_onRemoveAlarm }) onRemoveAlarm();
        });

        // 알림 목록: 행 단위 선택, 삭제 버튼을 눌러도 선택이 보이게 (마우스 · 키보드 선택 모두 반영)
        m_alarmGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        m_alarmGrid.SetSelectionAlwaysVisible(true);
        m_alarmGrid.SetOnSelectionChanged([this]
        {
            const std::optional<std::size_t> row{ m_alarmGrid.GetSelectedRow() };
            if (const auto onAlarmSelected{ m_onAlarmSelected }; row.has_value() && onAlarmSelected) onAlarmSelected(*row);
        });

        // 그래프: 보석을 바꾸면 그래프 갱신, 보석 = 보라 곡선
        m_chartGemBox.SetItems(gemItems);
        m_chartGemBox.SetSelectedIndex(0);
        m_chartGemBox.SetOnSelectionChanged([this]
        {
            if (const auto onChartGemChanged{ m_onChartGemChanged }) onChartGemChanged();
        });
        m_chart.SetSeriesColor(Theme::GEM_CHART_LINE_COLOR);
        m_chart.SetValueSuffix(std::wstring{ CHART_VALUE_SUFFIX });
    }

    GemFinderView::~GemFinderView() = default;

    void GemFinderView::SetOnRefresh(std::function<void()> onRefresh)
    {
        m_onRefresh = std::move(onRefresh);
    }

    void GemFinderView::SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged)
    {
        m_onAutoRefreshChanged = std::move(onAutoRefreshChanged);
    }

    void GemFinderView::SetOnIntervalChanged(std::function<void()> onIntervalChanged)
    {
        m_onIntervalChanged = std::move(onIntervalChanged);
    }

    void GemFinderView::SetOnPriceCellClicked(std::function<void(const Models::GemSelection& gem)> onPriceCellClicked)
    {
        m_onPriceCellClicked = std::move(onPriceCellClicked);
    }

    void GemFinderView::SetOnAlarmSelected(std::function<void(std::size_t alarmIndex)> onAlarmSelected)
    {
        m_onAlarmSelected = std::move(onAlarmSelected);
    }

    void GemFinderView::SetOnAddAlarm(std::function<void()> onAddAlarm)
    {
        m_onAddAlarm = std::move(onAddAlarm);
    }

    void GemFinderView::SetOnRemoveAlarm(std::function<void()> onRemoveAlarm)
    {
        m_onRemoveAlarm = std::move(onRemoveAlarm);
    }

    void GemFinderView::SetOnChartGemChanged(std::function<void()> onChartGemChanged)
    {
        m_onChartGemChanged = std::move(onChartGemChanged);
    }

    void GemFinderView::SetIntervals(int watchSeconds, int fullMinutes)
    {
        m_watchIntervalBox.SetSelectedIndex(FindOptionIndex(Core::APPConstants::WATCH_INTERVAL_SECONDS_OPTIONS, watchSeconds));
        m_fullIntervalBox.SetSelectedIndex(FindOptionIndex(Core::APPConstants::FULL_INTERVAL_MINUTES_OPTIONS, fullMinutes));
    }

    int GemFinderView::GetWatchIntervalSeconds() const noexcept
    {
        return GetOptionValue(Core::APPConstants::WATCH_INTERVAL_SECONDS_OPTIONS, m_watchIntervalBox.GetSelectedIndex(),
            Core::APPConstants::DEFAULT_WATCH_INTERVAL_SECONDS);
    }

    int GemFinderView::GetFullIntervalMinutes() const noexcept
    {
        return GetOptionValue(Core::APPConstants::FULL_INTERVAL_MINUTES_OPTIONS, m_fullIntervalBox.GetSelectedIndex(),
            Core::APPConstants::DEFAULT_FULL_INTERVAL_MINUTES);
    }

    void GemFinderView::SetAutoRefreshChecked(bool isChecked) noexcept
    {
        m_autoRefreshBox.SetChecked(isChecked);
    }

    bool GemFinderView::IsAutoRefreshChecked() const noexcept
    {
        return m_autoRefreshBox.IsChecked();
    }

    void GemFinderView::SetRefreshEnabled(bool isEnabled) noexcept
    {
        m_refreshButton.SetEnabled(isEnabled);
    }

    void GemFinderView::SetStatusText(std::wstring text)
    {
        m_statusLabel.SetText(std::move(text));
    }

    void GemFinderView::SetRateLimitText(std::wstring text, RateLimitLevel level)
    {
        m_rateLimitLabel.SetText(std::move(text));
        m_rateLimitLabel.SetColor(GetRateLimitColor(level));
    }

    // 가격 칸 글자 · 색 (▲ 빨강 / ▼ 파랑 / 매물 없음 회색, 원본 FormatPriceCell)
    void GemFinderView::ShowQuote(const Models::GemQuote& quote)
    {
        const Models::GemSelection& gem{ quote.GetGem() };
        const auto type{ std::find_if(Models::GemCatalog::GEM_TYPES.begin(), Models::GemCatalog::GEM_TYPES.end(),
            [&gem](const Models::GemType& gemType) { return gemType.m_shortName == gem.m_typeName; }) };
        if (type == Models::GemCatalog::GEM_TYPES.end() || !Models::GemCatalog::IsLevelInRange(gem.m_level)) return;

        const std::size_t row{ GetPriceRow(gem.m_level) };
        const std::size_t column{ PRICE_COL_FIRST_TYPE + static_cast<std::size_t>(std::distance(Models::GemCatalog::GEM_TYPES.begin(), type)) };

        std::wstring text;
        Core::Color textColor{ Theme::TEXT_COLOR };
        if (!quote.GetPrice().has_value())
        {
            text = std::wstring{ quote.IsQueried() ? NO_LISTING_TEXT : NOT_QUERIED_TEXT };   // 조회 전이면 "-"
            textColor = Theme::MUTED_TEXT_COLOR;
        }
        else
        {
            const std::int64_t price{ *quote.GetPrice() };
            const std::int64_t diff{ quote.GetDiff().value_or(0) };
            text = FormatAmount(price);
            if (diff > 0)
            {
                text += std::format(L" ({}{})", UP_MARK, FormatAmount(diff));
                textColor = Theme::UP_TEXT_COLOR;
            }
            else if (diff < 0)
            {
                text += std::format(L" ({}{})", DOWN_MARK, FormatAmount(-diff));
                textColor = Theme::DOWN_TEXT_COLOR;
            }
        }

        m_priceGrid.SetCellText(row, column, std::move(text));
        m_priceGrid.SetCellStyle(row, column, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, textColor });
    }

    // 알림 목록 다시 채우기 (행 수가 바뀌면 표가 맞춰 줄고 늘어남, 선택은 관리자가 SelectAlarm으로 되돌림)
    void GemFinderView::ShowAlarms(const std::vector<AlarmRow>& rows)
    {
        m_alarmGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const AlarmRow& alarm{ rows[row] };
            m_alarmGrid.SetCellText(row, ALARM_COL_GEM, alarm.m_name);
            m_alarmGrid.SetCellText(row, ALARM_COL_TARGET, FormatAmount(alarm.m_targetPrice));
            m_alarmGrid.SetCellText(row, ALARM_COL_CURRENT, alarm.m_currentPrice.has_value() ? FormatAmount(*alarm.m_currentPrice) : std::wstring{ NOT_QUERIED_TEXT });
            m_alarmGrid.SetCellText(row, ALARM_COL_STATE, std::wstring{ GetStateText(alarm.m_state) });

            const UI::GridCellStyle style{ GetAlarmRowStyle(alarm.m_state) };
            for (std::size_t column{}; column < ALARM_HEADERS.size(); ++column) m_alarmGrid.SetCellStyle(row, column, style);
        }
    }

    std::optional<std::size_t> GemFinderView::GetSelectedAlarmIndex() const noexcept
    {
        return m_alarmGrid.GetSelectedRow();
    }

    void GemFinderView::SelectAlarm(std::optional<std::size_t> alarmIndex)
    {
        m_alarmGrid.SelectRow(alarmIndex);
    }

    std::optional<Models::GemSelection> GemFinderView::GetAlarmGem() const
    {
        const std::optional<std::size_t> index{ m_alarmGemBox.GetSelectedIndex() };
        if (!index.has_value() || *index >= m_gems.size()) return std::nullopt;
        return m_gems[*index];
    }

    void GemFinderView::SelectAlarmGem(const Models::GemSelection& gem)
    {
        if (const std::optional<std::size_t> index{ FindGemIndex(gem) }; index.has_value()) m_alarmGemBox.SetSelectedIndex(index);
    }

    std::int64_t GemFinderView::GetTargetPrice() const noexcept
    {
        const double value{ Core::NumberFormat::ParseNumber(m_targetPriceBox.GetText()) };
        if (!(value > 0.0)) return 0;
        if (value >= static_cast<double>(Core::APPConstants::MAX_TARGET_PRICE)) return Core::APPConstants::MAX_TARGET_PRICE;
        return static_cast<std::int64_t>(std::llround(value));
    }

    void GemFinderView::SetTargetPrice(std::int64_t targetPrice)
    {
        m_targetPriceBox.SetText(FormatAmount(std::clamp<std::int64_t>(targetPrice, 0, Core::APPConstants::MAX_TARGET_PRICE)));
    }

    std::optional<Models::GemSelection> GemFinderView::GetChartGem() const
    {
        const std::optional<std::size_t> index{ m_chartGemBox.GetSelectedIndex() };
        if (!index.has_value() || *index >= m_gems.size()) return std::nullopt;
        return m_gems[*index];
    }

    void GemFinderView::SelectChartGem(const Models::GemSelection& gem)
    {
        if (const std::optional<std::size_t> index{ FindGemIndex(gem) }; index.has_value()) m_chartGemBox.SetSelectedIndex(index);
    }

    void GemFinderView::SetChart(std::wstring title, std::vector<UI::ChartPoint> points)
    {
        m_chart.SetTitle(std::move(title));
        m_chart.SetPoints(std::move(points));
    }

    void GemFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void GemFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderPanels(renderer);
        m_host.Render(renderer);
    }

    bool GemFinderView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape GemFinderView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void GemFinderView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void GemFinderView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 표 첫 모양: 레벨 열 (머리글 톤 + 가운데) · 가격 칸 "-" (원본 BuildPriceGroup)
    void GemFinderView::InitializePriceGrid()
    {
        for (std::size_t row{}; row < Models::GemCatalog::LEVEL_COUNT; ++row)
        {
            m_priceGrid.SetCellText(row, PRICE_COL_LEVEL, std::format(L"{}{}", GetRowLevel(row), LEVEL_SUFFIX));
            m_priceGrid.SetCellStyle(row, PRICE_COL_LEVEL, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR });

            for (std::size_t column{ PRICE_COL_FIRST_TYPE }; column < m_priceGrid.GetColumnCount(); ++column)
            {
                m_priceGrid.SetCellText(row, column, std::wstring{ NOT_QUERIED_TEXT });
                m_priceGrid.SetCellStyle(row, column, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR });
            }
        }

        // 가격 칸을 누르면 그 보석을 그래프 · 알림 입력칸에 바로 선택 (레벨 열은 무시)
        m_priceGrid.SetOnCellClicked([this](std::size_t row, std::size_t column)
        {
            if (column < PRICE_COL_FIRST_TYPE) return;

            const std::size_t typeIndex{ column - PRICE_COL_FIRST_TYPE };
            if (typeIndex >= Models::GemCatalog::GEM_TYPES.size()) return;

            const Models::GemSelection gem{ std::string{ Models::GemCatalog::GEM_TYPES[typeIndex].m_shortName }, GetRowLevel(row) };
            if (const auto onPriceCellClicked{ m_onPriceCellClicked }) onPriceCellClicked(gem);
        });
    }

    // 상단 바 + (왼쪽 최저가 표 | 오른쪽 위 알림 · 아래 그래프) — 원본 45% / 55%, 알림 40% / 그래프 60%
    void GemFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ contentArea.m_right - PAGE_PADDING };
        const float barTop{ contentArea.m_top + PAGE_PADDING };
        const float barBottom{ barTop + Theme::CONTROL_HEIGHT };

        // 상단 바: [시세 갱신] [자동 갱신] 감시 [10초] 전체 [1분] 상태 · 호출 한도
        float x{ left };
        const auto place{ [&x, barTop, barBottom](UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, barTop, x + width, barBottom });
            x += width + gapAfter;
        } };
        place(m_refreshButton, REFRESH_BUTTON_WIDTH, GROUP_GAP);
        place(m_autoRefreshBox, AUTO_REFRESH_WIDTH, GROUP_GAP);
        place(m_watchLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_watchIntervalBox, INTERVAL_BOX_WIDTH, GROUP_GAP);
        place(m_fullLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_fullIntervalBox, INTERVAL_BOX_WIDTH, SECTION_GAP);
        place(m_statusLabel, STATUS_WIDTH, GROUP_GAP);
        m_rateLimitLabel.SetBounds(Core::RectF{ x, barTop, std::max(x, right), barBottom });

        // 본문 영역
        const float bodyTop{ barBottom + SECTION_GAP };
        const float bodyBottom{ std::max(bodyTop, contentArea.m_bottom - PAGE_PADDING) };
        const float priceRight{ left + (right - left - SECTION_GAP) * PRICE_AREA_RATIO };
        const float rightLeft{ priceRight + SECTION_GAP };
        const float alarmBottom{ bodyTop + (bodyBottom - bodyTop - SECTION_GAP) * ALARM_AREA_RATIO };

        // 왼쪽: 최저가 표 (묶음 상자 안을 채움)
        const Core::RectF priceInner{ LayoutPanel(m_pricePanel, Core::RectF{ left, bodyTop, priceRight, bodyBottom }) };
        m_priceGrid.SetBounds(priceInner);

        // 오른쪽 위: [보석] 목표가 [입력] [추가/변경] [삭제] + 알림 목록
        const Core::RectF alarmInner{ LayoutPanel(m_alarmPanel, Core::RectF{ rightLeft, bodyTop, right, alarmBottom }) };
        const float inputBottom{ alarmInner.m_top + Theme::CONTROL_HEIGHT };
        float inputX{ alarmInner.m_left };
        const auto placeInput{ [&inputX, &alarmInner, inputBottom](UI::Control& control, float width)
        {
            control.SetBounds(Core::RectF{ inputX, alarmInner.m_top, inputX + width, inputBottom });
            inputX += width + CONTROL_GAP;
        } };
        placeInput(m_alarmGemBox, GEM_BOX_WIDTH);
        placeInput(m_targetLabel, TARGET_LABEL_WIDTH);
        placeInput(m_targetPriceBox, TARGET_BOX_WIDTH);
        placeInput(m_addAlarmButton, ADD_BUTTON_WIDTH);
        placeInput(m_removeAlarmButton, REMOVE_BUTTON_WIDTH);
        m_alarmGrid.SetBounds(Core::RectF{ alarmInner.m_left, inputBottom + CONTROL_GAP, alarmInner.m_right, std::max(inputBottom + CONTROL_GAP, alarmInner.m_bottom) });

        // 오른쪽 아래: [보석] + 일별 최저가 그래프
        const Core::RectF chartInner{ LayoutPanel(m_chartPanel, Core::RectF{ rightLeft, alarmBottom + SECTION_GAP, right, bodyBottom }) };
        const float chartBoxBottom{ chartInner.m_top + Theme::CONTROL_HEIGHT };
        m_chartGemBox.SetBounds(Core::RectF{ chartInner.m_left, chartInner.m_top, chartInner.m_left + GEM_BOX_WIDTH, chartBoxBottom });
        m_chart.SetBounds(Core::RectF{ chartInner.m_left, chartBoxBottom + CONTROL_GAP, chartInner.m_right, std::max(chartBoxBottom + CONTROL_GAP, chartInner.m_bottom) });
    }

    Core::RectF GemFinderView::LayoutPanel(Panel& panel, const Core::RectF& bounds) const
    {
        panel.m_bounds = bounds;
        return Core::RectF{ bounds.m_left + PANEL_PADDING, bounds.m_top + Theme::PANEL_TITLE_HEIGHT,
            bounds.m_right - PANEL_PADDING, std::max(bounds.m_top + Theme::PANEL_TITLE_HEIGHT, bounds.m_bottom - PANEL_PADDING) };
    }

    // 묶음 상자: 둥근 테두리 + 왼쪽 위 제목
    void GemFinderView::RenderPanels(Core::Renderer& renderer) const
    {
        for (const Panel* panel : { &m_pricePanel, &m_alarmPanel, &m_chartPanel })
        {
            renderer.DrawRoundedRect(panel->m_bounds, Theme::PANEL_CORNER_RADIUS, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);

            const Core::RectF titleRect{ panel->m_bounds.m_left + PANEL_TITLE_LEFT, panel->m_bounds.m_top,
                panel->m_bounds.m_right - PANEL_TITLE_LEFT, panel->m_bounds.m_top + Theme::PANEL_TITLE_HEIGHT };
            renderer.DrawString(panel->m_title, titleRect, Theme::TEXT_COLOR, PANEL_TITLE_STYLE);
        }
    }

    std::optional<std::size_t> GemFinderView::FindGemIndex(const Models::GemSelection& gem) const
    {
        const auto found{ std::find(m_gems.begin(), m_gems.end(), gem) };
        if (found == m_gems.end()) return std::nullopt;
        return static_cast<std::size_t>(std::distance(m_gems.begin(), found));
    }
}