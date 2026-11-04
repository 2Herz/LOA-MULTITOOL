module LostArkMultiTool.Views.RefineBookFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;

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
        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"강화책", L"현재 최저가", L"전일 평균가", L"최근 거래가", L"전일 대비" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ 1.7f, 1.3f, 1.0f, 1.0f, 0.8f };

        constexpr int AMOUNT_DECIMALS{ 0 };
        constexpr int RATE_DECIMALS{ 1 };

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view AUTO_REFRESH_FORMAT{ L"{}분마다 자동 갱신" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view METALLURGY_PANEL_TITLE{ L"야금술 (무기)" };
        constexpr std::wstring_view TAILORING_PANEL_TITLE{ L"재봉술 (방어구)" };
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };
        constexpr std::wstring_view PLUS_SIGN{ L"+" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 16.0f };
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float REFRESH_BUTTON_WIDTH{ 110.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 150.0f };
        constexpr float HALF{ 0.5f };                 // 두 표를 반씩
        constexpr float PANEL_PADDING{ 10.0f };
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

        // 오름 · 내림 글자색 (▲ 빨강 / ▼ 파랑 / 변화 없음 기본색)
        [[nodiscard]] const Core::Color& GetTrendColor(double value) noexcept
        {
            if (value > 0.0) return Theme::UP_TEXT_COLOR;
            if (value < 0.0) return Theme::DOWN_TEXT_COLOR;
            return Theme::TEXT_COLOR;
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
    }

    RefineBookFinderView::RefineBookFinderView(Core::Platform& platform)
        : m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::format(AUTO_REFRESH_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES),
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_metallurgyGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_tailoringGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_metallurgyPanel{ Core::RectF{}, std::wstring{ METALLURGY_PANEL_TITLE } }
        , m_tailoringPanel{ Core::RectF{}, std::wstring{ TAILORING_PANEL_TITLE } }
    {
        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 시세 표: 읽기 전용, 행 단위 선택
        m_metallurgyGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        m_tailoringGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
    }

    RefineBookFinderView::~RefineBookFinderView() = default;

    void RefineBookFinderView::SetOnRefresh(std::function<void()> onRefresh)
    {
        m_onRefresh = std::move(onRefresh);
    }

    void RefineBookFinderView::SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged)
    {
        m_onAutoRefreshChanged = std::move(onAutoRefreshChanged);
    }

    void RefineBookFinderView::SetAutoRefreshChecked(bool isChecked) noexcept
    {
        m_autoRefreshBox.SetChecked(isChecked);
    }

    bool RefineBookFinderView::IsAutoRefreshChecked() const noexcept
    {
        return m_autoRefreshBox.IsChecked();
    }

    void RefineBookFinderView::SetRefreshEnabled(bool isEnabled) noexcept
    {
        m_refreshButton.SetEnabled(isEnabled);
    }

    void RefineBookFinderView::SetStatusText(std::wstring text)
    {
        m_statusLabel.SetText(std::move(text));
    }

    void RefineBookFinderView::ShowMetallurgy(const std::vector<RefineBookRow>& rows)
    {
        FillGrid(m_metallurgyGrid, rows);
    }

    void RefineBookFinderView::ShowTailoring(const std::vector<RefineBookRow>& rows)
    {
        FillGrid(m_tailoringGrid, rows);
    }

    // 표 다시 채우기 (이름 = 등급색, 현재 최저가 = 등락색 + ▲▼, 전일 대비 = 변동률)
    void RefineBookFinderView::FillGrid(UI::Grid& grid, const std::vector<RefineBookRow>& rows)
    {
        const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

        grid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const RefineBookRow& book{ rows[row] };

            grid.SetCellText(row, COL_NAME, book.m_name);
            grid.SetCellStyle(row, COL_NAME, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GetGradeColor(book.m_gradeName), Core::FontWeight::BOLD });

            // 현재 최저가 + 직전 갱신 대비 등락 (0으로 반올림되면 표시하지 않음)
            std::wstring currentText{ FormatPrice(book.m_currentMinPrice) };
            const std::int64_t diff{ book.m_diff.has_value() ? std::llround(*book.m_diff) : 0 };
            if (diff > 0) currentText += std::format(L" ({}{})", UP_MARK, FormatAmount(static_cast<double>(diff)));
            else if (diff < 0) currentText += std::format(L" ({}{})", DOWN_MARK, FormatAmount(static_cast<double>(-diff)));
            grid.SetCellText(row, COL_CURRENT, std::move(currentText));
            grid.SetCellStyle(row, COL_CURRENT, book.m_currentMinPrice > 0.0
                ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(static_cast<double>(diff)), Core::FontWeight::BOLD } : mutedStyle);

            grid.SetCellText(row, COL_YESTERDAY, FormatPrice(book.m_yesterdayAvgPrice));
            grid.SetCellStyle(row, COL_YESTERDAY, book.m_yesterdayAvgPrice > 0.0 ? normalStyle : mutedStyle);
            grid.SetCellText(row, COL_RECENT, FormatPrice(book.m_recentPrice));
            grid.SetCellStyle(row, COL_RECENT, book.m_recentPrice > 0.0 ? normalStyle : mutedStyle);

            // 전일 대비 변동률 ("+3.2%" 빨강 / "-1.5%" 파랑)
            if (!book.m_dayChangeRate.has_value())
            {
                grid.SetCellText(row, COL_CHANGE, std::wstring{ EMPTY_TEXT });
                grid.SetCellStyle(row, COL_CHANGE, mutedStyle);
                continue;
            }

            const double rate{ *book.m_dayChangeRate };
            const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
            grid.SetCellText(row, COL_CHANGE, sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX });
            grid.SetCellStyle(row, COL_CHANGE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(rate) });
        }
    }

    void RefineBookFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void RefineBookFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderPanels(renderer);
        m_host.Render(renderer);
    }

    bool RefineBookFinderView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape RefineBookFinderView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void RefineBookFinderView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void RefineBookFinderView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 상단 바 + (왼쪽 야금술 | 오른쪽 재봉술) 반반
    void RefineBookFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ contentArea.m_right - PAGE_PADDING };
        const float barTop{ contentArea.m_top + PAGE_PADDING };
        const float barBottom{ barTop + Theme::CONTROL_HEIGHT };

        // 상단 바: [시세 갱신] [5분마다 자동 갱신] 상태
        float x{ left };
        const auto place{ [&x, barTop, barBottom](UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, barTop, x + width, barBottom });
            x += width + gapAfter;
        } };
        place(m_refreshButton, REFRESH_BUTTON_WIDTH, GROUP_GAP);
        place(m_autoRefreshBox, AUTO_REFRESH_WIDTH, SECTION_GAP);
        m_statusLabel.SetBounds(Core::RectF{ x, barTop, std::max(x, right), barBottom });

        // 본문: 두 묶음 상자
        const float bodyTop{ barBottom + SECTION_GAP };
        const float bodyBottom{ std::max(bodyTop, contentArea.m_bottom - PAGE_PADDING) };
        const float middle{ left + (right - left - SECTION_GAP) * HALF };
        m_metallurgyGrid.SetBounds(LayoutPanel(m_metallurgyPanel, Core::RectF{ left, bodyTop, middle, bodyBottom }));
        m_tailoringGrid.SetBounds(LayoutPanel(m_tailoringPanel, Core::RectF{ middle + SECTION_GAP, bodyTop, right, bodyBottom }));
    }

    Core::RectF RefineBookFinderView::LayoutPanel(Panel& panel, const Core::RectF& bounds) const
    {
        panel.m_bounds = bounds;
        return Core::RectF{ bounds.m_left + PANEL_PADDING, bounds.m_top + Theme::PANEL_TITLE_HEIGHT,
            bounds.m_right - PANEL_PADDING, std::max(bounds.m_top + Theme::PANEL_TITLE_HEIGHT, bounds.m_bottom - PANEL_PADDING) };
    }

    // 묶음 상자: 둥근 테두리 + 왼쪽 위 제목
    void RefineBookFinderView::RenderPanels(Core::Renderer& renderer) const
    {
        for (const Panel* panel : { &m_metallurgyPanel, &m_tailoringPanel })
        {
            renderer.DrawRoundedRect(panel->m_bounds, Theme::PANEL_CORNER_RADIUS, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);

            const Core::RectF titleRect{ panel->m_bounds.m_left + PANEL_TITLE_LEFT, panel->m_bounds.m_top,
                panel->m_bounds.m_right - PANEL_TITLE_LEFT, panel->m_bounds.m_top + Theme::PANEL_TITLE_HEIGHT };
            renderer.DrawString(panel->m_title, titleRect, Theme::TEXT_COLOR, PANEL_TITLE_STYLE);
        }
    }
}
