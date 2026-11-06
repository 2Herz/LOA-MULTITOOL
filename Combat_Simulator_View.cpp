module LostArkMultiTool.Views.CombatSimulatorView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.Control;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

namespace LostArkMultiTool::Views
{
    namespace
    {
        // ===== 장비 표 =====
        constexpr std::size_t GEAR_COLUMN_COUNT{ static_cast<std::size_t>(CombatGearColumn::COLUMN_COUNT) };
        constexpr std::array<std::wstring_view, GEAR_COLUMN_COUNT> GEAR_HEADERS{
            L"부위", L"아이템", L"현재 일반", L"현재 상급", L"목표 일반", L"목표 상급" };
        constexpr std::array<float, GEAR_COLUMN_COUNT> GEAR_WEIGHTS{ 0.6f, 2.6f, 0.8f, 0.8f, 0.8f, 0.8f };

        // ===== 보석 표 =====
        constexpr std::size_t GEM_COLUMN_COUNT{ static_cast<std::size_t>(CombatGemColumn::COLUMN_COUNT) };
        constexpr std::array<std::wstring_view, GEM_COLUMN_COUNT> GEM_HEADERS{ L"슬롯", L"보석", L"현재 레벨", L"목표 레벨" };
        constexpr std::array<float, GEM_COLUMN_COUNT> GEM_WEIGHTS{ 0.6f, 1.0f, 1.0f, 1.0f };

        // ===== 목록형 카드 표 (항목 / 현재 / 목표) =====
        constexpr std::size_t CARD_COUNT{ static_cast<std::size_t>(CombatCard::CARD_COUNT) };
        constexpr std::size_t LIST_COLUMN_COUNT{ 3 };
        constexpr std::size_t LIST_LABEL_COLUMN{ 0 };
        constexpr std::size_t LIST_CURRENT_COLUMN{ 1 };
        constexpr std::size_t LIST_TARGET_COLUMN{ 2 };
        constexpr std::array<std::wstring_view, LIST_COLUMN_COUNT> LIST_HEADERS{ L"항목", L"현재", L"목표" };
        constexpr std::array<float, LIST_COLUMN_COUNT> LIST_WEIGHTS{ 1.2f, 1.6f, 1.6f };
        constexpr std::array<std::wstring_view, CARD_COUNT> CARD_TITLES{
            L"악세서리 연마 옵션 5종", L"아크 그리드 (코어 6 · 젬 효과)", L"팔찌 · 어빌리티 스톤 · 각인서", L"아크 패시브 · 아바타" };

        // 카드가 놓이는 칸 (열, 행) — 위 줄: 장비 재련 | 악세서리 | 아크 그리드, 아래 줄: 보석 · 품질 | 팔찌 · 스톤 · 각인서 | 아크 패시브 · 아바타
        struct CardCell
        {
            std::size_t m_column{};
            std::size_t m_row{};
        };
        constexpr std::array<CardCell, CARD_COUNT> CARD_CELLS{ { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } } };
        constexpr CardCell GEAR_CELL{ 0, 0 };
        constexpr CardCell GEM_CELL{ 0, 1 };
        constexpr std::size_t GRID_COLUMN_COUNT{ 3 };
        constexpr std::size_t GRID_ROW_COUNT{ 2 };

        // ===== 글자 =====
        constexpr std::wstring_view NAME_TEXT{ L"캐릭터 이름" };
        constexpr std::wstring_view NAME_PLACEHOLDER{ L"캐릭터 이름" };
        constexpr std::wstring_view SEARCH_TEXT{ L"조회" };
        constexpr std::wstring_view SEARCH_BUSY_TEXT{ L"조회 중..." };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"캐릭터 이름을 넣고 [조회]를 누르면 무기고를 불러와요." };
        constexpr std::wstring_view QUALITY_PLACEHOLDER{ L"100" };
        constexpr std::wstring_view GEM_TARGET_TEXT{ L"모든 보석 목표 레벨" };
        constexpr std::wstring_view GEAR_PANEL_TITLE{ L"장비 재련 · 완갑 (목표 칸에 숫자를 넣어요)" };
        constexpr std::wstring_view OPTION_PANEL_TITLE{ L"보석 · 무기 품질" };
        constexpr std::wstring_view NOTE_TEXT{ L"※ lopec 측정 기반 추정치예요. 근사: 스톤 1번 줄 없음(불안정) · 직업 각인(미측정) · 스톤과 같은 이름의 각인서(약 ±4%) · 아크 패시브 랭크(세 트리 중 가장 낮은 값) · 깨달음 포인트 구성(최대로 가정) · 완갑(에스더/일반 두 가지 표)" };
        constexpr std::wstring_view HINT_DEFAULT_TEXT{ L"항목을 고르면 여기서 선택지를 고를 수 있어요. 숫자 항목은 목표 칸을 눌러 바로 입력해요." };
        constexpr std::wstring_view HINT_NUMBER_FORMAT{ L"{}   현재 {}  →  목표 {}   (목표 칸을 눌러 숫자를 입력)" };
        constexpr std::wstring_view HINT_CHOICE_FORMAT{ L"{}   현재 {}  →  목표 {}" };
        constexpr std::size_t NAME_MAX_LENGTH{ 20 };
        constexpr std::size_t QUALITY_MAX_LENGTH{ 3 };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 12.0f };
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float LABEL_GAP{ 6.0f };
        constexpr float NAME_LABEL_WIDTH{ 84.0f };
        constexpr float NAME_BOX_WIDTH{ 200.0f };
        constexpr float SEARCH_BUTTON_WIDTH{ 96.0f };
        constexpr float QUALITY_LABEL_WIDTH{ 230.0f };
        constexpr float QUALITY_BOX_WIDTH{ 72.0f };
        constexpr float GEM_TARGET_LABEL_WIDTH{ 140.0f };
        constexpr float GEM_TARGET_BOX_WIDTH{ 160.0f };
        constexpr float PANEL_PADDING{ 10.0f };
        constexpr float PANEL_TITLE_LEFT{ 12.0f };
        constexpr float BAR_PADDING{ 10.0f };
        constexpr float MIN_CARD_HEIGHT{ 150.0f };
        constexpr float ROW_GAP{ 4.0f };
        constexpr float HINT_LINE_COUNT{ 2.0f };       // 선택한 행 안내 자리 (두 줄)
        constexpr float HINT_LINE_HEIGHT{ 17.0f };
        constexpr float NOTE_LINE_COUNT{ 2.0f };       // 안내 문구 자리 (두 줄)
        constexpr float NOTE_LINE_HEIGHT{ 17.0f };
        constexpr float RESULT_FRACTION_CURRENT{ 0.22f };     // 결과 줄에서 칸마다 차지하는 너비 비율
        constexpr float RESULT_FRACTION_PREDICTED{ 0.30f };
        constexpr float RESULT_FRACTION_CHANGE{ 0.24f };

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle SETTING_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };
        constexpr Core::TextStyle RESULT_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle PREDICTED_TEXT_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::BOLD, Core::TextAlign::LEADING };
        constexpr Core::TextStyle NOTE_TEXT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle PANEL_TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };

        // 재련 · 품질 · 목록 숫자 칸: 숫자만
        [[nodiscard]] bool IsDigitCharacter(wchar_t character) noexcept
        {
            return character >= L'0' && character <= L'9';
        }

        [[nodiscard]] constexpr std::size_t ToColumn(CombatGearColumn column) noexcept
        {
            return static_cast<std::size_t>(column);
        }

        [[nodiscard]] constexpr std::size_t ToColumn(CombatGemColumn column) noexcept
        {
            return static_cast<std::size_t>(column);
        }

        // 보석 표 열 (모두 가운데 정렬)
        [[nodiscard]] std::vector<UI::GridColumn> MakeGemColumns()
        {
            std::vector<UI::GridColumn> columns;
            columns.reserve(GEM_COLUMN_COUNT);
            for (std::size_t column{}; column < GEM_COLUMN_COUNT; ++column)
            {
                columns.push_back(UI::GridColumn{ std::wstring{ GEM_HEADERS[column] }, GEM_WEIGHTS[column], Core::TextAlign::CENTER });
            }
            return columns;
        }

        // 부위 · 아이템 열은 왼쪽 정렬, 숫자 열은 가운데
        [[nodiscard]] std::vector<UI::GridColumn> MakeGearColumns()
        {
            std::vector<UI::GridColumn> columns;
            columns.reserve(GEAR_COLUMN_COUNT);
            for (std::size_t column{}; column < GEAR_COLUMN_COUNT; ++column)
            {
                const bool isLeading{ column == ToColumn(CombatGearColumn::COLUMN_PART) || column == ToColumn(CombatGearColumn::COLUMN_ITEM) };
                columns.push_back(UI::GridColumn{ std::wstring{ GEAR_HEADERS[column] }, GEAR_WEIGHTS[column], isLeading ? Core::TextAlign::LEADING : Core::TextAlign::CENTER });
            }
            return columns;
        }

        // 목록형 카드 표 열 (모두 왼쪽 정렬: 문구가 길어서)
        [[nodiscard]] std::vector<UI::GridColumn> MakeListColumns()
        {
            std::vector<UI::GridColumn> columns;
            columns.reserve(LIST_COLUMN_COUNT);
            for (std::size_t column{}; column < LIST_COLUMN_COUNT; ++column)
            {
                columns.push_back(UI::GridColumn{ std::wstring{ LIST_HEADERS[column] }, LIST_WEIGHTS[column], Core::TextAlign::LEADING });
            }
            return columns;
        }

        // 3 x 2 칸 중 (열, 행) 칸의 영역
        [[nodiscard]] Core::RectF GetCellBounds(const Core::RectF& area, const CardCell& cell) noexcept
        {
            const float width{ (area.GetWidth() - SECTION_GAP * static_cast<float>(GRID_COLUMN_COUNT - 1)) / static_cast<float>(GRID_COLUMN_COUNT) };
            const float height{ (area.GetHeight() - SECTION_GAP * static_cast<float>(GRID_ROW_COUNT - 1)) / static_cast<float>(GRID_ROW_COUNT) };
            const float left{ area.m_left + static_cast<float>(cell.m_column) * (width + SECTION_GAP) };
            const float top{ area.m_top + static_cast<float>(cell.m_row) * (height + SECTION_GAP) };
            return Core::RectF{ left, top, left + width, top + std::max(height, 0.0f) };
        }
    }

    CombatSimulatorView::CombatSimulatorView(Core::Platform& platform)
        : m_nameLabel{ m_host.Emplace<UI::Label>(std::wstring{ NAME_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }
        , m_searchButton{ m_host.Emplace<UI::Button>(std::wstring{ SEARCH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_gearGrid{ m_host.Emplace<UI::Grid>(platform, MakeGearColumns(), 0) }
        , m_qualityLabel{ m_host.Emplace<UI::Label>(std::wstring{}, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_qualityBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ QUALITY_PLACEHOLDER }) }
        , m_gemLabel{ m_host.Emplace<UI::Label>(std::wstring{}, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_gemTargetLabel{ m_host.Emplace<UI::Label>(std::wstring{ GEM_TARGET_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_gemTargetBox{ m_host.Emplace<UI::ComboBox>() }
        , m_gemGrid{ m_host.Emplace<UI::Grid>(platform, MakeGemColumns(), 0) }
        , m_currentPowerLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_predictedPowerLabel{ m_host.Emplace<UI::Label>(std::wstring{}, PREDICTED_TEXT_STYLE, Theme::HIGHLIGHT_TEXT_COLOR) }
        , m_changeLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_itemLevelLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_factorLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }
        , m_noteLabel{ m_host.Emplace<UI::Label>(std::wstring{ NOTE_TEXT }, NOTE_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }
        , m_barPanel{ Core::RectF{}, std::wstring{} }
        , m_gearPanel{ Core::RectF{}, std::wstring{ GEAR_PANEL_TITLE } }
        , m_optionPanel{ Core::RectF{}, std::wstring{ OPTION_PANEL_TITLE } }
    {
        // 조회: 버튼 · 이름 칸 Enter (이름 정리 · 요청은 매니저가)
        m_nameBox.SetMaxLength(NAME_MAX_LENGTH);
        m_nameBox.SetOnSubmit([this] { RequestSearch(); });
        m_searchButton.SetOnClick([this] { RequestSearch(); });

        // 장비 표: 목표 숫자 칸만 편집
        m_gearGrid.SetEditInput(false, IsDigitCharacter);
        m_gearGrid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text)
        {
            if (const auto onGearCellEdited{ m_onGearCellEdited }) onGearCellEdited(row, static_cast<CombatGearColumn>(column), text);
        });

        // 무기 품질: 숫자만, 입력할 때마다 알림 (범위 확인은 매니저가)
        m_qualityBox.SetIMEAllowed(false);
        m_qualityBox.SetMaxLength(QUALITY_MAX_LENGTH);
        m_qualityBox.SetInputFilter(IsDigitCharacter);
        m_qualityBox.SetOnTextChanged([this]
        {
            if (m_isUpdatingQualityText) return;
            if (const auto onQualityChanged{ m_onQualityChanged }) onQualityChanged(m_qualityBox.GetText());
        });

        // 보석 표: 목표 레벨 칸만 숫자 편집 (범위 확인은 매니저가)
        m_gemGrid.SetEditInput(false, IsDigitCharacter);
        m_gemGrid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text)
        {
            if (column != ToColumn(CombatGemColumn::COLUMN_TARGET_LEVEL)) return;
            if (const auto onGemCellEdited{ m_onGemCellEdited }) onGemCellEdited(row, text);
        });

        // 보석 목표: 고르면 번호만 알림
        m_gemTargetBox.SetOnSelectionChanged([this]
        {
            const std::optional<std::size_t> index{ m_gemTargetBox.GetSelectedIndex() };
            if (const auto onGemTargetSelected{ m_onGemTargetSelected }; index.has_value() && onGemTargetSelected) onGemTargetSelected(*index);
        });

        // 목록형 카드: 선택 상자 · 안내 · 표 (Tab 순서 = 카드 순서)
        for (std::size_t index{}; index < CARD_COUNT; ++index)
        {
            ListCard& card{ m_cards[index] };
            card.m_panel = Panel{ Core::RectF{}, std::wstring{ CARD_TITLES[index] } };
            card.m_combo = &m_host.Emplace<UI::ComboBox>();
            card.m_hint = &m_host.Emplace<UI::Label>(std::wstring{ HINT_DEFAULT_TEXT }, NOTE_TEXT_STYLE, Theme::MUTED_TEXT_COLOR);
            card.m_grid = &m_host.Emplace<UI::Grid>(platform, MakeListColumns(), 0);

            card.m_combo->SetEnabled(false);
            card.m_combo->SetOnSelectionChanged([this, index] { OnCardChoice(index); });

            card.m_grid->SetEditInput(false, IsDigitCharacter);
            card.m_grid->SetOnSelectionChanged([this, index] { SyncCardEditor(index); });
            card.m_grid->SetOnCellEdited([this, index](std::size_t row, std::size_t column, const std::wstring& text)
            {
                if (column != LIST_TARGET_COLUMN) return;
                if (const auto onCardEdited{ m_onCardEdited }) onCardEdited(static_cast<CombatCard>(index), row, text);
            });
        }
    }

    CombatSimulatorView::~CombatSimulatorView() = default;

    void CombatSimulatorView::SetOnSearch(std::function<void(const std::wstring& characterName)> onSearch)
    {
        m_onSearch = std::move(onSearch);
    }

    void CombatSimulatorView::SetOnGearCellEdited(std::function<void(std::size_t row, CombatGearColumn column, const std::wstring& text)> onGearCellEdited)
    {
        m_onGearCellEdited = std::move(onGearCellEdited);
    }

    void CombatSimulatorView::SetOnQualityChanged(std::function<void(const std::wstring& text)> onQualityChanged)
    {
        m_onQualityChanged = std::move(onQualityChanged);
    }

    void CombatSimulatorView::SetOnGemTargetSelected(std::function<void(std::size_t index)> onGemTargetSelected)
    {
        m_onGemTargetSelected = std::move(onGemTargetSelected);
    }

    void CombatSimulatorView::SetOnGemCellEdited(std::function<void(std::size_t row, const std::wstring& text)> onGemCellEdited)
    {
        m_onGemCellEdited = std::move(onGemCellEdited);
    }

    void CombatSimulatorView::SetOnCardEdited(std::function<void(CombatCard card, std::size_t row, const std::wstring& text)> onCardEdited)
    {
        m_onCardEdited = std::move(onCardEdited);
    }

    void CombatSimulatorView::SetCharacterName(std::wstring name)
    {
        m_nameBox.SetText(std::move(name));
    }

    void CombatSimulatorView::SetSearchBusy(bool isBusy)
    {
        m_searchButton.SetEnabled(!isBusy);
        m_searchButton.SetText(std::wstring{ isBusy ? SEARCH_BUSY_TEXT : SEARCH_TEXT });
    }

    void CombatSimulatorView::SetStatusText(std::wstring text, bool isError)
    {
        m_statusLabel.SetText(std::move(text));
        m_statusLabel.SetColor(isError ? Theme::ALERT_TEXT_COLOR : Theme::TEXT_COLOR);
    }

    // 장비 표 다시 채우기 (목표 칸 = 편집 가능하면 일반 칸 색, 바뀐 칸은 강조색 · 굵게, 아이템 = 등급 색)
    void CombatSimulatorView::ShowGears(const std::vector<CombatGearRow>& rows)
    {
        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle lockedStyle{ Theme::READONLY_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

        m_gearGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const CombatGearRow& gear{ rows[row] };
            const auto setCell{ [this, row](CombatGearColumn column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)
            {
                m_gearGrid.SetCellText(row, ToColumn(column), text);
                m_gearGrid.SetCellStyle(row, ToColumn(column), style);
                m_gearGrid.SetCellEditable(row, ToColumn(column), isEditable);
            } };

            const UI::GridCellStyle targetStyle{ Theme::CELL_BACK_COLOR, gear.m_hasChange ? Theme::UP_TEXT_COLOR : Theme::TEXT_COLOR,
                gear.m_hasChange ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL };

            setCell(CombatGearColumn::COLUMN_PART, gear.m_partName, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD }, false);
            setCell(CombatGearColumn::COLUMN_ITEM, gear.m_itemName, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GetGradeColor(gear.m_gradeName), Core::FontWeight::BOLD }, false);
            setCell(CombatGearColumn::COLUMN_CURRENT_ENHANCEMENT, gear.m_currentEnhancement, readonlyStyle, false);
            setCell(CombatGearColumn::COLUMN_CURRENT_ADVANCED, gear.m_currentAdvanced, readonlyStyle, false);
            setCell(CombatGearColumn::COLUMN_TARGET_ENHANCEMENT, gear.m_targetEnhancement, gear.m_isTargetEditable ? targetStyle : lockedStyle, gear.m_isTargetEditable);
            const bool isAdvancedEditable{ gear.m_isTargetEditable && gear.m_isTargetAdvancedEditable };
            setCell(CombatGearColumn::COLUMN_TARGET_ADVANCED, gear.m_targetAdvanced, isAdvancedEditable ? targetStyle : lockedStyle, isAdvancedEditable);
        }
    }

    void CombatSimulatorView::SetQualityText(std::wstring currentText)
    {
        m_qualityLabel.SetText(std::move(currentText));
    }

    void CombatSimulatorView::SetQualityTarget(std::wstring text)
    {
        if (m_qualityBox.GetText() == text) return;
        m_isUpdatingQualityText = true;
        m_qualityBox.SetText(std::move(text));
        m_isUpdatingQualityText = false;
    }

    void CombatSimulatorView::SetQualityEnabled(bool isEnabled) noexcept
    {
        m_qualityBox.SetEnabled(isEnabled);
    }

    void CombatSimulatorView::SetGemText(std::wstring text)
    {
        m_gemLabel.SetText(std::move(text));
    }

    void CombatSimulatorView::SetGemTargetItems(std::vector<std::wstring> items)
    {
        m_gemTargetBox.SetItems(std::move(items));
    }

    void CombatSimulatorView::SetGemTargetIndex(std::size_t index)
    {
        m_gemTargetBox.SetSelectedIndex(index);
    }

    void CombatSimulatorView::SetGemTargetEnabled(bool isEnabled) noexcept
    {
        m_gemTargetBox.SetEnabled(isEnabled);
    }

    // 보석 표 다시 채우기 (목표 레벨 = 편집 칸, 바뀐 칸은 강조색 · 굵게)
    void CombatSimulatorView::ShowGems(const std::vector<CombatGemRow>& rows)
    {
        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };

        m_gemGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const CombatGemRow& gem{ rows[row] };
            const auto setCell{ [this, row](CombatGemColumn column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)
            {
                m_gemGrid.SetCellText(row, ToColumn(column), text);
                m_gemGrid.SetCellStyle(row, ToColumn(column), style);
                m_gemGrid.SetCellEditable(row, ToColumn(column), isEditable);
            } };

            setCell(CombatGemColumn::COLUMN_SLOT, gem.m_slot, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD }, false);
            setCell(CombatGemColumn::COLUMN_KIND, gem.m_kind, readonlyStyle, false);
            setCell(CombatGemColumn::COLUMN_CURRENT_LEVEL, gem.m_currentLevel, readonlyStyle, false);
            setCell(CombatGemColumn::COLUMN_TARGET_LEVEL, gem.m_targetLevel, UI::GridCellStyle{ Theme::CELL_BACK_COLOR,
                gem.m_hasChange ? Theme::UP_TEXT_COLOR : Theme::TEXT_COLOR, gem.m_hasChange ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL }, true);
        }
    }

    // 목록형 카드 다시 채우기 (구분 행 = 굵은 제목, 숫자 행 = 목표 칸 편집, 선택 행 = 위 선택 상자로 고르기)
    void CombatSimulatorView::ShowCard(CombatCard card, std::vector<CombatListRow> rows)
    {
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };
        if (cardIndex >= CARD_COUNT) return;

        ListCard& listCard{ m_cards[cardIndex] };
        UI::Grid& grid{ *listCard.m_grid };
        listCard.m_rows = std::move(rows);

        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle labelStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::NORMAL };
        const UI::GridCellStyle headerStyle{ Theme::HEADER_BACK_COLOR, Theme::TOTAL_TEXT_COLOR, Core::FontWeight::BOLD };

        grid.SetRowCount(listCard.m_rows.size());
        for (std::size_t row{}; row < listCard.m_rows.size(); ++row)
        {
            const CombatListRow& item{ listCard.m_rows[row] };
            const bool isHeader{ item.m_kind == CombatRowKind::ROW_HEADER };
            const bool isNumber{ item.m_kind == CombatRowKind::ROW_NUMBER };

            const auto setCell{ [&grid, row](std::size_t column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)
            {
                grid.SetCellText(row, column, text);
                grid.SetCellStyle(row, column, style);
                grid.SetCellEditable(row, column, isEditable);
            } };

            const UI::GridCellStyle targetStyle{ Theme::CELL_BACK_COLOR, item.m_hasChange ? Theme::UP_TEXT_COLOR : Theme::TEXT_COLOR,
                item.m_hasChange ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL };

            setCell(LIST_LABEL_COLUMN, item.m_label, isHeader ? headerStyle : labelStyle, false);
            setCell(LIST_CURRENT_COLUMN, item.m_current, isHeader ? headerStyle : readonlyStyle, false);
            setCell(LIST_TARGET_COLUMN, item.m_target, isHeader ? headerStyle : targetStyle, isNumber);
        }
        SyncCardEditor(cardIndex);
    }

    // 선택한 행에 맞춰 선택 상자 · 안내 갱신 (선택지가 있는 행이면 상자를 켜고 목표를 표시)
    void CombatSimulatorView::SyncCardEditor(std::size_t cardIndex)
    {
        ListCard& card{ m_cards[cardIndex] };
        const std::optional<std::size_t> selected{ card.m_grid->GetSelectedRow() };
        const CombatListRow* row{ selected.has_value() && *selected < card.m_rows.size() ? &card.m_rows[*selected] : nullptr };

        if (row == nullptr || row->m_kind == CombatRowKind::ROW_HEADER)
        {
            card.m_combo->SetItems({});
            card.m_combo->SetEnabled(false);
            card.m_hint->SetText(std::wstring{ HINT_DEFAULT_TEXT });
            return;
        }

        const bool isChoice{ row->m_kind == CombatRowKind::ROW_CHOICE };
        card.m_combo->SetItems(isChoice ? row->m_choices : std::vector<std::wstring>{});
        card.m_combo->SetSelectedIndex(isChoice ? row->m_selectedChoice : std::nullopt);
        card.m_combo->SetEnabled(isChoice);
        card.m_hint->SetText(isChoice ? std::format(HINT_CHOICE_FORMAT, row->m_label, row->m_current, row->m_target)
            : std::format(HINT_NUMBER_FORMAT, row->m_label, row->m_current, row->m_target));
    }

    // 선택 상자에서 고름 → 선택한 행의 선택지 글자를 알림
    void CombatSimulatorView::OnCardChoice(std::size_t cardIndex)
    {
        ListCard& card{ m_cards[cardIndex] };
        const std::optional<std::size_t> selectedRow{ card.m_grid->GetSelectedRow() };
        const std::optional<std::size_t> choice{ card.m_combo->GetSelectedIndex() };
        if (!selectedRow.has_value() || !choice.has_value() || *selectedRow >= card.m_rows.size()) return;

        const CombatListRow& row{ card.m_rows[*selectedRow] };
        if (row.m_kind != CombatRowKind::ROW_CHOICE || *choice >= row.m_choices.size()) return;

        const std::wstring chosenText{ row.m_choices[*choice] };   // 알림 뒤 표가 다시 채워지므로 복사해서 넘김
        if (const auto onCardEdited{ m_onCardEdited }) onCardEdited(static_cast<CombatCard>(cardIndex), *selectedRow, chosenText);
    }

    // 결과 바 (증감 색: 오르면 빨강 ▲ · 내리면 파랑 ▼, 다른 표와 같은 색)
    void CombatSimulatorView::ShowResult(const CombatResultText& result)
    {
        m_currentPowerLabel.SetText(result.m_currentPower);
        m_predictedPowerLabel.SetText(result.m_predictedPower);
        m_changeLabel.SetText(result.m_change);
        m_changeLabel.SetColor(result.m_isUp ? Theme::UP_TEXT_COLOR : (result.m_isDown ? Theme::DOWN_TEXT_COLOR : Theme::TEXT_COLOR));
        m_itemLevelLabel.SetText(result.m_itemLevel);
        m_factorLabel.SetText(result.m_factors);
    }

    void CombatSimulatorView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void CombatSimulatorView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderPanels(renderer);
        m_host.Render(renderer);
    }

    bool CombatSimulatorView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape CombatSimulatorView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void CombatSimulatorView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void CombatSimulatorView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    void CombatSimulatorView::RequestSearch()
    {
        m_nameBox.CommitComposition();   // 조합 중인 한글 마지막 글자까지 포함
        if (const auto onSearch{ m_onSearch }) onSearch(m_nameBox.GetText());
    }

    // 맨 위 결과 바 / 3 x 2 카드 / 맨 아래 안내 문구
    void CombatSimulatorView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ std::max(contentArea.m_right - PAGE_PADDING, left) };
        const float barTop{ contentArea.m_top + PAGE_PADDING };
        const float innerLeft{ left + BAR_PADDING };
        const float innerRight{ std::max(right - BAR_PADDING, innerLeft) };

        // 줄 하나에 왼쪽부터 차례로 배치
        const auto placeRow{ [](float& x, float top, UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, top, x + width, top + Theme::CONTROL_HEIGHT });
            x += width + gapAfter;
        } };

        // 결과 바 첫 줄: 캐릭터 이름 [입력칸] [조회] 상태
        float y{ barTop + BAR_PADDING };
        float x{ innerLeft };
        placeRow(x, y, m_nameLabel, NAME_LABEL_WIDTH, LABEL_GAP);
        placeRow(x, y, m_nameBox, NAME_BOX_WIDTH, GROUP_GAP);
        placeRow(x, y, m_searchButton, SEARCH_BUTTON_WIDTH, SECTION_GAP);
        m_statusLabel.SetBounds(Core::RectF{ x, y, std::max(x, innerRight), y + Theme::CONTROL_HEIGHT });
        y += Theme::CONTROL_HEIGHT + ROW_GAP;

        // 둘째 줄: 현재 전투력 → 예상 전투력 · 증감 · 아이템 레벨
        const float innerWidth{ innerRight - innerLeft };
        const float currentWidth{ innerWidth * RESULT_FRACTION_CURRENT };
        const float predictedWidth{ innerWidth * RESULT_FRACTION_PREDICTED };
        const float changeWidth{ innerWidth * RESULT_FRACTION_CHANGE };
        x = innerLeft;
        placeRow(x, y, m_currentPowerLabel, currentWidth, 0.0f);
        placeRow(x, y, m_predictedPowerLabel, predictedWidth, 0.0f);
        placeRow(x, y, m_changeLabel, changeWidth, 0.0f);
        m_itemLevelLabel.SetBounds(Core::RectF{ x, y, std::max(x, innerRight), y + Theme::CONTROL_HEIGHT });
        y += Theme::CONTROL_HEIGHT + ROW_GAP;

        // 셋째 줄: 배율 내역
        m_factorLabel.SetBounds(Core::RectF{ innerLeft, y, innerRight, y + Theme::CONTROL_HEIGHT });
        y += Theme::CONTROL_HEIGHT + BAR_PADDING;
        m_barPanel.m_bounds = Core::RectF{ left, barTop, right, y };

        // 맨 아래 안내 문구
        const float noteHeight{ NOTE_LINE_HEIGHT * NOTE_LINE_COUNT };
        const float noteTop{ std::max(y + SECTION_GAP, contentArea.m_bottom - PAGE_PADDING - noteHeight) };
        m_noteLabel.SetBounds(Core::RectF{ left, noteTop, right, noteTop + noteHeight });

        // 3 x 2 카드
        const float cardsTop{ y + SECTION_GAP };
        const float cardsBottom{ std::max(cardsTop + MIN_CARD_HEIGHT * static_cast<float>(GRID_ROW_COUNT) + SECTION_GAP, noteTop - SECTION_GAP) };
        const Core::RectF cardsArea{ left, cardsTop, right, cardsBottom };

        // 장비 재련 카드: 표 하나 (완갑 행 포함)
        m_gearGrid.SetBounds(LayoutPanel(m_gearPanel, GetCellBounds(cardsArea, GEAR_CELL)));

        // 보석 · 무기 품질 카드
        const Core::RectF optionInner{ LayoutPanel(m_optionPanel, GetCellBounds(cardsArea, GEM_CELL)) };
        float rowTop{ optionInner.m_top };
        x = optionInner.m_left;
        placeRow(x, rowTop, m_qualityLabel, std::min(QUALITY_LABEL_WIDTH, optionInner.GetWidth() - QUALITY_BOX_WIDTH - LABEL_GAP), LABEL_GAP);
        placeRow(x, rowTop, m_qualityBox, QUALITY_BOX_WIDTH, GROUP_GAP);
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;
        m_gemLabel.SetBounds(Core::RectF{ optionInner.m_left, rowTop, optionInner.m_right, rowTop + Theme::CONTROL_HEIGHT });
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;
        x = optionInner.m_left;
        placeRow(x, rowTop, m_gemTargetLabel, GEM_TARGET_LABEL_WIDTH, LABEL_GAP);
        placeRow(x, rowTop, m_gemTargetBox, GEM_TARGET_BOX_WIDTH, GROUP_GAP);
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;
        m_gemGrid.SetBounds(Core::RectF{ optionInner.m_left, rowTop, optionInner.m_right, std::max(rowTop, optionInner.m_bottom) });   // 남은 높이 전부 (넘치면 표가 스크롤)

        // 목록형 카드 4개
        for (std::size_t index{}; index < CARD_COUNT; ++index) LayoutListCard(m_cards[index], GetCellBounds(cardsArea, CARD_CELLS[index]));
    }

    // 목록형 카드: 위 선택 상자 / 선택한 행 안내 / 아래 표 (펼친 목록이 표 위에 덮이도록 선택 상자를 맨 위에 둠)
    void CombatSimulatorView::LayoutListCard(ListCard& card, const Core::RectF& bounds)
    {
        const Core::RectF inner{ LayoutPanel(card.m_panel, bounds) };
        float rowTop{ inner.m_top };
        card.m_combo->SetBounds(Core::RectF{ inner.m_left, rowTop, inner.m_right, rowTop + Theme::CONTROL_HEIGHT });
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;

        const float hintHeight{ HINT_LINE_HEIGHT * HINT_LINE_COUNT };
        card.m_hint->SetBounds(Core::RectF{ inner.m_left, rowTop, inner.m_right, rowTop + hintHeight });
        rowTop += hintHeight + ROW_GAP;
        card.m_grid->SetBounds(Core::RectF{ inner.m_left, rowTop, inner.m_right, std::max(rowTop, inner.m_bottom) });
    }

    Core::RectF CombatSimulatorView::LayoutPanel(Panel& panel, const Core::RectF& bounds) const
    {
        panel.m_bounds = bounds;
        return Core::RectF{ bounds.m_left + PANEL_PADDING, bounds.m_top + Theme::PANEL_TITLE_HEIGHT,
            std::max(bounds.m_right - PANEL_PADDING, bounds.m_left + PANEL_PADDING), std::max(bounds.m_top + Theme::PANEL_TITLE_HEIGHT, bounds.m_bottom - PANEL_PADDING) };
    }

    // 묶음 상자: 둥근 테두리 + 왼쪽 위 제목 (결과 바는 제목 없음)
    void CombatSimulatorView::RenderPanels(Core::Renderer& renderer) const
    {
        std::vector<const Panel*> panels{ &m_barPanel, &m_gearPanel, &m_optionPanel };
        for (const ListCard& card : m_cards) panels.push_back(&card.m_panel);

        for (const Panel* panel : panels)
        {
            renderer.DrawRoundedRect(panel->m_bounds, Theme::PANEL_CORNER_RADIUS, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);
            if (panel->m_title.empty()) continue;

            const Core::RectF titleRect{ panel->m_bounds.m_left + PANEL_TITLE_LEFT, panel->m_bounds.m_top,
                panel->m_bounds.m_right - PANEL_TITLE_LEFT, panel->m_bounds.m_top + Theme::PANEL_TITLE_HEIGHT };
            renderer.DrawString(panel->m_title, titleRect, Theme::TEXT_COLOR, PANEL_TITLE_STYLE);
        }
    }
}
