module LostArkMultiTool.Views.AccessoryFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;

namespace LostArkMultiTool::Views
{
    namespace
    {
        // ===== 검색 조건 선택지 =====
        // 부위 (콤보 상자 항목 순서 = 카테고리 코드 순서)
        constexpr std::size_t CATEGORY_COUNT{ 3 };
        constexpr std::array<std::wstring_view, CATEGORY_COUNT> CATEGORY_ITEMS{ L"목걸이", L"귀걸이", L"반지" };
        constexpr std::array<int, CATEGORY_COUNT> CATEGORY_CODES{ Core::APPConstants::NECKLACE_CATEGORY_CODE,
            Core::APPConstants::EARRING_CATEGORY_CODE, Core::APPConstants::RING_CATEGORY_CODE };

        // 최소 품질 (0 = 전체)
        constexpr std::size_t QUALITY_COUNT{ 6 };
        constexpr std::array<int, QUALITY_COUNT> QUALITY_OPTIONS{ 0, 50, 70, 80, 90, 100 };
        constexpr int ANY_QUALITY{ 0 };
        constexpr std::wstring_view ANY_QUALITY_TEXT{ L"전체" };
        constexpr std::wstring_view QUALITY_ITEM_FORMAT{ L"{} 이상" };

        // 연마 효과 칸의 첫 항목 (선택 없음)
        constexpr std::wstring_view NO_OPTION_TEXT{ L"선택 안 함" };
        constexpr std::wstring_view ANY_VALUE_TEXT{ L"값 전체" };
        constexpr std::size_t NONE_ITEM_COUNT{ 1 };   // 목록 앞에 붙는 "선택 안 함" · "값 전체" 항목 수

        // ===== 결과 표 열 =====
        constexpr std::size_t COL_NAME{ 0 };
        constexpr std::size_t COL_QUALITY{ 1 };
        constexpr std::size_t COL_OPTIONS{ 2 };
        constexpr std::size_t COL_BUY_PRICE{ 3 };
        constexpr std::size_t COL_BID_PRICE{ 4 };
        constexpr std::size_t COL_TRADE_COUNT{ 5 };
        constexpr std::size_t COL_END_DATE{ 6 };
        constexpr std::size_t COLUMN_COUNT{ 7 };
        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"이름", L"품질", L"옵션", L"즉시 구매가", L"최소 입찰가", L"거래", L"마감" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ 1.3f, 0.45f, 3.4f, 0.85f, 0.85f, 0.45f, 0.75f };

        // ===== 표시 형식 =====
        constexpr int AMOUNT_DECIMALS{ 0 };
        constexpr int PERCENT_DECIMALS{ 2 };          // "0.60%"
        constexpr int HIGH_QUALITY{ 90 };             // 이 이상이면 하늘색
        constexpr int PERFECT_QUALITY{ 100 };         // 100이면 금색
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view OPTION_SEPARATOR{ L" / " };
        constexpr std::wstring_view OPTION_FORMAT{ L"{} +{}{}" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };
        constexpr std::wstring_view TRADE_COUNT_FORMAT{ L"{}회" };

        // 마감 "yyyy-MM-ddTHH:mm:ss…" → "MM/dd HH:mm"
        constexpr std::size_t END_DATE_MIN_LENGTH{ 16 };
        constexpr std::size_t END_MONTH_POSITION{ 5 };
        constexpr std::size_t END_DAY_POSITION{ 8 };
        constexpr std::size_t END_TIME_POSITION{ 11 };
        constexpr std::size_t DATE_PART_LENGTH{ 2 };
        constexpr std::size_t TIME_PART_LENGTH{ 5 };   // "HH:mm"
        constexpr std::wstring_view END_DATE_FORMAT{ L"{}/{} {}" };

        // ===== 글자 =====
        constexpr std::wstring_view CATEGORY_LABEL_TEXT{ L"부위" };
        constexpr std::wstring_view QUALITY_LABEL_TEXT{ L"품질" };
        constexpr std::wstring_view SEARCH_TEXT{ L"검색" };
        constexpr std::wstring_view OPTION_LABEL_TEXT{ L"연마 효과" };
        constexpr int PREVIOUS_PAGE_STEP{ -1 };
        constexpr int NEXT_PAGE_STEP{ 1 };
        constexpr std::wstring_view PREVIOUS_TEXT{ L"◀ 이전" };
        constexpr std::wstring_view NEXT_TEXT{ L"다음 ▶" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"조건을 고르고 [검색]을 눌러 주세요." };
        constexpr std::wstring_view PANEL_TITLE{ L"경매장 4티어 고대 악세 (딜러 유효옵션, 즉시 구매가 낮은 순)" };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 16.0f };
        constexpr float ROW_GAP{ 8.0f };               // 검색 조건 두 줄 사이
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float LABEL_GAP{ 4.0f };
        constexpr float SHORT_LABEL_WIDTH{ 30.0f };    // "부위" · "품질"
        constexpr float CATEGORY_BOX_WIDTH{ 90.0f };
        constexpr float QUALITY_BOX_WIDTH{ 100.0f };
        constexpr float SEARCH_BUTTON_WIDTH{ 90.0f };
        constexpr float OPTION_LABEL_WIDTH{ 60.0f };
        constexpr float OPTION_BOX_WIDTH{ 160.0f };
        constexpr float VALUE_BOX_WIDTH{ 100.0f };
        constexpr float PAGE_BUTTON_WIDTH{ 80.0f };
        constexpr float PAGE_LABEL_WIDTH{ 260.0f };
        constexpr float PANEL_PADDING{ 10.0f };
        constexpr float PANEL_TITLE_LEFT{ 12.0f };

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle PAGE_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };
        constexpr Core::TextStyle PANEL_TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };

        [[nodiscard]] std::wstring FormatAmount(std::int64_t value)
        {
            return Core::NumberFormat::FormatNumber(static_cast<double>(value), AMOUNT_DECIMALS);
        }

        // 고정 선택지를 콤보 상자 항목으로
        template <std::size_t Count>
        [[nodiscard]] std::vector<std::wstring> ToItems(const std::array<std::wstring_view, Count>& texts)
        {
            return std::vector<std::wstring>(texts.begin(), texts.end());
        }

        [[nodiscard]] std::vector<std::wstring> MakeQualityItems()
        {
            std::vector<std::wstring> items;
            items.reserve(QUALITY_COUNT);
            for (const int quality : QUALITY_OPTIONS)
            {
                items.push_back(quality == ANY_QUALITY ? std::wstring{ ANY_QUALITY_TEXT } : std::format(QUALITY_ITEM_FORMAT, quality));
            }
            return items;
        }

        // 앞에 "선택 안 함" 같은 항목을 붙인 목록
        [[nodiscard]] std::vector<std::wstring> WithNoneItem(std::wstring_view noneText, const std::vector<std::wstring>& items)
        {
            std::vector<std::wstring> result;
            result.reserve(items.size() + NONE_ITEM_COUNT);
            result.emplace_back(noneText);
            result.insert(result.end(), items.begin(), items.end());
            return result;
        }

        // 콤보 상자 번호 → 목록 번호 (첫 항목 "선택 안 함"이면 nullopt)
        [[nodiscard]] std::optional<std::size_t> ToListIndex(const std::optional<std::size_t>& comboIndex) noexcept
        {
            if (!comboIndex.has_value() || *comboIndex < NONE_ITEM_COUNT) return std::nullopt;
            return *comboIndex - NONE_ITEM_COUNT;
        }

        [[nodiscard]] std::vector<UI::GridColumn> MakeColumns()
        {
            std::vector<UI::GridColumn> columns;
            columns.reserve(COLUMN_COUNT);
            for (std::size_t column{}; column < COLUMN_COUNT; ++column)
            {
                const bool isText{ column == COL_NAME || column == COL_OPTIONS };
                columns.push_back(UI::GridColumn{ std::wstring{ COLUMN_HEADERS[column] }, COLUMN_WEIGHTS[column],
                    isText ? Core::TextAlign::LEADING : Core::TextAlign::TRAILING });
            }
            return columns;
        }

        // 옵션 한 줄씩 "추가 피해 +0.60%"로 이어 붙임 (이름이 없는 줄은 건너뜀)
        [[nodiscard]] std::wstring FormatOptions(const std::vector<Models::AccessoryOptionLine>& options)
        {
            std::wstring text;
            for (const Models::AccessoryOptionLine& option : options)
            {
                if (option.m_name.empty()) continue;
                if (!text.empty()) text += OPTION_SEPARATOR;

                const std::wstring value{ Core::NumberFormat::FormatNumber(option.m_value, option.m_isPercentage ? PERCENT_DECIMALS : AMOUNT_DECIMALS) };
                text += std::format(OPTION_FORMAT, Core::TextEncoding::UTF8ToWide(option.m_name), value,
                    option.m_isPercentage ? PERCENT_SUFFIX : std::wstring_view{});
            }
            return text.empty() ? std::wstring{ EMPTY_TEXT } : text;
        }

        // 마감 시각 (형식이 다르면 그대로)
        [[nodiscard]] std::wstring FormatEndDate(const std::string& endDate)
        {
            const std::wstring wide{ Core::TextEncoding::UTF8ToWide(endDate) };
            if (wide.size() < END_DATE_MIN_LENGTH) return wide.empty() ? std::wstring{ EMPTY_TEXT } : wide;

            return std::format(END_DATE_FORMAT, wide.substr(END_MONTH_POSITION, DATE_PART_LENGTH),
                wide.substr(END_DAY_POSITION, DATE_PART_LENGTH), wide.substr(END_TIME_POSITION, TIME_PART_LENGTH));
        }

        // 품질 100 금색 / 90 이상 하늘색 / 나머지 기본색
        [[nodiscard]] const Core::Color& GetQualityColor(int quality) noexcept
        {
            if (quality >= PERFECT_QUALITY) return Theme::HIGHLIGHT_TEXT_COLOR;
            if (quality >= HIGH_QUALITY) return Theme::TOTAL_TEXT_COLOR;
            return Theme::TEXT_COLOR;
        }
    }

    AccessoryFinderView::AccessoryFinderView(Core::Platform& platform)
        : m_categoryLabel{ m_host.Emplace<UI::Label>(std::wstring{ CATEGORY_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_categoryBox{ m_host.Emplace<UI::ComboBox>() }
        , m_qualityLabel{ m_host.Emplace<UI::Label>(std::wstring{ QUALITY_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_qualityBox{ m_host.Emplace<UI::ComboBox>() }
        , m_searchButton{ m_host.Emplace<UI::Button>(std::wstring{ SEARCH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_optionLabel{ m_host.Emplace<UI::Label>(std::wstring{ OPTION_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_optionSlots{ MakeOptionSlots() }
        , m_listingGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(), 0) }
        , m_previousButton{ m_host.Emplace<UI::Button>(std::wstring{ PREVIOUS_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_pageLabel{ m_host.Emplace<UI::Label>(std::wstring{}, PAGE_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_nextButton{ m_host.Emplace<UI::Button>(std::wstring{ NEXT_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
    {
        // 고정 선택지 (첫 항목이 기본: 목걸이 · 품질 전체, 등급은 고대 고정)
        m_categoryBox.SetItems(ToItems(CATEGORY_ITEMS));
        m_categoryBox.SetSelectedIndex(0);
        m_categoryBox.SetOnSelectionChanged([this]
        {
            if (const auto onCategoryChanged{ m_onCategoryChanged }) onCategoryChanged();
        });
        m_qualityBox.SetItems(MakeQualityItems());
        m_qualityBox.SetSelectedIndex(0);
        SetOptionNames({});   // 옵션을 불러오기 전에는 "선택 안 함"만

        m_searchButton.SetOnClick([this]
        {
            if (const auto onSearch{ m_onSearch }) onSearch();
        });
        m_previousButton.SetOnClick([this]
        {
            if (const auto onPageMove{ m_onPageMove }) onPageMove(PREVIOUS_PAGE_STEP);
        });
        m_nextButton.SetOnClick([this]
        {
            if (const auto onPageMove{ m_onPageMove }) onPageMove(NEXT_PAGE_STEP);
        });

        // 결과 표: 읽기 전용, 행 단위 선택
        m_listingGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        UpdatePageButtons();
    }

    AccessoryFinderView::~AccessoryFinderView() = default;

    std::array<AccessoryFinderView::OptionSlot, Core::APPConstants::MAX_ACCESSORY_OPTION_FILTERS> AccessoryFinderView::MakeOptionSlots()
    {
        std::array<OptionSlot, Core::APPConstants::MAX_ACCESSORY_OPTION_FILTERS> slots;
        for (std::size_t slotIndex{}; slotIndex < slots.size(); ++slotIndex)
        {
            OptionSlot& slot{ slots[slotIndex] };
            slot.m_optionBox = &m_host.Emplace<UI::ComboBox>();
            slot.m_valueBox = &m_host.Emplace<UI::ComboBox>();

            // 옵션을 바꾸면 매니저가 그 옵션의 값 목록으로 값 칸을 다시 채움
            slot.m_optionBox->SetOnSelectionChanged([this, slotIndex]
            {
                if (const auto onOptionChanged{ m_onOptionChanged }) onOptionChanged(slotIndex);
            });
        }
        return slots;
    }

    void AccessoryFinderView::SetOnSearch(std::function<void()> onSearch)
    {
        m_onSearch = std::move(onSearch);
    }

    void AccessoryFinderView::SetOnPageMove(std::function<void(int pageStep)> onPageMove)
    {
        m_onPageMove = std::move(onPageMove);
    }

    void AccessoryFinderView::SetOnOptionChanged(std::function<void(std::size_t slotIndex)> onOptionChanged)
    {
        m_onOptionChanged = std::move(onOptionChanged);
    }

    void AccessoryFinderView::SetOnCategoryChanged(std::function<void()> onCategoryChanged)
    {
        m_onCategoryChanged = std::move(onCategoryChanged);
    }

    int AccessoryFinderView::GetCategoryCode() const noexcept
    {
        const std::size_t index{ m_categoryBox.GetSelectedIndex().value_or(0) };
        return CATEGORY_CODES[index < CATEGORY_COUNT ? index : 0];
    }

    std::optional<int> AccessoryFinderView::GetMinQuality() const noexcept
    {
        const std::size_t index{ m_qualityBox.GetSelectedIndex().value_or(0) };
        const int quality{ QUALITY_OPTIONS[index < QUALITY_COUNT ? index : 0] };
        if (quality == ANY_QUALITY) return std::nullopt;
        return quality;
    }

    void AccessoryFinderView::SetOptionNames(const std::vector<std::wstring>& optionNames)
    {
        const std::vector<std::wstring> items{ WithNoneItem(NO_OPTION_TEXT, optionNames) };
        for (std::size_t slotIndex{}; slotIndex < m_optionSlots.size(); ++slotIndex)
        {
            m_optionSlots[slotIndex].m_optionBox->SetItems(items);
            m_optionSlots[slotIndex].m_optionBox->SetSelectedIndex(0);
            SetOptionValues(slotIndex, {});
        }
    }

    void AccessoryFinderView::SetOptionValues(std::size_t slotIndex, const std::vector<std::wstring>& valueTexts,
        const std::vector<std::optional<Core::Color>>& valueColors)
    {
        if (slotIndex >= m_optionSlots.size()) return;

        UI::ComboBox& valueBox{ *m_optionSlots[slotIndex].m_valueBox };
        valueBox.SetItems(WithNoneItem(ANY_VALUE_TEXT, valueTexts));

        // 첫 항목("값 전체")은 기본색 → 나머지는 받은 색 (상 노랑 · 중 보라 · 하 파랑)
        std::vector<std::optional<Core::Color>> itemColors;
        itemColors.reserve(valueColors.size() + 1);
        itemColors.emplace_back(std::nullopt);
        itemColors.insert(itemColors.end(), valueColors.begin(), valueColors.end());
        valueBox.SetItemColors(std::move(itemColors));
        valueBox.SetSelectedIndex(0);
    }

    std::optional<std::size_t> AccessoryFinderView::GetSelectedOption(std::size_t slotIndex) const noexcept
    {
        if (slotIndex >= m_optionSlots.size()) return std::nullopt;
        return ToListIndex(m_optionSlots[slotIndex].m_optionBox->GetSelectedIndex());
    }

    std::optional<std::size_t> AccessoryFinderView::GetSelectedValue(std::size_t slotIndex) const noexcept
    {
        if (slotIndex >= m_optionSlots.size()) return std::nullopt;
        return ToListIndex(m_optionSlots[slotIndex].m_valueBox->GetSelectedIndex());
    }

    void AccessoryFinderView::SetBusy(bool isBusy) noexcept
    {
        m_isBusy = isBusy;
        m_searchButton.SetEnabled(!isBusy);
        UpdatePageButtons();
    }

    void AccessoryFinderView::SetStatusText(std::wstring text)
    {
        m_statusLabel.SetText(std::move(text));
    }

    void AccessoryFinderView::SetPageInfo(std::wstring text, bool hasPrevious, bool hasNext)
    {
        m_pageLabel.SetText(std::move(text));
        m_hasPrevious = hasPrevious;
        m_hasNext = hasNext;
        UpdatePageButtons();
    }

    // 결과 표 다시 채우기 (행 수가 바뀌면 표가 맞춰 줄고 늘어남)
    void AccessoryFinderView::ShowListings(const std::vector<Models::AccessoryListing>& listings)
    {
        const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

        m_listingGrid.SetRowCount(listings.size());
        for (std::size_t row{}; row < listings.size(); ++row)
        {
            const Models::AccessoryListing& listing{ listings[row] };

            m_listingGrid.SetCellText(row, COL_NAME, Core::TextEncoding::UTF8ToWide(listing.m_name));
            m_listingGrid.SetCellStyle(row, COL_NAME, normalStyle);

            m_listingGrid.SetCellText(row, COL_QUALITY, std::to_wstring(listing.m_quality));
            m_listingGrid.SetCellStyle(row, COL_QUALITY, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetQualityColor(listing.m_quality) });

            m_listingGrid.SetCellText(row, COL_OPTIONS, FormatOptions(listing.m_options));
            m_listingGrid.SetCellStyle(row, COL_OPTIONS, normalStyle);

            // 즉시 구매가는 굵게 (입찰 전용이면 "-")
            m_listingGrid.SetCellText(row, COL_BUY_PRICE, listing.m_buyPrice.has_value() ? FormatAmount(*listing.m_buyPrice) : std::wstring{ EMPTY_TEXT });
            m_listingGrid.SetCellStyle(row, COL_BUY_PRICE, listing.m_buyPrice.has_value()
                ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::HIGHLIGHT_TEXT_COLOR, Core::FontWeight::BOLD } : mutedStyle);

            m_listingGrid.SetCellText(row, COL_BID_PRICE, FormatAmount(listing.m_bidStartPrice));
            m_listingGrid.SetCellStyle(row, COL_BID_PRICE, normalStyle);

            m_listingGrid.SetCellText(row, COL_TRADE_COUNT, listing.m_tradeAllowCount.has_value()
                ? std::format(TRADE_COUNT_FORMAT, *listing.m_tradeAllowCount) : std::wstring{ EMPTY_TEXT });
            m_listingGrid.SetCellStyle(row, COL_TRADE_COUNT, normalStyle);

            m_listingGrid.SetCellText(row, COL_END_DATE, FormatEndDate(listing.m_endDate));
            m_listingGrid.SetCellStyle(row, COL_END_DATE, mutedStyle);
        }
    }

    void AccessoryFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void AccessoryFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderPanel(renderer);
        m_host.Render(renderer);
    }

    bool AccessoryFinderView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape AccessoryFinderView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void AccessoryFinderView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void AccessoryFinderView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 조건 두 줄 + 결과 묶음 상자 + 아래 페이지 이동 줄
    void AccessoryFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ contentArea.m_right - PAGE_PADDING };

        // 한 줄에 왼쪽부터 차례로 놓기
        float x{ left };
        float rowTop{ contentArea.m_top + PAGE_PADDING };
        const auto place{ [&x, &rowTop](UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, rowTop, x + width, rowTop + Theme::CONTROL_HEIGHT });
            x += width + gapAfter;
        } };

        // 첫 줄: 부위 [목걸이] 품질 [전체] [검색] 상태
        place(m_categoryLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_categoryBox, CATEGORY_BOX_WIDTH, GROUP_GAP);
        place(m_qualityLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_qualityBox, QUALITY_BOX_WIDTH, SECTION_GAP);
        place(m_searchButton, SEARCH_BUTTON_WIDTH, SECTION_GAP);
        m_statusLabel.SetBounds(Core::RectF{ x, rowTop, std::max(x, right), rowTop + Theme::CONTROL_HEIGHT });

        // 둘째 줄: 연마 효과 [옵션][값] × 3
        x = left;
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;
        place(m_optionLabel, OPTION_LABEL_WIDTH, LABEL_GAP);
        for (const OptionSlot& slot : m_optionSlots)
        {
            place(*slot.m_optionBox, OPTION_BOX_WIDTH, LABEL_GAP);
            place(*slot.m_valueBox, VALUE_BOX_WIDTH, GROUP_GAP);
        }

        // 맨 아래 줄: [◀ 이전] 1 / 12 페이지 [다음 ▶]
        const float pageTop{ std::max(rowTop + Theme::CONTROL_HEIGHT, contentArea.m_bottom - PAGE_PADDING - Theme::CONTROL_HEIGHT) };
        x = left;
        rowTop = pageTop;
        place(m_previousButton, PAGE_BUTTON_WIDTH, GROUP_GAP);
        place(m_pageLabel, PAGE_LABEL_WIDTH, GROUP_GAP);
        place(m_nextButton, PAGE_BUTTON_WIDTH, GROUP_GAP);

        // 가운데: 결과 묶음 상자 (테두리 · 제목 자리를 뺀 안쪽에 표)
        const float bodyTop{ contentArea.m_top + PAGE_PADDING + (Theme::CONTROL_HEIGHT + ROW_GAP) + Theme::CONTROL_HEIGHT + SECTION_GAP };
        const float bodyBottom{ std::max(bodyTop, pageTop - ROW_GAP) };
        m_panelBounds = Core::RectF{ left, bodyTop, right, bodyBottom };
        m_listingGrid.SetBounds(Core::RectF{ left + PANEL_PADDING, bodyTop + Theme::PANEL_TITLE_HEIGHT,
            right - PANEL_PADDING, std::max(bodyTop + Theme::PANEL_TITLE_HEIGHT, bodyBottom - PANEL_PADDING) });
    }

    // 묶음 상자: 둥근 테두리 + 왼쪽 위 제목
    void AccessoryFinderView::RenderPanel(Core::Renderer& renderer) const
    {
        renderer.DrawRoundedRect(m_panelBounds, Theme::PANEL_CORNER_RADIUS, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);

        const Core::RectF titleRect{ m_panelBounds.m_left + PANEL_TITLE_LEFT, m_panelBounds.m_top,
            m_panelBounds.m_right - PANEL_TITLE_LEFT, m_panelBounds.m_top + Theme::PANEL_TITLE_HEIGHT };
        renderer.DrawString(PANEL_TITLE, titleRect, Theme::TEXT_COLOR, PANEL_TITLE_STYLE);
    }

    // 조회 중이 아니고 넘길 페이지가 있을 때만 이전 · 다음 버튼 사용
    void AccessoryFinderView::UpdatePageButtons() noexcept
    {
        m_previousButton.SetEnabled(!m_isBusy && m_hasPrevious);
        m_nextButton.SetEnabled(!m_isBusy && m_hasNext);
    }
}
