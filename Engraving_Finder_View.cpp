module LostArkMultiTool.Views.EngravingFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.PageLayout;
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
        // ===== 시세 표 열 =====
        constexpr std::size_t COL_NAME{ 0 };
        constexpr std::size_t COL_CURRENT{ 1 };
        constexpr std::size_t COL_YESTERDAY{ 2 };
        constexpr std::size_t COL_RECENT{ 3 };
        constexpr std::size_t COL_CHANGE{ 4 };
        constexpr std::size_t COL_TREND{ 5 };
        constexpr std::size_t COLUMN_COUNT{ 6 };
        constexpr float NAME_COLUMN_WEIGHT{ 1.5f };      // 각인서 이름은 넓게 (배지 + 이름)
        constexpr float CURRENT_COLUMN_WEIGHT{ 1.4f };   // "12,345 (▲1,234)" 이 들어가도록
        constexpr float PRICE_COLUMN_WEIGHT{ 1.0f };
        constexpr float CHANGE_COLUMN_WEIGHT{ 0.8f };
        constexpr float TREND_COLUMN_WEIGHT{ 1.0f };     // 추이 선 칸

        constexpr int AMOUNT_DECIMALS{ 0 };   // 골드는 정수로
        constexpr int RATE_DECIMALS{ 1 };     // 변동률은 소수 첫째 자리까지
        constexpr std::size_t SEARCH_MAX_LENGTH{ 20 };
        constexpr std::size_t BADGE_LETTER_LENGTH{ 1 };   // 이름 앞 배지에 넣는 글자 수 (첫 글자)

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view REFRESHING_TEXT{ L"조회 중…" };   // 조회하는 동안 버튼 글자
        constexpr std::wstring_view AUTO_REFRESH_FORMAT{ L"{}분마다 자동 갱신" };
        constexpr std::wstring_view SEARCH_PLACEHOLDER{ L"각인서 이름 검색" };
        constexpr std::wstring_view CLEAR_TEXT{ L"✕" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view TABLE_EYEBROW{ L"LIVE MARKET" };
        constexpr std::wstring_view TABLE_TITLE{ L"유물 각인서 최저가" };
        constexpr std::wstring_view TABLE_RIGHT_TEXT{ L"등락은 직전 갱신 대비" };
        constexpr std::wstring_view CHART_EYEBROW{ L"PRICE HISTORY" };
        constexpr std::wstring_view CHART_PANEL_TITLE{ L"시세 기록 (일별 최저가)" };   // 그래프 카드 제목
        constexpr std::wstring_view FOOTER_LEFT_FORMAT{ L"총 {}개의 검색 결과" };
        constexpr std::wstring_view FOOTER_RIGHT_TEXT{ L"변동률은 전일 평균가 대비" };
        constexpr std::wstring_view EMPTY_SEARCH_TITLE{ L"조건에 맞는 각인서가 없어요" };
        constexpr std::wstring_view EMPTY_SEARCH_HINT{ L"검색어를 줄이거나 ✕ 버튼으로 지운 뒤 다시 찾아 보세요." };
        constexpr std::wstring_view EMPTY_DATA_TITLE{ L"아직 불러온 시세가 없어요" };
        constexpr std::wstring_view EMPTY_DATA_HINT{ L"[시세 갱신]을 누르면 거래소 시세를 불러와요." };
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };
        constexpr std::wstring_view PLUS_SIGN{ L"+" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };
        constexpr std::wstring_view CHART_VALUE_SUFFIX{ L" 골드" };   // 말풍선 값 뒤 단위

        // ===== 통계 카드 =====
        constexpr std::size_t STAT_CARD_COUNT{ 4 };   // 위쪽 통계 카드 수
        constexpr std::wstring_view STAT_ICON_COUNT{ L"▤" };
        constexpr std::wstring_view STAT_ICON_RATE{ L"↗" };
        constexpr std::wstring_view STAT_ICON_TOP{ L"◎" };
        constexpr std::wstring_view STAT_ICON_TIME{ L"◷" };
        constexpr std::wstring_view STAT_LABEL_COUNT{ L"조회한 각인서" };
        constexpr std::wstring_view STAT_LABEL_RATE{ L"평균 등락" };
        constexpr std::wstring_view STAT_LABEL_TOP{ L"최고가 각인서" };
        constexpr std::wstring_view STAT_LABEL_TIME{ L"마지막 갱신" };
        constexpr std::wstring_view STAT_COUNT_FORMAT{ L"{}종" };
        constexpr std::wstring_view STAT_LISTED_FORMAT{ L"매물 {}종" };
        constexpr std::wstring_view STAT_RATE_SUB{ L"전일 평균가 대비" };
        constexpr std::wstring_view STAT_PRICE_FORMAT{ L"{} 골드" };
        constexpr std::wstring_view STAT_AUTO_OFF_TEXT{ L"자동 갱신 꺼짐" };

        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"각인서", L"현재 최저가", L"전일 평균가", L"최근 거래가", L"변동", L"추이" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ NAME_COLUMN_WEIGHT, CURRENT_COLUMN_WEIGHT, PRICE_COLUMN_WEIGHT, PRICE_COLUMN_WEIGHT, CHANGE_COLUMN_WEIGHT, TREND_COLUMN_WEIGHT };

        // 정렬 콤보 상자 항목 (EngravingSortOrder 순서와 같음)
        constexpr std::size_t SORT_OPTION_COUNT{ 5 };
        constexpr std::array<std::wstring_view, SORT_OPTION_COUNT> SORT_ITEMS{ L"가격 높은 순", L"가격 낮은 순", L"상승률 높은 순", L"하락률 높은 순", L"이름 순" };
        constexpr std::size_t DEFAULT_SORT_INDEX{ static_cast<std::size_t>(EngravingSortOrder::SORT_PRICE_DESC) };

        // ===== 배치 (DIP) =====
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float CONTROL_GAP{ 8.0f };               // 검색칸 · ✕ 사이
        constexpr float REFRESH_BUTTON_WIDTH{ 104.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 150.0f };
        constexpr float SEARCH_BOX_WIDTH{ 220.0f };
        constexpr float CLEAR_BUTTON_WIDTH{ 32.0f };
        constexpr float SORT_BOX_WIDTH{ 150.0f };
        constexpr float HALF{ 0.5f };                      // 세로 가운데 계산
        constexpr float TABLE_WIDTH_WEIGHT{ 1.65f };       // 표 카드 : 그래프 카드 너비 비율
        constexpr float CHART_WIDTH_WEIGHT{ 1.0f };
        constexpr std::size_t BODY_COLUMN_COUNT{ 2 };      // 아래쪽 카드 수 (표 · 그래프)

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };

        [[nodiscard]] std::wstring FormatAmount(double value)
        {
            return Core::NumberFormat::FormatNumber(value, AMOUNT_DECIMALS);
        }

        // 0 이하(거래 · 매물 없음)는 "-"
        [[nodiscard]] std::wstring FormatPrice(double value)
        {
            return value > 0.0 ? FormatAmount(value) : std::wstring{ EMPTY_TEXT };
        }

        // 등락이 0으로 반올림되면 표시하지 않음 (부동소수 오차로 "▲0"이 뜨지 않게)
        [[nodiscard]] std::int64_t RoundDiff(const std::optional<double>& diff) noexcept
        {
            return diff.has_value() ? std::llround(*diff) : 0;
        }

        [[nodiscard]] std::vector<UI::GridColumn> MakeColumns()
        {
            std::vector<UI::GridColumn> columns;
            columns.reserve(COLUMN_COUNT);
            for (std::size_t column{}; column < COLUMN_COUNT; ++column)
            {
                const Core::TextAlign align{ column == COL_NAME ? Core::TextAlign::LEADING : (column == COL_TREND ? Core::TextAlign::CENTER : Core::TextAlign::TRAILING) };
                columns.push_back(UI::GridColumn{ std::wstring{ COLUMN_HEADERS[column] }, COLUMN_WEIGHTS[column], align });
            }
            return columns;
        }

        [[nodiscard]] std::vector<std::wstring> MakeSortItems()
        {
            std::vector<std::wstring> items;
            items.reserve(SORT_ITEMS.size());
            for (const std::wstring_view item : SORT_ITEMS) items.emplace_back(item);
            return items;
        }

        // 오름 · 내림에 맞는 글자색 (▲ 빨강 / ▼ 파랑 / 변화 없음 기본색)
        [[nodiscard]] const Core::Color& GetTrendColor(double value) noexcept
        {
            if (value > 0.0) return Theme::UP_TEXT_COLOR;
            if (value < 0.0) return Theme::DOWN_TEXT_COLOR;
            return Theme::TEXT_COLOR;
        }

        // 통계 카드 작은 글자색 (오르면 청록 · 내리면 분홍 · 그 외 회색, 추이 선과 같은 색)
        [[nodiscard]] const Core::Color& GetStatTrendColor(double value) noexcept
        {
            if (value > 0.0) return Theme::SPARKLINE_UP_COLOR;
            if (value < 0.0) return Theme::SPARKLINE_DOWN_COLOR;
            return Theme::MUTED_TEXT_COLOR;
        }
    }

    EngravingFinderView::EngravingFinderView(Core::Platform& platform)
        : m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::format(AUTO_REFRESH_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES),
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_searchBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ SEARCH_PLACEHOLDER }) }
        , m_clearButton{ m_host.Emplace<UI::Button>(std::wstring{ CLEAR_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_sortBox{ m_host.Emplace<UI::ComboBox>() }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }
        , m_priceGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_chart{ m_host.Emplace<UI::LineChart>() }
    {
        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 검색: 한글 입력, 글자가 바뀔 때마다 바로 거름
        m_searchBox.SetMaxLength(SEARCH_MAX_LENGTH);
        m_searchBox.SetOnTextChanged([this] { NotifyFilterChanged(); });

        // ✕ 버튼: 검색어를 한 번에 지우고 입력칸에 다시 포커스
        m_clearButton.SetOnClick([this]
        {
            m_searchBox.SetText(std::wstring{});   // 검색어 비움
            NotifyFilterChanged();                  // 표를 전체로 되돌림
            m_host.SetFocus(&m_searchBox);          // 바로 다시 입력할 수 있게
        });

        // 정렬: 기본은 가격 높은 순
        m_sortBox.SetItems(MakeSortItems());
        m_sortBox.SetSelectedIndex(DEFAULT_SORT_INDEX);
        m_sortBox.SetOnSelectionChanged([this] { NotifyFilterChanged(); });

        // 시세 표: 읽기 전용, 행 단위 선택, 새 디자인 행 높이
        m_priceGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        m_priceGrid.SetRowHeight(PageLayout::GRID_ROW_HEIGHT);
        m_priceGrid.SetOnSelectionChanged([this]
        {
            if (const auto onRowSelected{ m_onRowSelected }) onRowSelected();   // 고른 각인서의 그래프로 바꿈
        });

        // 그래프: 유물 등급색 곡선 + 말풍선 "N 골드" (보석 검색기 그래프와 같은 형식)
        m_chart.SetSeriesColor(Theme::GRADE_RELIC_COLOR);
        m_chart.SetValueSuffix(std::wstring{ CHART_VALUE_SUFFIX });
    }

    EngravingFinderView::~EngravingFinderView() = default;

    void EngravingFinderView::SetOnRefresh(std::function<void()> onRefresh)
    {
        m_onRefresh = std::move(onRefresh);
    }

    void EngravingFinderView::SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged)
    {
        m_onAutoRefreshChanged = std::move(onAutoRefreshChanged);
    }

    void EngravingFinderView::SetOnFilterChanged(std::function<void()> onFilterChanged)
    {
        m_onFilterChanged = std::move(onFilterChanged);
    }

    void EngravingFinderView::SetOnRowSelected(std::function<void()> onRowSelected)
    {
        m_onRowSelected = std::move(onRowSelected);
    }

    std::optional<std::size_t> EngravingFinderView::GetSelectedRow() const noexcept
    {
        return m_priceGrid.GetSelectedRow();
    }

    void EngravingFinderView::SelectRow(std::optional<std::size_t> row)
    {
        m_priceGrid.SelectRow(row);
    }

    void EngravingFinderView::SetChart(std::wstring title, std::vector<UI::ChartPoint> points)
    {
        m_chart.SetTitle(std::move(title));
        m_chart.SetPoints(std::move(points));
    }

    void EngravingFinderView::SetAutoRefreshChecked(bool isChecked) noexcept
    {
        m_autoRefreshBox.SetChecked(isChecked);
    }

    bool EngravingFinderView::IsAutoRefreshChecked() const noexcept
    {
        return m_autoRefreshBox.IsChecked();
    }

    void EngravingFinderView::SetRefreshEnabled(bool isEnabled) noexcept
    {
        m_refreshButton.SetEnabled(isEnabled);
        m_refreshButton.SetText(std::wstring{ isEnabled ? REFRESH_TEXT : REFRESHING_TEXT });   // 조회 중에는 글자도 바꿔 알림
    }

    void EngravingFinderView::SetStatusText(std::wstring text)
    {
        m_statusLabel.SetText(std::move(text));
    }

    const std::wstring& EngravingFinderView::GetSearchText() const noexcept
    {
        return m_searchBox.GetText();
    }

    EngravingSortOrder EngravingFinderView::GetSortOrder() const noexcept
    {
        const std::size_t index{ m_sortBox.GetSelectedIndex().value_or(DEFAULT_SORT_INDEX) };
        return index < SORT_OPTION_COUNT ? static_cast<EngravingSortOrder>(index) : EngravingSortOrder::SORT_PRICE_DESC;
    }

    // 표 다시 채우기 (행 수가 바뀌면 표가 맞춰 줄고 늘어남)
    void EngravingFinderView::ShowRows(const std::vector<EngravingRow>& rows, std::size_t totalCount)
    {
        m_rowCount = rows.size();    // 표 아래 "총 N개" · 빈 안내 판단용
        m_totalCount = totalCount;
        m_priceGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const EngravingRow& engraving{ rows[row] };
            const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
            const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

            // 이름 (조회 대상이 유물 각인서뿐이라 유물 등급색 + 굵게, 앞에 이름 첫 글자 배지)
            m_priceGrid.SetCellText(row, COL_NAME, engraving.m_name);
            m_priceGrid.SetCellStyle(row, COL_NAME, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GRADE_RELIC_COLOR, Core::FontWeight::BOLD,
                UI::GridDecoration::DECORATION_BADGE, engraving.m_name.substr(0, BADGE_LETTER_LENGTH), Theme::GRADE_RELIC_COLOR });

            // 현재 최저가 + 직전 갱신 대비 등락 (▲ 빨강 / ▼ 파랑, 원본 보석 검색기와 같은 형식)
            std::wstring currentText{ FormatPrice(engraving.m_currentMinPrice) };
            const std::int64_t diff{ RoundDiff(engraving.m_diff) };
            if (diff > 0) currentText += std::format(L" ({}{})", UP_MARK, FormatAmount(static_cast<double>(diff)));
            else if (diff < 0) currentText += std::format(L" ({}{})", DOWN_MARK, FormatAmount(static_cast<double>(-diff)));
            m_priceGrid.SetCellText(row, COL_CURRENT, std::move(currentText));
            m_priceGrid.SetCellStyle(row, COL_CURRENT, engraving.m_currentMinPrice > 0.0
                ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(static_cast<double>(diff)), Core::FontWeight::BOLD } : mutedStyle);

            // 전일 평균가 · 최근 거래가
            m_priceGrid.SetCellText(row, COL_YESTERDAY, FormatPrice(engraving.m_yesterdayAvgPrice));
            m_priceGrid.SetCellStyle(row, COL_YESTERDAY, engraving.m_yesterdayAvgPrice > 0.0 ? normalStyle : mutedStyle);
            m_priceGrid.SetCellText(row, COL_RECENT, FormatPrice(engraving.m_recentPrice));
            m_priceGrid.SetCellStyle(row, COL_RECENT, engraving.m_recentPrice > 0.0 ? normalStyle : mutedStyle);

            // 전일 대비 변동률 ("+3.2%" 빨강 / "-1.5%" 파랑, 계산할 수 없으면 "-")
            if (engraving.m_dayChangeRate.has_value())
            {
                const double rate{ *engraving.m_dayChangeRate };
                const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
                m_priceGrid.SetCellText(row, COL_CHANGE, sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX });
                m_priceGrid.SetCellStyle(row, COL_CHANGE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(rate) });
            }
            else
            {
                m_priceGrid.SetCellText(row, COL_CHANGE, std::wstring{ EMPTY_TEXT });
                m_priceGrid.SetCellStyle(row, COL_CHANGE, mutedStyle);
            }

            // 추이: 일별 최저가 기록 (기록이 2개 미만이면 선 없음)
            m_priceGrid.SetCellText(row, COL_TREND, std::wstring{});
            m_priceGrid.SetCellStyle(row, COL_TREND, normalStyle);
            m_priceGrid.SetCellSparkline(row, COL_TREND, engraving.m_trend);
        }
    }

    void EngravingFinderView::SetStats(EngravingStats stats)
    {
        m_stats = std::move(stats);
    }

    void EngravingFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void EngravingFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);

        // 위쪽 통계 카드 4장
        const std::vector<PageLayout::StatCard> cards{ BuildStatCards() };
        for (std::size_t index{}; index < cards.size() && index < m_statBounds.size(); ++index)
        {
            PageLayout::DrawStatCard(renderer, m_statBounds[index], cards[index]);
        }

        PageLayout::DrawCard(renderer, m_filterBounds);                                                                // 검색 · 정렬 줄 카드
        PageLayout::DrawCardWithHeader(renderer, m_panelBounds, TABLE_EYEBROW, TABLE_TITLE, TABLE_RIGHT_TEXT, true);     // 시세 표 카드
        PageLayout::DrawCardWithHeader(renderer, m_chartPanelBounds, CHART_EYEBROW, CHART_PANEL_TITLE);                  // 그래프 카드
        m_host.Render(renderer);
        RenderTableExtras(renderer);                                                                                    // 표 위에 얹는 "총 N개" 줄 · 빈 안내
    }

    bool EngravingFinderView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape EngravingFinderView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void EngravingFinderView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void EngravingFinderView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 위에서부터: 통계 카드 4장 → 검색 · 정렬 줄 → (왼쪽 시세 표 카드 | 오른쪽 그래프 카드)
    void EngravingFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        Core::RectF remaining{ contentArea };
        m_statBounds = PageLayout::SplitColumns(PageLayout::TakeTop(remaining, PageLayout::STAT_CARD_HEIGHT), STAT_CARD_COUNT, PageLayout::CARD_GAP);   // 통계 카드 4장
        m_filterBounds = PageLayout::TakeTop(remaining, PageLayout::FILTER_BAR_HEIGHT);                                                                 // 검색 · 정렬 줄

        // 검색 줄: [시세 갱신] [5분마다 자동 갱신] [검색] [✕] [정렬] 상태 (카드 안 세로 가운데)
        const float controlTop{ m_filterBounds.m_top + (PageLayout::FILTER_BAR_HEIGHT - PageLayout::CONTROL_HEIGHT) * HALF };
        const float controlBottom{ controlTop + PageLayout::CONTROL_HEIGHT };
        const float right{ m_filterBounds.m_right - PageLayout::CARD_PADDING };
        float x{ m_filterBounds.m_left + PageLayout::CARD_PADDING };
        const auto place{ [&x, controlTop, controlBottom](UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, controlTop, x + width, controlBottom });
            x += width + gapAfter;
        } };
        place(m_refreshButton, REFRESH_BUTTON_WIDTH, GROUP_GAP);
        place(m_autoRefreshBox, AUTO_REFRESH_WIDTH, GROUP_GAP);
        place(m_searchBox, SEARCH_BOX_WIDTH, CONTROL_GAP);
        place(m_clearButton, CLEAR_BUTTON_WIDTH, GROUP_GAP);
        place(m_sortBox, SORT_BOX_WIDTH, GROUP_GAP);
        m_statusLabel.SetBounds(Core::RectF{ x, controlTop, std::max(x, right), controlBottom });

        // 본문: 시세 표 카드 | 그래프 카드
        const std::array<float, BODY_COLUMN_COUNT> weights{ TABLE_WIDTH_WEIGHT, CHART_WIDTH_WEIGHT };
        const std::vector<Core::RectF> columns{ PageLayout::SplitColumns(remaining, BODY_COLUMN_COUNT, PageLayout::CARD_GAP, weights) };
        m_panelBounds = columns[0];
        m_chartPanelBounds = columns[1];

        // 표 카드: 머리말 아래 표 + 맨 아래 "총 N개" 줄
        const float gridTop{ m_panelBounds.m_top + PageLayout::CARD_HEADER_HEIGHT };
        const float footerTop{ std::max(gridTop, m_panelBounds.m_bottom - PageLayout::TABLE_FOOTER_HEIGHT) };
        m_priceGrid.SetBounds(Core::RectF{ m_panelBounds.m_left, gridTop, m_panelBounds.m_right, footerTop });
        m_footerBounds = Core::RectF{ m_panelBounds.m_left, footerTop, m_panelBounds.m_right, m_panelBounds.m_bottom };
        m_emptyBounds = Core::RectF{ m_panelBounds.m_left, std::min(footerTop, gridTop + Theme::GRID_HEADER_HEIGHT), m_panelBounds.m_right, footerTop };

        // 그래프 카드: 머리말 아래 안쪽 여백을 두고 채움
        const float chartTop{ m_chartPanelBounds.m_top + PageLayout::CARD_HEADER_HEIGHT };
        m_chart.SetBounds(Core::RectF{ m_chartPanelBounds.m_left + PageLayout::CARD_PADDING, chartTop,
            m_chartPanelBounds.m_right - PageLayout::CARD_PADDING, std::max(chartTop, m_chartPanelBounds.m_bottom - PageLayout::CARD_PADDING) });
    }

    // 통계 값 → 카드 4장 (값이 없으면 "-", 실제 값만 보여 줌)
    std::vector<PageLayout::StatCard> EngravingFinderView::BuildStatCards() const
    {
        const bool hasQuotes{ m_stats.m_quoteCount > 0 };

        // 조회한 각인서 수 · 그중 매물이 있는 수
        PageLayout::StatCard countCard{ std::wstring{ STAT_ICON_COUNT }, Theme::ACCENT_COLOR, std::wstring{ STAT_LABEL_COUNT },
            hasQuotes ? std::format(STAT_COUNT_FORMAT, m_stats.m_quoteCount) : std::wstring{ EMPTY_TEXT },
            hasQuotes ? std::format(STAT_LISTED_FORMAT, m_stats.m_listedCount) : std::wstring{}, Theme::MUTED_TEXT_COLOR };

        // 전일 평균가 대비 평균 변동률
        PageLayout::StatCard rateCard{ std::wstring{ STAT_ICON_RATE }, Theme::SPARKLINE_UP_COLOR, std::wstring{ STAT_LABEL_RATE }, std::wstring{ EMPTY_TEXT },
            std::wstring{}, Theme::MUTED_TEXT_COLOR };
        if (m_stats.m_averageRate.has_value())
        {
            const double rate{ *m_stats.m_averageRate };
            const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
            rateCard.m_value = sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX };
            rateCard.m_sub = std::wstring{ STAT_RATE_SUB };
            rateCard.m_subColor = GetStatTrendColor(rate);
            if (rate != 0.0) rateCard.m_iconColor = GetStatTrendColor(rate);   // 아이콘도 오르내림 색
        }

        // 현재 최저가가 가장 비싼 각인서
        PageLayout::StatCard topCard{ std::wstring{ STAT_ICON_TOP }, Theme::GRADE_RELIC_COLOR, std::wstring{ STAT_LABEL_TOP }, std::wstring{ EMPTY_TEXT },
            std::wstring{}, Theme::MUTED_TEXT_COLOR };
        if (!m_stats.m_topName.empty())
        {
            topCard.m_value = m_stats.m_topName;
            topCard.m_sub = std::format(STAT_PRICE_FORMAT, FormatAmount(m_stats.m_topPrice));
        }

        // 마지막 갱신 시각 + 자동 갱신 상태
        const std::wstring autoText{ m_autoRefreshBox.IsChecked() ? std::format(AUTO_REFRESH_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES) : std::wstring{ STAT_AUTO_OFF_TEXT } };
        PageLayout::StatCard timeCard{ std::wstring{ STAT_ICON_TIME }, Theme::ACCENT_COLOR, std::wstring{ STAT_LABEL_TIME },
            m_stats.m_updatedText.empty() ? std::wstring{ EMPTY_TEXT } : m_stats.m_updatedText, autoText, Theme::MUTED_TEXT_COLOR };

        return std::vector<PageLayout::StatCard>{ std::move(countCard), std::move(rateCard), std::move(topCard), std::move(timeCard) };
    }

    // 표 아래 "총 N개의 검색 결과" 줄 + 표가 비었을 때 해결 방법 안내
    void EngravingFinderView::RenderTableExtras(Core::Renderer& renderer) const
    {
        PageLayout::DrawTableFooter(renderer, m_footerBounds, std::format(FOOTER_LEFT_FORMAT, m_rowCount), FOOTER_RIGHT_TEXT);
        if (m_rowCount > 0) return;

        // 거른 결과가 없는 경우와 아직 시세를 불러오지 못한 경우의 안내가 다름
        if (m_totalCount > 0) PageLayout::DrawEmptyState(renderer, m_emptyBounds, EMPTY_SEARCH_TITLE, EMPTY_SEARCH_HINT);
        else PageLayout::DrawEmptyState(renderer, m_emptyBounds, EMPTY_DATA_TITLE, EMPTY_DATA_HINT);
    }

    void EngravingFinderView::NotifyFilterChanged() const
    {
        if (const auto onFilterChanged{ m_onFilterChanged }) onFilterChanged();
    }
}
