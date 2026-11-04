export module LostArkMultiTool.Views.CalculatorView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.LineChart;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // 재료 등급 (표의 상위 · 하위 행)
    enum class MaterialTier
    {
        TIER_TOP,      // 상위 재료 (1730 이상: 결정 · 위대한 · 상급)
        TIER_BOTTOM    // 하위 재료
    };

    // 귀속재료 계산기 화면 구성 (xmind: 화면구성 · 렌더링)
    // 부품 배치 · 표의 칸 위치 · 표시 형식만 알고, 계산 · 저장 · API 요청은 CalculatorManager가 맡음
    // 재료 번호(materialIndex)는 APPConstants::MAT_KEYS 번호와 같음 (1 ~ 7, 표의 재료 열)
    class CalculatorView
    {
    public:
        explicit CalculatorView(Core::Platform& platform);
        ~CalculatorView();

        CalculatorView(const CalculatorView&) = delete;
        CalculatorView& operator=(const CalculatorView&) = delete;

        // ===== 사용자 동작 알림 (CalculatorManager가 연결) =====
        void SetOnSyncRoster(std::function<void(const std::wstring& mainCharacterName)> onSyncRoster);
        void SetOnRefresh(std::function<void()> onRefresh);
        void SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged);
        void SetOnCharacterSelected(std::function<void()> onCharacterSelected);
        void SetOnQuantityEdited(std::function<void(MaterialTier tier, std::size_t materialIndex, double quantity)> onQuantityEdited);

        // ===== 캐릭터 목록 =====
        void SetCharacterItems(std::vector<std::wstring> items, std::optional<std::size_t> selectedIndex);   // 캐릭터가 없으면 nullopt
        [[nodiscard]] std::optional<std::size_t> GetSelectedCharacterIndex() const noexcept;

        // ===== 표 =====
        void ShowPrice(MaterialTier tier, std::size_t materialIndex, double current, double diff);
        // 보유 수량 + 지난 입력 대비 증감 (증가 빨강 ▲ / 감소 파랑 ▼, 증감이 0이면 수량만)
        void ShowQuantity(MaterialTier tier, std::size_t materialIndex, double quantity, double change);
        void ShowMaterialValue(MaterialTier tier, std::size_t materialIndex, double value);
        void ShowValueTotals(double topTotal, double bottomTotal);   // 고른 캐릭터의 상위 · 하위 재료값 합계 + ★ 캐릭터 자산 ★
        void ShowRosterTotal(double rosterTotal);                    // 표 맨 아래 ★ 원정대 자산 ★ (모든 캐릭터의 상위 + 하위)
        // ★ 원정대 자산 ★ 행의 재료 칸 = 원정대 전체 보유수량 (상위 · 하위가 모두 있으면 "상위 / 하위")
        void ShowRosterQuantity(std::size_t materialIndex, double topQuantity, double bottomQuantity);
        void SetQuantityEditable(MaterialTier tier, bool isEditable);
        void CommitEdit();                         // 편집 중인 칸 확정 (시세 갱신 전, 원본 EndEdit)

        // ===== 그래프 · 버튼 상태 =====
        void SetChart(std::wstring title, std::vector<UI::ChartPoint> points);
        void SetRefreshBusy(bool isBusy);
        void SetSyncBusy(bool isBusy);
        void SetAutoRefreshChecked(bool isChecked) noexcept;

        // ===== 프레임 =====
        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때 (편집 확정 · IME 끔)
        void RestoreFocus();

    private:
        void InitializeGrid();
        void Layout(const Core::RectF& contentArea);
        void HandleCellEdited(std::size_t row, std::size_t column, const std::wstring& text);
        void ApplyQuantityStyle(MaterialTier tier, std::size_t materialIndex);   // 잠금 · 증감에 맞는 수량 칸 색

        static constexpr std::size_t TIER_COUNT{ 2 };   // 상위 · 하위

        UI::ControlHost m_host;

        // 수량 칸 상태 (잠금 · 증감 → 칸 색을 정할 때 함께 봄)
        std::array<bool, TIER_COUNT> m_isQuantityEditable{};
        std::array<std::array<double, Core::APPConstants::MAT_COLUMN_COUNT>, TIER_COUNT> m_quantityChanges{};

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::TextBox& m_nameBox;
        UI::Button& m_syncButton;
        UI::ComboBox& m_characterBox;
        UI::CheckBox& m_autoRefreshBox;
        UI::Button& m_refreshButton;
        UI::Grid& m_grid;
        UI::LineChart& m_chart;

        Core::RectF m_layoutArea;
        bool m_hasLayout{};

        std::function<void(const std::wstring&)> m_onSyncRoster;
        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
        std::function<void()> m_onCharacterSelected;
        std::function<void(MaterialTier, std::size_t, double)> m_onQuantityEdited;
    };
}