@echo off
setlocal ENABLEEXTENSIONS

set "SOLUTION=%~dp0..\RvrseMonitor.sln"
if not exist "%SOLUTION%" (
    echo Solution not found: %SOLUTION%
    exit /b 1
)

set "REQUESTED_CONFIG=%~1"
if "%REQUESTED_CONFIG%"=="" (
    set "REQUESTED_CONFIG=Release"
)

if /I "%REQUESTED_CONFIG%"=="ALL" (
    call :build_config Release || exit /b 1
    call :build_config Debug || exit /b 1
    call :run_tests Release || exit /b 1
    call :run_tests Debug || exit /b 1
) else (
    call :build_config %REQUESTED_CONFIG% || exit /b 1
    call :run_tests %REQUESTED_CONFIG% || exit /b 1
)

exit /b 0

:build_config
set "CONFIG=%~1"
set "PLATFORM=x64"

echo Building %SOLUTION% (%CONFIG%^|%PLATFORM%)
msbuild "%SOLUTION%" /p:Configuration=%CONFIG%;Platform=%PLATFORM% /m
exit /b %ERRORLEVEL%

:run_tests
set "CONFIG=%~1"
set "TEST_BIN=%~dp0..\build\%CONFIG%\RvrseMonitorTests.exe"

if not exist "%TEST_BIN%" (
    echo [WARN] Test binary not found for %CONFIG% configuration: %TEST_BIN%
    exit /b 1
)

pushd "%~dp0..\build\%CONFIG%" >NUL
set "PERF_JSON=telemetry\benchmarks.json"
RvrseMonitorTests.exe --build-config=%CONFIG% --perf-json="%PERF_JSON%"
set "TEST_RESULT=%ERRORLEVEL%"
popd >NUL

if not "%TEST_RESULT%"=="0" (
    echo [FAIL] Tests failed for %CONFIG% configuration.
    exit /b %TEST_RESULT%
)

echo [PASS] Tests succeeded for %CONFIG% configuration.
exit /b 0
