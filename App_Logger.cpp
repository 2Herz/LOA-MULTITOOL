module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>   // ShellExecuteW

module LostArkMultiTool.Core.AppLogger;

import std;
import LostArkMultiTool.Core.APPConstants;

namespace LostArkMultiTool::Core
{
    namespace
    {
        constexpr std::string_view LOG_FOLDER_NAME{ "Logs" };                 // 사용자 데이터 폴더 아래 로그 폴더
        constexpr std::string_view LOG_FILE_STEM{ "app" };                    // app.log / app.1.log …
        constexpr std::string_view LOG_FILE_EXTENSION{ ".log" };              // 로그 파일 확장자
        constexpr std::uintmax_t BYTES_PER_KILOBYTE{ 1024 };                  // 1KB
        constexpr std::uintmax_t MAX_LOG_FILE_BYTES{ BYTES_PER_KILOBYTE * BYTES_PER_KILOBYTE };   // 1MB를 넘으면 다음 파일로 밀어냄
        constexpr int KEPT_OLD_LOG_COUNT{ 3 };                                // app.1.log ~ app.3.log까지 보관
        constexpr int CURRENT_LOG_INDEX{ 0 };                                 // 0번 = 지금 쓰는 app.log
        constexpr std::string_view LEVEL_INFO_TEXT{ "INFO" };                 // 정보 표시
        constexpr std::string_view LEVEL_WARNING_TEXT{ "WARN" };              // 경고 표시
        constexpr std::string_view LEVEL_ERROR_TEXT{ "ERROR" };               // 오류 표시
        constexpr INT_PTR SHELL_EXECUTE_ERROR_LIMIT{ 32 };                    // ShellExecute 반환값이 이 이하면 실패

        // 중요도 → 파일에 적을 글자
        [[nodiscard]] std::string_view GetLevelText(LogLevel level) noexcept
        {
            switch (level)
            {
                case LogLevel::LOG_WARNING: return LEVEL_WARNING_TEXT;   // 경고
                case LogLevel::LOG_ERROR:   return LEVEL_ERROR_TEXT;     // 오류
                default:                    return LEVEL_INFO_TEXT;      // 정보
            }
        }

        // UTF-8 문자열 → 경로 (한글 이름도 깨지지 않게)
        [[nodiscard]] std::filesystem::path ToPath(std::string_view utf8Text)
        {
            return std::filesystem::path{ std::u8string{ utf8Text.begin(), utf8Text.end() } };   // char → char8_t 복사
        }

        // 순번이 붙은 로그 파일 경로 (0 = app.log, n = app.n.log)
        [[nodiscard]] std::filesystem::path MakeLogPath(const std::filesystem::path& directory, int index)
        {
            const std::string fileName{ index == CURRENT_LOG_INDEX
                ? std::format("{}{}", LOG_FILE_STEM, LOG_FILE_EXTENSION)                // 현재 파일
                : std::format("{}.{}{}", LOG_FILE_STEM, index, LOG_FILE_EXTENSION) };   // 지난 파일
            return directory / ToPath(fileName);                                        // 폴더 + 파일 이름
        }

        // 로컬 시각 "yyyy-MM-dd HH:mm:ss.fff" (로그 줄 머리)
        [[nodiscard]] std::string MakeLogTime()
        {
            SYSTEMTIME time{};             // 지금 로컬 시각
            ::GetLocalTime(&time);         // 밀리초까지 채워짐
            return std::format("{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}", time.wYear, time.wMonth, time.wDay,
                time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);   // 사람이 읽기 쉬운 형식
        }
    }

    void AppLogger::Write(LogLevel level, std::string_view message) noexcept
    {
        try
        {
            const std::scoped_lock lock{ m_mutex };                                              // 한 번에 한 스레드만 기록
            const std::filesystem::path logPath{ MakeLogPath(GetLogDirectory(), CURRENT_LOG_INDEX) };   // 현재 로그 파일
            RotateIfNeeded(logPath);                                                             // 너무 크면 먼저 밀어냄

            std::ofstream file{ logPath, std::ios::binary | std::ios::app };                     // 이어 쓰기 (UTF-8 그대로)
            if (!file) return;                                                                   // 열 수 없으면 조용히 포기
            file << std::format("{} [{}] [T{}] {}\r\n", MakeLogTime(), GetLevelText(level),
                ::GetCurrentThreadId(), message);                                                // 시각 · 중요도 · 스레드 · 내용
        }
        catch (...)
        {
            // 로그 실패가 프로그램을 멈추게 하지 않음 (디스크 가득 · 권한 문제 등)
        }
    }

    void AppLogger::Info(std::string_view message) noexcept
    {
        Write(LogLevel::LOG_INFO, message);      // 정보
    }

    void AppLogger::Warning(std::string_view message) noexcept
    {
        Write(LogLevel::LOG_WARNING, message);   // 경고
    }

    void AppLogger::Error(std::string_view message) noexcept
    {
        Write(LogLevel::LOG_ERROR, message);     // 오류
    }

    std::filesystem::path AppLogger::GetLogDirectory()
    {
        const std::filesystem::path directory{ APPConstants::GetUserDataFilePath(LOG_FOLDER_NAME) };   // %LOCALAPPDATA%\LostArkMultiTool\Logs
        std::error_code error;                                              // 만들기 실패는 예외 대신 무시 (쓰기에서 다시 실패함)
        std::filesystem::create_directories(directory, error);              // 없으면 만듦
        return directory;                                                   // 로그 폴더
    }

    bool AppLogger::OpenLogDirectory() noexcept
    {
        try
        {
            const std::wstring directory{ GetLogDirectory().wstring() };    // 폴더 경로 (UTF-16)
            const auto result{ reinterpret_cast<INT_PTR>(::ShellExecuteW(nullptr, L"explore", directory.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) };   // 탐색기로 열기
            return result > SHELL_EXECUTE_ERROR_LIMIT;                      // 32 초과면 성공
        }
        catch (...)
        {
            return false;                                                   // 경로 계산 실패
        }
    }

    void AppLogger::RotateIfNeeded(const std::filesystem::path& logPath) noexcept
    {
        std::error_code error;                                              // 파일 작업 실패는 무시 (다음 기록에서 다시 시도)
        const std::uintmax_t size{ std::filesystem::file_size(logPath, error) };   // 현재 크기
        if (error || size < MAX_LOG_FILE_BYTES) return;                     // 아직 작거나 파일이 없음

        const std::filesystem::path directory{ logPath.parent_path() };     // 같은 폴더 안에서 밀어냄
        std::filesystem::remove(MakeLogPath(directory, KEPT_OLD_LOG_COUNT), error);   // 가장 오래된 파일 삭제
        for (int index{ KEPT_OLD_LOG_COUNT - 1 }; index >= CURRENT_LOG_INDEX; --index)   // app.2 → app.3, app.1 → app.2, app → app.1
        {
            std::filesystem::rename(MakeLogPath(directory, index), MakeLogPath(directory, index + 1), error);   // 한 칸씩 밀기
        }
    }
}
