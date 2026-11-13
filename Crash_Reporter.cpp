module;

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>                       // MiniDumpWriteDump
#pragma comment(lib, "dbghelp.lib")        // MiniDumpWriteDump 구현

module LostArkMultiTool.Core.CrashReporter;

import std;
import LostArkMultiTool.Core.AppLogger;
import LostArkMultiTool.Core.LocalTime;

namespace LostArkMultiTool::Core::CrashReporter
{
    namespace
    {
        constexpr std::string_view DUMP_FILE_PREFIX{ "crash_" };          // crash_yyyyMMdd_HHmmss.dmp
        constexpr std::string_view DUMP_FILE_EXTENSION{ ".dmp" };         // 덤프 확장자
        constexpr DWORD NO_EXCEPTION_CODE{ 0 };                           // 예외 정보가 없을 때 코드
        constexpr std::string_view TERMINATE_MESSAGE{ "std::terminate 호출 (처리하지 못한 C++ 예외 또는 noexcept 위반)" };

        // 스레드 정보 + 스택 근처 메모리만 (파일 크기를 작게 유지)
        constexpr MINIDUMP_TYPE DUMP_TYPE{ static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory) };

        std::once_flag g_installFlag;                  // 처리기는 한 번만 등록
        std::terminate_handler g_previousTerminate{};  // 원래 terminate 처리기 (덤프 뒤 이어서 호출)

        // 처리하지 못한 SEH 예외 (접근 위반 등) → 로그 + 덤프 후 기본 처리(오류 보고 · 종료)로 넘김
        LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* exceptionPointers)
        {
            const DWORD code{ exceptionPointers != nullptr && exceptionPointers->ExceptionRecord != nullptr
                ? exceptionPointers->ExceptionRecord->ExceptionCode : NO_EXCEPTION_CODE };    // 예외 코드
            AppLogger::GetInstance().Error(std::format("처리하지 못한 예외 0x{:08X}", code));   // 무엇이 났는지 먼저 기록
            (void)WriteDump(exceptionPointers);                                                 // 덤프 남기기
            return EXCEPTION_CONTINUE_SEARCH;                                                   // 기본 처리 계속
        }

        // std::terminate → 로그 + 덤프 후 원래 처리기(abort)
        [[noreturn]] void OnTerminate()
        {
            AppLogger::GetInstance().Error(TERMINATE_MESSAGE);           // 원인 기록
            (void)WriteDump(nullptr);                                    // 현재 스레드 덤프
            if (g_previousTerminate != nullptr) g_previousTerminate();   // 원래 처리기
            std::abort();                                                // 원래 처리기가 돌아오면 강제 종료
        }
    }

    void Install() noexcept
    {
        std::call_once(g_installFlag, []
        {
            ::SetUnhandledExceptionFilter(&OnUnhandledException);       // SEH 예외
            g_previousTerminate = std::set_terminate(&OnTerminate);    // C++ terminate
        });
    }

    std::optional<std::filesystem::path> WriteDump(void* exceptionPointers) noexcept
    {
        try
        {
            const std::string fileName{ std::format("{}{}{}", DUMP_FILE_PREFIX, LocalTime::MakeTimestamp(), DUMP_FILE_EXTENSION) };   // 시각별 파일 이름
            const std::filesystem::path dumpPath{ AppLogger::GetLogDirectory() / std::filesystem::path{ std::u8string{ fileName.begin(), fileName.end() } } };   // 로그 폴더 안

            const HANDLE file{ ::CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr) };   // 덤프 파일 만들기
            if (file == INVALID_HANDLE_VALUE) return std::nullopt;                               // 만들 수 없음

            MINIDUMP_EXCEPTION_INFORMATION exceptionInfo{};                                      // 예외 위치 정보
            exceptionInfo.ThreadId = ::GetCurrentThreadId();                                     // 예외가 난 스레드
            exceptionInfo.ExceptionPointers = static_cast<EXCEPTION_POINTERS*>(exceptionPointers);   // 예외 레코드 · 문맥
            exceptionInfo.ClientPointers = FALSE;                                                // 같은 프로세스 주소

            const BOOL isWritten{ ::MiniDumpWriteDump(::GetCurrentProcess(), ::GetCurrentProcessId(), file, DUMP_TYPE,
                exceptionPointers != nullptr ? &exceptionInfo : nullptr, nullptr, nullptr) };   // 덤프 쓰기
            ::CloseHandle(file);                                                                 // 핸들 닫기 (누수 방지)

            if (isWritten == FALSE) return std::nullopt;                                         // 쓰기 실패
            AppLogger::GetInstance().Error(std::format("크래시 덤프 저장: {}", fileName));       // 덤프 이름 기록
            return dumpPath;                                                                     // 만든 파일
        }
        catch (...)
        {
            return std::nullopt;   // 경로 계산 등 실패 → 덤프 없이 진행
        }
    }
}
