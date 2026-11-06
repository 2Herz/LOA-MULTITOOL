export module LostArkMultiTool.Views.CombatSimulatorView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // 장비 표 열 (매니저가 고친 칸을 구분할 때 사용)
    enum class CombatGearColumn : std::size_t
    {
        COLUMN_PART,
        COLUMN_ITEM,
        COLUMN_CURRENT_ENHANCEMENT,
        COLUMN_CURRENT_ADVANCED,
        COLUMN_TARGET_ENHANCEMENT,    // 직접 입력
        COLUMN_TARGET_ADVANCED,       // 직접 입력 (결단 · 업화만)
        COLUMN_COUNT
    };

    // 장비 표 한 행 (글자는 매니저가 만들어 넘김)
    struct CombatGearRow
    {
        std::wstring m_partName;
        std::wstring m_itemName;                 // 장비 이름 (없으면 "-")
        std::string m_gradeName;                 // 등급 글자색 ("유물" · "고대" · "에스더", UTF-8)
        std::wstring m_currentEnhancement;
        std::wstring m_currentAdvanced;
        std::wstring m_targetEnhancement;
        std::wstring m_targetAdvanced;
        bool m_isTargetEditable{};               // 계수 표가 있는 장비만
        bool m_isTargetAdvancedEditable{};       // 상급 재련을 넣을 수 있는 단계일 때만
        bool m_hasChange{};                      // 목표가 현재와 다름 (목표 칸 강조)
    };

    // 보석 표 열
    enum class CombatGemColumn : std::size_t
    {
        COLUMN_SLOT,
        COLUMN_KIND,
        COLUMN_CURRENT_LEVEL,
        COLUMN_TARGET_LEVEL,          // 직접 입력 (1 ~ 10)
        COLUMN_COUNT
    };

    // 보석 표 한 행
    struct CombatGemRow
    {
        std::wstring m_slot;                     // "1" ~ "11"
        std::wstring m_kind;                     // "겁화" · "작열" (모르면 "-")
        std::wstring m_currentLevel;
        std::wstring m_targetLevel;
        bool m_hasChange{};                      // 목표가 현재와 다름 (목표 칸 강조)
    };

    // 목록형 카드 (악세서리 · 아크 그리드 · 팔찌/스톤/각인서 · 아크 패시브/아바타)
    enum class CombatCard : std::size_t
    {
        CARD_ACCESSORY,
        CARD_ARK_GRID,
        CARD_BRACELET,
        CARD_PASSIVE,
        CARD_COUNT
    };

    // 목록 행의 편집 방식
    enum class CombatRowKind
    {
        ROW_HEADER,    // 구분 제목 (편집 없음)
        ROW_NUMBER,    // 목표 칸에 숫자를 직접 입력
        ROW_CHOICE     // 행을 고르면 위 선택 상자에 선택지가 나옴
    };

    // 목록형 카드의 한 행 (글자는 매니저가 만들어 넘김)
    struct CombatListRow
    {
        std::wstring m_label;
        std::wstring m_current;
        std::wstring m_target;
        CombatRowKind m_kind{ CombatRowKind::ROW_HEADER };
        std::vector<std::wstring> m_choices;          // ROW_CHOICE의 선택지
        std::optional<std::size_t> m_selectedChoice;  // 목표가 선택지 중 몇 번째인지
        bool m_hasChange{};                           // 목표가 현재와 다름 (목표 칸 강조)
    };

    // 결과 상자 글자
    struct CombatResultText
    {
        std::wstring m_currentPower;             // "현재 전투력 (API)  1,234.56"
        std::wstring m_predictedPower;           // "예상 전투력  1,300.00"
        std::wstring m_change;                   // "증감  +65.44 (+5.30%)"
        bool m_isUp{};
        bool m_isDown{};
        std::wstring m_itemLevel;                // "예상 아이템 레벨  1,764.17 → 1,770.00"
        std::wstring m_factors;                  // "배율  장비 ×1.0300 · 무기 품질 ×1.0000 · 보석 ×1.0100"
    };

    // 전투력 시뮬레이터 화면 구성 (로펙형)
    // 위 결과 바: 캐릭터 이름 + [조회] + 상태 / 현재 → 예상 전투력 · 증감 · 아이템 레벨 / 배율 내역
    // 아래 3 x 2 카드: 장비 재련 · 악세서리 · 아크 그리드 / 보석 · 무기 품질 · 팔찌 스톤 각인서 · 아크 패시브 아바타
    // 부품 배치 · 표시 형식만 알고, 조회 · 계산 · 저장은 CombatSimulatorManager가 맡음
    class CombatSimulatorView
    {
    public:
        explicit CombatSimulatorView(Core::Platform& platform);
        ~CombatSimulatorView();

        CombatSimulatorView(const CombatSimulatorView&) = delete;
        CombatSimulatorView& operator=(const CombatSimulatorView&) = delete;

        // ===== 사용자 동작 알림 (CombatSimulatorManager가 연결) =====
        void SetOnSearch(std::function<void(const std::wstring& characterName)> onSearch);
        void SetOnGearCellEdited(std::function<void(std::size_t row, CombatGearColumn column, const std::wstring& text)> onGearCellEdited);
        void SetOnQualityChanged(std::function<void(const std::wstring& text)> onQualityChanged);
        void SetOnGemTargetSelected(std::function<void(std::size_t index)> onGemTargetSelected);
        void SetOnGemCellEdited(std::function<void(std::size_t row, const std::wstring& text)> onGemCellEdited);   // 목표 레벨 칸만
        void SetOnCardEdited(std::function<void(CombatCard card, std::size_t row, const std::wstring& text)> onCardEdited);   // 숫자 입력 또는 선택한 선택지 글자

        // ===== 상단 바 =====
        void SetCharacterName(std::wstring name);
        void SetSearchBusy(bool isBusy);
        void SetStatusText(std::wstring text, bool isError);

        // ===== 장비 · 품질 · 보석 =====
        void ShowGears(const std::vector<CombatGearRow>& rows);
        void SetQualityText(std::wstring currentText);     // "무기 품질  현재 95  →  목표"
        void SetQualityTarget(std::wstring text);          // 코드에서 바꿈 (알림 없음)
        void SetQualityEnabled(bool isEnabled) noexcept;   // 에스더 무기 · 조회 전이면 비활성
        void SetGemText(std::wstring text);                // "보석 11개 · 10레벨 5개 · 9레벨 6개"
        void SetGemTargetItems(std::vector<std::wstring> items);
        void SetGemTargetIndex(std::size_t index);         // 코드에서 바꿈 (알림 없음)
        void SetGemTargetEnabled(bool isEnabled) noexcept;
        void ShowGems(const std::vector<CombatGemRow>& rows);   // 보석 하나씩 목표 레벨 (모든 보석 목표 뒤에 개별로 덮어씀)

        // ===== 목록형 카드 =====
        void ShowCard(CombatCard card, std::vector<CombatListRow> rows);

        // ===== 결과 =====
        void ShowResult(const CombatResultText& result);

        // ===== 프레임 =====
        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때
        void RestoreFocus();

    private:
        // 묶음 상자 (테두리 + 제목)
        struct Panel
        {
            Core::RectF m_bounds;
            std::wstring m_title;
        };

        // 목록형 카드 하나 (제목 상자 + 선택 상자 + 선택한 행 안내 + 표)
        struct ListCard
        {
            Panel m_panel;
            UI::ComboBox* m_combo{};
            UI::Label* m_hint{};
            UI::Grid* m_grid{};
            std::vector<CombatListRow> m_rows;
        };

        void Layout(const Core::RectF& contentArea);
        void LayoutListCard(ListCard& card, const Core::RectF& bounds);
        void SyncCardEditor(std::size_t cardIndex);   // 선택한 행에 맞춰 선택 상자 · 안내 갱신
        void OnCardChoice(std::size_t cardIndex);     // 선택 상자에서 고름
        [[nodiscard]] Core::RectF LayoutPanel(Panel& panel, const Core::RectF& bounds) const;   // 테두리 · 제목 자리를 뺀 안쪽 영역
        void RenderPanels(Core::Renderer& renderer) const;
        void RequestSearch();

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Label& m_nameLabel;
        UI::TextBox& m_nameBox;
        UI::Button& m_searchButton;
        UI::Label& m_statusLabel;
        UI::Grid& m_gearGrid;
        UI::Label& m_qualityLabel;
        UI::TextBox& m_qualityBox;
        UI::Label& m_gemLabel;
        UI::Label& m_gemTargetLabel;
        UI::ComboBox& m_gemTargetBox;
        UI::Grid& m_gemGrid;
        std::array<ListCard, static_cast<std::size_t>(CombatCard::CARD_COUNT)> m_cards;
        UI::Label& m_currentPowerLabel;
        UI::Label& m_predictedPowerLabel;
        UI::Label& m_changeLabel;
        UI::Label& m_itemLevelLabel;
        UI::Label& m_factorLabel;
        UI::Label& m_noteLabel;

        Panel m_barPanel;      // 맨 위 결과 바
        Panel m_gearPanel;
        Panel m_optionPanel;
        Core::RectF m_layoutArea;
        bool m_hasLayout{};
        bool m_isUpdatingQualityText{};   // 코드에서 품질 칸을 바꾸는 중 (알림 막기)

        std::function<void(const std::wstring&)> m_onSearch;
        std::function<void(std::size_t, CombatGearColumn, const std::wstring&)> m_onGearCellEdited;
        std::function<void(const std::wstring&)> m_onQualityChanged;
        std::function<void(std::size_t)> m_onGemTargetSelected;
        std::function<void(std::size_t, const std::wstring&)> m_onGemCellEdited;
        std::function<void(CombatCard, std::size_t, const std::wstring&)> m_onCardEdited;
    };
}
