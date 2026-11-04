export module LostArkMultiTool.Views.UI.Grid;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.UI.Control;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views::UI
{
    // 표의 열 하나
    struct GridColumn
    {
        std::wstring m_header;
        float m_widthWeight{ 1.0f };                            // 표 너비를 나눠 가질 비율 (원본 FillWeight)
        Core::TextAlign m_align{ Core::TextAlign::TRAILING };   // 숫자 표라 기본은 오른쪽 정렬
    };

    // 칸 하나의 색 · 굵기 (원본 DataGridViewCellStyle)
    struct GridCellStyle
    {
        Core::Color m_backColor;
        Core::Color m_textColor;
        Core::FontWeight m_weight{ Core::FontWeight::NORMAL };
    };

    // 선택 단위 (원본 DataGridViewSelectionMode)
    enum class GridSelectionMode
    {
        SELECT_CELL,   // 칸 하나 (계산기 수량 표)
        SELECT_ROW     // 행 전체 (보석 알림 목록 — FullRowSelect)
    };

    // 표 (원본 DataGridView: 머리글 + 칸 색 + 편집 가능한 칸 + 세로 스크롤)
    // 편집: 편집 가능한 칸을 누르거나(Enter · 숫자 입력) 바로 입력 → Enter · 다른 칸 누르기 · 포커스 이동으로 확정, Esc로 취소
    // 행이 표 높이보다 많으면 휠 · 방향키로 스크롤 (오른쪽에 얇은 스크롤 막대)
    class Grid final : public Control
    {
    public:
        Grid(Core::Platform& platform, std::vector<GridColumn> columns, std::size_t rowCount);
        ~Grid() override;

        [[nodiscard]] std::size_t GetRowCount() const noexcept;
        [[nodiscard]] std::size_t GetColumnCount() const noexcept;

        // 행 수 바꾸기 (늘어난 행은 빈 기본 칸, 편집 중이면 취소, 선택 · 스크롤은 범위 안으로)
        void SetRowCount(std::size_t rowCount);

        // 머리글 행 높이 (기본 Theme::GRID_HEADER_HEIGHT, 두 줄 머리글 "직업\n(Lv.1740)"은 더 높게)
        void SetHeaderHeight(float headerHeight) noexcept;

        // 열 구성 바꾸기 (열 수가 바뀌는 표: 낙원력 트래커의 캐릭터 열, 모든 칸은 빈 기본 칸으로, 편집 · 선택은 취소)
        void SetColumns(std::vector<GridColumn> columns);

        // 칸 내용 · 모양 (범위를 벗어난 행 · 열이면 std::out_of_range → 경계 검사)
        // SetCellText는 편집용 글자(SetCellEditText)를 지움 → 표시 글자와 편집 글자가 다를 때만 SetCellEditText를 이어서 부름
        void SetCellText(std::size_t row, std::size_t column, std::wstring text);
        void SetCellEditText(std::size_t row, std::size_t column, std::wstring editText);   // 편집을 시작할 때 입력칸에 채울 글자 (예: "1,120 (▲120)" 칸 → "1,120")
        [[nodiscard]] const std::wstring& GetCellText(std::size_t row, std::size_t column) const;
        void SetCellStyle(std::size_t row, std::size_t column, const GridCellStyle& style);
        void SetCellEditable(std::size_t row, std::size_t column, bool isEditable);

        // 편집 입력 설정 (숫자 칸이면 IME를 끄고 숫자만 받음)
        void SetEditInput(bool isIMEAllowed, std::function<bool(wchar_t character)> inputFilter);
        // 편집을 확정해 내용이 바뀌었을 때 (text = 입력한 글자 그대로, 표시 형식은 받는 쪽이 SetCellText로 정함)
        void SetOnCellEdited(std::function<void(std::size_t row, std::size_t column, const std::wstring& text)> onCellEdited);

        // ===== 선택 =====
        void SetSelectionMode(GridSelectionMode mode) noexcept;
        void SetSelectionAlwaysVisible(bool isAlwaysVisible) noexcept;   // 포커스가 없어도 선택 표시 (삭제 버튼을 누를 때 선택 유지)
        void SetOnCellClicked(std::function<void(std::size_t row, std::size_t column)> onCellClicked);   // 마우스로 칸을 누름
        void SetOnSelectionChanged(std::function<void()> onSelectionChanged);   // 사용자가 마우스 · 키보드로 선택을 바꿈
        [[nodiscard]] std::optional<std::size_t> GetSelectedRow() const noexcept;
        void SelectRow(std::optional<std::size_t> row);   // 코드에서 바꿈 (알림 없음, 범위를 벗어나면 선택 해제)
        void SelectCell(std::size_t row, std::size_t column);   // 코드에서 칸 선택 + 그 행이 보이게 스크롤 (알림 없음, 범위를 벗어나면 무시)

        void CommitEdit();   // 편집 중이면 확정 (시세 갱신 · 캐릭터 변경 전 등)
        void CancelEdit();   // 편집 중이면 취소

        [[nodiscard]] float GetPreferredHeight() const noexcept;   // 머리글 + 모든 행이 보이는 높이

        [[nodiscard]] bool IsFocusable() const noexcept override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;

        void Update(double deltaSeconds) override;
        void Render(Core::Renderer& renderer) override;

        void OnMouseDown(const Core::MouseButtonEvent& event) override;
        void OnMouseUp(const Core::MouseButtonEvent& event, bool isInside) override;
        void OnMouseDrag(const Core::MouseMoveEvent& event) override;
        void OnMouseMove(const Core::MouseMoveEvent& event) override;
        [[nodiscard]] bool OnMouseWheel(const Core::MouseWheelEvent& event) override;
        [[nodiscard]] bool OnKeyDown(const Core::KeyEvent& event) override;
        void OnTextInput(std::wstring_view text) override;
        void OnComposition(std::wstring_view text) override;

    protected:
        void OnFocusChanged(bool isFocused) override;

    private:
        struct Cell
        {
            std::wstring m_text;
            GridCellStyle m_style;
            bool m_isEditable{};
            std::wstring m_editText;   // 비어 있으면 표시 글자(m_text)로 편집
        };

        struct CellPosition
        {
            std::size_t m_row{};
            std::size_t m_column{};

            [[nodiscard]] bool operator==(const CellPosition& other) const noexcept = default;
        };

        [[nodiscard]] Cell& GetCell(std::size_t row, std::size_t column);
        [[nodiscard]] const Cell& GetCell(std::size_t row, std::size_t column) const;
        [[nodiscard]] bool IsCellEditable(const std::optional<CellPosition>& position) const;
        [[nodiscard]] static Cell MakeDefaultCell();
        [[nodiscard]] static const std::wstring& GetEditSource(const Cell& cell) noexcept;   // 편집을 시작할 때 입력칸에 채울 글자

        // ===== 배치 =====
        [[nodiscard]] float GetColumnEdge(std::size_t edgeIndex) const noexcept;   // 열 경계 x (0 = 표 왼쪽, 열 수 = 표 오른쪽)
        [[nodiscard]] Core::RectF GetHeaderRect(std::size_t column) const;
        [[nodiscard]] Core::RectF GetCellRect(std::size_t row, std::size_t column) const;   // 스크롤 반영 (보이지 않는 행이면 영역 밖)
        [[nodiscard]] Core::RectF GetRowsArea() const noexcept;                              // 머리글 아래 행이 그려지는 영역
        [[nodiscard]] std::optional<CellPosition> FindCellAt(float x, float y) const;
        [[nodiscard]] bool IsRowVisible(std::size_t row) const noexcept;

        // ===== 스크롤 =====
        [[nodiscard]] std::size_t GetVisibleRowCapacity() const noexcept;   // 표 높이에 들어가는 행 수
        [[nodiscard]] std::size_t GetMaxFirstRow() const noexcept;
        void ScrollTo(std::size_t firstRow);
        void EnsureRowVisible(std::size_t row);

        // ===== 선택 · 편집 =====
        void MoveSelection(int rowStep, int columnStep);
        void SetSelectedCell(std::optional<CellPosition> position, bool isUserAction);
        void BeginEdit(bool isSelectingAll);
        void EndEdit();
        void CopySelectedCell();
        [[nodiscard]] bool IsCellSelected(const CellPosition& position) const noexcept;

        // ===== 그리기 =====
        void RenderHeader(Core::Renderer& renderer);
        void RenderCells(Core::Renderer& renderer);
        void RenderGridLines(Core::Renderer& renderer) const;
        void RenderScrollBar(Core::Renderer& renderer) const;

        Core::Platform& m_platform;
        std::vector<GridColumn> m_columns;
        std::size_t m_rowCount{};
        std::vector<Cell> m_cells;                 // 행 우선 순서 (row * 열 수 + column)
        float m_totalWeight{};                     // 모든 열의 너비 비율 합
        float m_headerHeight{ Theme::GRID_HEADER_HEIGHT };
        std::optional<CellPosition> m_selectedCell;
        std::optional<CellPosition> m_hoverCell;
        std::optional<CellPosition> m_editingCell;
        bool m_isEditorPressed{};                  // 편집 칸 안에서 눌렀음 (끌어서 글자 선택)
        std::size_t m_firstVisibleRow{};           // 스크롤: 맨 위에 보이는 행
        GridSelectionMode m_selectionMode{ GridSelectionMode::SELECT_CELL };
        bool m_isSelectionAlwaysVisible{};

        TextBox m_editor;                          // 편집할 때 칸 위에 띄우는 입력칸 (한글 · 선택 · 붙여넣기는 TextBox가 처리)
        std::function<bool(wchar_t)> m_inputFilter;   // 편집을 시작하는 글자인지 확인 (숫자 칸에 영문을 치면 편집을 시작하지 않음)
        std::function<void(std::size_t, std::size_t, const std::wstring&)> m_onCellEdited;
        std::function<void(std::size_t, std::size_t)> m_onCellClicked;
        std::function<void()> m_onSelectionChanged;
    };
}