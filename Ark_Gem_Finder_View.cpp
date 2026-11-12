module LostArkMultiTool.Views.ArkGemFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.BoundGemData;
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
        constexpr std::size_t COLUMN_COUNT{ 5 };
        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"젬", L"현재 최저가", L"전일 평균가", L"최근 거래가", L"전일 대비" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ 1.7f, 1.3f, 1.0f, 1.0f, 0.8f };

        // ===== 귀속 젬 계산기 표 열 (젬 | 등급마다 수량 · 단가 | 합계) =====
        constexpr std::size_t BOUND_COL_NAME{ 0 };
        constexpr std::size_t BOUND_COL_FIRST_GRADE{ 1 };
        constexpr std::size_t COLUMNS_PER_GRADE{ 2 };      // 수량 · 단가
        constexpr std::size_t QUANTITY_OFFSET{ 0 };
        constexpr std::size_t UNIT_PRICE_OFFSET{ 1 };
        constexpr std::size_t BOUND_COL_VALUE{ BOUND_COL_FIRST_GRADE + Models::BoundGemData::GRADE_COUNT * COLUMNS_PER_GRADE };
        constexpr std::size_t TOTAL_ROW{ Models::BoundGemData::GEM_COUNT };   // 고른 대상(캐릭터 · 원정대 전체)의 합계
        constexpr std::size_t ROSTER_TOTAL_ROW{ TOTAL_ROW + 1 };                // 원정대 전체 합계 (항상 표시)
        constexpr std::size_t BOUND_ROW_COUNT{ ROSTER_TOTAL_ROW + 1 };          // 젬 6행 + 합계 행 2개
        constexpr float BOUND_NAME_WEIGHT{ 1.4f };
        constexpr float BOUND_CELL_WEIGHT{ 0.8f };
        constexpr float BOUND_VALUE_WEIGHT{ 1.1f };
        constexpr std::wstring_view QUANTITY_HEADER_FORMAT{ L"{} 수량" };
        constexpr std::wstring_view UNIT_PRICE_HEADER_FORMAT{ L"{} 단가" };
        constexpr std::wstring_view VALUE_HEADER{ L"합계(골드)" };
        constexpr std::wstring_view CHARACTER_TOTAL_ROW_TEXT{ L"★ 캐릭터 가치 ★" };
        constexpr std::wstring_view SELECTED_ROSTER_ROW_TEXT{ L"★ 원정대 전체 ★" };   // 원정대 전체를 골랐을 때 합계 행 이름
        constexpr std::wstring_view ROSTER_TOTAL_ROW_TEXT{ L"★ 원정대 총 가치 ★" };
        constexpr std::wstring_view BOUND_PANEL_TITLE{ L"귀속 젬 계산기 (단가 = 현재 최저가 · 최근 거래가 · 전일 평균가 중 가장 낮은 값)" };

        // ===== 원정대 동기화 · 캐릭터 선택 (귀속재료 계산기와 같은 글자) =====
        constexpr std::wstring_view NAME_PLACEHOLDER{ L"대표 캐릭터 이름" };
        constexpr std::wstring_view SYNC_TEXT{ L"원정대 동기화" };
        constexpr std::wstring_view SYNC_BUSY_TEXT{ L"동기화 중..." };

        // 시세 표 높이 (질서 · 혼돈 각 3종 → 데이터가 오기 전에도 같은 높이)
        constexpr std::size_t PRICE_TABLE_ROWS{ Models::BoundGemData::GEM_COUNT / 2 };

        constexpr int AMOUNT_DECIMALS{ 0 };
        constexpr int RATE_DECIMALS{ 1 };

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view AUTO_REFRESH_FORMAT{ L"{}분마다 자동 갱신" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view ORDER_PANEL_TITLE{ L"질서의 젬 (안정 · 불변 · 견고)" };
        constexpr std::wstring_view CHAOS_PANEL_TITLE{ L"혼돈의 젬 (침식 · 왜곡 · 붕괴)" };
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
        constexpr float NAME_BOX_WIDTH{ 200.0f };
        constexpr float SYNC_BUTTON_WIDTH{ 120.0f };
        constexpr float CHARACTER_BOX_WIDTH{ 320.0f };
        constexpr float CHARACTER_BOX_MIN_WIDTH{ 160.0f };
        constexpr float CALCULATOR_BAR_GAP{ 8.0f };   // 동기화 줄과 표 사이
        constexpr float CALCULATOR_WIDTH_RATIO{ 0.64f }; // 아래 영역 중 계산기 몫 (나머지 = 그래프)

        // ===== 그래프 =====
        constexpr std::wstring_view CHART_PANEL_TITLE{ L"원정대 귀속 젬 가치 그래프" };
        constexpr std::wstring_view CHART_VALUE_SUFFIX{ L" 골드" };

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

        // 수량 칸에 받을 글자 (숫자 · 콤마)
        [[nodiscard]] bool IsQuantityCharacter(wchar_t character) noexcept
        {
            return (character >= L'0' && character <= L'9') || character == L',';
        }

        [[nodiscard]] std::size_t GetQuantityColumn(std::size_t gradeIndex) noexcept
        {
            return BOUND_COL_FIRST_GRADE + gradeIndex * COLUMNS_PER_GRADE + QUANTITY_OFFSET;
        }

        [[nodiscard]] std::size_t GetUnitPriceColumn(std::size_t gradeIndex) noexcept
        {
            return BOUND_COL_FIRST_GRADE + gradeIndex * COLUMNS_PER_GRADE + UNIT_PRICE_OFFSET;
        }

        // 귀속 젬 표 열: 젬 | 고급 수량 · 단가 | 희귀 수량 · 단가 | 영웅 수량 · 단가 | 합계
        [[nodiscard]] std::vector<UI::GridColumn> MakeBoundGemColumns()
        {
            std::vector<UI::GridColumn> columns{ UI::GridColumn{ std::wstring{ COLUMN_HEADERS[COL_NAME] }, BOUND_NAME_WEIGHT, Core::TextAlign::LEADING } };
            for (const std::string_view gradeName : Models::BoundGemData::GRADE_NAMES)
            {
                const std::wstring grade{ Core::TextEncoding::UTF8ToWide(gradeName) };
                columns.push_back(UI::GridColumn{ std::format(QUANTITY_HEADER_FORMAT, grade), BOUND_CELL_WEIGHT, Core::TextAlign::TRAILING });
                columns.push_back(UI::GridColumn{ std::format(UNIT_PRICE_HEADER_FORMAT, grade), BOUND_CELL_WEIGHT, Core::TextAlign::TRAILING });
            }
            columns.push_back(UI::GridColumn{ std::wstring{ VALUE_HEADER }, BOUND_VALUE_WEIGHT, Core::TextAlign::TRAILING });
            return columns;
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

    ArkGemFinderView::ArkGemFinderView(Core::Platform& platform)
        : m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::format(AUTO_REFRESH_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES),
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_orderGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_chaosGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }
        , m_syncButton{ m_host.Emplace<UI::Button>(std::wstring{ SYNC_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_characterBox{ m_host.Emplace<UI::ComboBox>() }
        , m_boundGemGrid{ m_host.Emplace<UI::Grid>(platform, MakeBoundGemColumns(), BOUND_ROW_COUNT) }
        , m_chart{ m_host.Emplace<UI::LineChart>() }
        , m_orderPanel{ Core::RectF{}, std::wstring{ ORDER_PANEL_TITLE } }
        , m_chaosPanel{ Core::RectF{}, std::wstring{ CHAOS_PANEL_TITLE } }
        , m_boundGemPanel{ Core::RectF{}, std::wstring{ BOUND_PANEL_TITLE } }
        , m_chartPanel{ Core::RectF{}, std::wstring{ CHART_PANEL_TITLE } }
    {
        // 그래프: 귀속재료 계산기와 같은 골드 곡선 + 말풍선 "N 골드" + 직전 기록 대비
        m_chart.SetSeriesColor(Theme::CHART_LINE_COLOR);
        m_chart.SetValueSuffix(std::wstring{ CHART_VALUE_SUFFIX });
        m_chart.SetChangeShown(true);

        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 원정대 동기화: 버튼 · 이름 칸 Enter (이름 해석 · 요청은 매니저가)
        m_nameBox.SetOnSubmit([this]
        {
            if (const auto onSyncRoster{ m_onSyncRoster }) onSyncRoster(m_nameBox.GetText());
        });
        m_syncButton.SetOnClick([this]
        {
            if (const auto onSyncRoster{ m_onSyncRoster }) onSyncRoster(m_nameBox.GetText());
        });
        m_characterBox.SetOnSelectionChanged([this]
        {
            m_boundGemGrid.CommitEdit();   // 입력 중이던 수량은 바꾸기 전 캐릭터에 반영
            if (const auto onCharacterSelected{ m_onCharacterSelected }) onCharacterSelected();
        });

        // 시세 표: 읽기 전용, 행 단위 선택
        m_orderGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        m_chaosGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);

        InitializeBoundGemGrid();
    }

    ArkGemFinderView::~ArkGemFinderView() = default;

    void ArkGemFinderView::SetOnRefresh(std::function<void()> onRefresh)
    {
        m_onRefresh = std::move(onRefresh);
    }

    void ArkGemFinderView::SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged)
    {
        m_onAutoRefreshChanged = std::move(onAutoRefreshChanged);
    }

    void ArkGemFinderView::SetOnQuantityEdited(std::function<void(std::size_t gemIndex, std::size_t gradeIndex, const std::wstring& text)> onQuantityEdited)
    {
        m_onQuantityEdited = std::move(onQuantityEdited);
    }

    void ArkGemFinderView::SetOnSyncRoster(std::function<void(const std::wstring& mainCharacterName)> onSyncRoster)
    {
        m_onSyncRoster = std::move(onSyncRoster);
    }

    void ArkGemFinderView::SetOnCharacterSelected(std::function<void()> onCharacterSelected)
    {
        m_onCharacterSelected = std::move(onCharacterSelected);
    }

    void ArkGemFinderView::SetCharacterItems(std::vector<std::wstring> items, std::optional<std::size_t> selectedIndex)
    {
        m_characterBox.SetItems(std::move(items));
        m_characterBox.SetSelectedIndex(selectedIndex);
    }

    std::optional<std::size_t> ArkGemFinderView::GetSelectedCharacterIndex() const noexcept
    {
        return m_characterBox.GetSelectedIndex();
    }

    void ArkGemFinderView::SetSyncBusy(bool isBusy)
    {
        m_syncButton.SetEnabled(!isBusy);
        m_syncButton.SetText(std::wstring{ isBusy ? SYNC_BUSY_TEXT : SYNC_TEXT });
    }

    // 수량 칸 편집 가능 여부 + 합계 행 이름 (캐릭터 = ★ 캐릭터 가치 ★ / 원정대 전체 = ★ 원정대 전체 ★)
    void ArkGemFinderView::SetQuantityEditable(bool isEditable)
    {
        if (!isEditable) m_boundGemGrid.CommitEdit();
        m_isQuantityEditable = isEditable;

        for (std::size_t row{}; row < Models::BoundGemData::GEM_COUNT; ++row)
        {
            for (std::size_t gradeIndex{}; gradeIndex < Models::BoundGemData::GRADE_COUNT; ++gradeIndex)
            {
                m_boundGemGrid.SetCellEditable(row, GetQuantityColumn(gradeIndex), isEditable);
            }
        }
        m_boundGemGrid.SetCellText(TOTAL_ROW, BOUND_COL_NAME, std::wstring{ isEditable ? CHARACTER_TOTAL_ROW_TEXT : SELECTED_ROSTER_ROW_TEXT });
    }

    void ArkGemFinderView::CommitEdit()
    {
        m_boundGemGrid.CommitEdit();
    }

    void ArkGemFinderView::SetAutoRefreshChecked(bool isChecked) noexcept
    {
        m_autoRefreshBox.SetChecked(isChecked);
    }

    bool ArkGemFinderView::IsAutoRefreshChecked() const noexcept
    {
        return m_autoRefreshBox.IsChecked();
    }

    void ArkGemFinderView::SetRefreshEnabled(bool isEnabled) noexcept
    {
        m_refreshButton.SetEnabled(isEnabled);
    }

    void ArkGemFinderView::SetStatusText(std::wstring text)
    {
        m_statusLabel.SetText(std::move(text));
    }

    void ArkGemFinderView::ShowOrder(const std::vector<ArkGemRow>& rows)
    {
        FillGrid(m_orderGrid, rows);
    }

    void ArkGemFinderView::ShowChaos(const std::vector<ArkGemRow>& rows)
    {
        FillGrid(m_chaosGrid, rows);
    }

    // 표 다시 채우기 (이름 = 등급색, 현재 최저가 = 등락색 + ▲▼, 전일 대비 = 변동률)
    void ArkGemFinderView::FillGrid(UI::Grid& grid, const std::vector<ArkGemRow>& rows)
    {
        const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

        grid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const ArkGemRow& gem{ rows[row] };

            grid.SetCellText(row, COL_NAME, gem.m_name);
            grid.SetCellStyle(row, COL_NAME, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GetGradeColor(gem.m_gradeName), Core::FontWeight::BOLD });

            // 현재 최저가 + 직전 갱신 대비 등락 (0으로 반올림되면 표시하지 않음)
            std::wstring currentText{ FormatPrice(gem.m_currentMinPrice) };
            const std::int64_t diff{ gem.m_diff.has_value() ? std::llround(*gem.m_diff) : 0 };
            if (diff > 0) currentText += std::format(L" ({}{})", UP_MARK, FormatAmount(static_cast<double>(diff)));
            else if (diff < 0) currentText += std::format(L" ({}{})", DOWN_MARK, FormatAmount(static_cast<double>(-diff)));
            grid.SetCellText(row, COL_CURRENT, std::move(currentText));
            grid.SetCellStyle(row, COL_CURRENT, gem.m_currentMinPrice > 0.0
                ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(static_cast<double>(diff)), Core::FontWeight::BOLD } : mutedStyle);

            grid.SetCellText(row, COL_YESTERDAY, FormatPrice(gem.m_yesterdayAvgPrice));
            grid.SetCellStyle(row, COL_YESTERDAY, gem.m_yesterdayAvgPrice > 0.0 ? normalStyle : mutedStyle);
            grid.SetCellText(row, COL_RECENT, FormatPrice(gem.m_recentPrice));
            grid.SetCellStyle(row, COL_RECENT, gem.m_recentPrice > 0.0 ? normalStyle : mutedStyle);

            // 전일 대비 변동률 ("+3.2%" 빨강 / "-1.5%" 파랑)
            if (!gem.m_dayChangeRate.has_value())
            {
                grid.SetCellText(row, COL_CHANGE, std::wstring{ EMPTY_TEXT });
                grid.SetCellStyle(row, COL_CHANGE, mutedStyle);
                continue;
            }

            const double rate{ *gem.m_dayChangeRate };
            const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
            grid.SetCellText(row, COL_CHANGE, sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX });
            grid.SetCellStyle(row, COL_CHANGE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(rate) });
        }
    }

    // 귀속 젬 표 첫 모양: 젬 이름 (구분 열) · 수량 칸 편집 가능 · 합계 행 2개 강조
    void ArkGemFinderView::InitializeBoundGemGrid()
    {
        for (std::size_t row{}; row < BOUND_ROW_COUNT; ++row)
        {
            const bool isTotal{ row >= TOTAL_ROW };
            const UI::GridCellStyle labelStyle{ isTotal ? Theme::HIGHLIGHT_BACK_COLOR : Theme::LABEL_BACK_COLOR,
                isTotal ? Theme::HIGHLIGHT_TEXT_COLOR : Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD };
            std::wstring label;
            if (row == TOTAL_ROW) label = CHARACTER_TOTAL_ROW_TEXT;
            else if (row == ROSTER_TOTAL_ROW) label = ROSTER_TOTAL_ROW_TEXT;
            else label = Core::TextEncoding::UTF8ToWide(Models::BoundGemData::GEM_NAMES[row]);
            m_boundGemGrid.SetCellText(row, BOUND_COL_NAME, std::move(label));
            m_boundGemGrid.SetCellStyle(row, BOUND_COL_NAME, labelStyle);

            if (isTotal) continue;
            for (std::size_t gradeIndex{}; gradeIndex < Models::BoundGemData::GRADE_COUNT; ++gradeIndex)
            {
                m_boundGemGrid.SetCellEditable(row, GetQuantityColumn(gradeIndex), true);
            }
        }

        // 숫자 칸: IME 끔, 확정하면 젬 · 등급 번호로 바꿔서 매니저에 알림 (단가 · 합계 · 합계 행은 편집 불가)
        m_boundGemGrid.SetEditInput(false, IsQuantityCharacter);
        m_boundGemGrid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text)
        {
            if (row >= Models::BoundGemData::GEM_COUNT || column < BOUND_COL_FIRST_GRADE || column >= BOUND_COL_VALUE) return;
            if ((column - BOUND_COL_FIRST_GRADE) % COLUMNS_PER_GRADE != QUANTITY_OFFSET) return;

            const std::size_t gradeIndex{ (column - BOUND_COL_FIRST_GRADE) / COLUMNS_PER_GRADE };
            if (const auto onQuantityEdited{ m_onQuantityEdited }) onQuantityEdited(row, gradeIndex, text);
        });
    }

    // 수량 · 단가 · 합계 채우기 + 맨 아래 합계 행 (등급별 수량 합 · 총 가치)
    void ArkGemFinderView::ShowBoundGems(const std::vector<BoundGemRow>& rows)
    {
        // 원정대 전체(읽기 전용)면 수량 칸 배경도 읽기 전용 색
        const UI::GridCellStyle quantityStyle{ m_isQuantityEditable ? Theme::CELL_BACK_COLOR : Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle priceStyle{ Theme::READONLY_BACK_COLOR, Theme::MUTED_TEXT_COLOR };
        const UI::GridCellStyle valueStyle{ Theme::TOTAL_BACK_COLOR, Theme::TOTAL_TEXT_COLOR, Core::FontWeight::BOLD };
        const UI::GridCellStyle totalStyle{ Theme::HIGHLIGHT_BACK_COLOR, Theme::HIGHLIGHT_TEXT_COLOR, Core::FontWeight::BOLD };

        std::array<int, Models::BoundGemData::GRADE_COUNT> gradeTotals{};
        double totalValue{};
        const std::size_t rowCount{ std::min(rows.size(), Models::BoundGemData::GEM_COUNT) };
        for (std::size_t row{}; row < rowCount; ++row)
        {
            const BoundGemRow& gem{ rows[row] };
            for (std::size_t gradeIndex{}; gradeIndex < Models::BoundGemData::GRADE_COUNT; ++gradeIndex)
            {
                const int quantity{ gem.m_quantities[gradeIndex] };
                gradeTotals[gradeIndex] += quantity;
                m_boundGemGrid.SetCellText(row, GetQuantityColumn(gradeIndex), FormatAmount(static_cast<double>(quantity)));
                m_boundGemGrid.SetCellStyle(row, GetQuantityColumn(gradeIndex), quantityStyle);
                m_boundGemGrid.SetCellText(row, GetUnitPriceColumn(gradeIndex), FormatPrice(gem.m_unitPrices[gradeIndex]));
                m_boundGemGrid.SetCellStyle(row, GetUnitPriceColumn(gradeIndex), priceStyle);
            }
            totalValue += gem.m_value;
            m_boundGemGrid.SetCellText(row, BOUND_COL_VALUE, FormatAmount(gem.m_value));
            m_boundGemGrid.SetCellStyle(row, BOUND_COL_VALUE, valueStyle);
        }

        // 합계 행 (단가 칸은 비움)
        for (std::size_t gradeIndex{}; gradeIndex < Models::BoundGemData::GRADE_COUNT; ++gradeIndex)
        {
            m_boundGemGrid.SetCellText(TOTAL_ROW, GetQuantityColumn(gradeIndex), FormatAmount(static_cast<double>(gradeTotals[gradeIndex])));
            m_boundGemGrid.SetCellStyle(TOTAL_ROW, GetQuantityColumn(gradeIndex), totalStyle);
            m_boundGemGrid.SetCellText(TOTAL_ROW, GetUnitPriceColumn(gradeIndex), std::wstring{});
            m_boundGemGrid.SetCellStyle(TOTAL_ROW, GetUnitPriceColumn(gradeIndex), totalStyle);
        }
        m_boundGemGrid.SetCellText(TOTAL_ROW, BOUND_COL_VALUE, FormatAmount(totalValue));
        m_boundGemGrid.SetCellStyle(TOTAL_ROW, BOUND_COL_VALUE, totalStyle);
    }

    void ArkGemFinderView::SetChart(std::wstring title, std::vector<UI::ChartPoint> points)
    {
        m_chart.SetTitle(std::move(title));
        m_chart.SetPoints(std::move(points));
    }

    // ★ 원정대 총 가치 ★ 행 (단가 칸은 비움)
    void ArkGemFinderView::ShowRosterTotal(const std::array<int, Models::BoundGemData::GRADE_COUNT>& quantities, double value)
    {
        const UI::GridCellStyle totalStyle{ Theme::HIGHLIGHT_BACK_COLOR, Theme::HIGHLIGHT_TEXT_COLOR, Core::FontWeight::BOLD };
        for (std::size_t gradeIndex{}; gradeIndex < Models::BoundGemData::GRADE_COUNT; ++gradeIndex)
        {
            m_boundGemGrid.SetCellText(ROSTER_TOTAL_ROW, GetQuantityColumn(gradeIndex), FormatAmount(static_cast<double>(quantities[gradeIndex])));
            m_boundGemGrid.SetCellStyle(ROSTER_TOTAL_ROW, GetQuantityColumn(gradeIndex), totalStyle);
            m_boundGemGrid.SetCellText(ROSTER_TOTAL_ROW, GetUnitPriceColumn(gradeIndex), std::wstring{});
            m_boundGemGrid.SetCellStyle(ROSTER_TOTAL_ROW, GetUnitPriceColumn(gradeIndex), totalStyle);
        }
        m_boundGemGrid.SetCellText(ROSTER_TOTAL_ROW, BOUND_COL_VALUE, FormatAmount(value));
        m_boundGemGrid.SetCellStyle(ROSTER_TOTAL_ROW, BOUND_COL_VALUE, totalStyle);
    }

    void ArkGemFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void ArkGemFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderPanels(renderer);
        m_host.Render(renderer);
    }

    bool ArkGemFinderView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape ArkGemFinderView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void ArkGemFinderView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void ArkGemFinderView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 상단 바 + 위 (왼쪽 질서 | 오른쪽 혼돈, 3행 높이) + 아래 귀속 젬 계산기 (남은 높이)
    void ArkGemFinderView::Layout(const Core::RectF& contentArea)
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

        // 위: 시세 묶음 상자 두 개 (제목 + 머리글 + 3행이 딱 들어가는 높이)
        const float priceTop{ barBottom + SECTION_GAP };
        const float priceBottom{ priceTop + Theme::PANEL_TITLE_HEIGHT + Theme::GRID_HEADER_HEIGHT
            + Theme::GRID_ROW_HEIGHT * static_cast<float>(PRICE_TABLE_ROWS) + PANEL_PADDING + Theme::BORDER_WIDTH };
        const float middle{ left + (right - left - SECTION_GAP) * HALF };
        m_orderGrid.SetBounds(LayoutPanel(m_orderPanel, Core::RectF{ left, priceTop, middle, priceBottom }));
        m_chaosGrid.SetBounds(LayoutPanel(m_chaosPanel, Core::RectF{ middle + SECTION_GAP, priceTop, right, priceBottom }));

        // 아래: 귀속 젬 계산기 (남은 높이를 채움) = 동기화 줄 [이름] [원정대 동기화] [캐릭터 선택] + 표
        const float calculatorTop{ priceBottom + SECTION_GAP };
        const float calculatorBottom{ std::max(calculatorTop, contentArea.m_bottom - PAGE_PADDING) };
        // 왼쪽 계산기 | 오른쪽 그래프
        const float calculatorRight{ left + (right - left - SECTION_GAP) * CALCULATOR_WIDTH_RATIO };
        m_chart.SetBounds(LayoutPanel(m_chartPanel, Core::RectF{ calculatorRight + SECTION_GAP, calculatorTop, right, calculatorBottom }));
        const Core::RectF inner{ LayoutPanel(m_boundGemPanel, Core::RectF{ left, calculatorTop, calculatorRight, calculatorBottom }) };

        const float rosterTop{ inner.m_top };
        const float rosterBottom{ rosterTop + Theme::CONTROL_HEIGHT };
        float rosterX{ inner.m_left };
        m_nameBox.SetBounds(Core::RectF{ rosterX, rosterTop, rosterX + NAME_BOX_WIDTH, rosterBottom });
        rosterX += NAME_BOX_WIDTH + GROUP_GAP;
        m_syncButton.SetBounds(Core::RectF{ rosterX, rosterTop, rosterX + SYNC_BUTTON_WIDTH, rosterBottom });
        rosterX += SYNC_BUTTON_WIDTH + SECTION_GAP;
        const float characterRight{ std::min(rosterX + CHARACTER_BOX_WIDTH, inner.m_right) };
        m_characterBox.SetBounds(Core::RectF{ rosterX, rosterTop, std::max(characterRight, rosterX + CHARACTER_BOX_MIN_WIDTH), rosterBottom });

        // 표는 동기화 줄 아래 남은 높이
        const float gridTop{ rosterBottom + CALCULATOR_BAR_GAP };
        m_boundGemGrid.SetBounds(Core::RectF{ inner.m_left, gridTop, inner.m_right, std::max(gridTop, inner.m_bottom) });
    }

    Core::RectF ArkGemFinderView::LayoutPanel(Panel& panel, const Core::RectF& bounds) const
    {
        panel.m_bounds = bounds;
        return Core::RectF{ bounds.m_left + PANEL_PADDING, bounds.m_top + Theme::PANEL_TITLE_HEIGHT,
            bounds.m_right - PANEL_PADDING, std::max(bounds.m_top + Theme::PANEL_TITLE_HEIGHT, bounds.m_bottom - PANEL_PADDING) };
    }

    // 묶음 상자: 둥근 테두리 + 왼쪽 위 제목
    void ArkGemFinderView::RenderPanels(Core::Renderer& renderer) const
    {
        for (const Panel* panel : { &m_orderPanel, &m_chaosPanel, &m_boundGemPanel, &m_chartPanel })
        {
            renderer.FillRoundedRect(panel->m_bounds, Theme::PANEL_CORNER_RADIUS, Theme::CARD_BACK_COLOR);   // 카드 바탕 (배경보다 살짝 밝게)
            renderer.DrawRoundedRect(panel->m_bounds, Theme::PANEL_CORNER_RADIUS, Theme::CARD_BORDER_COLOR, Theme::BORDER_WIDTH);   // 카드 얇은 테두리

            const Core::RectF titleRect{ panel->m_bounds.m_left + PANEL_TITLE_LEFT, panel->m_bounds.m_top,
                panel->m_bounds.m_right - PANEL_TITLE_LEFT, panel->m_bounds.m_top + Theme::PANEL_TITLE_HEIGHT };
            renderer.DrawString(panel->m_title, titleRect, Theme::HEADER_TEXT_COLOR, PANEL_TITLE_STYLE);
        }
    }
}
