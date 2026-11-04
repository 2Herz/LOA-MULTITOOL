export module LostArkMultiTool.Views.UI.TextBox;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Control;

export namespace LostArkMultiTool::Views::UI
{
    // 한 줄 글자 입력칸 (커서 · 선택 · 복사/잘라내기/붙여넣기 · 한글 조합 · 가로 스크롤)
    // 한글은 조합 중인 글자를 칸 안에 밑줄로 직접 그리고, 포커스가 옮겨 가면 조합 중인 글자를 확정함
    class TextBox final : public Control
    {
    public:
        TextBox(Core::Platform& platform, std::wstring placeholder);

        void SetText(std::wstring text);   // 커서는 끝으로, 선택 해제
        [[nodiscard]] const std::wstring& GetText() const noexcept;

        void SetMaxLength(std::size_t maxLength) noexcept;
        void SetIMEAllowed(bool isAllowed) noexcept;   // false: 영문 · 숫자 전용 칸 (한/영 상태와 상관없이 영문 입력)
        void SetOnTextChanged(std::function<void()> onTextChanged);
        void SetOnSubmit(std::function<void()> onSubmit);   // Enter

        void SelectAll();
        void CommitComposition();   // 조합 중인 글자를 확정 (클릭 · 포커스 이동 · Enter 전에)

        [[nodiscard]] bool IsFocusable() const noexcept override;
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept override;

        void Update(double deltaSeconds) override;
        void Render(Core::Renderer& renderer) override;

        void OnMouseDown(const Core::MouseButtonEvent& event) override;
        void OnMouseUp(const Core::MouseButtonEvent& event, bool isInside) override;
        void OnMouseDrag(const Core::MouseMoveEvent& event) override;
        [[nodiscard]] bool OnKeyDown(const Core::KeyEvent& event) override;
        void OnTextInput(std::wstring_view text) override;
        void OnComposition(std::wstring_view text) override;

    protected:
        void OnFocusChanged(bool isFocused) override;

    private:
        // ===== 편집 =====
        void InsertText(std::wstring_view text);
        [[nodiscard]] bool DeleteSelection();
        void DeleteBackward();
        void DeleteForward();
        void CopySelection();
        void CutSelection();
        void Paste();

        // ===== 커서 · 선택 =====
        [[nodiscard]] bool HasSelection() const noexcept;
        [[nodiscard]] std::size_t GetSelectionStart() const noexcept;
        [[nodiscard]] std::size_t GetSelectionEnd() const noexcept;
        void MoveCaret(std::size_t position, bool isExtending) noexcept;
        [[nodiscard]] std::size_t GetPreviousPosition(std::size_t position) const noexcept;
        [[nodiscard]] std::size_t GetNextPosition(std::size_t position) const noexcept;
        [[nodiscard]] std::size_t FindPositionAt(float x) const noexcept;

        // ===== 그리기 =====
        [[nodiscard]] Core::RectF GetTextArea() const noexcept;
        [[nodiscard]] std::wstring BuildDisplayText() const;
        void EnsureCaretVisible(float areaWidth, std::size_t displayCaret) noexcept;
        void UpdateIMEPosition(const Core::RectF& caretRect);
        [[nodiscard]] bool IsCaretVisible() const noexcept;
        void ResetCaretBlink() noexcept;
        void NotifyTextChanged();

        Core::Platform& m_platform;
        std::wstring m_text;
        std::wstring m_placeholder;
        std::wstring m_composition;     // 한글 조합 중인 글자 (커서 위치에 끼워서 표시)
        std::size_t m_caret{};          // 커서 위치 (글자 사이 경계, 0 ~ 글자 수)
        std::size_t m_anchor{};         // 선택 시작 위치 (m_caret과 같으면 선택 없음)
        std::size_t m_maxLength{};
        bool m_isIMEAllowed{ true };
        bool m_isDragSelecting{};       // 마우스로 끌어서 선택 중 (더블 클릭 전체 선택 뒤에는 끌기 무시)
        double m_blinkSeconds{};
        float m_scrollX{};              // 글자가 칸보다 길 때 왼쪽으로 밀어 낸 양

        // 마지막 그리기에서 잰 글자 위치 (같은 글자면 다시 재지 않음 → 매 프레임 측정 방지)
        std::wstring m_measuredText;
        std::vector<float> m_caretPositions;

        Core::RectF m_lastIMERect;
        bool m_hasIMERect{};

        std::function<void()> m_onTextChanged;
        std::function<void()> m_onSubmit;
    };
}