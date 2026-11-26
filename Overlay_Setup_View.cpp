module LostArkMultiTool.Views.OverlaySetupView;  // 오버레이 · 인식 설정 화면 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Core.Input;  // 입력 이벤트 가져오기
import LostArkMultiTool.Core.LocalTime;  // 시각 글자 가져오기
import LostArkMultiTool.Core.Platform;  // 플랫폼 가져오기
import LostArkMultiTool.Core.Renderer;  // 렌더러 가져오기
import LostArkMultiTool.Core.TextEncoding;  // UTF-8 → 넓은 글자 가져오기
import LostArkMultiTool.Views.CalculatorManager;  // 현재 캐릭터 · 화면 인식 상태 형식 가져오기
import LostArkMultiTool.Views.CharacterPanel;  // 현재 캐릭터 패널 이름 가져오기
import LostArkMultiTool.Views.MaterialPanel;  // 재료 패널 이름 가져오기
import LostArkMultiTool.Views.MiniToolPanel;  // 미니 탭 패널 이름 가져오기
import LostArkMultiTool.Views.OverlayManager;  // 오버레이 요소 형식 가져오기
import LostArkMultiTool.Views.PageLayout;  // 카드 · 배치 도우미 가져오기
import LostArkMultiTool.Views.ParadisePanel;  // 낙원력 패널 이름 가져오기
import LostArkMultiTool.Views.Theme;  // 색 · 글꼴 크기 가져오기
import LostArkMultiTool.Views.UI.Button;  // 버튼 부품 가져오기
import LostArkMultiTool.Views.UI.CheckBox;  // 체크 상자 부품 가져오기
import LostArkMultiTool.Views.UI.ComboBox;  // 펼침 목록 부품 가져오기
import LostArkMultiTool.Views.UI.ControlHost;  // 부품 호스트 가져오기
import LostArkMultiTool.Views.UI.Slider;  // 값 막대 부품 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기
import LostArkMultiTool.Vision.ScreenWatcher;  // 감시 상태 가져오기

namespace LostArkMultiTool::Views  // 화면 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        // ===== 쪽 =====
        constexpr std::size_t PAGE_COUNT{ static_cast<std::size_t>(SetupPage::PAGE_COUNT) };   // 쪽 수
        constexpr std::array<std::wstring_view, PAGE_COUNT> PAGE_TITLES{ L"현재 캐릭터", L"UI 구성", L"인식 설정" };   // 쪽 단추 글자

        // ===== 요소 (오버레이 패널, 설정 파일 이름과 같은 순서) =====
        struct ElementSpec
        {
            std::string_view m_id;               // 패널 이름
            std::wstring_view m_title;           // 표시 이름
            std::wstring_view m_description;     // 설명 · 기본 위치
        };
        const std::array<ElementSpec, OverlaySetupView::ELEMENT_COUNT> ELEMENT_SPECS{ {
            { CharacterPanel::PANEL_ID, L"현재 캐릭터", L"이름 · 직업 · 아이템 레벨 · 알아본 방법 (조작 모드에서 [변경]) · 기본: 왼쪽 위" },
            { MaterialPanel::PANEL_ID, L"재료 시세 · 자산", L"재료 수량 · 단가 · 가치 · 캐릭터/원정대 자산 · 기본: 오른쪽 위" },
            { ParadisePanel::PANEL_ID, L"낙원력 주간 현황", L"이번 주차 캐릭터별 낙원력 (미기록 강조) · 기본: 오른쪽 위" },
            { MiniToolPanel::PANEL_ID, L"탭 미니 모드", L"멀티툴 탭을 게임 위에서 그대로 (◀ ▶ · 크기 조절) · 기본: 화면 가운데" },
        } };

        // ===== 캐릭터 확인 간격 =====
        constexpr std::array<int, 4> INTERVAL_SECONDS{ 1, 2, 3, 5 };                           // 고를 수 있는 간격 (초)
        constexpr std::wstring_view INTERVAL_FORMAT{ L"{}초" };                                // 목록 글자

        // ===== 글자 =====
        constexpr std::wstring_view CHARACTER_EYEBROW{ L"CHARACTER" };
        constexpr std::wstring_view CHARACTER_TITLE{ L"현재 캐릭터 자동 인식" };
        constexpr std::wstring_view CHARACTER_HINT{ L"머리 위 이름표 · 캐릭터 정보 창 · 로딩 화면" };
        constexpr std::wstring_view CHARACTER_DESCRIPTION{ L"게임에 접속하면 머리 위 내 이름표(내 이름만 연한 노란색)로 지금 캐릭터를 알아보고, 화면 인식으로 읽은 재료 · 운명의 파편 · 낙원력을 그 캐릭터에 넣어요. 처음 한 번은 이름표 글자를 읽어 원정대 이름과 맞추고, 그다음부터는 기억한 이름표 모양으로 바로 알아봐요. 로딩 화면이 보이면 다시 확인해요." };
        constexpr std::wstring_view AUTO_TEXT{ L"자동 인식 (끄면 직접 고른 캐릭터만 써요)" };
        constexpr std::wstring_view PICK_LABEL_TEXT{ L"직접 고르기" };
        constexpr std::wstring_view PICK_TEXT{ L"이 캐릭터로 지정" };
        constexpr std::wstring_view REFRESH_TEXT{ L"정보 갱신" };
        constexpr std::wstring_view MEMORY_EYEBROW{ L"MEMORY" };
        constexpr std::wstring_view MEMORY_TITLE{ L"기억한 캐릭터" };
        constexpr std::wstring_view MEMORY_HINT{ L"직접 고른 캐릭터는 다음부터 바로 알아봐요" };
        constexpr std::wstring_view SHAPE_TEXT_FORMAT{ L"이름표 모양 · {}명: 머리 위 내 이름표 모양을 캐릭터별로 기억해요. 같은 이름표가 보이면 글자를 읽지 않고 바로 알아봐요." };
        constexpr std::wstring_view ALIAS_TEXT_FORMAT{ L"읽은 글자 · {}개: 글자 인식이 늘 다르게 읽는 이름(겹받침 등)을 [변경] · 직접 고르기로 고른 캐릭터로 기억해요." };
        constexpr std::wstring_view CLEAR_SHAPES_FORMAT{ L"초기화 ({}명)" };
        constexpr std::wstring_view CLEAR_ALIASES_FORMAT{ L"초기화 ({}개)" };

        constexpr std::wstring_view LAYOUT_EYEBROW{ L"LAYOUT" };
        constexpr std::wstring_view LAYOUT_TITLE{ L"화면 배치" };
        constexpr std::wstring_view LAYOUT_HINT{ L"Ctrl+Shift+O 켜기 · Ctrl+Shift+I 편집" };
        constexpr std::wstring_view OVERLAY_TEXT{ L"인게임 오버레이 켜기 (게임이 맨 앞일 때만 보여요)" };
        constexpr std::wstring_view EDIT_LABEL_TEXT{ L"끌어 옮기기" };
        constexpr std::wstring_view LOCK_TEXT{ L"잠금" };
        constexpr std::wstring_view EDIT_TEXT{ L"편집" };
        constexpr std::wstring_view EDITING_STATUS_TEXT{ L"편집 중 · 게임 화면에서 패널 머리말을 끌어 옮기고, 다 되면 [잠금] 또는 Ctrl+Shift+I" };
        constexpr std::wstring_view LOCKED_STATUS_TEXT{ L"잠김 · 평소엔 클릭이 게임으로 지나가요. [편집]으로 게임 위에서 패널을 옮겨요" };
        constexpr std::wstring_view NO_GAME_STATUS_TEXT{ L"게임 창을 못 찾았어요 · 창 모드나 전체 창 모드로 켠 뒤 [편집]을 눌러 주세요" };
        constexpr std::wstring_view OVERLAY_OFF_STATUS_TEXT{ L"오버레이가 꺼져 있어요. 위에서 먼저 켜 주세요." };
        constexpr std::wstring_view BACK_LABEL_TEXT{ L"패널 바탕 진하기" };
        constexpr std::wstring_view ELEMENTS_EYEBROW{ L"ELEMENTS" };
        constexpr std::wstring_view ELEMENTS_TITLE{ L"요소별 설정" };
        constexpr std::wstring_view ELEMENTS_HINT{ L"투명도 10~100% · 편집 중엔 진하게" };
        constexpr std::wstring_view VISIBLE_TEXT{ L"표시" };
        constexpr std::wstring_view RESET_TEXT{ L"기본 위치로" };
        constexpr std::wstring_view PERCENT_FORMAT{ L"{}%" };

        constexpr std::wstring_view PREVIEW_EYEBROW{ L"PREVIEW" };
        constexpr std::wstring_view PREVIEW_TITLE{ L"인식 화면" };
        constexpr std::wstring_view PREVIEW_HINT{ L"1초마다 갱신 · 이 PC 안에서만 보여 줘요" };
        constexpr std::wstring_view PREVIEW_EMPTY_TITLE{ L"게임 화면이 아직 없어요" };
        constexpr std::wstring_view PREVIEW_EMPTY_HINT{ L"화면 인식을 켜고 게임을 맨 앞에 두면 여기에 보여요" };
        constexpr std::wstring_view LEGEND_TEXT{ L"■ 초록: 내 이름표   ■ 노랑: 마우스 주변 읽기   ■ 파랑: 맨 위 재화 줄 (운명의 파편)" };
        constexpr std::wstring_view STATUS_EYEBROW{ L"STATUS" };
        constexpr std::wstring_view STATUS_TITLE{ L"인식 정보" };
        constexpr std::wstring_view VISION_TEXT{ L"화면 인식 켜기 (귀속재료 계산기 [화면 인식]과 같아요)" };
        constexpr std::wstring_view RESTART_TEXT{ L"다시 찾기" };
        constexpr std::wstring_view INTERVAL_LABEL_TEXT{ L"캐릭터 확인 간격" };

        // ===== 정보 줄 글자 =====
        constexpr std::wstring_view ROW_CURRENT{ L"현재 캐릭터" };
        constexpr std::wstring_view ROW_SOURCE{ L"알아본 방법" };
        constexpr std::wstring_view ROW_STORED{ L"저장된 정보" };
        constexpr std::wstring_view ROW_STATE{ L"상태" };
        constexpr std::wstring_view ROW_ROSTER{ L"원정대" };
        constexpr std::wstring_view ROW_READING{ L"이름 읽기" };
        constexpr std::wstring_view ROW_GAME{ L"게임 창" };
        constexpr std::wstring_view ROW_PLATE{ L"내 이름표" };
        constexpr std::wstring_view ROW_LAST{ L"마지막 값" };
        constexpr std::wstring_view ROW_SPEED{ L"처리 속도" };
        constexpr std::wstring_view ROW_METHOD{ L"읽는 방식" };
        constexpr std::wstring_view NONE_TEXT{ L"-" };
        constexpr std::wstring_view VISION_OFF_TEXT{ L"화면 인식이 꺼져 있어요" };
        constexpr std::wstring_view NO_ROSTER_TEXT{ L"원정대를 아직 불러오지 않았어요 (귀속재료 계산기)" };
        constexpr std::wstring_view CHECKING_TEXT{ L"확인 중 · 이름표나 캐릭터 정보 창(P)을 보여 주세요" };
        constexpr std::wstring_view SOURCE_PLATE_TEXT{ L"머리 위 이름표" };
        constexpr std::wstring_view SOURCE_INFO_TEXT{ L"캐릭터 정보 창" };
        constexpr std::wstring_view SOURCE_MANUAL_TEXT{ L"직접 고름" };
        constexpr std::wstring_view STORED_FORMAT{ L"{} · 아이템 레벨 {:.2f}" };
        constexpr std::wstring_view STORED_LEVEL_FORMAT{ L"아이템 레벨 {:.2f}" };
        constexpr std::wstring_view OUTDATED_FORMAT{ L"아이템 레벨이 달라요 (저장 {:.2f} → 화면 {:.2f}) · [정보 갱신]을 눌러 주세요" };
        constexpr std::wstring_view SYNCING_TEXT{ L"원정대 정보를 받는 중..." };
        constexpr std::wstring_view SAME_TEXT{ L"저장된 정보를 쓰고 있어요" };
        constexpr std::wstring_view ROSTER_FORMAT{ L"{}명 (귀속재료 계산기에서 불러온 목록과만 비교)" };
        constexpr std::wstring_view READING_TEXT{ L"내 이름표는 모양으로, 처음 한 번과 정보 창은 Windows 글자 인식(OCR)으로 읽어요" };
        constexpr std::wstring_view GAME_WATCHING_FORMAT{ L"캡처 중 · {}×{}" };
        constexpr std::wstring_view PLATE_FOUND_FORMAT{ L"찾음 · {}×{} (화면 {}, {})" };
        constexpr std::wstring_view PLATE_MISSING_TEXT{ L"못 찾음 (가려졌거나 화면 가운데에 없음)" };
        constexpr std::wstring_view LAST_FORMAT{ L"{} · {}" };
        constexpr std::wstring_view SPEED_FORMAT{ L"마우스 주변 {:.0f}ms · 캐릭터 {:.0f}ms (평균)" };
        constexpr std::wstring_view METHOD_FORMAT{ L"마우스 멈춤 → 주변 · {}초마다 → 내 이름표" };

        // ===== 배치 (DIP) =====
        constexpr float PAGE_PADDING{ 16.0f };          // 화면 바깥 여백
        constexpr float PAGE_BAR_HEIGHT{ 34.0f };       // 쪽 단추 줄 높이
        constexpr float PAGE_BUTTON_WIDTH{ 120.0f };    // 쪽 단추 너비
        constexpr float PAGE_BUTTON_GAP{ 6.0f };        // 쪽 단추 사이
        constexpr float ACTIVE_LINE_WIDTH{ 2.0f };      // 지금 쪽 밑줄 두께
        constexpr float ROW_GAP{ 10.0f };               // 줄 사이
        constexpr float ITEM_GAP{ 8.0f };               // 부품 사이
        constexpr float INFO_ROW_HEIGHT{ 24.0f };       // 정보 줄 높이
        constexpr float INFO_LABEL_WIDTH{ 120.0f };     // 정보 줄 이름 너비
        constexpr std::size_t CHARACTER_INFO_ROWS{ 6 }; // 현재 캐릭터 정보 줄 수
        constexpr std::size_t VISION_INFO_ROWS{ 6 };    // 인식 정보 줄 수
        constexpr float DESCRIPTION_HEIGHT{ 48.0f };    // 설명 글 높이 (세 줄)
        constexpr float CHECK_WIDTH{ 420.0f };          // 체크 상자 너비 (글자 포함)
        constexpr float COMBO_WIDTH{ 220.0f };          // 펼침 목록 너비
        constexpr float BUTTON_WIDTH{ 132.0f };         // 버튼 너비
        constexpr float SMALL_BUTTON_WIDTH{ 84.0f };    // 작은 버튼 너비
        constexpr float ELEMENT_ROW_HEIGHT{ 54.0f };    // 요소 줄 높이
        constexpr float VISIBLE_BOX_WIDTH{ 64.0f };     // [표시] 체크 너비
        constexpr float SLIDER_WIDTH{ 200.0f };         // 투명도 막대 너비
        constexpr float PERCENT_WIDTH{ 52.0f };         // % 글자 너비
        constexpr float RESET_WIDTH{ 104.0f };          // [기본 위치로] 너비
        constexpr float ELEMENT_TITLE_HEIGHT{ 20.0f };  // 요소 이름 줄 높이
        constexpr float LEGEND_HEIGHT{ 22.0f };         // 상자 색 설명 높이
        constexpr float MIN_PREVIEW_CARD_HEIGHT{ 360.0f };   // 미리 보기 카드 최소 높이
        constexpr std::array<float, 2> VISION_WEIGHTS{ 1.35f, 1.0f };   // 미리 보기 · 정보 카드 너비 비율
        constexpr float BOX_STROKE{ 2.0f };             // 읽는 곳 상자 두께
        constexpr float PREVIEW_BORDER{ 1.0f };         // 미리 보기 테두리 두께
        constexpr float FULL_OPACITY{ 1.0f };           // 그림 불투명도
        constexpr float PERCENT_SCALE{ 100.0f };        // 0 ~ 1 → %
        constexpr double ELEMENT_ALPHA_MIN{ 10.0 };     // 요소 투명도 막대 최솟값 (%)
        constexpr double BACK_OPACITY_MIN{ 30.0 };      // 바탕 진하기 막대 최솟값 (%)
        constexpr double PERCENT_MAX{ 100.0 };          // 막대 최댓값 (%)
        constexpr double PERCENT_STEP{ 5.0 };           // 막대 단위 (%)
        constexpr float HALF{ 0.5f };                   // 절반

        constexpr Core::TextStyle LABEL_STYLE{ Theme::FONT_SIZE_NORMAL };                                                        // 정보 줄 이름
        constexpr Core::TextStyle VALUE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD };                                // 정보 줄 값
        constexpr Core::TextStyle WRAP_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING,
            Core::VerticalAlign::ALIGN_TOP, Core::TextWrap::WORD_WRAP };                                                         // 여러 줄 설명
        constexpr Core::TextStyle TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD };                                // 요소 이름
        constexpr Core::TextStyle SMALL_STYLE{ Theme::FONT_SIZE_SMALL };                                                         // 작은 글자
        constexpr Core::TextStyle PERCENT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::TRAILING };   // % 글자

        // 정보 줄 하나
        struct InfoRow
        {
            std::wstring_view m_label;     // 이름
            std::wstring m_value;          // 값
            Core::Color m_color;           // 값 색
        };

        // 정보 줄들 그리기 (이름은 흐리게, 값은 굵게)
        void DrawInfoRows(Core::Renderer& renderer, const Core::RectF& area, std::span<const InfoRow> rows)
        {
            float y = area.m_top;                                                                        // 줄 위쪽
            for (const InfoRow& row : rows)                                                              // 줄마다
            {
                renderer.DrawString(row.m_label, Core::RectF{ area.m_left, y, area.m_left + INFO_LABEL_WIDTH, y + INFO_ROW_HEIGHT }, Theme::MUTED_TEXT_COLOR, LABEL_STYLE);
                renderer.DrawString(row.m_value, Core::RectF{ area.m_left + INFO_LABEL_WIDTH, y, area.m_right, y + INFO_ROW_HEIGHT }, row.m_color, VALUE_STYLE);
                y += INFO_ROW_HEIGHT;                                                                    // 다음 줄
            }
        }

        // 카드 본문 영역 (머리말 · 여백 제외)
        [[nodiscard]] Core::RectF GetCardContent(const Core::RectF& card) noexcept
        {
            return Core::RectF{ card.m_left + PageLayout::CARD_PADDING, card.m_top + PageLayout::CARD_HEADER_HEIGHT,
                std::max(card.m_right - PageLayout::CARD_PADDING, card.m_left + PageLayout::CARD_PADDING),
                std::max(card.m_bottom - PageLayout::CARD_PADDING, card.m_top + PageLayout::CARD_HEADER_HEIGHT) };
        }

        // 프레임 기준 사각형 → 미리 보기 위 사각형
        [[nodiscard]] Core::RectF MapRect(const Vision::PixelRect& rect, const Core::RectF& image, float scale) noexcept
        {
            return Core::RectF{ image.m_left + rect.m_x * scale, image.m_top + rect.m_y * scale,
                image.m_left + (rect.m_x + rect.m_width) * scale, image.m_top + (rect.m_y + rect.m_height) * scale };
        }

        // 현재 캐릭터 한 줄 글자 · 색
        [[nodiscard]] std::pair<std::wstring, Core::Color> DescribeCharacter(const CurrentCharacterSummary& character)
        {
            switch (character.m_state)                                                                    // 상태별
            {
            case CurrentCharacterState::CHARACTER_VISION_OFF: return { std::wstring{ VISION_OFF_TEXT }, Theme::MUTED_TEXT_COLOR };
            case CurrentCharacterState::CHARACTER_NO_ROSTER:  return { std::wstring{ NO_ROSTER_TEXT }, Theme::CAUTION_TEXT_COLOR };
            case CurrentCharacterState::CHARACTER_CHECKING:   return { std::wstring{ CHECKING_TEXT }, Theme::CAUTION_TEXT_COLOR };
            case CurrentCharacterState::CHARACTER_KNOWN:      break;                                     // 아래
            }
            return { character.m_name, Theme::HEADER_TEXT_COLOR };                                       // 이름
        }
    }

    OverlaySetupView::OverlaySetupView(Core::Platform&)
        : m_autoBox{ m_host.Emplace<UI::CheckBox>(std::wstring{ AUTO_TEXT }, [this](bool isChecked) { if (m_callbacks.m_onAutoChanged) m_callbacks.m_onAutoChanged(isChecked); }) }
        , m_characterBox{ m_host.Emplace<UI::ComboBox>() }
        , m_pickButton{ m_host.Emplace<UI::Button>(std::wstring{ PICK_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, nullptr) }
        , m_refreshButton{ m_host.Emplace<UI::Button>(std::wstring{ REFRESH_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, [this] { if (m_callbacks.m_onRefreshCharacter) m_callbacks.m_onRefreshCharacter(); }) }
        , m_clearShapesButton{ m_host.Emplace<UI::Button>(std::wstring{}, UI::ButtonStyle::BUTTON_SECONDARY, [this] { if (m_callbacks.m_onClearShapes) m_callbacks.m_onClearShapes(); }) }
        , m_clearAliasesButton{ m_host.Emplace<UI::Button>(std::wstring{}, UI::ButtonStyle::BUTTON_SECONDARY, [this] { if (m_callbacks.m_onClearAliases) m_callbacks.m_onClearAliases(); }) }
        , m_overlayBox{ m_host.Emplace<UI::CheckBox>(std::wstring{ OVERLAY_TEXT }, [this](bool isChecked) { if (m_callbacks.m_onOverlayEnabled) m_callbacks.m_onOverlayEnabled(isChecked); }) }
        , m_lockButton{ m_host.Emplace<UI::Button>(std::wstring{ LOCK_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, [this] { if (m_callbacks.m_onEditing) m_callbacks.m_onEditing(false); }) }
        , m_editButton{ m_host.Emplace<UI::Button>(std::wstring{ EDIT_TEXT }, UI::ButtonStyle::BUTTON_PRIMARY, [this] { if (m_callbacks.m_onEditing) m_callbacks.m_onEditing(true); }) }
        , m_backSlider{ m_host.Emplace<UI::Slider>(BACK_OPACITY_MIN, PERCENT_MAX, PERCENT_STEP,
            [this](double value) { if (m_callbacks.m_onBackOpacity) m_callbacks.m_onBackOpacity(static_cast<float>(value / PERCENT_SCALE), false); },
            [this](double value) { if (m_callbacks.m_onBackOpacity) m_callbacks.m_onBackOpacity(static_cast<float>(value / PERCENT_SCALE), true); }) }
        , m_visionBox{ m_host.Emplace<UI::CheckBox>(std::wstring{ VISION_TEXT }, [this](bool isChecked) { if (m_callbacks.m_onVisionEnabled) m_callbacks.m_onVisionEnabled(isChecked); }) }
        , m_restartButton{ m_host.Emplace<UI::Button>(std::wstring{ RESTART_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY, [this] { if (m_callbacks.m_onRestartVision) m_callbacks.m_onRestartVision(); }) }
        , m_intervalBox{ m_host.Emplace<UI::ComboBox>() }
    {
        for (std::size_t page = 0; page < PAGE_COUNT; ++page)                                            // 쪽 단추
        {
            m_pageButtons[page] = &m_host.Emplace<UI::Button>(std::wstring{ PAGE_TITLES[page] }, UI::ButtonStyle::BUTTON_SECONDARY,
                [this, page] { SelectPage(static_cast<SetupPage>(page)); });                             // 누르면 그 쪽
            m_pageButtons[page]->SetFocusable(false);                                                    // 탭 막대처럼 포커스 없음
        }
        m_pickButton.SetOnClick([this]                                                                   // [이 캐릭터로 지정]
            {
                if (const auto index = m_characterBox.GetSelectedIndex(); index && m_callbacks.m_onPickCharacter) m_callbacks.m_onPickCharacter(*index);
            });
        for (std::size_t index = 0; index < ELEMENT_COUNT; ++index)                                      // 요소 줄마다
        {
            const std::string id{ ELEMENT_SPECS[index].m_id };                                           // 패널 이름
            ElementRow& row = m_elementRows[index];                                                      // 줄
            row.m_visibleBox = &m_host.Emplace<UI::CheckBox>(std::wstring{ VISIBLE_TEXT },
                [this, id](bool isChecked) { if (m_callbacks.m_onElementVisible) m_callbacks.m_onElementVisible(id, isChecked); });   // 표시
            row.m_alphaSlider = &m_host.Emplace<UI::Slider>(ELEMENT_ALPHA_MIN, PERCENT_MAX, PERCENT_STEP,
                [this, id](double value) { if (m_callbacks.m_onElementAlpha) m_callbacks.m_onElementAlpha(id, static_cast<float>(value / PERCENT_SCALE), false); },
                [this, id](double value) { if (m_callbacks.m_onElementAlpha) m_callbacks.m_onElementAlpha(id, static_cast<float>(value / PERCENT_SCALE), true); });   // 투명도
            row.m_resetButton = &m_host.Emplace<UI::Button>(std::wstring{ RESET_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY,
                [this, id] { if (m_callbacks.m_onElementReset) m_callbacks.m_onElementReset(id); });     // 기본 위치로
        }
        std::vector<std::wstring> intervals;                                                             // 간격 목록
        for (int seconds : INTERVAL_SECONDS) intervals.push_back(std::format(INTERVAL_FORMAT, seconds)); // "2초"
        m_intervalBox.SetItems(std::move(intervals));                                                    // 목록
        m_intervalBox.SetOnSelectionChanged([this]                                                       // 간격 바꿈
            {
                if (const auto index = m_intervalBox.GetSelectedIndex(); index && *index < INTERVAL_SECONDS.size() && m_callbacks.m_onIntervalChanged)
                    m_callbacks.m_onIntervalChanged(INTERVAL_SECONDS[*index]);
            });
        SelectPage(SetupPage::PAGE_CHARACTER);                                                           // 첫 쪽
    }

    OverlaySetupView::~OverlaySetupView() = default;   // 부품은 호스트가 해제

    void OverlaySetupView::SetCallbacks(OverlaySetupCallbacks callbacks)
    {
        m_callbacks = std::move(callbacks);   // 연결
    }

    SetupPage OverlaySetupView::GetPage() const noexcept
    {
        return m_page;   // 지금 쪽
    }

    void OverlaySetupView::SelectPage(SetupPage page)
    {
        const bool isPageChanged = m_page != page;                                                       // 바뀌었는지
        m_page = page;                                                                                   // 기록
        const bool isCharacter = page == SetupPage::PAGE_CHARACTER;                                      // 쪽별 보임
        const bool isLayout = page == SetupPage::PAGE_LAYOUT;
        const bool isVision = page == SetupPage::PAGE_VISION;
        for (UI::Control* control : std::initializer_list<UI::Control*>{ &m_autoBox, &m_characterBox, &m_pickButton, &m_refreshButton, &m_clearShapesButton, &m_clearAliasesButton })
            control->SetVisible(isCharacter);                                                            // 현재 캐릭터 쪽
        for (UI::Control* control : std::initializer_list<UI::Control*>{ &m_overlayBox, &m_lockButton, &m_editButton, &m_backSlider })
            control->SetVisible(isLayout);                                                               // UI 구성 쪽
        for (const ElementRow& row : m_elementRows)                                                      // 요소 줄
        {
            row.m_visibleBox->SetVisible(isLayout);
            row.m_alphaSlider->SetVisible(isLayout);
            row.m_resetButton->SetVisible(isLayout);
        }
        for (UI::Control* control : std::initializer_list<UI::Control*>{ &m_visionBox, &m_restartButton, &m_intervalBox })
            control->SetVisible(isVision);                                                               // 인식 설정 쪽
        if (isPageChanged && m_callbacks.m_onPageChanged) m_callbacks.m_onPageChanged(page);             // 알림 (미리 보기 켜기 · 끄기)
    }

    const OverlayElementInfo* OverlaySetupView::FindElement(std::size_t index) const noexcept
    {
        if (index >= ELEMENT_COUNT) return nullptr;                                                      // 경계 검사
        const auto found = std::ranges::find(m_snapshot.m_elements, ELEMENT_SPECS[index].m_id, &OverlayElementInfo::m_id);   // 이름으로
        return found == m_snapshot.m_elements.end() ? nullptr : &*found;                                 // 없으면 nullptr
    }

    void OverlaySetupView::Show(const OverlaySetupSnapshot& snapshot)
    {
        const std::optional<std::size_t> previousCurrent = m_snapshot.m_character.m_currentIndex;        // 지난 현재 캐릭터
        m_snapshot = snapshot;                                                                           // 기록
        const CurrentCharacterSummary& character = m_snapshot.m_character;                               // 현재 캐릭터

        // 현재 캐릭터 쪽
        m_autoBox.SetChecked(character.m_isAutoEnabled);                                                 // 자동 인식
        m_autoBox.SetEnabled(m_snapshot.m_isVisionEnabled);                                              // 화면 인식이 켜져 있을 때만
        const bool isCandidatesChanged = character.m_candidates != m_candidateItems;                     // 후보가 바뀜
        if (isCandidatesChanged && !m_characterBox.IsPopupOpen())                                        // 펼친 중엔 건드리지 않음
        {
            m_candidateItems = character.m_candidates;                                                   // 기록
            m_characterBox.SetItems(m_candidateItems);                                                   // 목록
        }
        if ((isCandidatesChanged || previousCurrent != character.m_currentIndex) && !m_characterBox.IsPopupOpen())
            m_characterBox.SetSelectedIndex(character.m_currentIndex);                                   // 현재 캐릭터 고르기 (사용자가 고르는 중엔 그대로)
        const bool canPick = m_snapshot.m_isVisionEnabled && !m_candidateItems.empty();                  // 고를 수 있음
        m_characterBox.SetEnabled(canPick);
        m_pickButton.SetEnabled(canPick);
        m_refreshButton.SetEnabled(character.m_state == CurrentCharacterState::CHARACTER_KNOWN && !character.m_isSyncing);   // 확인된 캐릭터만
        m_clearShapesButton.SetText(std::format(CLEAR_SHAPES_FORMAT, character.m_shapeCount));          // 개수
        m_clearShapesButton.SetEnabled(character.m_shapeCount > 0);
        m_clearAliasesButton.SetText(std::format(CLEAR_ALIASES_FORMAT, character.m_aliasCount));
        m_clearAliasesButton.SetEnabled(character.m_aliasCount > 0);

        // UI 구성 쪽
        m_overlayBox.SetChecked(m_snapshot.m_isOverlayEnabled);                                          // 오버레이 켜짐
        m_editButton.SetEnabled(m_snapshot.m_isOverlayEnabled && !m_snapshot.m_isEditing);               // 편집 (켜져 있을 때만)
        m_lockButton.SetEnabled(m_snapshot.m_isEditing);                                                 // 잠금 (편집 중에만)
        m_backSlider.SetValue(m_snapshot.m_backOpacity * PERCENT_SCALE);                                 // 바탕 진하기
        for (std::size_t index = 0; index < ELEMENT_COUNT; ++index)                                      // 요소마다
        {
            const OverlayElementInfo* element = FindElement(index);                                      // 정보
            ElementRow& row = m_elementRows[index];                                                      // 줄
            const bool hasElement = element != nullptr;                                                  // 오버레이가 준비됐는지
            row.m_visibleBox->SetEnabled(hasElement);
            row.m_alphaSlider->SetEnabled(hasElement);
            row.m_resetButton->SetEnabled(hasElement);
            if (!hasElement) continue;                                                                   // 없음
            row.m_visibleBox->SetChecked(element->m_isVisible);                                          // 표시
            row.m_alphaSlider->SetValue(element->m_alpha * PERCENT_SCALE);                               // 투명도
        }

        // 인식 설정 쪽
        m_visionBox.SetChecked(m_snapshot.m_isVisionEnabled);                                            // 화면 인식
        m_restartButton.SetEnabled(m_snapshot.m_isVisionEnabled);                                        // 켜져 있을 때만
        m_intervalBox.SetEnabled(m_snapshot.m_isVisionEnabled);
        if (!m_intervalBox.IsPopupOpen())                                                                // 펼친 중엔 그대로
        {
            const auto found = std::ranges::find(INTERVAL_SECONDS, m_snapshot.m_characterIntervalSeconds);   // 지금 간격
            m_intervalBox.SetSelectedIndex(found == INTERVAL_SECONDS.end() ? std::nullopt
                : std::optional<std::size_t>{ static_cast<std::size_t>(found - INTERVAL_SECONDS.begin()) });
        }
    }

    void OverlaySetupView::Update(double deltaSeconds)
    {
        m_host.Update(deltaSeconds);   // 부품 갱신
    }

    void OverlaySetupView::Render(Core::Renderer& renderer, const Core::RectF& contentArea)
    {
        Layout(contentArea);                                                                             // 배치
        for (std::size_t page = 0; page < PAGE_COUNT; ++page)                                            // 지금 쪽 밑줄
        {
            if (static_cast<SetupPage>(page) != m_page) continue;                                        // 다른 쪽
            const Core::RectF& bounds = m_pageButtons[page]->GetBounds();                                // 단추
            renderer.DrawLine(bounds.m_left, bounds.m_bottom + ACTIVE_LINE_WIDTH, bounds.m_right, bounds.m_bottom + ACTIVE_LINE_WIDTH, Theme::ACCENT_COLOR, ACTIVE_LINE_WIDTH);
        }
        switch (m_page)                                                                                  // 쪽별 글자 · 카드
        {
        case SetupPage::PAGE_CHARACTER: RenderCharacter(renderer); break;
        case SetupPage::PAGE_LAYOUT: RenderLayout(renderer); break;
        case SetupPage::PAGE_VISION: RenderVision(renderer); break;
        case SetupPage::PAGE_COUNT: break;
        }
        m_host.Render(renderer);                                                                         // 부품 (카드 위에)
    }

    bool OverlaySetupView::HandleInput(const Core::InputEvent& event)
    {
        return m_host.HandleInput(event);   // 부품 호스트
    }

    Core::CursorShape OverlaySetupView::GetCursor() const noexcept
    {
        return m_host.GetCursor();   // 부품 커서
    }

    void OverlaySetupView::SuspendFocus()
    {
        m_host.SuspendFocus();   // 포커스 놓기
    }

    void OverlaySetupView::RestoreFocus()
    {
        m_host.RestoreFocus();   // 포커스 되돌리기
    }

    void OverlaySetupView::Layout(const Core::RectF& contentArea)
    {
        Core::RectF column = PageLayout::GetPageColumn(Core::RectF{ contentArea.m_left + PAGE_PADDING, contentArea.m_top + PAGE_PADDING,
            std::max(contentArea.m_right - PAGE_PADDING, contentArea.m_left + PAGE_PADDING), std::max(contentArea.m_bottom - PAGE_PADDING, contentArea.m_top + PAGE_PADDING) });   // 본문 열
        m_pageBarBounds = PageLayout::TakeTop(column, PAGE_BAR_HEIGHT);                                  // 쪽 단추 줄
        float x = m_pageBarBounds.m_left;                                                                // 가로 위치
        for (UI::Button* button : m_pageButtons)                                                         // 쪽 단추
        {
            button->SetBounds(Core::RectF{ x, m_pageBarBounds.m_top, x + PAGE_BUTTON_WIDTH, m_pageBarBounds.m_top + Theme::CONTROL_HEIGHT });
            x += PAGE_BUTTON_WIDTH + PAGE_BUTTON_GAP;                                                    // 다음
        }
        switch (m_page)                                                                                  // 쪽별
        {
        case SetupPage::PAGE_CHARACTER: LayoutCharacter(column); break;
        case SetupPage::PAGE_LAYOUT: LayoutLayout(column); break;
        case SetupPage::PAGE_VISION: LayoutVision(column); break;
        case SetupPage::PAGE_COUNT: break;
        }
    }

    void OverlaySetupView::LayoutCharacter(const Core::RectF& area)
    {
        Core::RectF column = area;                                                                       // 남은 영역
        const float firstHeight = PageLayout::CARD_HEADER_HEIGHT + DESCRIPTION_HEIGHT + ROW_GAP + Theme::CONTROL_HEIGHT + ROW_GAP
            + INFO_ROW_HEIGHT * CHARACTER_INFO_ROWS + ROW_GAP + Theme::CONTROL_HEIGHT + PageLayout::CARD_PADDING;   // 첫 카드 높이
        m_firstCard = PageLayout::TakeTop(column, firstHeight);                                         // 첫 카드
        m_firstContent = GetCardContent(m_firstCard);                                                    // 본문
        float y = m_firstContent.m_top;                                                                  // 줄 위치
        m_descriptionBounds = Core::RectF{ m_firstContent.m_left, y, m_firstContent.m_right, y + DESCRIPTION_HEIGHT };   // 설명
        y += DESCRIPTION_HEIGHT + ROW_GAP;
        m_autoBox.SetBounds(Core::RectF{ m_firstContent.m_left, y, m_firstContent.m_left + CHECK_WIDTH, y + Theme::CONTROL_HEIGHT });   // 자동 인식
        y += Theme::CONTROL_HEIGHT + ROW_GAP;
        m_infoBounds = Core::RectF{ m_firstContent.m_left, y, m_firstContent.m_right, y + INFO_ROW_HEIGHT * CHARACTER_INFO_ROWS };       // 정보 줄
        y += INFO_ROW_HEIGHT * CHARACTER_INFO_ROWS + ROW_GAP;
        float x = m_firstContent.m_left;                                                                 // 직접 고르기 줄
        m_pickLabelBounds = Core::RectF{ x, y, x + INFO_LABEL_WIDTH, y + Theme::CONTROL_HEIGHT };        // 이름
        x += INFO_LABEL_WIDTH;
        m_characterBox.SetBounds(Core::RectF{ x, y, x + COMBO_WIDTH, y + Theme::CONTROL_HEIGHT });      // 후보 목록
        x += COMBO_WIDTH + ITEM_GAP;
        m_pickButton.SetBounds(Core::RectF{ x, y, x + BUTTON_WIDTH, y + Theme::CONTROL_HEIGHT });       // 지정
        x += BUTTON_WIDTH + ITEM_GAP;
        m_refreshButton.SetBounds(Core::RectF{ x, y, x + BUTTON_WIDTH, y + Theme::CONTROL_HEIGHT });    // 정보 갱신

        const float secondHeight = PageLayout::CARD_HEADER_HEIGHT + (Theme::CONTROL_HEIGHT + ROW_GAP) * 2.0f + PageLayout::CARD_PADDING;   // 둘째 카드 높이
        m_secondCard = PageLayout::TakeTop(column, secondHeight);                                       // 둘째 카드
        m_secondContent = GetCardContent(m_secondCard);                                                  // 본문
        y = m_secondContent.m_top;                                                                       // 줄 위치
        const float buttonLeft = m_secondContent.m_right - BUTTON_WIDTH;                                 // 오른쪽 단추
        m_shapeTextBounds = Core::RectF{ m_secondContent.m_left, y, buttonLeft - ITEM_GAP, y + Theme::CONTROL_HEIGHT };   // 이름표 모양 설명
        m_clearShapesButton.SetBounds(Core::RectF{ buttonLeft, y, m_secondContent.m_right, y + Theme::CONTROL_HEIGHT });
        y += Theme::CONTROL_HEIGHT + ROW_GAP;
        m_aliasTextBounds = Core::RectF{ m_secondContent.m_left, y, buttonLeft - ITEM_GAP, y + Theme::CONTROL_HEIGHT };   // 읽은 글자 설명
        m_clearAliasesButton.SetBounds(Core::RectF{ buttonLeft, y, m_secondContent.m_right, y + Theme::CONTROL_HEIGHT });
    }

    void OverlaySetupView::LayoutLayout(const Core::RectF& area)
    {
        Core::RectF column = area;                                                                       // 남은 영역
        const float firstHeight = PageLayout::CARD_HEADER_HEIGHT + (Theme::CONTROL_HEIGHT + ROW_GAP) * 3.0f + PageLayout::CARD_PADDING;   // 첫 카드 높이
        m_firstCard = PageLayout::TakeTop(column, firstHeight);                                         // 첫 카드
        m_firstContent = GetCardContent(m_firstCard);                                                    // 본문
        float y = m_firstContent.m_top;                                                                  // 줄 위치
        m_overlayBox.SetBounds(Core::RectF{ m_firstContent.m_left, y, m_firstContent.m_left + CHECK_WIDTH, y + Theme::CONTROL_HEIGHT });   // 오버레이 켜기
        y += Theme::CONTROL_HEIGHT + ROW_GAP;
        float x = m_firstContent.m_left;                                                                 // 잠금 · 편집 줄
        m_editLabelBounds = Core::RectF{ x, y, x + INFO_LABEL_WIDTH, y + Theme::CONTROL_HEIGHT };
        x += INFO_LABEL_WIDTH;
        m_lockButton.SetBounds(Core::RectF{ x, y, x + SMALL_BUTTON_WIDTH, y + Theme::CONTROL_HEIGHT });  // 잠금
        x += SMALL_BUTTON_WIDTH + ITEM_GAP;
        m_editButton.SetBounds(Core::RectF{ x, y, x + SMALL_BUTTON_WIDTH, y + Theme::CONTROL_HEIGHT });  // 편집
        x += SMALL_BUTTON_WIDTH + ITEM_GAP * 2.0f;
        m_editStatusBounds = Core::RectF{ x, y, std::max(x, m_firstContent.m_right), y + Theme::CONTROL_HEIGHT };   // 상태 글자
        y += Theme::CONTROL_HEIGHT + ROW_GAP;
        x = m_firstContent.m_left;                                                                       // 바탕 진하기 줄
        m_backLabelBounds = Core::RectF{ x, y, x + INFO_LABEL_WIDTH, y + Theme::CONTROL_HEIGHT };
        x += INFO_LABEL_WIDTH;
        m_backSlider.SetBounds(Core::RectF{ x, y, x + SLIDER_WIDTH, y + Theme::CONTROL_HEIGHT });       // 막대
        x += SLIDER_WIDTH + ITEM_GAP;
        m_backPercentBounds = Core::RectF{ x, y, x + PERCENT_WIDTH, y + Theme::CONTROL_HEIGHT };        // %

        const float secondHeight = PageLayout::CARD_HEADER_HEIGHT + ELEMENT_ROW_HEIGHT * ELEMENT_COUNT + PageLayout::CARD_PADDING;   // 둘째 카드 높이
        m_secondCard = PageLayout::TakeTop(column, secondHeight);                                       // 둘째 카드
        m_secondContent = GetCardContent(m_secondCard);                                                  // 본문
        y = m_secondContent.m_top;                                                                       // 줄 위치
        for (ElementRow& row : m_elementRows)                                                            // 요소 줄마다
        {
            const float controlTop = y + (ELEMENT_ROW_HEIGHT - Theme::CONTROL_HEIGHT) * HALF;            // 부품 세로 가운데
            float right = m_secondContent.m_right;                                                       // 오른쪽부터 놓음
            row.m_resetButton->SetBounds(Core::RectF{ right - RESET_WIDTH, controlTop, right, controlTop + Theme::CONTROL_HEIGHT });   // 기본 위치로
            right -= RESET_WIDTH + ITEM_GAP;
            row.m_percentBounds = Core::RectF{ right - PERCENT_WIDTH, controlTop, right, controlTop + Theme::CONTROL_HEIGHT };          // %
            right -= PERCENT_WIDTH + ITEM_GAP;
            row.m_alphaSlider->SetBounds(Core::RectF{ right - SLIDER_WIDTH, controlTop, right, controlTop + Theme::CONTROL_HEIGHT });  // 투명도
            right -= SLIDER_WIDTH + ITEM_GAP;
            row.m_visibleBox->SetBounds(Core::RectF{ right - VISIBLE_BOX_WIDTH, controlTop, right, controlTop + Theme::CONTROL_HEIGHT });   // 표시
            right -= VISIBLE_BOX_WIDTH + ITEM_GAP;
            row.m_textBounds = Core::RectF{ m_secondContent.m_left, y, std::max(m_secondContent.m_left, right), y + ELEMENT_ROW_HEIGHT };   // 이름 · 설명
            y += ELEMENT_ROW_HEIGHT;                                                                     // 다음 줄
        }
    }

    void OverlaySetupView::LayoutVision(const Core::RectF& area)
    {
        const Core::RectF cards{ area.m_left, area.m_top, area.m_right, std::max(area.m_bottom, area.m_top + MIN_PREVIEW_CARD_HEIGHT) };   // 카드 영역
        const std::vector<Core::RectF> columns = PageLayout::SplitColumns(cards, VISION_WEIGHTS.size(), PageLayout::CARD_GAP, VISION_WEIGHTS);   // 두 칸
        m_firstCard = columns[0];                                                                        // 미리 보기 카드
        m_firstContent = GetCardContent(m_firstCard);                                                    // 본문
        m_legendBounds = Core::RectF{ m_firstContent.m_left, m_firstContent.m_bottom - LEGEND_HEIGHT, m_firstContent.m_right, m_firstContent.m_bottom };   // 색 설명
        m_previewBounds = Core::RectF{ m_firstContent.m_left, m_firstContent.m_top, m_firstContent.m_right, std::max(m_firstContent.m_top, m_legendBounds.m_top - ITEM_GAP) };   // 그림

        m_secondCard = columns[1];                                                                       // 정보 카드
        m_secondContent = GetCardContent(m_secondCard);                                                  // 본문
        float y = m_secondContent.m_top;                                                                 // 줄 위치
        m_visionBox.SetBounds(Core::RectF{ m_secondContent.m_left, y, m_secondContent.m_right, y + Theme::CONTROL_HEIGHT });   // 화면 인식 켜기
        y += Theme::CONTROL_HEIGHT + ROW_GAP;
        m_infoBounds = Core::RectF{ m_secondContent.m_left, y, m_secondContent.m_right, y + INFO_ROW_HEIGHT * VISION_INFO_ROWS };   // 정보 줄
        y += INFO_ROW_HEIGHT * VISION_INFO_ROWS + ROW_GAP;
        m_restartButton.SetBounds(Core::RectF{ m_secondContent.m_left, y, m_secondContent.m_left + BUTTON_WIDTH, y + Theme::CONTROL_HEIGHT });   // 다시 찾기
        y += Theme::CONTROL_HEIGHT + ROW_GAP;
        m_intervalLabelBounds = Core::RectF{ m_secondContent.m_left, y, m_secondContent.m_left + INFO_LABEL_WIDTH, y + Theme::CONTROL_HEIGHT };   // 간격 이름
        m_intervalBox.SetBounds(Core::RectF{ m_intervalLabelBounds.m_right, y, m_intervalLabelBounds.m_right + SMALL_BUTTON_WIDTH, y + Theme::CONTROL_HEIGHT });   // 간격
    }

    void OverlaySetupView::RenderCharacter(Core::Renderer& renderer) const
    {
        PageLayout::DrawCardWithHeader(renderer, m_firstCard, CHARACTER_EYEBROW, CHARACTER_TITLE, CHARACTER_HINT);   // 첫 카드
        renderer.DrawString(CHARACTER_DESCRIPTION, m_descriptionBounds, Theme::MUTED_TEXT_COLOR, WRAP_STYLE);       // 설명

        const CurrentCharacterSummary& character = m_snapshot.m_character;                               // 현재 캐릭터
        const auto [currentText, currentColor] = DescribeCharacter(character);                           // 현재 캐릭터 글자
        const bool isKnown = character.m_state == CurrentCharacterState::CHARACTER_KNOWN;                // 확인됨
        const std::wstring sourceText{ !isKnown ? NONE_TEXT : (character.m_source == CurrentCharacterSource::SOURCE_MANUAL ? SOURCE_MANUAL_TEXT
            : (character.m_source == CurrentCharacterSource::SOURCE_INFO_WINDOW ? SOURCE_INFO_TEXT : SOURCE_PLATE_TEXT)) };   // 알아본 방법
        const std::wstring storedText = !isKnown || character.m_itemLevel <= 0.0 ? std::wstring{ NONE_TEXT }
            : (character.m_className.empty() ? std::format(STORED_LEVEL_FORMAT, character.m_itemLevel) : std::format(STORED_FORMAT, character.m_className, character.m_itemLevel));   // 저장된 정보
        std::wstring stateText{ NONE_TEXT };                                                             // 상태
        Core::Color stateColor = Theme::TEXT_COLOR;                                                      // 상태 색
        if (isKnown && character.m_isSyncing) stateText = SYNCING_TEXT;                                  // 갱신 중
        else if (isKnown && character.m_isOutdated && character.m_readItemLevel)                         // 옛날 정보
        {
            stateText = std::format(OUTDATED_FORMAT, character.m_itemLevel, *character.m_readItemLevel);
            stateColor = Theme::CAUTION_TEXT_COLOR;
        }
        else if (isKnown) stateText = SAME_TEXT;                                                         // 그대로
        const std::array rows{
            InfoRow{ ROW_CURRENT, currentText, currentColor },
            InfoRow{ ROW_SOURCE, sourceText, Theme::TEXT_COLOR },
            InfoRow{ ROW_STORED, storedText, Theme::TEXT_COLOR },
            InfoRow{ ROW_STATE, stateText, stateColor },
            InfoRow{ ROW_ROSTER, std::format(ROSTER_FORMAT, m_snapshot.m_rosterCount), Theme::TEXT_COLOR },
            InfoRow{ ROW_READING, std::wstring{ READING_TEXT }, Theme::MUTED_TEXT_COLOR } };              // 정보 줄
        DrawInfoRows(renderer, m_infoBounds, rows);                                                      // 그리기
        renderer.DrawString(PICK_LABEL_TEXT, m_pickLabelBounds, Theme::MUTED_TEXT_COLOR, LABEL_STYLE);   // 직접 고르기

        PageLayout::DrawCardWithHeader(renderer, m_secondCard, MEMORY_EYEBROW, MEMORY_TITLE, MEMORY_HINT);   // 둘째 카드
        renderer.DrawString(std::format(SHAPE_TEXT_FORMAT, character.m_shapeCount), m_shapeTextBounds, Theme::TEXT_COLOR, LABEL_STYLE);    // 이름표 모양
        renderer.DrawString(std::format(ALIAS_TEXT_FORMAT, character.m_aliasCount), m_aliasTextBounds, Theme::TEXT_COLOR, LABEL_STYLE);    // 읽은 글자
    }

    void OverlaySetupView::RenderLayout(Core::Renderer& renderer) const
    {
        PageLayout::DrawCardWithHeader(renderer, m_firstCard, LAYOUT_EYEBROW, LAYOUT_TITLE, LAYOUT_HINT);   // 첫 카드
        renderer.DrawString(EDIT_LABEL_TEXT, m_editLabelBounds, Theme::MUTED_TEXT_COLOR, LABEL_STYLE);  // 끌어 옮기기
        std::wstring_view status = LOCKED_STATUS_TEXT;                                                   // 상태 글자
        Core::Color statusColor = Theme::MUTED_TEXT_COLOR;                                               // 색
        if (!m_snapshot.m_isOverlayEnabled) { status = OVERLAY_OFF_STATUS_TEXT; statusColor = Theme::CAUTION_TEXT_COLOR; }   // 꺼짐
        else if (m_snapshot.m_isEditing) { status = EDITING_STATUS_TEXT; statusColor = Theme::ACCENT_COLOR; }                // 편집 중
        else if (!m_snapshot.m_isGameFound) { status = NO_GAME_STATUS_TEXT; statusColor = Theme::CAUTION_TEXT_COLOR; }       // 게임 없음
        renderer.DrawString(status, m_editStatusBounds, statusColor, SMALL_STYLE);                       // 상태
        renderer.DrawString(BACK_LABEL_TEXT, m_backLabelBounds, Theme::MUTED_TEXT_COLOR, LABEL_STYLE);  // 바탕 진하기
        renderer.DrawString(std::format(PERCENT_FORMAT, std::lround(m_snapshot.m_backOpacity * PERCENT_SCALE)), m_backPercentBounds, Theme::TEXT_COLOR, PERCENT_STYLE);   // %

        PageLayout::DrawCardWithHeader(renderer, m_secondCard, ELEMENTS_EYEBROW, ELEMENTS_TITLE, ELEMENTS_HINT);   // 둘째 카드
        for (std::size_t index = 0; index < ELEMENT_COUNT; ++index)                                      // 요소마다
        {
            const ElementRow& row = m_elementRows[index];                                                // 줄
            const float titleTop = row.m_textBounds.m_top + (ELEMENT_ROW_HEIGHT - ELEMENT_TITLE_HEIGHT * 2.0f) * HALF;   // 두 줄 세로 가운데
            renderer.DrawString(ELEMENT_SPECS[index].m_title, Core::RectF{ row.m_textBounds.m_left, titleTop, row.m_textBounds.m_right, titleTop + ELEMENT_TITLE_HEIGHT },
                Theme::HEADER_TEXT_COLOR, TITLE_STYLE);                                                  // 이름
            renderer.DrawString(ELEMENT_SPECS[index].m_description, Core::RectF{ row.m_textBounds.m_left, titleTop + ELEMENT_TITLE_HEIGHT, row.m_textBounds.m_right,
                titleTop + ELEMENT_TITLE_HEIGHT * 2.0f }, Theme::MUTED_TEXT_COLOR, SMALL_STYLE);           // 설명
            const OverlayElementInfo* element = FindElement(index);                                      // 정보
            const std::wstring percent = element == nullptr ? std::wstring{ NONE_TEXT } : std::format(PERCENT_FORMAT, std::lround(element->m_alpha * PERCENT_SCALE));   // %
            renderer.DrawString(percent, row.m_percentBounds, Theme::TEXT_COLOR, PERCENT_STYLE);          // %
            if (index + 1 < ELEMENT_COUNT)                                                               // 줄 구분선
                renderer.DrawLine(row.m_textBounds.m_left, row.m_textBounds.m_bottom, m_secondContent.m_right, row.m_textBounds.m_bottom, Theme::GRID_LINE_COLOR, PREVIEW_BORDER);
        }
    }

    void OverlaySetupView::RenderPreview(Core::Renderer& renderer, const Core::RectF& area) const
    {
        const std::optional<VisionStatusSnapshot>& vision = m_snapshot.m_vision;                         // 화면 인식 상태
        const std::shared_ptr<const Core::ImageData> preview = vision ? vision->m_diagnostics.m_preview : nullptr;   // 그림
        if (!preview || preview->m_width <= 0 || preview->m_height <= 0 || vision->m_diagnostics.m_frameWidth <= 0)  // 그림 없음
        {
            PageLayout::DrawEmptyState(renderer, area, PREVIEW_EMPTY_TITLE, PREVIEW_EMPTY_HINT);         // 빈 화면 안내
            return;
        }
        const float scale = std::min(area.GetWidth() / preview->m_width, area.GetHeight() / preview->m_height);   // 그림 배율 (비율 유지)
        const float width = preview->m_width * scale;                                                    // 그릴 가로
        const float height = preview->m_height * scale;                                                  // 그릴 세로
        const Core::RectF image{ area.m_left + (area.GetWidth() - width) * HALF, area.m_top, area.m_left + (area.GetWidth() + width) * HALF, area.m_top + height };   // 가운데
        renderer.DrawImage(*preview, image, FULL_OPACITY);                                               // 게임 화면
        renderer.DrawRect(image, Theme::GRID_LINE_COLOR, PREVIEW_BORDER);                                // 테두리
        const float frameScale = width / static_cast<float>(vision->m_diagnostics.m_frameWidth);         // 프레임 → 그림 배율
        renderer.DrawRect(MapRect(vision->m_topBar, image, frameScale), Theme::DOWN_TEXT_COLOR, BOX_STROKE);   // 재화 줄 (파랑)
        if (vision->m_diagnostics.m_hoverArea) renderer.DrawRect(MapRect(*vision->m_diagnostics.m_hoverArea, image, frameScale), Theme::HIGHLIGHT_TEXT_COLOR, BOX_STROKE);   // 마우스 주변 (노랑)
        if (vision->m_diagnostics.m_nameplate) renderer.DrawRect(MapRect(*vision->m_diagnostics.m_nameplate, image, frameScale), Theme::NOTICE_SUCCESS_COLOR, BOX_STROKE);   // 내 이름표 (초록)
    }

    void OverlaySetupView::RenderVision(Core::Renderer& renderer) const
    {
        PageLayout::DrawCardWithHeader(renderer, m_firstCard, PREVIEW_EYEBROW, PREVIEW_TITLE, PREVIEW_HINT);   // 미리 보기 카드
        RenderPreview(renderer, m_previewBounds);                                                        // 그림
        renderer.DrawString(LEGEND_TEXT, m_legendBounds, Theme::MUTED_TEXT_COLOR, SMALL_STYLE);          // 색 설명

        PageLayout::DrawCardWithHeader(renderer, m_secondCard, STATUS_EYEBROW, STATUS_TITLE);            // 정보 카드
        const std::optional<VisionStatusSnapshot>& vision = m_snapshot.m_vision;                         // 상태
        std::wstring gameText{ VISION_OFF_TEXT };                                                        // 게임 창
        Core::Color gameColor = Theme::MUTED_TEXT_COLOR;                                                 // 색
        std::wstring plateText{ NONE_TEXT };                                                             // 내 이름표
        std::wstring lastText{ NONE_TEXT };                                                              // 마지막 값
        std::wstring speedText{ NONE_TEXT };                                                             // 처리 속도
        if (vision)                                                                                      // 켜져 있음
        {
            const Vision::WatcherDiagnostics& diagnostics = vision->m_diagnostics;                       // 진단
            const bool isWatching = vision->m_status.m_state == Vision::WatcherState::WATCHER_WATCHING;  // 감시 중
            gameText = isWatching ? std::format(GAME_WATCHING_FORMAT, diagnostics.m_frameWidth, diagnostics.m_frameHeight)
                : Core::TextEncoding::UTF8ToWide(vision->m_status.m_message);                            // 상태 글자
            gameColor = isWatching ? Theme::NOTICE_SUCCESS_COLOR : Theme::CAUTION_TEXT_COLOR;            // 색
            plateText = diagnostics.m_nameplate ? std::format(PLATE_FOUND_FORMAT, diagnostics.m_nameplate->m_width, diagnostics.m_nameplate->m_height,
                diagnostics.m_nameplate->m_x, diagnostics.m_nameplate->m_y) : std::wstring{ PLATE_MISSING_TEXT };   // 이름표
            if (!diagnostics.m_lastRead.empty())                                                         // 마지막 값
            {
                const std::int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(diagnostics.m_lastReadAt.time_since_epoch()).count();   // 유닉스 초
                lastText = std::format(LAST_FORMAT, Core::TextEncoding::UTF8ToWide(diagnostics.m_lastRead), Core::LocalTime::FormatClock(seconds));
            }
            speedText = std::format(SPEED_FORMAT, diagnostics.m_hoverScanMs, diagnostics.m_characterScanMs);   // 평균 시간
        }
        const auto [currentText, currentColor] = DescribeCharacter(m_snapshot.m_character);              // 현재 캐릭터
        const std::array rows{
            InfoRow{ ROW_GAME, gameText, gameColor },
            InfoRow{ ROW_CURRENT, currentText, currentColor },
            InfoRow{ ROW_PLATE, plateText, Theme::TEXT_COLOR },
            InfoRow{ ROW_LAST, lastText, Theme::TEXT_COLOR },
            InfoRow{ ROW_SPEED, speedText, Theme::TEXT_COLOR },
            InfoRow{ ROW_METHOD, std::format(METHOD_FORMAT, m_snapshot.m_characterIntervalSeconds), Theme::MUTED_TEXT_COLOR } };   // 정보 줄
        DrawInfoRows(renderer, m_infoBounds, rows);                                                      // 그리기
        renderer.DrawString(INTERVAL_LABEL_TEXT, m_intervalLabelBounds, Theme::MUTED_TEXT_COLOR, LABEL_STYLE);   // 간격 이름
    }
}
