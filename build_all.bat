@echo off
rem 한글 깨짐 방지 (UTF-8 설정)
chcp 65001 > nul

echo ==============================================
echo Visual Studio 2026 환경 변수 자동 검색 및 로드 중...
echo ==============================================

rem vswhere를 사용하여 설치된 VS 2026 빌드 환경 도구 경로를 자동으로 찾습니다.
set "VS_PATH="
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version [18.0^,19.0^) -property installationPath`) do (
    set "VS_PATH=%%i"
)

if "%VS_PATH%"=="" (
    echo [에러] Visual Studio 2026 설치 경로를 찾을 수 없습니다.
    goto error
)

rem 환경 변수 파일 실행
call "%VS_PATH%\Common7\Tools\VsDevCmd.bat"

rem 2. 로스트아크 멀티툴 프로젝트 디렉토리로 이동
cd /d "C:\Users\Administrator\Desktop\My Project\5. 3D\로스트아크 멀티툴"

echo ==============================================
echo 1. Debug 구성을 빌드합니다...
echo ==============================================
msbuild "로스트아크 멀티툴.slnx" /t:Rebuild /p:Configuration=Debug
if %errorlevel% neq 0 goto error

echo ==============================================
echo 2. Release 구성을 빌드합니다...
echo ==============================================
msbuild "로스트아크 멀티툴.slnx" /t:Rebuild /p:Configuration=Release
if %errorlevel% neq 0 goto error

echo ==============================================
echo 3. 데이터 자체 검사 (전투력 계수 · 재련 표 · lopec 회귀)
echo ==============================================
rem lopec 회귀는 앱 엔진으로 Verify\lopec_results 측정을 다시 계산 (측정 데이터가 없는 PC면 건너뜀, 결과 상세: Verify\lopec_results\regression\last_result.csv)
rem GUI 프로그램이라 start /wait로 끝날 때까지 기다려 종료 코드(실패 항목 수)를 받음
for %%C in (Debug Release) do (
    start "" /wait "x64\%%C\LOA-MULTITOOL.exe" --self-test
    if errorlevel 1 (
        type "x64\%%C\self_test_report.txt"
        echo [에러] %%C 자체 검사 실패
        goto error
    )
)
type "x64\Release\self_test_report.txt"

echo ==============================================
echo [성공] Debug 및 Release 멀티 빌드가 완료되었습니다!
echo ==============================================
pause
exit

:error
echo ----------------------------------------------
echo [에러] 빌드 도중 오류가 발생하여 중단되었습니다.
echo ----------------------------------------------
pause
