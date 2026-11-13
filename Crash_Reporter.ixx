export module LostArkMultiTool.Core.CrashReporter;

import std;

// 처리하지 못한 예외 · 강제 종료(terminate) 때 크래시 덤프(.dmp)를 로그 폴더에 남기는 도구
// 사용자 PC에서 프로그램이 죽어도 덤프 + app.log로 원인을 찾을 수 있게 함
export namespace LostArkMultiTool::Core::CrashReporter
{
    // 프로그램 시작 때 한 번 호출 (처리기 등록, 여러 번 불러도 한 번만 등록)
    void Install() noexcept;

    // 지금 시점의 덤프를 남김 (exceptionPointers = EXCEPTION_POINTERS*, 없으면 nullptr / 만든 파일 경로 · 실패면 nullopt)
    std::optional<std::filesystem::path> WriteDump(void* exceptionPointers) noexcept;
}
