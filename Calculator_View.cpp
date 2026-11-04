module LostArkMultiTool.Views.CalculatorView;

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
import LostArkMultiTool.Views.UI.LineChart;
import LostArkMultiTool.Views.UI.TextBox;

namespace LostArkMultiTool::Views
{
    namespace
    {
        // ===== 표 행 · 열 (원본 Form1과 같은 순서) =====
        constexpr std::size_t ROW_TOP_PRICE{ 0 };
        constexpr std::size_t ROW_BOT_PRICE{ 1 };
        constexpr std::size_t ROW_TOP_VALUE{ 2 };
        constexpr std::size_t ROW_BOT_VALUE{ 3 };
        constexpr std::size_t ROW_TOP_QTY{ 4 };
        constexpr std::size_t ROW_BOT_QTY{ 5 };
        constexpr std::size_t ROW_CHAR_TOTAL{ 6 };
        constexpr std::size_t ROW_ROSTER_TOTAL{ 7 };     // 표 맨 아래: 원정대 전체 자산
        constexpr std::size_t ROW_COUNT{ 8 };

        constexpr std::size_t COL_CATEGORY{ 0 };
        constexpr std::size_t COL_FIRST_MAT{ 1 };
        constexpr std::size_t COL_LAST_MAT{ 7 };
        constexpr std::size_t COL_TOTAL{ 8 };

        constexpr float LABEL_COLUMN_WEIGHT{ 1.3f };     // 구분 · 합계 열은 재료 열보다 조금 넓게
        constexpr float MATERIAL_COLUMN_WEIGHT{ 1.0f };

        constexpr int PRICE_DECIMALS{ 2 };               // 원본 단가 "N2"
        constexpr int AMOUNT_DECIMALS{ 0 };              // 원본 수량 · 가치 "N0"

        // ===== 글자 =====
        constexpr std::array<std::wstring_view, ROW_COUNT> ROW_LABELS{
            L"상위단가", L"하위단가", L"상위재료값", L"하위재료값", L"보유수량(상위)", L"보유수량(하위)", L"★ 캐릭터 자산 ★", L"★ 원정대 자산 ★"
        };
        constexpr std::array<std::wstring_view, COL_TOTAL + 1> COLUMN_HEADERS{
            L"구분", L"운파", L"파괴석", L"수호석", L"돌파석", L"아비도스", L"용숨", L"빙숨", L"합계(골드)"
        };
        constexpr std::wstring_view ZERO_TEXT{ L"0" };
        constexpr std::wstring_view ROSTER_QUANTITY_FORMAT{ L"{} / {}" };   // 원정대 자산 행: "상위 / 하위" 보유수량
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };

        constexpr std::wstring_view NAME_PLACEHOLDER{ L"대표 캐릭터 이름" };
        constexpr std::wstring_view SYNC_TEXT{ L"원정대 동기화" };
        constexpr std::wstring_view SYNC_BUSY_TEXT{ L"동기화 중..." };
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view REFRESH_BUSY_TEXT{ L"시세 동기화 중..." };
        constexpr std::wstring_view CHART_VALUE_SUFFIX{ L" 골드" };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 16.0f };            // 도구 막대 · 표 · 그래프 사이
        constexpr float CONTROL_GAP{ 8.0f };
        constexpr float GROUP_GAP{ 16.0f };              // 원정대 동기화 묶음과 캐릭터 선택 사이
        constexpr float NAME_BOX_WIDTH{ 200.0f };
        constexpr float SYNC_BUTTON_WIDTH{ 120.0f };
        constexpr float CHARACTER_BOX_WIDTH{ 320.0f };
        constexpr float CHARACTER_BOX_MIN_WIDTH{ 160.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 150.0f };
        constexpr float REFRESH_BUTTON_WIDTH{ 130.0f };

        // 수량 칸에 받을 글자 (숫자 · 콤마 · 소수점)
        [[nodiscard]] bool IsQuantityCharacter(wchar_t character) noexcept
        {
            return (character >= L'0' && character <= L'9') || character == L',' || character == L'.';
        }

        [[nodiscard]] bool IsMaterialColumn(std::size_t column) noexcept
        {
            return column >= COL_FIRST_MAT && column <= COL_LAST_MAT;
        }

        [[nodiscard]] std::size_t GetPriceRow(MaterialTier tier) noexcept
        {
            return tier == MaterialTier::TIER_TOP ? ROW_TOP_PRICE : ROW_BOT_PRICE;
        }

        [[nodiscard]] std::size_t GetValueRow(MaterialTier tier) noexcept
        {
            return tier == MaterialTier::TIER_TOP ? ROW_TOP_VALUE : ROW_BOT_VALUE;
        }

        // 등급 → 배열 번호 (상위 0, 하위 1 — enum 기본 타입이 int라 size_t로 명시 변환)
        [[nodiscard]] std::size_t GetTierIndex(MaterialTier tier) noexcept
        {
            return static_cast<std::size_t>(std::to_underlying(tier));
        }

        [[nodiscard]] std::size_t GetQuantityRow(MaterialTier tier) noexcept
        {
            return tier == MaterialTier::TIER_TOP ? ROW_TOP_QTY : ROW_BOT_QTY;
        }

        // 재료 번호가 표의 재료 열 범위 밖이면 예외 (경계 검사)
        void CheckMaterialIndex(std::size_t materialIndex)
        {
            if (!IsMaterialColumn(materialIndex)) throw std::out_of_range{ "재료 번호가 범위를 벗어났어요!" };
        }

        [[nodiscard]] std::vector<UI::GridColumn> MakeColumns()
        {
            std::vector<UI::GridColumn> columns;
            for (std::size_t column{}; column < COLUMN_HEADERS.size(); ++column)
            {
                const bool isLabelColumn{ column == COL_CATEGORY || column == COL_TOTAL };
                columns.push_back(UI::GridColumn{ std::wstring{ COLUMN_HEADERS[column] },
                    isLabelColumn ? LABEL_COLUMN_WEIGHT : MATERIAL_COLUMN_WEIGHT,
                    column == COL_CATEGORY ? Core::TextAlign::LEADING : Core::TextAlign::TRAILING });
            }
            return columns;
        }
    }

    CalculatorView::CalculatorView(Core::Platform& platform)
        : m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }
        , m_syncButton{ m_host.Emplace<UI::Button>(std::wstring{ SYNC_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_characterBox{ m_host.Emplace<UI::ComboBox>() }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::format(L"{}분마다 자동 갱신", Core::APPConstants::AUTO_REFRESH_MINUTES),
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_grid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), ROW_COUNT) }
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
        m_characterBox.SetOnSelectionChanged([this]
        {
            if (const auto onCharacterSelected{ m_onCharacterSelected }) onCharacterSelected();
        });
        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        m_chart.SetValueSuffix(std::wstring{ CHART_VALUE_SUFFIX });

        InitializeGrid();
    }

    CalculatorView::~CalculatorView() = default;

    void CalculatorView::SetOnSyncRoster(std::function<void(const std::wstring& mainCharacterName)> onSyncRoster)
    {
        m_onSyncRoster = std::move(onSyncRoster);
    }

    void CalculatorView::SetOnRefresh(std::function<void()> onRefresh)
    {
        m_onRefresh = std::move(onRefresh);
    }

    void CalculatorView::SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged)
    {
        m_onAutoRefreshChanged = std::move(onAutoRefreshChanged);
    }

    void CalculatorView::SetOnCharacterSelected(std::function<void()> onCharacterSelected)
    {
        m_onCharacterSelected = std::move(onCharacterSelected);
    }

    void CalculatorView::SetOnQuantityEdited(std::function<void(MaterialTier tier, std::size_t materialIndex, double quantity)> onQuantityEdited)
    {
        m_onQuantityEdited = std::move(onQuantityEdited);
    }

    void CalculatorView::SetCharacterItems(std::vector<std::wstring> items, std::optional<std::size_t> selectedIndex)
    {
        m_characterBox.SetItems(std::move(items));
        m_characterBox.SetSelectedIndex(selectedIndex);
    }

    std::optional<std::size_t> CalculatorView::GetSelectedCharacterIndex() const noexcept
    {
        return m_characterBox.GetSelectedIndex();
    }

    // 단가: 상승 빨강 ▲ / 하락 파랑 ▼ / 변동 없음 기본색 (원본 FormatPriceCell)
    void CalculatorView::ShowPrice(MaterialTier tier, std::size_t materialIndex, double current, double diff)
    {
        CheckMaterialIndex(materialIndex);

        std::wstring text{ Core::NumberFormat::FormatNumber(current, PRICE_DECIMALS) };
        Core::Color textColor{ Theme::TEXT_COLOR };
        if (diff > 0.0)
        {
            text += std::format(L" ({}{})", UP_MARK, Core::NumberFormat::FormatNumber(diff, PRICE_DECIMALS));
            textColor = Theme::UP_TEXT_COLOR;
        }
        else if (diff < 0.0)
        {
            text += std::format(L" ({}{})", DOWN_MARK, Core::NumberFormat::FormatNumber(std::abs(diff), PRICE_DECIMALS));
            textColor = Theme::DOWN_TEXT_COLOR;
        }

        const std::size_t row{ GetPriceRow(tier) };
        m_grid.SetCellText(row, materialIndex, std::move(text));
        m_grid.SetCellStyle(row, materialIndex, UI::GridCellStyle{ Theme::READONLY_BACK_COLOR, textColor });
    }

    // 수량 + 지난 입력 대비 증감 "1,120 (▲120)" — 편집을 시작하면 입력칸에는 수량 "1,120"만 채움
    void CalculatorView::ShowQuantity(MaterialTier tier, std::size_t materialIndex, double quantity, double change)
    {
        CheckMaterialIndex(materialIndex);

        const std::wstring quantityText{ Core::NumberFormat::FormatNumber(quantity, AMOUNT_DECIMALS) };
        const double roundedChange{ std::round(change) };   // 표시 자릿수(N0) 기준으로 증감이 있는지 판단

        std::wstring text{ quantityText };
        if (roundedChange > 0.0) text += std::format(L" ({}{})", UP_MARK, Core::NumberFormat::FormatNumber(roundedChange, AMOUNT_DECIMALS));
        else if (roundedChange < 0.0) text += std::format(L" ({}{})", DOWN_MARK, Core::NumberFormat::FormatNumber(std::abs(roundedChange), AMOUNT_DECIMALS));

        const std::size_t row{ GetQuantityRow(tier) };
        m_grid.SetCellText(row, materialIndex, std::move(text));
        if (roundedChange != 0.0) m_grid.SetCellEditText(row, materialIndex, quantityText);

        m_quantityChanges.at(GetTierIndex(tier)).at(materialIndex) = roundedChange;
        ApplyQuantityStyle(tier, materialIndex);
    }

    void CalculatorView::ShowMaterialValue(MaterialTier tier, std::size_t materialIndex, double value)
    {
        CheckMaterialIndex(materialIndex);
        m_grid.SetCellText(GetValueRow(tier), materialIndex, Core::NumberFormat::FormatNumber(value, AMOUNT_DECIMALS));
    }

    void CalculatorView::ShowValueTotals(double topTotal, double bottomTotal)
    {
        m_grid.SetCellText(ROW_TOP_VALUE, COL_TOTAL, Core::NumberFormat::FormatNumber(topTotal, AMOUNT_DECIMALS));
        m_grid.SetCellText(ROW_BOT_VALUE, COL_TOTAL, Core::NumberFormat::FormatNumber(bottomTotal, AMOUNT_DECIMALS));
        m_grid.SetCellText(ROW_CHAR_TOTAL, COL_TOTAL, Core::NumberFormat::FormatNumber(topTotal + bottomTotal, AMOUNT_DECIMALS));
    }

    // 잠긴 수량 행은 어둡게 (원본 SetQtyRowEditable, 구분 · 합계 칸 색은 그대로)
    void CalculatorView::SetQuantityEditable(MaterialTier tier, bool isEditable)
    {
        const std::size_t row{ GetQuantityRow(tier) };
        m_isQuantityEditable.at(GetTierIndex(tier)) = isEditable;

        for (std::size_t column{ COL_FIRST_MAT }; column <= COL_LAST_MAT; ++column)
        {
            m_grid.SetCellEditable(row, column, isEditable);
            ApplyQuantityStyle(tier, column);
        }
    }

    // 잠긴 칸: 흐린 글자 / 편집 칸: 증가 빨강 · 감소 파랑 · 그대로면 기본 글자색 (단가 등락 색과 같은 규칙)
    void CalculatorView::ApplyQuantityStyle(MaterialTier tier, std::size_t materialIndex)
    {
        const std::size_t tierIndex{ GetTierIndex(tier) };
        const bool isEditable{ m_isQuantityEditable.at(tierIndex) };
        const double change{ m_quantityChanges.at(tierIndex).at(materialIndex) };

        Core::Color textColor{ isEditable ? Theme::TEXT_COLOR : Theme::MUTED_TEXT_COLOR };
        if (isEditable && change > 0.0) textColor = Theme::UP_TEXT_COLOR;
        else if (isEditable && change < 0.0) textColor = Theme::DOWN_TEXT_COLOR;

        m_grid.SetCellStyle(GetQuantityRow(tier), materialIndex,
            UI::GridCellStyle{ isEditable ? Theme::CELL_BACK_COLOR : Theme::READONLY_BACK_COLOR, textColor });
    }

    void CalculatorView::ShowRosterTotal(double rosterTotal)
    {
        m_grid.SetCellText(ROW_ROSTER_TOTAL, COL_TOTAL, Core::NumberFormat::FormatNumber(rosterTotal, AMOUNT_DECIMALS));
    }

    // 상위 · 하위는 다른 아이템이라 더하지 않고, 둘 다 있을 때만 나란히 표시 (한쪽만 있으면 그 수량, 둘 다 없으면 0)
    void CalculatorView::ShowRosterQuantity(std::size_t materialIndex, double topQuantity, double bottomQuantity)
    {
        CheckMaterialIndex(materialIndex);

        const std::wstring topText{ Core::NumberFormat::FormatNumber(topQuantity, AMOUNT_DECIMALS) };
        const std::wstring bottomText{ Core::NumberFormat::FormatNumber(bottomQuantity, AMOUNT_DECIMALS) };
        std::wstring text{ topText };
        if (topQuantity > 0.0 && bottomQuantity > 0.0) text = std::format(ROSTER_QUANTITY_FORMAT, topText, bottomText);
        else if (bottomQuantity > 0.0) text = bottomText;

        m_grid.SetCellText(ROW_ROSTER_TOTAL, materialIndex, std::move(text));
    }

    void CalculatorView::CommitEdit()
    {
        m_grid.CommitEdit();
    }

    void CalculatorView::SetChart(std::wstring title, std::vector<UI::ChartPoint> points)
    {
        m_chart.SetTitle(std::move(title));
        m_chart.SetPoints(std::move(points));
    }

    void CalculatorView::SetRefreshBusy(bool isBusy)
    {
        m_refreshButton.SetEnabled(!isBusy);
        m_refreshButton.SetText(std::wstring{ isBusy ? REFRESH_BUSY_TEXT : REFRESH_TEXT });
    }

    void CalculatorView::SetSyncBusy(bool isBusy)
    {
        m_syncButton.SetEnabled(!isBusy);
        m_syncButton.SetText(std::wstring{ isBusy ? SYNC_BUSY_TEXT : SYNC_TEXT });
    }

    void CalculatorView::SetAutoRefreshChecked(bool isChecked) noexcept
    {
        m_autoRefreshBox.SetChecked(isChecked);
    }

    void CalculatorView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void CalculatorView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        m_host.Render(renderer);
    }

    bool CalculatorView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape CalculatorView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void CalculatorView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void CalculatorView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 표 첫 모양: 행 이름 · 0 · 칸 색 (원본 InitializeGrid)
    void CalculatorView::InitializeGrid()
    {
        for (std::size_t row{}; row < ROW_COUNT; ++row)
        {
            m_grid.SetCellText(row, COL_CATEGORY, std::wstring{ ROW_LABELS[row] });
            m_grid.SetCellStyle(row, COL_CATEGORY, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR });

            // ★ 행 (캐릭터 자산 · 원정대 자산): 합계는 골드 강조, 재료 칸은 캐릭터 자산 행만 비움 (원정대 자산 행 = 원정대 전체 보유수량)
            const bool isAssetRow{ row == ROW_CHAR_TOTAL || row == ROW_ROSTER_TOTAL };
            const UI::GridCellStyle rowStyle{ isAssetRow ? Theme::HIGHLIGHT_BACK_COLOR : Theme::READONLY_BACK_COLOR,
                isAssetRow ? Theme::HIGHLIGHT_TEXT_COLOR : Theme::TEXT_COLOR };
            for (std::size_t column{ COL_FIRST_MAT }; column <= COL_LAST_MAT; ++column)
            {
                m_grid.SetCellText(row, column, row == ROW_CHAR_TOTAL ? std::wstring{} : std::wstring{ ZERO_TEXT });
                m_grid.SetCellStyle(row, column, rowStyle);
            }

            // 합계 열: 가치 행 · ★ 행만 숫자
            const bool hasTotal{ row == ROW_TOP_VALUE || row == ROW_BOT_VALUE || isAssetRow };
            m_grid.SetCellText(row, COL_TOTAL, hasTotal ? std::wstring{ ZERO_TEXT } : std::wstring{});
            m_grid.SetCellStyle(row, COL_TOTAL, isAssetRow
                ? UI::GridCellStyle{ Theme::HIGHLIGHT_BACK_COLOR, Theme::HIGHLIGHT_TEXT_COLOR, Core::FontWeight::BOLD }
            : UI::GridCellStyle{ Theme::TOTAL_BACK_COLOR, Theme::TOTAL_TEXT_COLOR });
        }

        // 수량 행: 처음엔 둘 다 편집 가능 (캐릭터를 고르면 레벨에 따라 잠금)
        SetQuantityEditable(MaterialTier::TIER_TOP, true);
        SetQuantityEditable(MaterialTier::TIER_BOTTOM, true);

        m_grid.SetEditInput(false, IsQuantityCharacter);   // 숫자 칸: IME 끔 (한/영 상태와 상관없이 숫자 입력)
        m_grid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text) { HandleCellEdited(row, column, text); });
    }

    // 도구 막대(위) → 표 → 그래프(남은 높이 전부)
    void CalculatorView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ contentArea.m_right - PAGE_PADDING };
        const float toolbarTop{ contentArea.m_top + PAGE_PADDING };
        const float toolbarBottom{ toolbarTop + Theme::CONTROL_HEIGHT };

        // 왼쪽: 대표 캐릭터 이름 · 원정대 동기화 · 캐릭터 선택
        float x{ left };
        m_nameBox.SetBounds(Core::RectF{ x, toolbarTop, x + NAME_BOX_WIDTH, toolbarBottom });
        x += NAME_BOX_WIDTH + CONTROL_GAP;
        m_syncButton.SetBounds(Core::RectF{ x, toolbarTop, x + SYNC_BUTTON_WIDTH, toolbarBottom });
        x += SYNC_BUTTON_WIDTH + GROUP_GAP;

        // 오른쪽: 자동 갱신 · 시세 갱신
        const float refreshLeft{ right - REFRESH_BUTTON_WIDTH };
        m_refreshButton.SetBounds(Core::RectF{ refreshLeft, toolbarTop, right, toolbarBottom });
        const float autoRefreshLeft{ refreshLeft - CONTROL_GAP - AUTO_REFRESH_WIDTH };
        m_autoRefreshBox.SetBounds(Core::RectF{ autoRefreshLeft, toolbarTop, refreshLeft - CONTROL_GAP, toolbarBottom });

        // 캐릭터 선택은 남는 공간에 맞춰 줄어듦
        const float characterRight{ std::min(x + CHARACTER_BOX_WIDTH, autoRefreshLeft - GROUP_GAP) };
        m_characterBox.SetBounds(Core::RectF{ x, toolbarTop, std::max(characterRight, x + CHARACTER_BOX_MIN_WIDTH), toolbarBottom });

        const float gridTop{ toolbarBottom + SECTION_GAP };
        const float gridBottom{ gridTop + m_grid.GetPreferredHeight() };
        m_grid.SetBounds(Core::RectF{ left, gridTop, right, gridBottom });

        const float chartTop{ gridBottom + SECTION_GAP };
        const float chartBottom{ std::max(chartTop, contentArea.m_bottom - PAGE_PADDING) };
        m_chart.SetBounds(Core::RectF{ left, chartTop, right, chartBottom });
    }

    // 수량 칸 편집 확정 → 숫자로 바꿔 보기 좋게 다시 쓰고 알림 (원본 CellValueChanged)
    void CalculatorView::HandleCellEdited(std::size_t row, std::size_t column, const std::wstring& text)
    {
        if ((row != ROW_TOP_QTY && row != ROW_BOT_QTY) || !IsMaterialColumn(column)) return;

        const double quantity{ Core::NumberFormat::ParseNumber(text) };
        m_grid.SetCellText(row, column, Core::NumberFormat::FormatNumber(quantity, AMOUNT_DECIMALS));

        const MaterialTier tier{ row == ROW_TOP_QTY ? MaterialTier::TIER_TOP : MaterialTier::TIER_BOTTOM };
        if (const auto onQuantityEdited{ m_onQuantityEdited }) onQuantityEdited(tier, column, quantity);
    }
}