module LostArkMultiTool.Views.TransferView;  // 2부 이관 화면 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Core.Platform;  // 플랫폼 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Views.PageLayout;  // 새 디자인 공통 카드 가져오기
import LostArkMultiTool.Views.Theme;  // 테마(색 · 글꼴 크기) 모듈 가져오기
import LostArkMultiTool.Views.UI.Button;  // 버튼 부품 가져오기
import LostArkMultiTool.Views.UI.Control;  // 부품 기반 클래스 가져오기
import LostArkMultiTool.Views.UI.ControlHost;  // 부품 호스트 가져오기
import LostArkMultiTool.Views.UI.Grid;  // 표 부품 가져오기
import LostArkMultiTool.Views.UI.Label;  // 글자 상자 부품 가져오기
import LostArkMultiTool.Views.UI.TextBox;  // 입력 상자 부품 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        // ===== 카드별 표 열 =====
        constexpr std::size_t CARD_COUNT{ static_cast<std::size_t>(TransferCard::CARD_COUNT) };   // 카드 개수

        struct CardSpec   // 카드 하나의 머리말 · 열 정의
        {
            std::wstring_view m_eyebrow;                 // 청록 영문 소제목
            std::wstring_view m_title;                   // 제목
            std::wstring_view m_hint;                    // 오른쪽 안내 (매니저가 바꿀 수 있음)
            std::span<const std::wstring_view> m_headers;   // 열 머리글
            std::span<const float> m_weights;            // 열 너비 비율
        };

        constexpr std::array<std::wstring_view, 6> ARMOR_HEADERS{ L"2부 슬롯", L"기준", L"지금 Lv", L"이관 장비", L"이관 Lv", L"슬롯 재련" };   // 장비 카드 열
        constexpr std::array<float, 6> ARMOR_WEIGHTS{ 0.8f, 0.6f, 0.7f, 1.3f, 0.7f, 0.9f };   // 장비 카드 열 비율
        constexpr std::array<std::wstring_view, 5> ACCESSORY_HEADERS{ L"2부 슬롯", L"기준 부위", L"기준 Lv", L"이관 장비", L"이관 Lv" };   // 장신구 슬롯 카드 열
        constexpr std::array<float, 5> ACCESSORY_WEIGHTS{ 0.8f, 0.8f, 0.7f, 1.3f, 0.7f };   // 장신구 슬롯 카드 열 비율
        constexpr std::array<std::wstring_view, 5> SCROLL_HEADERS{ L"부위", L"장신구", L"품질", L"연마", L"주문서" };   // 주문서 카드 열
        constexpr std::array<float, 5> SCROLL_WEIGHTS{ 0.7f, 1.6f, 0.5f, 0.7f, 1.2f };   // 주문서 카드 열 비율
        constexpr std::array<std::wstring_view, 3> BRACELET_HEADERS{ L"팔찌 옵션", L"딜러", L"서폿" };   // 팔찌 카드 열
        constexpr std::array<float, 3> BRACELET_WEIGHTS{ 2.4f, 0.6f, 0.6f };   // 팔찌 카드 열 비율
        constexpr std::array<std::wstring_view, 3> GEM_HEADERS{ L"항목", L"지금", L"2부" };   // 보석 · 스톤 카드 열
        constexpr std::array<float, 3> GEM_WEIGHTS{ 0.8f, 0.9f, 1.9f };   // 보석 · 스톤 카드 열 비율
        constexpr std::array<std::wstring_view, 3> OTHER_HEADERS{ L"항목", L"지금", L"2부" };   // 그 밖 카드 열
        constexpr std::array<float, 3> OTHER_WEIGHTS{ 0.8f, 0.9f, 1.9f };   // 그 밖 카드 열 비율

        const std::array<CardSpec, CARD_COUNT> CARD_SPECS{ {   // 카드 정의 (TransferCard 순서)
            { L"EQUIPMENT", L"장비 이관 · 슬롯 재련", L"부위별 아이템 레벨 기준", ARMOR_HEADERS, ARMOR_WEIGHTS },
            { L"ACCESSORY SLOT", L"장신구 슬롯 장비", L"짝 장비 레벨 기준", ACCESSORY_HEADERS, ACCESSORY_WEIGHTS },
            { L"POLISH SCROLL", L"장신구 → 장신구 주문서", L"4티어 · 품질 67↑ · ×=옵션 없음", SCROLL_HEADERS, SCROLL_WEIGHTS },
            { L"SEAL SCROLL", L"팔찌 → 인장 주문서", L"옵션 티어 · 11/25 공지 기준", BRACELET_HEADERS, BRACELET_WEIGHTS },
            { L"GEM · STONE", L"보석 · 어빌리티 스톤", L"광휘 · 성휘 · 각인 업그레이드", GEM_HEADERS, GEM_WEIGHTS },
            { L"OTHERS", L"에스더 무기 · 그 밖", L"삭제 · 환산 항목", OTHER_HEADERS, OTHER_WEIGHTS },
        } };

        // 카드 칸 (열, 행) — 위: 장비 | 장신구 슬롯 | 주문서, 아래: 팔찌 | 보석 · 스톤 | 그 밖
        struct CardCell
        {
            std::size_t m_column{};  // 열 번호
            std::size_t m_row{};     // 행 번호
        };
        constexpr std::array<CardCell, CARD_COUNT> CARD_CELLS{ { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 0, 1 }, { 1, 1 }, { 2, 1 } } };   // 카드별 칸
        constexpr std::size_t GRID_COLUMN_COUNT{ 3 };   // 전체 칸 열 수
        constexpr std::size_t GRID_ROW_COUNT{ 2 };      // 전체 칸 행 수

        // ===== 글자 =====
        constexpr std::wstring_view NAME_TEXT{ L"캐릭터 이름" };   // 이름 안내 글자
        constexpr std::wstring_view NAME_PLACEHOLDER{ L"캐릭터 이름" };   // 이름 입력 칸 안내
        constexpr std::wstring_view SEARCH_TEXT{ L"조회" };   // 조회 버튼 글자
        constexpr std::wstring_view SEARCH_BUSY_TEXT{ L"조회 중..." };   // 조회 중 버튼 글자
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"캐릭터 이름을 넣고 [조회]를 누르면 2부에서 받을 장비 · 주문서를 보여줘요." };   // 처음 상태 글자
        constexpr std::wstring_view CAPTION_CURRENT{ L"지금 평균 아이템 레벨" };   // 이름표 1
        constexpr std::wstring_view CAPTION_TRANSFER{ L"이관 뒤 16부위 평균 (추정)" };   // 이름표 2
        constexpr std::wstring_view CAPTION_SCROLL{ L"최고 장신구 주문서" };   // 이름표 3
        constexpr std::wstring_view CAPTION_GEM{ L"10레벨 보석 → 성휘" };   // 이름표 4
        constexpr std::wstring_view ARROW_TEXT{ L"→" };   // 화살표
        constexpr std::size_t NAME_MAX_LENGTH{ 20 };   // 캐릭터 이름 최대 글자 수

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };          // 화면 바깥 여백
        constexpr float SECTION_GAP{ 12.0f };           // 구역 사이 간격
        constexpr float GROUP_GAP{ 12.0f };             // 묶음 사이 간격
        constexpr float LABEL_GAP{ 6.0f };              // 안내 글자와 부품 사이 간격
        constexpr float NAME_LABEL_WIDTH{ 84.0f };      // 이름 안내 글자 너비
        constexpr float NAME_BOX_WIDTH{ 200.0f };       // 이름 입력 칸 너비
        constexpr float SEARCH_BUTTON_WIDTH{ 96.0f };   // 조회 버튼 너비
        constexpr float PANEL_PADDING{ 10.0f };         // 상자 안쪽 여백
        constexpr float MIN_CARD_HEIGHT{ 230.0f };      // 카드 최소 높이
        constexpr float CAPTION_HEIGHT{ 16.0f };        // 이름표 줄 높이
        constexpr float VALUE_HEIGHT{ 32.0f };          // 큰 값 줄 높이
        constexpr float NOTE_LINE_COUNT{ 2.0f };        // 안내 문구 줄 수
        constexpr float NOTE_LINE_HEIGHT{ 17.0f };      // 안내 문구 한 줄 높이
        constexpr float RESULT_FRACTION_CURRENT{ 0.22f };    // 지금 평균 칸 너비 비율
        constexpr float RESULT_FRACTION_ARROW{ 0.04f };      // 화살표 칸 너비 비율
        constexpr float RESULT_FRACTION_TRANSFER{ 0.26f };   // 이관 뒤 평균 칸 너비 비율
        constexpr float RESULT_FRACTION_SCROLL{ 0.22f };     // 주문서 칸 너비 비율

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };      // 상태 글자 모양
        constexpr Core::TextStyle SETTING_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };   // 설정 안내 글자 모양
        constexpr Core::TextStyle POWER_TEXT_STYLE{ Theme::FONT_SIZE_TITLE, Core::FontWeight::BOLD, Core::TextAlign::LEADING };       // 큰 값 글자 모양
        constexpr Core::TextStyle SUB_VALUE_STYLE{ Theme::FONT_SIZE_CHART_TITLE, Core::FontWeight::BOLD, Core::TextAlign::LEADING };  // 중간 값 글자 모양
        constexpr Core::TextStyle CAPTION_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };        // 이름표 글자 모양
        constexpr Core::TextStyle ARROW_STYLE{ Theme::FONT_SIZE_TITLE, Core::FontWeight::BOLD, Core::TextAlign::CENTER };             // 화살표 글자 모양
        constexpr Core::TextStyle NOTE_TEXT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };      // 안내 문구 글자 모양

        // 카드 정의 → 표 열 (첫 열은 왼쪽, 나머지는 가운데 정렬)
        [[nodiscard]] std::vector<UI::GridColumn> MakeColumns(const CardSpec& spec)
        {
            std::vector<UI::GridColumn> columns;   // 열 목록
            columns.reserve(spec.m_headers.size());   // 열 수만큼 확보
            for (std::size_t column{}; column < spec.m_headers.size(); ++column)   // 열마다
            {
                const Core::TextAlign align{ column == 0 ? Core::TextAlign::LEADING : Core::TextAlign::CENTER };   // 이름 열만 왼쪽 정렬
                columns.push_back(UI::GridColumn{ std::wstring{ spec.m_headers[column] }, spec.m_weights[column], align });   // 열 추가
            }
            return columns;   // 열 목록 반환
        }

        // 글자 모양 → 표 칸 모양
        [[nodiscard]] UI::GridCellStyle ToCellStyle(const TransferCell& cell)
        {
            switch (cell.m_tone)   // 모양별 분기
            {
                case TransferTone::TONE_LABEL: return UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD };                 // 이름 칸
                case TransferTone::TONE_GRADE: return UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GetGradeColor(cell.m_gradeName), Core::FontWeight::BOLD };    // 등급 색
                case TransferTone::TONE_HIGHLIGHT: return UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::HIGHLIGHT_TEXT_COLOR, Core::FontWeight::BOLD };         // 강조
                case TransferTone::TONE_MUTED: return UI::GridCellStyle{ Theme::READONLY_BACK_COLOR, Theme::MUTED_TEXT_COLOR };                                     // 흐린 글자
                case TransferTone::TONE_HEADER: return UI::GridCellStyle{ Theme::HEADER_BACK_COLOR, Theme::TOTAL_TEXT_COLOR, Core::FontWeight::BOLD };              // 구분 제목
                default: return UI::GridCellStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };                                                                  // 일반 글자
            }
        }

        // 3 x 2 칸 중 (열, 행) 칸의 영역
        [[nodiscard]] Core::RectF GetCellBounds(const Core::RectF& area, const CardCell& cell) noexcept
        {
            const float width{ (area.GetWidth() - SECTION_GAP * static_cast<float>(GRID_COLUMN_COUNT - 1)) / static_cast<float>(GRID_COLUMN_COUNT) };   // 칸 너비
            const float height{ (area.GetHeight() - SECTION_GAP * static_cast<float>(GRID_ROW_COUNT - 1)) / static_cast<float>(GRID_ROW_COUNT) };      // 칸 높이
            const float left{ area.m_left + static_cast<float>(cell.m_column) * (width + SECTION_GAP) };   // 칸 왼쪽
            const float top{ area.m_top + static_cast<float>(cell.m_row) * (height + SECTION_GAP) };       // 칸 위쪽
            return Core::RectF{ left, top, left + width, top + std::max(height, 0.0f) };   // 칸 영역 (높이 음수 방지)
        }
    }

    TransferView::TransferView(Core::Platform& platform)  // 생성자: 부품을 호스트에 추가 (추가 순서 = Tab 순서)
        : m_nameLabel{ m_host.Emplace<UI::Label>(std::wstring{ NAME_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }  // 이름 안내 글자
        , m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }  // 이름 입력 칸
        , m_searchButton{ m_host.Emplace<UI::Button>(std::wstring{ SEARCH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }  // 조회 버튼
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }  // 상태 글자
        , m_currentLevelLabel{ m_host.Emplace<UI::Label>(std::wstring{}, POWER_TEXT_STYLE, Theme::HEADER_TEXT_COLOR) }  // 지금 평균 (큰 글자)
        , m_transferLevelLabel{ m_host.Emplace<UI::Label>(std::wstring{}, POWER_TEXT_STYLE, Theme::HIGHLIGHT_TEXT_COLOR) }  // 이관 뒤 평균 (금색)
        , m_scrollLabel{ m_host.Emplace<UI::Label>(std::wstring{}, SUB_VALUE_STYLE, Theme::TEXT_COLOR) }  // 주문서 요약
        , m_gemLabel{ m_host.Emplace<UI::Label>(std::wstring{}, SUB_VALUE_STYLE, Theme::TEXT_COLOR) }  // 성휘 요약
        , m_noteLabel{ m_host.Emplace<UI::Label>(std::wstring{}, NOTE_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }  // 안내 문구
        , m_barPanel{ Core::RectF{}, std::wstring{} }  // 결과 카드 (머리말 없음)
    {
        m_nameBox.SetMaxLength(NAME_MAX_LENGTH);   // 이름 최대 글자 수
        m_nameBox.SetOnSubmit([this] { RequestSearch(); });   // Enter → 조회
        m_searchButton.SetOnClick([this] { RequestSearch(); });   // 버튼 → 조회

        for (std::size_t index{}; index < CARD_COUNT; ++index)   // 카드마다 표 만들기
        {
            const CardSpec& spec{ CARD_SPECS[index] };   // 카드 정의
            TableCard& card{ m_cards[index] };           // 카드
            card.m_panel = Panel{ Core::RectF{}, std::wstring{ spec.m_title }, std::wstring{ spec.m_eyebrow }, std::wstring{ spec.m_hint } };   // 머리말
            card.m_columnCount = spec.m_headers.size();  // 열 수
            card.m_grid = &m_host.Emplace<UI::Grid>(platform, MakeColumns(spec), 0);   // 표 (읽기 전용)
            card.m_grid->SetShowVerticalLines(true);     // 칸 구분 세로선
        }
    }

    TransferView::~TransferView() = default;  // 소멸자 (부품은 호스트가 해제)

    void TransferView::SetOnSearch(std::function<void(const std::wstring& characterName)> onSearch)  // 조회 알림 연결
    {
        m_onSearch = std::move(onSearch);  // 콜백 보관
    }

    void TransferView::SetCharacterName(std::wstring name)  // 이름 칸 글자 설정
    {
        m_nameBox.SetText(std::move(name));  // 이름 반영
    }

    void TransferView::SetSearchBusy(bool isBusy)  // 조회 중 표시
    {
        m_searchButton.SetEnabled(!isBusy);  // 조회 중 버튼 잠금
        m_searchButton.SetText(std::wstring{ isBusy ? SEARCH_BUSY_TEXT : SEARCH_TEXT });  // 버튼 글자
    }

    void TransferView::SetStatusText(std::wstring text, bool isError)  // 상태 글자 설정
    {
        m_statusLabel.SetText(std::move(text));  // 글자 반영
        m_statusLabel.SetColor(isError ? Theme::ALERT_TEXT_COLOR : Theme::TEXT_COLOR);  // 오류면 경고색
    }

    void TransferView::ShowSummary(const TransferSummaryText& summary)  // 요약 큰 글자
    {
        m_currentLevelLabel.SetText(summary.m_currentLevel);    // 지금 평균
        m_transferLevelLabel.SetText(summary.m_transferLevel);  // 이관 뒤 평균
        m_scrollLabel.SetText(summary.m_scroll);                // 주문서
        m_gemLabel.SetText(summary.m_gem);                      // 성휘
    }

    // 카드 표 다시 채우기 (열 수보다 적은 칸은 빈 글자)
    void TransferView::ShowCard(TransferCard card, const std::vector<TransferRow>& rows)
    {
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };   // 카드 번호
        if (cardIndex >= CARD_COUNT) return;   // 범위 밖이면 무시

        TableCard& table{ m_cards[cardIndex] };   // 카드
        UI::Grid& grid{ *table.m_grid };          // 카드 표
        grid.SetRowCount(rows.size());            // 행 수 맞추기
        for (std::size_t row{}; row < rows.size(); ++row)   // 행마다
        {
            for (std::size_t column{}; column < table.m_columnCount; ++column)   // 열마다
            {
                const TransferCell cell{ column < rows[row].size() ? rows[row][column] : TransferCell{} };   // 칸 (없으면 빈 칸)
                grid.SetCellText(row, column, cell.m_text);        // 글자
                grid.SetCellStyle(row, column, ToCellStyle(cell)); // 모양
                grid.SetCellEditable(row, column, false);          // 읽기 전용
            }
        }
    }

    void TransferView::SetCardHint(TransferCard card, std::wstring hint)  // 카드 머리말 오른쪽 안내
    {
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };   // 카드 번호
        if (cardIndex < CARD_COUNT) m_cards[cardIndex].m_panel.m_hint = std::move(hint);   // 범위 안이면 반영
    }

    void TransferView::SetNote(std::wstring note)  // 맨 아래 안내 문구
    {
        m_noteLabel.SetText(std::move(note));  // 글자 반영
    }

    void TransferView::Update(double deltaSeconds)  // 프레임 갱신
    {
        m_host.Update(deltaSeconds);  // 부품 갱신
    }

    void TransferView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)  // 화면 그리기
    {
        Layout(contentArea);       // 배치 계산 (영역이 같으면 건너뜀)
        RenderPanels(renderer);    // 상자 그리기
        RenderCaptions(renderer);  // 이름표 · 화살표
        m_host.Render(renderer);   // 부품 그리기
    }

    bool TransferView::HandleInput(const Core::InputEvent& event)  // 입력 처리
    {
        return m_host.HandleInput(event);  // 부품 호스트에 전달
    }

    Core::CursorShape TransferView::GetCursor() const noexcept  // 커서 모양
    {
        return m_host.GetCursor();  // 부품 호스트의 커서
    }

    void TransferView::SuspendFocus()  // 포커스 일시 해제
    {
        m_host.SuspendFocus();  // 부품 호스트 포커스 해제
    }

    void TransferView::RestoreFocus()  // 포커스 복원
    {
        m_host.RestoreFocus();  // 부품 호스트 포커스 복원
    }

    void TransferView::RequestSearch()  // 조회 요청 보내기
    {
        m_nameBox.CommitComposition();   // 조합 중인 한글 마지막 글자까지 포함
        if (const auto onSearch{ m_onSearch }) onSearch(m_nameBox.GetText());  // 콜백이 있으면 이름 알림
    }

    // 맨 위 결과 카드 / 3 x 2 카드 / 맨 아래 안내 문구
    void TransferView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;  // 영역이 같으면 건너뜀
        m_layoutArea = contentArea;  // 영역 기억
        m_hasLayout = true;          // 계산 완료 표시

        const Core::RectF column{ PageLayout::GetPageColumn(Core::RectF{ contentArea.m_left + PAGE_PADDING, contentArea.m_top + PAGE_PADDING,
            std::max(contentArea.m_right - PAGE_PADDING, contentArea.m_left + PAGE_PADDING), contentArea.m_bottom - PAGE_PADDING }) };  // 본문 열
        const float left{ column.m_left };                 // 화면 왼쪽
        const float right{ std::max(column.m_right, left) };   // 화면 오른쪽
        const float barTop{ column.m_top };                // 결과 카드 위쪽
        const float innerLeft{ left + PageLayout::CARD_PADDING };   // 결과 카드 안쪽 왼쪽
        const float innerRight{ std::max(right - PageLayout::CARD_PADDING, innerLeft) };   // 결과 카드 안쪽 오른쪽

        const auto placeRow{ [](float& x, float top, UI::Control& control, float width, float gapAfter)   // 부품을 가로로 놓고 x를 옮김
        {
            control.SetBounds(Core::RectF{ x, top, x + width, top + Theme::CONTROL_HEIGHT });   // 부품 영역
            x += width + gapAfter;   // 다음 위치
        } };

        // 첫 줄: 캐릭터 이름 [입력칸] [조회] 상태
        float y{ barTop + PageLayout::CARD_PADDING };   // 첫 줄 높이
        float x{ innerLeft };   // 가로 위치
        placeRow(x, y, m_nameLabel, NAME_LABEL_WIDTH, LABEL_GAP);       // 이름 안내
        placeRow(x, y, m_nameBox, NAME_BOX_WIDTH, GROUP_GAP);           // 이름 칸
        placeRow(x, y, m_searchButton, SEARCH_BUTTON_WIDTH, SECTION_GAP);   // 조회 버튼
        m_statusLabel.SetBounds(Core::RectF{ x, y, std::max(x, innerRight), y + Theme::CONTROL_HEIGHT });   // 상태는 남은 너비
        y += Theme::CONTROL_HEIGHT + SECTION_GAP;   // 다음 줄

        // 둘째 줄: 이름표 아래 큰 값 — 지금 평균 → 이관 뒤 평균 · 주문서 · 성휘
        const float innerWidth{ innerRight - innerLeft };   // 안쪽 너비
        const float currentWidth{ innerWidth * RESULT_FRACTION_CURRENT };     // 지금 평균 칸
        const float arrowWidth{ innerWidth * RESULT_FRACTION_ARROW };         // 화살표 칸
        const float transferWidth{ innerWidth * RESULT_FRACTION_TRANSFER };   // 이관 뒤 칸
        const float scrollWidth{ innerWidth * RESULT_FRACTION_SCROLL };       // 주문서 칸
        const float valueTop{ y + CAPTION_HEIGHT };   // 큰 값 위쪽
        x = innerLeft;   // 가로 위치 처음으로
        m_captionBounds[0] = Core::RectF{ x, y, x + currentWidth, valueTop };   // 이름표 1
        m_currentLevelLabel.SetBounds(Core::RectF{ x, valueTop, x + currentWidth, valueTop + VALUE_HEIGHT });   // 지금 평균
        x += currentWidth;   // 다음 칸
        m_arrowBounds = Core::RectF{ x, valueTop, x + arrowWidth, valueTop + VALUE_HEIGHT };   // 화살표
        x += arrowWidth;     // 다음 칸
        m_captionBounds[1] = Core::RectF{ x, y, x + transferWidth, valueTop };   // 이름표 2
        m_transferLevelLabel.SetBounds(Core::RectF{ x, valueTop, x + transferWidth, valueTop + VALUE_HEIGHT });   // 이관 뒤 평균
        x += transferWidth;  // 다음 칸
        m_captionBounds[2] = Core::RectF{ x, y, x + scrollWidth, valueTop };   // 이름표 3
        m_scrollLabel.SetBounds(Core::RectF{ x, valueTop, x + scrollWidth, valueTop + VALUE_HEIGHT });   // 주문서
        x += scrollWidth;    // 다음 칸
        m_captionBounds[3] = Core::RectF{ x, y, std::max(x, innerRight), valueTop };   // 이름표 4
        m_gemLabel.SetBounds(Core::RectF{ x, valueTop, std::max(x, innerRight), valueTop + VALUE_HEIGHT });   // 성휘 (남은 너비)
        y = valueTop + VALUE_HEIGHT + PageLayout::CARD_PADDING;   // 아래 여백
        m_barPanel.m_bounds = Core::RectF{ left, barTop, right, y };   // 결과 카드 영역

        // 맨 아래 안내 문구
        const float noteHeight{ NOTE_LINE_HEIGHT * NOTE_LINE_COUNT };   // 안내 높이
        const float noteTop{ std::max(y + SECTION_GAP, contentArea.m_bottom - PAGE_PADDING - noteHeight) };   // 안내 위치 (카드와 겹치지 않게)
        m_noteLabel.SetBounds(Core::RectF{ left, noteTop, right, noteTop + noteHeight });   // 안내 배치

        // 3 x 2 카드
        const float cardsTop{ y + SECTION_GAP };   // 카드 영역 위쪽
        const float cardsBottom{ std::max(cardsTop + MIN_CARD_HEIGHT * static_cast<float>(GRID_ROW_COUNT) + SECTION_GAP, noteTop - SECTION_GAP) };   // 카드 영역 아래쪽 (최소 높이 보장)
        const Core::RectF cardsArea{ left, cardsTop, right, cardsBottom };   // 카드 영역
        for (std::size_t index{}; index < CARD_COUNT; ++index)   // 카드마다
        {
            TableCard& card{ m_cards[index] };   // 카드
            card.m_grid->SetBounds(LayoutPanel(card.m_panel, GetCellBounds(cardsArea, CARD_CELLS[index])));   // 표 = 카드 안쪽 전체
        }
    }

    Core::RectF TransferView::LayoutPanel(Panel& panel, const Core::RectF& bounds) const   // 상자 안쪽 영역
    {
        panel.m_bounds = bounds;   // 상자 영역 기록
        return Core::RectF{ bounds.m_left + PANEL_PADDING, bounds.m_top + PageLayout::CARD_HEADER_HEIGHT,   // 테두리 · 머리말을 뺀 안쪽
            std::max(bounds.m_right - PANEL_PADDING, bounds.m_left + PANEL_PADDING), std::max(bounds.m_top + PageLayout::CARD_HEADER_HEIGHT, bounds.m_bottom - PANEL_PADDING) };
    }

    // 결과 카드는 바탕만, 나머지는 머리말 카드
    void TransferView::RenderPanels(Core::Renderer& renderer) const
    {
        PageLayout::DrawCard(renderer, m_barPanel.m_bounds);   // 결과 카드
        for (const TableCard& card : m_cards)   // 표 카드마다
        {
            PageLayout::DrawCardWithHeader(renderer, card.m_panel.m_bounds, card.m_panel.m_eyebrow, card.m_panel.m_title, card.m_panel.m_hint);   // 머리말 카드
        }
    }

    // 요약 이름표(작은 회색)와 화살표
    void TransferView::RenderCaptions(Core::Renderer& renderer) const
    {
        const std::array<std::wstring_view, CAPTION_COUNT> captions{ CAPTION_CURRENT, CAPTION_TRANSFER, CAPTION_SCROLL, CAPTION_GEM };   // 이름표 글자
        for (std::size_t index{}; index < CAPTION_COUNT; ++index) renderer.DrawString(captions[index], m_captionBounds[index], Theme::MUTED_TEXT_COLOR, CAPTION_STYLE);   // 이름표
        renderer.DrawString(ARROW_TEXT, m_arrowBounds, Theme::MUTED_TEXT_COLOR, ARROW_STYLE);   // 화살표
    }
}
