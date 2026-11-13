module LostArkMultiTool.Views.RefineBookFinderView;

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
        constexpr std::size_t COLUMN_COUNT{ 6 };
        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"강화책", L"분류", L"현재 최저가", L"전일 평균가", L"최근 거래가", L"변동" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ 1.7f, 0.8f, 1.3f, 1.0f, 1.0f, 0.8f };

        constexpr int AMOUNT_DECIMALS{ 0 };
        constexpr int RATE_DECIMALS{ 1 };

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view REFRESHING_TEXT{ L"조회 중..." };
        constexpr std::wstring_view AUTO_REFRESH_FORMAT{ L"{}분마다 자동 갱신" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view SEARCH_PLACEHOLDER{ L"강화책 이름 검색" };
        constexpr std::wstring_view CLEAR_TEXT{ L"✕" };
        constexpr std::wstring_view ALL_KIND_TEXT{ L"전체 분류" };
        constexpr std::wstring_view METALLURGY_KIND_TEXT{ L"야금술" };
        constexpr std::wstring_view TAILORING_KIND_TEXT{ L"재봉술" };
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };
        constexpr std::wstring_view PLUS_SIGN{ L"+" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };
        constexpr std::wstring_view MARKET_EYEBROW{ L"LIVE MARKET" };
        constexpr std::wstring_view MARKET_TITLE{ L"야금술 · 재봉술 최저가" };
        constexpr std::wstring_view FOOTER_COUNT_FORMAT{ L"총 {}개의 검색 결과" };
        constexpr std::wstring_view EMPTY_NO_DATA_TITLE{ L"아직 불러온 시세가 없어요" };
        constexpr std::wstring_view EMPTY_NO_DATA_HINT{ L"[시세 갱신]을 눌러 거래소 시세를 불러와 보세요." };
        constexpr std::wstring_view EMPTY_NO_MATCH_TITLE{ L"조건에 맞는 강화책이 없어요" };
        constexpr std::wstring_view EMPTY_NO_MATCH_HINT{ L"검색어를 줄이거나 ✕로 지우고, 분류를 \"전체 분류\"로 바꿔 보세요." };

        // 분류 · 정렬 목록 (번호 = 목록 순서)
        constexpr std::size_t KIND_ALL{ 0 };
        constexpr std::size_t KIND_METALLURGY{ 1 };
        constexpr std::size_t KIND_TAILORING{ 2 };
        constexpr std::size_t SORT_DEFAULT{ 0 };
        constexpr std::size_t SORT_PRICE_HIGH{ 1 };
        constexpr std::size_t SORT_PRICE_LOW{ 2 };
        constexpr std::size_t SORT_CHANGE{ 3 };

        // ===== 통계 카드 =====
        constexpr std::size_t STAT_CARD_COUNT{ 4 };
        constexpr std::wstring_view STAT_COUNT_LABEL{ L"조회 종류" };
        constexpr std::wstring_view STAT_COUNT_VALUE_FORMAT{ L"{}종" };
        constexpr std::wstring_view STAT_COUNT_SUB_FORMAT{ L"야금술 {} · 재봉술 {}" };
        constexpr std::wstring_view STAT_AVERAGE_LABEL{ L"평균 전일 대비" };
        constexpr std::wstring_view STAT_AVERAGE_SUB{ L"전일 평균가 기준" };
        constexpr std::wstring_view STAT_TOP_LABEL{ L"최고가 강화책" };
        constexpr std::wstring_view STAT_RISE_LABEL{ L"전일 대비 상승" };
        constexpr std::wstring_view STAT_RISE_SUB_FORMAT{ L"하락 {}종" };
        constexpr std::wstring_view ICON_COUNT{ L"▤" };
        constexpr std::wstring_view ICON_AVERAGE{ L"↗" };
        constexpr std::wstring_view ICON_TOP{ L"◎" };
        constexpr std::wstring_view ICON_RISE{ L"▲" };

        // ===== 배치 (DIP) =====
        constexpr float CONTROL_GAP{ 8.0f };
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float CLEAR_BUTTON_WIDTH{ 32.0f };
        constexpr float KIND_BOX_WIDTH{ 130.0f };
        constexpr float SORT_BOX_WIDTH{ 150.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 150.0f };
        constexpr float REFRESH_BUTTON_WIDTH{ 110.0f };
        constexpr float SEARCH_MAX_WIDTH{ 360.0f };
        constexpr float SEARCH_MIN_WIDTH{ 120.0f };
        constexpr float FILTER_GAP_COUNT{ 3.0f };   // 검색 · ✕ · 분류 · 정렬 사이 간격 수
        constexpr float HALF{ 0.5f };
        constexpr float TABLE_INSET{ 1.0f };   // 카드 테두리 안쪽

        [[nodiscard]] std::wstring FormatAmount(double value)
        {
            return Core::NumberFormat::FormatNumber(value, AMOUNT_DECIMALS);
        }

        // 0 이하(거래 · 매물 없음)는 "-"
        [[nodiscard]] std::wstring FormatPrice(double value)
        {
            return value > 0.0 ? FormatAmount(value) : std::wstring{ EMPTY_TEXT };
        }

        // 변동률 글자 ("+3.2%" / "-1.5%")
        [[nodiscard]] std::wstring FormatRate(double rate)
        {
            const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
            return sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX };
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
                const bool isLeading{ column == COL_NAME || column == COL_KIND };   // 이름 · 분류는 왼쪽 정렬
                columns.push_back(UI::GridColumn{ std::wstring{ COLUMN_HEADERS[column] }, COLUMN_WEIGHTS[column],
                    isLeading ? Core::TextAlign::LEADING : Core::TextAlign::TRAILING });
            }
            return columns;
        }
    }

    RefineBookFinderView::RefineBookFinderView(Core::Platform& platform)
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
        , m_grid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_statusText{ INITIAL_STATUS_TEXT }
    {
        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 검색어가 바뀔 때마다 표에 바로 반영
        m_searchBox.SetOnTextChanged([this] { ApplyFilter(); });
        m_clearButton.SetOnClick([this] { ClearSearch(); });

        // 분류 · 정렬 목록 (처음은 전체 분류 · 기본 순서)
        m_kindBox.SetItems({ std::wstring{ ALL_KIND_TEXT }, std::wstring{ METALLURGY_KIND_TEXT }, std::wstring{ TAILORING_KIND_TEXT } });
        m_kindBox.SetSelectedIndex(KIND_ALL);
        m_kindBox.SetOnSelectionChanged([this] { ApplyFilter(); });
        m_sortBox.SetItems({ L"기본 정렬", L"가격 높은 순", L"가격 낮은 순", L"등락 순" });
        m_sortBox.SetSelectedIndex(SORT_DEFAULT);
        m_sortBox.SetOnSelectionChanged([this] { ApplyFilter(); });

        // 시세 표: 읽기 전용, 행 단위 선택
        m_grid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        m_grid.SetRowHeight(PageLayout::GRID_ROW_HEIGHT);
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

    // 조회 중에는 버튼을 끄고 글자를 "조회 중..."으로
    void RefineBookFinderView::SetRefreshEnabled(bool isEnabled)
    {
        m_refreshButton.SetEnabled(isEnabled);
        m_refreshButton.SetText(std::wstring{ isEnabled ? REFRESH_TEXT : REFRESHING_TEXT });
    }

    void RefineBookFinderView::SetStatusText(std::wstring text)
    {
        m_statusText = std::move(text);
    }

    void RefineBookFinderView::ShowMetallurgy(const std::vector<RefineBookRow>& rows)
    {
        m_metallurgyEntries.clear();
        for (const RefineBookRow& row : rows) m_metallurgyEntries.push_back(Entry{ true, row });
        ApplyFilter();
    }

    void RefineBookFinderView::ShowTailoring(const std::vector<RefineBookRow>& rows)
    {
        m_tailoringEntries.clear();
        for (const RefineBookRow& row : rows) m_tailoringEntries.push_back(Entry{ false, row });
        ApplyFilter();
    }

    // 검색어 · 분류에 맞는 책만 모아 정렬 → 표 채우기 (정렬을 고르지 않으면 서비스가 정한 순서 그대로)
    void RefineBookFinderView::ApplyFilter()
    {
        const std::wstring& keyword{ m_searchBox.GetText() };
        const std::size_t kind{ m_kindBox.GetSelectedIndex().value_or(KIND_ALL) };
        const std::size_t sort{ m_sortBox.GetSelectedIndex().value_or(SORT_DEFAULT) };

        std::vector<const Entry*> shown;
        for (const std::vector<Entry>* group : { &m_metallurgyEntries, &m_tailoringEntries })
        {
            for (const Entry& entry : *group)
            {
                if (kind == KIND_METALLURGY && !entry.m_isMetallurgy) continue;   // 야금술만
                if (kind == KIND_TAILORING && entry.m_isMetallurgy) continue;     // 재봉술만
                if (!keyword.empty() && entry.m_row.m_name.find(keyword) == std::wstring::npos) continue;   // 검색어 포함
                shown.push_back(&entry);
            }
        }

        // 정렬: 가격 · 변동률이 없는 책은 항상 맨 아래
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

        FillGrid(shown);
        m_shownCount = shown.size();
        m_hasLayout = false;   // 행 수가 바뀌면 표 높이 · 안내 자리를 다시 계산
    }

    // 표 다시 채우기 (이름 = 등급색, 분류 = 칩, 현재 최저가 = 등락색 + ▲▼, 변동 = 전일 평균가 대비 변동률)
    void RefineBookFinderView::FillGrid(const std::vector<const Entry*>& entries)
    {
        const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

        m_grid.SetRowCount(entries.size());
        for (std::size_t row{}; row < entries.size(); ++row)
        {
            const Entry& entry{ *entries[row] };
            const RefineBookRow& book{ entry.m_row };

            m_grid.SetCellText(row, COL_NAME, book.m_name);
            m_grid.SetCellStyle(row, COL_NAME, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GetGradeColor(book.m_gradeName), Core::FontWeight::BOLD });

            // 분류 칩
            m_grid.SetCellText(row, COL_KIND, std::wstring{ entry.m_isMetallurgy ? METALLURGY_KIND_TEXT : TAILORING_KIND_TEXT });
            m_grid.SetCellStyle(row, COL_KIND, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR, Core::FontWeight::NORMAL,
                UI::GridDecoration::DECORATION_CHIP, std::wstring{}, Theme::ACCENT_COLOR });

            // 현재 최저가 + 직전 갱신 대비 등락 (0으로 반올림되면 표시하지 않음)
            std::wstring currentText{ FormatPrice(book.m_currentMinPrice) };
            const std::int64_t diff{ book.m_diff.has_value() ? std::llround(*book.m_diff) : 0 };
            if (diff > 0) currentText += std::format(L" ({}{})", UP_MARK, FormatAmount(static_cast<double>(diff)));
            else if (diff < 0) currentText += std::format(L" ({}{})", DOWN_MARK, FormatAmount(static_cast<double>(-diff)));
            m_grid.SetCellText(row, COL_CURRENT, std::move(currentText));
            m_grid.SetCellStyle(row, COL_CURRENT, book.m_currentMinPrice > 0.0
                ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(static_cast<double>(diff)), Core::FontWeight::BOLD } : mutedStyle);

            m_grid.SetCellText(row, COL_YESTERDAY, FormatPrice(book.m_yesterdayAvgPrice));
            m_grid.SetCellStyle(row, COL_YESTERDAY, book.m_yesterdayAvgPrice > 0.0 ? normalStyle : mutedStyle);
            m_grid.SetCellText(row, COL_RECENT, FormatPrice(book.m_recentPrice));
            m_grid.SetCellStyle(row, COL_RECENT, book.m_recentPrice > 0.0 ? normalStyle : mutedStyle);

            // 변동률 ("+3.2%" 빨강 / "-1.5%" 파랑)
            if (!book.m_dayChangeRate.has_value())
            {
                m_grid.SetCellText(row, COL_CHANGE, std::wstring{ EMPTY_TEXT });
                m_grid.SetCellStyle(row, COL_CHANGE, mutedStyle);
                continue;
            }
            m_grid.SetCellText(row, COL_CHANGE, FormatRate(*book.m_dayChangeRate));
            m_grid.SetCellStyle(row, COL_CHANGE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(*book.m_dayChangeRate) });
        }
    }

    // 검색어를 한 번에 지우고 입력칸으로 포커스
    void RefineBookFinderView::ClearSearch()
    {
        m_searchBox.SetText(std::wstring{});
        ApplyFilter();
        m_host.SetFocus(&m_searchBox);
    }

    void RefineBookFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void RefineBookFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        DrawStatCards(renderer);

        // 필터 줄 · 시세 카드 (표 · 필터 부품은 아래 m_host.Render가 위에 그림)
        PageLayout::DrawCard(renderer, m_filterRect);
        const bool isLive{ m_autoRefreshBox.IsChecked() };
        static_cast<void>(PageLayout::DrawCardWithHeader(renderer, m_marketRect, MARKET_EYEBROW, MARKET_TITLE, m_statusText, isLive));

        // 결과가 없으면 표 머리글 아래에 해결 방법 안내
        if (m_shownCount == 0)
        {
            const bool hasAnyBook{ !m_metallurgyEntries.empty() || !m_tailoringEntries.empty() };
            PageLayout::DrawEmptyState(renderer, m_emptyRect, hasAnyBook ? EMPTY_NO_MATCH_TITLE : EMPTY_NO_DATA_TITLE,
                hasAnyBook ? EMPTY_NO_MATCH_HINT : EMPTY_NO_DATA_HINT);
        }
        PageLayout::DrawTableFooter(renderer, m_footerRect, std::format(FOOTER_COUNT_FORMAT, m_shownCount), m_statusText);

        m_host.Render(renderer);
    }

    // 통계 카드 4장 (전체 시세 기준, 검색 · 분류와 상관없음)
    void RefineBookFinderView::DrawStatCards(Core::Renderer& renderer) const
    {
        std::size_t bookCount{};
        std::size_t riseCount{};
        std::size_t fallCount{};
        double rateSum{};
        std::size_t rateCount{};
        const Entry* topEntry{};   // 현재 최저가가 가장 높은 책
        for (const std::vector<Entry>* group : { &m_metallurgyEntries, &m_tailoringEntries })
        {
            for (const Entry& entry : *group)
            {
                ++bookCount;
                if (entry.m_row.m_dayChangeRate.has_value())
                {
                    rateSum += *entry.m_row.m_dayChangeRate;
                    ++rateCount;
                    if (*entry.m_row.m_dayChangeRate > 0.0) ++riseCount;
                    else if (*entry.m_row.m_dayChangeRate < 0.0) ++fallCount;
                }
                if (topEntry == nullptr || entry.m_row.m_currentMinPrice > topEntry->m_row.m_currentMinPrice) topEntry = &entry;
            }
        }

        const double averageRate{ rateCount > 0 ? rateSum / static_cast<double>(rateCount) : 0.0 };
        const std::size_t metallurgyCount{ m_metallurgyEntries.size() };
        const bool hasTop{ topEntry != nullptr && topEntry->m_row.m_currentMinPrice > 0.0 };

        const std::array<PageLayout::StatCard, STAT_CARD_COUNT> cards{
            PageLayout::StatCard{ std::wstring{ ICON_COUNT }, Theme::ACCENT_COLOR, std::wstring{ STAT_COUNT_LABEL },
                std::format(STAT_COUNT_VALUE_FORMAT, bookCount), std::format(STAT_COUNT_SUB_FORMAT, metallurgyCount, bookCount - metallurgyCount),
                Theme::MUTED_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_AVERAGE }, Theme::SPARKLINE_UP_COLOR, std::wstring{ STAT_AVERAGE_LABEL },
                rateCount > 0 ? FormatRate(averageRate) : std::wstring{ EMPTY_TEXT }, std::wstring{ STAT_AVERAGE_SUB }, Theme::MUTED_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_TOP }, Theme::HIGHLIGHT_TEXT_COLOR, std::wstring{ STAT_TOP_LABEL },
                hasTop ? FormatAmount(topEntry->m_row.m_currentMinPrice) : std::wstring{ EMPTY_TEXT },
                hasTop ? topEntry->m_row.m_name : std::wstring{}, hasTop ? Theme::GetGradeColor(topEntry->m_row.m_gradeName) : Theme::MUTED_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_RISE }, Theme::UP_TEXT_COLOR, std::wstring{ STAT_RISE_LABEL },
                std::format(STAT_COUNT_VALUE_FORMAT, riseCount), std::format(STAT_RISE_SUB_FORMAT, fallCount), Theme::DOWN_TEXT_COLOR }
        };
        for (std::size_t index{}; index < cards.size() && index < m_statRects.size(); ++index)
        {
            PageLayout::DrawStatCard(renderer, m_statRects[index], cards[index]);
        }
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

    // 위에서부터: 통계 카드 4장 → 검색 · 분류 · 정렬 줄 → LIVE MARKET 카드 (머리말 + 표 + 아래 줄)
    void RefineBookFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        Core::RectF remaining{ contentArea };
        m_statRects = PageLayout::SplitColumns(PageLayout::TakeTop(remaining, PageLayout::STAT_CARD_HEIGHT), STAT_CARD_COUNT, PageLayout::CARD_GAP);
        m_filterRect = PageLayout::TakeTop(remaining, PageLayout::FILTER_BAR_HEIGHT);
        m_marketRect = remaining;

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

        // 시세 카드: 머리말 아래 표, 맨 아래 줄 = 총 개수
        const float bodyTop{ m_marketRect.m_top + PageLayout::CARD_HEADER_HEIGHT };
        const float footerTop{ std::max(bodyTop, m_marketRect.m_bottom - PageLayout::TABLE_FOOTER_HEIGHT) };
        m_footerRect = Core::RectF{ m_marketRect.m_left, footerTop, m_marketRect.m_right, m_marketRect.m_bottom };

        // 결과가 없으면 표는 머리글만 (그 아래에 안내를 그림)
        const float gridBottom{ m_shownCount == 0 ? std::min(footerTop, bodyTop + Theme::GRID_HEADER_HEIGHT) : footerTop };
        m_grid.SetBounds(Core::RectF{ m_marketRect.m_left + TABLE_INSET, bodyTop, m_marketRect.m_right - TABLE_INSET, gridBottom });
        m_emptyRect = Core::RectF{ m_marketRect.m_left, gridBottom, m_marketRect.m_right, footerTop };
    }
}
