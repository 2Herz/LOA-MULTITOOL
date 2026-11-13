export module LostArkMultiTool.Views.RefineSimulatorView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // 부위 표 열 (매니저가 누른 칸 · 고친 칸을 구분할 때 사용)
    enum class RefineSlotColumn : std::size_t
    {
        COLUMN_SLOT,
        COLUMN_START_TIER,          // 누르면 에기르 ↔ 세르카
        COLUMN_START_ENHANCEMENT,
        COLUMN_START_ADVANCED,
        COLUMN_TARGET_TIER,         // 부위별 단계 기준일 때 누르면 에기르 ↔ 세르카
        COLUMN_TARGET_ENHANCEMENT,
        COLUMN_TARGET_ADVANCED,
        COLUMN_CHANGE,
        COLUMN_COST,
        COLUMN_COUNT
    };

    // 재료 표 열
    enum class RefineMaterialColumn : std::size_t
    {
        COLUMN_NAME,
        COLUMN_PRICE,               // 직접 입력
        COLUMN_TRADE,               // 누르면 거래 ↔ 귀속
        COLUMN_AMOUNT,
        COLUMN_COST,
        COLUMN_COUNT
    };

    // 부위 표 한 행 (글자는 매니저가 만들어 넘김)
    struct RefineSlotRow
    {
        std::wstring m_slotName;
        std::wstring m_startTier;
        std::wstring m_startEnhancement;
        std::wstring m_startAdvanced;
        bool m_isStartAdvancedEditable{};    // 에기르만 상급 재련 입력
        std::wstring m_targetTier;
        std::wstring m_targetEnhancement;
        std::wstring m_targetAdvanced;
        bool m_isTargetEditable{};           // 부위별 단계 기준일 때만
        bool m_isTargetAdvancedEditable{};
        bool m_isTargetSerca{};              // 목표 장비 글자색 (세르카 = 강조)
        bool m_isStartSerca{};
        std::wstring m_change;               // "+5단계" · "계승 +7단계" · "변화 없음"
        std::wstring m_cost;                 // "59.0만 G · 5회"
        bool m_hasChange{};
    };

    // 재료 표 한 행
    struct RefineMaterialRow
    {
        std::wstring m_name;
        std::wstring m_price;
        bool m_isPriceEditable{};            // 골드는 1 고정
        std::wstring m_trade;                // "거래" · "귀속" (골드: "일반 골드" · "귀속 골드")
        bool m_isBound{};
        std::wstring m_amount;               // 예상 소모 (없으면 "-")
        std::wstring m_cost;                 // 예상 골드 (귀속이면 "-")
    };

    // 재련 경로 표 한 행 (같은 걸음은 묶어서 한 행)
    struct RefineRouteRow
    {
        std::wstring m_order;                // "1~5"
        std::wstring m_description;          // "방어구 전체 +11 → +12"
        std::wstring m_itemLevel;            // 달성 아이템 레벨
        std::wstring m_cost;                 // 이 묶음 골드
        std::wstring m_cumulative;           // 누적 골드
        bool m_isInherit{};                  // 계승 행 (강조)
        bool m_isReached{};                  // 목표 아이템 레벨을 처음 넘긴 행
    };

    // 재련 시뮬레이터 화면 구성 (로아업 통합 재련 형식)
    // 위: 상단 바 + 시작 · 목표 설정 + 부위 표 / 아래 왼쪽: 재료 가격 · 귀속 / 아래 오른쪽: 총 소요 재화 + 재련 경로
    // 부품 배치 · 표시 형식만 알고, 계산 · 저장 · 시세 조회는 RefineSimulatorManager가 맡음
    class RefineSimulatorView
    {
    public:
        explicit RefineSimulatorView(Core::Platform& platform);
        ~RefineSimulatorView();

        RefineSimulatorView(const RefineSimulatorView&) = delete;
        RefineSimulatorView& operator=(const RefineSimulatorView&) = delete;

        // ===== 사용자 동작 알림 (RefineSimulatorManager가 연결) =====
        void SetOnRefresh(std::function<void()> onRefresh);
        void SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged);
        void SetOnStartPresetSelected(std::function<void(std::size_t presetIndex)> onStartPresetSelected);
        void SetOnLookup(std::function<void(const std::wstring& characterName)> onLookup);   // [조회] 버튼 · 이름 칸 Enter
        void SetOnCalculate(std::function<void()> onCalculate);   // [계산] 버튼 (지금 입력으로 다시 계산)
        void SetOnTargetModeSelected(std::function<void(std::size_t modeIndex)> onTargetModeSelected);
        void SetOnTargetItemLevelChanged(std::function<void(const std::wstring& text)> onTargetItemLevelChanged);
        void SetOnSlotCellClicked(std::function<void(std::size_t row, RefineSlotColumn column)> onSlotCellClicked);
        void SetOnSlotCellEdited(std::function<void(std::size_t row, RefineSlotColumn column, const std::wstring& text)> onSlotCellEdited);
        void SetOnMaterialCellClicked(std::function<void(std::size_t row, RefineMaterialColumn column)> onMaterialCellClicked);
        void SetOnMaterialCellEdited(std::function<void(std::size_t row, RefineMaterialColumn column, const std::wstring& text)> onMaterialCellEdited);

        // ===== 상단 바 =====
        void SetAutoRefreshChecked(bool isChecked) noexcept;   // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsAutoRefreshChecked() const noexcept;
        void SetRefreshEnabled(bool isEnabled) noexcept;
        void SetStatusText(std::wstring text);

        // ===== 시작 · 목표 설정 =====
        void SetStartPresetItems(std::vector<std::wstring> items);
        void SetStartPresetIndex(std::optional<std::size_t> index);   // 코드에서 바꿈 (알림 없음, nullopt = 직접 설정)
        void SetCharacterName(std::wstring name);                      // 코드에서 바꿈 (저장된 마지막 조회 이름)
        void SetLookupBusy(bool isBusy);                      // 조회 중이면 버튼을 "조회 중..."으로 잠금
        void SetCalculateBusy(bool isBusy);                   // 계산 중이면 [계산] 버튼을 "계산 중..."으로 잠금
        void SetTargetModeItems(std::vector<std::wstring> items);
        void SetTargetModeIndex(std::size_t index);
        void SetTargetItemLevelText(std::wstring text);                // 코드에서 바꿈 (알림 없음)
        void SetTargetItemLevelEnabled(bool isEnabled) noexcept;       // 부위별 단계 기준이면 비활성
        void SetSummaryText(std::wstring text, bool isError);          // "1730.00 → 1750.00" · 오류 안내

        // ===== 표 =====
        void ShowSlots(const std::vector<RefineSlotRow>& rows);
        void ShowMaterials(const std::vector<RefineMaterialRow>& rows);
        void ShowRoute(std::wstring totalText, const std::vector<RefineRouteRow>& rows);

        // ===== 프레임 =====
        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때
        void RestoreFocus();

    private:
        // 카드 (피그마 머리말: 영문 소제목 + 제목)
        struct Panel
        {
            Core::RectF m_bounds;
            std::wstring m_eyebrow;
            std::wstring m_title;
        };

        void Layout(const Core::RectF& contentArea);
        [[nodiscard]] Core::RectF LayoutPanel(Panel& panel, const Core::RectF& bounds) const;   // 테두리 · 제목 자리를 뺀 안쪽 영역
        void RenderPanels(Core::Renderer& renderer) const;
        void RequestLookup();   // 이름 칸의 글자를 확정해서 조회 알림

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Button& m_refreshButton;
        UI::CheckBox& m_autoRefreshBox;
        UI::Label& m_statusLabel;
        UI::Label& m_startPresetLabel;
        UI::ComboBox& m_startPresetBox;
        UI::TextBox& m_nameBox;          // 캐릭터 이름 입력 칸
        UI::Button& m_lookupButton;      // [조회] 버튼
        UI::Label& m_targetModeLabel;
        UI::ComboBox& m_targetModeBox;
        UI::Label& m_targetLevelLabel;
        UI::TextBox& m_targetLevelBox;
        UI::Button& m_calculateButton;   // [계산] 버튼
        UI::Label& m_summaryLabel;
        UI::Grid& m_slotGrid;
        UI::Grid& m_materialGrid;
        UI::Label& m_totalLabel;
        UI::Grid& m_routeGrid;

        Panel m_setupPanel;
        Panel m_materialPanel;
        Panel m_routePanel;
        Core::RectF m_layoutArea;
        bool m_hasLayout{};
        bool m_isUpdatingTargetText{};   // 코드에서 목표 칸을 바꾸는 중 (알림 막기)

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
        std::function<void(std::size_t)> m_onStartPresetSelected;
        std::function<void(const std::wstring&)> m_onLookup;   // 캐릭터 조회 알림
        std::function<void()> m_onCalculate;   // 계산 알림
        std::function<void(std::size_t)> m_onTargetModeSelected;
        std::function<void(const std::wstring&)> m_onTargetItemLevelChanged;
        std::function<void(std::size_t, RefineSlotColumn)> m_onSlotCellClicked;
        std::function<void(std::size_t, RefineSlotColumn, const std::wstring&)> m_onSlotCellEdited;
        std::function<void(std::size_t, RefineMaterialColumn)> m_onMaterialCellClicked;
        std::function<void(std::size_t, RefineMaterialColumn, const std::wstring&)> m_onMaterialCellEdited;
    };
}
