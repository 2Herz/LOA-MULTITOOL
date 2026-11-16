module LostArkMultiTool.Views.AccessoryFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.NumberFormat;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Views.PageLayout;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

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
        constexpr std::size_t COL_NAME{ 0 };         // 아이템 (등급색 첫 글자 배지 + 이름)
        constexpr std::size_t COL_CATEGORY{ 1 };     // 분류 칩 ("목걸이 · 품질 98")
        constexpr std::size_t COL_OPTIONS{ 2 };
        constexpr std::size_t COL_BUY_PRICE{ 3 };
        constexpr std::size_t COL_BID_PRICE{ 4 };
        constexpr std::size_t COL_TRADE_COUNT{ 5 };
        constexpr std::size_t COL_END_DATE{ 6 };
        constexpr std::size_t COLUMN_COUNT{ 7 };
        constexpr std::array<std::wstring_view, COLUMN_COUNT> COLUMN_HEADERS{ L"아이템", L"분류", L"옵션", L"즉시 구매가", L"최소 입찰가", L"거래", L"마감" };
        constexpr std::array<float, COLUMN_COUNT> COLUMN_WEIGHTS{ 1.4f, 1.15f, 3.2f, 0.9f, 0.9f, 0.5f, 0.75f };
        constexpr std::wstring_view CHIP_FORMAT{ L"{} · 품질 {}" };   // 분류 칩 글자

        // ===== 표시 형식 =====
        constexpr int AMOUNT_DECIMALS{ 0 };
        constexpr int PERCENT_DECIMALS{ 2 };          // "0.60%"
        constexpr int HIGH_QUALITY{ 90 };             // 이 이상이면 하늘색
        constexpr int PERFECT_QUALITY{ 100 };         // 100이면 금색
        constexpr std::wstring_view EMPTY_TEXT{ L"-" };
        constexpr std::wstring_view OPTION_SEPARATOR{ L" / " };
        constexpr std::wstring_view OPTION_FORMAT{ L"{} +{}{}" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };
        constexpr std::string_view POLISH_OPTION_TYPE{ "ACCESSORY_UPGRADE" };   // 연마 효과 줄의 옵션 종류 (옵션 1 · 2 · 3)
        constexpr std::size_t MAIN_STAT_COUNT{ 3 };                             // 주스탯 종류 수
        constexpr std::array<std::string_view, MAIN_STAT_COUNT> MAIN_STAT_NAMES{ "힘", "민첩", "지능" };   // 매물에 같은 값으로 세 줄이 붙음 → 하나만 보여 줌
        constexpr std::string_view VITALITY_STAT_NAME{ "체력" };                // 주스탯 다음에 보여 줄 스탯
        constexpr std::string_view MAIN_STAT_DISPLAY_NAME{ "힘·민·지" };        // 주스탯 한 줄 이름 (UTF-8)
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
        constexpr std::wstring_view CATEGORY_LABEL_TEXT{ L"분류" };
        constexpr std::wstring_view QUALITY_LABEL_TEXT{ L"품질" };
        constexpr std::wstring_view SEARCH_TEXT{ L"검색" };
        constexpr std::wstring_view SEARCHING_BUTTON_TEXT{ L"검색 중…" };   // 조회 중 버튼 글자
        constexpr std::wstring_view OPTION_LABEL_TEXT{ L"연마 효과" };
        constexpr std::wstring_view MAIN_STAT_LABEL_TEXT{ L"주스탯" };            // 주스탯 최소 수치 칸 이름
        constexpr std::wstring_view MAIN_STAT_PLACEHOLDER{ L"최소 수치" };        // 비어 있을 때 안내 글자
        constexpr std::size_t MAIN_STAT_MAX_LENGTH{ 6 };                          // 주스탯 입력 최대 자릿수 (악세 주스탯은 5자리)
        constexpr int DECIMAL_BASE{ 10 };                                         // 십진수 자릿수 올림
        constexpr int PREVIOUS_PAGE_STEP{ -1 };
        constexpr int NEXT_PAGE_STEP{ 1 };
        constexpr std::wstring_view PREVIOUS_TEXT{ L"◀ 이전" };
        constexpr std::wstring_view NEXT_TEXT{ L"다음 ▶" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"조건을 고르고 [검색]을 눌러 주세요." };
        constexpr std::wstring_view PANEL_TITLE{ L"경매장 4티어 고대 악세 (딜러 유효옵션, 즉시 구매가 낮은 순)" };
        constexpr std::wstring_view CARD_EYEBROW{ L"LIVE MARKET" };       // 결과 카드 영문 소제목
        constexpr std::wstring_view CARD_TITLE{ L"악세서리 매물" };        // 결과 카드 제목
        constexpr std::wstring_view EMPTY_BEFORE_TITLE{ L"아직 검색하지 않았어요" };
        constexpr std::wstring_view EMPTY_BEFORE_HINT{ L"분류와 연마 효과 조건을 고르고 [검색]을 눌러 주세요." };
        constexpr std::wstring_view EMPTY_NONE_TITLE{ L"조건에 맞는 매물이 없어요" };
        constexpr std::wstring_view EMPTY_NONE_HINT{ L"최소 품질을 낮추거나 연마 효과 조건을 줄여서 다시 검색해 보세요." };

        // ===== 통계 카드 글자 =====
        constexpr std::wstring_view STAT_COUNT_LABEL{ L"검색 결과 수" };
        constexpr std::wstring_view STAT_COUNT_FORMAT{ L"{}개" };
        constexpr std::wstring_view STAT_COUNT_SUB{ L"조건 일치" };
        constexpr std::wstring_view STAT_LOWEST_LABEL{ L"최저가 (이 페이지)" };
        constexpr std::wstring_view STAT_AVERAGE_LABEL{ L"평균가 (이 페이지)" };
        constexpr std::wstring_view STAT_PRICE_SUB{ L"골드" };
        constexpr std::wstring_view STAT_TIME_LABEL{ L"마지막 검색 시각" };
        constexpr std::wstring_view ICON_COUNT{ L"▤" };
        constexpr std::wstring_view ICON_LOWEST{ L"↘" };
        constexpr std::wstring_view ICON_AVERAGE{ L"◎" };
        constexpr std::wstring_view ICON_TIME{ L"◷" };
        constexpr std::size_t BADGE_LENGTH{ 1 };   // 배지에 넣는 이름 앞 글자 수

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
        constexpr float MAIN_STAT_LABEL_WIDTH{ 42.0f };   // "주스탯"
        constexpr float MAIN_STAT_BOX_WIDTH{ 100.0f };    // 주스탯 최소 수치 입력칸
        constexpr float OPTION_LABEL_WIDTH{ 60.0f };
        constexpr float OPTION_BOX_WIDTH{ 160.0f };
        constexpr float VALUE_BOX_WIDTH{ 100.0f };
        constexpr float PAGE_BUTTON_WIDTH{ 80.0f };
        constexpr float PAGE_LABEL_WIDTH{ 260.0f };
        constexpr float FILTER_CARD_HEIGHT{ PageLayout::CARD_PADDING * 2.0f + Theme::CONTROL_HEIGHT * 2.0f + ROW_GAP };   // 검색 조건 카드 (두 줄)
        constexpr float FOOTER_HEIGHT{ 44.0f };         // 표 아래 줄 (페이지 이동 버튼이 들어감)
        constexpr float EMPTY_STATE_TOP_OFFSET{ 48.0f };   // 빈 상태 안내가 표 머리글 아래에서 시작하는 위치
        constexpr float HALF{ 0.5f };
        constexpr float PAGE_LABEL_RIGHT_GAP{ 8.0f };   // 페이지 글자와 이전 버튼 사이

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

        // 옵션 한 줄을 "추가 피해 +0.60%"로 이어 붙임 (앞에 글자가 있으면 구분자부터)
        void AppendOption(std::wstring& text, const Models::AccessoryOptionLine& option)
        {
            if (!text.empty()) text += OPTION_SEPARATOR;                                                                              // 앞 옵션과 구분
            const std::wstring value{ Core::NumberFormat::FormatNumber(option.m_value, option.m_isPercentage ? PERCENT_DECIMALS : AMOUNT_DECIMALS) };   // 값 글자
            text += std::format(OPTION_FORMAT, Core::TextEncoding::UTF8ToWide(option.m_name), value,
                option.m_isPercentage ? PERCENT_SUFFIX : std::wstring_view{});                                                         // "이름 +값(%)"
        }

        // 옵션 칸 글자: 연마 효과(옵션 1 · 2 · 3) → 주스탯(힘·민·지 한 줄) → 체력 순서만 보여 줌 (깨달음 등은 칸이 잘려서 뺌)
        [[nodiscard]] std::wstring FormatOptions(const std::vector<Models::AccessoryOptionLine>& options)
        {
            std::wstring text;                                                                          // 옵션 칸 글자
            for (const Models::AccessoryOptionLine& option : options)                                   // 연마 효과를 매물 순서대로
            {
                if (option.m_type == POLISH_OPTION_TYPE && !option.m_name.empty()) AppendOption(text, option);
            }
            // 주스탯: 힘 · 민첩 · 지능 중 처음 찾은 줄 하나만 "힘·민·지 +값"으로
            const auto mainStat{ std::ranges::find_if(options, [](const Models::AccessoryOptionLine& option)
            {
                return std::ranges::contains(MAIN_STAT_NAMES, std::string_view{ option.m_name });   // 주스탯 이름인지
            }) };
            if (mainStat != options.end())
            {
                Models::AccessoryOptionLine merged{ *mainStat };            // 값은 그대로
                merged.m_name = std::string{ MAIN_STAT_DISPLAY_NAME };      // 이름만 합친 표시로
                AppendOption(text, merged);
            }

            // 체력
            const auto vitality{ std::ranges::find(options, VITALITY_STAT_NAME, &Models::AccessoryOptionLine::m_name) };   // 체력 줄
            if (vitality != options.end()) AppendOption(text, *vitality);
            return text.empty() ? std::wstring{ EMPTY_TEXT } : text;                                    // 보여 줄 옵션이 없으면 "-"
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
        , m_mainStatLabel{ m_host.Emplace<UI::Label>(std::wstring{ MAIN_STAT_LABEL_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_mainStatBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ MAIN_STAT_PLACEHOLDER }) }
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

        // 주스탯 칸: 숫자만 (영문 · 숫자 입력), Enter로 바로 검색
        m_mainStatBox.SetMaxLength(MAIN_STAT_MAX_LENGTH);
        m_mainStatBox.SetIMEAllowed(false);
        m_mainStatBox.SetInputFilter([](wchar_t character) { return character >= L'0' && character <= L'9'; });   // 숫자만 받음
        m_mainStatBox.SetOnSubmit([this]
        {
            if (!m_isBusy) if (const auto onSearch{ m_onSearch }) onSearch();   // 조회 중이 아니면 검색
        });

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
        m_listingGrid.SetRowHeight(PageLayout::GRID_ROW_HEIGHT);   // 새 디자인 행 높이 (배지 · 칩이 들어감)
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

    std::optional<int> AccessoryFinderView::GetMinMainStat() const
    {
        const std::wstring& text{ m_mainStatBox.GetText() };                                 // 입력한 글자 (숫자만 받음)
        int value{};                                                                         // 읽은 수치
        const wchar_t* const first{ text.data() };                                           // 글자 시작
        const wchar_t* const last{ first + text.size() };                                    // 글자 끝
        for (const wchar_t* cursor{ first }; cursor != last; ++cursor)                       // 자릿수가 작아서(최대 6) 넘침 없음
        {
            if (*cursor < L'0' || *cursor > L'9') return std::nullopt;                       // 숫자가 아니면 조건 없음
            value = value * DECIMAL_BASE + static_cast<int>(*cursor - L'0');                // 다음 자리
        }
        if (value <= 0) return std::nullopt;                                                 // 비었거나 0이면 조건 없음
        return value;
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
        m_searchButton.SetText(std::wstring{ isBusy ? SEARCHING_BUTTON_TEXT : SEARCH_TEXT });   // 조회 중에는 "검색 중…"
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

    // 결과 표 다시 채우기 (행 수가 바뀌면 표가 맞춰 줄고 늘어남) + 통계 카드 값 계산
    void AccessoryFinderView::ShowListings(const std::vector<Models::AccessoryListing>& listings, int categoryCode, int totalCount, std::wstring searchedAtText)
    {
        const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };   // 기본 칸
        const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };   // 흐린 칸

        // 검색한 부위 이름 (알 수 없으면 빈 글자)
        std::wstring_view categoryName{};
        for (std::size_t index{}; index < CATEGORY_COUNT; ++index)
        {
            if (CATEGORY_CODES[index] == categoryCode) categoryName = CATEGORY_ITEMS[index];
        }

        m_hasSearched = true;                                  // 결과를 받음
        m_totalCount = totalCount;                             // 전체 매물 수
        m_searchedAtText = std::move(searchedAtText);          // 검색 시각
        std::int64_t priceSum{};                               // 즉시 구매가 합
        std::int64_t priceCount{};                             // 즉시 구매가가 있는 매물 수
        m_lowestPrice.reset();                                 // 최저가 초기화
        m_averagePrice.reset();                                // 평균가 초기화

        m_listingGrid.SetRowCount(listings.size());
        for (std::size_t row{}; row < listings.size(); ++row)
        {
            const Models::AccessoryListing& listing{ listings[row] };
            const std::wstring name{ Core::TextEncoding::UTF8ToWide(listing.m_name) };   // 아이템 이름
            const Core::Color& gradeColor{ Theme::GetGradeColor(listing.m_gradeName) };   // 등급색

            // 아이템: 등급색 첫 글자 배지 + 이름
            m_listingGrid.SetCellText(row, COL_NAME, name);
            m_listingGrid.SetCellStyle(row, COL_NAME, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR, Core::FontWeight::BOLD,
                UI::GridDecoration::DECORATION_BADGE, name.substr(0, BADGE_LENGTH), gradeColor });

            // 분류 칩: "목걸이 · 품질 98" (품질 100 금색 / 90 이상 하늘색)
            m_listingGrid.SetCellText(row, COL_CATEGORY, std::format(CHIP_FORMAT, categoryName, listing.m_quality));
            m_listingGrid.SetCellStyle(row, COL_CATEGORY, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetQualityColor(listing.m_quality), Core::FontWeight::NORMAL,
                UI::GridDecoration::DECORATION_CHIP, {}, GetQualityColor(listing.m_quality) });

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

            // 통계: 즉시 구매가가 있는 매물만
            if (listing.m_buyPrice.has_value())
            {
                priceSum += *listing.m_buyPrice;
                ++priceCount;
                m_lowestPrice = m_lowestPrice.has_value() ? std::min(*m_lowestPrice, *listing.m_buyPrice) : *listing.m_buyPrice;
            }
        }
        if (priceCount > 0) m_averagePrice = priceSum / priceCount;   // 평균 (정수 골드)
    }

    void AccessoryFinderView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void AccessoryFinderView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderCards(renderer);
        m_host.Render(renderer);
        RenderEmptyState(renderer);   // 부품 위에 빈 상태 안내
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

    // 통계 카드 4장 + 검색 조건 카드 + 결과 카드 (머리말 · 표 · 아래 줄)
    void AccessoryFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        // 가운데 정렬 열 (바깥 여백 안쪽)
        Core::RectF area{ PageLayout::GetPageColumn(Core::RectF{ contentArea.m_left + PAGE_PADDING, contentArea.m_top + PAGE_PADDING,
            contentArea.m_right - PAGE_PADDING, contentArea.m_bottom - PAGE_PADDING }) };

        // 위쪽: 통계 카드 4장
        const Core::RectF statRow{ PageLayout::TakeTop(area, PageLayout::STAT_CARD_HEIGHT) };
        const std::vector<Core::RectF> statColumns{ PageLayout::SplitColumns(statRow, STAT_CARD_COUNT, PageLayout::CARD_GAP) };
        for (std::size_t index{}; index < STAT_CARD_COUNT; ++index) m_statBounds[index] = statColumns[index];

        // 검색 조건 카드
        m_filterBounds = PageLayout::TakeTop(area, FILTER_CARD_HEIGHT);
        m_resultBounds = area;   // 나머지는 모두 결과 카드

        // 한 줄에 왼쪽부터 차례로 놓기
        const float left{ m_filterBounds.m_left + PageLayout::CARD_PADDING };
        const float right{ m_filterBounds.m_right - PageLayout::CARD_PADDING };
        float x{ left };
        float rowTop{ m_filterBounds.m_top + PageLayout::CARD_PADDING };
        const auto place{ [&x, &rowTop](UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, rowTop, x + width, rowTop + Theme::CONTROL_HEIGHT });
            x += width + gapAfter;
        } };

        // 첫 줄: 분류 [목걸이] 품질 [전체] 주스탯 [최소 수치] [검색] 상태
        place(m_categoryLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_categoryBox, CATEGORY_BOX_WIDTH, GROUP_GAP);
        place(m_qualityLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_qualityBox, QUALITY_BOX_WIDTH, GROUP_GAP);
        place(m_mainStatLabel, MAIN_STAT_LABEL_WIDTH, LABEL_GAP);   // 주스탯
        place(m_mainStatBox, MAIN_STAT_BOX_WIDTH, SECTION_GAP);     // [최소 수치]
        place(m_searchButton, SEARCH_BUTTON_WIDTH, SECTION_GAP);
        m_statusLabel.SetBounds(Core::RectF{ x, rowTop, std::max(x, right), rowTop + Theme::CONTROL_HEIGHT });

        // 둘째 줄: 연마 효과 [옵션][값] x 3
        x = left;
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;
        place(m_optionLabel, OPTION_LABEL_WIDTH, LABEL_GAP);
        for (const OptionSlot& slot : m_optionSlots)
        {
            place(*slot.m_optionBox, OPTION_BOX_WIDTH, LABEL_GAP);
            place(*slot.m_valueBox, VALUE_BOX_WIDTH, GROUP_GAP);
        }

        // 결과 카드: 머리말 아래 표, 맨 아래 줄에 페이지 이동
        const float bodyTop{ m_resultBounds.m_top + PageLayout::CARD_HEADER_HEIGHT };
        const float footerTop{ std::max(bodyTop, m_resultBounds.m_bottom - FOOTER_HEIGHT) };
        m_footerBounds = Core::RectF{ m_resultBounds.m_left, footerTop, m_resultBounds.m_right, m_resultBounds.m_bottom };
        m_gridBounds = Core::RectF{ m_resultBounds.m_left, bodyTop, m_resultBounds.m_right, footerTop };
        m_listingGrid.SetBounds(m_gridBounds);

        // 아래 줄 오른쪽 끝에서부터: [다음 ▶] [◀ 이전] 페이지 글자
        const float buttonTop{ footerTop + (FOOTER_HEIGHT - Theme::CONTROL_HEIGHT) * HALF };
        float buttonRight{ m_footerBounds.m_right - PageLayout::CARD_PADDING };
        m_nextButton.SetBounds(Core::RectF{ buttonRight - PAGE_BUTTON_WIDTH, buttonTop, buttonRight, buttonTop + Theme::CONTROL_HEIGHT });
        buttonRight -= PAGE_BUTTON_WIDTH + LABEL_GAP;
        m_previousButton.SetBounds(Core::RectF{ buttonRight - PAGE_BUTTON_WIDTH, buttonTop, buttonRight, buttonTop + Theme::CONTROL_HEIGHT });
        buttonRight -= PAGE_BUTTON_WIDTH + PAGE_LABEL_RIGHT_GAP;
        m_pageLabel.SetBounds(Core::RectF{ buttonRight - PAGE_LABEL_WIDTH, buttonTop, buttonRight, buttonTop + Theme::CONTROL_HEIGHT });
    }

    // 카드 바탕 · 머리말 · 통계 값 · 표 아래 설명 (부품은 m_host가 그림)
    void AccessoryFinderView::RenderCards(Core::Renderer& renderer) const
    {
        // 통계 카드 (실제 검색 결과 값만, 검색 전이면 "-")
        const std::wstring noValue{ EMPTY_TEXT };
        const std::array<PageLayout::StatCard, STAT_CARD_COUNT> cards{
            PageLayout::StatCard{ std::wstring{ ICON_COUNT }, Theme::ACCENT_COLOR, std::wstring{ STAT_COUNT_LABEL },
                m_hasSearched ? std::format(STAT_COUNT_FORMAT, Core::NumberFormat::FormatNumber(static_cast<double>(m_totalCount), AMOUNT_DECIMALS)) : noValue,
                m_hasSearched ? std::wstring{ STAT_COUNT_SUB } : std::wstring{}, Theme::MUTED_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_LOWEST }, Theme::SPARKLINE_UP_COLOR, std::wstring{ STAT_LOWEST_LABEL },
                m_lowestPrice.has_value() ? FormatAmount(*m_lowestPrice) : noValue,
                m_lowestPrice.has_value() ? std::wstring{ STAT_PRICE_SUB } : std::wstring{}, Theme::HIGHLIGHT_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_AVERAGE }, Theme::HIGHLIGHT_TEXT_COLOR, std::wstring{ STAT_AVERAGE_LABEL },
                m_averagePrice.has_value() ? FormatAmount(*m_averagePrice) : noValue,
                m_averagePrice.has_value() ? std::wstring{ STAT_PRICE_SUB } : std::wstring{}, Theme::HIGHLIGHT_TEXT_COLOR },
            PageLayout::StatCard{ std::wstring{ ICON_TIME }, Theme::TOTAL_TEXT_COLOR, std::wstring{ STAT_TIME_LABEL },
                m_searchedAtText.empty() ? noValue : m_searchedAtText, std::wstring{}, Theme::MUTED_TEXT_COLOR } };
        for (std::size_t index{}; index < STAT_CARD_COUNT; ++index) PageLayout::DrawStatCard(renderer, m_statBounds[index], cards[index]);

        PageLayout::DrawCard(renderer, m_filterBounds);   // 검색 조건 카드
        PageLayout::DrawCardWithHeader(renderer, m_resultBounds, CARD_EYEBROW, CARD_TITLE, {}, true);   // 결과 카드 (LIVE)
        renderer.DrawLine(m_footerBounds.m_left, m_footerBounds.m_top, m_footerBounds.m_right, m_footerBounds.m_top,
            Theme::CARD_BORDER_COLOR, Theme::BORDER_WIDTH);   // 표와 아래 줄 사이 선
        PageLayout::DrawTableFooter(renderer, m_footerBounds, PANEL_TITLE, {});   // 아래 줄 왼쪽 설명 (오른쪽은 페이지 이동 버튼)
    }

    // 표 위에 덧그리는 빈 상태 안내 (매물이 없을 때)
    void AccessoryFinderView::RenderEmptyState(Core::Renderer& renderer) const
    {
        if (m_listingGrid.GetRowCount() > 0) return;   // 매물이 있으면 안내 없음
        const Core::RectF emptyRect{ m_gridBounds.m_left, m_gridBounds.m_top + EMPTY_STATE_TOP_OFFSET, m_gridBounds.m_right, m_gridBounds.m_bottom };
        if (m_hasSearched) PageLayout::DrawEmptyState(renderer, emptyRect, EMPTY_NONE_TITLE, EMPTY_NONE_HINT);   // 검색했지만 0건
        else PageLayout::DrawEmptyState(renderer, emptyRect, EMPTY_BEFORE_TITLE, EMPTY_BEFORE_HINT);   // 아직 검색 전
    }

    // 조회 중이 아니고 넘길 페이지가 있을 때만 이전 · 다음 버튼 사용
    void AccessoryFinderView::UpdatePageButtons() noexcept
    {
        m_previousButton.SetEnabled(!m_isBusy && m_hasPrevious);
        m_nextButton.SetEnabled(!m_isBusy && m_hasNext);
    }
}
