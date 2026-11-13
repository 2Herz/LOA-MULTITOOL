export module LostArkMultiTool.Core.SelfTest;

import std;

// 데이터 자체 검사 (실행 파일에 --self-test 인자를 주면 화면 없이 실행 → build_all.bat이 빌드 직후 호출)
// 전투력 계수 · 재련 표가 망가지지 않았는지(범위 · 단조성 · 합계) 확인해서, 계수를 다시 만들거나 표를 고친 뒤 실수를 빌드 단계에서 잡음
export namespace LostArkMultiTool::Core::SelfTest
{
    // 명령줄에 자체 검사 인자가 있는지 ("--self-test")
    [[nodiscard]] bool IsRequested(std::wstring_view commandLine) noexcept;

    // 모든 검사를 실행하고 결과를 reportPath(UTF-8 텍스트)에 씀 → 실패한 항목 수 (0이면 통과)
    [[nodiscard]] int Run(const std::filesystem::path& reportPath);
}
