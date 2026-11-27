module LostArkMultiTool.Views.Application;

import std;
import LostArkMultiTool.Core.APIKeyStore;
import LostArkMultiTool.Core.APIFormatMonitor;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.AppLogger;
import LostArkMultiTool.Core.BackgroundTask;
import LostArkMultiTool.Core.Input;
import LostArkMultiTool.Core.IntervalTimer;
import LostArkMultiTool.Core.MarketAPI;
import LostArkMultiTool.Core.Notifier;
import LostArkMultiTool.Core.Renderer;
import LostArkMultiTool.Core.ServerConfigChecker;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Core.ToolContext;
import LostArkMultiTool.Core.ToolManager;
import LostArkMultiTool.Models.ServerConfig;
import LostArkMultiTool.Services.DPAPIKeyStore;
import LostArkMultiTool.Services.GistServerConfigChecker;
import LostArkMultiTool.Services.UpdateInstaller;
import LostArkMultiTool.Services.WinHTTPMarketAPI;
import LostArkMultiTool.Views.AccessoryFinderManager;
import LostArkMultiTool.Views.APIKeyDialog;
import LostArkMultiTool.Views.ArkGemFinderManager;
import LostArkMultiTool.Views.CalculatorManager;
import LostArkMultiTool.Views.CombatSimulatorManager;
import LostArkMultiTool.Views.DX11Renderer;
import LostArkMultiTool.Views.EngravingFinderManager;
import LostArkMultiTool.Views.GemFinderManager;
import LostArkMultiTool.Views.MainWindow;
import LostArkMultiTool.Views.PageLayout;
import LostArkMultiTool.Views.RefineBookFinderManager;
import LostArkMultiTool.Views.RefineSimulatorManager;
import LostArkMultiTool.Views.ServerCheckDialog;
import LostArkMultiTool.Views.TransferManager;
import LostArkMultiTool.Views.OverlaySetupManager;   // 오버레이 · 인식 설정 탭
import LostArkMultiTool.Views.Theme;
import LostArkMultiTool.Views.TrackerManager;
import LostArkMultiTool.Views.UI.Button;
import LostArkMultiTool.Views.UI.ControlHost;
import LostArkMultiTool.Views.UI.Dialog;
import LostArkMultiTool.Views.UI.OverlayNotifier;
import LostArkMultiTool.Views.UI.TextBox;
import LostArkMultiTool.Views.OverlayManager;   // 인게임 오버레이

namespace LostArkMultiTool::Views
{
    namespace
    {
        constexpr std::wstring_view WINDOW_TITLE{ L"로스트아크 멀티툴" };

        // 원격 점검 상태를 따를지 (작업 중에는 점검을 켜 두므로 Debug 빌드는 무시하고 바로 테스트, Release 배포판은 항상 따름)
#ifdef _DEBUG  // Debug 빌드에서만 적용
        constexpr bool IS_MAINTENANCE_LOCK_ENABLED{ false };  // Debug 빌드는 원격 점검 상태를 따르지 않음 (바로 테스트)
#else  // Release 빌드에서는
        constexpr bool IS_MAINTENANCE_LOCK_ENABLED{ true };  // 원격 점검 상태를 항상 따름
#endif  // 빌드 구성 분기 끝
        constexpr int WINDOW_WIDTH{ 1920 };    // 처음 창 안쪽 크기 (DIP)
        constexpr int WINDOW_HEIGHT{ 1080 };

        // ===== 왼쪽 메뉴 (DIP) =====
        constexpr float SIDEBAR_WIDTH{ 180.0f };         // 왼쪽 메뉴 너비 (예전 위쪽 탭 막대를 대신함)
        constexpr float SIDEBAR_PADDING{ 10.0f };        // 메뉴 항목 좌우 여백
        constexpr float LOGO_ICON_SIZE{ 28.0f };         // 로고 청록 정사각형 아이콘 크기
        constexpr float LOGO_ICON_LEFT{ 16.0f };         // 로고 아이콘 왼쪽 위치
        constexpr float LOGO_TEXT_GAP{ 10.0f };          // 로고 아이콘과 글자 사이
        constexpr float LOGO_TITLE_HEIGHT{ 16.0f };      // 로고 "LOA KIT" 줄 높이
        constexpr float LOGO_SUBTITLE_HEIGHT{ 14.0f };   // 로고 "로스트아크 멀티툴" 줄 높이
        constexpr float LOGO_ICON_RADIUS{ 6.0f };        // 로고 아이콘 모서리
        constexpr int LOGO_ICON_PIXELS{ 64 };            // 로고 아이콘 그림 크기 (픽셀, 큰 배율에서도 또렷하게 줄여 그림)
        constexpr float LOGO_ICON_OPACITY{ 1.0f };       // 로고 아이콘 불투명도
        constexpr float SEARCH_BOX_TOP{ 68.0f };         // 도구 검색칸 위치 (로고 아래)
        constexpr float SEARCH_BOX_HEIGHT{ 32.0f };      // 도구 검색칸 높이
        constexpr float SECTION_LABEL_TOP{ 110.0f };     // "TOOLS" 구역 이름 위치
        constexpr float SECTION_LABEL_HEIGHT{ 20.0f };   // "TOOLS" 줄 높이
        constexpr float SECTION_LABEL_LEFT{ 16.0f };     // "TOOLS" 왼쪽 위치
        constexpr float NAV_TOP{ 134.0f };               // 첫 메뉴 항목 위치
        constexpr float NO_RESULT_HEIGHT{ 40.0f };       // 검색 결과 없음 안내 줄 높이
        constexpr float NAV_ITEM_HEIGHT{ 36.0f };        // 메뉴 항목 하나 높이
        constexpr float NAV_ITEM_GAP{ 2.0f };            // 메뉴 항목 사이 간격
        constexpr float NAV_ITEM_RADIUS{ 6.0f };         // 메뉴 항목 바탕 모서리
        constexpr float NAV_ICON_LEFT{ 8.0f };          // 항목 안 아이콘 왼쪽 위치
        constexpr float NAV_ICON_WIDTH{ 20.0f };         // 아이콘 칸 너비
        constexpr float NAV_TEXT_LEFT{ 34.0f };          // 항목 안 제목 왼쪽 위치
        constexpr float NAV_CHEVRON_WIDTH{ 14.0f };      // 오른쪽 "›" 칸 너비 (선택 항목만 차지)
        constexpr float NAV_TEXT_RIGHT_PADDING{ 4.0f };  // 선택 안 된 항목 제목 오른쪽 여백
        constexpr float NAV_ACCENT_WIDTH{ 3.0f };        // 선택 항목 왼쪽 청록 막대 두께
        constexpr float NAV_ACCENT_INSET{ 8.0f };        // 청록 막대 위아래 여백
        constexpr float SIDEBAR_BOTTOM_PADDING{ 12.0f }; // 메뉴 아래쪽 여백
        constexpr float API_STATUS_HEIGHT{ 34.0f };      // 아래 API 상태 줄 높이
        constexpr float API_STATUS_DOT_RADIUS{ 3.5f };   // API 상태 점 반지름
        constexpr float API_STATUS_DOT_LEFT{ 12.0f };    // 상태 점 가운데 x (상태 줄 왼쪽 기준)
        constexpr float API_STATUS_TEXT_LEFT{ 22.0f };   // 상태 글자 왼쪽 위치 (상태 줄 왼쪽 기준)
        constexpr float API_BUTTON_HEIGHT{ 34.0f };      // "API 키 설정" 버튼 높이
        constexpr float API_BUTTON_GAP{ 6.0f };          // 버튼과 상태 줄 사이
        constexpr float LINE_WIDTH{ 1.0f };              // 구분선 두께
        constexpr float HALF{ 0.5f };                    // 가운데 정렬 계산용

        // ===== 위 막대 · 페이지 머리말 (DIP) =====
        constexpr float TOP_BAR_HEIGHT{ 56.0f };         // 위 막대 높이 (경로 표시)
        constexpr float TOP_BAR_PADDING{ 24.0f };        // 위 막대 좌우 여백
        constexpr float BREADCRUMB_GAP{ 10.0f };         // 경로 글자 사이 간격 ("멀티툴" › "제목")
        constexpr float VERSION_TEXT_WIDTH{ 120.0f };    // 오른쪽 버전 글자 칸 너비
        constexpr float PAGE_HEADER_HEIGHT{ 84.0f };     // 페이지 머리말 높이 (영문 · 제목 · 설명)
        constexpr float PAGE_HEADER_LEFT{ 24.0f };       // 머리말 왼쪽 여백 (탭 내용 안쪽 여백과 맞춤)
        constexpr float PAGE_EYEBROW_TOP{ 16.0f };       // 영문 소제목 위치
        constexpr float PAGE_EYEBROW_HEIGHT{ 14.0f };    // 영문 소제목 줄 높이
        constexpr float PAGE_TITLE_TOP{ 32.0f };         // 큰 제목 위치
        constexpr float PAGE_TITLE_HEIGHT{ 32.0f };      // 큰 제목 줄 높이
        constexpr float PAGE_DESCRIPTION_TOP{ 64.0f };   // 설명 위치
        constexpr float PAGE_DESCRIPTION_HEIGHT{ 18.0f };// 설명 줄 높이
        constexpr float CONTENT_INSET{ 8.0f };           // 탭 내용 좌우 여백 (탭 화면 자체 여백 16과 합쳐 머리말 24에 맞춤)

        // ===== 탭 전환 애니메이션 =====
        constexpr double INDICATOR_SMOOTHING{ 14.0 };   // 클수록 밑줄이 빨리 따라감 (초당 감쇠 계수)
        constexpr float INDICATOR_SNAP_DISTANCE{ 0.001f };
        constexpr double CONTENT_FADE_SECONDS{ 0.18 };  // 새 탭 내용이 나타나는 시간
        constexpr float FULL_FADE{ 1.0f };
        constexpr float NO_FADE{ 0.0f };
        constexpr double MAX_FRAME_SECONDS{ 0.1 };      // 창을 끌거나 멈췄다 돌아왔을 때 애니메이션이 튀지 않게
        constexpr int BACKGROUND_TICK_MS{ 250 };        // 트레이에 숨은 동안 백그라운드 작업 · 자동 갱신을 확인하는 간격
        constexpr int OVERLAY_TICK_MS{ 33 };            // 트레이에 숨어 있어도 인게임 오버레이가 보이면 이 간격으로 갱신 (초당 약 30번)
        constexpr std::wstring_view OVERLAY_ON_BUTTON_TEXT{ L"🖥  오버레이 켜짐" };     // 오버레이 켜짐 버튼 글자
        constexpr std::wstring_view OVERLAY_OFF_BUTTON_TEXT{ L"🖥  오버레이 꺼짐" };    // 오버레이 꺼짐 버튼 글자
        constexpr std::wstring_view OVERLAY_ON_TOAST_TEXT{ L"인게임 오버레이를 켰어요. 게임이 맨 앞일 때 보이고, Ctrl+Shift+I로 패널을 옮길 수 있어요. (게임은 창 모드 · 전체 창 모드)" };
        constexpr std::wstring_view OVERLAY_OFF_TOAST_TEXT{ L"인게임 오버레이를 껐어요. (Ctrl+Shift+O로 다시 켜요)" };

        // ===== 프레임 간격 측정 =====
        using Clock = std::chrono::steady_clock;        // 시스템 시간을 바꿔도 거꾸로 가지 않는 시계

        // 시계 눈금(tick) 1초당 개수 (눈금 단위 = Clock::period, MSVC는 나노초 → 1'000'000'000)
        constexpr double CLOCK_TICKS_PER_SECOND{ static_cast<double>(Clock::period::den) / static_cast<double>(Clock::period::num) };

        // ===== 단축키 =====
        constexpr std::size_t NEXT_TAB_STEP{ 1 };

        // ===== 문구 =====
        constexpr std::wstring_view API_KEY_BUTTON_TEXT{ L"⚙  API 키 설정" };   // 왼쪽 메뉴 아래 버튼 (톱니 기호 + 문구)
        constexpr std::wstring_view REPORT_BUTTON_TEXT{ L"✉  문제 보고" };      // 로그 폴더 열기 버튼 (API 키 버튼 위)
        constexpr std::wstring_view REPORT_TITLE{ L"문제 보고" };                // 안내 창 제목
        constexpr std::wstring_view REPORT_OPENED_TEXT{ L"로그 폴더를 열었어요!\n\napp.log (crash_*.dmp 파일이 있으면 함께) 를 첨부해서\n문제 상황과 같이 알려 주세요." };
        constexpr std::wstring_view REPORT_FAILED_TEXT{ L"로그 폴더를 열지 못했어요.\n\n%LocalAppData%\\LostArkMultiTool\\Logs 폴더를 직접 열어 주세요." };
        constexpr std::wstring_view KEY_SAVE_ERROR_TITLE{ L"API 키 저장 실패" };
        constexpr std::wstring_view KEY_SAVE_ERROR_TEXT{ L"API 키를 저장하지 못했어요!\n" };
        constexpr std::wstring_view KEY_SAVED_TEXT{ L"API 키를 저장했어요!" };
        constexpr std::wstring_view UNKNOWN_ERROR_TEXT{ L"(알 수 없는 오류)" };

        // ===== 트레이 · 원격 설정 문구 =====
        constexpr std::wstring_view TRAY_HIDDEN_TITLE{ L"로스트아크 멀티툴 백그라운드 실행 중" };
        constexpr std::wstring_view TRAY_HIDDEN_DEFAULT_TEXT{ L"트레이 아이콘을 더블클릭하면 다시 열려요." };
        constexpr std::wstring_view LINE_BREAK{ L"\n" };
        constexpr std::wstring_view PARAGRAPH_BREAK{ L"\n\n" };
        constexpr std::wstring_view MAINTENANCE_TITLE{ L"서버 점검 안내" };
        constexpr std::wstring_view MAINTENANCE_EXIT_TEXT{ L"\n\n프로그램을 종료해요." };
        constexpr std::wstring_view UPDATE_TITLE{ L"업데이트 안내" };
        constexpr std::wstring_view UPDATE_TEXT_FORMAT{ L"새로운 버전({})이 출시됐어요!\n최신 버전으로 업데이트를 권장해요." };
        constexpr std::wstring_view UPDATE_QUESTION_TEXT{ L"\n\n다운로드 페이지를 열까요?" };
        constexpr std::wstring_view UPDATE_OPEN_FAILED_TEXT{ L"다운로드 페이지를 열지 못했어요.\n" };
        constexpr std::wstring_view AUTO_UPDATE_QUESTION_TEXT{ L"\n\n지금 자동으로 업데이트할까요?\n(받은 파일을 검사한 뒤 프로그램을 다시 시작해요. [아니오]를 누르면 다운로드 페이지를 열지 물어봐요)" };
        constexpr std::wstring_view AUTO_UPDATE_PROGRESS_TEXT{ L"업데이트 파일을 받고 있어요… 끝나면 자동으로 다시 시작해요." };
        constexpr std::wstring_view AUTO_UPDATE_FAILED_FORMAT{ L"자동 업데이트를 하지 못했어요.\n{}\n\n[확인]을 누르면 다운로드 페이지를 열어요." };
        constexpr std::wstring_view AUTO_UPDATE_LAUNCH_FAILED_TEXT{ L"업데이트 설치를 시작하지 못했어요." };
        constexpr std::wstring_view NOTICE_TITLE{ L"공지" };
        constexpr std::wstring_view HTTPS_PREFIX{ L"https://" };
        constexpr std::wstring_view HTTP_PREFIX{ L"http://" };
        constexpr std::wstring_view TRIM_CHARACTERS{ L" \t\r\n" };

        // ===== 화면 틀 문구 =====
        constexpr std::wstring_view LOGO_TITLE_TEXT{ L"LOA KIT" };              // 로고 큰 글자
        constexpr std::wstring_view LOGO_SUBTITLE_TEXT{ L"로스트아크 멀티툴" };   // 로고 작은 글자
        constexpr std::wstring_view LOGO_ICON_TEXT{ L"L" };                     // 로고 아이콘 안 글자
        constexpr std::wstring_view SECTION_LABEL_TEXT{ L"TOOLS" };             // 메뉴 구역 이름
        constexpr std::wstring_view NAV_CHEVRON_TEXT{ L"›" };                   // 선택 항목 오른쪽 꺾쇠
        constexpr std::wstring_view BREADCRUMB_ROOT_TEXT{ L"멀티툴" };           // 위 막대 경로 첫 단계
        constexpr std::wstring_view BREADCRUMB_SEPARATOR_TEXT{ L"›" };          // 위 막대 경로 구분 기호
        constexpr std::wstring_view VERSION_PREFIX_TEXT{ L"v" };                // 버전 앞 글자
        constexpr std::wstring_view API_STATUS_READY_TEXT{ L"LOA API · 키 등록됨" };     // API 키가 저장되어 있음
        constexpr std::wstring_view API_STATUS_MISSING_TEXT{ L"LOA API · 키 미등록" };   // API 키가 없음
        constexpr std::wstring_view TOOL_SEARCH_PLACEHOLDER{ L"⌕  도구 검색   Ctrl+K" };   // 도구 검색칸 안내 글자
        constexpr std::wstring_view TOOL_SEARCH_EMPTY_TEXT{ L"검색 결과가 없어요" };     // 맞는 도구가 없을 때
        constexpr std::wstring_view TOP_API_READY_TEXT{ L"● API 연결됨" };              // 위 막대 오른쪽 상태 (키 있음)
        constexpr std::wstring_view TOP_API_MISSING_TEXT{ L"● API 키 미등록" };          // 위 막대 오른쪽 상태 (키 없음)
        constexpr std::wstring_view TITLE_SEPARATOR_TEXT{ L" - " };                      // 제목 표시줄 "앱 - 탭"
        constexpr std::string_view LAST_TOOL_FILE_NAME{ "LastTool.txt" };                // 마지막으로 쓴 도구 번호 (사용자 데이터 폴더)
        constexpr float TOP_STATUS_WIDTH{ 240.0f };                                       // 위 막대 오른쪽 상태 · 버전 칸 너비

        constexpr std::size_t TAB_TEXT_COUNT{ 11 };   // 탭별 아이콘 · 머리말 문구 수 (Application::TOOL_COUNT와 같은지 RenderPageHeader에서 검사)

        // 탭별 메뉴 아이콘 (탭 순서 = m_tools 순서, 이미지 없이 유니코드 기호 → DirectWrite가 기호 글꼴로 대체해서 그림)
        constexpr std::array<std::wstring_view, TAB_TEXT_COUNT> TAB_ICONS{
            L"▤",   // 귀속재료 계산기
            L"◆",   // 보석 최저가 검색기
            L"▲",   // 낙원력 트래커
            L"↗",   // 유각 시세 검색기
            L"✦",   // 악세 검색기
            L"▣",   // 강화책 검색기
            L"◈",   // 젬 시세 검색기
            L"⚔",   // 전투력 시뮬레이터
            L"⚒",   // 재련 시뮬레이터
            L"⇄",   // 2부 이관
            L"⚙"    // 오버레이 · 인식 설정
        };

        // 탭별 페이지 머리말 영문 소제목 (탭 순서 = m_tools 순서)
        constexpr std::array<std::wstring_view, TAB_TEXT_COUNT> TAB_EYEBROWS{
            L"BOUND MATERIAL",      // 귀속재료 계산기
            L"JEWEL MARKET",        // 보석 최저가 검색기
            L"PARADISE TRACKER",    // 낙원력 트래커
            L"ENGRAVING MARKET",    // 유각 시세 검색기
            L"ACCESSORY FINDER",    // 악세 검색기
            L"HONING BOOK",         // 강화책 검색기
            L"ARK PASSIVE GEM",     // 젬 시세 검색기
            L"COMBAT LAB",          // 전투력 시뮬레이터
            L"HONING SIMULATOR",    // 재련 시뮬레이터
            L"SEASON 2 TRANSFER",   // 2부 이관
            L"OVERLAY · VISION"     // 오버레이 · 인식 설정
        };

        // 탭별 페이지 머리말 설명 (각 탭이 실제로 하는 일, 탭 순서 = m_tools 순서)
        constexpr std::array<std::wstring_view, TAB_TEXT_COUNT> TAB_DESCRIPTIONS{
            L"원정대 캐릭터의 보유 귀속 재료를 골드 가치로 환산하고, 총자산 변동을 기록해요.",   // 귀속재료 계산기
            L"레벨 · 종류별 보석 최저가와 등락을 확인하고, 목표가 알림과 시세 기록을 관리해요.",   // 보석 최저가 검색기
            L"캐릭터별 낙원력을 기록하고 변화 추이를 표와 그래프로 확인해요.",                    // 낙원력 트래커
            L"거래소 유물 각인서 시세와 직전 · 전일 대비 등락을 한눈에 비교해요.",                 // 유각 시세 검색기
            L"경매장 4티어 고대 악세서리를 딜러 유효 옵션 기준으로 즉시 구매가 낮은 순으로 찾아요.", // 악세 검색기
            L"야금술 · 재봉술 강화책 시세를 단계별로 확인해요.",                                  // 강화책 검색기
            L"질서 · 혼돈 젬 시세를 확인하고, 원정대 귀속 젬 가치를 계산해요.",                    // 젬 시세 검색기
            L"장비 · 보석 · 각인 · 아크패시브를 바꿨을 때의 예상 전투력을 비교해요.",              // 전투력 시뮬레이터
            L"시작 → 목표 단계까지 필요한 재료 · 골드와 재련 경로를 계산해요.",                    // 재련 시뮬레이터
            L"2부 업데이트 때 지금 장비 · 장신구 · 팔찌 · 보석 · 스톤이 무엇으로 이관되는지 미리 보여줘요.",   // 2부 이관
            L"현재 캐릭터 인식 · 인게임 오버레이 배치 · 투명도 · 화면 인식 상태를 한곳에서 확인하고 바꿔요."      // 오버레이 · 인식 설정
        };

        constexpr Core::TextStyle LOGO_TITLE_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::BOLD, Core::TextAlign::LEADING };        // 로고 큰 글자
        constexpr Core::TextStyle LOGO_SUBTITLE_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };  // 로고 작은 글자
        constexpr Core::TextStyle LOGO_ICON_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::BOLD, Core::TextAlign::CENTER };           // 로고 아이콘 글자
        constexpr Core::TextStyle SECTION_LABEL_STYLE{ Theme::FONT_SIZE_EYEBROW, Core::FontWeight::BOLD, Core::TextAlign::LEADING };  // "TOOLS"
        constexpr Core::TextStyle NAV_ICON_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };       // 메뉴 아이콘
        constexpr Core::TextStyle ACTIVE_TAB_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };      // 선택한 메뉴 제목
        constexpr Core::TextStyle INACTIVE_TAB_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };  // 다른 메뉴 제목
        constexpr Core::TextStyle NAV_CHEVRON_STYLE{ Theme::FONT_SIZE_TAB, Core::FontWeight::NORMAL, Core::TextAlign::CENTER };       // "›"
        constexpr Core::TextStyle API_STATUS_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };     // API 상태 줄
        constexpr Core::TextStyle BREADCRUMB_ROOT_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING };   // 경로 첫 단계
        constexpr Core::TextStyle BREADCRUMB_TITLE_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::BOLD, Core::TextAlign::LEADING };    // 경로 현재 탭
        constexpr Core::TextStyle VERSION_STYLE{ Theme::FONT_SIZE_SMALL, Core::FontWeight::NORMAL, Core::TextAlign::TRAILING };          // 오른쪽 버전
        constexpr Core::TextStyle PAGE_EYEBROW_STYLE{ Theme::FONT_SIZE_EYEBROW, Core::FontWeight::BOLD, Core::TextAlign::LEADING };      // 머리말 영문 소제목
        constexpr Core::TextStyle PAGE_TITLE_STYLE{ Theme::FONT_SIZE_PAGE_TITLE, Core::FontWeight::BOLD, Core::TextAlign::LEADING };     // 머리말 큰 제목
        constexpr Core::TextStyle PAGE_DESCRIPTION_STYLE{ Theme::FONT_SIZE_NORMAL, Core::FontWeight::NORMAL, Core::TextAlign::LEADING }; // 머리말 설명

        // 제목 표시줄: "로스트아크 멀티툴 v1.1.0"
        [[nodiscard]] std::wstring BuildWindowTitle()
        {
            return std::wstring{ WINDOW_TITLE } + L" v" + Core::TextEncoding::UTF8ToWide(Core::APPConstants::APP_VERSION);
        }

        // 현재 시각을 시계 눈금 수로 (duration 변환 템플릿을 거치지 않아 편집기가 해석하지 못하는 부분이 없음)
        [[nodiscard]] Clock::rep GetClockTicks() noexcept
        {
            return Clock::now().time_since_epoch().count();
        }

        // 두 눈금 사이 시간 (초) — duration<double> 변환과 같은 계산 (정수 눈금 → double → 1초당 눈금 수로 나눔)
        [[nodiscard]] double TicksToSeconds(Clock::rep elapsedTicks) noexcept
        {
            return static_cast<double>(elapsedTicks) / CLOCK_TICKS_PER_SECOND;
        }

        // 원격 설정 글자(UTF-8) → 화면 문구 (앞뒤 공백 제거, 비었거나 변환할 수 없으면 빈 문자열)
        [[nodiscard]] std::wstring ToTrimmedText(const std::optional<std::string>& text)
        {
            if (!text.has_value()) return {};

            try
            {
                const std::wstring wideText{ Core::TextEncoding::UTF8ToWide(*text) };
                const std::size_t first{ wideText.find_first_not_of(TRIM_CHARACTERS) };
                if (first == std::wstring::npos) return {};
                return wideText.substr(first, wideText.find_last_not_of(TRIM_CHARACTERS) - first + 1);
            }
            catch (const std::exception&)
            {
                return {};
            }
        }

        [[nodiscard]] bool IsWebURL(std::wstring_view url) noexcept
        {
            return url.starts_with(HTTPS_PREFIX) || url.starts_with(HTTP_PREFIX);
        }

        // 예외 메시지(UTF-8) → 화면 문구(UTF-16), 변환할 수 없으면 기본 문구
        [[nodiscard]] std::wstring ToWideMessage(const char* message)
        {
            try
            {
                return Core::TextEncoding::UTF8ToWide(message);
            }
            catch (const std::exception&)
            {
                return std::wstring{ UNKNOWN_ERROR_TEXT };
            }
        }
    }

    Application::Application()
        : m_tools{ CalculatorManager::GetInstance(), GemFinderManager::GetInstance(), TrackerManager::GetInstance(), EngravingFinderManager::GetInstance(), AccessoryFinderManager::GetInstance(), RefineBookFinderManager::GetInstance(), ArkGemFinderManager::GetInstance(), CombatSimulatorManager::GetInstance(), RefineSimulatorManager::GetInstance(), TransferManager::GetInstance(), OverlaySetupManager::GetInstance() }  // 탭 순서대로 매니저 목록 초기화 (2부 이관 · 오버레이 설정 탭 포함, 매니저는 싱글톤이 소유)
    {
        // 창을 만드는 도중에도 WM_SIZE가 OnResize로 들어오므로, 모든 unique_ptr가 비어 있는 상태로 만들어진 뒤에 생성
        m_window = std::make_unique<MainWindow>(BuildWindowTitle(), WINDOW_WIDTH, WINDOW_HEIGHT, *this);
        m_renderer = std::make_unique<DX11Renderer>(m_window->GetNativeHandle(), m_window->GetDPI());
        m_logoImage = MainWindow::LoadProgramIconImage(LOGO_ICON_PIXELS);   // 메뉴 로고 = 프로그램 아이콘

        // 공용 서비스 (세 툴이 API 키 하나 · API 하나 · 알림 층 하나를 공유)
        m_keyStore = std::make_unique<Services::DPAPIKeyStore>();
        const std::optional<std::string> savedKey{ m_keyStore->Load() };
        m_hasAPIKey = savedKey.has_value() && !savedKey->empty();
        m_marketAPI = std::make_unique<Services::WinHTTPMarketAPI>(savedKey.value_or(std::string{}));
        m_notifier = std::make_unique<UI::OverlayNotifier>(*m_window);   // 창이 숨어 있으면 토스트 → 트레이 풍선 알림
        m_configChecker = std::make_unique<Services::GistServerConfigChecker>();

        m_toolContext = std::make_unique<Core::ToolContext>(Core::ToolContext{ *m_window, *m_notifier, *m_marketAPI,
            [this] { OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_CHANGE); } });

        // 왼쪽 메뉴 도구 검색칸 (입력하면 바로 메뉴 목록이 걸러짐, Enter = 첫 결과로 이동)
        m_toolSearchBox = &m_headerHost.Emplace<UI::TextBox>(*m_window, std::wstring{ TOOL_SEARCH_PLACEHOLDER });
        m_toolSearchBox->SetOnTextChanged([this] { ApplyToolFilter(); });                         // 입력마다 목록 갱신
        m_toolSearchBox->SetOnSubmit([this]
        {
            if (!m_visibleTools.empty()) SelectTool(m_visibleTools.front());                     // 첫 결과 도구로
            LeaveToolSearch(false);                                                               // 탭 화면으로 포커스
        });
        for (std::size_t index{}; index < TOOL_COUNT; ++index) m_visibleTools.push_back(index); // 처음엔 전부 보임

        // 왼쪽 메뉴 아래 버튼 (키보드 포커스를 가져가지 않음 → 탭 화면 입력칸의 포커스 유지)
        m_apiKeyButton = &m_headerHost.Emplace<UI::Button>(std::wstring{ API_KEY_BUTTON_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY,
            [this] { OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_CHANGE); });
        m_apiKeyButton->SetFocusable(false);

        // 문제 보고: 로그 · 크래시 덤프 폴더를 열어 첨부하기 쉽게 (포커스는 가져가지 않음)
        m_reportButton = &m_headerHost.Emplace<UI::Button>(std::wstring{ REPORT_BUTTON_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY,
            [this] { OpenProblemReport(); });
        m_reportButton->SetFocusable(false);   // 탭 화면 입력칸 포커스 유지

        // 인게임 오버레이 켜기 · 끄기 (문제 보고 버튼 위, 포커스는 가져가지 않음)
        m_overlayButton = &m_headerHost.Emplace<UI::Button>(std::wstring{ OVERLAY_OFF_BUTTON_TEXT }, UI::ButtonStyle::BUTTON_SECONDARY,
            [] { OverlayManager::GetInstance().SetEnabled(!OverlayManager::GetInstance().IsEnabled()); });   // 누르면 반대로
        m_overlayButton->SetFocusable(false);   // 탭 화면 입력칸 포커스 유지
    }

    Application::~Application()
    {
        // 예외로 Run을 빠져나온 경우에도 매니저 저장 기회를 줌 (소멸자 밖으로 예외를 던지지 않음)
        try
        {
            ShutdownTools();
        }
        catch (...)
        {
        }

        // 원격 설정 확인이 진행 중이면 끝날 때까지 기다림 (확인 객체보다 먼저)
        m_configTask.Wait();
        m_updateTask.Wait();   // 업데이트 받기가 진행 중이면 끝날 때까지

        // 서비스 · 렌더러(스왑체인)를 먼저 놓고 창을 닫음 → 창이 닫히며 오는 메시지는 빈 포인터 검사로 무시됨
        m_toolContext.reset();
        m_notifier.reset();
        m_renderer.reset();
        m_window.reset();
    }

    int Application::Run(int showCommand)
    {
        InitializeTools();
        InitializeOverlay();             // 인게임 오버레이 (탭이 다 준비된 뒤: 미니 모드 · 패널이 탭 매니저를 씀)
        LoadLastTool();                  // 마지막으로 쓴 도구부터 (없으면 첫 탭)
        GetActiveTool().OnActivated();
        SyncWindowTitle();               // 제목 표시줄에 탭 이름
        m_window->Show(showCommand);

        // 서버 상태부터 확인 (원본 ServerCheckForm) → 점검이면 안내 후 종료, 아니면 API 키 확인 · 업데이트 안내
        StartServerCheck(true);

        Clock::rep previousTicks{ GetClockTicks() };
        while (m_window->ProcessMessages())
        {
            const Clock::rep currentTicks{ GetClockTicks() };
            const double deltaSeconds{ TicksToSeconds(currentTicks - previousTicks) };
            previousTicks = currentTicks;

            // 트레이에 숨어 있어도 갱신은 계속 (백그라운드 작업 결과 · 자동 갱신 · 점검 확인)
            Update(std::min(deltaSeconds, MAX_FRAME_SECONDS));

            if (m_window->IsMinimized())
            {
                // 메인 창은 그리지 않고 잠깐씩만 깨어남 (메시지가 오면 바로 깨어남 → CPU · GPU 사용 0에 가깝게)
                // 게임 위 오버레이가 보이면 오버레이만 그리면서 조금 더 자주 깨어남
                const bool isOverlayVisible{ OverlayManager::GetInstance().IsVisible() };
                if (isOverlayVisible) OverlayManager::GetInstance().Render();
                m_window->WaitForMessage(isOverlayVisible ? OVERLAY_TICK_MS : BACKGROUND_TICK_MS);
                continue;
            }
            Render();   // EndFrame의 수직 동기화가 프레임 속도를 모니터 주사율로 맞춤
            OverlayManager::GetInstance().Render();   // 인게임 오버레이 (보일 때만, 초당 20번까지)
        }

        if (!m_isModal) GetActiveTool().OnDeactivated();   // 대화상자가 열려 있었다면 이미 내려놓은 상태
        ShutdownTools();
        return m_window->GetExitCode();
    }

    void Application::OnResize(int pixelWidth, int pixelHeight)
    {
        if (!m_renderer) return;   // 창을 만드는 도중 (렌더러 생성 전)

        m_renderer->Resize(pixelWidth, pixelHeight);
        Render();   // 창 크기를 끄는 동안(메시지 루프가 멈춘 상태)에도 화면이 따라오게
    }

    void Application::OnDPIChanged(float dpi)
    {
        if (m_renderer) m_renderer->SetDPI(dpi);
    }

    void Application::OnInput(const Core::InputEvent& event)
    {
        if (!m_notifier || !m_window || m_initializedToolCount < TOOL_COUNT) return;   // 만드는 중 · 닫는 중

        SyncModalState();
        if (m_isModal)
        {
            m_notifier->HandleInput(event);   // 대화상자가 열려 있으면 대화상자만 입력을 받음
        }
        else
        {
            const auto* key{ std::get_if<Core::KeyEvent>(&event) };
            if (key == nullptr || !HandleShortcut(*key)) RouteInput(event);
        }

        SyncModalState();   // 입력 처리 중 대화상자가 열리거나 닫혔을 수 있음
        UpdateCursor();
    }

    void Application::OnTrayCommand(TrayCommand command)
    {
        switch (command)
        {
            case TrayCommand::TRAY_OPEN:
                m_window->RestoreWindow();
                break;

            case TrayCommand::TRAY_CHANGE_API_KEY:
                // 창을 꺼내서 키 입력 창을 띄움 (점검 안내 중이거나 아직 시작 전이면 열지 않음)
                m_window->RestoreWindow();
                if (m_notifier && !m_isMaintenanceLocked && m_initializedToolCount == TOOL_COUNT) OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_CHANGE);
                break;

            case TrayCommand::TRAY_EXIT:
                m_window->RequestClose();
                break;

            default:
                break;
        }
    }

    // 트레이로 숨으면 백그라운드에서 무엇이 계속 도는지 알려 줌 (원본 Form1_Resize 풍선 알림)
    void Application::OnHiddenToTray()
    {
        std::wstring message;
        if (m_initializedToolCount == TOOL_COUNT)
        {
            for (const Core::ToolManager& tool : m_tools)
            {
                const std::optional<std::wstring> status{ tool.GetBackgroundStatus() };
                if (!status.has_value()) continue;

                if (!message.empty()) message += LINE_BREAK;
                message += *status;
            }
        }
        if (message.empty()) message = TRAY_HIDDEN_DEFAULT_TEXT;

        m_window->ShowTrayBalloon(TRAY_HIDDEN_TITLE, message, Core::NoticeKind::NOTICE_INFO);
    }

    // 원격 설정 받기 시작 (블로킹 요청이라 백그라운드에서, 실행 직후면 확인 중 대화상자를 띄움)
    void Application::StartServerCheck(bool isStartup)
    {
        if (m_configTask.IsRunning() || m_isMaintenanceLocked) return;

        m_isStartupCheck = isStartup;
        if (isStartup)
        {
            auto dialog{ std::make_unique<ServerCheckDialog>() };
            m_serverCheckDialog = dialog.get();
            m_notifier->OpenDialog(std::move(dialog));
        }

        Core::ServerConfigChecker* checker{ m_configChecker.get() };
        m_configTask.Start([checker] { return checker->Fetch(); });
    }

    // 원격 설정을 받음 → 점검이면 종료 안내, 실행 직후면 주기 확인 시작 · 공지 · 첫 실행 키 입력
    void Application::FinishServerCheck()
    {
        std::optional<Models::ServerConfig> config;
        try
        {
            config = m_configTask.TakeResult();
        }
        catch (const std::exception&)
        {
            config.reset();   // 확인 실패는 점검 아님으로 취급 (원본과 같음 → 서버 문제로 프로그램이 막히지 않게)
        }

        const bool isStartup{ m_isStartupCheck };
        if (m_serverCheckDialog != nullptr)
        {
            m_serverCheckDialog->Close();   // 실제 삭제는 다음 Update에서 (OverlayNotifier)
            m_serverCheckDialog = nullptr;
        }

        // Debug 빌드는 점검 중이어도 잠그지 않음 (작업 중 테스트용, IS_MAINTENANCE_LOCK_ENABLED 참고)
        if (IS_MAINTENANCE_LOCK_ENABLED && config.has_value() && config->IsMaintenance())
        {
            LockForMaintenance(config->GetMaintenanceMessage());
            return;
        }
        if (!isStartup) return;

        m_configTimer.Start(Core::APPConstants::CONFIG_CHECK_INTERVAL_MS);

        // 대화상자는 나중에 연 것이 위 → 키 입력 창을 맨 위로, 업데이트 · 공지는 그다음에 보이게
        if (config.has_value()) ShowStartupNotices(*config);
        if (!m_hasAPIKey) OpenAPIKeyDialog(APIKeyDialogMode::API_KEY_FIRST_RUN);
    }

    // 점검 중: 숨어 있어도 창을 꺼내 안내 → [확인]을 누르면 종료 (원본: 실행 중 점검 감지 → 안내 후 Application.Exit)
    void Application::LockForMaintenance(const std::optional<std::string>& maintenanceMessage)
    {
        m_isMaintenanceLocked = true;
        m_configTimer.Stop();
        m_window->RestoreWindow();

        std::wstring message{ ToTrimmedText(maintenanceMessage) };
        if (message.empty()) message = ToTrimmedText(std::string{ Core::APPConstants::DEFAULT_MAINTENANCE_MESSAGE });

        m_notifier->ShowMessage(std::wstring{ MAINTENANCE_TITLE }, message + std::wstring{ MAINTENANCE_EXIT_TEXT }, Core::NoticeKind::NOTICE_WARNING,
            [this] { m_window->RequestClose(); });
    }

    // 새 버전 안내 (공지 포함) 또는 공지만 (원본 ShowUpdateNotice)
    void Application::ShowStartupNotices(const Models::ServerConfig& config)
    {
        const std::wstring notice{ ToTrimmedText(config.GetNotice()) };
        const std::wstring latestVersion{ ToTrimmedText(config.GetLatestVersion()) };
        const std::string latestVersionText{ config.GetLatestVersion().value_or(std::string{}) };

        if (latestVersion.empty() || !Core::ServerConfigChecker::IsNewerVersion(latestVersionText, Core::ServerConfigChecker::GetCurrentVersion()))
        {
            if (!notice.empty()) m_notifier->ShowMessage(std::wstring{ NOTICE_TITLE }, notice, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        std::wstring message{ std::format(UPDATE_TEXT_FORMAT, latestVersion) };
        if (!notice.empty()) message += std::wstring{ PARAGRAPH_BREAK } + notice;

        // 다운로드 주소가 웹 주소일 때만 열지 물어봄 (파일 · 프로그램 실행 방지)
        const std::wstring downloadURL{ ToTrimmedText(config.GetDownloadURL()) };
        if (!IsWebURL(downloadURL))
        {
            m_notifier->ShowMessage(std::wstring{ UPDATE_TITLE }, message, Core::NoticeKind::NOTICE_INFO, nullptr);
            return;
        }

        m_downloadURL = downloadURL;   // 자동 업데이트가 실패하면 이 페이지를 안내

        // 배포 zip 주소 + SHA-256이 있으면 자동 업데이트를 먼저 물어봄 ([아니오] → 다운로드 페이지)
        Services::UpdatePackage package{ latestVersionText, config.GetPackageURL().value_or(std::string{}), config.GetPackageSHA256().value_or(std::string{}) };
        if (Services::UpdateInstaller::IsValidPackage(package))
        {
            m_notifier->AskYesNo(std::wstring{ UPDATE_TITLE }, message + std::wstring{ AUTO_UPDATE_QUESTION_TEXT },
                [this, package, message](bool isYes)
            {
                if (isYes) StartAutoUpdate(package);   // 받기 · 검증 시작
                else AskOpenDownloadPage(message);     // 직접 받기
            });
            return;
        }

        AskOpenDownloadPage(message);   // 자동 업데이트 정보가 없는 버전 → 다운로드 페이지만
    }

    // 다운로드 페이지를 열지 물어봄 (원래 업데이트 안내)
    void Application::AskOpenDownloadPage(const std::wstring& message)
    {
        m_notifier->AskYesNo(std::wstring{ UPDATE_TITLE }, message + std::wstring{ UPDATE_QUESTION_TEXT }, [this](bool isYes)
        {
            if (!isYes || m_window->OpenURL(m_downloadURL)) return;   // 열었거나 원하지 않음
            m_notifier->ShowMessage(std::wstring{ UPDATE_TITLE }, std::wstring{ UPDATE_OPEN_FAILED_TEXT } + m_downloadURL, Core::NoticeKind::NOTICE_WARNING, nullptr);   // 열기 실패
        });
    }

    // 자동 업데이트: 백그라운드에서 zip 받기 + SHA-256 확인
    void Application::StartAutoUpdate(const Services::UpdatePackage& package)
    {
        if (m_updateTask.IsRunning()) return;   // 이미 받는 중
        m_notifier->ShowMessage(std::wstring{ UPDATE_TITLE }, std::wstring{ AUTO_UPDATE_PROGRESS_TEXT }, Core::NoticeKind::NOTICE_INFO, nullptr);   // 진행 안내
        m_updateTask.Start([package] { return Services::UpdateInstaller::DownloadAndVerify(package); });   // 블로킹 요청은 백그라운드 (값 복사)
    }

    // 받기 · 검증 결과 → 성공하면 설치 스크립트를 띄우고 종료, 실패하면 이유 + 다운로드 페이지 안내
    void Application::FinishAutoUpdate()
    {
        std::wstring failure;   // 실패 이유
        try
        {
            const std::filesystem::path zipPath{ m_updateTask.TakeResult() };   // 검증까지 끝난 zip
            if (Services::UpdateInstaller::LaunchInstaller(zipPath))
            {
                m_window->RequestClose();      // 종료 (탭 저장은 평소 종료 흐름) → 설치 스크립트가 덮어쓰고 새 버전 실행
                return;
            }
            failure = std::wstring{ AUTO_UPDATE_LAUNCH_FAILED_TEXT };   // 스크립트 실행 실패
        }
        catch (const std::exception& exception)
        {
            Core::AppLogger::GetInstance().Warning(std::format("자동 업데이트 실패: {}", exception.what()));   // 원인 기록
            failure = Core::TextEncoding::UTF8ToWide(exception.what());                                       // 화면 표시용
        }

        m_notifier->ShowMessage(std::wstring{ UPDATE_TITLE }, std::vformat(AUTO_UPDATE_FAILED_FORMAT, std::make_wformat_args(failure)),
            Core::NoticeKind::NOTICE_WARNING, [this] { if (!m_downloadURL.empty()) (void)m_window->OpenURL(m_downloadURL); });   // 확인 → 다운로드 페이지
    }

    // 문제 보고: 로그 폴더를 열고 무엇을 첨부할지 안내
    void Application::OpenProblemReport()
    {
        Core::AppLogger::GetInstance().Info(std::format("문제 보고 열기 (버전 {})", Core::APPConstants::APP_VERSION));   // 보고 시점의 버전 기록
        const bool isOpened{ Core::AppLogger::OpenLogDirectory() };                                                     // 탐색기로 열기
        m_notifier->ShowMessage(std::wstring{ REPORT_TITLE }, std::wstring{ isOpened ? REPORT_OPENED_TEXT : REPORT_FAILED_TEXT },
            isOpened ? Core::NoticeKind::NOTICE_INFO : Core::NoticeKind::NOTICE_WARNING, nullptr);                       // 안내
    }

    void Application::InitializeTools()
    {
        for (Core::ToolManager& tool : m_tools)
        {
            tool.Initialize(*m_toolContext);
            ++m_initializedToolCount;
        }
        ArkGemFinderManager::GetInstance().SetVisionControl(                                     // 젬 탭 [화면 인식] = 귀속재료 계산기 [화면 인식]
            [] { return CalculatorManager::GetInstance().IsVisionEnabled(); },
            [](bool isEnabled) { CalculatorManager::GetInstance().SetVisionEnabled(isEnabled); });
    }

    // 인게임 오버레이 준비: 탭 목록 넘기기 · 메뉴 버튼 글자 · 토스트를 게임 위에도
    void Application::InitializeOverlay()
    {
        OverlayManager& overlay{ OverlayManager::GetInstance() };
        try
        {
            overlay.Initialize(std::span{ m_tools }.first<OVERLAY_TOOL_COUNT>());   // 투명 창 · 렌더러 · 단축키 · 패널 (미니 탭에는 설정 탭을 빼고 넘김)
            overlay.SetOnRefreshHotkey([] { CalculatorManager::GetInstance().RequestRefresh(); });   // Ctrl+Shift+F5 = 시세 갱신 (게임 중에도)
        }
        catch (const std::exception& error)
        {
            Core::AppLogger::GetInstance().Error(std::format("오버레이 준비 실패: {}", error.what()));   // 오버레이 없이 계속 (메인 기능은 그대로)
            overlay.Shutdown();
            m_overlayButton->SetEnabled(false);
            return;
        }

        m_overlayButton->SetText(std::wstring{ overlay.IsEnabled() ? OVERLAY_ON_BUTTON_TEXT : OVERLAY_OFF_BUTTON_TEXT });   // 지금 상태
        overlay.SetOnEnabledChanged([this](bool isEnabled)
        {
            m_overlayButton->SetText(std::wstring{ isEnabled ? OVERLAY_ON_BUTTON_TEXT : OVERLAY_OFF_BUTTON_TEXT });   // 버튼 글자
            if (m_notifier) m_notifier->ShowToast(std::wstring{ isEnabled ? OVERLAY_ON_TOAST_TEXT : OVERLAY_OFF_TOAST_TEXT }, Core::NoticeKind::NOTICE_INFO);   // 안내
        });
        m_notifier->SetToastListener([](const std::wstring& message, Core::NoticeKind kind)
        {
            return OverlayManager::GetInstance().ShowToast(message, kind);   // 게임 위에 보이면 거기에도 띄움
        });
    }

    void Application::ShutdownTools()
    {
        OverlayManager::GetInstance().Shutdown();   // 오버레이 먼저 (미니 모드가 탭 매니저를 그리고 있을 수 있음)

        // 초기화한 역순으로 정리, 하나가 실패해도 나머지는 끝까지 저장하고 첫 예외만 다시 던짐
        std::exception_ptr firstError;
        while (m_initializedToolCount > 0)
        {
            --m_initializedToolCount;
            try
            {
                m_tools[m_initializedToolCount].get().Shutdown();
            }
            catch (...)
            {
                if (!firstError) firstError = std::current_exception();
            }
        }

        if (firstError) std::rethrow_exception(firstError);
    }

    void Application::SelectTool(std::size_t index)
    {
        if (index >= TOOL_COUNT || index == m_activeIndex) return;

        GetActiveTool().OnDeactivated();
        m_activeIndex = index;
        GetActiveTool().OnActivated();

        m_contentFade = FULL_FADE;   // 새 탭 내용이 배경에서 서서히 나타나게
        SyncWindowTitle();           // 제목 표시줄 동기화
        SaveLastTool();              // 다음 실행 때 이 도구부터
    }

    void Application::SelectAdjacentTool(bool isNext)
    {
        if (m_visibleTools.empty()) return;                                                      // 보이는 도구 없음
        const auto found{ std::ranges::find(m_visibleTools, m_activeIndex) };                    // 지금 도구의 목록 위치
        std::size_t position{ found == m_visibleTools.end() ? 0 : static_cast<std::size_t>(found - m_visibleTools.begin()) };   // 목록에 없으면 처음
        const std::size_t count{ m_visibleTools.size() };                                        // 보이는 도구 수
        if (found != m_visibleTools.end()) position = isNext ? (position + 1) % count : (position + count - 1) % count;   // 끝에서 처음으로 순환
        SelectTool(m_visibleTools[position]);                                                    // 이동
    }

    void Application::ApplyToolFilter()
    {
        std::wstring query{ m_toolSearchBox->GetText() };                                        // 검색어
        std::erase(query, L' ');                                                                 // 띄어쓰기 무시
        m_visibleTools.clear();                                                                  // 다시 계산
        for (std::size_t index{}; index < TOOL_COUNT; ++index)
        {
            std::wstring title{ m_tools[index].get().GetTabTitle() };                            // 도구 이름
            std::erase(title, L' ');                                                             // 띄어쓰기 무시
            if (query.empty() || title.find(query) != std::wstring::npos) m_visibleTools.push_back(index);   // 포함하면 보임
        }
        m_hoverIndex.reset();                                                                    // 위치가 바뀌었으니 마우스 표시 초기화
    }

    void Application::FocusToolSearch()
    {
        if (IsToolSearchFocused()) { m_toolSearchBox->SelectAll(); return; }                     // 이미 검색 중이면 전체 선택
        GetActiveTool().OnDeactivated();                                                         // 탭 화면 입력 상태 내려놓기
        m_headerHost.SetFocus(m_toolSearchBox);                                                  // 검색칸으로
        m_toolSearchBox->SelectAll();                                                            // 바로 새 검색어 입력
    }

    void Application::LeaveToolSearch(bool isClearing)
    {
        if (isClearing) m_toolSearchBox->SetText(std::wstring{});                                 // Esc = 검색어 초기화
        ApplyToolFilter();                                                                       // 목록 다시 계산
        if (!IsToolSearchFocused()) return;                                                      // 이미 떠났음
        m_headerHost.SetFocus(nullptr);                                                          // 검색칸 포커스 해제
        GetActiveTool().OnActivated();                                                           // 탭 화면 포커스 되돌리기
    }

    bool Application::IsToolSearchFocused() const noexcept
    {
        return m_toolSearchBox != nullptr && m_headerHost.GetFocused() == m_toolSearchBox;      // 검색칸이 포커스를 가짐
    }

    void Application::LoadLastTool()
    {
        try
        {
            std::ifstream file{ Core::APPConstants::GetUserDataFilePath(LAST_TOOL_FILE_NAME) };  // 숫자 하나
            std::size_t index{};                                                                 // 저장된 도구 번호
            if (file >> index && index < TOOL_COUNT) m_activeIndex = index;                      // 범위 안이면 사용
        }
        catch (...)
        {
            m_activeIndex = 0;                                                                   // 읽기 실패 → 첫 탭
        }
        m_indicatorPosition = static_cast<float>(m_activeIndex);                                 // 선택 표시는 바로 그 자리
    }

    void Application::SaveLastTool() const noexcept
    {
        try
        {
            const std::filesystem::path path{ Core::APPConstants::GetUserDataFilePath(LAST_TOOL_FILE_NAME) };   // 저장 위치
            std::error_code error;                                                               // 폴더 만들기 실패는 쓰기에서 드러남
            std::filesystem::create_directories(path.parent_path(), error);                     // 첫 실행 대비
            std::ofstream file{ path, std::ios::trunc };                                         // 덮어쓰기
            file << m_activeIndex;                                                               // 숫자 하나
        }
        catch (...)
        {
            // 저장 실패는 다음 실행 때 첫 탭으로 열릴 뿐
        }
    }

    void Application::SyncWindowTitle()
    {
        m_window->SetTitle(std::format(L"{}{}{}", BuildWindowTitle(), TITLE_SEPARATOR_TEXT, GetActiveTool().GetTabTitle()));   // "로스트아크 멀티툴 v1.6.0 - 탭"
    }

    void Application::OpenAPIKeyDialog(APIKeyDialogMode mode)
    {
        if (m_isAPIKeyDialogOpen) return;

        // 재입력이면 기존 키를 채워서 보여 줌 (원본과 같음)
        std::wstring currentKey;
        if (mode == APIKeyDialogMode::API_KEY_CHANGE)
        {
            try
            {
                if (const std::optional<std::string> savedKey{ m_keyStore->Load() }) currentKey = Core::TextEncoding::UTF8ToWide(*savedKey);
            }
            catch (const std::exception&)
            {
                currentKey.clear();   // 읽을 수 없으면 빈칸으로 새로 입력
            }
        }

        m_isAPIKeyDialogOpen = true;
        m_notifier->OpenDialog(std::make_unique<APIKeyDialog>(*m_window, *m_notifier, mode, std::move(currentKey),
            [this](const std::string& apiKey) { return SaveAPIKey(apiKey); },
            [this, mode]
        {
            m_isAPIKeyDialogOpen = false;
            if (mode == APIKeyDialogMode::API_KEY_FIRST_RUN) m_window->RequestClose();   // 처음 실행에서 [종료] → 프로그램 종료 (원본과 같음)
        }));
    }

    bool Application::SaveAPIKey(const std::string& apiKey)
    {
        try
        {
            m_keyStore->Save(apiKey);
        }
        catch (const std::exception& error)
        {
            m_notifier->ShowMessage(std::wstring{ KEY_SAVE_ERROR_TITLE }, std::wstring{ KEY_SAVE_ERROR_TEXT } + ToWideMessage(error.what()),
                Core::NoticeKind::NOTICE_ERROR, nullptr);
            return false;
        }

        m_marketAPI->SetAPIKey(apiKey);   // 이후 요청부터 새 키 사용 (세 툴 공용)
        m_hasAPIKey = true;
        m_isAPIKeyDialogOpen = false;
        m_notifier->ShowToast(std::wstring{ KEY_SAVED_TEXT }, Core::NoticeKind::NOTICE_SUCCESS);

        // 키 오류로 멈춘 자동 갱신 등을 툴마다 다시 시작할 수 있게 알림 (초기화가 끝난 툴만)
        for (std::size_t index{}; index < m_initializedToolCount; ++index) m_tools[index].get().OnAPIKeyChanged();
        return true;
    }

    bool Application::HandleShortcut(const Core::KeyEvent& event)
    {
        // Alt+↑/↓: 도구 간 빠른 이동 (검색으로 걸러진 목록 안에서)
        if (event.m_isAltDown && (event.m_key == Core::Key::KEY_UP || event.m_key == Core::Key::KEY_DOWN))
        {
            SelectAdjacentTool(event.m_key == Core::Key::KEY_DOWN);   // 아래 = 다음
            return true;
        }

        // 검색칸에서 Esc = 검색어 초기화 후 탭 화면으로, ↓ = 탭 화면으로 (Enter는 검색칸이 처리)
        if (IsToolSearchFocused() && event.m_key == Core::Key::KEY_ESCAPE)
        {
            LeaveToolSearch(true);
            return true;
        }

        if (!event.m_isCtrlDown) return false;

        // Ctrl+K: 도구 검색칸으로 바로 포커스
        if (event.m_key == Core::Key::KEY_K)
        {
            FocusToolSearch();
            return true;
        }

        // Ctrl+Tab: 다음 탭, Ctrl+Shift+Tab: 이전 탭 (끝에서 처음으로 돌아감)
        if (event.m_key == Core::Key::KEY_TAB)
        {
            const std::size_t step{ event.m_isShiftDown ? TOOL_COUNT - NEXT_TAB_STEP : NEXT_TAB_STEP };
            SelectTool((m_activeIndex + step) % TOOL_COUNT);
            return true;
        }

        // Ctrl+1 ~ Ctrl+3: 해당 탭으로 바로 이동 (Ctrl+A · C · V · X는 입력칸으로 보냄)
        const int tabIndex{ std::to_underlying(event.m_key) - std::to_underlying(Core::Key::KEY_1) };
        if (tabIndex >= 0 && static_cast<std::size_t>(tabIndex) < TOOL_COUNT)
        {
            SelectTool(static_cast<std::size_t>(tabIndex));
            return true;
        }
        return false;
    }

    // 탭 막대(위) · 탭 내용(아래)으로 입력을 나눠 줌 (키보드 · 글자 입력은 탭 내용으로)
    void Application::RouteInput(const Core::InputEvent& event)
    {
        Core::ToolManager& tool{ GetVisibleTool() };

        std::visit(Core::Overloaded{
            [&](const Core::MouseMoveEvent& move)
            {
                m_isPointerInHeader = IsInHeader(move.m_x, move.m_y);   // 왼쪽 메뉴 · 위 막대 · 머리말 위인지

                // 누른 채 움직이면 누른 곳으로만 보냄
                if (m_pointerOwner == PointerOwner::POINTER_TOOL)
                {
                    static_cast<void>(tool.HandleInput(event));
                    return;
                }
                if (m_pointerOwner == PointerOwner::POINTER_HEADER)
                {
                    m_headerHost.HandleInput(event);
                    return;
                }

                m_hoverIndex = FindTabAt(move.m_x, move.m_y);
                m_headerHost.HandleInput(event);
                static_cast<void>(tool.HandleInput(m_isPointerInHeader ? Core::InputEvent{ Core::MouseLeaveEvent{} } : event));
            },
            [&](const Core::MouseLeaveEvent&)
            {
                m_isPointerInHeader = false;
                m_hoverIndex.reset();
                m_headerHost.HandleInput(event);
                static_cast<void>(tool.HandleInput(event));
            },
            [&](const Core::MouseButtonEvent& button) { RouteMouseButton(event, button); },
            [&](const Core::MouseWheelEvent& wheel)
            {
                if (IsInHeader(wheel.m_x, wheel.m_y)) m_headerHost.HandleInput(event);   // 화면 틀 위 휠은 틀로
                else static_cast<void>(tool.HandleInput(event));
            },
            [&](const auto&)
            {
                if (IsToolSearchFocused()) m_headerHost.HandleInput(event);   // 도구 검색칸 입력
                else static_cast<void>(tool.HandleInput(event));              // 키 · 글자 · 한글 조합
            }
            }, event);
    }

    void Application::RouteMouseButton(const Core::InputEvent& event, const Core::MouseButtonEvent& button)
    {
        Core::ToolManager& tool{ GetVisibleTool() };
        const bool isLeft{ button.m_button == Core::MouseButton::MOUSE_BUTTON_LEFT };

        if (button.m_isPressed)
        {
            const bool isHeader{ IsInHeader(button.m_x, button.m_y) };   // 화면 틀(메뉴 · 위 막대 · 머리말)을 눌렀는지
            if (isLeft) m_pointerOwner = isHeader ? PointerOwner::POINTER_HEADER : PointerOwner::POINTER_TOOL;

            if (!isHeader)
            {
                if (IsToolSearchFocused()) LeaveToolSearch(false);   // 검색칸에서 탭 화면으로
                static_cast<void>(tool.HandleInput(event));
                return;
            }

            if (const auto index{ FindTabAt(button.m_x, button.m_y) }; index.has_value() && isLeft) SelectTool(*index);
            m_headerHost.HandleInput(event);
            return;
        }

        // 뗄 때는 누른 곳으로 (누른 적이 없으면 마우스 위치 기준)
        PointerOwner owner{ isLeft ? m_pointerOwner : PointerOwner::POINTER_NONE };
        if (owner == PointerOwner::POINTER_NONE) owner = IsInHeader(button.m_x, button.m_y) ? PointerOwner::POINTER_HEADER : PointerOwner::POINTER_TOOL;   // 누른 적 없으면 위치로 판단
        if (isLeft) m_pointerOwner = PointerOwner::POINTER_NONE;

        if (owner == PointerOwner::POINTER_HEADER) m_headerHost.HandleInput(event);
        else static_cast<void>(tool.HandleInput(event));
    }

    // 대화상자가 열리고 닫힐 때 탭 화면의 입력 상태를 맞춤 (열리면 포커스 내려놓기, 닫히면 되돌리기)
    void Application::SyncModalState()
    {
        const bool isModal{ m_notifier->HasDialog() };
        if (isModal == m_isModal) return;

        m_isModal = isModal;
        m_pointerOwner = PointerOwner::POINTER_NONE;
        m_hoverIndex.reset();

        if (isModal)
        {
            m_headerHost.SuspendFocus();
            GetActiveTool().OnDeactivated();
        }
        else
        {
            GetActiveTool().OnActivated();
        }
    }

    // 마우스 위치 · 누른 곳에 맞는 커서 (글자 입력칸 = I자, 버튼 · 탭 = 손 모양)
    void Application::UpdateCursor()
    {
        Core::CursorShape shape{ Core::CursorShape::CURSOR_ARROW };
        if (m_isModal) shape = m_notifier->GetCursor();
        else if (m_pointerOwner == PointerOwner::POINTER_TOOL) shape = GetVisibleTool().GetCursor();
        else if (m_pointerOwner == PointerOwner::POINTER_HEADER) shape = m_headerHost.GetCursor();
        else if (m_isPointerInHeader) shape = m_hoverIndex.has_value() ? Core::CursorShape::CURSOR_HAND : m_headerHost.GetCursor();
        else shape = GetVisibleTool().GetCursor();

        m_window->SetCursor(shape);
    }

    void Application::Update(double deltaSeconds)
    {
        // 원격 설정 확인 결과 · 30분 주기 확인 (대화상자를 열 수 있으므로 알림 층 갱신보다 먼저)
        if (m_configTask.IsReady()) FinishServerCheck();
        if (m_configTimer.Poll()) StartServerCheck(false);
        if (m_updateTask.IsReady()) FinishAutoUpdate();   // 자동 업데이트 받기 · 검증 결과

        // 조회 스레드가 신고한 API 형식 변경 → 응답마다 한 번만 알림
        if (const std::optional<std::wstring> formatNotice{ Core::APIFormatMonitor::TakePendingNotice() }; formatNotice.has_value())
            m_notifier->ShowToast(*formatNotice, Core::NoticeKind::NOTICE_WARNING);

        m_notifier->Update(deltaSeconds);   // 닫힌 대화상자 정리 → 모달 상태 확인보다 먼저
        SyncModalState();

        // 메뉴 선택 표시: 남은 거리의 일정 비율씩 다가감 (지수 감쇠 → 프레임 속도가 달라도 같은 빠르기)
        const auto activeIt{ std::ranges::find(m_visibleTools, m_activeIndex) };   // 보이는 목록에서 선택 도구 위치
        const float targetPosition{ activeIt == m_visibleTools.end() ? m_indicatorPosition : static_cast<float>(activeIt - m_visibleTools.begin()) };   // 숨었으면 그대로
        const float followRatio{ static_cast<float>(1.0 - std::exp(-INDICATOR_SMOOTHING * deltaSeconds)) };
        m_indicatorPosition += (targetPosition - m_indicatorPosition) * followRatio;
        if (std::abs(targetPosition - m_indicatorPosition) < INDICATOR_SNAP_DISTANCE) m_indicatorPosition = targetPosition;

        const float fadeStep{ static_cast<float>(deltaSeconds / CONTENT_FADE_SECONDS) };
        m_contentFade = std::max(NO_FADE, m_contentFade - fadeStep);

        m_headerHost.Update(deltaSeconds);

        // 인게임 오버레이: 게임 창 따라가기 · 패널 갱신, 안내(단축키 등록 실패 등)는 메인 창 토스트로
        OverlayManager::GetInstance().Update(deltaSeconds);
        if (const std::optional<std::wstring> overlayNotice{ OverlayManager::GetInstance().TakeNotice() }; overlayNotice.has_value())
            m_notifier->ShowToast(*overlayNotice, Core::NoticeKind::NOTICE_WARNING);

        // 모든 탭을 갱신 (다른 탭을 보거나 대화상자가 떠 있어도 백그라운드 작업 결과 · 자동 갱신 진행), 점검 중이면 멈춤
        if (!m_isMaintenanceLocked)
        {
            for (Core::ToolManager& tool : m_tools) tool.Update(deltaSeconds);
        }
        UpdateCursor();   // 대화상자가 닫히는 등 마우스를 움직이지 않아도 커서가 바뀔 수 있음
    }

    void Application::Render()
    {
        if (!m_renderer || !m_notifier || !m_renderer->BeginFrame()) return;   // 그릴 준비가 안 됐으면 건너뜀

        m_renderer->Clear(Theme::BACKGROUND_COLOR);   // 전체 배경 (거의 검정인 남색)

        // 1) 선택한 탭 내용
        const Core::RectF contentRect{ GetContentRect() };   // 왼쪽 메뉴 · 위 막대 · 머리말을 뺀 영역
        GetVisibleTool().Render(*m_renderer, contentRect);   // 탭 화면 그리기 (미니 모드로 띄운 탭이면 안내만)

        // 2) 페이지 머리말 (영문 소제목 · 큰 제목 · 설명)
        RenderPageHeader();

        // 3) 탭 전환 직후: 머리말 · 내용 위에 배경색을 덮었다가 걷어 냄 (페이드 인)
        if (m_contentFade > NO_FADE)
        {
            const Core::RectF pageRect{ SIDEBAR_WIDTH, TOP_BAR_HEIGHT, m_renderer->GetWidth(), m_renderer->GetHeight() };   // 머리말 + 내용 영역
            m_renderer->FillRect(pageRect, Core::WithAlpha(Theme::BACKGROUND_COLOR, m_contentFade));   // 배경색을 점점 투명하게
        }

        // 4) 위 막대 · 왼쪽 메뉴는 내용 위에 그려서 항상 보이게
        RenderTopBar();
        RenderSidebar();

        // 5) 대화상자 · 알림은 맨 위
        m_notifier->Render(*m_renderer, GetScreenRect());

        m_renderer->EndFrame();   // 화면에 내보내기
    }

    void Application::RenderSidebar()
    {
        const float height{ m_renderer->GetHeight() };   // 창 높이
        m_renderer->FillRect(Core::RectF{ 0.0f, 0.0f, SIDEBAR_WIDTH, height }, Theme::SIDEBAR_BACK_COLOR);   // 메뉴 바탕
        m_renderer->DrawLine(SIDEBAR_WIDTH, 0.0f, SIDEBAR_WIDTH, height, Theme::CARD_BORDER_COLOR, LINE_WIDTH);   // 메뉴 오른쪽 경계선

        // ===== 로고 (청록 정사각형 아이콘 + "LOA KIT / 로스트아크 멀티툴") =====
        const float logoTop{ (TOP_BAR_HEIGHT - LOGO_ICON_SIZE) * HALF };   // 아이콘을 위 막대 높이 가운데에
        const Core::RectF logoIcon{ LOGO_ICON_LEFT, logoTop, LOGO_ICON_LEFT + LOGO_ICON_SIZE, logoTop + LOGO_ICON_SIZE };   // 아이콘 영역
        if (m_logoImage) m_renderer->DrawImage(*m_logoImage, logoIcon, LOGO_ICON_OPACITY);   // 프로그램 아이콘
        else                                                                                 // 아이콘을 못 읽으면 글자 로고
        {
            m_renderer->FillRoundedRect(logoIcon, LOGO_ICON_RADIUS, Theme::ACCENT_COLOR);   // 청록 정사각형
            m_renderer->DrawString(LOGO_ICON_TEXT, logoIcon, Theme::ACCENT_TEXT_COLOR, LOGO_ICON_STYLE);   // 아이콘 안 글자
        }

        const float logoTextLeft{ logoIcon.m_right + LOGO_TEXT_GAP };   // 로고 글자 왼쪽
        const float logoTextTop{ (TOP_BAR_HEIGHT - LOGO_TITLE_HEIGHT - LOGO_SUBTITLE_HEIGHT) * HALF };   // 두 줄을 세로 가운데에
        m_renderer->DrawString(LOGO_TITLE_TEXT, Core::RectF{ logoTextLeft, logoTextTop, SIDEBAR_WIDTH, logoTextTop + LOGO_TITLE_HEIGHT },
            Theme::HEADER_TEXT_COLOR, LOGO_TITLE_STYLE);   // "LOA KIT"
        m_renderer->DrawString(LOGO_SUBTITLE_TEXT, Core::RectF{ logoTextLeft, logoTextTop + LOGO_TITLE_HEIGHT, SIDEBAR_WIDTH,
            logoTextTop + LOGO_TITLE_HEIGHT + LOGO_SUBTITLE_HEIGHT }, Theme::MUTED_TEXT_COLOR, LOGO_SUBTITLE_STYLE);   // "로스트아크 멀티툴"
        m_renderer->DrawLine(0.0f, TOP_BAR_HEIGHT, SIDEBAR_WIDTH, TOP_BAR_HEIGHT, Theme::CARD_BORDER_COLOR, LINE_WIDTH);   // 로고 아래 경계선

        // ===== "TOOLS" 구역 이름 =====
        m_renderer->DrawString(SECTION_LABEL_TEXT, Core::RectF{ SECTION_LABEL_LEFT, SECTION_LABEL_TOP, SIDEBAR_WIDTH, SECTION_LABEL_TOP + SECTION_LABEL_HEIGHT },
            Theme::MUTED_TEXT_COLOR, SECTION_LABEL_STYLE);   // 구역 이름 (회색 작은 영문)

        // ===== 선택 표시 (청록 바탕 + 왼쪽 막대, m_indicatorPosition을 따라 미끄러지듯 이동, 검색으로 숨었으면 그리지 않음) =====
        const bool isActiveVisible{ std::ranges::find(m_visibleTools, m_activeIndex) != m_visibleTools.end() };   // 선택 도구가 목록에 있는지
        if (isActiveVisible)
        {
            const Core::RectF firstRect{ GetTabRect(0) };   // 첫 항목 영역 (선택 표시 위치 계산 기준)
            const float indicatorTop{ firstRect.m_top + m_indicatorPosition * (NAV_ITEM_HEIGHT + NAV_ITEM_GAP) };   // 선택 표시 위쪽
            const Core::RectF indicatorRect{ firstRect.m_left, indicatorTop, firstRect.m_right, indicatorTop + NAV_ITEM_HEIGHT };   // 선택 표시 영역
            m_renderer->FillRoundedRect(indicatorRect, NAV_ITEM_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, Theme::NAV_ACTIVE_ALPHA));   // 청록 바탕
            m_renderer->FillRect(Core::RectF{ indicatorRect.m_left, indicatorRect.m_top + NAV_ACCENT_INSET, indicatorRect.m_left + NAV_ACCENT_WIDTH,
                indicatorRect.m_bottom - NAV_ACCENT_INSET }, Theme::ACCENT_COLOR);   // 왼쪽 청록 막대
        }

        // 검색 결과가 없으면 안내 (해결 방법: Esc로 검색어 지우기)
        if (m_visibleTools.empty())
        {
            const Core::RectF firstRect{ GetTabRect(0) };   // 첫 항목 자리
            m_renderer->DrawString(TOOL_SEARCH_EMPTY_TEXT, Core::RectF{ firstRect.m_left, firstRect.m_top, firstRect.m_right, firstRect.m_top + NO_RESULT_HEIGHT },
                Theme::MUTED_TEXT_COLOR, INACTIVE_TAB_STYLE);   // 회색 안내
        }

        // ===== 탭 항목 (아이콘 + 제목, 선택 항목은 "›") — 검색어에 맞는 도구만 =====
        for (std::size_t position{}; position < m_visibleTools.size(); ++position)
        {
            const std::size_t index{ m_visibleTools[position] };   // 도구 번호
            const Core::RectF tabRect{ GetTabRect(position) };   // 항목 영역 (보이는 순서 기준)
            const bool isActive{ index == m_activeIndex };   // 선택한 탭인지

            if (!isActive && m_hoverIndex == index)
            {
                m_renderer->FillRoundedRect(tabRect, NAV_ITEM_RADIUS, Core::WithAlpha(Theme::ACCENT_COLOR, Theme::NAV_HOVER_ALPHA));   // 마우스를 올린 항목
            }

            const Core::Color& iconColor{ isActive ? Theme::ACCENT_COLOR : Theme::MUTED_TEXT_COLOR };   // 아이콘 색
            const Core::Color& textColor{ isActive ? Theme::HEADER_TEXT_COLOR : Theme::MUTED_TEXT_COLOR };   // 제목 색
            m_renderer->DrawString(TAB_ICONS[index], Core::RectF{ tabRect.m_left + NAV_ICON_LEFT, tabRect.m_top,
                tabRect.m_left + NAV_ICON_LEFT + NAV_ICON_WIDTH, tabRect.m_bottom }, iconColor, NAV_ICON_STYLE);   // 아이콘
            const float textRight{ tabRect.m_right - (isActive ? NAV_CHEVRON_WIDTH : NAV_TEXT_RIGHT_PADDING) };   // 꺾쇠는 선택 항목만 자리를 차지 (긴 제목이 잘리지 않게)
            m_renderer->DrawString(m_tools[index].get().GetTabTitle(), Core::RectF{ tabRect.m_left + NAV_TEXT_LEFT, tabRect.m_top,
                textRight, tabRect.m_bottom }, textColor, isActive ? ACTIVE_TAB_STYLE : INACTIVE_TAB_STYLE);   // 제목

            if (isActive)
            {
                m_renderer->DrawString(NAV_CHEVRON_TEXT, Core::RectF{ tabRect.m_right - NAV_CHEVRON_WIDTH, tabRect.m_top, tabRect.m_right, tabRect.m_bottom },
                    Theme::MUTED_TEXT_COLOR, NAV_CHEVRON_STYLE);   // 선택 항목 오른쪽 꺾쇠
            }
        }

        // ===== 아래: API 키 설정 버튼 + API 상태 줄 =====
        const float statusTop{ height - SIDEBAR_BOTTOM_PADDING - API_STATUS_HEIGHT };   // 상태 줄 위쪽
        const float buttonTop{ statusTop - API_BUTTON_GAP - API_BUTTON_HEIGHT };   // 버튼 위쪽
        const float reportTop{ buttonTop - API_BUTTON_GAP - API_BUTTON_HEIGHT };   // 문제 보고 버튼 위쪽
        const float overlayTop{ reportTop - API_BUTTON_GAP - API_BUTTON_HEIGHT };  // 인게임 오버레이 버튼 위쪽
        const float dividerY{ overlayTop - API_BUTTON_GAP };   // 아래 구역 경계선 위치 (버튼 세 개 위)
        m_renderer->DrawLine(0.0f, dividerY, SIDEBAR_WIDTH, dividerY, Theme::CARD_BORDER_COLOR, LINE_WIDTH);   // 아래 구역 경계선

        LayoutHeader();                     // 창 높이가 바뀌면 버튼이 따라 움직임
        m_headerHost.Render(*m_renderer);   // "API 키 설정" 버튼

        const Core::RectF statusRect{ SIDEBAR_PADDING, statusTop, SIDEBAR_WIDTH - SIDEBAR_PADDING, statusTop + API_STATUS_HEIGHT };   // 상태 줄 영역
        m_renderer->FillRoundedRect(statusRect, NAV_ITEM_RADIUS, Theme::CARD_BACK_COLOR);   // 상태 줄 바탕 (카드)
        m_renderer->DrawRoundedRect(statusRect, NAV_ITEM_RADIUS, Theme::CARD_BORDER_COLOR, Theme::BORDER_WIDTH);   // 상태 줄 테두리
        const Core::Color& statusColor{ m_hasAPIKey ? Theme::NOTICE_SUCCESS_COLOR : Theme::NOTICE_ERROR_COLOR };   // 키가 있으면 초록, 없으면 빨강
        const float statusMiddle{ statusTop + API_STATUS_HEIGHT * HALF };   // 상태 줄 세로 가운데
        m_renderer->FillEllipse(Core::PointF{ statusRect.m_left + API_STATUS_DOT_LEFT, statusMiddle },
            API_STATUS_DOT_RADIUS, API_STATUS_DOT_RADIUS, statusColor);   // 상태 점
        m_renderer->DrawString(m_hasAPIKey ? API_STATUS_READY_TEXT : API_STATUS_MISSING_TEXT,
            Core::RectF{ statusRect.m_left + API_STATUS_TEXT_LEFT, statusTop, statusRect.m_right, statusRect.m_bottom }, Theme::TEXT_COLOR, API_STATUS_STYLE);   // 상태 글자
    }

    void Application::RenderTopBar()
    {
        const float width{ m_renderer->GetWidth() };   // 창 너비
        m_renderer->FillRect(Core::RectF{ SIDEBAR_WIDTH, 0.0f, width, TOP_BAR_HEIGHT }, Theme::SIDEBAR_BACK_COLOR);   // 위 막대 바탕
        m_renderer->DrawLine(SIDEBAR_WIDTH, TOP_BAR_HEIGHT, width, TOP_BAR_HEIGHT, Theme::CARD_BORDER_COLOR, LINE_WIDTH);   // 위 막대 아래 경계선

        // 경로 표시: "멀티툴 › {탭 제목}"
        float left{ SIDEBAR_WIDTH + TOP_BAR_PADDING };   // 다음 글자를 그릴 왼쪽 위치
        const float right{ std::max(left, width - TOP_BAR_PADDING - TOP_STATUS_WIDTH) };   // 경로가 오른쪽 상태 글자를 가리지 않는 끝
        const float availableWidth{ right - left };   // 경로에 쓸 수 있는 너비
        const float rootWidth{ m_renderer->MeasureString(BREADCRUMB_ROOT_TEXT, BREADCRUMB_ROOT_STYLE, availableWidth).m_width };   // "멀티툴" 너비
        m_renderer->DrawString(BREADCRUMB_ROOT_TEXT, Core::RectF{ left, 0.0f, right, TOP_BAR_HEIGHT }, Theme::MUTED_TEXT_COLOR, BREADCRUMB_ROOT_STYLE);   // "멀티툴"
        left = std::min(right, left + rootWidth + BREADCRUMB_GAP);   // 구분 기호 자리로

        const float separatorWidth{ m_renderer->MeasureString(BREADCRUMB_SEPARATOR_TEXT, BREADCRUMB_ROOT_STYLE, availableWidth).m_width };   // "›" 너비
        m_renderer->DrawString(BREADCRUMB_SEPARATOR_TEXT, Core::RectF{ left, 0.0f, right, TOP_BAR_HEIGHT }, Theme::MUTED_TEXT_COLOR, BREADCRUMB_ROOT_STYLE);   // "›"
        left = std::min(right, left + separatorWidth + BREADCRUMB_GAP);   // 탭 제목 자리로

        m_renderer->DrawString(GetActiveTool().GetTabTitle(), Core::RectF{ left, 0.0f, right, TOP_BAR_HEIGHT },
            Theme::HEADER_TEXT_COLOR, BREADCRUMB_TITLE_STYLE);   // 현재 탭 제목

        // 오른쪽: 현재 버전
        const std::wstring versionText{ std::wstring{ VERSION_PREFIX_TEXT } + Core::TextEncoding::UTF8ToWide(Core::APPConstants::APP_VERSION) };   // "v1.5.0"
        m_renderer->DrawString(versionText, Core::RectF{ width - TOP_BAR_PADDING - VERSION_TEXT_WIDTH, 0.0f, width - TOP_BAR_PADDING, TOP_BAR_HEIGHT },
            Theme::MUTED_TEXT_COLOR, VERSION_STYLE);   // 회색 버전 글자
        m_renderer->DrawString(m_hasAPIKey ? TOP_API_READY_TEXT : TOP_API_MISSING_TEXT,
            Core::RectF{ width - TOP_BAR_PADDING - TOP_STATUS_WIDTH, 0.0f, width - TOP_BAR_PADDING - VERSION_TEXT_WIDTH, TOP_BAR_HEIGHT },
            m_hasAPIKey ? Theme::NOTICE_SUCCESS_COLOR : Theme::NOTICE_ERROR_COLOR, VERSION_STYLE);   // API 상태 (초록 · 빨강)
    }

    void Application::RenderPageHeader()
    {
        static_assert(TAB_TEXT_COUNT == TOOL_COUNT, "탭별 아이콘 · 머리말 문구 수가 탭 수와 달라요");   // 탭을 추가하면 문구도 함께 추가하도록 검사

        const float width{ m_renderer->GetWidth() };   // 창 너비
        const float headerTop{ TOP_BAR_HEIGHT };   // 머리말 위쪽 (위 막대 바로 아래)
        m_renderer->FillRect(Core::RectF{ SIDEBAR_WIDTH, headerTop, width, headerTop + PAGE_HEADER_HEIGHT }, Theme::BACKGROUND_COLOR);   // 탭 내용이 넘쳐도 머리말이 깨끗하게

        const Core::RectF column{ PageLayout::GetPageColumn(Core::RectF{ SIDEBAR_WIDTH + PAGE_HEADER_LEFT, headerTop,
            std::max(SIDEBAR_WIDTH + PAGE_HEADER_LEFT, width - PAGE_HEADER_LEFT), headerTop + PAGE_HEADER_HEIGHT }) };   // 본문과 같은 가운데 열
        const float left{ column.m_left };   // 머리말 글자 왼쪽
        const float right{ column.m_right };   // 머리말 글자 오른쪽
        m_renderer->DrawString(TAB_EYEBROWS[m_activeIndex], Core::RectF{ left, headerTop + PAGE_EYEBROW_TOP, right, headerTop + PAGE_EYEBROW_TOP + PAGE_EYEBROW_HEIGHT },
            Theme::ACCENT_COLOR, PAGE_EYEBROW_STYLE);   // 청록 영문 소제목
        m_renderer->DrawString(GetActiveTool().GetTabTitle(), Core::RectF{ left, headerTop + PAGE_TITLE_TOP, right, headerTop + PAGE_TITLE_TOP + PAGE_TITLE_HEIGHT },
            Theme::HEADER_TEXT_COLOR, PAGE_TITLE_STYLE);   // 큰 제목
        m_renderer->DrawString(TAB_DESCRIPTIONS[m_activeIndex], Core::RectF{ left, headerTop + PAGE_DESCRIPTION_TOP, right,
            headerTop + PAGE_DESCRIPTION_TOP + PAGE_DESCRIPTION_HEIGHT }, Theme::MUTED_TEXT_COLOR, PAGE_DESCRIPTION_STYLE);   // 회색 설명
    }

    // "API 키 설정" 버튼을 왼쪽 메뉴 아래(상태 줄 바로 위)에 배치
    void Application::LayoutHeader()
    {
        m_toolSearchBox->SetBounds(Core::RectF{ SIDEBAR_PADDING, SEARCH_BOX_TOP, SIDEBAR_WIDTH - SIDEBAR_PADDING, SEARCH_BOX_TOP + SEARCH_BOX_HEIGHT });   // 로고 아래 검색칸
        const float statusTop{ m_renderer->GetHeight() - SIDEBAR_BOTTOM_PADDING - API_STATUS_HEIGHT };   // 상태 줄 위쪽
        const float buttonTop{ statusTop - API_BUTTON_GAP - API_BUTTON_HEIGHT };   // 버튼 위쪽
        m_apiKeyButton->SetBounds(Core::RectF{ SIDEBAR_PADDING, buttonTop, SIDEBAR_WIDTH - SIDEBAR_PADDING, buttonTop + API_BUTTON_HEIGHT });   // 메뉴 너비에 맞춤
        const float reportTop{ buttonTop - API_BUTTON_GAP - API_BUTTON_HEIGHT };   // 문제 보고 버튼은 API 키 버튼 바로 위
        m_reportButton->SetBounds(Core::RectF{ SIDEBAR_PADDING, reportTop, SIDEBAR_WIDTH - SIDEBAR_PADDING, reportTop + API_BUTTON_HEIGHT });   // 같은 너비
        const float overlayTop{ reportTop - API_BUTTON_GAP - API_BUTTON_HEIGHT };   // 인게임 오버레이 버튼은 문제 보고 버튼 바로 위
        m_overlayButton->SetBounds(Core::RectF{ SIDEBAR_PADDING, overlayTop, SIDEBAR_WIDTH - SIDEBAR_PADDING, overlayTop + API_BUTTON_HEIGHT });   // 같은 너비
    }

    // 왼쪽 메뉴의 탭 항목 하나 (세로로 차례대로 쌓임)
    Core::RectF Application::GetTabRect(std::size_t index) const noexcept
    {
        const float itemTop{ NAV_TOP + static_cast<float>(index) * (NAV_ITEM_HEIGHT + NAV_ITEM_GAP) };   // 항목 위쪽
        return Core::RectF{ SIDEBAR_PADDING, itemTop, SIDEBAR_WIDTH - SIDEBAR_PADDING, itemTop + NAV_ITEM_HEIGHT };   // 메뉴 좌우 여백 안쪽
    }

    // 탭 내용 영역: 왼쪽 메뉴 · 위 막대 · 페이지 머리말을 뺀 나머지
    Core::RectF Application::GetContentRect() const noexcept
    {
        const float left{ SIDEBAR_WIDTH + CONTENT_INSET };   // 왼쪽 메뉴 오른쪽
        const float right{ std::max(left, m_renderer->GetWidth() - CONTENT_INSET) };   // 창 오른쪽 여백
        const float top{ TOP_BAR_HEIGHT + PAGE_HEADER_HEIGHT };   // 머리말 아래
        return PageLayout::GetPageColumn(Core::RectF{ left, top, right, std::max(top, m_renderer->GetHeight()) });   // 넓은 창에서는 가운데 정렬 열 (창이 아주 작아도 뒤집히지 않게)
    }

    Core::RectF Application::GetScreenRect() const noexcept
    {
        return Core::RectF{ 0.0f, 0.0f, m_renderer->GetWidth(), m_renderer->GetHeight() };
    }

    std::optional<std::size_t> Application::FindTabAt(float x, float y) const noexcept
    {
        for (std::size_t position{}; position < m_visibleTools.size(); ++position)
        {
            if (GetTabRect(position).Contains(x, y)) return m_visibleTools[position];   // 보이는 순서 → 도구 번호
        }
        return std::nullopt;
    }

    // 왼쪽 메뉴 · 위 막대 · 페이지 머리말 안이면 화면 틀 (탭 내용으로 입력을 보내지 않음)
    bool Application::IsInHeader(float x, float y) const noexcept
    {
        return x < SIDEBAR_WIDTH || y < TOP_BAR_HEIGHT + PAGE_HEADER_HEIGHT;   // 메뉴 너비 안이거나 머리말 아래 경계보다 위
    }

    Core::ToolManager& Application::GetActiveTool() const noexcept
    {
        return m_tools[m_activeIndex].get();   // m_activeIndex는 SelectTool에서 TOOL_COUNT 미만만 허용
    }

    // 메인 창에 그리고 입력을 넘길 탭 (오버레이 미니 모드로 띄운 탭이면 안내 탭 → 같은 탭을 두 창이 동시에 쓰지 않음)
    Core::ToolManager& Application::GetVisibleTool() const noexcept
    {
        if (OverlayManager::GetInstance().GetMiniToolIndex() == m_activeIndex) return m_overlayPlaceholder;   // 오버레이에서 사용 중
        return GetActiveTool();                                                                              // 그대로
    }
}