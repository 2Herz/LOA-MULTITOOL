#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

import std;
import LostArkMultiTool.Core.APPConstants;
import LostArkMultiTool.Core.AppLogger;
import LostArkMultiTool.Core.CrashReporter;
import LostArkMultiTool.Core.SelfTest;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Views.Application;

namespace LostArkMultiTool::APP
{
    namespace
    {
        constexpr int EXIT_CODE_FAILURE{ 1 };   // 처리하지 못한 오류로 종료

        constexpr const wchar_t* FATAL_ERROR_CAPTION{ L"로스트아크 멀티툴 - 오류" };
        constexpr const wchar_t* UNKNOWN_ERROR_MESSAGE{ L"알 수 없는 오류가 발생했어요!" };
        constexpr std::wstring_view SELF_TEST_REPORT_NAME{ L"self_test_report.txt" };   // 자체 검사 보고서 (exe 옆)

        void ShowFatalError(const wchar_t* message) noexcept
        {
            ::MessageBoxW(nullptr, message, FATAL_ERROR_CAPTION, MB_OK | MB_ICONERROR);
        }

        // 예외 메시지(UTF-8)를 오류 창(UTF-16)으로 표시 (변환까지 실패하면 기본 문구)
        void ShowFatalError(const std::exception& exception) noexcept
        {
            try
            {
                const std::wstring message{ Core::TextEncoding::UTF8ToWide(exception.what()) };
                ShowFatalError(message.c_str());
            }
            catch (...)
            {
                ShowFatalError(UNKNOWN_ERROR_MESSAGE);
            }
        }

        // 지금 실행 중인 exe가 있는 폴더 (자체 검사 보고서 위치)
        [[nodiscard]] std::filesystem::path GetExecutableDirectory()
        {
            std::wstring buffer(MAX_PATH, L'\0');                                                   // 처음 크기
            for (;;)
            {
                const DWORD length{ ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size())) };   // exe 경로
                if (length < buffer.size()) { buffer.resize(length); break; }                       // 다 들어감
                buffer.resize(buffer.size() * 2);                                                   // 잘렸으면 두 배로
            }
            return std::filesystem::path{ buffer }.parent_path();                                   // 폴더만
        }
    }
}

// GUI 하위 시스템(/SUBSYSTEM:WINDOWS)의 진입점
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int showCommand)
{
    using namespace LostArkMultiTool;

    Core::CrashReporter::Install();   // 처리하지 못한 예외 · terminate → 크래시 덤프 + 로그 (가장 먼저)

    // 빌드 스크립트용 자체 검사: 화면 없이 데이터 표를 검사하고 실패 수를 종료 코드로 돌려줌
    if (Core::SelfTest::IsRequested(::GetCommandLineW()))
    {
        try
        {
            return Core::SelfTest::Run(APP::GetExecutableDirectory() / APP::SELF_TEST_REPORT_NAME);   // 0 = 통과
        }
        catch (...)
        {
            return APP::EXIT_CODE_FAILURE;   // 보고서조차 못 씀
        }
    }

    Core::AppLogger::GetInstance().Info(std::format("시작 v{}", Core::APPConstants::APP_VERSION));   // 실행 기록 (문제 보고 때 버전 확인)

    try
    {
        Views::Application application;
        const int exitCode{ application.Run(showCommand) };                                      // 메시지 루프
        Core::AppLogger::GetInstance().Info(std::format("정상 종료 (코드 {})", exitCode));         // 종료 기록
        return exitCode;
    }
    catch (const std::exception& exception)
    {
        Core::AppLogger::GetInstance().Error(std::format("치명 오류: {}", exception.what()));     // 원인 기록
        APP::ShowFatalError(exception);
    }
    catch (...)
    {
        Core::AppLogger::GetInstance().Error("치명 오류: 알 수 없는 예외");                       // 원인 기록
        APP::ShowFatalError(APP::UNKNOWN_ERROR_MESSAGE);
    }
    return APP::EXIT_CODE_FAILURE;
}
