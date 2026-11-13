module LostArkMultiTool.Views.UI.Grid;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;
import LostArkMultiTool.Views.UI.TextBox;

namespace LostArkMultiTool::Views::UI
{
    namespace
    {
        constexpr float CELL_PADDING{ 8.0f };            // 칸 안 글자 좌우 여백
        constexpr float BADGE_SIZE_RATIO{ 0.62f };          // 배지 한 변 = 행 높이 × 비율
        constexpr float BADGE_RADIUS{ 5.0f };               // 배지 모서리
        constexpr float BADGE_GAP{ 10.0f };                 // 배지와 글자 사이
        constexpr float BADGE_BACK_ALPHA{ 0.18f };          // 배지 바탕 = 강조색 연하게
        constexpr float CHIP_PADDING_X{ 8.0f };             // 칩 좌우 안쪽 여백
        constexpr float CHIP_HEIGHT_RATIO{ 0.56f };         // 칩 높이 = 행 높이 × 비율
        constexpr float CHIP_RADIUS{ 4.0f };                // 칩 모서리
        constexpr float CHIP_BACK_ALPHA{ 0.10f };           // 칩 바탕 투명도
        constexpr float SPARKLINE_HEIGHT_RATIO{ 0.42f };    // 추이 선 높이 = 행 높이 × 비율
        constexpr float SPARKLINE_MAX_WIDTH{ 64.0f };       // 추이 선 최대 너비
        constexpr float SPARKLINE_STROKE{ 1.5f };           // 추이 선 두께
        constexpr std::size_t MIN_SPARKLINE_POINTS{ 2 };    // 선을 그릴 최소 점 수
        constexpr float HALF_RATIO{ 0.5f };                 // 가운데 계산
        constexpr float EDITOR_INSET{ 1.0f };            // 편집 입력칸을 칸 테두리 안쪽으로
        constexpr float HOVER_ALPHA{ 0.35f };            // 편집 가능한 칸에 마우스를 올렸을 때 강조 투명도
        constexpr float PIXEL_CENTER{ 0.5f };            // 1픽셀 선을 픽셀 가운데에 그려 번지지 않게
        constexpr float ROW_FIT_TOLERANCE{ 0.01f };      // 표 높이가 행 높이의 정확한 배수일 때 반올림 오차 허용
        constexpr float SCROLLBAR_WIDTH{ 3.0f };
        constexpr float SCROLLBAR_MARGIN{ 2.0f };
        constexpr float HALF{ 0.5f };
        constexpr float BOTH_SIDES{ 2.0f };
        constexpr float WHEEL_ROWS_PER_NOTCH{ 3.0f };    // 휠 한 칸에 넘기는 행 수
        constexpr int PREVIOUS_STEP{ -1 };
        constexpr int NEXT_STEP{ 1 };
        constexpr int NO_STEP{ 0 };
        constexpr std::size_t FIRST_COLUMN{ 0 };

        constexpr Core::TextStyle HEADER_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::CENTER };

        [[nodiscard]] float SnapToPixel(float position) noexcept
        {
            return std::floor(position) + PIXEL_CENTER;
        }
    }

    Grid::Grid(Core::Platform& platform, std::vector<GridColumn> columns, std::size_t rowCount)
        : m_platform{ platform }
        , m_columns{ std::move(columns) }
        , m_rowCount{ rowCount }
        , m_cells(m_rowCount* m_columns.size(), MakeDefaultCell())
        , m_editor{ platform, std::wstring{} }
    {
        for (const GridColumn& column : m_columns) m_totalWeight += std::max(column.m_widthWeight, 0.0f);
    }

    Grid::~Grid() = default;

    std::size_t Grid::GetRowCount() const noexcept
    {
        return m_rowCount;
    }

    std::size_t Grid::GetColumnCount() const noexcept
    {
        return m_columns.size();
    }

    void Grid::SetRowCount(std::size_t rowCount)
    {
        if (rowCount == m_rowCount) return;

        CancelEdit();   // 편집 중인 칸이 사라질 수 있으므로 먼저 취소
        m_cells.resize(rowCount * m_columns.size(), MakeDefaultCell());
        m_rowCount = rowCount;

        if (m_selectedCell.has_value() && m_selectedCell->m_row >= m_rowCount) m_selectedCell.reset();
        if (m_hoverCell.has_value() && m_hoverCell->m_row >= m_rowCount) m_hoverCell.reset();
        m_firstVisibleRow = std::min(m_firstVisibleRow, GetMaxFirstRow());
    }

    void Grid::SetHeaderHeight(float headerHeight) noexcept
    {
        m_headerHeight = std::max(headerHeight, 0.0f);
    }

    void Grid::SetColumns(std::vector<GridColumn> columns)
    {
        CancelEdit();   // 편집 중인 칸이 사라질 수 있으므로 먼저 취소

        m_columns = std::move(columns);
        m_totalWeight = 0.0f;
        for (const GridColumn& column : m_columns) m_totalWeight += std::max(column.m_widthWeight, 0.0f);

        m_cells.assign(m_rowCount * m_columns.size(), MakeDefaultCell());
        m_selectedCell.reset();
        m_hoverCell.reset();
    }

    void Grid::SetCellText(std::size_t row, std::size_t column, std::wstring text)
    {
        Cell& cell{ GetCell(row, column) };
        cell.m_text = std::move(text);
        cell.m_editText.clear();   // 표시 글자가 바뀌면 이전 편집용 글자는 맞지 않음
    }

    void Grid::SetCellEditText(std::size_t row, std::size_t column, std::wstring editText)
    {
        GetCell(row, column).m_editText = std::move(editText);
    }

    const std::wstring& Grid::GetCellText(std::size_t row, std::size_t column) const
    {
        return GetCell(row, column).m_text;
    }

    void Grid::SetCellStyle(std::size_t row, std::size_t column, const GridCellStyle& style)
    {
        GetCell(row, column).m_style = style;
    }

    void Grid::SetCellEditable(std::size_t row, std::size_t column, bool isEditable)
    {
        GetCell(row, column).m_isEditable = isEditable;

        // 편집 중인 칸이 잠기면 편집 취소 (잠긴 칸에 값이 들어가지 않게)
        if (!isEditable && m_editingCell == CellPosition{ row, column }) CancelEdit();
    }

    void Grid::SetEditInput(bool isIMEAllowed, std::function<bool(wchar_t character)> inputFilter)
    {
        m_editor.SetIMEAllowed(isIMEAllowed);
        m_editor.SetInputFilter(inputFilter);
        m_inputFilter = std::move(inputFilter);
    }

    void Grid::SetOnCellEdited(std::function<void(std::size_t row, std::size_t column, const std::wstring& text)> onCellEdited)
    {
        m_onCellEdited = std::move(onCellEdited);
    }

    void Grid::SetSelectionMode(GridSelectionMode mode) noexcept
    {
        m_selectionMode = mode;
    }

    void Grid::SetSelectionAlwaysVisible(bool isAlwaysVisible) noexcept
    {
        m_isSelectionAlwaysVisible = isAlwaysVisible;
    }

    void Grid::SetOnCellClicked(std::function<void(std::size_t row, std::size_t column)> onCellClicked)
    {
        m_onCellClicked = std::move(onCellClicked);
    }

    void Grid::SetOnSelectionChanged(std::function<void()> onSelectionChanged)
    {
        m_onSelectionChanged = std::move(onSelectionChanged);
    }

    std::optional<std::size_t> Grid::GetSelectedRow() const noexcept
    {
        if (!m_selectedCell.has_value()) return std::nullopt;
        return m_selectedCell->m_row;
    }

    void Grid::SelectRow(std::optional<std::size_t> row)
    {
        if (!row.has_value() || *row >= m_rowCount)
        {
            m_selectedCell.reset();
            return;
        }

        m_selectedCell = CellPosition{ *row, FIRST_COLUMN };
        EnsureRowVisible(*row);
    }

    void Grid::SelectCell(std::size_t row, std::size_t column)
    {
        if (row >= m_rowCount || column >= m_columns.size()) return;   // 경계 검사

        m_selectedCell = CellPosition{ row, column };
        EnsureRowVisible(row);
    }

    void Grid::CommitEdit()
    {
        if (!m_editingCell.has_value()) return;

        m_editor.CommitComposition();   // 조합 중인 글자까지 포함
        const CellPosition position{ *m_editingCell };
        std::wstring text{ m_editor.GetText() };
        EndEdit();

        Cell& cell{ GetCell(position.m_row, position.m_column) };
        if (text == GetEditSource(cell)) return;   // 바뀐 게 없으면 알리지 않음 (편집을 시작할 때 채운 글자와 비교)

        cell.m_text = text;   // 받는 쪽이 형식을 맞춰 다시 쓸 때까지 입력한 그대로 표시
        cell.m_editText.clear();
        if (!m_onCellEdited) return;

        const auto onCellEdited{ m_onCellEdited };   // 콜백 안에서 표를 다시 설정해도 안전하게
        onCellEdited(position.m_row, position.m_column, text);
    }

    void Grid::CancelEdit()
    {
        EndEdit();
    }

    float Grid::GetPreferredHeight() const noexcept
    {
        return m_headerHeight + m_rowHeight * static_cast<float>(m_rowCount);
    }

    bool Grid::IsFocusable() const noexcept
    {
        return true;
    }

    Core::CursorShape Grid::GetCursor() const noexcept
    {
        // 편집 가능한 칸 위 = 글자 입력 커서
        const bool isEditableHover{ m_hoverCell.has_value() && m_hoverCell->m_row < m_rowCount && m_hoverCell->m_column < m_columns.size()
            && m_cells[m_hoverCell->m_row * m_columns.size() + m_hoverCell->m_column].m_isEditable };
        return isEditableHover ? Core::CursorShape::CURSOR_TEXT : Core::CursorShape::CURSOR_ARROW;
    }

    void Grid::Update(double deltaSeconds)
    {
        if (m_editingCell.has_value()) m_editor.Update(deltaSeconds);
    }

    void Grid::Render(Core::Renderer& renderer)
    {
        // 표 크기가 바뀌어 보이는 행 수가 달라졌으면 스크롤 위치를 범위 안으로
        m_firstVisibleRow = std::min(m_firstVisibleRow, GetMaxFirstRow());

        RenderCells(renderer);
        RenderHeader(renderer);
        RenderGridLines(renderer);
        RenderScrollBar(renderer);

        if (!m_editingCell.has_value() || !IsRowVisible(m_editingCell->m_row)) return;

        // 표가 움직여도(창 크기 변경) 편집 칸이 따라가게 매번 위치를 맞춤
        const Core::RectF cellRect{ GetCellRect(m_editingCell->m_row, m_editingCell->m_column) };
        m_editor.SetBounds(Core::RectF{ cellRect.m_left + EDITOR_INSET, cellRect.m_top + EDITOR_INSET,
            cellRect.m_right - EDITOR_INSET, cellRect.m_bottom - EDITOR_INSET });
        m_editor.Render(renderer);
    }

    void Grid::OnMouseDown(const Core::MouseButtonEvent& event)
    {
        // 편집 칸 안: 글자 커서 · 선택 (더블 클릭 = 전체 선택)
        if (m_editingCell.has_value() && m_editor.HitTest(event.m_x, event.m_y))
        {
            m_isEditorPressed = true;
            m_editor.OnMouseDown(event);
            return;
        }

        CommitEdit();

        const std::optional<CellPosition> cell{ FindCellAt(event.m_x, event.m_y) };
        if (!cell.has_value()) return;   // 머리글 · 빈 곳

        // 편집 가능한 칸은 누르자마자 편집 (전체 선택 → 바로 입력하면 교체)
        SetSelectedCell(cell, true);
        if (IsCellEditable(cell)) BeginEdit(true);

        if (!m_onCellClicked) return;
        const auto onCellClicked{ m_onCellClicked };   // 콜백 안에서 표를 다시 설정해도 안전하게
        onCellClicked(cell->m_row, cell->m_column);
    }

    void Grid::OnMouseUp(const Core::MouseButtonEvent& event, bool isInside)
    {
        if (!m_isEditorPressed) return;

        m_isEditorPressed = false;
        if (m_editingCell.has_value()) m_editor.OnMouseUp(event, isInside);
    }

    void Grid::OnMouseDrag(const Core::MouseMoveEvent& event)
    {
        if (m_isEditorPressed && m_editingCell.has_value()) m_editor.OnMouseDrag(event);
    }

    void Grid::OnMouseMove(const Core::MouseMoveEvent& event)
    {
        m_hoverCell = FindCellAt(event.m_x, event.m_y);
    }

    bool Grid::OnMouseWheel(const Core::MouseWheelEvent& event)
    {
        if (GetMaxFirstRow() == 0) return false;   // 모든 행이 보이면 스크롤할 것이 없음

        // 휠을 위로 = 위쪽 행 (부호 있는 계산 후 범위 안으로)
        const auto step{ static_cast<long long>(std::lround(event.m_notches * WHEEL_ROWS_PER_NOTCH)) };
        const auto target{ std::clamp(static_cast<long long>(m_firstVisibleRow) - step, 0LL, static_cast<long long>(GetMaxFirstRow())) };
        ScrollTo(static_cast<std::size_t>(target));
        return true;
    }

    bool Grid::OnKeyDown(const Core::KeyEvent& event)
    {
        if (m_editingCell.has_value())
        {
            switch (event.m_key)
            {
                case Core::Key::KEY_ENTER:
                    CommitEdit();
                    return true;

                case Core::Key::KEY_ESCAPE:
                    CancelEdit();
                    return true;

                case Core::Key::KEY_TAB:
                    CommitEdit();
                    return false;   // 다음 부품으로 포커스 이동은 ControlHost가

                case Core::Key::KEY_UP:
                case Core::Key::KEY_DOWN:
                    CommitEdit();
                    MoveSelection(event.m_key == Core::Key::KEY_UP ? PREVIOUS_STEP : NEXT_STEP, NO_STEP);
                    return true;

                default:
                    return m_editor.OnKeyDown(event);   // ← → Home End Backspace Delete Ctrl+A · C · X · V
            }
        }

        const bool isEditable{ IsCellEditable(m_selectedCell) };
        const bool isRowMode{ m_selectionMode == GridSelectionMode::SELECT_ROW };
        switch (event.m_key)
        {
            case Core::Key::KEY_UP:
                MoveSelection(PREVIOUS_STEP, NO_STEP);
                return true;

            case Core::Key::KEY_DOWN:
                MoveSelection(NEXT_STEP, NO_STEP);
                return true;

            case Core::Key::KEY_LEFT:
                if (isRowMode) return false;   // 행 선택 표는 좌우 이동 없음
                MoveSelection(NO_STEP, PREVIOUS_STEP);
                return true;

            case Core::Key::KEY_RIGHT:
                if (isRowMode) return false;
                MoveSelection(NO_STEP, NEXT_STEP);
                return true;

            case Core::Key::KEY_ENTER:
                if (!isEditable) return false;
                BeginEdit(true);
                return true;

            case Core::Key::KEY_DELETE:
            case Core::Key::KEY_BACKSPACE:
                // 칸을 비우고 편집 시작 (그대로 Enter → 0)
                if (!isEditable) return false;
                BeginEdit(false);
                m_editor.SetText(std::wstring{});
                return true;

            case Core::Key::KEY_C:
                if (!event.m_isCtrlDown) return false;
                CopySelectedCell();
                return true;

            case Core::Key::KEY_V:
                // 붙여넣기 = 칸을 비우고 클립보드 글자로 편집 시작
                if (!event.m_isCtrlDown || !isEditable) return false;
                BeginEdit(false);
                m_editor.SetText(std::wstring{});
                static_cast<void>(m_editor.OnKeyDown(event));
                return true;

            default:
                return false;
        }
    }

    void Grid::OnTextInput(std::wstring_view text)
    {
        if (m_editingCell.has_value())
        {
            m_editor.OnTextInput(text);
            return;
        }

        // 편집 가능한 칸을 고른 상태에서 바로 입력 → 그 글자로 편집 시작 (받을 수 없는 글자면 무시)
        if (!IsCellEditable(m_selectedCell)) return;
        if (m_inputFilter && std::ranges::none_of(text, [this](wchar_t character) { return m_inputFilter(character); })) return;

        BeginEdit(false);
        m_editor.SetText(std::wstring{});
        m_editor.OnTextInput(text);
    }

    void Grid::OnComposition(std::wstring_view text)
    {
        if (m_editingCell.has_value()) m_editor.OnComposition(text);
    }

    void Grid::OnFocusChanged(bool isFocused)
    {
        if (!isFocused) CommitEdit();   // 다른 곳을 누르거나 탭을 바꾸면 입력한 값 확정 (원본 EndEdit)
    }

    Grid::Cell& Grid::GetCell(std::size_t row, std::size_t column)
    {
        if (row >= m_rowCount || column >= m_columns.size()) throw std::out_of_range{ "표의 행 · 열 범위를 벗어났어요!" };
        return m_cells[row * m_columns.size() + column];
    }

    const Grid::Cell& Grid::GetCell(std::size_t row, std::size_t column) const
    {
        if (row >= m_rowCount || column >= m_columns.size()) throw std::out_of_range{ "표의 행 · 열 범위를 벗어났어요!" };
        return m_cells[row * m_columns.size() + column];
    }

    bool Grid::IsCellEditable(const std::optional<CellPosition>& position) const
    {
        return position.has_value() && GetCell(position->m_row, position->m_column).m_isEditable;
    }

    Grid::Cell Grid::MakeDefaultCell()
    {
        return Cell{ std::wstring{}, GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::TEXT_COLOR }, false, std::wstring{} };
    }

    const std::wstring& Grid::GetEditSource(const Cell& cell) noexcept
    {
        return cell.m_editText.empty() ? cell.m_text : cell.m_editText;
    }

    // 열 너비 = 표 너비를 비율대로 나눔 (경계는 정수 DIP로 맞춰 선이 번지지 않게)
    float Grid::GetColumnEdge(std::size_t edgeIndex) const noexcept
    {
        const float width{ GetBounds().GetWidth() };
        if (edgeIndex >= m_columns.size() || m_totalWeight <= 0.0f) return edgeIndex == 0 ? 0.0f : width;

        float weightBefore{};
        for (std::size_t index{}; index < edgeIndex; ++index) weightBefore += std::max(m_columns[index].m_widthWeight, 0.0f);
        return std::round(width * weightBefore / m_totalWeight);
    }

    Core::RectF Grid::GetHeaderRect(std::size_t column) const
    {
        const Core::RectF& bounds{ GetBounds() };
        return Core::RectF{ bounds.m_left + GetColumnEdge(column), bounds.m_top,
            bounds.m_left + GetColumnEdge(column + 1), bounds.m_top + m_headerHeight };
    }

    Core::RectF Grid::GetCellRect(std::size_t row, std::size_t column) const
    {
        const Core::RectF& bounds{ GetBounds() };
        const float visibleIndex{ static_cast<float>(row) - static_cast<float>(m_firstVisibleRow) };
        const float top{ bounds.m_top + m_headerHeight + m_rowHeight * visibleIndex };
        return Core::RectF{ bounds.m_left + GetColumnEdge(column), top, bounds.m_left + GetColumnEdge(column + 1), top + m_rowHeight };
    }

    Core::RectF Grid::GetRowsArea() const noexcept
    {
        const Core::RectF& bounds{ GetBounds() };
        return Core::RectF{ bounds.m_left, std::min(bounds.m_top + m_headerHeight, bounds.m_bottom), bounds.m_right, bounds.m_bottom };
    }

    std::optional<Grid::CellPosition> Grid::FindCellAt(float x, float y) const
    {
        const Core::RectF rowsArea{ GetRowsArea() };
        if (!GetBounds().Contains(x, y) || y < rowsArea.m_top) return std::nullopt;

        const auto visibleIndex{ static_cast<std::size_t>((y - rowsArea.m_top) / m_rowHeight) };
        if (visibleIndex >= GetVisibleRowCapacity()) return std::nullopt;

        const std::size_t row{ m_firstVisibleRow + visibleIndex };
        if (row >= m_rowCount) return std::nullopt;

        for (std::size_t column{}; column < m_columns.size(); ++column)
        {
            if (x < GetBounds().m_left + GetColumnEdge(column + 1)) return CellPosition{ row, column };
        }
        return std::nullopt;
    }

    bool Grid::IsRowVisible(std::size_t row) const noexcept
    {
        return row >= m_firstVisibleRow && row < m_firstVisibleRow + GetVisibleRowCapacity();
    }

    std::size_t Grid::GetVisibleRowCapacity() const noexcept
    {
        const float rowsHeight{ GetRowsArea().GetHeight() };
        if (rowsHeight <= 0.0f) return 0;
        return static_cast<std::size_t>((rowsHeight + ROW_FIT_TOLERANCE) / m_rowHeight);
    }

    std::size_t Grid::GetMaxFirstRow() const noexcept
    {
        const std::size_t capacity{ GetVisibleRowCapacity() };
        return m_rowCount > capacity ? m_rowCount - capacity : 0;
    }

    void Grid::ScrollTo(std::size_t firstRow)
    {
        const std::size_t target{ std::min(firstRow, GetMaxFirstRow()) };
        if (target == m_firstVisibleRow) return;

        CommitEdit();   // 편집 칸이 화면 밖으로 밀려나지 않게 먼저 확정
        m_firstVisibleRow = target;
        m_hoverCell.reset();
    }

    void Grid::EnsureRowVisible(std::size_t row)
    {
        const std::size_t capacity{ GetVisibleRowCapacity() };
        if (capacity == 0) return;   // 아직 배치 전 (그릴 때 범위가 맞춰짐)

        if (row < m_firstVisibleRow) ScrollTo(row);
        else if (row >= m_firstVisibleRow + capacity) ScrollTo(row - capacity + 1);
    }

    void Grid::MoveSelection(int rowStep, int columnStep)
    {
        if (m_rowCount == 0 || m_columns.empty()) return;

        if (!m_selectedCell.has_value())
        {
            SetSelectedCell(CellPosition{}, true);   // 처음 방향키 = 왼쪽 위 칸
            EnsureRowVisible(0);
            return;
        }

        const auto lastRow{ static_cast<long long>(m_rowCount - 1) };
        const auto lastColumn{ static_cast<long long>(m_columns.size() - 1) };
        const CellPosition next{
            static_cast<std::size_t>(std::clamp(static_cast<long long>(m_selectedCell->m_row) + rowStep, 0LL, lastRow)),
            static_cast<std::size_t>(std::clamp(static_cast<long long>(m_selectedCell->m_column) + columnStep, 0LL, lastColumn)) };

        SetSelectedCell(next, true);
        EnsureRowVisible(next.m_row);
    }

    void Grid::SetSelectedCell(std::optional<CellPosition> position, bool isUserAction)
    {
        if (position == m_selectedCell) return;

        m_selectedCell = position;
        if (!isUserAction || !m_onSelectionChanged) return;

        const auto onSelectionChanged{ m_onSelectionChanged };   // 콜백 안에서 표를 다시 설정해도 안전하게
        onSelectionChanged();
    }

    void Grid::BeginEdit(bool isSelectingAll)
    {
        if (!IsCellEditable(m_selectedCell) || m_editingCell == m_selectedCell) return;

        CommitEdit();   // 다른 칸을 편집 중이었으면 먼저 확정
        EnsureRowVisible(m_selectedCell->m_row);
        m_editingCell = m_selectedCell;

        const Core::RectF cellRect{ GetCellRect(m_editingCell->m_row, m_editingCell->m_column) };
        m_editor.SetBounds(Core::RectF{ cellRect.m_left + EDITOR_INSET, cellRect.m_top + EDITOR_INSET,
            cellRect.m_right - EDITOR_INSET, cellRect.m_bottom - EDITOR_INSET });
        m_editor.SetText(GetEditSource(GetCell(m_editingCell->m_row, m_editingCell->m_column)));
        m_editor.SetFocused(true);   // 커서 깜빡임 · IME 설정
        if (isSelectingAll) m_editor.SelectAll();
    }

    void Grid::EndEdit()
    {
        if (!m_editingCell.has_value()) return;

        m_editingCell.reset();
        m_isEditorPressed = false;
        m_editor.SetFocused(false);   // IME 끔
    }

    void Grid::CopySelectedCell()
    {
        if (!m_selectedCell.has_value()) return;
        static_cast<void>(m_platform.SetClipboardText(GetCell(m_selectedCell->m_row, m_selectedCell->m_column).m_text));
    }

    // 선택 표시: 포커스가 있을 때만 (항상 표시 설정이면 포커스와 상관없이), 행 선택이면 행 전체
    bool Grid::IsCellSelected(const CellPosition& position) const noexcept
    {
        if (!m_selectedCell.has_value() || (!IsFocused() && !m_isSelectionAlwaysVisible)) return false;
        if (m_selectionMode == GridSelectionMode::SELECT_ROW) return m_selectedCell->m_row == position.m_row;
        return *m_selectedCell == position;
    }

    void Grid::SetCellSparkline(std::size_t row, std::size_t column, std::vector<double> values)
    {
        if (row >= m_rowCount || column >= m_columns.size()) return;   // 범위 밖은 무시
        GetCell(row, column).m_sparkline = std::move(values);          // 다음 프레임부터 그림
    }

    void Grid::SetRowHeight(float rowHeight) noexcept
    {
        m_rowHeight = std::max(rowHeight, Theme::GRID_ROW_HEIGHT);     // 기본보다 낮으면 편집 칸이 안 들어감
    }

    void Grid::SetShowVerticalLines(bool isShown) noexcept
    {
        m_isVerticalLineShown = isShown;                               // 열 경계 세로선
    }

    void Grid::RenderHeader(Core::Renderer& renderer)
    {
        for (std::size_t column{}; column < m_columns.size(); ++column)
        {
            const Core::RectF headerRect{ GetHeaderRect(column) };
            renderer.FillRect(headerRect, Theme::HEADER_BACK_COLOR);
            renderer.DrawString(m_columns[column].m_header, headerRect, Theme::HEADER_TEXT_COLOR, HEADER_STYLE);
        }
    }

    void Grid::RenderCells(Core::Renderer& renderer)
    {
        // 행이 표 높이보다 적으면 남는 아래쪽은 빈 공간 색 (원본 DataGridView BackgroundColor)
        const Core::RectF rowsArea{ GetRowsArea() };
        renderer.FillRect(rowsArea, Theme::BACKGROUND_COLOR);

        const Core::ClipScope clip{ renderer, rowsArea };
        const std::size_t lastRow{ std::min(m_rowCount, m_firstVisibleRow + GetVisibleRowCapacity()) };
        for (std::size_t row{ m_firstVisibleRow }; row < lastRow; ++row)
        {
            for (std::size_t column{}; column < m_columns.size(); ++column)
            {
                const Cell& cell{ m_cells[row * m_columns.size() + column] };
                const Core::RectF cellRect{ GetCellRect(row, column) };
                const CellPosition position{ row, column };
                const bool isSelected{ IsCellSelected(position) };

                renderer.FillRect(cellRect, isSelected ? Theme::SELECTION_BACK_COLOR : cell.m_style.m_backColor);
                if (!isSelected && cell.m_isEditable && IsHovered() && m_hoverCell == position)
                {
                    renderer.FillRect(cellRect, Core::WithAlpha(Theme::SELECTION_BACK_COLOR, HOVER_ALPHA));
                }

                if (m_editingCell == position || (cell.m_text.empty() && cell.m_sparkline.empty() && cell.m_style.m_decoration == GridDecoration::DECORATION_NONE)) continue;   // 편집 중인 칸은 입력칸이 대신 그림 · 그릴 것 없는 빈 칸은 건너뜀 (추이 선만 있는 칸은 그림)

                RenderCellContent(renderer, cell, cellRect, column, isSelected);   // 꾸밈 · 글자 · 추이 선
            }
        }
    }

    // 칸 안 그리기: 배지(앞) · 칩(글자 감쌈) · 글자 · 추이 선(글자가 없을 때 칸 안, 있으면 글자 뒤)
    void Grid::RenderCellContent(Core::Renderer& renderer, const Cell& cell, const Core::RectF& cellRect, std::size_t column, bool isSelected)
    {
        Core::RectF textRect{ cellRect.m_left + CELL_PADDING, cellRect.m_top, cellRect.m_right - CELL_PADDING, cellRect.m_bottom };   // 글자 영역
        const Core::TextStyle textStyle{ Theme::FONT_SIZE_NORMAL, cell.m_style.m_weight, m_columns[column].m_align };              // 글자 모양
        const Core::Color& textColor{ isSelected ? Theme::SELECTION_TEXT_COLOR : cell.m_style.m_textColor };                     // 글자 색
        const float middleY{ (cellRect.m_top + cellRect.m_bottom) * HALF_RATIO };                                               // 세로 가운데

        // 배지: 왼쪽에 둥근 사각형 + 짧은 글자, 본문 글자는 그 오른쪽부터
        if (cell.m_style.m_decoration == GridDecoration::DECORATION_BADGE)
        {
            const float size{ m_rowHeight * BADGE_SIZE_RATIO };                                                              // 배지 한 변
            const Core::RectF badgeRect{ textRect.m_left, middleY - size * HALF_RATIO, textRect.m_left + size, middleY + size * HALF_RATIO };   // 배지 영역
            renderer.FillRoundedRect(badgeRect, BADGE_RADIUS, Core::WithAlpha(cell.m_style.m_accentColor, BADGE_BACK_ALPHA));   // 연한 바탕
            renderer.DrawRoundedRect(badgeRect, BADGE_RADIUS, cell.m_style.m_accentColor, Theme::BORDER_WIDTH);               // 테두리
            renderer.DrawString(cell.m_style.m_badgeText, badgeRect, cell.m_style.m_accentColor,
                Core::TextStyle{ Theme::FONT_SIZE_SMALL, Core::FontWeight::BOLD, Core::TextAlign::CENTER });                 // 배지 글자
            textRect.m_left = badgeRect.m_right + BADGE_GAP;                                                                 // 본문은 배지 오른쪽
        }

        // 칩: 글자 크기에 맞춘 둥근 테두리 (정렬 방향에 맞춰 위치)
        if (cell.m_style.m_decoration == GridDecoration::DECORATION_CHIP && !cell.m_text.empty())
        {
            const Core::TextStyle chipStyle{ Theme::FONT_SIZE_SMALL, cell.m_style.m_weight, Core::TextAlign::CENTER };       // 칩 글자
            const float textWidth{ renderer.MeasureString(cell.m_text, chipStyle, textRect.m_right - textRect.m_left).m_width };   // 글자 너비
            const float chipWidth{ std::min(textWidth + CHIP_PADDING_X * 2.0f, textRect.m_right - textRect.m_left) };         // 칩 너비
            const float chipHeight{ m_rowHeight * CHIP_HEIGHT_RATIO };                                                       // 칩 높이
            float chipLeft{ textRect.m_left };                                                                               // 왼쪽 정렬 기본
            if (m_columns[column].m_align == Core::TextAlign::TRAILING) chipLeft = textRect.m_right - chipWidth;            // 오른쪽 정렬
            else if (m_columns[column].m_align == Core::TextAlign::CENTER) chipLeft = (textRect.m_left + textRect.m_right - chipWidth) * HALF_RATIO;   // 가운데
            const Core::RectF chipRect{ chipLeft, middleY - chipHeight * HALF_RATIO, chipLeft + chipWidth, middleY + chipHeight * HALF_RATIO };   // 칩 영역
            renderer.FillRoundedRect(chipRect, CHIP_RADIUS, Core::WithAlpha(cell.m_style.m_accentColor, CHIP_BACK_ALPHA));   // 연한 바탕
            renderer.DrawRoundedRect(chipRect, CHIP_RADIUS, Core::WithAlpha(cell.m_style.m_accentColor, HALF_RATIO), Theme::BORDER_WIDTH);   // 테두리
            renderer.DrawString(cell.m_text, chipRect, textColor, chipStyle);                                                // 칩 글자
        }
        else if (!cell.m_text.empty())
        {
            renderer.DrawString(cell.m_text, textRect, textColor, textStyle);                                                // 일반 글자
        }

        // 추이 선: 칸 왼쪽(정렬 기준)부터 최대 너비만큼, 값 범위를 칸 높이 비율에 맞춤
        if (cell.m_sparkline.size() < MIN_SPARKLINE_POINTS) return;                                                         // 점이 부족
        const auto [minIt, maxIt] { std::ranges::minmax_element(cell.m_sparkline) };                                       // 값 범위
        const double range{ *maxIt - *minIt };                                                                              // 높이 환산 기준
        const float width{ std::min(SPARKLINE_MAX_WIDTH, textRect.m_right - textRect.m_left) };                             // 그래프 너비
        const float height{ m_rowHeight * SPARKLINE_HEIGHT_RATIO };                                                         // 그래프 높이
        const float left{ textRect.m_left };                                                                                // 그래프 왼쪽
        const float bottom{ middleY + height * HALF_RATIO };                                                                // 그래프 아래
        const bool isRising{ cell.m_sparkline.back() >= cell.m_sparkline.front() };                                         // 상승 여부
        const Core::Color& lineColor{ isRising ? Theme::SPARKLINE_UP_COLOR : Theme::SPARKLINE_DOWN_COLOR };                 // 상승 청록 · 하락 빨강
        const float stepX{ width / static_cast<float>(cell.m_sparkline.size() - 1) };                                       // 점 간격
        const auto toY{ [&](double value) { return range > 0.0 ? bottom - static_cast<float>((value - *minIt) / range) * height : middleY; } };   // 값 → y
        for (std::size_t index{ 1 }; index < cell.m_sparkline.size(); ++index)                                              // 선분으로 연결
        {
            renderer.DrawLine(left + stepX * static_cast<float>(index - 1), toY(cell.m_sparkline[index - 1]),
                left + stepX * static_cast<float>(index), toY(cell.m_sparkline[index]), lineColor, SPARKLINE_STROKE);   // 한 구간
        }
    }

    // 칸 경계선 (원본 CellBorderStyle Single) — 세로선은 머리글과 그려진 행까지만, 바깥 테두리는 표 전체
    void Grid::RenderGridLines(Core::Renderer& renderer) const
    {
        const Core::RectF& bounds{ GetBounds() };
        const std::size_t drawnRowCount{ std::min(m_rowCount - std::min(m_firstVisibleRow, m_rowCount), GetVisibleRowCapacity()) };
        const float rowsBottom{ bounds.m_top + m_headerHeight + m_rowHeight * static_cast<float>(drawnRowCount) };

        for (std::size_t edge{ 1 }; m_isVerticalLineShown && edge < m_columns.size(); ++edge)   // 세로선은 켰을 때만
        {
            const float x{ SnapToPixel(bounds.m_left + GetColumnEdge(edge)) };
            renderer.DrawLine(x, bounds.m_top, x, rowsBottom, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);
        }

        // 각 행의 위 경계 + 마지막 행의 아래 경계
        for (std::size_t index{}; index <= drawnRowCount; ++index)
        {
            const float y{ SnapToPixel(bounds.m_top + m_headerHeight + m_rowHeight * static_cast<float>(index)) };
            if (y > bounds.m_bottom) break;
            renderer.DrawLine(bounds.m_left, y, bounds.m_right, y, Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);
        }

        renderer.DrawRect(Core::RectF{ SnapToPixel(bounds.m_left), SnapToPixel(bounds.m_top), SnapToPixel(bounds.m_right), SnapToPixel(bounds.m_bottom) },
            Theme::GRID_LINE_COLOR, Theme::BORDER_WIDTH);
    }

    // 행이 더 있으면 오른쪽에 얇은 스크롤 막대 (전체 중 어디를 보고 있는지)
    void Grid::RenderScrollBar(Core::Renderer& renderer) const
    {
        if (GetMaxFirstRow() == 0) return;

        const Core::RectF rowsArea{ GetRowsArea() };
        const float trackHeight{ rowsArea.GetHeight() - SCROLLBAR_MARGIN * BOTH_SIDES };
        const float rowCount{ static_cast<float>(m_rowCount) };
        const float thumbTop{ rowsArea.m_top + SCROLLBAR_MARGIN + trackHeight * static_cast<float>(m_firstVisibleRow) / rowCount };
        const float thumbHeight{ trackHeight * static_cast<float>(GetVisibleRowCapacity()) / rowCount };
        const float thumbRight{ rowsArea.m_right - SCROLLBAR_MARGIN };

        renderer.FillRoundedRect(Core::RectF{ thumbRight - SCROLLBAR_WIDTH, thumbTop, thumbRight, thumbTop + thumbHeight },
            SCROLLBAR_WIDTH * HALF, Theme::MUTED_TEXT_COLOR);
    }
}