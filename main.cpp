#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

import std;
import LostArkMultiTool.Core.TextEncoding;
import LostArkMultiTool.Views.Application;
                                                                            L
namespace LostArkMultiTool::App
{
    namespace
    {
        constexpr int EXIT_CODE_FAILURE{ 1 };   // 처리하지 못한 오류로 종료

        constexpr const wchar_t* FATAL_ERROR_CAPTION{ L"로스트아크 멀티툴 - 오류" };
        constexpr const wchar_t* UNKNOWN_ERROR_MESSAGE{ L"알 수 없는 오류가 발생했어요!" };

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
    }
}

// GUI 하위 시스템(/SUBSYSTEM:WINDOWS)의 진입점
int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int showCommand)
{
    using namespace LostArkMultiTool;

    try
    {
        Views::Application application;
        return application.Run(showCommand);
    }
    catch (const std::exception& exception)
    {
        App::ShowFatalError(exception);
    }
    catch (...)
    {
        App::ShowFatalError(App::UNKNOWN_ERROR_MESSAGE);
    }
    return App::EXIT_CODE_FAILURE;
}