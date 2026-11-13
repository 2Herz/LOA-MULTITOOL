export module LostArkMultiTool.Core.AppLogger;

import std;
import LostArkMultiTool.Core.Singleton;

export namespace LostArkMultiTool::Core
{
    // 로그 한 줄의 중요도 (파일에 [INFO] / [WARN] / [ERROR]로 남음)
    enum class LogLevel
    {
        LOG_INFO,      // 실행 · 업데이트 같은 흐름 기록
        LOG_WARNING,   // 계속 쓸 수는 있지만 확인이 필요한 일 (원격 설정 실패 · API 형식 변경 등)
        LOG_ERROR      // 처리하지 못한 오류 · 크래시
    };

    // 진단용 순환 로그 파일 (사용자 데이터 폴더\Logs\app.log, 일정 크기를 넘으면 app.1.log ~ 로 밀어냄)
    // 여러 스레드(백그라운드 조회 · 화면)에서 같이 쓰므로 내부에서 잠금 / 로그 실패는 프로그램을 멈추지 않음
    class AppLogger final : public Singleton<AppLogger>
    {
    public:
        // 한 줄 기록 (message는 UTF-8, 시각 · 중요도 · 스레드 번호를 앞에 붙임)
        void Write(LogLevel level, std::string_view message) noexcept;

        // 자주 쓰는 중요도별 줄임
        void Info(std::string_view message) noexcept;
        void Warning(std::string_view message) noexcept;
        void Error(std::string_view message) noexcept;

        // 로그 · 크래시 덤프를 모아 두는 폴더 (없으면 만듦)
        [[nodiscard]] static std::filesystem::path GetLogDirectory();

        // 탐색기로 로그 폴더 열기 ([문제 보고] 버튼, 실패하면 false)
        [[nodiscard]] static bool OpenLogDirectory() noexcept;

    private:
        friend class Singleton<AppLogger>;   // GetInstance만 생성할 수 있게
        AppLogger() = default;               // 생성은 GetInstance에서만
        ~AppLogger() = default;              // 프로그램 종료 때 자동 소멸

        // 현재 파일이 너무 커졌으면 app.log → app.1.log → app.2.log … 로 밀어냄 (잠근 상태에서 호출)
        void RotateIfNeeded(const std::filesystem::path& logPath) noexcept;

        std::mutex m_mutex;   // 여러 스레드가 동시에 같은 파일에 쓰지 않게
    };
}
