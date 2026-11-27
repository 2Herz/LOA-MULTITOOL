module LostArkMultiTool.Views.ArkGemFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.BoundGemData;
import LostArkMultiTool.Views.PageLayout;
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
        // ===== 시세 표 열 =====
        constexpr std::size_t COL_NAME{ 0 };
        constexpr std::size_t COL_KIND{ 1 };
        constexpr std::size_t COL_CURRENT{ 2 };
        constexpr std::size_t COL_YESTERDAY{ 3 };
        constexpr std::size_t COL_RECENT{ 4 };
        constexpr std::size_t COL_CHANGE{ 5 };
        constexpr std::size_t COL_TREND{ 6 };
        constexpr std::size_t COLUMN_COUNT{ 7 };
        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"젬", L"분류", L"현재 최저가", L"전일 평균가", L"최근 거래가", L"변동", L"추이" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ 1.7f, 0.8f, 1.3f, 1.0f, 1.0f, 0.8f, 1.0f };

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

        // ===== 원정대 동기화 · 캐릭터 선택 (귀속재료 계산기와 같은 글자) =====
        constexpr std::wstring_view NAME_PLACEHOLDER{ L"대표 캐릭터 이름" };
        constexpr std::wstring_view SYNC_TEXT{ L"원정대 동기화" };
        constexpr std::wstring_view SYNC_BUSY_TEXT{ L"동기화 중..." };

        constexpr int AMOUNT_DECIMALS{ 0 };
        constexpr int RATE_DECIMALS{ 1 };

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view REFRESHING_TEXT{ L"조회 중..." };
        constexpr std::wstring_view AUTO_REFRESH_FORMAT{ L"{}분마다 자동 갱신" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view SEARCH_PLACEHOLDER{ L"젬 이름 검색" };
        constexpr std::wstring_view CLEAR_TEXT{ L"✕" };
        constexpr std::wstring_view ALL_KIND_TEXT{ L"전체 분류" };
        constexpr std::wstring_view ORDER_KIND_TEXT{ L"질서" };
        constexpr std::wstring_view CHAOS_KIND_TEXT{ L"혼돈" };
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };
        constexpr std::wstring_view PLUS_SIGN{ L"+" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };
        constexpr std::wstring_view MARKET_EYEBROW{ L"LIVE MARKET" };
        constexpr std::wstring_view MARKET_TITLE{ L"질서 · 혼돈의 젬 최저가" };
        constexpr std::wstring_view FOOTER_COUNT_FORMAT{ L"총 {}개의 검색 결과" };
        constexpr std::wstring_view EMPTY_NO_DATA_TITLE{ L"아직 불러온 시세가 없어요" };
        constexpr std::wstring_view EMPTY_NO_DATA_HINT{ L"[시세 갱신]을 눌러 거래소 시세를 불러와 보세요." };
        constexpr std::wstring_view EMPTY_NO_MATCH_TITLE{ L"조건에 맞는 젬이 없어요" };
        constexpr std::wstring_view EMPTY_NO_MATCH_HINT{ L"검색어를 줄이거나 ✕로 지우고, 분류를 \"전체 분류\"로 바꿔 보세요." };
        constexpr std::wstring_view CALCULATOR_EYEBROW{ L"BOUND GEM" };
        constexpr std::wstring_view CALCULATOR_TITLE{ L"귀속 젬 계산기" };
        constexpr std::wstring_view CALCULATOR_RIGHT_TEXT{ L"단가: 세 시세 중 가장 낮은 값" };
        constexpr std::wstring_view PRICE_CHART_EYEBROW{ L"PRICE HISTORY" };
        constexpr std::wstring_view PRICE_CHART_CARD_TITLE{ L"시세 기록 (일별 최저가)" };
        constexpr std::wstring_view CHART_EYEBROW{ L"ROSTER VALUE" };
        constexpr std::wstring_view CHART_CARD_TITLE{ L"원정대 귀속 젬 가치 그래프" };

        // 분류 · 정렬 목록 (번호 = 목록 순서)
        constexpr std::size_t KIND_ALL{ 0 };
        constexpr std::size_t KIND_ORDER{ 1 };
        constexpr std::size_t KIND_CHAOS{ 2 };
        constexpr std::size_t SORT_DEFAULT{ 0 };
        constexpr std::size_t SORT_PRICE_HIGH{ 1 };
        constexpr std::size_t SORT_PRICE_LOW{ 2 };
        constexpr std::size_t SORT_CHANGE{ 3 };

        // ===== 통계 카드 =====
        constexpr std::size_t STAT_CARD_COUNT{ 4 };
        constexpr std::wstring_view STAT_COUNT_LABEL{ L"조회 종류" };
        constexpr std::wstring_view STAT_COUNT_VALUE_FORMAT{ L"{}종" };
        constexpr std::wstring_view STAT_COUNT_SUB_FORMAT{ L"질서 {} · 혼돈 {}" };
        constexpr std::wstring_view STAT_AVERAGE_LABEL{ L"평균 전일 대비" };
        constexpr std::wstring_view STAT_AVERAGE_SUB{ L"전일 평균가 기준" };
        constexpr std::wstring_view STAT_TOP_LABEL{ L"최고가 젬" };
        constexpr std::wstring_view STAT_ROSTER_LABEL{ L"원정대 귀속 젬 가치" };
        constexpr std::wstring_view STAT_ROSTER_SUB{ L"골드" };
        constexpr std::wstring_view ICON_COUNT{ L"▤" };
        constexpr std::wstring_view ICON_AVERAGE{ L"↗" };
        constexpr std::wstring_view ICON_TOP{ L"◎" };
        constexpr std::wstring_view ICON_ROSTER{ L"◷" };

        // ===== 배치 (DIP) =====
        constexpr float CONTROL_GAP{ 8.0f };
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float SECTION_GAP{ 16.0f };
        constexpr float CLEAR_BUTTON_WIDTH{ 32.0f };
        constexpr float KIND_BOX_WIDTH{ 130.0f };
        constexpr float SORT_BOX_WIDTH{ 150.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 150.0f };
        constexpr float REFRESH_BUTTON_WIDTH{ 110.0f };
        constexpr float SEARCH_MAX_WIDTH{ 360.0f };
        constexpr float SEARCH_MIN_WIDTH{ 120.0f };
        constexpr float FILTER_GAP_COUNT{ 3.0f };   // 검색 · ✕ · 분류 · 정렬 사이 간격 수
        constexpr std::size_t MIN_TREND_POINTS{ 2 };   // 추이 선을 그리는 데 필요한 기록 수
        constexpr float HALF{ 0.5f };
        constexpr float TABLE_INSET{ 1.0f };        // 카드 테두리 안쪽
        constexpr std::size_t MIN_MARKET_ROWS{ 3 };    // 시세 카드가 최소로 확보하는 행 수 (결과가 없을 때 안내 자리)
        constexpr float MIN_BOTTOM_HEIGHT{ 300.0f };   // 아래 계산기 · 그래프 카드의 최소 높이
        constexpr float MIN_MARKET_HEIGHT{ 204.0f };   // 시세 카드 최소 높이 (머리말 + 머리글 + 2행 + 아래 줄)
        constexpr float NAME_BOX_WIDTH{ 200.0f };
        constexpr float SYNC_BUTTON_WIDTH{ 120.0f };
        constexpr float CHARACTER_BOX_WIDTH{ 320.0f };
        constexpr float CHARACTER_BOX_MIN_WIDTH{ 160.0f };
        constexpr float VISION_BOX_WIDTH{ 110.0f };        // [화면 인식] 체크 너비
        constexpr std::wstring_view VISION_TEXT{ L"화면 인식" };   // [화면 인식] (게임에서 귀속 젬 툴팁을 읽어 현재 캐릭터에 넣음)
        constexpr float CALCULATOR_BAR_GAP{ 8.0f };      // 동기화 줄과 표 사이
        constexpr float CALCULATOR_WIDTH_RATIO{ 0.64f }; // 아래 영역 중 계산기 몫 (나머지 = 그래프)
        constexpr float PRICE_CHART_HEIGHT_RATIO{ 0.5f };   // 오른쪽 그래프 영역 중 시세 기록 그래프 몫 (나머지 = 귀속 젬 가치 그래프)

        // ===== 그래프 =====
        constexpr std::wstring_view CHART_VALUE_SUFFIX{ L" 골드" };

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

        // 변동률 글자 ("+3.2%" / "-1.5%")
        [[nodiscard]] std::wstring FormatRate(double rate)
        {
            const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
            return sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX };
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
                const bool isLeading{ column == COL_NAME || column == COL_KIND };   // 이름 · 분류는 왼쪽 정렬
                columns.push_back(UI::GridColumn{ std::wstring{ COLUMN_HEADERS[column] }, COLUMN_WEIGHTS[column],
                    isLeading ? Core::TextAlign::LEADING : Core::TextAlign::TRAILING });
            }
            return columns;
        }
    }

    ArkGemFinderView::ArkGemFinderView(Core::Platform& platform)
        : m_searchBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ SEARCH_PLACEHOLDER }) }
        , m_clearButton{ m_host.Emplace<UI::Button>(std::wstring{ CLEAR_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_kindBox{ m_host.Emplace<UI::ComboBox>() }
        , m_sortBox{ m_host.Emplace<UI::ComboBox>() }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::format(AUTO_REFRESH_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES),
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_marketGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }
        , m_syncButton{ m_host.Emplace<UI::Button>(std::wstring{ SYNC_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_characterBox{ m_host.Emplace<UI::ComboBox>() }
        , m_visionBox{ m_host.Emplace<UI::CheckBox>(std::wstring{ VISION_TEXT }, [this](bool isChecked) { if (m_onVisionChanged) m_onVisionChanged(isChecked); }) }
        , m_boundGemGrid{ m_host.Emplace<UI::Grid>(platform, MakeBoundGemColumns(), BOUND_ROW_COUNT) }
        , m_chart{ m_host.Emplace<UI::LineChart>() }
        , m_priceChart{ m_host.Emplace<UI::LineChart>() }
        , m_statusText{ INITIAL_STATUS_TEXT }
    {
        // 그래프: 귀속재료 계산기와 같은 골드 곡선 + 말풍선 "N 골드" + 직전 기록 대비
        m_chart.SetSeriesColor(Theme::CHART_LINE_COLOR);
        m_chart.SetValueSuffix(std::wstring{ CHART_VALUE_SUFFIX });
        m_chart.SetChangeShown(true);

        // 시세 기록 그래프: 젬 색(보라) 곡선 + 말풍선 "N 골드" (보석 검색기 그래프와 같은 형식)
        m_priceChart.SetSeriesColor(Theme::GEM_CHART_LINE_COLOR);
        m_priceChart.SetValueSuffix(std::wstring{ CHART_VALUE_SUFFIX });

        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 검색어가 바뀔 때마다 표에 바로 반영
        m_searchBox.SetOnTextChanged([this] { ApplyFilter(); });
        m_clearButton.SetOnClick([this] { ClearSearch(); });

        // 분류 · 정렬 목록 (처음은 전체 분류 · 기본 순서)
        m_kindBox.SetItems({ std::wstring{ ALL_KIND_TEXT }, std::wstring{ ORDER_KIND_TEXT }, std::wstring{ CHAOS_KIND_TEXT } });
        m_kindBox.SetSelectedIndex(KIND_ALL);
        m_kindBox.SetOnSelectionChanged([this] { ApplyFilter(); });
        m_sortBox.SetItems({ L"기본 정렬", L"가격 높은 순", L"가격 낮은 순", L"등락 순" });
        m_sortBox.SetSelectedIndex(SORT_DEFAULT);
        m_sortBox.SetOnSelectionChanged([this] { ApplyFilter(); });

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

        // 시세 표: 읽기 전용, 행 단위 선택 (행을 고르면 그 젬의 그래프)
        m_marketGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        m_marketGrid.SetRowHeight(PageLayout::GRID_ROW_HEIGHT);
        m_marketGrid.SetOnSelectionChanged([this] { HandlePriceRowSelected(); });

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

    void ArkGemFinderView::SetOnVisionChanged(std::function<void(bool isEnabled)> onVisionChanged)
    {
        m_onVisionChanged = std::move(onVisionChanged);   // 알림 연결
    }

    void ArkGemFinderView::SetVisionChecked(bool isChecked) noexcept
    {
        m_visionBox.SetChecked(isChecked);   // 표시만 (알림 없음)
    }

    void ArkGemFinderView::SelectCharacterIndex(std::size_t index)
    {
        m_characterBox.SetSelectedIndex(index);   // 표시만 (매니저가 직접 SelectCharacter를 부름)
    }

    void ArkGemFinderView::SetOnPriceRowSelected(std::function<void(bool isOrder, std::size_t row)> onPriceRowSelected)
    {
        m_onPriceRowSelected = std::move(onPriceRowSelected);
    }

    // 표에서 고른 행의 젬을 기억하고 매니저에 알림
    void ArkGemFinderView::HandlePriceRowSelected()
    {
        const std::optional<std::size_t> row{ m_marketGrid.GetSelectedRow() };
        if (!row.has_value() || *row >= m_shownEntries.size()) return;   // 선택이 풀린 경우는 알리지 않음 (경계 검사)

        const Entry& entry{ *m_shownEntries[*row] };
        m_selectedGem = std::pair{ entry.m_isOrder, entry.m_kindIndex };
        if (const auto onPriceRowSelected{ m_onPriceRowSelected }) onPriceRowSelected(entry.m_isOrder, entry.m_kindIndex);
    }

    // 기억해 둔 젬이 지금 표에 보이면 그 행을 선택 (걸러 내서 안 보이면 선택 없음, 기억은 유지)
    void ArkGemFinderView::ReselectRow()
    {
        std::optional<std::size_t> visibleRow;
        if (m_selectedGem.has_value())
        {
            for (std::size_t row{}; row < m_shownEntries.size(); ++row)
            {
                if (m_shownEntries[row]->m_isOrder == m_selectedGem->first && m_shownEntries[row]->m_kindIndex == m_selectedGem->second)
                {
                    visibleRow = row;
                    break;
                }
            }
        }
        m_marketGrid.SelectRow(visibleRow);
    }

    // 매니저가 질서 쪽 선택을 알려 줌 (값이 없으면 질서 젬 선택이었을 때만 해제)
    void ArkGemFinderView::SelectOrderRow(std::optional<std::size_t> row)
    {
        if (row.has_value()) m_selectedGem = std::pair{ true, *row };
        else if (m_selectedGem.has_value() && m_selectedGem->first) m_selectedGem.reset();
        ReselectRow();
    }

    void ArkGemFinderView::SelectChaosRow(std::optional<std::size_t> row)
    {
        if (row.has_value()) m_selectedGem = std::pair{ false, *row };
        else if (m_selectedGem.has_value() && !m_selectedGem->first) m_selectedGem.reset();
        ReselectRow();
    }

    void ArkGemFinderView::SetPriceChart(std::wstring title, std::vector<UI::ChartPoint> points)
    {
        m_priceChart.SetTitle(std::move(title));
        m_priceChart.SetPoints(std::move(points));
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

    // 조회 중에는 버튼을 끄고 글자를 "조회 중..."으로
    void ArkGemFinderView::SetRefreshEnabled(bool isEnabled)
    {
        m_refreshButton.SetEnabled(isEnabled);
        m_refreshButton.SetText(std::wstring{ isEnabled ? REFRESH_TEXT : REFRESHING_TEXT });
    }

    void ArkGemFinderView::SetStatusText(std::wstring text)
    {
        m_statusText = std::move(text);
    }

    void ArkGemFinderView::ShowOrder(const std::vector<ArkGemRow>& rows)
    {
        m_orderEntries.clear();
        for (std::size_t index{}; index < rows.size(); ++index) m_orderEntries.push_back(Entry{ true, index, rows[index] });
        ApplyFilter();
    }

    void ArkGemFinderView::ShowChaos(const std::vector<ArkGemRow>& rows)
    {
        m_chaosEntries.clear();
        for (std::size_t index{}; index < rows.size(); ++index) m_chaosEntries.push_back(Entry{ false, index, rows[index] });
        ApplyFilter();
    }

    // 검색어 · 분류에 맞는 젬만 모아 정렬 → 표 채우기 (정렬을 고르지 않으면 서비스가 정한 순서 그대로)
    void ArkGemFinderView::ApplyFilter()
    {
        const std::wstring& keyword{ m_searchBox.GetText() };
        const std::size_t kind{ m_kindBox.GetSelectedIndex().value_or(KIND_ALL) };
        const std::size_t sort{ m_sortBox.GetSelectedIndex().value_or(SORT_DEFAULT) };

        std::vector<const Entry*> shown;
        for (const std::vector<Entry>* group : { &m_orderEntries, &m_chaosEntries })
        {
            for (const Entry& entry : *group)
            {
                if (kind == KIND_ORDER && !entry.m_isOrder) continue;   // 질서만
                if (kind == KIND_CHAOS && entry.m_isOrder) continue;    // 혼돈만
                if (!keyword.empty() && entry.m_row.m_name.find(keyword) == std::wstring::npos) continue;   // 검색어 포함
                shown.push_back(&entry);
            }
        }

        // 정렬: 가격 · 변동률이 없는 젬은 항상 맨 아래
        if (sort == SORT_PRICE_HIGH || sort == SORT_PRICE_LOW)
        {
            const bool isHigh{ sort == SORT_PRICE_HIGH };
            std::ranges::stable_sort(shown, [isHigh](const Entry* left, const Entry* right)
            {
                const double leftPrice{ left->m_row.m_currentMinPrice };
                const double rightPrice{ right->m_row.m_currentMinPrice };
                if ((leftPrice > 0.0) != (rightPrice > 0.0)) return leftPrice > 0.0;   // 시세 있는 쪽이 먼저
                return isHigh ? leftPrice > rightPrice : leftPrice < rightPrice;
            });
        }
        else if (sort == SORT_CHANGE)
        {
            std::ranges::stable_sort(shown, [](const Entry* left, const Entry* right)
            {
                const bool hasLeft{ left->m_row.m_dayChangeRate.has_value() };
                const bool hasRight{ right->m_row.m_dayChangeRate.has_value() };
                if (hasLeft != hasRight) return hasLeft;   // 변동률 있는 쪽이 먼저
                return hasLeft && *left->m_row.m_dayChangeRate > *right->m_row.m_dayChangeRate;   // 많이 오른 순
            });
        }

        m_shownEntries = std::move(shown);
        FillGrid(m_shownEntries);
        ReselectRow();         // 고른 젬이 아직 보이면 그 행 선택 유지
        m_hasLayout = false;   // 행 수가 바뀌면 시세 카드 높이 · 안내 자리를 다시 계산
    }

    // 검색어를 한 번에 지우고 입력칸으로 포커스
    void ArkGemFinderView::ClearSearch()
    {
        m_searchBox.SetText(std::wstring{});
        ApplyFilter();
        m_host.SetFocus(&m_searchBox);
    }

    // 표 다시 채우기 (이름 = 등급색, 분류 = 칩, 현재 최저가 = 등락색 + ▲▼, 변동 = 전일 평균가 대비 변동률, 추이 = 일별 최저가 선)
    void ArkGemFinderView::FillGrid(const std::vector<const Entry*>& entries)
    {
        const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

        m_marketGrid.SetRowCount(entries.size());
        for (std::size_t row{}; row < entries.size(); ++row)
        {
            const Entry& entry{ *entries[row] };
            const ArkGemRow& gem{ entry.m_row };

            m_marketGrid.SetCellText(row, COL_NAME, gem.m_name);
            m_marketGrid.SetCellStyle(row, COL_NAME, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GetGradeColor(gem.m_gradeName), Core::FontWeight::BOLD });

            // 분류 칩
            m_marketGrid.SetCellText(row, COL_KIND, std::wstring{ entry.m_isOrder ? ORDER_KIND_TEXT : CHAOS_KIND_TEXT });
            m_marketGrid.SetCellStyle(row, COL_KIND, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR, Core::FontWeight::NORMAL,
                UI::GridDecoration::DECORATION_CHIP, std::wstring{}, Theme::ACCENT_COLOR });

            // 현재 최저가 + 직전 갱신 대비 등락 (0으로 반올림되면 표시하지 않음)
            std::wstring currentText{ FormatPrice(gem.m_currentMinPrice) };
            const std::int64_t diff{ gem.m_diff.has_value() ? std::llround(*gem.m_diff) : 0 };
            if (diff > 0) currentText += std::format(L" ({}{})", UP_MARK, FormatAmount(static_cast<double>(diff)));
            else if (diff < 0) currentText += std::format(L" ({}{})", DOWN_MARK, FormatAmount(static_cast<double>(-diff)));
            m_marketGrid.SetCellText(row, COL_CURRENT, std::move(currentText));
            m_marketGrid.SetCellStyle(row, COL_CURRENT, gem.m_currentMinPrice > 0.0
                ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(static_cast<double>(diff)), Core::FontWeight::BOLD } : mutedStyle);

            m_marketGrid.SetCellText(row, COL_YESTERDAY, FormatPrice(gem.m_yesterdayAvgPrice));
            m_marketGrid.SetCellStyle(row, COL_YESTERDAY, gem.m_yesterdayAvgPrice > 0.0 ? normalStyle : mutedStyle);
            m_marketGrid.SetCellText(row, COL_RECENT, FormatPrice(gem.m_recentPrice));
            m_marketGrid.SetCellStyle(row, COL_RECENT, gem.m_recentPrice > 0.0 ? normalStyle : mutedStyle);

            // 변동률 (빨강 상승 / 파랑 하락)
            if (gem.m_dayChangeRate.has_value())
            {
                m_marketGrid.SetCellText(row, COL_CHANGE, FormatRate(*gem.m_dayChangeRate));
                m_marketGrid.SetCellStyle(row, COL_CHANGE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(*gem.m_dayChangeRate) });
            }
            else
            {
                m_marketGrid.SetCellText(row, COL_CHANGE, std::wstring{ EMPTY_TEXT });
                m_marketGrid.SetCellStyle(row, COL_CHANGE, mutedStyle);
            }

            // 추이: 기록이 2일 이상 쌓였을 때만 선을 그림
            m_marketGrid.SetCellText(row, COL_TREND, std::wstring{});
            m_marketGrid.SetCellStyle(row, COL_TREND, normalStyle);
            if (gem.m_trend.size() >= MIN_TREND_POINTS) m_marketGrid.SetCellSparkline(row, COL_TREND, gem.m_trend);
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
        m_rosterValue = value;   // 통계 카드에도 보여 줌
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
        DrawStatCards(renderer);

        // 필터 줄 · 시세 카드 (표 · 필터 부품은 아래 m_host.Render가 위에 그림)
        PageLayout::DrawCard(renderer, m_filterRect);
        const bool isLive{ m_autoRefreshBox.IsChecked() };
        static_cast<void>(PageLayout::DrawCardWithHeader(renderer, m_marketRect, MARKET_EYEBROW, MARKET_TITLE, m_statusText, isLive));

        // 결과가 없으면 표 머리글 아래에 해결 방법 안내
        if (m_shownEntries.empty())
        {
            const bool hasAnyGem{ !m_orderEntries.empty() || !m_chaosEntries.empty() };
            PageLayout::DrawEmptyState(renderer, m_emptyRect, hasAnyGem ? EMPTY_NO_MATCH_TITLE : EMPTY_NO_DATA_TITLE,
                hasAnyGem ? EMPTY_NO_MATCH_HINT : EMPTY_NO_DATA_HINT);
        }
        PageLayout::DrawTableFooter(renderer, m_footerRect, std::format(FOOTER_COUNT_FORMAT, m_shownEntries.size()), m_statusText);

        // 아래: 귀속 젬 계산기 카드 + 그래프 카드 둘
        static_cast<void>(PageLayout::DrawCardWithHeader(renderer, m_calculatorRect, CALCULATOR_EYEBROW, CALCULATOR_TITLE, CALCULATOR_RIGHT_TEXT));
        static_cast<void>(PageLayout::DrawCardWithHeader(renderer, m_priceChartRect, PRICE_CHART_EYEBROW, PRICE_CHART_CARD_TITLE));
        static_cast<void>(PageLayout::DrawCardWithHeader(renderer, m_chartRect, CHART_EYEBROW, CHART_CARD_TITLE));

        m_host.Render(renderer);
    }

    // 통계 카드 4장 (전체 시세 기준, 검색 · 분류와 상관없음)
    void ArkGemFinderView::DrawStatCards(Core::Renderer& renderer) const
    {
        std::size_t gemCount{};
        double rateSum{};
        std::size_t rateCount{};
        const Entry* topEntry{};   // 현재 최저가가 가장 높은 젬
        for (const std::vector<Entry>* group : { &m_orderEntries, &m_chaosEntries })
        {
            for (const Entry& entry : *group)
            {
                ++gemCount;
                if (entry.m_row.m_dayChangeRate.has_value())
                {
                    rateSum += *entry.m_row.m_dayChangeRate;
                    ++rateCount;
                }
                if (topEntry == nullptr || entry.m_row.m_currentMinPrice > topEntry->m_row.m_currentMinPrice) topEntry = &entry;
            }
        }

        const double averageRate{ rateCount > 0 ? rateSum / static_cast<double>(rateCount) : 0.0 };
        const bool hasTop{ topEntry != nullptr && topEntry->m_row.m_currentMinPrice > 0.0 };

        const std::array<PageLayout::StatCard, STAT_CARD_COUNT> cards{
            PageLayout::StatCard{ std::wstring{ ICON_COUNT }, Theme::ACCENT_COLOR, std::wstring{ STAT_COUNT_LABEL },
                std::format(STAT_COUNT_VALUE_FORMAT, gemCount), std::format(STAT_COUNT_SUB_FORMAT, m_orderEntries.size(), m_chaosEntries.size()),
                Theme::MUTED_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_AVERAGE }, Theme::SPARKLINE_UP_COLOR, std::wstring{ STAT_AVERAGE_LABEL },
                rateCount > 0 ? FormatRate(averageRate) : std::wstring{ EMPTY_TEXT }, std::wstring{ STAT_AVERAGE_SUB }, Theme::MUTED_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_TOP }, Theme::HIGHLIGHT_TEXT_COLOR, std::wstring{ STAT_TOP_LABEL },
                hasTop ? FormatAmount(topEntry->m_row.m_currentMinPrice) : std::wstring{ EMPTY_TEXT },
                hasTop ? topEntry->m_row.m_name : std::wstring{}, hasTop ? Theme::GetGradeColor(topEntry->m_row.m_gradeName) : Theme::MUTED_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_ROSTER }, Theme::UP_TEXT_COLOR, std::wstring{ STAT_ROSTER_LABEL },
                FormatAmount(m_rosterValue), std::wstring{ STAT_ROSTER_SUB }, Theme::MUTED_TEXT_COLOR }
        };
        for (std::size_t index{}; index < cards.size() && index < m_statRects.size(); ++index)
        {
            PageLayout::DrawStatCard(renderer, m_statRects[index], cards[index]);
        }
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

    // 위에서부터: 통계 카드 4장 → 검색 · 분류 · 정렬 줄 → LIVE MARKET 카드 → 아래 (귀속 젬 계산기 | 그래프 둘)
    void ArkGemFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        Core::RectF remaining{ contentArea };
        m_statRects = PageLayout::SplitColumns(PageLayout::TakeTop(remaining, PageLayout::STAT_CARD_HEIGHT), STAT_CARD_COUNT, PageLayout::CARD_GAP);
        m_filterRect = PageLayout::TakeTop(remaining, PageLayout::FILTER_BAR_HEIGHT);

        // 시세 카드: 보여 줄 행이 딱 들어가는 높이 (아래 카드 최소 높이를 남기고, 모자라면 표가 스크롤)
        const float rowsHeight{ PageLayout::GRID_ROW_HEIGHT * static_cast<float>(std::max(m_shownEntries.size(), MIN_MARKET_ROWS)) };
        const float idealHeight{ PageLayout::CARD_HEADER_HEIGHT + Theme::GRID_HEADER_HEIGHT + rowsHeight + PageLayout::TABLE_FOOTER_HEIGHT };
        const float availableHeight{ remaining.m_bottom - remaining.m_top - PageLayout::CARD_GAP - MIN_BOTTOM_HEIGHT };
        m_marketRect = PageLayout::TakeTop(remaining, std::max(MIN_MARKET_HEIGHT, std::min(idealHeight, availableHeight)));

        // 필터 줄: [검색][✕][분류][정렬] ........ [자동 갱신][시세 갱신]
        const float controlTop{ m_filterRect.m_top + (PageLayout::FILTER_BAR_HEIGHT - PageLayout::CONTROL_HEIGHT) * HALF };
        const float controlBottom{ controlTop + PageLayout::CONTROL_HEIGHT };
        const float left{ m_filterRect.m_left + PageLayout::CARD_PADDING };
        const float right{ m_filterRect.m_right - PageLayout::CARD_PADDING };

        const float refreshLeft{ right - REFRESH_BUTTON_WIDTH };
        const float autoLeft{ refreshLeft - GROUP_GAP - AUTO_REFRESH_WIDTH };
        m_refreshButton.SetBounds(Core::RectF{ refreshLeft, controlTop, right, controlBottom });
        m_autoRefreshBox.SetBounds(Core::RectF{ autoLeft, controlTop, autoLeft + AUTO_REFRESH_WIDTH, controlBottom });

        const float fixedWidth{ CLEAR_BUTTON_WIDTH + KIND_BOX_WIDTH + SORT_BOX_WIDTH + CONTROL_GAP * FILTER_GAP_COUNT };
        const float searchWidth{ std::clamp(autoLeft - GROUP_GAP - left - fixedWidth, SEARCH_MIN_WIDTH, SEARCH_MAX_WIDTH) };
        float x{ left };
        m_searchBox.SetBounds(Core::RectF{ x, controlTop, x + searchWidth, controlBottom });
        x += searchWidth + CONTROL_GAP;
        m_clearButton.SetBounds(Core::RectF{ x, controlTop, x + CLEAR_BUTTON_WIDTH, controlBottom });
        x += CLEAR_BUTTON_WIDTH + CONTROL_GAP;
        m_kindBox.SetBounds(Core::RectF{ x, controlTop, x + KIND_BOX_WIDTH, controlBottom });
        x += KIND_BOX_WIDTH + CONTROL_GAP;
        m_sortBox.SetBounds(Core::RectF{ x, controlTop, x + SORT_BOX_WIDTH, controlBottom });

        // 시세 카드 안: 머리말 아래 표, 맨 아래 줄 = 총 개수
        const float bodyTop{ m_marketRect.m_top + PageLayout::CARD_HEADER_HEIGHT };
        const float footerTop{ std::max(bodyTop, m_marketRect.m_bottom - PageLayout::TABLE_FOOTER_HEIGHT) };
        m_footerRect = Core::RectF{ m_marketRect.m_left, footerTop, m_marketRect.m_right, m_marketRect.m_bottom };

        // 결과가 없으면 표는 머리글만 (그 아래에 안내를 그림)
        const float gridBottom{ m_shownEntries.empty() ? std::min(footerTop, bodyTop + Theme::GRID_HEADER_HEIGHT) : footerTop };
        m_marketGrid.SetBounds(Core::RectF{ m_marketRect.m_left + TABLE_INSET, bodyTop, m_marketRect.m_right - TABLE_INSET, gridBottom });
        m_emptyRect = Core::RectF{ m_marketRect.m_left, gridBottom, m_marketRect.m_right, footerTop };

        // 아래: 왼쪽 계산기 카드 | 오른쪽 위 = 고른 젬의 일별 최저가 그래프, 아래 = 원정대 귀속 젬 가치 그래프
        const float calculatorRight{ remaining.m_left + (remaining.m_right - remaining.m_left - PageLayout::CARD_GAP) * CALCULATOR_WIDTH_RATIO };
        m_calculatorRect = Core::RectF{ remaining.m_left, remaining.m_top, calculatorRight, remaining.m_bottom };
        const float chartLeft{ calculatorRight + PageLayout::CARD_GAP };
        const float chartSplit{ remaining.m_top + (remaining.m_bottom - remaining.m_top - PageLayout::CARD_GAP) * PRICE_CHART_HEIGHT_RATIO };
        m_priceChartRect = Core::RectF{ chartLeft, remaining.m_top, remaining.m_right, chartSplit };
        m_chartRect = Core::RectF{ chartLeft, chartSplit + PageLayout::CARD_GAP, remaining.m_right, remaining.m_bottom };

        // 그래프는 카드 머리말 아래 여백을 두고 채움
        const auto chartBounds{ [](const Core::RectF& card)
        {
            const float top{ card.m_top + PageLayout::CARD_HEADER_HEIGHT + PageLayout::CARD_PADDING * HALF };
            return Core::RectF{ card.m_left + PageLayout::CARD_PADDING, top, card.m_right - PageLayout::CARD_PADDING,
                std::max(top, card.m_bottom - PageLayout::CARD_PADDING) };
        } };
        m_priceChart.SetBounds(chartBounds(m_priceChartRect));
        m_chart.SetBounds(chartBounds(m_chartRect));

        // 계산기: 동기화 줄 [이름] [원정대 동기화] [캐릭터 선택] + 아래 표
        const float innerLeft{ m_calculatorRect.m_left + PageLayout::CARD_PADDING };
        const float innerRight{ m_calculatorRect.m_right - PageLayout::CARD_PADDING };
        const float rosterTop{ m_calculatorRect.m_top + PageLayout::CARD_HEADER_HEIGHT + PageLayout::CARD_PADDING * HALF };
        const float rosterBottom{ rosterTop + PageLayout::CONTROL_HEIGHT };
        float rosterX{ innerLeft };
        m_nameBox.SetBounds(Core::RectF{ rosterX, rosterTop, rosterX + NAME_BOX_WIDTH, rosterBottom });
        rosterX += NAME_BOX_WIDTH + GROUP_GAP;
        m_syncButton.SetBounds(Core::RectF{ rosterX, rosterTop, rosterX + SYNC_BUTTON_WIDTH, rosterBottom });
        rosterX += SYNC_BUTTON_WIDTH + SECTION_GAP;
        const float characterRight{ std::min(rosterX + CHARACTER_BOX_WIDTH, innerRight) };
        const float characterBoxRight{ std::max(characterRight, rosterX + CHARACTER_BOX_MIN_WIDTH) };   // 캐릭터 선택 오른쪽
        m_characterBox.SetBounds(Core::RectF{ rosterX, rosterTop, characterBoxRight, rosterBottom });
        m_visionBox.SetBounds(Core::RectF{ characterBoxRight + SECTION_GAP, rosterTop, characterBoxRight + SECTION_GAP + VISION_BOX_WIDTH, rosterBottom });   // [화면 인식]

        // 표는 동기화 줄 아래 남은 높이
        const float gridTop{ rosterBottom + CALCULATOR_BAR_GAP };
        m_boundGemGrid.SetBounds(Core::RectF{ innerLeft, gridTop, innerRight, std::max(gridTop, m_calculatorRect.m_bottom - PageLayout::CARD_PADDING) });
    }
}
