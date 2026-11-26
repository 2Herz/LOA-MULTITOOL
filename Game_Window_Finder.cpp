module;  // 전역 모듈 조각 시작 (윈도우 헤더는 모듈 밖에서 포함)

#define NOMINMAX                 // min · max 매크로 끄기
#define WIN32_LEAN_AND_MEAN      // 잘 안 쓰는 윈도우 헤더 빼기
#include <Windows.h>             // EnumWindows · GetWindowTextW · GetClientRect
#include <TlHelp32.h>            // CreateToolhelp32Snapshot (프로세스 이름 목록, 프로세스를 열지 않음)

module LostArkMultiTool.Vision.GameWindowFinder;  // 게임 창 찾기 모듈 구현

import std;  // 표준 라이브러리 모듈 가져오기
import LostArkMultiTool.Vision.FrameImage;  // 픽셀 사각형 가져오기

namespace LostArkMultiTool::Vision  // 화면 인식 네임스페이스 시작
{
    namespace  // 이 파일 전용 상수 · 도우미
    {
        constexpr std::wstring_view GAME_TITLE_PREFIX{ L"LOST ARK" };       // 게임 창 제목 시작 글자 ("LOST ARK (64-bit, DX11) v...")
        constexpr std::wstring_view GAME_PROCESS_NAME{ L"LOSTARK.exe" };    // 게임 실행 파일 이름
        constexpr int TITLE_BUFFER_LENGTH{ 256 };                           // 창 제목 버퍼 길이 (글자 수)

        // 스냅샷 핸들을 범위 안에서만 유지 (RAII)
        class SnapshotHandle
        {
        public:
            explicit SnapshotHandle(HANDLE handle) noexcept : m_handle{ handle } {}   // 핸들 보관
            ~SnapshotHandle() { if (IsValid()) ::CloseHandle(m_handle); }            // 닫기
            SnapshotHandle(const SnapshotHandle&) = delete;                           // 복사 금지
            SnapshotHandle& operator=(const SnapshotHandle&) = delete;                // 복사 대입 금지
            [[nodiscard]] bool IsValid() const noexcept { return m_handle != INVALID_HANDLE_VALUE && m_handle != nullptr; }   // 유효한지
            [[nodiscard]] HANDLE Get() const noexcept { return m_handle; }            // 핸들
        private:
            HANDLE m_handle;   // 스냅샷 핸들
        };

        // 대소문자를 무시하고 같은 이름인지
        [[nodiscard]] bool IsSameName(std::wstring_view a, std::wstring_view b) noexcept
        {
            return a.size() == b.size() && std::ranges::equal(a, b, [](wchar_t left, wchar_t right)
                { return std::towlower(left) == std::towlower(right); });   // 글자마다 소문자로 비교
        }

        // 게임 프로세스 번호들 (프로세스 목록 스냅샷에서 이름만 봄 → 게임 프로세스를 열지 않음)
        [[nodiscard]] std::vector<DWORD> FindGameProcessIds()
        {
            std::vector<DWORD> ids;                                                    // 결과
            const SnapshotHandle snapshot{ ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };   // 프로세스 목록
            if (!snapshot.IsValid()) return ids;                                       // 목록을 못 얻음
            PROCESSENTRY32W entry{};                                                   // 프로세스 한 줄
            entry.dwSize = sizeof(entry);                                              // 구조체 크기
            for (BOOL hasEntry = ::Process32FirstW(snapshot.Get(), &entry); hasEntry; hasEntry = ::Process32NextW(snapshot.Get(), &entry))   // 목록 훑기
                if (IsSameName(entry.szExeFile, GAME_PROCESS_NAME)) ids.push_back(entry.th32ProcessID);   // 게임 프로세스 번호 모음
            return ids;                                                                // 번호들
        }

        // 창 핸들 → 정보 (보이지 않거나 크기가 0이면 nullopt)
        [[nodiscard]] std::optional<GameWindowInfo> ReadInfo(HWND window) noexcept
        {
            if (!::IsWindow(window) || !::IsWindowVisible(window)) return std::nullopt;   // 사라졌거나 숨은 창

            GameWindowInfo info;                                        // 결과
            info.m_handle = window;                                     // 핸들
            info.m_isMinimized = ::IsIconic(window) != FALSE;           // 최소화 여부
            info.m_isForeground = ::GetForegroundWindow() == window;    // 맨 앞 여부

            RECT client{};                                              // 화면 영역 (창 기준)
            if (!::GetClientRect(window, &client)) return std::nullopt; // 크기를 못 읽음
            POINT origin{ client.left, client.top };                    // 화면 영역 왼쪽 위
            if (!::ClientToScreen(window, &origin)) return std::nullopt;// 화면 좌표로 못 바꿈
            info.m_clientArea = PixelRect{ origin.x, origin.y,
                client.right - client.left, client.bottom - client.top };   // 화면 좌표 영역
            return info;                                                // 정보
        }

        // 창 하나가 게임 창인지 (게임 프로세스의 창이거나, 제목이 게임 제목으로 시작)
        [[nodiscard]] bool IsGameWindow(HWND window, std::span<const DWORD> gameProcessIds) noexcept
        {
            DWORD processId = 0;                                        // 창을 만든 프로세스 번호
            ::GetWindowThreadProcessId(window, &processId);             // 번호 읽기 (프로세스를 열지 않음)
            if (processId == ::GetCurrentProcessId()) return false;    // 내 프로그램 창은 제외
            if (::GetWindow(window, GW_OWNER) != nullptr) return false;// 다른 창에 딸린 창(대화상자 등) 제외
            if (std::ranges::contains(gameProcessIds, processId)) return true;   // 게임 프로세스의 창

            std::array<wchar_t, TITLE_BUFFER_LENGTH> title{};           // 제목 버퍼
            const int length = ::GetWindowTextW(window, title.data(), static_cast<int>(title.size()));   // 제목 읽기
            const std::wstring_view text{ title.data(), static_cast<std::size_t>(std::max(length, 0)) };  // 제목 글자
            return text.starts_with(GAME_TITLE_PREFIX);                 // 게임 제목으로 시작하는지
        }

        // EnumWindows에 넘길 검색 상태
        struct SearchState
        {
            std::vector<DWORD> m_gameProcessIds;      // 게임 프로세스 번호들
            std::optional<GameWindowInfo> m_best;     // 지금까지 고른 가장 큰 창
        };

        // EnumWindows 콜백 (가장 큰 게임 창을 고름)
        BOOL CALLBACK CollectGameWindow(HWND window, LPARAM parameter) noexcept
        {
            auto* state = reinterpret_cast<SearchState*>(parameter);                      // 검색 상태
            if (!IsGameWindow(window, state->m_gameProcessIds)) return TRUE;              // 게임 창이 아니면 다음
            const auto info = ReadInfo(window);                                           // 정보
            if (!info) return TRUE;                                                       // 못 읽으면 다음
            const auto area = [](const GameWindowInfo& value)                             // 넓이 계산
                { return static_cast<long long>(value.m_clientArea.m_width) * value.m_clientArea.m_height; };
            if (!state->m_best || area(*info) > area(*state->m_best)) state->m_best = info;   // 더 크면 교체
            return TRUE;                                                                  // 계속 훑기
        }
    }

    std::optional<GameWindowInfo> GameWindowFinder::Find()
    {
        SearchState state{ FindGameProcessIds() };                                         // 검색 상태 (게임 프로세스 번호 준비)
        ::EnumWindows(&CollectGameWindow, reinterpret_cast<LPARAM>(&state));               // 최상위 창 훑기
        return state.m_best;                                                               // 가장 큰 게임 창
    }

    std::optional<GameWindowInfo> GameWindowFinder::Refresh(void* handle)
    {
        const HWND window = static_cast<HWND>(handle);                    // 핸들 형 변환
        const auto gameProcessIds = FindGameProcessIds();                 // 게임 프로세스 번호들
        if (!IsGameWindow(window, gameProcessIds)) return std::nullopt;   // 더 이상 게임 창이 아님
        return ReadInfo(window);                                          // 최신 정보
    }

    std::optional<PixelPoint> GameWindowFinder::GetCursorPosition() noexcept
    {
        POINT cursor{};                                           // 커서 위치
        if (!::GetPhysicalCursorPos(&cursor)) return std::nullopt;// 물리 픽셀 좌표로 읽기 (캡처 · 창 영역과 같은 단위)
        return PixelPoint{ cursor.x, cursor.y };                  // 위치
    }
}
