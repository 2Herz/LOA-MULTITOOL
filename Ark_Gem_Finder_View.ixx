export module LostArkMultiTool.Views.ArkGemFinderView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Models.BoundGemData;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.LineChart;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // 젬 표 한 행 (값이 0이면 "-"로 표시)
    struct ArkGemRow
    {
        std::wstring m_name;
        std::string m_gradeName;                  // 등급 (UTF-8, 이름 글자색)
        double m_currentMinPrice{};
        std::optional<double> m_diff;             // 직전 갱신 대비 등락 (없으면 등락 표시 없음)
        double m_yesterdayAvgPrice{};
        double m_recentPrice{};
        std::optional<double> m_dayChangeRate;    // 전일 평균가 대비 변동률(%)
    };

    // 귀속 젬 계산기 표 한 행 (젬 하나의 등급별 수량 · 단가 · 합계)
    struct BoundGemRow
    {
        std::array<int, Models::BoundGemData::GRADE_COUNT> m_quantities{};
        std::array<double, Models::BoundGemData::GRADE_COUNT> m_unitPrices{};   // 0 = 시세 없음
        double m_value{};                                                       // 수량 × 단가의 합
    };

    // 젬 시세 검색기 화면 구성 (위: 왼쪽 질서 · 오른쪽 혼돈 시세 표, 아래: 귀속 젬 계산기 = 원정대 동기화 · 캐릭터 선택 + 표)
    // 부품 배치 · 표시 형식만 알고, 조회 · 자동 갱신 일정은 ArkGemFinderManager가 맡음
    class ArkGemFinderView
    {
    public:
        explicit ArkGemFinderView(Core::Platform& platform);
        ~ArkGemFinderView();

        ArkGemFinderView(const ArkGemFinderView&) = delete;
        ArkGemFinderView& operator=(const ArkGemFinderView&) = delete;

        // ===== 사용자 동작 알림 (ArkGemFinderManager가 연결) =====
        void SetOnRefresh(std::function<void()> onRefresh);
        void SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged);
        // 귀속 젬 수량 칸을 고쳐서 확정함 (text = 입력한 글자 그대로, 해석 · 저장은 매니저가)
        void SetOnQuantityEdited(std::function<void(std::size_t gemIndex, std::size_t gradeIndex, const std::wstring& text)> onQuantityEdited);
        void SetOnSyncRoster(std::function<void(const std::wstring& mainCharacterName)> onSyncRoster);   // [원정대 동기화] · 이름 칸 Enter
        void SetOnCharacterSelected(std::function<void()> onCharacterSelected);                           // 캐릭터 선택을 바꿈

        // ===== 상단 바 =====
        void SetAutoRefreshChecked(bool isChecked) noexcept;   // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsAutoRefreshChecked() const noexcept;
        void SetRefreshEnabled(bool isEnabled) noexcept;       // 조회 중에는 [시세 갱신] 비활성
        void SetStatusText(std::wstring text);

        // ===== 시세 표 =====
        void ShowOrder(const std::vector<ArkGemRow>& rows);   // 질서의 젬
        void ShowChaos(const std::vector<ArkGemRow>& rows);    // 혼돈의 젬

        // ===== 귀속 젬 계산기 =====
        // 캐릭터 선택 목록 (0번 = ★ 원정대 전체 ★, 1번부터 캐릭터, 선택이 없으면 nullopt)
        void SetCharacterItems(std::vector<std::wstring> items, std::optional<std::size_t> selectedIndex);
        [[nodiscard]] std::optional<std::size_t> GetSelectedCharacterIndex() const noexcept;
        void SetSyncBusy(bool isBusy);                              // 동기화 중에는 버튼 비활성 + "동기화 중..."
        void SetQuantityEditable(bool isEditable);                  // 캐릭터를 골랐을 때만 수량 입력 (원정대 전체는 합계라 읽기 전용)
        void CommitEdit();                                          // 편집 중인 칸 확정 (캐릭터를 바꾸기 전)
        void ShowBoundGems(const std::vector<BoundGemRow>& rows);   // 젬 6행 (BoundGemData::GEM_NAMES 순서) + 고른 대상의 합계 행
        // 맨 아래 ★ 원정대 총 가치 ★ 행 (등급별 원정대 전체 수량 + 총 가치)
        void ShowRosterTotal(const std::array<int, Models::BoundGemData::GRADE_COUNT>& quantities, double value);
        void SetChart(std::wstring title, std::vector<UI::ChartPoint> points);   // 오른쪽 아래: 날짜별 원정대 귀속 젬 총 가치

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

        static void FillGrid(UI::Grid& grid, const std::vector<ArkGemRow>& rows);
        void InitializeBoundGemGrid();
        void Layout(const Core::RectF& contentArea);
        [[nodiscard]] Core::RectF LayoutPanel(Panel& panel, const Core::RectF& bounds) const;   // 테두리 · 제목 자리를 뺀 안쪽 영역
        void RenderPanels(Core::Renderer& renderer) const;

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Button& m_refreshButton;
        UI::CheckBox& m_autoRefreshBox;
        UI::Label& m_statusLabel;
        UI::Grid& m_orderGrid;
        UI::Grid& m_chaosGrid;
        UI::TextBox& m_nameBox;          // 대표 캐릭터 이름
        UI::Button& m_syncButton;        // [원정대 동기화]
        UI::ComboBox& m_characterBox;    // 캐릭터 선택 (★ 원정대 전체 ★ + 캐릭터)
        UI::Grid& m_boundGemGrid;
        UI::LineChart& m_chart;          // 원정대 귀속 젬 총 가치 그래프

        Panel m_orderPanel;
        Panel m_chaosPanel;
        Panel m_boundGemPanel;
        Panel m_chartPanel;
        Core::RectF m_layoutArea;
        bool m_hasLayout{};
        bool m_isQuantityEditable{ true };   // 수량 칸 편집 가능 여부 (원정대 전체를 고르면 false)

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
        std::function<void(std::size_t, std::size_t, const std::wstring&)> m_onQuantityEdited;
        std::function<void(const std::wstring&)> m_onSyncRoster;
        std::function<void()> m_onCharacterSelected;
    };
}
