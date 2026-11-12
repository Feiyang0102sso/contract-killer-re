@echo off
rem One-click build: compile + link in the build WSL distro; result in _output\BountyHunter.
rem Usage: build.bat [-B] [-c]   (-B recompiles every object, -c compiles only)
rem The distro is ck-build (made by setup.bat); set CK_DISTRO to use another one that has the toolchain.
setlocal
set DISTRO=%CK_DISTRO%
if "%DISTRO%"=="" set DISTRO=ck-build
wsl.exe -d %DISTRO% -u root --cd "%~dp0." --exec bash toolchain/build.sh %*
set STATUS=%ERRORLEVEL%
if "%STATUS%"=="0" (
    echo build finished: %~dp0_output
) else (
    echo build failed with exit code %STATUS%
)
pause
exit /b %STATUS%
