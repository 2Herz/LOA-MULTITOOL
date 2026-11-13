module LostArkMultiTool.Views.CombatSimulatorView;  // 전투력 시뮬레이터 화면 모듈 구현 파일

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 모듈 가져오기
import LostArkMultiTool.Models.CombatCoefficientData;  // 계수 데이터 모듈 가져오기 (계수 기준 날짜)
import LostArkMultiTool.Core.Platform;  // 플랫폼 모듈 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 모듈 가져오기
import LostArkMultiTool.Views.Theme;  // 테마(색 · 글꼴 크기) 모듈 가져오기
import LostArkMultiTool.Views.UI.Button;  // 버튼 부품 가져오기
import LostArkMultiTool.Views.UI.ComboBox;  // 선택 상자 부품 가져오기
import LostArkMultiTool.Views.UI.Control;  // 부품 기반 클래스 가져오기
import LostArkMultiTool.Views.UI.ControlHost;  // 부품 호스트 가져오기
import LostArkMultiTool.Views.UI.Grid;  // 표 부품 가져오기
import LostArkMultiTool.Views.UI.Label;  // 글자 상자 부품 가져오기
import LostArkMultiTool.Views.UI.TextBox;  // 입력 상자 부품 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 파일 내부 전용 상수 · 보조 함수 영역 시작
    {
        // ===== 장비 표 =====
        constexpr std::size_t GEAR_COLUMN_COUNT{ static_cast<std::size_t>(CombatGearColumn::COLUMN_COUNT) };  // 장비 표 열 개수
        constexpr std::array<std::wstring_view, GEAR_COLUMN_COUNT> GEAR_HEADERS{  // 장비 표 머리글
            L"부위", L"아이템", L"현재 일반", L"현재 상급", L"목표 일반", L"목표 상급" };
        constexpr std::array<float, GEAR_COLUMN_COUNT> GEAR_WEIGHTS{ 0.6f, 2.6f, 0.8f, 0.8f, 0.8f, 0.8f };  // 장비 표 열 너비 비율

        // ===== 보석 표 =====
        constexpr std::size_t GEM_COLUMN_COUNT{ static_cast<std::size_t>(CombatGemColumn::COLUMN_COUNT) };  // 보석 표 열 개수
        constexpr std::array<std::wstring_view, GEM_COLUMN_COUNT> GEM_HEADERS{ L"슬롯", L"보석", L"현재 레벨", L"목표 레벨" };  // 보석 표 머리글
        constexpr std::array<float, GEM_COLUMN_COUNT> GEM_WEIGHTS{ 0.6f, 1.0f, 1.0f, 1.0f };  // 보석 표 열 너비 비율

        // ===== 목록형 카드 표 (항목 / 현재 / 목표) =====
        constexpr std::size_t CARD_COUNT{ static_cast<std::size_t>(CombatCard::CARD_COUNT) };  // 목록형 카드 개수
        constexpr std::size_t LIST_COLUMN_COUNT{ 3 };  // 목록형 표 열 개수
        constexpr std::size_t LIST_LABEL_COLUMN{ 0 };  // 항목 열 번호
        constexpr std::size_t LIST_CURRENT_COLUMN{ 1 };  // 현재 열 번호
        constexpr std::size_t LIST_TARGET_COLUMN{ 2 };  // 목표 열 번호
        constexpr std::array<std::wstring_view, LIST_COLUMN_COUNT> LIST_HEADERS{ L"항목", L"현재", L"목표" };  // 목록형 표 머리글
        constexpr std::array<float, LIST_COLUMN_COUNT> LIST_WEIGHTS{ 1.2f, 1.6f, 1.6f };  // 목록형 표 열 너비 비율
        constexpr std::array<std::wstring_view, CARD_COUNT> CARD_TITLES{  // 카드 제목 목록
            L"악세서리 연마 옵션 5종", L"아크 그리드 (코어 6 · 젬 효과)", L"팔찌 · 어빌리티 스톤 · 각인서", L"아크 패시브 · 아바타" };

        // 카드가 놓이는 칸 (열, 행) — 위 줄: 장비 재련 | 악세서리 | 아크 그리드, 아래 줄: 보석 · 품질 | 팔찌 · 스톤 · 각인서 | 아크 패시브 · 아바타
        struct CardCell  // 카드가 놓이는 칸 (열 · 행)
        {
            std::size_t m_column{};  // 칸의 열 번호
            std::size_t m_row{};  // 칸의 행 번호
        };
        constexpr std::array<CardCell, CARD_COUNT> CARD_CELLS{ { { 1, 0 }, { 2, 0 }, { 1, 1 }, { 2, 1 } } };  // 카드별 칸 위치 (악세서리 · 아크 그리드 · 팔찌 · 패시브)
        constexpr CardCell GEAR_CELL{ 0, 0 };  // 장비 재련 칸 위치
        constexpr CardCell GEM_CELL{ 0, 1 };  // 보석 · 품질 칸 위치
        constexpr std::size_t GRID_COLUMN_COUNT{ 3 };  // 전체 칸 열 개수
        constexpr std::size_t GRID_ROW_COUNT{ 2 };  // 전체 칸 행 개수

        // ===== 글자 =====
        constexpr std::wstring_view NAME_TEXT{ L"캐릭터 이름" };  // 이름 안내 글자
        constexpr std::wstring_view NAME_PLACEHOLDER{ L"캐릭터 이름" };  // 이름 입력 칸 안내 글자
        constexpr std::wstring_view SEARCH_TEXT{ L"조회" };  // 조회 버튼 글자
        constexpr std::wstring_view SEARCH_BUSY_TEXT{ L"조회 중..." };  // 조회 중 버튼 글자
        constexpr std::wstring_view INITIAL_STATUS_TEXT{ L"캐릭터 이름을 넣고 [조회]를 누르면 무기고를 불러와요." };  // 처음 상태 글자
        constexpr std::wstring_view QUALITY_PLACEHOLDER{ L"100" };  // 무기 품질 입력 칸 안내 글자
        constexpr std::wstring_view GEM_TARGET_TEXT{ L"모든 보석 목표 레벨" };  // 보석 전체 목표 안내 글자
        constexpr std::wstring_view GEAR_PANEL_TITLE{ L"장비 재련 · 완갑 (목표 칸에 숫자를 넣어요)" };  // 장비 재련 상자 제목
        constexpr std::wstring_view OPTION_PANEL_TITLE{ L"보석 · 무기 품질" };  // 보석 · 품질 상자 제목
        constexpr std::wstring_view NOTE_TEXT{ L"※ lopec 측정 기반 추정치예요. 근사: 스톤 1번 줄 없음(불안정) · 스톤과 같은 이름의 각인서(약 ±4%) · 깨달음 포인트 구성(최대로 가정) · 완갑(에스더/일반 두 가지 표)" };  // 추정치 안내 문구
        constexpr std::wstring_view COEFFICIENT_NOTE_FORMAT{ L" 계수 기준: {} · {}" };  // 계수 기준 안내 서식 (측정 날짜 · 측정 근거)
        constexpr std::wstring_view NOTE_EXTRA_SEPARATOR{ L" " };  // 안내 문구와 추가 안내 사이 구분 글자

        // 안내 문구 전체 글자 (추정치 안내 + 계수 기준 + 필요할 때 추가 안내)
        [[nodiscard]] std::wstring BuildNoteText(std::wstring_view extra)  // 안내 문구 만들기
        {
            std::wstring text{ NOTE_TEXT };  // 기본 추정치 안내
            text += std::format(COEFFICIENT_NOTE_FORMAT, Models::CombatCoefficientData::GetCoefficientMeasuredAt(), Models::CombatCoefficientData::GetCoefficientBasis());  // 계수 기준 덧붙임
            if (!extra.empty())  // 추가 안내가 있으면
            {
                text += NOTE_EXTRA_SEPARATOR;  // 구분 글자
                text += extra;  // 추가 안내 덧붙임
            }
            return text;  // 완성된 안내 문구 반환
        }

        constexpr std::wstring_view HINT_DEFAULT_TEXT{ L"항목을 고르면 여기서 선택지를 고를 수 있어요. 숫자 항목은 목표 칸을 눌러 바로 입력해요." };  // 카드 기본 안내 문구
        constexpr std::wstring_view HINT_NUMBER_FORMAT{ L"{}   현재 {}  →  목표 {}   (목표 칸을 눌러 숫자를 입력)" };  // 숫자 항목 안내 서식
        constexpr std::wstring_view HINT_CHOICE_FORMAT{ L"{}   현재 {}  →  목표 {}" };  // 선택 항목 안내 서식
        constexpr std::size_t NAME_MAX_LENGTH{ 20 };  // 캐릭터 이름 최대 글자 수
        constexpr std::size_t QUALITY_MAX_LENGTH{ 3 };  // 무기 품질 최대 글자 수

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };  // 화면 바깥 여백
        constexpr float SECTION_GAP{ 12.0f };  // 구역 사이 간격
        constexpr float GROUP_GAP{ 12.0f };  // 묶음 사이 간격
        constexpr float LABEL_GAP{ 6.0f };  // 안내 글자와 부품 사이 간격
        constexpr float NAME_LABEL_WIDTH{ 84.0f };  // 이름 안내 글자 너비
        constexpr float NAME_BOX_WIDTH{ 200.0f };  // 이름 입력 칸 너비
        constexpr float SEARCH_BUTTON_WIDTH{ 96.0f };  // 조회 버튼 너비
        constexpr float QUALITY_LABEL_WIDTH{ 230.0f };  // 무기 품질 안내 글자 너비
        constexpr float QUALITY_BOX_WIDTH{ 72.0f };  // 무기 품질 입력 칸 너비
        constexpr float GEM_TARGET_LABEL_WIDTH{ 140.0f };  // 보석 전체 목표 안내 글자 너비
        constexpr float GEM_TARGET_BOX_WIDTH{ 160.0f };  // 보석 전체 목표 선택 상자 너비
        constexpr float PANEL_PADDING{ 10.0f };  // 상자 안쪽 여백
        constexpr float PANEL_TITLE_LEFT{ 12.0f };  // 상자 제목의 왼쪽 위치
        constexpr float BAR_PADDING{ 10.0f };  // 결과 바 안쪽 여백
        constexpr float MIN_CARD_HEIGHT{ 150.0f };  // 카드 최소 높이
        constexpr float ROW_GAP{ 4.0f };  // 행 사이 간격
        constexpr float HINT_LINE_COUNT{ 2.0f };       // 선택한 행 안내 자리 (두 줄)
        constexpr float HINT_LINE_HEIGHT{ 17.0f };  // 선택한 행 안내 한 줄 높이
        constexpr float NOTE_LINE_COUNT{ 2.0f };       // 안내 문구 자리 (두 줄)
        constexpr float NOTE_LINE_HEIGHT{ 17.0f };  // 안내 문구 한 줄 높이
        constexpr float RESULT_FRACTION_CURRENT{ 0.22f };     // 결과 줄에서 칸마다 차지하는 너비 비율
        constexpr float RESULT_FRACTION_PREDICTED{ 0.30f };  // 결과 줄에서 예상 전투력 칸 너비 비율
        constexpr float RESULT_FRACTION_CHANGE{ 0.24f };  // 결과 줄에서 증감 칸 너비 비율

        constexpr Core::TextStyle BAR_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };  // 결과 바 글자 모양
        constexpr Core::TextStyle SETTING_LABEL_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };  // 설정 안내 글자 모양 (굵게)
        constexpr Core::TextStyle RESULT_TEXT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };  // 결과 글자 모양
        constexpr Core::TextStyle PREDICTED_TEXT_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::BOLD, Core::TextAlign::LEADING };  // 예상 전투력 글자 모양 (크고 굵게)
        constexpr Core::TextStyle NOTE_TEXT_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };  // 안내 문구 글자 모양 (작게)
        constexpr Core::TextStyle PANEL_TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };  // 상자 제목 글자 모양 (굵게)

        // 재련 · 품질 · 목록 숫자 칸: 숫자만
        [[nodiscard]] bool IsDigitCharacter(wchar_t character) noexcept  // 숫자 글자 판별
        {
            return character >= L'0' && character <= L'9';  // 0 ~ 9 사이이면 true
        }

        [[nodiscard]] constexpr std::size_t ToColumn(CombatGearColumn column) noexcept  // 장비 열 → 열 번호 변환
        {
            return static_cast<std::size_t>(column);  // 열거형 값을 번호로 변환
        }

        [[nodiscard]] constexpr std::size_t ToColumn(CombatGemColumn column) noexcept  // 보석 열 → 열 번호 변환
        {
            return static_cast<std::size_t>(column);  // 열거형 값을 번호로 변환
        }

        // 보석 표 열 (모두 가운데 정렬)
        [[nodiscard]] std::vector<UI::GridColumn> MakeGemColumns()  // 보석 표 열 만들기
        {
            std::vector<UI::GridColumn> columns;  // 열 목록
            columns.reserve(GEM_COLUMN_COUNT);  // 열 개수만큼 메모리 미리 확보
            for (std::size_t column{}; column < GEM_COLUMN_COUNT; ++column)  // 열마다
            {
                columns.push_back(UI::GridColumn{ std::wstring{ GEM_HEADERS[column] }, GEM_WEIGHTS[column], Core::TextAlign::CENTER });  // 머리글 · 너비 비율 · 가운데 정렬로 열 추가
            }
            return columns;  // 열 목록 반환
        }

        // 부위 · 아이템 열은 왼쪽 정렬, 숫자 열은 가운데
        [[nodiscard]] std::vector<UI::GridColumn> MakeGearColumns()  // 장비 표 열 만들기
        {
            std::vector<UI::GridColumn> columns;  // 열 목록
            columns.reserve(GEAR_COLUMN_COUNT);  // 열 개수만큼 메모리 미리 확보
            for (std::size_t column{}; column < GEAR_COLUMN_COUNT; ++column)  // 열마다
            {
                const bool isLeading{ column == ToColumn(CombatGearColumn::COLUMN_PART) || column == ToColumn(CombatGearColumn::COLUMN_ITEM) };  // 부위 · 아이템 열이면 왼쪽 정렬
                columns.push_back(UI::GridColumn{ std::wstring{ GEAR_HEADERS[column] }, GEAR_WEIGHTS[column], isLeading ? Core::TextAlign::LEADING : Core::TextAlign::CENTER });  // 머리글 · 너비 비율 · 정렬로 열 추가
            }
            return columns;  // 열 목록 반환
        }

        // 목록형 카드 표 열 (모두 왼쪽 정렬: 문구가 길어서)
        [[nodiscard]] std::vector<UI::GridColumn> MakeListColumns()  // 목록형 카드 표 열 만들기
        {
            std::vector<UI::GridColumn> columns;  // 열 목록
            columns.reserve(LIST_COLUMN_COUNT);  // 열 개수만큼 메모리 미리 확보
            for (std::size_t column{}; column < LIST_COLUMN_COUNT; ++column)  // 열마다
            {
                columns.push_back(UI::GridColumn{ std::wstring{ LIST_HEADERS[column] }, LIST_WEIGHTS[column], Core::TextAlign::LEADING });  // 머리글 · 너비 비율 · 왼쪽 정렬로 열 추가
            }
            return columns;  // 열 목록 반환
        }

        // 3 x 2 칸 중 (열, 행) 칸의 영역
        [[nodiscard]] Core::RectF GetCellBounds(const Core::RectF& area, const CardCell& cell) noexcept  // 칸의 영역 계산
        {
            const float width{ (area.GetWidth() - SECTION_GAP * static_cast<float>(GRID_COLUMN_COUNT - 1)) / static_cast<float>(GRID_COLUMN_COUNT) };  // 칸 너비 = (전체 너비 - 간격 합) ÷ 열 개수
            const float height{ (area.GetHeight() - SECTION_GAP * static_cast<float>(GRID_ROW_COUNT - 1)) / static_cast<float>(GRID_ROW_COUNT) };  // 칸 높이 = (전체 높이 - 간격 합) ÷ 행 개수
            const float left{ area.m_left + static_cast<float>(cell.m_column) * (width + SECTION_GAP) };  // 칸 왼쪽 위치
            const float top{ area.m_top + static_cast<float>(cell.m_row) * (height + SECTION_GAP) };  // 칸 위쪽 위치
            return Core::RectF{ left, top, left + width, top + std::max(height, 0.0f) };  // 높이가 음수가 되지 않게 해 영역 반환
        }
    }

    CombatSimulatorView::CombatSimulatorView(Core::Platform& platform)  // 생성자: 부품을 호스트에 추가하고 참조 보관 (추가 순서 = Tab 순서)
        : m_nameLabel{ m_host.Emplace<UI::Label>(std::wstring{ NAME_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }  // 이름 안내 글자 생성
        , m_nameBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ NAME_PLACEHOLDER }) }  // 이름 입력 칸 생성
        , m_searchButton{ m_host.Emplace<UI::Button>(std::wstring{ SEARCH_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }  // 조회 버튼 생성
        , m_statusLabel{ m_host.Emplace<UI::Label>(std::wstring{ INITIAL_STATUS_TEXT }, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }  // 상태 글자 생성
        , m_gearGrid{ m_host.Emplace<UI::Grid>(platform, MakeGearColumns(), 0) }  // 장비 표 생성
        , m_qualityLabel{ m_host.Emplace<UI::Label>(std::wstring{}, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }  // 무기 품질 안내 글자 생성
        , m_qualityBox{ m_host.Emplace<UI::TextBox>(platform, std::wstring{ QUALITY_PLACEHOLDER }) }  // 무기 품질 입력 칸 생성
        , m_gemLabel{ m_host.Emplace<UI::Label>(std::wstring{}, BAR_TEXT_STYLE, Theme::TEXT_COLOR) }  // 보석 요약 글자 생성
        , m_gemTargetLabel{ m_host.Emplace<UI::Label>(std::wstring{ GEM_TARGET_TEXT }, SETTING_LABEL_STYLE, Theme::TEXT_COLOR) }  // 보석 전체 목표 안내 글자 생성
        , m_gemTargetBox{ m_host.Emplace<UI::ComboBox>() }  // 보석 전체 목표 선택 상자 생성
        , m_gemGrid{ m_host.Emplace<UI::Grid>(platform, MakeGemColumns(), 0) }  // 보석 표 생성
        , m_currentPowerLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::TEXT_COLOR) }  // 현재 전투력 글자 생성
        , m_predictedPowerLabel{ m_host.Emplace<UI::Label>(std::wstring{}, PREDICTED_TEXT_STYLE, Theme::HIGHLIGHT_TEXT_COLOR) }  // 예상 전투력 글자 생성 (강조색)
        , m_changeLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::TEXT_COLOR) }  // 증감 글자 생성
        , m_itemLevelLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::TEXT_COLOR) }  // 아이템 레벨 글자 생성
        , m_factorLabel{ m_host.Emplace<UI::Label>(std::wstring{}, RESULT_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }  // 배율 내역 글자 생성 (흐린 색)
        , m_noteLabel{ m_host.Emplace<UI::Label>(BuildNoteText({}), NOTE_TEXT_STYLE, Theme::MUTED_TEXT_COLOR) }  // 추정치 안내 글자 생성
        , m_barPanel{ Core::RectF{}, std::wstring{} }  // 결과 바 상자 초기화
        , m_gearPanel{ Core::RectF{}, std::wstring{ GEAR_PANEL_TITLE } }  // 장비 재련 상자 초기화
        , m_optionPanel{ Core::RectF{}, std::wstring{ OPTION_PANEL_TITLE } }  // 보석 · 품질 상자 초기화
    {
        // 조회: 버튼 · 이름 칸 Enter (이름 정리 · 요청은 매니저가)
        m_nameBox.SetMaxLength(NAME_MAX_LENGTH);  // 이름 입력 칸 최대 글자 수 설정
        m_nameBox.SetOnSubmit([this] { RequestSearch(); });  // 이름 칸에서 Enter를 누르면 조회 요청
        m_searchButton.SetOnClick([this] { RequestSearch(); });  // 조회 버튼을 누르면 조회 요청

        // 장비 표: 목표 숫자 칸만 편집
        m_gearGrid.SetEditInput(false, IsDigitCharacter);  // 장비 표는 숫자만 입력 받음
        m_gearGrid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text)  // 장비 칸 편집이 끝나면
        {
            if (const auto onGearCellEdited{ m_onGearCellEdited }) onGearCellEdited(row, static_cast<CombatGearColumn>(column), text);  // 등록된 콜백이 있으면 행 · 열 · 글자를 알림
        });

        // 무기 품질: 숫자만, 입력할 때마다 알림 (범위 확인은 매니저가)
        m_qualityBox.SetIMEAllowed(false);  // 품질 입력 칸은 한글 입력기 사용 안 함
        m_qualityBox.SetMaxLength(QUALITY_MAX_LENGTH);  // 품질 입력 칸 최대 글자 수 설정
        m_qualityBox.SetInputFilter(IsDigitCharacter);  // 품질 입력 칸은 숫자만 허용
        m_qualityBox.SetOnTextChanged([this]  // 품질 입력 글자가 바뀌면
        {
            if (m_isUpdatingQualityText) return;  // 코드에서 바꾸는 중이면 알림 막기
            if (const auto onQualityChanged{ m_onQualityChanged }) onQualityChanged(m_qualityBox.GetText());  // 등록된 콜백이 있으면 입력 글자를 알림
        });

        // 보석 표: 목표 레벨 칸만 숫자 편집 (범위 확인은 매니저가)
        m_gemGrid.SetEditInput(false, IsDigitCharacter);  // 보석 표는 숫자만 입력 받음
        m_gemGrid.SetOnCellEdited([this](std::size_t row, std::size_t column, const std::wstring& text)  // 보석 칸 편집이 끝나면
        {
            if (column != ToColumn(CombatGemColumn::COLUMN_TARGET_LEVEL)) return;  // 목표 레벨 열이 아니면 무시
            if (const auto onGemCellEdited{ m_onGemCellEdited }) onGemCellEdited(row, text);  // 등록된 콜백이 있으면 행 · 글자를 알림
        });

        // 보석 목표: 고르면 번호만 알림
        m_gemTargetBox.SetOnSelectionChanged([this]  // 보석 전체 목표 선택이 바뀌면
        {
            const std::optional<std::size_t> index{ m_gemTargetBox.GetSelectedIndex() };  // 선택한 번호
            if (const auto onGemTargetSelected{ m_onGemTargetSelected }; index.has_value() && onGemTargetSelected) onGemTargetSelected(*index);  // 콜백이 있고 번호가 있으면 알림
        });

        // 목록형 카드: 선택 상자 · 안내 · 표 (Tab 순서 = 카드 순서)
        for (std::size_t index{}; index < CARD_COUNT; ++index)  // 카드마다 부품 만들기
        {
            ListCard& card{ m_cards[index] };  // 카드 참조
            card.m_panel = Panel{ Core::RectF{}, std::wstring{ CARD_TITLES[index] } };  // 카드 상자 초기화
            card.m_combo = &m_host.Emplace<UI::ComboBox>();  // 선택 상자 생성
            card.m_hint = &m_host.Emplace<UI::Label>(std::wstring{ HINT_DEFAULT_TEXT }, NOTE_TEXT_STYLE, Theme::MUTED_TEXT_COLOR);  // 선택한 행 안내 글자 생성
            card.m_grid = &m_host.Emplace<UI::Grid>(platform, MakeListColumns(), 0);  // 목록 표 생성

            card.m_combo->SetEnabled(false);  // 처음에는 선택 상자 비활성
            card.m_combo->SetOnSelectionChanged([this, index] { OnCardChoice(index); });  // 선택 상자에서 고르면 카드 선택 처리

            card.m_grid->SetEditInput(false, IsDigitCharacter);  // 목록 표는 숫자만 입력 받음
            card.m_grid->SetOnSelectionChanged([this, index] { SyncCardEditor(index); });  // 표에서 행을 고르면 선택 상자 · 안내 갱신
            card.m_grid->SetOnCellEdited([this, index](std::size_t row, std::size_t column, const std::wstring& text)  // 목록 표 칸 편집이 끝나면
            {
                if (column != LIST_TARGET_COLUMN) return;  // 목표 열이 아니면 무시
                if (const auto onCardEdited{ m_onCardEdited }) onCardEdited(static_cast<CombatCard>(index), row, text);  // 등록된 콜백이 있으면 카드 · 행 · 글자를 알림
            });
        }
    }

    CombatSimulatorView::~CombatSimulatorView() = default;  // 소멸자 기본 구현 (부품은 호스트가 해제)

    void CombatSimulatorView::SetOnSearch(std::function<void(const std::wstring& characterName)> onSearch)  // 조회 알림 연결
    {
        m_onSearch = std::move(onSearch);  // 콜백 보관
    }

    void CombatSimulatorView::SetOnGearCellEdited(std::function<void(std::size_t row, CombatGearColumn column, const std::wstring& text)> onGearCellEdited)  // 장비 칸 편집 알림 연결
    {
        m_onGearCellEdited = std::move(onGearCellEdited);  // 콜백 보관
    }

    void CombatSimulatorView::SetOnQualityChanged(std::function<void(const std::wstring& text)> onQualityChanged)  // 무기 품질 변경 알림 연결
    {
        m_onQualityChanged = std::move(onQualityChanged);  // 콜백 보관
    }

    void CombatSimulatorView::SetOnGemTargetSelected(std::function<void(std::size_t index)> onGemTargetSelected)  // 보석 목표 선택 알림 연결
    {
        m_onGemTargetSelected = std::move(onGemTargetSelected);  // 콜백 보관
    }

    void CombatSimulatorView::SetOnGemCellEdited(std::function<void(std::size_t row, const std::wstring& text)> onGemCellEdited)  // 보석 칸 편집 알림 연결
    {
        m_onGemCellEdited = std::move(onGemCellEdited);  // 콜백 보관
    }

    void CombatSimulatorView::SetOnCardEdited(std::function<void(CombatCard card, std::size_t row, const std::wstring& text)> onCardEdited)  // 목록형 카드 편집 알림 연결
    {
        m_onCardEdited = std::move(onCardEdited);  // 콜백 보관
    }

    void CombatSimulatorView::SetCharacterName(std::wstring name)  // 캐릭터 이름 칸 글자 설정
    {
        m_nameBox.SetText(std::move(name));  // 이름 입력 칸에 글자 반영
    }

    void CombatSimulatorView::SetSearchBusy(bool isBusy)  // 조회 중 표시 설정
    {
        m_searchButton.SetEnabled(!isBusy);  // 조회 중에는 버튼 비활성
        m_searchButton.SetText(std::wstring{ isBusy ? SEARCH_BUSY_TEXT : SEARCH_TEXT });  // 조회 중이면 "조회 중..." 글자로 바꿈
    }

    void CombatSimulatorView::SetStatusText(std::wstring text, bool isError)  // 상태 글자 설정
    {
        m_statusLabel.SetText(std::move(text));  // 상태 글자 반영
        m_statusLabel.SetColor(isError ? Theme::ALERT_TEXT_COLOR : Theme::TEXT_COLOR);  // 오류면 경고색, 아니면 기본색
    }

    // 장비 표 다시 채우기 (목표 칸 = 편집 가능하면 일반 칸 색, 바뀐 칸은 강조색 · 굵게, 아이템 = 등급 색)
    void CombatSimulatorView::ShowGears(const std::vector<CombatGearRow>& rows)  // 장비 표 다시 채우기
    {
        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };  // 읽기 전용 칸 모양
        const UI::GridCellStyle lockedStyle{ Theme::READONLY_BACK_COLOR, Theme::MUTED_TEXT_COLOR };  // 잠긴 칸 모양 (흐린 글자색)

        m_gearGrid.SetRowCount(rows.size());  // 행 개수 맞추기
        for (std::size_t row{}; row < rows.size(); ++row)  // 행마다
        {
            const CombatGearRow& gear{ rows[row] };  // 이번 행의 장비
            const auto setCell{ [this, row](CombatGearColumn column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)  // 칸 하나를 채우는 람다
            {
                m_gearGrid.SetCellText(row, ToColumn(column), text);  // 칸 글자 설정
                m_gearGrid.SetCellStyle(row, ToColumn(column), style);  // 칸 모양 설정
                m_gearGrid.SetCellEditable(row, ToColumn(column), isEditable);  // 칸 편집 가능 여부 설정
            } };

            const UI::GridCellStyle targetStyle{ Theme::CELL_BACK_COLOR, gear.m_hasChange ? Theme::UP_TEXT_COLOR : Theme::TEXT_COLOR,  // 목표 칸 모양 (바뀌었으면 강조색 · 굵게)
                gear.m_hasChange ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL };

            setCell(CombatGearColumn::COLUMN_PART, gear.m_partName, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD }, false);  // 부위 이름 칸 (편집 불가)
            setCell(CombatGearColumn::COLUMN_ITEM, gear.m_itemName, UI::GridCellStyle{ Theme::CELL_BACK_COLOR, Theme::GetGradeColor(gear.m_gradeName), Core::FontWeight::BOLD }, false);  // 아이템 이름 칸 (등급 색, 편집 불가)
            setCell(CombatGearColumn::COLUMN_CURRENT_ENHANCEMENT, gear.m_currentEnhancement, readonlyStyle, false);  // 현재 일반 재련 칸
            setCell(CombatGearColumn::COLUMN_CURRENT_ADVANCED, gear.m_currentAdvanced, readonlyStyle, false);  // 현재 상급 재련 칸
            setCell(CombatGearColumn::COLUMN_TARGET_ENHANCEMENT, gear.m_targetEnhancement, gear.m_isTargetEditable ? targetStyle : lockedStyle, gear.m_isTargetEditable);  // 목표 일반 재련 칸 (표가 있으면 편집 가능)
            const bool isAdvancedEditable{ gear.m_isTargetEditable && gear.m_isTargetAdvancedEditable };  // 상급 재련 편집 가능 여부
            setCell(CombatGearColumn::COLUMN_TARGET_ADVANCED, gear.m_targetAdvanced, isAdvancedEditable ? targetStyle : lockedStyle, isAdvancedEditable);  // 목표 상급 재련 칸
        }
    }

    void CombatSimulatorView::SetQualityText(std::wstring currentText)  // 무기 품질 안내 글자 설정
    {
        m_qualityLabel.SetText(std::move(currentText));  // 글자 반영
    }

    void CombatSimulatorView::SetQualityTarget(std::wstring text)  // 무기 품질 목표 칸 글자 설정 (알림 없음)
    {
        if (m_qualityBox.GetText() == text) return;  // 이미 같은 글자면 건너뜀
        m_isUpdatingQualityText = true;  // 코드에서 바꾸는 중 표시 (알림 막기)
        m_qualityBox.SetText(std::move(text));  // 입력 칸에 글자 반영
        m_isUpdatingQualityText = false;  // 표시 해제
    }

    void CombatSimulatorView::SetQualityEnabled(bool isEnabled) noexcept  // 무기 품질 입력 칸 활성 여부 설정
    {
        m_qualityBox.SetEnabled(isEnabled);  // 입력 칸 활성 상태 반영
    }

    void CombatSimulatorView::SetGemText(std::wstring text)  // 보석 요약 글자 설정
    {
        m_gemLabel.SetText(std::move(text));  // 글자 반영
    }

    void CombatSimulatorView::SetGemTargetItems(std::vector<std::wstring> items)  // 보석 전체 목표 선택지 설정
    {
        m_gemTargetBox.SetItems(std::move(items));  // 선택 상자에 선택지 반영
    }

    void CombatSimulatorView::SetGemTargetIndex(std::size_t index)  // 보석 전체 목표 선택 번호 설정
    {
        m_gemTargetBox.SetSelectedIndex(index);  // 선택 번호 반영
    }

    void CombatSimulatorView::SetGemTargetEnabled(bool isEnabled) noexcept  // 보석 전체 목표 선택 상자 활성 여부 설정
    {
        m_gemTargetBox.SetEnabled(isEnabled);  // 선택 상자 활성 상태 반영
    }

    // 보석 표 다시 채우기 (목표 레벨 = 편집 칸, 바뀐 칸은 강조색 · 굵게)
    void CombatSimulatorView::ShowGems(const std::vector<CombatGemRow>& rows)
    {
        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };  // 읽기 전용 칸 모양

        m_gemGrid.SetRowCount(rows.size());  // 행 개수 맞추기
        for (std::size_t row{}; row < rows.size(); ++row)  // 행마다
        {
            const CombatGemRow& gem{ rows[row] };  // 이번 행의 보석
            const auto setCell{ [this, row](CombatGemColumn column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)  // 칸 하나를 채우는 람다
            {
                m_gemGrid.SetCellText(row, ToColumn(column), text);  // 칸 글자 설정
                m_gemGrid.SetCellStyle(row, ToColumn(column), style);  // 칸 모양 설정
                m_gemGrid.SetCellEditable(row, ToColumn(column), isEditable);  // 칸 편집 가능 여부 설정
            } };

            setCell(CombatGemColumn::COLUMN_SLOT, gem.m_slot, UI::GridCellStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::BOLD }, false);  // 슬롯 번호 칸 (편집 불가)
            setCell(CombatGemColumn::COLUMN_KIND, gem.m_kind, readonlyStyle, false);  // 보석 종류 칸 (편집 불가)
            setCell(CombatGemColumn::COLUMN_CURRENT_LEVEL, gem.m_currentLevel, readonlyStyle, false);  // 현재 레벨 칸 (편집 불가)
            setCell(CombatGemColumn::COLUMN_TARGET_LEVEL, gem.m_targetLevel, UI::GridCellStyle{ Theme::CELL_BACK_COLOR,  // 목표 레벨 칸 (편집 가능, 바뀌었으면 강조색 · 굵게)
                gem.m_hasChange ? Theme::UP_TEXT_COLOR : Theme::TEXT_COLOR, gem.m_hasChange ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL }, true);
        }
    }

    // 목록형 카드 다시 채우기 (구분 행 = 굵은 제목, 숫자 행 = 목표 칸 편집, 선택 행 = 위 선택 상자로 고르기)
    void CombatSimulatorView::ShowCard(CombatCard card, std::vector<CombatListRow> rows)
    {
        const std::size_t cardIndex{ static_cast<std::size_t>(card) };  // 카드 번호
        if (cardIndex >= CARD_COUNT) return;  // 범위 밖 카드면 무시

        ListCard& listCard{ m_cards[cardIndex] };  // 카드 참조
        UI::Grid& grid{ *listCard.m_grid };  // 카드의 표
        listCard.m_rows = std::move(rows);  // 행 목록 보관

        const UI::GridCellStyle readonlyStyle{ Theme::READONLY_BACK_COLOR, Theme::TEXT_COLOR };  // 읽기 전용 칸 모양
        const UI::GridCellStyle labelStyle{ Theme::LABEL_BACK_COLOR, Theme::HEADER_TEXT_COLOR, Core::FontWeight::NORMAL };  // 항목 이름 칸 모양
        const UI::GridCellStyle headerStyle{ Theme::HEADER_BACK_COLOR, Theme::TOTAL_TEXT_COLOR, Core::FontWeight::BOLD };  // 구분 제목 행 모양

        grid.SetRowCount(listCard.m_rows.size());  // 행 개수 맞추기
        for (std::size_t row{}; row < listCard.m_rows.size(); ++row)  // 행마다
        {
            const CombatListRow& item{ listCard.m_rows[row] };  // 이번 행
            const bool isHeader{ item.m_kind == CombatRowKind::ROW_HEADER };  // 구분 제목 행 여부
            const bool isNumber{ item.m_kind == CombatRowKind::ROW_NUMBER };  // 숫자 입력 행 여부

            const auto setCell{ [&grid, row](std::size_t column, const std::wstring& text, const UI::GridCellStyle& style, bool isEditable)  // 칸 하나를 채우는 람다
            {
                grid.SetCellText(row, column, text);  // 칸 글자 설정
                grid.SetCellStyle(row, column, style);  // 칸 모양 설정
                grid.SetCellEditable(row, column, isEditable);  // 칸 편집 가능 여부 설정
            } };

            const UI::GridCellStyle targetStyle{ Theme::CELL_BACK_COLOR, item.m_hasChange ? Theme::UP_TEXT_COLOR : Theme::TEXT_COLOR,  // 목표 칸 모양 (바뀌었으면 강조색 · 굵게)
                item.m_hasChange ? Core::FontWeight::BOLD : Core::FontWeight::NORMAL };

            setCell(LIST_LABEL_COLUMN, item.m_label, isHeader ? headerStyle : labelStyle, false);  // 항목 이름 칸
            setCell(LIST_CURRENT_COLUMN, item.m_current, isHeader ? headerStyle : readonlyStyle, false);  // 현재 값 칸
            setCell(LIST_TARGET_COLUMN, item.m_target, isHeader ? headerStyle : targetStyle, isNumber);  // 목표 값 칸 (숫자 행만 편집 가능)
        }
        SyncCardEditor(cardIndex);  // 선택 상자 · 안내 갱신
    }

    // 선택한 행에 맞춰 선택 상자 · 안내 갱신 (선택지가 있는 행이면 상자를 켜고 목표를 표시)
    void CombatSimulatorView::SyncCardEditor(std::size_t cardIndex)
    {
        ListCard& card{ m_cards[cardIndex] };  // 카드 참조
        const std::optional<std::size_t> selected{ card.m_grid->GetSelectedRow() };  // 표에서 선택한 행 번호
        const CombatListRow* row{ selected.has_value() && *selected < card.m_rows.size() ? &card.m_rows[*selected] : nullptr };  // 선택한 행의 내용 (없으면 빈 포인터)

        if (row == nullptr || row->m_kind == CombatRowKind::ROW_HEADER)  // 선택한 행이 없거나 구분 제목이면
        {
            card.m_combo->SetItems({});  // 선택지 비움
            card.m_combo->SetEnabled(false);  // 선택 상자 비활성
            card.m_hint->SetText(std::wstring{ HINT_DEFAULT_TEXT });  // 기본 안내 글자 표시
            return;  // 처리 끝
        }

        const bool isChoice{ row->m_kind == CombatRowKind::ROW_CHOICE };  // 선택 행 여부
        card.m_combo->SetItems(isChoice ? row->m_choices : std::vector<std::wstring>{});  // 선택 행이면 선택지 설정, 아니면 비움
        card.m_combo->SetSelectedIndex(isChoice ? row->m_selectedChoice : std::nullopt);  // 선택 행이면 현재 목표 선택지 표시
        card.m_combo->SetEnabled(isChoice);  // 선택 행일 때만 선택 상자 활성
        card.m_hint->SetText(isChoice ? std::format(HINT_CHOICE_FORMAT, row->m_label, row->m_current, row->m_target)  // 안내 글자: 선택 행 / 숫자 행 서식 구분
            : std::format(HINT_NUMBER_FORMAT, row->m_label, row->m_current, row->m_target));
    }

    // 선택 상자에서 고름 → 선택한 행의 선택지 글자를 알림
    void CombatSimulatorView::OnCardChoice(std::size_t cardIndex)
    {
        ListCard& card{ m_cards[cardIndex] };  // 카드 참조
        const std::optional<std::size_t> selectedRow{ card.m_grid->GetSelectedRow() };  // 표에서 선택한 행 번호
        const std::optional<std::size_t> choice{ card.m_combo->GetSelectedIndex() };  // 선택 상자에서 고른 선택지 번호
        if (!selectedRow.has_value() || !choice.has_value() || *selectedRow >= card.m_rows.size()) return;  // 선택한 행 · 선택지가 없거나 범위 밖이면 무시

        const CombatListRow& row{ card.m_rows[*selectedRow] };  // 선택한 행
        if (row.m_kind != CombatRowKind::ROW_CHOICE || *choice >= row.m_choices.size()) return;  // 선택 행이 아니거나 선택지 번호가 범위 밖이면 무시

        const std::wstring chosenText{ row.m_choices[*choice] };   // 알림 뒤 표가 다시 채워지므로 복사해서 넘김
        if (const auto onCardEdited{ m_onCardEdited }) onCardEdited(static_cast<CombatCard>(cardIndex), *selectedRow, chosenText);  // 등록된 콜백이 있으면 카드 · 행 · 선택지 글자를 알림
    }

    // 결과 바 (증감 색: 오르면 빨강 ▲ · 내리면 파랑 ▼, 다른 표와 같은 색)
    void CombatSimulatorView::ShowResult(const CombatResultText& result)
    {
        m_currentPowerLabel.SetText(result.m_currentPower);  // 현재 전투력 글자 반영
        m_predictedPowerLabel.SetText(result.m_predictedPower);  // 예상 전투력 글자 반영
        m_changeLabel.SetText(result.m_change);  // 증감 글자 반영
        m_changeLabel.SetColor(result.m_isUp ? Theme::UP_TEXT_COLOR : (result.m_isDown ? Theme::DOWN_TEXT_COLOR : Theme::TEXT_COLOR));  // 증가면 상승색, 감소면 하락색, 같으면 기본색
        m_itemLevelLabel.SetText(result.m_itemLevel);  // 아이템 레벨 글자 반영
        m_factorLabel.SetText(result.m_factors);  // 배율 내역 글자 반영
        m_noteLabel.SetText(BuildNoteText(result.m_note));  // 안내 문구 반영 (모르는 혼돈 코어 안내 포함)
    }

    void CombatSimulatorView::Update(double deltaSeconds)  // 프레임 갱신
    {
        m_host.Update(deltaSeconds);  // 부품 호스트 갱신
    }

    void CombatSimulatorView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)  // 화면 그리기
    {
        Layout(contentArea);  // 배치 계산 (영역이 같으면 건너뜀)
        RenderPanels(renderer);  // 상자 테두리 · 제목 그리기
        m_host.Render(renderer);  // 부품 그리기
    }

    bool CombatSimulatorView::HandleInput(const Core::InputEvent& event)  // 입력 처리
    {
        return m_host.HandleInput(event);  // 부품 호스트에 입력 전달
    }

    Core::CursorShape CombatSimulatorView::GetCursor() const noexcept  // 마우스 커서 모양 조회
    {
        return m_host.GetCursor();  // 부품 호스트의 커서 모양 반환
    }

    void CombatSimulatorView::SuspendFocus()  // 입력 포커스 일시 해제
    {
        m_host.SuspendFocus();  // 부품 호스트 포커스 해제
    }

    void CombatSimulatorView::RestoreFocus()  // 입력 포커스 복원
    {
        m_host.RestoreFocus();  // 부품 호스트 포커스 복원
    }

    void CombatSimulatorView::RequestSearch()  // 조회 요청 보내기
    {
        m_nameBox.CommitComposition();   // 조합 중인 한글 마지막 글자까지 포함
        if (const auto onSearch{ m_onSearch }) onSearch(m_nameBox.GetText());  // 등록된 콜백이 있으면 이름을 알림
    }

    // 맨 위 결과 바 / 3 x 2 카드 / 맨 아래 안내 문구
    void CombatSimulatorView::Layout(const Core::RectF& contentArea)
    {
        if (m_hasLayout && m_layoutArea == contentArea) return;  // 영역이 같으면 이미 계산했으므로 건너뜀
        m_layoutArea = contentArea;  // 영역 기억
        m_hasLayout = true;  // 배치 계산 완료 표시

        const float left{ contentArea.m_left + PAGE_PADDING };  // 화면 왼쪽
        const float right{ std::max(contentArea.m_right - PAGE_PADDING, left) };  // 화면 오른쪽 (왼쪽보다 작아지지 않게)
        const float barTop{ contentArea.m_top + PAGE_PADDING };  // 결과 바 위쪽
        const float innerLeft{ left + BAR_PADDING };  // 결과 바 안쪽 왼쪽
        const float innerRight{ std::max(right - BAR_PADDING, innerLeft) };  // 결과 바 안쪽 오른쪽

        // 줄 하나에 왼쪽부터 차례로 배치
        const auto placeRow{ [](float& x, float top, UI::Control& control, float width, float gapAfter)  // 부품 하나를 가로로 놓고 x를 옮기는 람다
        {
            control.SetBounds(Core::RectF{ x, top, x + width, top + Theme::CONTROL_HEIGHT });  // 부품 영역 설정
            x += width + gapAfter;  // 다음 부품 위치로 이동
        } };

        // 결과 바 첫 줄: 캐릭터 이름 [입력칸] [조회] 상태
        float y{ barTop + BAR_PADDING };  // 첫 줄 높이
        float x{ innerLeft };  // 가로 위치 시작
        placeRow(x, y, m_nameLabel, NAME_LABEL_WIDTH, LABEL_GAP);  // 이름 안내 글자 배치
        placeRow(x, y, m_nameBox, NAME_BOX_WIDTH, GROUP_GAP);  // 이름 입력 칸 배치
        placeRow(x, y, m_searchButton, SEARCH_BUTTON_WIDTH, SECTION_GAP);  // 조회 버튼 배치
        m_statusLabel.SetBounds(Core::RectF{ x, y, std::max(x, innerRight), y + Theme::CONTROL_HEIGHT });  // 상태 글자는 남은 너비 전체
        y += Theme::CONTROL_HEIGHT + ROW_GAP;  // 다음 줄로 이동

        // 둘째 줄: 현재 전투력 → 예상 전투력 · 증감 · 아이템 레벨
        const float innerWidth{ innerRight - innerLeft };  // 결과 줄 안쪽 너비
        const float currentWidth{ innerWidth * RESULT_FRACTION_CURRENT };  // 현재 전투력 칸 너비
        const float predictedWidth{ innerWidth * RESULT_FRACTION_PREDICTED };  // 예상 전투력 칸 너비
        const float changeWidth{ innerWidth * RESULT_FRACTION_CHANGE };  // 증감 칸 너비
        x = innerLeft;  // 가로 위치 처음으로
        placeRow(x, y, m_currentPowerLabel, currentWidth, 0.0f);  // 현재 전투력 배치
        placeRow(x, y, m_predictedPowerLabel, predictedWidth, 0.0f);  // 예상 전투력 배치
        placeRow(x, y, m_changeLabel, changeWidth, 0.0f);  // 증감 배치
        m_itemLevelLabel.SetBounds(Core::RectF{ x, y, std::max(x, innerRight), y + Theme::CONTROL_HEIGHT });  // 아이템 레벨은 남은 너비 전체
        y += Theme::CONTROL_HEIGHT + ROW_GAP;  // 다음 줄로 이동

        // 셋째 줄: 배율 내역
        m_factorLabel.SetBounds(Core::RectF{ innerLeft, y, innerRight, y + Theme::CONTROL_HEIGHT });  // 배율 내역 배치
        y += Theme::CONTROL_HEIGHT + BAR_PADDING;  // 아래쪽 여백 반영
        m_barPanel.m_bounds = Core::RectF{ left, barTop, right, y };  // 결과 바 상자 영역 확정

        // 맨 아래 안내 문구
        const float noteHeight{ NOTE_LINE_HEIGHT * NOTE_LINE_COUNT };  // 안내 문구 높이
        const float noteTop{ std::max(y + SECTION_GAP, contentArea.m_bottom - PAGE_PADDING - noteHeight) };  // 안내 문구 위치 (카드 영역과 겹치지 않게)
        m_noteLabel.SetBounds(Core::RectF{ left, noteTop, right, noteTop + noteHeight });  // 안내 문구 배치

        // 3 x 2 카드
        const float cardsTop{ y + SECTION_GAP };  // 카드 영역 위쪽
        const float cardsBottom{ std::max(cardsTop + MIN_CARD_HEIGHT * static_cast<float>(GRID_ROW_COUNT) + SECTION_GAP, noteTop - SECTION_GAP) };  // 카드 영역 아래쪽 (최소 높이 보장)
        const Core::RectF cardsArea{ left, cardsTop, right, cardsBottom };  // 카드 영역

        // 장비 재련 카드: 표 하나 (완갑 행 포함)
        m_gearGrid.SetBounds(LayoutPanel(m_gearPanel, GetCellBounds(cardsArea, GEAR_CELL)));  // 장비 표 배치

        // 보석 · 무기 품질 카드
        const Core::RectF optionInner{ LayoutPanel(m_optionPanel, GetCellBounds(cardsArea, GEM_CELL)) };  // 보석 · 품질 상자 안쪽 영역
        float rowTop{ optionInner.m_top };  // 줄 위쪽 위치
        x = optionInner.m_left;  // 가로 위치 시작
        placeRow(x, rowTop, m_qualityLabel, std::min(QUALITY_LABEL_WIDTH, optionInner.GetWidth() - QUALITY_BOX_WIDTH - LABEL_GAP), LABEL_GAP);  // 무기 품질 안내 글자 배치
        placeRow(x, rowTop, m_qualityBox, QUALITY_BOX_WIDTH, GROUP_GAP);  // 무기 품질 입력 칸 배치
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;  // 다음 줄로 이동
        m_gemLabel.SetBounds(Core::RectF{ optionInner.m_left, rowTop, optionInner.m_right, rowTop + Theme::CONTROL_HEIGHT });  // 보석 요약 글자 배치
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;  // 다음 줄로 이동
        x = optionInner.m_left;  // 가로 위치 시작
        placeRow(x, rowTop, m_gemTargetLabel, GEM_TARGET_LABEL_WIDTH, LABEL_GAP);  // 보석 전체 목표 안내 글자 배치
        placeRow(x, rowTop, m_gemTargetBox, GEM_TARGET_BOX_WIDTH, GROUP_GAP);  // 보석 전체 목표 선택 상자 배치
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;  // 다음 줄로 이동
        m_gemGrid.SetBounds(Core::RectF{ optionInner.m_left, rowTop, optionInner.m_right, std::max(rowTop, optionInner.m_bottom) });   // 남은 높이 전부 (넘치면 표가 스크롤)

        // 목록형 카드 4개
        for (std::size_t index{}; index < CARD_COUNT; ++index) LayoutListCard(m_cards[index], GetCellBounds(cardsArea, CARD_CELLS[index]));  // 목록형 카드 배치
    }

    // 목록형 카드: 위 선택 상자 / 선택한 행 안내 / 아래 표 (펼친 목록이 표 위에 덮이도록 선택 상자를 맨 위에 둠)
    void CombatSimulatorView::LayoutListCard(ListCard& card, const Core::RectF& bounds)
    {
        const Core::RectF inner{ LayoutPanel(card.m_panel, bounds) };  // 카드 안쪽 영역
        float rowTop{ inner.m_top };  // 줄 위쪽 위치
        card.m_combo->SetBounds(Core::RectF{ inner.m_left, rowTop, inner.m_right, rowTop + Theme::CONTROL_HEIGHT });  // 선택 상자 배치
        rowTop += Theme::CONTROL_HEIGHT + ROW_GAP;  // 다음 줄로 이동

        const float hintHeight{ HINT_LINE_HEIGHT * HINT_LINE_COUNT };  // 안내 높이
        card.m_hint->SetBounds(Core::RectF{ inner.m_left, rowTop, inner.m_right, rowTop + hintHeight });  // 안내 글자 배치
        rowTop += hintHeight + ROW_GAP;  // 다음 줄로 이동
        card.m_grid->SetBounds(Core::RectF{ inner.m_left, rowTop, inner.m_right, std::max(rowTop, inner.m_bottom) });  // 표 배치
    }

    Core::RectF CombatSimulatorView::LayoutPanel(Panel& panel, const Core::RectF& bounds) const
    {
        panel.m_bounds = bounds;  // 상자 영역 기록
        return Core::RectF{ bounds.m_left + PANEL_PADDING, bounds.m_top + Theme::PANEL_TITLE_HEIGHT,  // 테두리 · 제목 자리를 뺀 안쪽 영역 반환
            std::max(bounds.m_right - PANEL_PADDING, bounds.m_left + PANEL_PADDING), std::max(bounds.m_top + Theme::PANEL_TITLE_HEIGHT, bounds.m_bottom - PANEL_PADDING) };
    }

    // 묶음 상자: 둥근 테두리 + 왼쪽 위 제목 (결과 바는 제목 없음)
    void CombatSimulatorView::RenderPanels(Core::Renderer& renderer) const
    {
        std::vector<const Panel*> panels{ &m_barPanel, &m_gearPanel, &m_optionPanel };  // 그릴 상자 목록 (결과 바 · 장비 · 보석 상자)
        for (const ListCard& card : m_cards) panels.push_back(&card.m_panel);  // 목록형 카드 상자 추가

        for (const Panel* panel : panels)  // 상자마다
        {
            renderer.FillRoundedRect(panel->m_bounds, Theme::PANEL_CORNER_RADIUS, Theme::CARD_BACK_COLOR);   // 카드 바탕 (배경보다 살짝 밝게)
            renderer.DrawRoundedRect(panel->m_bounds, Theme::PANEL_CORNER_RADIUS, Theme::CARD_BORDER_COLOR, Theme::BORDER_WIDTH);   // 카드 얇은 테두리
            if (panel->m_title.empty()) continue;  // 제목이 없으면 건너뜀

            const Core::RectF titleRect{ panel->m_bounds.m_left + PANEL_TITLE_LEFT, panel->m_bounds.m_top,  // 제목 영역
                panel->m_bounds.m_right - PANEL_TITLE_LEFT, panel->m_bounds.m_top + Theme::PANEL_TITLE_HEIGHT };
            renderer.DrawString(panel->m_title, titleRect, Theme::HEADER_TEXT_COLOR, PANEL_TITLE_STYLE);  // 제목 글자 그리기
        }
    }
}
