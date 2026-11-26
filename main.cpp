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
import LostArkMultiTool.Vision.FrameImage;            // 캡처 프레임
import LostArkMultiTool.Vision.ImageFile;             // PNG 저장
import LostArkMultiTool.Vision.ScreenLayout;          // 템플릿 폴더 위치
import LostArkMultiTool.Vision.ScreenWatcher;         // 게임 화면 한 장 캡처

namespace LostArkMultiTool::APP
{
    namespace
    {
        constexpr int EXIT_CODE_FAILURE{ 1 };   // 처리하지 못한 오류로 종료

        constexpr const wchar_t* FATAL_ERROR_CAPTION{ L"로스트아크 멀티툴 - 오류" };
        constexpr const wchar_t* UNKNOWN_ERROR_MESSAGE{ L"알 수 없는 오류가 발생했어요!" };
        constexpr std::wstring_view SELF_TEST_REPORT_NAME{ L"self_test_report.txt" };   // 자체 검사 보고서 (exe 옆)
        constexpr std::wstring_view CAPTURE_GAME_ARGUMENT{ L"--capture-game" };          // 게임 화면 한 장 저장 인자 (템플릿 · 교정 샘플용)
        constexpr std::wstring_view CAPTURE_FOLDER_NAME{ L"Captures" };                  // 저장 폴더 (Vision 폴더 아래)
        constexpr std::chrono::milliseconds CAPTURE_TIMEOUT{ 3000 };                     // 첫 프레임을 기다릴 최대 시간
        constexpr const wchar_t* CAPTURE_CAPTION{ L"로스트아크 멀티툴 - 게임 화면 저장" }; // 결과 창 제목

        // 명령줄에 게임 화면 저장 인자가 있는지
        [[nodiscard]] bool IsCaptureRequested(std::wstring_view commandLine) noexcept
        {
            return commandLine.find(CAPTURE_GAME_ARGUMENT) != std::wstring_view::npos;   // 인자 포함 여부
        }

        // 게임 화면(제목 표시줄 제외) 한 장을 PNG로 저장하고 결과를 창으로 알림 → 종료 코드 (0 = 성공)
        [[nodiscard]] int CaptureGameScreen()
        {
            std::string error;                                                                       // 실패 이유 (UTF-8)
            const auto frame = Vision::ScreenWatcher::CaptureOnce(CAPTURE_TIMEOUT, error);           // 한 장 캡처
            if (!frame)                                                                              // 실패
            {
                const std::wstring message{ Core::TextEncoding::UTF8ToWide(error) };                 // 안내 글자
                ::MessageBoxW(nullptr, message.c_str(), CAPTURE_CAPTION, MB_OK | MB_ICONWARNING);   // 알림
                return EXIT_CODE_FAILURE;                                                            // 실패 코드
            }
            const auto now = std::chrono::zoned_time{ std::chrono::current_zone(),
                std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()) };        // 지금 시각 (현지)
            const std::filesystem::path path = Vision::ScreenLayoutStore::GetTemplateFolder() / CAPTURE_FOLDER_NAME
                / std::format(L"GameCapture_{:%Y%m%d_%H%M%S}.png", now);                            // 저장 경로
            if (!Vision::ImageFile::SavePNG(path, *frame)) return EXIT_CODE_FAILURE;                 // 저장 실패
            const std::wstring message{ std::format(L"{} x {} 화면을 저장했어요.\n{}", frame->GetWidth(), frame->GetHeight(), path.wstring()) };   // 안내 글자
            ::MessageBoxW(nullptr, message.c_str(), CAPTURE_CAPTION, MB_OK | MB_ICONINFORMATION);   // 알림
            return 0;                                                                                // 성공
        }

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

    // 화면 인식 준비용: 게임 화면 한 장을 저장하고 바로 종료 (템플릿 · 교정 샘플 만들기)
    if (APP::IsCaptureRequested(::GetCommandLineW()))
    {
        try
        {
            return APP::CaptureGameScreen();   // 0 = 저장 성공
        }
        catch (...)
        {
            return APP::EXIT_CODE_FAILURE;     // 예외로 실패
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
