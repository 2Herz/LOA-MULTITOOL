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
        std::string m_key;                        // 일별 기록을 찾는 키 (UTF-8 젬 이름, 화면에는 표시하지 않음)
        std::vector<double> m_trend;              // 일별 최저가 기록 (오래된 날 → 최근, 추이 선 · 2개 미만이면 그리지 않음)
    };

    // 귀속 젬 계산기 표 한 행 (젬 하나의 등급별 수량 · 단가 · 합계)
    struct BoundGemRow
    {
        std::array<int, Models::BoundGemData::GRADE_COUNT> m_quantities{};
        std::array<double, Models::BoundGemData::GRADE_COUNT> m_unitPrices{};   // 0 = 시세 없음
        double m_value{};                                                       // 수량 × 단가의 합
    };

    // 젬 시세 검색기 화면 구성 (통계 카드 · 검색/분류/정렬 줄 · LIVE MARKET 표 하나, 아래: 귀속 젬 계산기 카드 + 그래프 카드 둘)
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
        void SetOnVisionChanged(std::function<void(bool isEnabled)> onVisionChanged);                     // [화면 인식] 켜기 · 끄기
        void SetVisionChecked(bool isChecked) noexcept;                                                   // [화면 인식] 표시 (알림 없음)
        void SelectCharacterIndex(std::size_t index);                                                     // 캐릭터 고르기 (화면 인식이 현재 캐릭터로 바꿀 때, 알림 없음)
        // 시세 표에서 행을 골랐음 (isOrder = 질서의 젬인지, row = 그 분류 목록 안 번호, 그래프 대상)
        void SetOnPriceRowSelected(std::function<void(bool isOrder, std::size_t row)> onPriceRowSelected);

        // ===== 필터 줄 =====
        void SetAutoRefreshChecked(bool isChecked) noexcept;   // 코드에서 바꿈 (알림 없음)
        [[nodiscard]] bool IsAutoRefreshChecked() const noexcept;
        void SetRefreshEnabled(bool isEnabled);                // 조회 중에는 [시세 갱신] 비활성 + "조회 중..."
        void SetStatusText(std::wstring text);                 // 표 머리말 · 아래 줄 오른쪽 글자 (마지막 갱신 · 조회 중)

        // ===== 시세 표 =====
        void ShowOrder(const std::vector<ArkGemRow>& rows);   // 질서의 젬
        void ShowChaos(const std::vector<ArkGemRow>& rows);    // 혼돈의 젬
        void SelectOrderRow(std::optional<std::size_t> row);    // 질서 젬 선택을 코드에서 바꿈 (알림 없음, 값이 없으면 질서 젬 선택만 해제)
        void SelectChaosRow(std::optional<std::size_t> row);    // 혼돈 젬 선택을 코드에서 바꿈 (알림 없음)
        void SetPriceChart(std::wstring title, std::vector<UI::ChartPoint> points);   // 오른쪽 위: 고른 젬의 일별 최저가

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
        // 표에 올릴 젬 하나 (분류 + 시세)
        struct Entry
        {
            bool m_isOrder{};            // 질서의 젬인지 (false = 혼돈)
            std::size_t m_kindIndex{};   // 매니저가 넘긴 그 분류 목록에서의 번호 (그래프 대상 찾기용)
            ArkGemRow m_row;
        };

        void ApplyFilter();                                  // 검색어 · 분류 · 정렬에 맞게 표 다시 채우기
        void FillGrid(const std::vector<const Entry*>& entries);
        void ClearSearch();                                  // 검색어 지우고 입력칸으로 포커스
        void HandlePriceRowSelected();                       // 표에서 행을 고르면 그 젬을 매니저에 알림
        void ReselectRow();                                  // 기억해 둔 젬이 보이는 표에 있으면 그 행을 선택
        void InitializeBoundGemGrid();
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
        UI::Grid& m_marketGrid;
        UI::TextBox& m_nameBox;          // 대표 캐릭터 이름
        UI::Button& m_syncButton;        // [원정대 동기화]
        UI::ComboBox& m_characterBox;    // 캐릭터 선택 (★ 원정대 전체 ★ + 캐릭터)
        UI::CheckBox& m_visionBox;       // [화면 인식] (귀속재료 계산기와 같은 설정)
        UI::Grid& m_boundGemGrid;
        UI::LineChart& m_chart;          // 원정대 귀속 젬 총 가치 그래프
        UI::LineChart& m_priceChart;     // 고른 젬의 일별 최저가 그래프

        std::vector<Entry> m_orderEntries;        // 서비스가 정한 순서 그대로
        std::vector<Entry> m_chaosEntries;
        std::vector<const Entry*> m_shownEntries; // 지금 표 행 순서대로 (두 목록 안을 가리킴)
        std::optional<std::pair<bool, std::size_t>> m_selectedGem;   // 고른 젬 (질서 여부, 분류 안 번호) → 걸러 내도 기억
        double m_rosterValue{};                   // 원정대 귀속 젬 총 가치 (통계 카드)
        std::wstring m_statusText;

        // 마지막 배치 (Render에서 카드를 그림)
        Core::RectF m_layoutArea;
        bool m_hasLayout{};
        std::vector<Core::RectF> m_statRects;
        Core::RectF m_filterRect;
        Core::RectF m_marketRect;
        Core::RectF m_emptyRect;
        Core::RectF m_footerRect;
        Core::RectF m_calculatorRect;
        Core::RectF m_priceChartRect;
        Core::RectF m_chartRect;
        bool m_isQuantityEditable{ true };   // 수량 칸 편집 가능 여부 (원정대 전체를 고르면 false)

        std::function<void()> m_onRefresh;
        std::function<void(bool)> m_onAutoRefreshChanged;
        std::function<void(std::size_t, std::size_t, const std::wstring&)> m_onQuantityEdited;
        std::function<void(const std::wstring&)> m_onSyncRoster;
        std::function<void()> m_onCharacterSelected;
        std::function<void(bool)> m_onVisionChanged;   // [화면 인식] 알림
        std::function<void(bool, std::size_t)> m_onPriceRowSelected;
    };
}
