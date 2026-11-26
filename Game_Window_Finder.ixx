export module LostArkMultiTool.Vision.GameWindowFinder;  // 게임 창 찾기 모듈 선언 (내보내기)

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기

// 로스트아크 게임 창 찾기
// 안전: 게임 프로세스를 열지 않음 (OpenProcess · 메모리 읽기 없음). 프로세스 목록의 이름 · 창 제목 · 위치만 윈도우에 물어봄
export namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 내보내기 시작
{
    // 찾은 게임 창 정보
    struct GameWindowInfo
    {
        void* m_handle{};             // 창 핸들 (HWND)
        PixelRect m_clientArea;       // 게임 화면 영역 (화면 좌표, 물리 픽셀)
        bool m_isForeground{};        // 지금 맨 앞(입력 받는 창)인지
        bool m_isMinimized{};         // 최소화돼 있는지
    };

    // 게임 창 찾기 (상태 없음)
    class GameWindowFinder
    {
    public:
        // 보이는 최상위 창 중 LOSTARK.exe의 창이거나 제목이 "LOST ARK"로 시작하는 가장 큰 창 (내 프로그램 창은 제외, 없으면 nullopt)
        [[nodiscard]] static std::optional<GameWindowInfo> Find();

        // 이미 찾은 창 핸들의 최신 정보 (창이 사라졌으면 nullopt)
        [[nodiscard]] static std::optional<GameWindowInfo> Refresh(void* handle);

        // 마우스 커서 위치 (화면 좌표, 물리 픽셀, 못 읽으면 nullopt) — 커서를 읽기만 하고 움직이지 않음
        [[nodiscard]] static std::optional<PixelPoint> GetCursorPosition() noexcept;
    };
}
