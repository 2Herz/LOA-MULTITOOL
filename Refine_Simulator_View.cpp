module LostArkMultiTool.Views.RefineSimulatorView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.PageLayout;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
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
        // ===== 부위 표 =====
        constexpr std::size_t SLOT_COLUMN_COUNT{ static_cast<std::size_t>(RefineSlotColumn::COLUMN_COUNT) };
        constexpr std::array<std::wstring_view, SLOT_COLUMN_COUNT> SLOT_HEADERS{
            L"부위", L"시작 장비", L"시작 강화", L"시작 상급", L"목표 장비", L"목표 강화", L"목표 상급", L"변화", L"예상 골드 · 횟수" };
        constexpr std::array<float, SLOT_COLUMN_COUNT> SLOT_WEIGHTS{ 0.7f, 0.9f, 0.8f, 0.8f, 0.9f, 0.8f, 0.8f, 1.1f, 1.5f };

        // ===== 재료 표 =====
        constexpr std::size_t MATERIAL_COLUMN_COUNT{ static_cast<std::size_t>(RefineMaterialColumn::COLUMN_COUNT) };
        constexpr std::array<std::wstring_view, MATERIAL_COLUMN_COUNT> MATERIAL_HEADERS{ L"재료", L"가격", L"거래", L"예상 소모", L"예상 골드" };
        constexpr std::array<float, MATERIAL_COLUMN_COUNT> MATERIAL_WEIGHTS{ 2.0f, 0.9f, 0.8f, 1.1f, 1.1f };

        // ===== 경로 표 =====
        constexpr std::size_t ROUTE_COL_ORDER{ 0 };
        constexpr std::size_t ROUTE_COL_DESCRIPTION{ 1 };
        constexpr std::size_t ROUTE_COL_ITEM_LEVEL{ 2 };
        constexpr std::size_t ROUTE_COL_COST{ 3 };
        constexpr std::size_t ROUTE_COL_CUMULATIVE{ 4 };
        constexpr std::size_t ROUTE_COLUMN_COUNT{ 5 };
        constexpr std::array<std::wstring_view, ROUTE_COLUMN_COUNT> ROUTE_HEADERS{ L"단계", L"재련 내용", L"달성 아이템 레벨", L"골드", L"누적 골드" };
        constexpr std::array<float, ROUTE_COLUMN_COUNT> ROUTE_WEIGHTS{ 0.6f, 2.6f, 1.1f, 1.0f, 1.0f };

        // ===== 글자 =====
        constexpr std::wstring_view REFRESH_TEXT{ L"시세 갱신" };
        constexpr std::wstring_view AUTO_REFRESH_FORMAT{ L"{}분마다 자동 갱신" };
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"아직 조회하지 않았어요." };
        constexpr std::wstring_view START_PRESET_TEXT{ L"시작 프리셋" };
        constexpr std::wstring_view NAME_PLACEHOLDER{ L"캐릭터 이름" };   // 이름 입력 칸 안내 글자
        constexpr std::wstring_view LOOKUP_TEXT{ L"조회" };   // 조회 버튼 글자
        constexpr std::wstring_view LOOKUP_BUSY_TEXT{ L"조회 중..." };   // 조회 중 버튼 글자
        constexpr std::wstring_view TARGET_MODE_TEXT{ L"목표 기준" };
        constexpr std::wstring_view TARGET_LEVEL_TEXT{ L"목표 아이템 레벨" };
        constexpr std::wstring_view TARGET_LEVEL_PLACEHOLDER{ L"1750" };
        constexpr std::wstring_view CALCULATE_TEXT{ L"계산" };   // 계산 버튼 글자
        constexpr std::wstring_view CALCULATE_BUSY_TEXT{ L"계산 중..." };   // 계산 중 버튼 글자
        constexpr std::wstring_view SETUP_EYEBROW{ L"HONING TARGET" };   // 시작 → 목표 카드 영문 소제목
        constexpr std::wstring_view SETUP_TITLE{ L"시작 → 목표" };   // 시작 → 목표 카드 제목
        constexpr std::wstring_view SETUP_FOOTER_TEXT{ L"장비 칸을 누르면 에기르 ↔ 세르카, 숫자 칸은 직접 입력" };   // 부위 표 아래 안내
        constexpr std::wstring_view MATERIAL_EYEBROW{ L"MATERIAL COST" };   // 재료 카드 영문 소제목
        constexpr std::wstring_view MATERIAL_TITLE{ L"재료 가격 · 귀속 설정" };   // 재료 카드 제목
        constexpr std::wstring_view MATERIAL_FOOTER_TEXT{ L"거래 칸을 누르면 귀속 → 비용에서 빠짐" };   // 재료 표 아래 안내
        constexpr std::wstring_view ROUTE_EYEBROW{ L"HONING PATH" };   // 경로 카드 영문 소제목
        constexpr std::wstring_view ROUTE_TITLE{ L"재련 경로" };   // 경로 카드 제목
        constexpr std::wstring_view LEVEL_BADGE_TEXT{ L"Lv" };   // 달성 아이템 레벨 배지 글자
        constexpr std::size_t TARGET_LEVEL_MAX_LENGTH{ 4 };
        constexpr std::size_t NAME_MAX_LENGTH{ 20 };   // 캐릭터 이름 최대 글자 수 (전투력 시뮬레이터와 같음)

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };
        constexpr float SECTION_GAP{ 16.0f };
        constexpr float CALCULATE_BUTTON_WIDTH{ 92.0f };   // 계산 버튼 너비 ("계산 중..."이 들어가는 크기)
        constexpr float HEADER_RIGHT_LEFT_OFFSET{ 190.0f };   // 카드 머리말 오른쪽 글자가 시작할 수 있는 가장 왼쪽 (제목 오른쪽)
        constexpr float SLOT_ROW_HEIGHT{ 34.0f };   // 부위 표 행 높이 (세로 공간 절약)
        constexpr float GROUP_GAP{ 12.0f };
        constexpr float LABEL_GAP{ 6.0f };
        constexpr float REFRESH_BUTTON_WIDTH{ 110.0f };
        constexpr float AUTO_REFRESH_WIDTH{ 150.0f };
        constexpr float START_PRESET_LABEL_WIDTH{ 72.0f };
        constexpr float START_PRESET_BOX_WIDTH{ 240.0f };
        constexpr float NAME_BOX_WIDTH{ 150.0f };   // 캐릭터 이름 칸 너비
        constexpr float LOOKUP_BUTTON_WIDTH{ 84.0f };   // 조회 버튼 너비 ("조회 중..."이 들어가는 크기)
        constexpr float TARGET_MODE_LABEL_WIDTH{ 58.0f };
        constexpr float TARGET_MODE_BOX_WIDTH{ 120.0f };
        constexpr float TARGET_LEVEL_LABEL_WIDTH{ 104.0f };
        constexpr float TARGET_LEVEL_BOX_WIDTH{ 72.0f };
        constexpr float MATERIAL_PANEL_RATIO{ 0.42f };   // 아래 영역에서 재료 상자 너비 비율
        constexpr float MIN_GRID_HEIGHT{ 120.0f };

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };
        constexpr Core::TextStyle SETTING_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };
        constexpr Core::TextStyle TOTAL_TEXT_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };
        constexpr Core::TextStyle HEADER_RIGHT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };   // 카드 머리말 오른쪽 글자

        // 강화 · 상급 칸: 숫자만
        [[nodiscard]] bool IsDigitCharacter(wchar_t character) noexcept
        {
            return character >= L'0' && character <= L'9';
        }

        // 가격 칸: 숫자 · 소수점 · 쉼표
        [[nodiscard]] bool IsPriceCharacter(wchar_t character) noexcept
        {
            return IsDigitCharacter(character) || character == L'.' || character == L',';
        }

        template <std::size_t Count>
        [[nodiscard]] std::vector<UI::GridColumn> MakeColumns(const std::array<std::wstring_view, Count>& headers, const std::array<float, Count>& weights,
            std::size_t leadingColumn, std::size_t secondLeadingColumn)
        {
            std::vector<UI::GridColumn> columns;
            columns.reserve(Count);
            for (std::size_t column{}; column < Count; ++column)
            {
                const bool isLeading{ column == leadingColumn || column == secondLeadingColumn };
                columns.push_back(UI::GridColumn{ std::wstring{ headers[column] }, weights[column], isLeading ? Core::TextAlign::LEADING : Core::TextAlign::CENTER });
            }
            return columns;
        }

        [[nodiscard]] constexpr std::size_t ToColumn(RefineSlotColumn column) noexcept
        {
            return static_cast<std::size_t>(column);
        }

        [[nodiscard]] constexpr std::size_t ToColumn(RefineMaterialColumn column) noexcept
        {
            return static_cast<std::size_t>(column);
        }

        // 장비 단계 글자색 (에기르 = 고대 색, 세르카 = 강조 색)
        [[nodiscard]] const Core::Color& GetTierColor(bool isSerca) noexcept
        {
            return isSerca ? Theme::TOTAL_TEXT_COLOR : Theme::GRADE_ANCIENT_COLOR;
        }
    }

    RefineSimulatorView::RefineSimulatorView(Core::Platform& platform)
        : m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_autoRefreshBox{ m_host.Emplace<UI::CheckBox>(std::format(AUTO_REFRESH_FORMAT, Core::APPConstants::AUTO_REFRESH_MINUTES),
            [this](bool isChecked)
            {
                if (const auto onAutoRefreshChanged{ m_onAutoRefreshChanged }) onAutoRefreshChanged(isChecked);
            }) }
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }
        , m_startPresetLabel{ m_host.Emplace<UI::Label>(std::wstring{ START_PRESET_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_startPresetBox{ m_host.Emplace<UI::ComboBox>() }
        , m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }   // 이름 입력 칸 생성
        , m_lookupButton{ m_host.Emplace<UI::Button>(std::wstring{ LOOKUP_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }   // 조회 버튼 생성
        , m_targetModeLabel{ m_host.Emplace<UI::Label>(std::wstring{ TARGET_MODE_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_targetModeBox{ m_host.Emplace<UI::ComboBox>() }
        , m_targetLevelLabel{ m_host.Emplace<UI::Label>(std::wstring{ TARGET_LEVEL_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }
        , m_targetLevelBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ TARGET_LEVEL_PLACEHOLDER }) }
        , m_calculateButton{ m_host.Emplace<UI::Button>(std::wstring{ CALCULATE_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }   // 계산 버튼 생성
        , m_summaryLabel{ m_host.Emplace<UI::Label>(std::wstring{}, HEADER_RIGHT_STYLE, Theme::TEXT_COLOR) }
        , m_slotGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(SLOT_HEADERS, SLOT_WEIGHTS, ToColumn(RefineSlotColumn::COLUMN_SLOT), ToColumn(RefineSlotColumn::COLUMN_COST)), 0) }
        , m_materialGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(MATERIAL_HEADERS, MATERIAL_WEIGHTS, ToColumn(RefineMaterialColumn::COLUMN_NAME), ToColumn(RefineMaterialColumn::COLUMN_NAME)), 0) }
        , m_totalLabel{ m_host.Emplace<UI::Label>(std::wstring{}, TOTAL_TEXT_STYLE, Theme::HIGHLIGHT_TEXT_COLOR) }
        , m_routeGrid{ m_host.Emplace<UI::Grid>(platform, MakeColumns(ROUTE_HEADERS, ROUTE_WEIGHTS, ROUTE_COL_DESCRIPTION, ROUTE_COL_DESCRIPTION), 0) }
        , m_setupPanel{ Core::RectF{}, std::wstring{ SETUP_EYEBROW }, std::wstring{ SETUP_TITLE } }
        , m_materialPanel{ Core::RectF{}, std::wstring{ MATERIAL_EYEBROW }, std::wstring{ MATERIAL_TITLE } }
        , m_routePanel{ Core::RectF{}, std::wstring{ ROUTE_EYEBROW }, std::wstring{ ROUTE_TITLE } }
    {
        m_calculateButton.SetOnClick([this]   // 계산 버튼을 누르면 매니저에 알림
        {
            if (const auto onCalculate{ m_onCalculate }) onCalculate();
        });

        // 표 행 높이: 새 디자인 (부위 표는 세로 공간 때문에 조금 낮게)
        m_slotGrid.SetRowHeight(SLOT_ROW_HEIGHT);
        m_materialGrid.SetRowHeight(PageLayout::GRID_ROW_HEIGHT);
        m_routeGrid.SetRowHeight(PageLayout::GRID_ROW_HEIGHT);

        m_refreshButton.SetOnClick([this]
        {
            if (const auto onRefresh{ m_onRefresh }) onRefresh();
        });

        // 시작 프리셋 · 목표 기준: 고르면 매니저에 번호만 알림
        m_startPresetBox.SetOnSelectionChanged([this]
        {
            const std::optional<std::size_t> index{ m_startPresetBox.GetSelectedIndex() };
            if (const auto onStartPresetSelected{ m_onStartPresetSelected }; index.has_value() && onStartPresetSelected) onStartPresetSelected(*index);
        });
        m_targetModeBox.SetOnSelectionChanged([this]
        {
            const std::optional<std::size_t> index{ m_targetModeBox.GetSelectedIndex() };
            if (const auto onTargetModeSelected{ m_onTargetModeSelected }; index.has_value() && onTargetModeSelected) onTargetModeSelected(*index);
        });

        // 캐릭터 조회: 이름 칸에서 Enter 또는 [조회] 버튼
        m_nameBox.SetMaxLength(NAME_MAX_LENGTH);   // 이름 최대 글자 수 제한
        m_nameBox.SetOnSubmit([this] { RequestLookup(); });   // 이름 칸에서 Enter를 누르면 조회 요청
        m_lookupButton.SetOnClick([this] { RequestLookup(); });   // 조회 버튼을 누르면 조회 요청

        // 목표 아이템 레벨: 숫자만, 입력할 때마다 알림 (범위 확인은 매니저가)
        m_targetLevelBox.SetIMEAllowed(false);
        m_targetLevelBox.SetMaxLength(TARGET_LEVEL_MAX_LENGTH);
        m_targetLevelBox.SetInputFilter(IsDigitCharacter);
        m_targetLevelBox.SetOnTextChanged([this]
        {
            if (m_isUpdatingTargetText) return;
            if (const auto onTargetItemLevelChanged{ m_onTargetItemLevelChanged }) onTargetItemLevelChanged(m_targetLevelBox.GetText());
        });

        // 부위 표: 장비 칸은 누르기, 숫자 칸은 편집
        m_slotGrid.SetEditInput(false, IsDigitCharacter);
        m_slotGrid.SetOnCellClicked([this](std::size_t row, std::size_t column)
        {
            if (const auto onSlotCellClicked{ m_onSlotCellClicked }) onSlotCellClicked(row, static_cast<RefineSlotColumn>(column));
        });
        m_slotGrid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text)
        {
            if (const auto onSlotCellEdited{ m_onSlotCellEdited }) onSlotCellEdited(row, static_cast<RefineSlotColumn>(column), text);
        });

        // 재료 표: 거래 칸은 누르기, 가격 칸은 편집
        m_materialGrid.SetEditInput(false, IsPriceCharacter);
        m_materialGrid.SetOnCellClicked([this](std::size_t row, std::size_t column)
        {
            if (const auto onMaterialCellClicked{ m_onMaterialCellClicked }) onMaterialCellClicked(row, static_cast<RefineMaterialColumn>(column));
        });
        m_materialGrid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text)
        {
            if (const auto onMaterialCellEdited{ m_onMaterialCellEdited }) onMaterialCellEdited(row, static_cast<RefineMaterialColumn>(column), text);
        });

        // 경로 표: 읽기 전용, 행 단위 선택
        m_routeGrid.SetSelectionMode(UI::GridSelectionMode::SELECT_ROW);
    }

    RefineSimulatorView::~RefineSimulatorView() = default;

    void RefineSimulatorView::SetOnRefresh(std::function<void()> onRefresh)
    {
        m_onRefresh = std::move(onRefresh);
    }

    void RefineSimulatorView::SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged)
    {
        m_onAutoRefreshChanged = std::move(onAutoRefreshChanged);
    }

    void RefineSimulatorView::SetOnStartPresetSelected(std::function<void(std::size_t presetIndex)> onStartPresetSelected)
    {
        m_onStartPresetSelected = std::move(onStartPresetSelected);
    }

    void RefineSimulatorView::SetOnLookup(std::function<void(const std::wstring& characterName)> onLookup)
    {
        m_onLookup = std::move(onLookup);   // 조회 알림 연결
    }

    void RefineSimulatorView::SetOnCalculate(std::function<void()> onCalculate)
    {
        m_onCalculate = std::move(onCalculate);   // 계산 알림 연결
    }

    void RefineSimulatorView::SetOnTargetModeSelected(std::function<void(std::size_t modeIndex)> onTargetModeSelected)
    {
        m_onTargetModeSelected = std::move(onTargetModeSelected);
    }

    void RefineSimulatorView::SetOnTargetItemLevelChanged(std::function<void(const std::wstring& text)> onTargetItemLevelChanged)
    {
        m_onTargetItemLevelChanged = std::move(onTargetItemLevelChanged);
    }

    void RefineSimulatorView::SetOnSlotCellClicked(std::function<void(std::size_t row, RefineSlotColumn column)> onSlotCellClicked)
    {
        m_onSlotCellClicked = std::move(onSlotCellClicked);
    }

    void RefineSimulatorView::SetOnSlotCellEdited(std::function<void(std::size_t row, RefineSlotColumn column, const std::wstring& text)> onSlotCellEdited)
    {
        m_onSlotCellEdited = std::move(onSlotCellEdited);
    }

    void RefineSimulatorView::SetOnMaterialCellClicked(std::function<void(std::size_t row, RefineMaterialColumn column)> onMaterialCellClicked)
    {
        m_onMaterialCellClicked = std::move(onMaterialCellClicked);
    }

    void RefineSimulatorView::SetOnMaterialCellEdited(std::function<void(std::size_t row, RefineMaterialColumn column, const std::wstring& text)> onMaterialCellEdited)
    {
        m_onMaterialCellEdited = std::move(onMaterialCellEdited);
    }

    void RefineSimulatorView::SetAutoRefreshChecked(bool isChecked) noexcept
    {
        m_autoRefreshBox.SetChecked(isChecked);
    }

    bool RefineSimulatorView::IsAutoRefreshChecked() const noexcept
    {
        return m_autoRefreshBox.IsChecked();
    }

    void RefineSimulatorView::SetRefreshEnabled(bool isEnabled) noexcept
    {
        m_refreshButton.SetEnabled(isEnabled);
    }

    void RefineSimulatorView::SetStatusText(std::wstring text)
    {
        m_statusLabel.SetText(std::move(text));
    }

    void RefineSimulatorView::SetStartPresetItems(std::vector<std::wstring> items)
    {
        m_startPresetBox.SetItems(std::move(items));
    }

    void RefineSimulatorView::SetStartPresetIndex(std::optional<std::size_t> index)
    {
        m_startPresetBox.SetSelectedIndex(index);
    }

    void RefineSimulatorView::SetCharacterName(std::wstring name)
    {
        m_nameBox.SetText(std::move(name));   // 이름 입력 칸에 글자 반영
    }

    void RefineSimulatorView::SetLookupBusy(bool isBusy)
    {
        m_lookupButton.SetEnabled(!isBusy);   // 조회 중에는 버튼 비활성
        m_lookupButton.SetText(std::wstring{ isBusy ? LOOKUP_BUSY_TEXT : LOOKUP_TEXT });   // 조회 중이면 "조회 중..." 글자로 바꿈
    }

    void RefineSimulatorView::SetCalculateBusy(bool isBusy)
    {
        m_calculateButton.SetEnabled(!isBusy);   // 계산 중에는 버튼 비활성
        m_calculateButton.SetText(std::wstring{ isBusy ? CALCULATE_BUSY_TEXT : CALCULATE_TEXT });   // 계산 중이면 "계산 중..." 글자로 바꿈
    }

    void RefineSimulatorView::SetTargetModeItems(std::vector<std::wstring> items)
    {
        m_targetModeBox.SetItems(std::move(items));
    }

    void RefineSimulatorView::SetTargetModeIndex(std::size_t index)
    {
        m_targetModeBox.SetSelectedIndex(index);
    }

    void RefineSimulatorView::SetTargetItemLevelText(std::wstring text)
    {
        if (m_targetLevelBox.GetText() == text) return;
        m_isUpdatingTargetText = true;
        m_targetLevelBox.SetText(std::move(text));
        m_isUpdatingTargetText = false;
    }

    void RefineSimulatorView::SetTargetItemLevelEnabled(bool isEnabled) noexcept
    {
        m_targetLevelBox.SetEnabled(isEnabled);
    }

    void RefineSimulatorView::SetSummaryText(std::wstring text, bool isError)
    {
        m_summaryLabel.SetText(std::move(text));
        m_summaryLabel.SetColor(isError ? Theme::ALERT_TEXT_COLOR : Theme::TEXT_COLOR);
    }

    // 부위 표 다시 채우기 (편집 가능한 칸 = 일반 칸 색, 읽기 전용 = 어두운 칸, 장비 칸 = 단계 색)
    void RefineSimulatorView::ShowSlots(const std::vector<RefineSlotRow>& rows)
    {
        const UI::GridCellStyle editableStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR };
        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::MUTED_TEXT_COLOR };

        m_slotGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const RefineSlotRow& slot{ rows[row] };
            const auto setCell{ [this, row](RefineSlotColumn column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)
            {
                m_slotGrid.SetCellText(row, ToColumn(column), text);
                m_slotGrid.SetCellStyle(row, ToColumn(column), style);
                m_slotGrid.SetCellEditable(row, ToColumn(column), isEditable);
            } };

            setCell(RefineSlotColumn::COLUMN_SLOT, slot.m_slotName, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD }, false);
            setCell(RefineSlotColumn::COLUMN_START_TIER, slot.m_startTier, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, GetTierColor(slot.m_isStartSerca), Core::FontWeight::BOLD, UI::GridDecoration::DECORATION_CHIP, std::wstring{}, GetTierColor(slot.m_isStartSerca) }, false);   // 장비 단계 칩
            setCell(RefineSlotColumn::COLUMN_START_ENHANCEMENT, slot.m_startEnhancement, editableStyle, true);
            setCell(RefineSlotColumn::COLUMN_START_ADVANCED, slot.m_startAdvanced, slot.m_isStartAdvancedEditable ? editableStyle : readonlyStyle, slot.m_isStartAdvancedEditable);

            const UI::GridCellStyle targetTierStyle{ slot.m_isTargetEditable ? Theme::CELL_BACK_COLOR : Theme::READONLY_BACK_COLOR, GetTierColor(slot.m_isTargetSerca), Core::FontWeight::BOLD,
                UI::GridDecoration::DECORATION_CHIP, std::wstring{}, GetTierColor(slot.m_isTargetSerca) };   // 목표 장비 단계 칩
            const UI::GridCellStyle targetStyle{ slot.m_isTargetEditable ? editableStyle : UI::GridCellStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR } };
            const bool isTargetAdvancedEditable{ slot.m_isTargetEditable && slot.m_isTargetAdvancedEditable };
            setCell(RefineSlotColumn::COLUMN_TARGET_TIER, slot.m_targetTier, targetTierStyle, false);
            setCell(RefineSlotColumn::COLUMN_TARGET_ENHANCEMENT, slot.m_targetEnhancement, targetStyle, slot.m_isTargetEditable);
            setCell(RefineSlotColumn::COLUMN_TARGET_ADVANCED, slot.m_targetAdvanced, isTargetAdvancedEditable ? editableStyle : readonlyStyle, isTargetAdvancedEditable);

            const UI::GridCellStyle resultStyle{ Theme::READONLY_BACK_COLOR, slot.m_hasChange ? Theme::UP_TEXT_COLOR : Theme::MUTED_TEXT_COLOR, slot.m_hasChange ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL };
            setCell(RefineSlotColumn::COLUMN_CHANGE, slot.m_change, resultStyle, false);
            setCell(RefineSlotColumn::COLUMN_COST, slot.m_cost, UI::GridCellStyle{ Theme::READONLY_BACK_COLOR, slot.m_hasChange ? Theme::TEXT_COLOR : Theme::MUTED_TEXT_COLOR }, false);
        }
    }

    // 재료 표 다시 채우기 (가격 = 편집 칸, 거래 칸 = 귀속이면 강조)
    void RefineSimulatorView::ShowMaterials(const std::vector<RefineMaterialRow>& rows)
    {
        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };

        m_materialGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const RefineMaterialRow& material{ rows[row] };
            const auto setCell{ [this, row](RefineMaterialColumn column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)
            {
                m_materialGrid.SetCellText(row, ToColumn(column), text);
                m_materialGrid.SetCellStyle(row, ToColumn(column), style);
                m_materialGrid.SetCellEditable(row, ToColumn(column), isEditable);
            } };

            setCell(RefineMaterialColumn::COLUMN_NAME, material.m_name, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR }, false);
            setCell(RefineMaterialColumn::COLUMN_PRICE, material.m_price,
                material.m_isPriceEditable ? UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR } : readonlyStyle, material.m_isPriceEditable);
            setCell(RefineMaterialColumn::COLUMN_TRADE, material.m_trade,
                UI::GridCellStyle{ Theme::CELL_BACK_COLOR, material.m_isBound ? Theme::CAUTION_TEXT_COLOR : Theme::NOTICE_SUCCESS_COLOR, Core::FontWeight::BOLD,
                    UI::GridDecoration::DECORATION_CHIP, std::wstring{}, material.m_isBound ? Theme::CAUTION_TEXT_COLOR : Theme::NOTICE_SUCCESS_COLOR }, false);   // 거래 · 귀속 칩
            setCell(RefineMaterialColumn::COLUMN_AMOUNT, material.m_amount, readonlyStyle, false);
            setCell(RefineMaterialColumn::COLUMN_COST, material.m_cost, UI::GridCellStyle{ Theme::READONLY_BACK_COLOR, Theme::HIGHLIGHT_TEXT_COLOR }, false);
        }
    }

    // 총 소요 재화 + 경로 표 (계승 행 = 강조색, 목표를 넘긴 행 = 도달색)
    void RefineSimulatorView::ShowRoute(std::wstring totalText, const std::vector<RefineRouteRow>& rows)
    {
        m_totalLabel.SetText(std::move(totalText));

        const bool hasReachedRow{ std::ranges::any_of(rows, [](const RefineRouteRow& route) { return route.m_isReached; }) };   // 목표를 넘긴 행이 있는지
        m_routeGrid.SetRowCount(rows.size());
        for (std::size_t row{}; row < rows.size(); ++row)
        {
            const RefineRouteRow& route{ rows[row] };
            const bool isHighlighted{ route.m_isReached || (!hasReachedRow && row + 1 == rows.size()) };   // 도달 행(없으면 마지막 걸음)을 강조
            const Core::Color backColor{ isHighlighted ? Theme::REACHED_BACK_COLOR : Theme::CELL_BACK_COLOR };
            const Core::Color textColor{ route.m_isInherit ? Theme::TOTAL_TEXT_COLOR : (isHighlighted ? Theme::REACHED_TEXT_COLOR : Theme::TEXT_COLOR) };

            m_routeGrid.SetCellText(row, ROUTE_COL_ORDER, route.m_order);
            m_routeGrid.SetCellText(row, ROUTE_COL_DESCRIPTION, route.m_description);
            m_routeGrid.SetCellText(row, ROUTE_COL_ITEM_LEVEL, route.m_itemLevel);
            m_routeGrid.SetCellText(row, ROUTE_COL_COST, route.m_cost);
            m_routeGrid.SetCellText(row, ROUTE_COL_CUMULATIVE, route.m_cumulative);
            for (std::size_t column{}; column < ROUTE_COLUMN_COUNT; ++column)
            {
                const bool isStrong{ column == ROUTE_COL_DESCRIPTION || column == ROUTE_COL_CUMULATIVE };
                UI::GridCellStyle style{ backColor,
                    column == ROUTE_COL_CUMULATIVE ? Theme::HIGHLIGHT_TEXT_COLOR : textColor, isStrong ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL };
                if (column == ROUTE_COL_ITEM_LEVEL)   // 달성 아이템 레벨 앞에 Lv 배지
                {
                    style.m_decoration = UI::GridDecoration::DECORATION_BADGE;
                    style.m_badgeText = std::wstring{ LEVEL_BADGE_TEXT };
                    style.m_accentColor = Theme::ACCENT_COLOR;
                }
                m_routeGrid.SetCellStyle(row, column, style);
            }
        }
    }

    void RefineSimulatorView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);
    }

    void RefineSimulatorView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);
        RenderPanels(renderer);
        m_host.Render(renderer);
    }

    bool RefineSimulatorView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);
    }

    Core::CursorShape RefineSimulatorView::GetCursor() const noexcept
    {
        return m_host.GetCursor();
    }

    void RefineSimulatorView::SuspendFocus()
    {
        m_host.SuspendFocus();
    }

    void RefineSimulatorView::RestoreFocus()
    {
        m_host.RestoreFocus();
    }

    // 상단 바 / 시작 → 목표 상자(설정 줄 + 부위 표) / (왼쪽 재료 상자 | 오른쪽 경로 상자)
    void RefineSimulatorView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;
        m_layoutArea = contentArea;
        m_hasLayout = true;

        const float left{ contentArea.m_left + PAGE_PADDING };
        const float right{ contentArea.m_right - PAGE_PADDING };
        const float barTop{ contentArea.m_top + PAGE_PADDING };
        const float barBottom{ barTop + Theme::CONTROL_HEIGHT };

        // 줄 하나에 왼쪽부터 차례로 배치
        const auto placeRow{ [](float& x, float top, UI::Control& control, float width, float gapAfter)
        {
            control.SetBounds(Core::RectF{ x, top, x + width, top + Theme::CONTROL_HEIGHT });
            x += width + gapAfter;
        } };

        // 상단 바: [시세 갱신] [5분마다 자동 갱신] 상태
        float x{ left };
        placeRow(x, barTop, m_refreshButton, REFRESH_BUTTON_WIDTH, GROUP_GAP);
        placeRow(x, barTop, m_autoRefreshBox, AUTO_REFRESH_WIDTH, SECTION_GAP);
        m_statusLabel.SetBounds(Core::RectF{ x, barTop, std::max(x, right), barBottom });

        // 시작 → 목표 카드: 머리말 + 설정 줄 + 부위 표 + 안내 줄 (행이 모두 보이는 높이)
        const float setupTop{ barBottom + PageLayout::CARD_GAP };
        const float setupHeight{ PageLayout::CARD_HEADER_HEIGHT + GROUP_GAP + Theme::CONTROL_HEIGHT + GROUP_GAP + m_slotGrid.GetPreferredHeight() + PageLayout::TABLE_FOOTER_HEIGHT };
        const Core::RectF setupInner{ LayoutPanel(m_setupPanel, Core::RectF{ left, setupTop, right, setupTop + setupHeight }) };

        x = setupInner.m_left;
        const float settingTop{ setupInner.m_top };
        placeRow(x, settingTop, m_startPresetLabel, START_PRESET_LABEL_WIDTH, LABEL_GAP);
        placeRow(x, settingTop, m_startPresetBox, START_PRESET_BOX_WIDTH, LABEL_GAP);   // 프리셋 바로 옆에 이름 칸 · 조회 버튼
        placeRow(x, settingTop, m_nameBox, NAME_BOX_WIDTH, LABEL_GAP);   // 캐릭터 이름 칸 배치
        placeRow(x, settingTop, m_lookupButton, LOOKUP_BUTTON_WIDTH, SECTION_GAP);   // 조회 버튼 배치
        placeRow(x, settingTop, m_targetModeLabel, TARGET_MODE_LABEL_WIDTH, LABEL_GAP);
        placeRow(x, settingTop, m_targetModeBox, TARGET_MODE_BOX_WIDTH, SECTION_GAP);
        placeRow(x, settingTop, m_targetLevelLabel, TARGET_LEVEL_LABEL_WIDTH, LABEL_GAP);
        placeRow(x, settingTop, m_targetLevelBox, TARGET_LEVEL_BOX_WIDTH, SECTION_GAP);
        placeRow(x, settingTop, m_calculateButton, CALCULATE_BUTTON_WIDTH, SECTION_GAP);   // 계산 버튼 배치
        m_summaryLabel.SetBounds(Core::RectF{ m_setupPanel.m_bounds.m_left + HEADER_RIGHT_LEFT_OFFSET, m_setupPanel.m_bounds.m_top,
            m_setupPanel.m_bounds.m_right - PageLayout::CARD_PADDING, m_setupPanel.m_bounds.m_top + PageLayout::CARD_HEADER_HEIGHT });   // 요약은 카드 머리말 오른쪽에

        const float slotTop{ settingTop + Theme::CONTROL_HEIGHT + GROUP_GAP };
        m_slotGrid.SetBounds(Core::RectF{ setupInner.m_left, slotTop, setupInner.m_right, std::max(slotTop, setupInner.m_bottom) });

        // 아래: 재료 카드 | 경로 카드
        const float bodyTop{ m_setupPanel.m_bounds.m_bottom + PageLayout::CARD_GAP };
        const float bodyBottom{ std::max(bodyTop + MIN_GRID_HEIGHT, contentArea.m_bottom - PAGE_PADDING) };
        const float middle{ left + (right - left - PageLayout::CARD_GAP) * MATERIAL_PANEL_RATIO };
        m_materialGrid.SetBounds(LayoutPanel(m_materialPanel, Core::RectF{ left, bodyTop, middle, bodyBottom }));

        const Core::RectF routeInner{ LayoutPanel(m_routePanel, Core::RectF{ middle + PageLayout::CARD_GAP, bodyTop, right, bodyBottom }) };
        m_totalLabel.SetBounds(Core::RectF{ m_routePanel.m_bounds.m_left + HEADER_RIGHT_LEFT_OFFSET, m_routePanel.m_bounds.m_top,
            m_routePanel.m_bounds.m_right - PageLayout::CARD_PADDING, m_routePanel.m_bounds.m_top + PageLayout::CARD_HEADER_HEIGHT });   // 총 골드는 카드 머리말 오른쪽에
        m_routeGrid.SetBounds(Core::RectF{ routeInner.m_left, routeInner.m_top, routeInner.m_right, std::max(routeInner.m_top, routeInner.m_bottom + PageLayout::TABLE_FOOTER_HEIGHT - GROUP_GAP) });   // 경로 표는 아래 안내 줄 없이 카드 끝까지
    }


    void RefineSimulatorView::RequestLookup()
    {
        m_nameBox.CommitComposition();   // 조합 중인 한글 마지막 글자까지 포함
        if (const auto onLookup{ m_onLookup }) onLookup(m_nameBox.GetText());   // 등록된 콜백이 있으면 이름을 알림
    }

    // 카드 본문 영역: 머리말 · 위 여백 · 아래 안내 줄 자리를 뺀 안쪽
    Core::RectF RefineSimulatorView::LayoutPanel(Panel& panel, const Core::RectF& bounds) const
    {
        panel.m_bounds = bounds;
        const float top{ bounds.m_top + PageLayout::CARD_HEADER_HEIGHT + GROUP_GAP };   // 머리말 + 위 여백 아래
        return Core::RectF{ bounds.m_left + PageLayout::CARD_PADDING, top,
            bounds.m_right - PageLayout::CARD_PADDING, std::max(top, bounds.m_bottom - PageLayout::TABLE_FOOTER_HEIGHT) };
    }

    // 카드: 피그마 머리말(영문 소제목 + 제목) + 표 아래 안내 줄 (경로 카드는 안내 줄 없음)
    void RefineSimulatorView::RenderPanels(Core::Renderer& renderer) const
    {
        for (const Panel* panel : { &m_setupPanel, &m_materialPanel, &m_routePanel })
        {
            PageLayout::DrawCardWithHeader(renderer, panel->m_bounds, panel->m_eyebrow, panel->m_title);   // 카드 + 머리말
        }

        const auto drawFooter{ [&renderer](const Panel& panel, std::wstring_view text)
        {
            PageLayout::DrawTableFooter(renderer, Core::RectF{ panel.m_bounds.m_left, panel.m_bounds.m_bottom - PageLayout::TABLE_FOOTER_HEIGHT,
                panel.m_bounds.m_right, panel.m_bounds.m_bottom }, text, std::wstring_view{});   // 카드 맨 아래 안내 줄
        } };
        drawFooter(m_setupPanel, SETUP_FOOTER_TEXT);   // 부위 표 사용법
        drawFooter(m_materialPanel, MATERIAL_FOOTER_TEXT);   // 재료 표 사용법
    }
}
