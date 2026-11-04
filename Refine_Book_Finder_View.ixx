export module LostArkMultiTool.Views.RefineBookFinderView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;

export namespace LostArkMultiTool::Views
{
    // 강화책 표 한 행 (값이 0이면 "-"로 표시)
    struct RefineBookRow
    {
        std::wstring m_name;
        std::string m_gradeName;                  // 등급 (UTF-8, 이름 글자색)
        double m_currentMinPrice{};
        std::optional<double> m_diff;             // 직전 갱신 대비 등락 (없으면 등락 표시 없음)
        double m_yesterdayAvgPrice{};
        double m_recentPrice{};
        std::optional<double> m_dayChangeRate;    // 전일 평균가 대비 변동률(%)
    };

    // 강화책 검색기 화면 구성 (왼쪽 야금술 · 오른쪽 재봉술 표)
    // 부품 배치 · 표시 형식만 알고, 조회 · 자동 갱신 일정은 RefineBookFinderManager가 맡음
    class RefineBookFinderView
    {
    public:
        explicit RefineBookFinderView(Core::Platform& platform);
        ~RefineBookFinderView();

        RefineBookFinderView(const RefineBookFinderView&) = delete;
        RefineBookFinderView& operator=(const RefineBookFinderView&) = delete;

        // ===== 사용자 동작 알림 (RefineBookFinderManager가 연결) =====
        void SetOnRefresh(std::function<void()> onRefresh);
        void SetOnAutoRefreshChanged(std::function<void(bool isEnabled)> onAutoRefreshChanged);

        // ===== 상단 바 =====
        void SetAutoRefreshChecked(bool isChecked) noexcept;   // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsAutoRefreshChecked() const noexcept;
        void SetRefreshEnabled(bool isEnabled) noexcept;       // 조회 중에는 [시세 갱신] 비활성
        void SetStatusText(std::wstring text);

        // ===== 시세 표 =====
        void ShowMetallurgy(const std::vector<RefineBookRow>& rows);   // 야금술 (무기)
        void ShowTailoring(const std::vector<RefineBookRow>& rows);    // 재봉술 (방어구)

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

        static void FillGrid(UI::Grid& grid, const std::vector<RefineBookRow>& rows);
        void Layout(const Core::RectF& contentArea);
        [[nodiscard]] Core::RectF LayoutPanel(Panel& panel, const Core::RectF& bounds) const;   // 테두리 · 제목 자리를 뺀 안쪽 영역
        void RenderPanels(Core::Renderer& renderer) const;

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Button& m_refreshButton;
        UI::CheckBox& m_autoRefreshBox;
        UI::Label& m_statusLabel;
        UI::Grid& m_metallurgyGrid;
        UI::Grid& m_tailoringGrid;

        Panel m_metallurgyPanel;
        Panel m_tailoringPanel;
        Core::RectF m_layoutArea;
        bool m_hasLayout{};

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
    };
}
