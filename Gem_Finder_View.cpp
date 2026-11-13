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
        // ===== 최저가 표 (행 = 보석 하나, 레벨 × 종류) =====
        constexpr std::size_t PRICE_COL_NAME{ 0 };
        constexpr std::size_t PRICE_COL_TYPE{ 1 };
        constexpr std::size_t PRICE_COL_CURRENT{ 2 };
        constexpr std::size_t PRICE_COL_YESTERDAY{ 3 };
        constexpr std::size_t PRICE_COL_CHANGE{ 4 };
        constexpr std::size_t PRICE_COL_TREND{ 5 };
        constexpr std::size_t PRICE_COLUMN_COUNT{ 6 };
        constexpr float NAME_COLUMN_WEIGHT{ 1.8f };      // 레벨 배지 + 이름
        constexpr float TYPE_COLUMN_WEIGHT{ 0.8f };      // 분류 칩
        constexpr float CURRENT_COLUMN_WEIGHT{ 1.5f };   // "12,345 (▲1,234)" 이 들어가도록
        constexpr float YESTERDAY_COLUMN_WEIGHT{ 1.1f };
        constexpr float CHANGE_COLUMN_WEIGHT{ 0.8f };
        constexpr float TREND_COLUMN_WEIGHT{ 1.0f };     // 추이 선 칸

        // ===== 알림 표 =====
        constexpr std::size_t ALARM_COL_GEM{ 0 };
        constexpr std::size_t ALARM_COL_TARGET{ 1 };
        constexpr std::size_t ALARM_COL_CURRENT{ 2 };
        constexpr std::size_t ALARM_COL_STATE{ 3 };
        constexpr std::size_t ALARM_COLUMN_COUNT{ 4 };
        constexpr float ALARM_COLUMN_WEIGHT{ 1.0f };

        constexpr int AMOUNT_DECIMALS{ 0 };              // 원본 "N0"
        constexpr int RATE_DECIMALS{ 1 };                // 변동률은 소수 첫째 자리까지
        constexpr std::size_t TARGET_PRICE_MAX_LENGTH{ 14 };   // "100,000,000" + 여유
        constexpr std::size_t SEARCH_MAX_LENGTH{ 20 };
        constexpr std::size_t STAT_CARD_COUNT{ 4 };      // 위쪽 통계 카드 수

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view REFRESHING_TEXT{ L"조회 중…" };   // 조회하는 동안 버튼 글자
        constexpr std::wstring_view AUTO_REFRESH_TEXT{ L"자동 갱신" };
        constexpr std::wstring_view WATCH_LABEL_TEXT{ L"감시" };
        constexpr std::wstring_view FULL_LABEL_TEXT{ L"전체" };
        constexpr std::wstring_view SECONDS_SUFFIX{ L"초" };
        constexpr std::wstring_view MINUTES_SUFFIX{ L"분" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view SEARCH_PLACEHOLDER{ L"보석 이름 검색" };
        constexpr std::wstring_view CLEAR_TEXT{ L"✕" };
        constexpr std::wstring_view ALL_CATEGORY_TEXT{ L"전체 분류" };
        constexpr std::wstring_view NOT_QUERIED_TEXT{ L"-" };
        constexpr std::wstring_view NO_LISTING_TEXT{ L"매물 없음" };
        constexpr std::wstring_view UP_MARK{ L"▲" };
        constexpr std::wstring_view DOWN_MARK{ L"▼" };
        constexpr std::wstring_view PLUS_SIGN{ L"+" };
        constexpr std::wstring_view PERCENT_SUFFIX{ L"%" };
        constexpr std::wstring_view TARGET_PLACEHOLDER{ L"목표가 (골드)" };
        constexpr std::wstring_view ADD_ALARM_TEXT{ L"추가/변경" };
        constexpr std::wstring_view REMOVE_ALARM_TEXT{ L"삭제" };
        constexpr std::wstring_view CHART_VALUE_SUFFIX{ L" 골드" };

        constexpr std::array<std::wstring_view, ALARM_COLUMN_COUNT> ALARM_HEADERS{ L"보석", L"목표가", L"현재 최저가", L"상태" };
        constexpr std::array<std::wstring_view, PRICE_COLUMN_COUNT> PRICE_HEADERS{ L"보석", L"분류", L"현재 최저가", L"전일 최저가", L"변동", L"추이" };
        constexpr std::array<float, PRICE_COLUMN_COUNT> PRICE_WEIGHTS{ NAME_COLUMN_WEIGHT, TYPE_COLUMN_WEIGHT, CURRENT_COLUMN_WEIGHT,
            YESTERDAY_COLUMN_WEIGHT, CHANGE_COLUMN_WEIGHT, TREND_COLUMN_WEIGHT };
        constexpr std::wstring_view ALARM_STATE_OUT_OF_RANGE{ L"조회 범위 밖 (삭제 권장)" };
        constexpr std::wstring_view ALARM_STATE_NOT_CHECKED{ L"확인 전" };
        constexpr std::wstring_view ALARM_STATE_REACHED{ L"✔ 도달" };
        constexpr std::wstring_view ALARM_STATE_WAITING{ L"대기" };

        // ===== 카드 머리말 · 표 아래 줄 · 빈 안내 =====
        constexpr std::wstring_view PRICE_EYEBROW{ L"LIVE MARKET" };
        constexpr std::wstring_view PRICE_TITLE{ L"겁화 · 작열 최저가" };
        constexpr std::wstring_view PRICE_RIGHT_TEXT{ L"등락은 직전 전체 갱신 대비" };
        constexpr std::wstring_view ALARM_EYEBROW{ L"PRICE ALERT" };
        constexpr std::wstring_view ALARM_TITLE{ L"목표가 알림" };
        constexpr std::wstring_view ALARM_RIGHT_TEXT{ L"감시 갱신 대상" };
        constexpr std::wstring_view CHART_EYEBROW{ L"PRICE HISTORY" };
        constexpr std::wstring_view CHART_TITLE{ L"시세 기록 (일별 최저가)" };
        constexpr std::wstring_view FOOTER_LEFT_FORMAT{ L"총 {}개의 검색 결과" };
        constexpr std::wstring_view FOOTER_RIGHT_TEXT{ L"변동률은 전일 최저가 대비" };
        constexpr std::wstring_view EMPTY_SEARCH_TITLE{ L"조건에 맞는 보석이 없어요" };
        constexpr std::wstring_view EMPTY_SEARCH_HINT{ L"검색어를 줄이거나 분류를 [전체 분류]로 바꿔 보세요." };

        // ===== 통계 카드 =====
        constexpr std::wstring_view STAT_ICON_COUNT{ L"▤" };
        constexpr std::wstring_view STAT_ICON_RATE{ L"↗" };
        constexpr std::wstring_view STAT_ICON_TOP{ L"◎" };
        constexpr std::wstring_view STAT_ICON_TIME{ L"◷" };
        constexpr std::wstring_view STAT_LABEL_COUNT{ L"조회한 보석" };
        constexpr std::wstring_view STAT_LABEL_RATE{ L"평균 등락" };
        constexpr std::wstring_view STAT_LABEL_TOP{ L"최고가 보석" };
        constexpr std::wstring_view STAT_LABEL_TIME{ L"마지막 갱신" };
        constexpr std::wstring_view STAT_COUNT_FORMAT{ L"{}종" };
        constexpr std::wstring_view STAT_LISTED_FORMAT{ L"매물 {}종" };
        constexpr std::wstring_view STAT_RATE_SUB{ L"전일 최저가 대비" };
        constexpr std::wstring_view STAT_PRICE_FORMAT{ L"{} 골드" };
        constexpr std::wstring_view STAT_AUTO_ON_TEXT{ L"자동 갱신 켜짐" };
        constexpr std::wstring_view STAT_AUTO_OFF_TEXT{ L"자동 갱신 꺼짐" };

        // 정렬 콤보 상자 항목 (GemSortOrder 순서와 같음)
        constexpr std::size_t SORT_OPTION_COUNT{ 4 };
        constexpr std::array<std::wstring_view, SORT_OPTION_COUNT> SORT_ITEMS{ L"가격 높은 순", L"가격 낮은 순", L"등락 높은 순", L"등락 낮은 순" };
        constexpr std::size_t DEFAULT_SORT_INDEX{ static_cast<std::size_t>(GemSortOrder::SORT_PRICE_DESC) };

        // ===== 배치 (DIP) =====
        constexpr float CONTROL_GAP{ 8.0f };
        constexpr float NO_GAP{ 0.0f };                  // 줄 맨 끝 부품 뒤에는 간격 없음
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float LABEL_GAP{ 4.0f };               // "감시" 글자와 콤보 상자 사이
        constexpr float REFRESH_BUTTON_WIDTH{ 104.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 96.0f };
        constexpr float SHORT_LABEL_WIDTH{ 30.0f };      // "감시" · "전체"
        constexpr float INTERVAL_BOX_WIDTH{ 76.0f };
        constexpr float SEARCH_BOX_MAX_WIDTH{ 220.0f };
        constexpr float SEARCH_BOX_MIN_WIDTH{ 100.0f };
        constexpr float CLEAR_BUTTON_WIDTH{ 32.0f };
        constexpr float CATEGORY_BOX_WIDTH{ 120.0f };
        constexpr float SORT_BOX_WIDTH{ 130.0f };
        constexpr float STATUS_WIDTH{ 280.0f };
        constexpr float STATUS_ROW_HEIGHT{ 26.0f };      // 검색 줄 아래 상태 · 호출 한도 줄
        constexpr float STATUS_ROW_INSET{ 6.0f };        // 그 줄의 위아래 여백
        constexpr float GEM_BOX_WIDTH{ 130.0f };
        constexpr float CHART_GEM_BOX_WIDTH{ 150.0f };
        constexpr float TARGET_BOX_MIN_WIDTH{ 80.0f };
        constexpr float ADD_BUTTON_WIDTH{ 90.0f };
        constexpr float REMOVE_BUTTON_WIDTH{ 64.0f };
        constexpr float ALARM_INPUT_GAP_COUNT{ 3.0f };   // 알림 입력 줄 부품 4개 사이 간격 수
        constexpr float INPUT_TOP_GAP{ 12.0f };          // 카드 머리말 아래 ~ 입력 줄
        constexpr float HALF{ 0.5f };                    // 세로 가운데 계산
        constexpr float PRICE_WIDTH_WEIGHT{ 1.45f };     // 표 카드 : 오른쪽 열 너비 비율
        constexpr float SIDE_WIDTH_WEIGHT{ 1.0f };
        constexpr std::size_t BODY_COLUMN_COUNT{ 2 };    // 본문 열 수 (표 · 알림+그래프)
        constexpr float ALARM_HEIGHT_RATIO{ 0.45f };     // 오른쪽 열 중 알림 카드 높이 몫

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };

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

        // ===== 보석 레벨 → 게임 등급 (7 = 전설 · 8~9 = 유물 · 10 = 고대) =====
        constexpr int LEGENDARY_MAX_GEM_LEVEL{ 7 };    // 이 레벨까지 전설 등급
        constexpr int ANCIENT_MIN_GEM_LEVEL{ 10 };     // 이 레벨부터 고대 등급 (그 사이는 유물)

        // 레벨 배지 · 이름 글자색 (강화책 · 젬 이름과 같은 게임 등급 색)
        [[nodiscard]] const Core::Color& GetLevelGradeColor(int level) noexcept
        {
            if (level <= LEGENDARY_MAX_GEM_LEVEL) return Theme::GRADE_LEGENDARY_COLOR;   // 전설
            if (level >= ANCIENT_MIN_GEM_LEVEL) return Theme::GRADE_ANCIENT_COLOR;       // 고대
            return Theme::GRADE_RELIC_COLOR;                                             // 유물
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
                default:                           return Theme::MUTED_TEXT_COLOR;
            }
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

        [[nodiscard]] std::vector<UI::GridColumn> MakePriceColumns()
        {
            std::vector<UI::GridColumn> columns;
            columns.reserve(PRICE_COLUMN_COUNT);
            for (std::size_t column{}; column < PRICE_COLUMN_COUNT; ++column)
            {
                Core::TextAlign align{ Core::TextAlign::TRAILING };   // 숫자 열은 오른쪽
                if (column == PRICE_COL_NAME) align = Core::TextAlign::LEADING;
                else if (column == PRICE_COL_TYPE || column == PRICE_COL_TREND) align = Core::TextAlign::CENTER;
                columns.push_back(UI::GridColumn{ std::wstring{ PRICE_HEADERS[column] }, PRICE_WEIGHTS[column], align });
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

        // 분류 콤보 상자 항목 ("전체 분류" + 보석 종류들)
        [[nodiscard]] std::vector<std::wstring> MakeCategoryItems()
        {
            std::vector<std::wstring> items{ std::wstring{ ALL_CATEGORY_TEXT } };
            for (const Models::GemType& type : Models::GemCatalog::GEM_TYPES) items.push_back(Core::TextEncoding::UTF8ToWide(std::string{ type.m_shortName }));
            return items;
        }

        [[nodiscard]] std::vector<std::wstring> MakeSortItems()
        {
            std::vector<std::wstring> items;
            items.reserve(SORT_ITEMS.size());
            for (const std::wstring_view item : SORT_ITEMS) items.emplace_back(item);
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
        , m_searchBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ SEARCH_PLACEHOLDER }) }
        , m_clearButton{ m_host.Emplace<UI::Button>(std::wstring{ CLEAR_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_categoryBox{ m_host.Emplace<UI::ComboBox>() }
        , m_sortBox{ m_host.Emplace<UI::ComboBox>() }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }
        , m_rateLimitLabel{ m_host.Emplace<UI::Label>(std::wstring{}, BAR_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }
        , m_priceGrid{ m_host.Emplace<UI::Grid>(platform, MakePriceColumns(), 0) }
        , m_alarmGemBox{ m_host.Emplace<UI::ComboBox>() }
        , m_targetPriceBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ TARGET_PLACEHOLDER }) }
        , m_addAlarmButton{ m_host.Emplace<UI::Button>(std::wstring{ ADD_ALARM_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_removeAlarmButton{ m_host.Emplace<UI::Button>(std::wstring{ REMOVE_ALARM_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, nullptr) }
        , m_alarmGrid{ m_host.Emplace<UI::Grid>(platform, MakeAlarmColumns(), 0) }
        , m_chartGemBox{ m_host.Emplace<UI::ComboBox>() }
        , m_chart{ m_host.Emplace<UI::LineChart>() }
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

        // 검색: 글자가 바뀔 때마다 바로 거름, ✕ = 한 번에 지우고 다시 입력할 수 있게 포커스
        m_searchBox.SetMaxLength(SEARCH_MAX_LENGTH);
        m_searchBox.SetOnTextChanged([this] { NotifyFilterChanged(); });
        m_clearButton.SetOnClick([this]
        {
            m_searchBox.SetText(std::wstring{});   // 검색어 비움
            NotifyFilterChanged();                  // 표를 전체로 되돌림
            m_host.SetFocus(&m_searchBox);          // 바로 다시 입력할 수 있게
        });

        // 분류 · 정렬: 기본은 전체 분류 + 가격 높은 순
        m_categoryBox.SetItems(MakeCategoryItems());
        m_categoryBox.SetSelectedIndex(0);
        m_categoryBox.SetOnSelectionChanged([this] { NotifyFilterChanged(); });
        m_sortBox.SetItems(MakeSortItems());
        m_sortBox.SetSelectedIndex(DEFAULT_SORT_INDEX);
        m_sortBox.SetOnSelectionChanged([this] { NotifyFilterChanged(); });

        // 최저가 표: 읽기 전용, 행 단위 선택, 행을 누르면 그 보석을 그래프 · 알림 입력칸에 바로 선택
        m_priceGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
        m_priceGrid.SetRowHeight(PageLayout::GRID_ROW_HEIGHT);
        m_priceGrid.SetOnCellClicked([this](std::size_t row, std::size_t)
        {
            if (row >= m_rowGems.size()) return;   // 경계 검사 (표 · 행 목록이 어긋난 순간 방어)
            if (const auto onPriceCellClicked{ m_onPriceCellClicked }) onPriceCellClicked(m_rowGems[row]);
        });

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
            SyncRowSelection();   // 표에서도 같은 보석 행이 선택되게
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

    void GemFinderView::SetOnFilterChanged(std::function<void()> onFilterChanged)
    {
        m_onFilterChanged = std::move(onFilterChanged);
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
        m_refreshButton.SetText(std::wstring{ isEnabled ? REFRESH_TEXT : REFRESHING_TEXT });   // 조회 중에는 글자도 바꿔 알림
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

    const std::wstring& GemFinderView::GetSearchText() const noexcept
    {
        return m_searchBox.GetText();
    }

    // 분류 콤보 상자 0번 = 전체 분류, 1번부터 보석 종류 순서
    std::optional<std::string> GemFinderView::GetCategoryTypeName() const
    {
        const std::optional<std::size_t> index{ m_categoryBox.GetSelectedIndex() };
        if (!index.has_value() || *index == 0 || *index > Models::GemCatalog::GEM_TYPES.size()) return std::nullopt;
        return std::string{ Models::GemCatalog::GEM_TYPES[*index - 1].m_shortName };
    }

    GemSortOrder GemFinderView::GetSortOrder() const noexcept
    {
        const std::size_t index{ m_sortBox.GetSelectedIndex().value_or(DEFAULT_SORT_INDEX) };
        return index < SORT_OPTION_COUNT ? static_cast<GemSortOrder>(index) : GemSortOrder::SORT_PRICE_DESC;
    }

    // 표 다시 채우기: 보석 한 줄 = 레벨 배지 + 이름 · 분류 칩 · 현재 최저가(▲▼) · 전일 최저가 · 변동 % · 추이 선
    void GemFinderView::ShowRows(const std::vector<GemRow>& rows, std::size_t totalCount)
    {
        m_rowCount = rows.size();    // 표 아래 "총 N개" · 빈 안내 판단용
        m_totalCount = totalCount;
        m_rowGems.clear();
        m_rowGems.reserve(rows.size());
        m_priceGrid.SetRowCount(rows.size());

        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const GemRow& gemRow{ rows[row] };
            m_rowGems.push_back(gemRow.m_gem);
            const UI::GridCellStyle normalStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
            const UI::GridCellStyle mutedStyle{ Theme::CELL_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

            // 이름: 레벨 배지 + 등급색 이름 (7 전설 · 8~9 유물 · 10 고대)
            const Core::Color& gradeColor{ GetLevelGradeColor(gemRow.m_gem.m_level) };
            m_priceGrid.SetCellText(row, PRICE_COL_NAME, gemRow.m_name);
            m_priceGrid.SetCellStyle(row, PRICE_COL_NAME, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, gradeColor, Core::FontWeight::BOLD,
                UI::GridDecoration::DECORATION_BADGE, std::format(L"{}", gemRow.m_gem.m_level), gradeColor });

            // 분류: 테두리 칩
            m_priceGrid.SetCellText(row, PRICE_COL_TYPE, gemRow.m_typeText);
            m_priceGrid.SetCellStyle(row, PRICE_COL_TYPE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR, Core::FontWeight::NORMAL,
                UI::GridDecoration::DECORATION_CHIP, std::wstring{}, Theme::MUTED_TEXT_COLOR });

            // 현재 최저가 (▲ 빨강 / ▼ 파랑 / 매물 없음 회색, 원본 FormatPriceCell)
            std::wstring currentText;
            Core::Color currentColor{ Theme::TEXT_COLOR };
            if (!gemRow.m_price.has_value())
            {
                currentText = std::wstring{ gemRow.m_isQueried ? NO_LISTING_TEXT : NOT_QUERIED_TEXT };   // 조회 전이면 "-"
                currentColor = Theme::MUTED_TEXT_COLOR;
            }
            else
            {
                const std::int64_t diff{ gemRow.m_diff.value_or(0) };
                currentText = FormatAmount(*gemRow.m_price);
                if (diff > 0) currentText += std::format(L" ({}{})", UP_MARK, FormatAmount(diff));
                else if (diff < 0) currentText += std::format(L" ({}{})", DOWN_MARK, FormatAmount(-diff));
                currentColor = GetTrendColor(static_cast<double>(diff));
            }
            m_priceGrid.SetCellText(row, PRICE_COL_CURRENT, std::move(currentText));
            m_priceGrid.SetCellStyle(row, PRICE_COL_CURRENT, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, currentColor,
                gemRow.m_price.has_value() ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL });

            // 전일 최저가 (일별 기록에 어제 값이 있을 때만)
            m_priceGrid.SetCellText(row, PRICE_COL_YESTERDAY, gemRow.m_yesterdayPrice.has_value() ? FormatAmount(*gemRow.m_yesterdayPrice) : std::wstring{ NOT_QUERIED_TEXT });
            m_priceGrid.SetCellStyle(row, PRICE_COL_YESTERDAY, gemRow.m_yesterdayPrice.has_value() ? normalStyle : mutedStyle);

            // 전일 대비 변동률 ("+3.2%" 빨강 / "-1.5%" 파랑, 계산할 수 없으면 "-")
            if (gemRow.m_changeRate.has_value())
            {
                const double rate{ *gemRow.m_changeRate };
                const std::wstring sign{ rate > 0.0 ? PLUS_SIGN : std::wstring_view{} };
                m_priceGrid.SetCellText(row, PRICE_COL_CHANGE, sign + Core::NumberFormat::FormatNumber(rate, RATE_DECIMALS) + std::wstring{ PERCENT_SUFFIX });
                m_priceGrid.SetCellStyle(row, PRICE_COL_CHANGE, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTrendColor(rate) });
            }
            else
            {
                m_priceGrid.SetCellText(row, PRICE_COL_CHANGE, std::wstring{ NOT_QUERIED_TEXT });
                m_priceGrid.SetCellStyle(row, PRICE_COL_CHANGE, mutedStyle);
            }

            // 추이: 일별 최저가 기록 (기록이 2개 미만이면 선 없음)
            m_priceGrid.SetCellText(row, PRICE_COL_TREND, std::wstring{});
            m_priceGrid.SetCellStyle(row, PRICE_COL_TREND, normalStyle);
            m_priceGrid.SetCellSparkline(row, PRICE_COL_TREND, gemRow.m_trend);
        }

        SyncRowSelection();   // 그래프 보석의 행을 다시 선택 상태로
    }

    void GemFinderView::SetStats(GemStats stats)
    {
        m_stats = std::move(stats);
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
        SyncRowSelection();   // 표에서도 같은 보석 행을 선택 상태로
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

        // 위쪽 통계 카드 4장
        const std::vector<PageLayout::StatCard> cards{ BuildStatCards() };
        for (std::size_t index{}; index < cards.size() && index < m_statBounds.size(); ++index)
        {
            PageLayout::DrawStatCard(renderer, m_statBounds[index], cards[index]);
        }

        PageLayout::DrawCard(renderer, m_filterBounds);                                                                         // 조회 · 검색 줄 카드
        PageLayout::DrawCardWithHeader(renderer, m_priceCardBounds, PRICE_EYEBROW, PRICE_TITLE, PRICE_RIGHT_TEXT, true);          // 최저가 표 카드
        PageLayout::DrawCardWithHeader(renderer, m_alarmCardBounds, ALARM_EYEBROW, ALARM_TITLE, ALARM_RIGHT_TEXT);                // 목표가 알림 카드
        PageLayout::DrawCardWithHeader(renderer, m_chartCardBounds, CHART_EYEBROW, CHART_TITLE);                                  // 시세 기록 카드
        m_host.Render(renderer);
        RenderTableExtras(renderer);                                                                                           // 표 위에 얹는 "총 N개" 줄 · 빈 안내
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

    // 위에서부터: 통계 카드 4장 → 조회 · 검색 줄 → (왼쪽 최저가 표 | 오른쪽 위 알림 · 아래 그래프)
    void GemFinderView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        Core::RectF remaining{ contentArea };
        m_statBounds = PageLayout::SplitColumns(PageLayout::TakeTop(remaining, PageLayout::STAT_CARD_HEIGHT), STAT_CARD_COUNT, PageLayout::CARD_GAP);   // 통계 카드 4장
        m_filterBounds = PageLayout::TakeTop(remaining, PageLayout::FILTER_BAR_HEIGHT + STATUS_ROW_HEIGHT);                                              // 조회 · 검색 줄 (+ 상태 줄)

        // 윗줄: [시세 갱신] [자동 갱신] 감시 [10초] 전체 [1분] [검색 ✕] [분류] [정렬]
        const float controlTop{ m_filterBounds.m_top + (PageLayout::FILTER_BAR_HEIGHT - PageLayout::CONTROL_HEIGHT) * HALF };
        const float controlBottom{ controlTop + PageLayout::CONTROL_HEIGHT };
        const float left{ m_filterBounds.m_left + PageLayout::CARD_PADDING };
        const float right{ m_filterBounds.m_right - PageLayout::CARD_PADDING };
        float x{ left };
        const auto place{ [&x, controlTop, controlBottom](UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, controlTop, x + width, controlBottom });
            x += width + gapAfter;
        } };
        place(m_refreshButton, REFRESH_BUTTON_WIDTH, GROUP_GAP);
        place(m_autoRefreshBox, AUTO_REFRESH_WIDTH, GROUP_GAP);
        place(m_watchLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_watchIntervalBox, INTERVAL_BOX_WIDTH, GROUP_GAP);
        place(m_fullLabel, SHORT_LABEL_WIDTH, LABEL_GAP);
        place(m_fullIntervalBox, INTERVAL_BOX_WIDTH, GROUP_GAP);

        // 검색칸은 남는 너비에 맞춰 줄어듦 (오른쪽 ✕ · 분류 · 정렬 자리를 먼저 뺌)
        const float rightGroupWidth{ CONTROL_GAP + CLEAR_BUTTON_WIDTH + GROUP_GAP + CATEGORY_BOX_WIDTH + CONTROL_GAP + SORT_BOX_WIDTH };
        const float searchWidth{ std::clamp(right - x - rightGroupWidth, SEARCH_BOX_MIN_WIDTH, SEARCH_BOX_MAX_WIDTH) };
        place(m_searchBox, searchWidth, CONTROL_GAP);
        place(m_clearButton, CLEAR_BUTTON_WIDTH, GROUP_GAP);
        place(m_categoryBox, CATEGORY_BOX_WIDTH, CONTROL_GAP);
        place(m_sortBox, SORT_BOX_WIDTH, NO_GAP);

        // 아랫줄: 마지막 갱신 상태 · 호출 한도 글자
        const float statusTop{ m_filterBounds.m_top + PageLayout::FILTER_BAR_HEIGHT - STATUS_ROW_INSET };
        const float statusBottom{ m_filterBounds.m_bottom - STATUS_ROW_INSET };
        m_statusLabel.SetBounds(Core::RectF{ left, statusTop, std::min(right, left + STATUS_WIDTH), statusBottom });
        const float rateLeft{ std::min(right, left + STATUS_WIDTH + GROUP_GAP) };
        m_rateLimitLabel.SetBounds(Core::RectF{ rateLeft, statusTop, std::max(rateLeft, right), statusBottom });

        // 본문: 왼쪽 최저가 표 카드 | 오른쪽 열 (위 알림 카드 · 아래 그래프 카드)
        const std::array<float, BODY_COLUMN_COUNT> weights{ PRICE_WIDTH_WEIGHT, SIDE_WIDTH_WEIGHT };
        const std::vector<Core::RectF> columns{ PageLayout::SplitColumns(remaining, BODY_COLUMN_COUNT, PageLayout::CARD_GAP, weights) };
        m_priceCardBounds = columns[0];
        Core::RectF side{ columns[1] };
        const float alarmHeight{ (side.m_bottom - side.m_top - PageLayout::CARD_GAP) * ALARM_HEIGHT_RATIO };
        m_alarmCardBounds = PageLayout::TakeTop(side, alarmHeight);
        m_chartCardBounds = side;

        // 최저가 표 카드: 머리말 아래 표 + 맨 아래 "총 N개" 줄
        const float gridTop{ m_priceCardBounds.m_top + PageLayout::CARD_HEADER_HEIGHT };
        const float footerTop{ std::max(gridTop, m_priceCardBounds.m_bottom - PageLayout::TABLE_FOOTER_HEIGHT) };
        m_priceGrid.SetBounds(Core::RectF{ m_priceCardBounds.m_left, gridTop, m_priceCardBounds.m_right, footerTop });
        m_footerBounds = Core::RectF{ m_priceCardBounds.m_left, footerTop, m_priceCardBounds.m_right, m_priceCardBounds.m_bottom };
        m_emptyBounds = Core::RectF{ m_priceCardBounds.m_left, std::min(footerTop, gridTop + Theme::GRID_HEADER_HEIGHT), m_priceCardBounds.m_right, footerTop };

        // 알림 카드: [보석] [목표가 입력] [추가/변경] [삭제] + 알림 목록
        const float alarmInnerLeft{ m_alarmCardBounds.m_left + PageLayout::CARD_PADDING };
        const float alarmInnerRight{ m_alarmCardBounds.m_right - PageLayout::CARD_PADDING };
        const float inputTop{ m_alarmCardBounds.m_top + PageLayout::CARD_HEADER_HEIGHT + INPUT_TOP_GAP };
        const float inputBottom{ inputTop + PageLayout::CONTROL_HEIGHT };
        const float targetWidth{ std::max(TARGET_BOX_MIN_WIDTH, alarmInnerRight - alarmInnerLeft - GEM_BOX_WIDTH - ADD_BUTTON_WIDTH - REMOVE_BUTTON_WIDTH - CONTROL_GAP * ALARM_INPUT_GAP_COUNT) };
        float inputX{ alarmInnerLeft };
        const auto placeInput{ [&inputX, inputTop, inputBottom](UI::Control& control, float width)
        {
            control.SetBounds(Core::RectF{ inputX, inputTop, inputX + width, inputBottom });
            inputX += width + CONTROL_GAP;
        } };
        placeInput(m_alarmGemBox, GEM_BOX_WIDTH);
        placeInput(m_targetPriceBox, targetWidth);
        placeInput(m_addAlarmButton, ADD_BUTTON_WIDTH);
        placeInput(m_removeAlarmButton, REMOVE_BUTTON_WIDTH);
        const float alarmGridTop{ inputBottom + CONTROL_GAP };
        m_alarmGrid.SetBounds(Core::RectF{ alarmInnerLeft, alarmGridTop, alarmInnerRight,
            std::max(alarmGridTop, m_alarmCardBounds.m_bottom - PageLayout::CARD_PADDING) });

        // 그래프 카드: [보석] + 일별 최저가 그래프
        const float chartInnerLeft{ m_chartCardBounds.m_left + PageLayout::CARD_PADDING };
        const float chartInnerRight{ m_chartCardBounds.m_right - PageLayout::CARD_PADDING };
        const float chartBoxTop{ m_chartCardBounds.m_top + PageLayout::CARD_HEADER_HEIGHT + INPUT_TOP_GAP };
        const float chartBoxBottom{ chartBoxTop + PageLayout::CONTROL_HEIGHT };
        m_chartGemBox.SetBounds(Core::RectF{ chartInnerLeft, chartBoxTop, std::min(chartInnerRight, chartInnerLeft + CHART_GEM_BOX_WIDTH), chartBoxBottom });
        const float chartTop{ chartBoxBottom + CONTROL_GAP };
        m_chart.SetBounds(Core::RectF{ chartInnerLeft, chartTop, chartInnerRight, std::max(chartTop, m_chartCardBounds.m_bottom - PageLayout::CARD_PADDING) });
    }

    // 통계 값 → 카드 4장 (값이 없으면 "-", 실제 값만 보여 줌)
    std::vector<PageLayout::StatCard> GemFinderView::BuildStatCards() const
    {
        const bool hasQuotes{ m_stats.m_queriedCount > 0 };

        // 조회한 보석 수 · 그중 매물이 있는 수
        PageLayout::StatCard countCard{ std::wstring{ STAT_ICON_COUNT }, Theme::ACCENT_COLOR, std::wstring{ STAT_LABEL_COUNT },
            hasQuotes ? std::format(STAT_COUNT_FORMAT, m_stats.m_queriedCount) : std::wstring{ NOT_QUERIED_TEXT },
            hasQuotes ? std::format(STAT_LISTED_FORMAT, m_stats.m_listedCount) : std::wstring{}, Theme::MUTED_TEXT_COLOR };

        // 전일 최저가 대비 평균 변동률
        PageLayout::StatCard rateCard{ std::wstring{ STAT_ICON_RATE }, Theme::SPARKLINE_UP_COLOR, std::wstring{ STAT_LABEL_RATE }, std::wstring{ NOT_QUERIED_TEXT },
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

        // 현재 최저가가 가장 비싼 보석
        PageLayout::StatCard topCard{ std::wstring{ STAT_ICON_TOP }, Theme::GRADE_ANCIENT_COLOR, std::wstring{ STAT_LABEL_TOP }, std::wstring{ NOT_QUERIED_TEXT },
            std::wstring{}, Theme::MUTED_TEXT_COLOR };
        if (!m_stats.m_topName.empty())
        {
            topCard.m_value = m_stats.m_topName;
            topCard.m_sub = std::format(STAT_PRICE_FORMAT, FormatAmount(m_stats.m_topPrice));
        }

        // 마지막 갱신 시각 + 자동 갱신 상태
        PageLayout::StatCard timeCard{ std::wstring{ STAT_ICON_TIME }, Theme::ACCENT_COLOR, std::wstring{ STAT_LABEL_TIME },
            m_stats.m_updatedText.empty() ? std::wstring{ NOT_QUERIED_TEXT } : m_stats.m_updatedText,
            std::wstring{ m_autoRefreshBox.IsChecked() ? STAT_AUTO_ON_TEXT : STAT_AUTO_OFF_TEXT }, Theme::MUTED_TEXT_COLOR };

        return std::vector<PageLayout::StatCard>{ std::move(countCard), std::move(rateCard), std::move(topCard), std::move(timeCard) };
    }

    // 표 아래 "총 N개의 검색 결과" 줄 + 거른 결과가 없을 때 해결 방법 안내
    void GemFinderView::RenderTableExtras(Core::Renderer& renderer) const
    {
        PageLayout::DrawTableFooter(renderer, m_footerBounds, std::format(FOOTER_LEFT_FORMAT, m_rowCount), FOOTER_RIGHT_TEXT);
        if (m_rowCount == 0) PageLayout::DrawEmptyState(renderer, m_emptyBounds, EMPTY_SEARCH_TITLE, EMPTY_SEARCH_HINT);
    }

    void GemFinderView::NotifyFilterChanged() const
    {
        if (const auto onFilterChanged{ m_onFilterChanged }) onFilterChanged();
    }

    // 그래프 보석과 같은 보석의 행을 표에서 선택 상태로 (거르기 · 정렬로 행 순서가 바뀌어도 따라감, 표에 없으면 선택 해제)
    void GemFinderView::SyncRowSelection()
    {
        const std::optional<Models::GemSelection> gem{ GetChartGem() };
        std::optional<std::size_t> selectedRow;
        if (gem.has_value())
        {
            if (const auto found{ std::ranges::find(m_rowGems, *gem) }; found != m_rowGems.end())
            {
                selectedRow = static_cast<std::size_t>(std::distance(m_rowGems.begin(), found));
            }
        }
        m_priceGrid.SelectRow(selectedRow);
    }

    std::optional<std::size_t> GemFinderView::FindGemIndex(const Models::GemSelection& gem) const
    {
        const auto found{ std::find(m_gems.begin(), m_gems.end(), gem) };
        if (found == m_gems.end()) return std::nullopt;
        return static_cast<std::size_t>(std::distance(m_gems.begin(), found));
    }
}
