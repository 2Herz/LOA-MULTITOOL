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
echo 3. 데이터 자체 검사 (전투력 계수 · 재련 표)
echo ==============================================
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
echo 4. 전투력 계수 회귀 테스트 (lopec 측정값, 보정 키트가 있는 PC만)
echo ==============================================
where python > nul 2>&1
if %errorlevel% equ 0 (
    python tools\Test-CombatRegression.py
    if errorlevel 1 goto error
) else (
    echo [건너뜀] python이 없어 회귀 테스트를 건너뜁니다.
)

echo ==============================================
echo 5. 코드 서명 (SIGN_CERT_THUMBPRINT 환경 변수가 있을 때만)
echo ==============================================
rem 인증서를 Windows 인증서 저장소에 설치한 뒤: setx SIGN_CERT_THUMBPRINT "인증서 지문"
if defined SIGN_CERT_THUMBPRINT (
    signtool sign /sha1 %SIGN_CERT_THUMBPRINT% /fd SHA256 /tr http://timestamp.digicert.com /td SHA256 "x64\Release\LOA-MULTITOOL.exe"
    if errorlevel 1 goto error
    signtool verify /pa "x64\Release\LOA-MULTITOOL.exe"
    if errorlevel 1 goto error
) else (
    echo [건너뜀] SIGN_CERT_THUMBPRINT가 없어 서명하지 않습니다.
)

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
