export module LostArkMultiTool.Views.Theme;

import std;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;

// 다크 테마 색 · 글꼴 · 크기 (원본 GridTheme와 같은 색 체계 → 탭 · 표 · 그래프 · 대화상자가 한 가지 톤으로 보이게)
// 상수와 constexpr 함수만 있어서 .cpp가 필요 없음
export namespace LostArkMultiTool::Views::Theme
{
    inline constexpr float COLOR_CHANNEL_MAX{ 255.0f };   // 0~255 정수 채널의 최댓값
    inline constexpr float FULL_ALPHA{ 1.0f };            // 완전 불투명

    // 0~255 정수 RGB로 색 만들기 (아래 색 상수를 컴파일 시간에 계산)
    // 테마 색을 만드는 곳이 여기뿐이라 같은 모듈에 둠 → 다른 모듈의 constexpr 함수를 상수 초기화에 쓰면
    // 편집기(IntelliSense)가 함수 본문을 못 읽어 E0028(상수 값이 있어야 함)을 띄우는 문제도 피함
    [[nodiscard]] constexpr Core::Color MakeColor(int red, int green, int blue, float alpha = FULL_ALPHA) noexcept
    {
        return Core::Color{ red / COLOR_CHANNEL_MAX, green / COLOR_CHANNEL_MAX, blue / COLOR_CHANNEL_MAX, alpha };
    }

    // ===== 기본 색 (v1.5.0 새 디자인: 남색에 가까운 검정 배경 + 청록 강조) =====
    inline constexpr Core::Color BACKGROUND_COLOR{ MakeColor(11, 15, 21) };        // 빈 공간 (그래프 배경과 동일, 거의 검정인 남색)
    inline constexpr Core::Color CELL_BACK_COLOR{ MakeColor(18, 24, 32) };         // 일반 칸 (카드 바탕과 같은 톤)
    inline constexpr Core::Color HEADER_BACK_COLOR{ MakeColor(24, 31, 41) };       // 표 머리글 · 구분 열
    inline constexpr Core::Color GRID_LINE_COLOR{ MakeColor(36, 46, 58) };         // 경계선 (카드 테두리보다 한 단계 밝게)
    inline constexpr Core::Color TEXT_COLOR{ MakeColor(222, 228, 236) };           // 일반 글자 (어두운 바탕에서 대비 확보)
    inline constexpr Core::Color HEADER_TEXT_COLOR{ MakeColor(255, 255, 255) };    // 머리글 · 선택된 메뉴 글자
    inline constexpr Core::Color SELECTION_BACK_COLOR{ MakeColor(22, 72, 74) };    // 선택 · 마우스를 올린 항목 (청록 톤)
    inline constexpr Core::Color SELECTION_TEXT_COLOR{ MakeColor(255, 255, 255) };  // 선택된 칸 글자 (흰색)
    inline constexpr Core::Color MUTED_TEXT_COLOR{ MakeColor(136, 148, 162) };     // 보조 글자 · 선택 안 된 메뉴 (회색)
    inline constexpr Core::Color TOTAL_TEXT_COLOR{ MakeColor(94, 234, 212) };      // 강조 (합계 · 포커스 · 청록 #5EEAD4)

    // ===== 용도별 색 (원본 GridTheme) =====
    inline constexpr Core::Color READONLY_BACK_COLOR{ MakeColor(15, 20, 27) };     // 읽기 전용 칸 (카드보다 살짝 어둡게)
    inline constexpr Core::Color TOTAL_BACK_COLOR{ MakeColor(16, 46, 48) };        // 합계 열 (어두운 청록)
    inline constexpr Core::Color HIGHLIGHT_BACK_COLOR{ MakeColor(46, 36, 18) };    // 강조 행 (★ 캐릭터 총합, 어두운 금색)
    inline constexpr Core::Color HIGHLIGHT_TEXT_COLOR{ MakeColor(255, 196, 77) };  // 골드 금액 (금색)
    inline constexpr Core::Color UP_TEXT_COLOR{ MakeColor(255, 112, 112) };        // 상승 ▲ · 오류
    inline constexpr Core::Color DOWN_TEXT_COLOR{ MakeColor(110, 168, 255) };      // 하락 ▼
    inline constexpr Core::Color ERROR_BACK_COLOR{ MakeColor(92, 32, 38) };        // 잘못된 입력 칸
    inline constexpr Core::Color LABEL_BACK_COLOR{ HEADER_BACK_COLOR };                  // 표의 구분 열

    // ===== 화면 부품 (버튼 · 입력칸) =====
    inline constexpr Core::Color BUTTON_BACK_COLOR{ MakeColor(30, 39, 50) };       // 보조 버튼 바탕
    inline constexpr Core::Color BUTTON_HOVER_COLOR{ MakeColor(40, 52, 66) };      // 보조 버튼 마우스 올림
    inline constexpr Core::Color BUTTON_PRESSED_COLOR{ MakeColor(24, 31, 40) };    // 보조 버튼 누름
    inline constexpr Core::Color PRIMARY_BACK_COLOR{ MakeColor(79, 209, 197) };    // 주요 버튼 (저장 · 확인 · 검색, 청록 #4FD1C5)
    inline constexpr Core::Color PRIMARY_HOVER_COLOR{ MakeColor(110, 231, 214) };  // 주요 버튼 마우스 올림 (더 밝은 청록)
    inline constexpr Core::Color PRIMARY_PRESSED_COLOR{ MakeColor(56, 170, 160) }; // 주요 버튼 누름 (어두운 청록)
    inline constexpr Core::Color PRIMARY_TEXT_COLOR{ MakeColor(8, 24, 26) };       // 주요 버튼 글자 (청록 바탕 위 어두운 글자)
    inline constexpr Core::Color DISABLED_BACK_COLOR{ MakeColor(26, 32, 41) };     // 꺼진 버튼 · 입력칸 바탕
    inline constexpr Core::Color INPUT_BACK_COLOR{ MakeColor(12, 17, 23) };        // 입력칸 바탕 (카드 안에서 한 단계 어둡게)
    inline constexpr Core::Color FOCUS_BORDER_COLOR{ TOTAL_TEXT_COLOR };           // 포커스 테두리 · 링크 글자
    inline constexpr Core::Color CARET_COLOR{ MakeColor(235, 235, 235) };
    inline constexpr Core::Color COMPOSITION_LINE_COLOR{ TOTAL_TEXT_COLOR };       // 한글 조합 중인 글자 밑줄

    // ===== 그래프 (원본 차트: 다크 배경 + 골드 곡선 + 아래 영역 그라데이션) =====
    inline constexpr float CHART_AREA_ALPHA{ 70.0f / COLOR_CHANNEL_MAX };                // 곡선 아래 영역 투명도 (원본 70/255)
    inline constexpr float TRANSPARENT_ALPHA{ 0.0f };
    inline constexpr Core::Color CHART_LINE_COLOR{ MakeColor(218, 165, 32) };      // 곡선 · 점 테두리 (Goldenrod = 자산)
    inline constexpr Core::Color CHART_MARKER_COLOR{ MakeColor(255, 255, 255) };   // 점 안쪽
    inline constexpr Core::Color CHART_AREA_TOP_COLOR{ MakeColor(218, 165, 32, CHART_AREA_ALPHA) };
    inline constexpr Core::Color CHART_AREA_BOTTOM_COLOR{ MakeColor(218, 165, 32, TRANSPARENT_ALPHA) };
    inline constexpr Core::Color GEM_CHART_LINE_COLOR{ MakeColor(147, 112, 219) };  // 보석 그래프 곡선 (MediumPurple = 보석)
    inline constexpr Core::Color PARADISE_CHART_LINE_COLOR{ MakeColor(255, 69, 0) }; // 낙원력 그래프 곡선 (원본 OrangeRed)

    // ===== 보석 검색기 (원본 GridTheme 용도별 색) =====
    inline constexpr Core::Color REACHED_BACK_COLOR{ MakeColor(26, 60, 38) };      // 목표가 도달 행
    inline constexpr Core::Color REACHED_TEXT_COLOR{ MakeColor(140, 232, 160) };
    inline constexpr Core::Color ALERT_TEXT_COLOR{ UP_TEXT_COLOR };                // 일시정지 · 한도 보호 안내
    inline constexpr Core::Color CAUTION_TEXT_COLOR{ MakeColor(255, 180, 84) };    // 호출량 경고 (DarkOrange 대체)

    // 악세 연마 효과 수치 단계 (게임 표기 색)
    inline constexpr Core::Color POLISH_HIGH_COLOR{ MakeColor(255, 210, 40) };     // 상 = 노랑
    inline constexpr Core::Color POLISH_MIDDLE_COLOR{ MakeColor(190, 110, 255) };  // 중 = 보라
    inline constexpr Core::Color POLISH_LOW_COLOR{ MakeColor(70, 170, 255) };      // 하 = 파랑

    // ===== 대화상자 · 알림 =====
    inline constexpr Core::Color OVERLAY_DIM_COLOR{ MakeColor(0, 0, 0, 0.55f) };   // 대화상자 뒤 화면을 어둡게
    inline constexpr Core::Color PANEL_BACK_COLOR{ MakeColor(20, 27, 36) };        // 대화상자 · 알림 · 펼침 목록 바탕
    inline constexpr Core::Color PANEL_BORDER_COLOR{ MakeColor(44, 56, 70) };      // 대화상자 · 알림 테두리
    inline constexpr Core::Color NOTICE_INFO_COLOR{ TOTAL_TEXT_COLOR };
    inline constexpr Core::Color NOTICE_SUCCESS_COLOR{ MakeColor(112, 208, 128) };
    inline constexpr Core::Color NOTICE_WARNING_COLOR{ HIGHLIGHT_TEXT_COLOR };
    inline constexpr Core::Color NOTICE_ERROR_COLOR{ UP_TEXT_COLOR };

    // ===== 카드 · 화면 틀 (v1.5.0 왼쪽 메뉴 · 위 막대 · 페이지 머리말) =====
    inline constexpr Core::Color CARD_BACK_COLOR{ MakeColor(17, 23, 31) };         // 묶음 상자(카드) 바탕
    inline constexpr Core::Color CARD_BORDER_COLOR{ MakeColor(32, 42, 54) };       // 카드 얇은 테두리
    inline constexpr Core::Color SIDEBAR_BACK_COLOR{ MakeColor(13, 18, 25) };      // 왼쪽 메뉴 · 위 막대 바탕
    inline constexpr Core::Color ACCENT_COLOR{ TOTAL_TEXT_COLOR };                 // 청록 강조 (로고 · 선택 막대 · 머리말 영문)
    inline constexpr Core::Color ACCENT_TEXT_COLOR{ PRIMARY_TEXT_COLOR };          // 청록 바탕 위 글자 (로고 아이콘)
    inline constexpr float NAV_ACTIVE_ALPHA{ 0.12f };                              // 선택한 메뉴의 청록 바탕 투명도
    inline constexpr float NAV_HOVER_ALPHA{ 0.06f };                               // 마우스를 올린 메뉴의 청록 바탕 투명도

    // 알림 종류별 강조색 (대화상자 제목 막대 · 토스트 왼쪽 막대가 같은 색을 쓰도록 한 곳에서 결정)
    [[nodiscard]] constexpr const Core::Color& GetNoticeColor(Core::NoticeKind kind) noexcept
    {
        switch (kind)
        {
            case Core::NoticeKind::NOTICE_SUCCESS: return NOTICE_SUCCESS_COLOR;
            case Core::NoticeKind::NOTICE_WARNING: return NOTICE_WARNING_COLOR;
            case Core::NoticeKind::NOTICE_ERROR:   return NOTICE_ERROR_COLOR;
            default:                               return NOTICE_INFO_COLOR;
        }
    }

    // ===== 아이템 등급 글자색 (게임 등급 색 → 강화책 · 젬 · 유각 이름, 보석 레벨) =====
    inline constexpr Core::Color GRADE_NORMAL_COLOR{ MakeColor(220, 220, 220) };    // 일반
    inline constexpr Core::Color GRADE_UNCOMMON_COLOR{ MakeColor(141, 198, 63) };   // 고급
    inline constexpr Core::Color GRADE_RARE_COLOR{ MakeColor(0, 176, 250) };        // 희귀
    inline constexpr Core::Color GRADE_EPIC_COLOR{ MakeColor(206, 67, 252) };       // 영웅
    inline constexpr Core::Color GRADE_LEGENDARY_COLOR{ MakeColor(249, 146, 0) };   // 전설
    inline constexpr Core::Color GRADE_RELIC_COLOR{ MakeColor(250, 93, 0) };        // 유물
    inline constexpr Core::Color GRADE_ANCIENT_COLOR{ MakeColor(220, 201, 153) };   // 고대
    inline constexpr Core::Color GRADE_ESTHER_COLOR{ MakeColor(60, 245, 226) };     // 에스더

    // 등급 이름(UTF-8, API 응답 그대로) → 글자색 (모르는 등급이면 기본 글자색)
    [[nodiscard]] constexpr const Core::Color& GetGradeColor(std::string_view gradeName) noexcept
    {
        if (gradeName == "일반") return GRADE_NORMAL_COLOR;
        if (gradeName == "고급") return GRADE_UNCOMMON_COLOR;
        if (gradeName == "희귀") return GRADE_RARE_COLOR;
        if (gradeName == "영웅") return GRADE_EPIC_COLOR;
        if (gradeName == "전설") return GRADE_LEGENDARY_COLOR;
        if (gradeName == "유물") return GRADE_RELIC_COLOR;
        if (gradeName == "고대") return GRADE_ANCIENT_COLOR;
        if (gradeName == "에스더") return GRADE_ESTHER_COLOR;
        return TEXT_COLOR;
    }

    // ===== 글꼴 =====
    inline constexpr const wchar_t* FONT_FAMILY{ L"Malgun Gothic" };   // 맑은 고딕 (원본 표 글꼴)
    inline constexpr const wchar_t* FONT_LOCALE{ L"ko-KR" };
    inline constexpr float FONT_SIZE_SMALL{ 11.0f };
    inline constexpr float FONT_SIZE_NORMAL{ 12.0f };   // 원본 9pt ≈ 12 DIP
    inline constexpr float FONT_SIZE_TAB{ 14.0f };
    inline constexpr float FONT_SIZE_DIALOG_TITLE{ 15.0f };
    inline constexpr float FONT_SIZE_CHART_TITLE{ 16.0f };   // 원본 12pt
    inline constexpr float FONT_SIZE_TITLE{ 22.0f };
    inline constexpr float FONT_SIZE_EYEBROW{ 10.0f };   // 페이지 머리말 · 카드 위 작은 영문 (청록 대문자)
    inline constexpr float FONT_SIZE_PAGE_TITLE{ 24.0f }; // 페이지 머리말 큰 제목

    // ===== 크기 (DIP) =====
    inline constexpr float CORNER_RADIUS{ 4.0f };        // 버튼 · 입력칸 모서리
    inline constexpr float PANEL_CORNER_RADIUS{ 8.0f };  // 대화상자 · 알림 모서리
    inline constexpr float BORDER_WIDTH{ 1.0f };
    inline constexpr float FOCUS_BORDER_WIDTH{ 1.5f };
    inline constexpr float CONTROL_HEIGHT{ 32.0f };      // 버튼 · 입력칸 기본 높이
    inline constexpr float GRID_HEADER_HEIGHT{ 34.0f };  // 표 머리글 행
    inline constexpr float GRID_ROW_HEIGHT{ 30.0f };     // 표 한 행 (원본 26px → 편집 칸이 들어가도록 여유 있게)
    inline constexpr float PANEL_TITLE_HEIGHT{ 28.0f };  // 묶음 상자(원본 GroupBox) 제목 줄
}