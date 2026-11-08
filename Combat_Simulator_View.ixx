export module LostArkMultiTool.Views.CombatSimulatorView;  // 전투력 시뮬레이터 화면 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Core.Platform;  // 플랫폼 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Views.UI.Button;  // 버튼 부품 가져오기
import LostArkMultiTool.Views.UI.ComboBox;  // 선택 상자 부품 가져오기
import LostArkMultiTool.Views.UI.ControlHost;  // 부품 호스트 가져오기
import LostArkMultiTool.Views.UI.Grid;  // 표 부품 가져오기
import LostArkMultiTool.Views.UI.Label;  // 글자 상자 부품 가져오기
import LostArkMultiTool.Views.UI.TextBox;  // 입력 상자 부품 가져오기

export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 장비 표 열 (매니저가 고친 칸을 구분할 때 사용)
    enum class CombatGearColumn : std::size_t
    {
        COLUMN_PART,  // 부위 열
        COLUMN_ITEM,  // 장비 이름 열
        COLUMN_CURRENT_ENHANCEMENT,  // 현재 일반 재련 열
        COLUMN_CURRENT_ADVANCED,  // 현재 상급 재련 열
        COLUMN_TARGET_ENHANCEMENT,    // 직접 입력
        COLUMN_TARGET_ADVANCED,       // 직접 입력 (결단 · 업화만)
        COLUMN_COUNT  // 열 개수 (끝 표식)
    };

    // 장비 표 한 행 (글자는 매니저가 만들어 넘김)
    struct CombatGearRow
    {
        std::wstring m_partName;  // 부위 이름
        std::wstring m_itemName;                 // 장비 이름 (없으면 "-")
        std::string m_gradeName;                 // 등급 글자색 ("유물" · "고대" · "에스더", UTF-8)
        std::wstring m_currentEnhancement;  // 현재 일반 재련 글자
        std::wstring m_currentAdvanced;  // 현재 상급 재련 글자
        std::wstring m_targetEnhancement;  // 목표 일반 재련 글자
        std::wstring m_targetAdvanced;  // 목표 상급 재련 글자
        bool m_isTargetEditable{};               // 계수 표가 있는 장비만
        bool m_isTargetAdvancedEditable{};       // 상급 재련을 넣을 수 있는 단계일 때만
        bool m_hasChange{};                      // 목표가 현재와 다름 (목표 칸 강조)
    };

    // 보석 표 열
    enum class CombatGemColumn : std::size_t
    {
        COLUMN_SLOT,  // 보석 칸 번호 열
        COLUMN_KIND,  // 보석 종류 열
        COLUMN_CURRENT_LEVEL,  // 현재 레벨 열
        COLUMN_TARGET_LEVEL,          // 직접 입력 (1 ~ 10)
        COLUMN_COUNT  // 열 개수 (끝 표식)
    };

    // 보석 표 한 행
    struct CombatGemRow
    {
        std::wstring m_slot;                     // "1" ~ "11"
        std::wstring m_kind;                     // "겁화" · "작열" (모르면 "-")
        std::wstring m_currentLevel;  // 현재 레벨 글자
        std::wstring m_targetLevel;  // 목표 레벨 글자
        bool m_hasChange{};                      // 목표가 현재와 다름 (목표 칸 강조)
    };

    // 목록형 카드 (악세서리 · 아크 그리드 · 팔찌/스톤/각인서 · 아크 패시브/아바타)
    enum class CombatCard : std::size_t
    {
        CARD_ACCESSORY,  // 악세서리 카드
        CARD_ARK_GRID,  // 아크 그리드 카드
        CARD_BRACELET,  // 팔찌 · 스톤 · 각인서 카드
        CARD_PASSIVE,  // 아크 패시브 · 아바타 카드
        CARD_COUNT  // 카드 개수 (끝 표식)
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
        std::wstring m_label;  // 행 이름
        std::wstring m_current;  // 현재 값 글자
        std::wstring m_target;  // 목표 값 글자
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
        bool m_isUp{};  // 예상 전투력이 현재보다 높은지
        bool m_isDown{};  // 예상 전투력이 현재보다 낮은지
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
        explicit CombatSimulatorView(Core::Platform& platform);  // 생성자: 플랫폼으로 부품을 만들어 배치
        ~CombatSimulatorView();  // 소멸자

        CombatSimulatorView(const CombatSimulatorView&) = delete;  // 복사 생성 금지 (부품 참조 보유)
        CombatSimulatorView& operator=(const CombatSimulatorView&) = delete;  // 복사 대입 금지

        // ===== 사용자 동작 알림 (CombatSimulatorManager가 연결) =====
        void SetOnSearch(std::function<void(const std::wstring& characterName)> onSearch);  // 조회 요청 알림 연결
        void SetOnGearCellEdited(std::function<void(std::size_t row, CombatGearColumn column, const std::wstring& text)> onGearCellEdited);  // 장비 칸 편집 알림 연결
        void SetOnQualityChanged(std::function<void(const std::wstring& text)> onQualityChanged);  // 무기 품질 변경 알림 연결
        void SetOnGemTargetSelected(std::function<void(std::size_t index)> onGemTargetSelected);  // 보석 목표 선택 알림 연결
        void SetOnGemCellEdited(std::function<void(std::size_t row, const std::wstring& text)> onGemCellEdited);   // 목표 레벨 칸만
        void SetOnCardEdited(std::function<void(CombatCard card, std::size_t row, const std::wstring& text)> onCardEdited);   // 숫자 입력 또는 선택한 선택지 글자

        // ===== 상단 바 =====
        void SetCharacterName(std::wstring name);  // 캐릭터 이름 입력 칸 설정
        void SetSearchBusy(bool isBusy);  // 조회 중 표시 설정 (버튼 · 입력 잠금)
        void SetStatusText(std::wstring text, bool isError);  // 상태 글자 설정 (오류면 빨간색)

        // ===== 장비 · 품질 · 보석 =====
        void ShowGears(const std::vector<CombatGearRow>& rows);  // 장비 표 채우기
        void SetQualityText(std::wstring currentText);     // "무기 품질  현재 95  →  목표"
        void SetQualityTarget(std::wstring text);          // 코드에서 바꿈 (알림 없음)
        void SetQualityEnabled(bool isEnabled) noexcept;   // 에스더 무기 · 조회 전이면 비활성
        void SetGemText(std::wstring text);                // "보석 11개 · 10레벨 5개 · 9레벨 6개"
        void SetGemTargetItems(std::vector<std::wstring> items);  // 보석 전체 목표 선택지 설정
        void SetGemTargetIndex(std::size_t index);         // 코드에서 바꿈 (알림 없음)
        void SetGemTargetEnabled(bool isEnabled) noexcept;  // 보석 전체 목표 선택 상자 활성 여부 설정
        void ShowGems(const std::vector<CombatGemRow>& rows);   // 보석 하나씩 목표 레벨 (모든 보석 목표 뒤에 개별로 덮어씀)

        // ===== 목록형 카드 =====
        void ShowCard(CombatCard card, std::vector<CombatListRow> rows);  // 목록형 카드 채우기

        // ===== 결과 =====
        void ShowResult(const CombatResultText& result);  // 결과 상자 채우기

        // ===== 프레임 =====
        void Update(double deltaSeconds);  // 프레임 갱신
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);  // 화면 그리기
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);  // 입력 처리 (처리했으면 true)
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;  // 마우스 커서 모양 조회
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때
        void RestoreFocus();  // 입력 포커스 복원

    private:  // 비공개 멤버 시작
        // 묶음 상자 (테두리 + 제목)
        struct Panel
        {
            Core::RectF m_bounds;  // 상자 영역
            std::wstring m_title;  // 상자 제목
        };

        // 목록형 카드 하나 (제목 상자 + 선택 상자 + 선택한 행 안내 + 표)
        struct ListCard
        {
            Panel m_panel;  // 카드 제목 상자
            UI::ComboBox* m_combo{};  // 선택 상자
            UI::Label* m_hint{};  // 선택한 행 안내
            UI::Grid* m_grid{};  // 행 표
            std::vector<CombatListRow> m_rows;  // 행 목록
        };

        void Layout(const Core::RectF& contentArea);  // 전체 배치 계산
        void LayoutListCard(ListCard& card, const Core::RectF& bounds);  // 목록형 카드 하나 배치
        void SyncCardEditor(std::size_t cardIndex);   // 선택한 행에 맞춰 선택 상자 · 안내 갱신
        void OnCardChoice(std::size_t cardIndex);     // 선택 상자에서 고름
        [[nodiscard]] Core::RectF LayoutPanel(Panel& panel, const Core::RectF& bounds) const;   // 테두리 · 제목 자리를 뺀 안쪽 영역
        void RenderPanels(Core::Renderer& renderer) const;  // 상자 테두리 · 제목 그리기
        void RequestSearch();  // 조회 요청 보내기

        UI::ControlHost m_host;  // 부품 호스트 (부품 소유 · 포커스 관리)

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Label& m_nameLabel;  // 이름 안내 글자
        UI::TextBox& m_nameBox;  // 이름 입력 칸
        UI::Button& m_searchButton;  // 조회 버튼
        UI::Label& m_statusLabel;  // 상태 글자
        UI::Grid& m_gearGrid;  // 장비 표
        UI::Label& m_qualityLabel;  // 무기 품질 안내 글자
        UI::TextBox& m_qualityBox;  // 무기 품질 입력 칸
        UI::Label& m_gemLabel;  // 보석 요약 글자
        UI::Label& m_gemTargetLabel;  // 보석 전체 목표 안내 글자
        UI::ComboBox& m_gemTargetBox;  // 보석 전체 목표 선택 상자
        UI::Grid& m_gemGrid;  // 보석 표
        std::array<ListCard, static_cast<std::size_t>(CombatCard::CARD_COUNT)> m_cards;  // 목록형 카드 배열
        UI::Label& m_currentPowerLabel;  // 현재 전투력 글자
        UI::Label& m_predictedPowerLabel;  // 예상 전투력 글자
        UI::Label& m_changeLabel;  // 증감 글자
        UI::Label& m_itemLevelLabel;  // 아이템 레벨 글자
        UI::Label& m_factorLabel;  // 배율 내역 글자
        UI::Label& m_noteLabel;  // 안내 글자

        Panel m_barPanel;      // 맨 위 결과 바
        Panel m_gearPanel;
        Panel m_optionPanel;
        Core::RectF m_layoutArea;  // 배치 영역
        bool m_hasLayout{};  // 배치를 계산했는지 여부
        bool m_isUpdatingQualityText{};   // 코드에서 품질 칸을 바꾸는 중 (알림 막기)

        std::function<void(const std::wstring&)> m_onSearch;  // 조회 알림 콜백
        std::function<void(std::size_t, CombatGearColumn, const std::wstring&)> m_onGearCellEdited;  // 장비 칸 편집 알림 콜백
        std::function<void(const std::wstring&)> m_onQualityChanged;  // 무기 품질 변경 알림 콜백
        std::function<void(std::size_t)> m_onGemTargetSelected;  // 보석 목표 선택 알림 콜백
        std::function<void(std::size_t, const std::wstring&)> m_onGemCellEdited;  // 보석 칸 편집 알림 콜백
        std::function<void(CombatCard, std::size_t, const std::wstring&)> m_onCardEdited;  // 목록형 카드 편집 알림 콜백
    };
}
