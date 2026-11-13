export module LostArkMultiTool.Views.RefineBookFinderView;

import std;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.CheckBox;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.TextBox;

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

    // 강화책 검색기 화면 구성 (통계 카드 · 검색/분류/정렬 줄 · LIVE MARKET 표 하나)
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

        // ===== 필터 줄 =====
        void SetAutoRefreshChecked(bool isChecked) noexcept;   // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsAutoRefreshChecked() const noexcept;
        void SetRefreshEnabled(bool isEnabled);                // 조회 중에는 [시세 갱신] 비활성 + "조회 중..."
        void SetStatusText(std::wstring text);                 // 표 머리말 오른쪽 글자 (마지막 갱신 · 조회 중)

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
        // 표에 올릴 책 한 권 (분류 + 시세)
        struct Entry
        {
            bool m_isMetallurgy{};
            RefineBookRow m_row;
        };

        void ApplyFilter();                                    // 검색어 · 분류 · 정렬에 맞게 표 다시 채우기
        void FillGrid(const std::vector<const Entry*>& entries);
        void ClearSearch();                                    // 검색어 지우고 입력칸으로 포커스
        void DrawStatCards(Core::Renderer& renderer) const;
        void Layout(const Core::RectF& contentArea);

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::TextBox& m_searchBox;
        UI::Button& m_clearButton;
        UI::ComboBox& m_kindBox;
        UI::ComboBox& m_sortBox;
        UI::CheckBox& m_autoRefreshBox;
        UI::Button& m_refreshButton;
        UI::Grid& m_grid;

        std::vector<Entry> m_metallurgyEntries;   // 서비스가 정한 순서 그대로
        std::vector<Entry> m_tailoringEntries;
        std::size_t m_shownCount{};               // 지금 표에 보이는 행 수
        std::wstring m_statusText;

        // 마지막 배치 (Render에서 카드를 그림)
        Core::RectF m_layoutArea;
        bool m_hasLayout{};
        std::vector<Core::RectF> m_statRects;
        Core::RectF m_filterRect;
        Core::RectF m_marketRect;
        Core::RectF m_emptyRect;                  // 결과가 없을 때 안내가 들어가는 자리
        Core::RectF m_footerRect;

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
    };
}
