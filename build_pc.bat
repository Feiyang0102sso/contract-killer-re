@echo off
setlocal
cd /d "%~dp0"

set "CK_MSBUILD="
set "CK_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if defined CK_VS if exist "%CK_VS%\MSBuild\Current\Bin\amd64\MSBuild.exe" set "CK_MSBUILD=%CK_VS%\MSBuild\Current\Bin\amd64\MSBuild.exe"
if not defined CK_MSBUILD if exist "%CK_VSWHERE%" (
    for /f "usebackq delims=" %%V in (`call "%CK_VSWHERE%" -latest -products * -version "[17.0,19.0)" -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find MSBuild\Current\Bin\amd64\MSBuild.exe`) do set "CK_MSBUILD=%%V"
)
if not defined CK_MSBUILD (
    echo Install Visual Studio 2022 or 2026 with the Windows C++ x86 tools and Windows SDK.
    set "CK_RESULT=1"
    goto finish
)
echo Using MSBuild: "%CK_MSBUILD%"
set "CK_TARGET=ck_pc"
set "CK_EXE=BountyHunter.exe"
"%CK_MSBUILD%" "%CD%\vs\ck_pc.sln" /m /t:%CK_TARGET% /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 (
    echo Build failed.
    set "CK_RESULT=1"
    goto finish
)
echo Built: %CD%\bin\Win32\Release\%CK_EXE%
set "CK_RESULT=0"

:finish
rem Keep the window open when the script is started by double-click.
pause
exit /b %CK_RESULT%
