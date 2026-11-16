export module LostArkMultiTool.Views.AccessoryFinderView;

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.Platform;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Models.AccessoryListing;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ComboBox;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Grid;
import LostArkMultiTool.Views.UI.Label;
import LostArkMultiTool.Views.UI.TextBox;

export namespace LostArkMultiTool::Views
{
    // 악세 검색기 화면 구성
    // 부품 배치 · 표시 형식만 알고, 옵션 불러오기 · 검색 · 페이지 이동은 AccessoryFinderManager가 맡음
    class AccessoryFinderView
    {
    public:
        explicit AccessoryFinderView(Core::Platform& platform);
        ~AccessoryFinderView();

        AccessoryFinderView(const AccessoryFinderView&) = delete;
        AccessoryFinderView& operator=(const AccessoryFinderView&) = delete;

        // ===== 사용자 동작 알림 (AccessoryFinderManager가 연결) =====
        void SetOnSearch(std::function<void()> onSearch);
        void SetOnPageMove(std::function<void(int pageStep)> onPageMove);                     // -1 = 이전, +1 = 다음
        void SetOnOptionChanged(std::function<void(std::size_t slotIndex)> onOptionChanged);   // 연마 효과 칸의 옵션을 바꿈
        void SetOnCategoryChanged(std::function<void()> onCategoryChanged);                     // 부위를 바꿈 (유효옵션 목록이 달라짐)

        // ===== 검색 조건 =====
        [[nodiscard]] int GetCategoryCode() const noexcept;
        [[nodiscard]] std::optional<int> GetMinQuality() const noexcept;   // nullopt = 품질 전체
        [[nodiscard]] std::optional<int> GetMinMainStat() const;           // 주스탯(힘 · 민첩 · 지능) 최소 수치 (비었거나 0이면 nullopt)

        // 연마 효과 칸 (옵션 이름 목록은 모든 칸 공통, 값 목록은 칸마다)
        void SetOptionNames(const std::vector<std::wstring>& optionNames);   // 모든 칸이 "선택 안 함"으로 돌아감
        // 값 칸이 "값 전체"로 돌아감 (valueColors = 항목마다 글자색, 비어 있으면 기본색)
        void SetOptionValues(std::size_t slotIndex, const std::vector<std::wstring>& valueTexts,
            const std::vector<std::optional<Core::Color>>& valueColors = {});
        [[nodiscard]] std::optional<std::size_t> GetSelectedOption(std::size_t slotIndex) const noexcept;   // 옵션 이름 목록의 번호 (선택 안 함 = nullopt)
        [[nodiscard]] std::optional<std::size_t> GetSelectedValue(std::size_t slotIndex) const noexcept;    // 값 목록의 번호 (값 전체 = nullopt)

        // ===== 상태 =====
        void SetBusy(bool isBusy) noexcept;   // 조회 중에는 검색 · 페이지 버튼 비활성
        void SetStatusText(std::wstring text);
        void SetPageInfo(std::wstring text, bool hasPrevious, bool hasNext);

        // ===== 결과 표 =====
        // categoryCode = 이 결과를 검색한 부위, totalCount = 조건에 맞는 전체 매물 수, searchedAtText = 검색을 마친 시각
        void ShowListings(const std::vector<Models::AccessoryListing>& listings, int categoryCode, int totalCount, std::wstring searchedAtText);

        // ===== 프레임 =====
        void Update(double deltaSeconds);
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때
        void RestoreFocus();

    private:
        // 연마 효과 조건 칸 하나 (옵션 + 최소 값)
        struct OptionSlot
        {
            UI::ComboBox* m_optionBox{};   // m_host가 소유
            UI::ComboBox* m_valueBox{};    // m_host가 소유
        };

        // 연마 효과 칸 부품 만들기 (생성자 초기화 목록에서 호출 → 옵션 · 값 상자가 Tab 순서대로 m_host에 추가됨)
        [[nodiscard]] std::array<OptionSlot, Core::APPConstants::MAX_ACCESSORY_OPTION_FILTERS> MakeOptionSlots();

        void Layout(const Core::RectF& contentArea);
        void RenderCards(Core::Renderer& renderer) const;
        void RenderEmptyState(Core::Renderer& renderer) const;   // 매물이 없을 때 표 위 안내
        void UpdatePageButtons() noexcept;

        UI::ControlHost m_host;

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Label& m_categoryLabel;
        UI::ComboBox& m_categoryBox;
        UI::Label& m_qualityLabel;
        UI::ComboBox& m_qualityBox;
        UI::Label& m_mainStatLabel;      // "주스탯"
        UI::TextBox& m_mainStatBox;      // 주스탯 최소 수치 (숫자만, 비우면 조건 없음)
        UI::Button& m_searchButton;
        UI::Label& m_statusLabel;
        UI::Label& m_optionLabel;
        std::array<OptionSlot, Core::APPConstants::MAX_ACCESSORY_OPTION_FILTERS> m_optionSlots;
        UI::Grid& m_listingGrid;
        UI::Button& m_previousButton;
        UI::Label& m_pageLabel;
        UI::Button& m_nextButton;

        bool m_isBusy{};
        bool m_hasPrevious{};
        bool m_hasNext{};

        // 새 디자인 카드 영역 (Layout이 계산, Render가 그림)
        static constexpr std::size_t STAT_CARD_COUNT{ 4 };   // 위쪽 통계 카드 수
        std::array<Core::RectF, STAT_CARD_COUNT> m_statBounds;   // 통계 카드 칸
        Core::RectF m_filterBounds;    // 검색 조건 카드
        Core::RectF m_resultBounds;    // 결과 카드 (머리말 + 표 + 아래 줄)
        Core::RectF m_gridBounds;      // 결과 표 칸
        Core::RectF m_footerBounds;    // 표 아래 줄 (설명 + 페이지 이동)
        Core::RectF m_layoutArea;
        bool m_hasLayout{};

        // 통계 카드에 보여 줄 실제 검색 결과 값
        bool m_hasSearched{};                    // 한 번이라도 결과를 받았는지
        int m_totalCount{};                      // 조건에 맞는 전체 매물 수
        std::optional<std::int64_t> m_lowestPrice;    // 이 페이지 즉시 구매가 최저
        std::optional<std::int64_t> m_averagePrice;   // 이 페이지 즉시 구매가 평균
        std::wstring m_searchedAtText;           // 마지막 검색 시각

        std::function<void()> m_onSearch;
        std::function<void(int)> m_onPageMove;
        std::function<void(std::size_t)> m_onOptionChanged;
        std::function<void()> m_onCategoryChanged;
    };
}
