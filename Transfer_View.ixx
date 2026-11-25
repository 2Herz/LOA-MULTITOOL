export module LostArkMultiTool.Views.TransferView;  // 2부 이관 화면 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Core.Platform;  // 플랫폼 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Views.UI.Button;  // 버튼 부품 가져오기
import LostArkMultiTool.Views.UI.ControlHost;  // 부품 호스트 가져오기
import LostArkMultiTool.Views.UI.Grid;  // 표 부품 가져오기
import LostArkMultiTool.Views.UI.Label;  // 글자 상자 부품 가져오기
import LostArkMultiTool.Views.UI.TextBox;  // 입력 상자 부품 가져오기

export namespace LostArkMultiTool::Views  // 화면 네임스페이스 내보내기 시작
{
    // 이관 화면의 카드 (위 줄: 장비 · 장신구 슬롯 · 장신구 주문서, 아래 줄: 팔찌 · 보석/스톤 · 그 밖)
    enum class TransferCard : std::size_t
    {
        CARD_ARMOR,        // 장비 쪽 8칸 + 슬롯 재련
        CARD_ACCESSORY,    // 장신구 쪽 8칸
        CARD_SCROLL,       // 장신구 → 장신구 주문서
        CARD_BRACELET,     // 팔찌 → 인장 주문서
        CARD_GEM_STONE,    // 보석 → 광휘 · 성휘, 스톤 → 각인 업그레이드
        CARD_OTHER,        // 에스더 무기 · 품질/카르마 · 그 밖 변경
        CARD_COUNT         // 카드 개수 (끝 표식)
    };

    // 칸 글자 모양
    enum class TransferTone
    {
        TONE_NORMAL,       // 일반 글자
        TONE_LABEL,        // 왼쪽 이름 칸 (굵게)
        TONE_GRADE,        // 등급 색 (m_gradeName)
        TONE_HIGHLIGHT,    // 강조 (이관 결과 · 좋은 등급)
        TONE_MUTED,        // 흐린 글자 (대상 아님 · 안내 문구)
        TONE_HEADER        // 구분 제목 행
    };

    // 표 칸 하나 (글자는 매니저가 만들어 넘김)
    struct TransferCell
    {
        std::wstring m_text;                               // 칸 글자
        TransferTone m_tone{ TransferTone::TONE_NORMAL };  // 글자 모양
        std::string m_gradeName;                           // TONE_GRADE일 때 등급 이름 (UTF-8, "고대" 등)
    };
    using TransferRow = std::vector<TransferCell>;         // 표 행 하나 (열 수만큼)

    // 맨 위 요약 글자
    struct TransferSummaryText
    {
        std::wstring m_currentLevel;      // 지금 평균 아이템 레벨
        std::wstring m_transferLevel;     // 이관 뒤 16부위 평균 (추정)
        std::wstring m_scroll;            // 가장 좋은 장신구 주문서 등급
        std::wstring m_gem;               // 성휘로 만들 수 있는 수
    };

    // 2부 이관 화면 구성
    // 위 결과 카드: 캐릭터 이름 + [조회] + 상태 / 지금 평균 → 이관 뒤 평균 · 주문서 · 성휘
    // 아래 3 x 2 카드: 장비 · 장신구 슬롯 · 장신구 주문서 / 팔찌 · 보석 · 스톤 · 그 밖
    // 부품 배치 · 표시 형식만 알고, 조회 · 계산은 TransferManager가 맡음
    class TransferView
    {
    public:
        explicit TransferView(Core::Platform& platform);  // 생성자: 플랫폼으로 부품을 만들어 배치
        ~TransferView();  // 소멸자

        TransferView(const TransferView&) = delete;             // 복사 생성 금지 (부품 참조 보유)
        TransferView& operator=(const TransferView&) = delete;  // 복사 대입 금지

        // ===== 사용자 동작 알림 (TransferManager가 연결) =====
        void SetOnSearch(std::function<void(const std::wstring& characterName)> onSearch);  // 조회 요청 알림 연결

        // ===== 위 결과 카드 =====
        void SetCharacterName(std::wstring name);                // 캐릭터 이름 입력 칸 설정
        void SetSearchBusy(bool isBusy);                         // 조회 중 표시 (버튼 잠금)
        void SetStatusText(std::wstring text, bool isError);     // 상태 글자 (오류면 빨간색)
        void ShowSummary(const TransferSummaryText& summary);    // 요약 큰 글자

        // ===== 카드 =====
        void ShowCard(TransferCard card, const std::vector<TransferRow>& rows);   // 카드 표 채우기
        void SetCardHint(TransferCard card, std::wstring hint);                   // 카드 머리말 오른쪽 안내
        void SetNote(std::wstring note);                                          // 맨 아래 안내 문구

        // ===== 프레임 =====
        void Update(double deltaSeconds);  // 프레임 갱신
        void Render(Core::Renderer& renderer, const Core::RectF& contentArea);  // 화면 그리기
        [[nodiscard]] bool HandleInput(const Core::InputEvent& event);  // 입력 처리 (처리했으면 true)
        [[nodiscard]] Core::CursorShape GetCursor() const noexcept;  // 마우스 커서 모양 조회
        void SuspendFocus();   // 다른 탭 · 대화상자로 입력을 넘길 때
        void RestoreFocus();   // 입력 포커스 복원

    private:
        // 묶음 상자 (테두리 + 머리말)
        struct Panel
        {
            Core::RectF m_bounds;    // 상자 영역
            std::wstring m_title;    // 상자 제목 (비면 머리말 없는 카드)
            std::wstring m_eyebrow;  // 청록 영문 소제목
            std::wstring m_hint;     // 머리말 오른쪽 작은 안내
        };

        // 표 카드 하나
        struct TableCard
        {
            Panel m_panel;           // 카드 상자
            UI::Grid* m_grid{};      // 카드 표 (m_host가 소유)
            std::size_t m_columnCount{};   // 열 수
        };

        void Layout(const Core::RectF& contentArea);  // 전체 배치 계산
        [[nodiscard]] Core::RectF LayoutPanel(Panel& panel, const Core::RectF& bounds) const;   // 테두리 · 머리말 자리를 뺀 안쪽 영역
        void RenderPanels(Core::Renderer& renderer) const;     // 상자 테두리 · 머리말 그리기
        void RenderCaptions(Core::Renderer& renderer) const;   // 요약 이름표 · 화살표 그리기
        void RequestSearch();  // 조회 요청 보내기

        UI::ControlHost m_host;  // 부품 호스트 (부품 소유 · 포커스 관리)

        // 부품은 m_host가 소유 (참조는 배치 · 값 설정용, 추가 순서 = Tab 이동 순서)
        UI::Label& m_nameLabel;          // 이름 안내 글자
        UI::TextBox& m_nameBox;          // 이름 입력 칸
        UI::Button& m_searchButton;      // 조회 버튼
        UI::Label& m_statusLabel;        // 상태 글자
        UI::Label& m_currentLevelLabel;  // 지금 평균 레벨 (큰 글자)
        UI::Label& m_transferLevelLabel; // 이관 뒤 평균 (큰 글자, 금색)
        UI::Label& m_scrollLabel;        // 주문서 요약
        UI::Label& m_gemLabel;           // 성휘 요약
        UI::Label& m_noteLabel;          // 맨 아래 안내
        std::array<TableCard, static_cast<std::size_t>(TransferCard::CARD_COUNT)> m_cards;   // 표 카드들

        static constexpr std::size_t CAPTION_COUNT{ 4 };       // 지금 · 이관 뒤 · 주문서 · 성휘
        std::array<Core::RectF, CAPTION_COUNT> m_captionBounds; // 이름표 칸
        Core::RectF m_arrowBounds;   // 지금 → 이관 뒤 화살표 칸

        Panel m_barPanel;            // 맨 위 결과 카드
        Core::RectF m_layoutArea;    // 배치 영역
        bool m_hasLayout{};          // 배치를 계산했는지

        std::function<void(const std::wstring&)> m_onSearch;   // 조회 알림 콜백
    };
}
