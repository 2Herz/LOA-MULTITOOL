module LostArkMultiTool.Views.EngravingFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
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
        constexpr std::size_t COLUMN_COUNT{ 5 };
        constexpr float NAME_COLUMN_WEIGHT{ 1.4f };      // 각인서 이름은 넓게
        constexpr float CURRENT_COLUMN_WEIGHT{ 1.3f };   // "12,345 (▲1,234)" 이 들어가도록
        constexpr float PRICE_COLUMN_WEIGHT{ 1.0f };
        constexpr float CHANGE_COLUMN_WEIGHT{ 0.8f };

        constexpr int AMOUNT_DECIMALS{ 0 };   // 골드는 정수로
        constexpr int RATE_DECIMALS{ 1 };     // 변동률은 소수 첫째 자리까지
        constexpr std::size_t SEARCH_MAX_LENGTH{ 20 };

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view AUTO_REFRESH_FORMAT{ L"{}분마다 자동 갱신" };
        constexpr std::wstring_view SEARCH_LABEL_TEXT{ L"검색" };
        constexpr std::wstring_view SEARCH_PLACEHOLDER{ L"각인서 이름" };
        constexpr std::wstring_view SORT_LABEL_TEXT{ L"정렬" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view PANEL_TITLE{ L"유물 각인서 시세 (거래소, 등락은 직전 갱신 대비 · 변동률은 전일 평균가 대비)" };
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };
        constexpr std::wstring_view PLUS_SIGN{ L"+" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };

        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"각인서", L"현재 최저가", L"전일 평균가", L"최근 거래가", L"전일 대비" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ NAME_COLUMN_WEIGHT, CURRENT_COLUMN_WEIGHT, PRICE_COLUMN_WEIGHT, PRICE_COLUMN_WEIGHT, CHANGE_COLUMN_WEIGHT };

        // 정렬 콤보 상자 항목 (EngravingSortOrder 순서와 같음)
        constexpr std::size_t SORT_OPTION_COUNT{ 5 };
        constexpr std::array<std::wstring_view, SORT_OPTION_COUNT> SORT_ITEMS{ L"최저가 높은 순", L"최저가 낮은 순", L"상승률 높은 순", L"하락률 높은 순", L"이름 순" };
        constexpr std::size_t DEFAULT_SORT_INDEX{ static_cast<std::size_t>(EngravingSortOrder::SORT_PRICE_DESC) };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 16.0f };          // 상단 바 · 묶음 상자 사이
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float LABEL_GAP{ 4.0f };             // "검색" 글자와 입력칸 사이
        constexpr float REFRESH_BUTTON_WIDTH{ 110.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 150.0f };
        constexpr float SHORT_LABEL_WIDTH{ 30.0f };    // "검색" · "정렬"
        constexpr float SEARCH_BOX_WIDTH{ 180.0f };
        constexpr float SORT_BOX_WIDTH{ 140.0f };
        constexpr float PANEL_PADDING{ 10.0f };        // 묶음 상자 테두리 안쪽 여백
        constexpr float PANEL_TITLE_LEFT{ 12.0f };

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle PANEL_TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };

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
                columns.push_back(UI::GridColumn{ std::wstring{ COLUMN_HEADERS[column] }, COLUMN_WEIGHTS[column],
                    column == COL_NAME ? Core::TextAlign::LEADING : Core::TextAlign::TRAILING });
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
    }

    EngravingFinderView::EngravingFinderView(Core::Platform& platform)
        : m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::format(AUTO_REFRESH_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES),
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_searchLabel{ m_host.Emplace<UI::Label>(std::wstring{ SEARCH_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_searchBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ SEARCH_PLACEHOLDER }) }
        , m_sortLabel{ m_host.Emplace<UI::Label>(std::wstring{ SORT_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_sortBox{ m_host.Emplace<UI::ComboBox>() }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_priceGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
    {
        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 검색: 한글 입력, 글자가 바뀔 때마다 바로 거름
        m_searchBox.SetMaxLength(SEARCH_MAX_LENGTH);
        m_searchBox.SetOnTextChanged([this] { NotifyFilterChanged(); });

        // 정렬: 기본은 최저가 높은 순
        m_sortBox.SetItems(MakeSortItems());
        m_sortBox.SetSelectedIndex(DEFAULT_SORT_INDEX);
        m_sortBox.SetOnSelectionChanged([this] { NotifyFilterChanged(); });

        // 시세 표: 읽기 전용, 행 단위 선택 (긴 표에서 줄을 따라 읽기 쉽게)
        m_priceGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
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
    void EngravingFinderView::ShowRows(const std::vector<EngravingRow>& rows)
    {
        m_priceGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const EngravingRow& engraving{ rows[row] };
            const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
            const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

            // 이름
            m_priceGrid.SetCellText(row, COL_NAME, engraving.m_name);
            m_priceGrid.SetCellStyle(row, COL_NAME, normalStyle);

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

            // 전일 대비 변동률 ("+3.2%" 빨강 / "-1.5%" 파랑)
            if (!engraving.m_dayChangeRate.has_value())
            {
                m_priceGrid.SetCellText(row, COL_CHANGE, std::wstring{ EMPTY_TEXT });
                m_priceGrid.SetCellStyle(row, COL_CHANGE, mutedStyle);
                continue;
            }

            const double rate{ *engraving.m_dayChangeRate };
            const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
            m_priceGrid.SetCellText(row, COL_CHANGE, sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX });
            m_priceGrid.SetCellStyle(row, COL_CHANGE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(rate) });
        }
    }

    void EngravingFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void EngravingFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderPanel(renderer);
        m_host.Render(renderer);
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

    // 상단 바 + 묶음 상자 안을 채우는 시세 표
    void EngravingFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ contentArea.m_right - PAGE_PADDING };
        const float barTop{ contentArea.m_top + PAGE_PADDING };
        const float barBottom{ barTop + Theme::CONTROL_HEIGHT };

        // 상단 바: [시세 갱신] [5분마다 자동 갱신] 검색 [입력] 정렬 [콤보] 상태
        float x{ left };
        const auto place{ [&x, barTop, barBottom](UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, barTop, x + width, barBottom });
            x += width + gapAfter;
        } };
        place(m_refreshButton, REFRESH_BUTTON_WIDTH, GROUP_GAP);
        place(m_autoRefreshBox, AUTO_REFRESH_WIDTH, SECTION_GAP);
        place(m_searchLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_searchBox, SEARCH_BOX_WIDTH, GROUP_GAP);
        place(m_sortLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_sortBox, SORT_BOX_WIDTH, SECTION_GAP);
        m_statusLabel.SetBounds(Core::RectF{ x, barTop, std::max(x, right), barBottom });

        // 본문: 묶음 상자 (테두리 · 제목 자리를 뺀 안쪽에 표)
        const float bodyTop{ barBottom + SECTION_GAP };
        const float bodyBottom{ std::max(bodyTop, contentArea.m_bottom - PAGE_PADDING) };
        m_panelBounds = Core::RectF{ left, bodyTop, right, bodyBottom };
        m_priceGrid.SetBounds(Core::RectF{ left + PANEL_PADDING, bodyTop + Theme::PANEL_TITLE_HEIGHT,
            right - PANEL_PADDING, std::max(bodyTop + Theme::PANEL_TITLE_HEIGHT, bodyBottom - PANEL_PADDING) });
    }

    // 묶음 상자: 둥근 테두리 + 왼쪽 위 제목
    void EngravingFinderView::RenderPanel(Core::Renderer& renderer) const
    {
        renderer.DrawRoundedRect(m_panelBounds, Theme::PANEL_CORNER_RADIUS, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);

        const Core::RectF titleRect{ m_panelBounds.m_left + PANEL_TITLE_LEFT, m_panelBounds.m_top,
            m_panelBounds.m_right - PANEL_TITLE_LEFT, m_panelBounds.m_top + Theme::PANEL_TITLE_HEIGHT };
        renderer.DrawString(PANEL_TITLE, titleRect, Theme::TEXT_COLOR, PANEL_TITLE_STYLE);
    }

    void EngravingFinderView::NotifyFilterChanged() const
    {
        if (const auto onFilterChanged{ m_onFilterChanged }) onFilterChanged();
    }
}
