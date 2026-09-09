@echo off
REM setup.bat - Load the MSVC x64 build environment (cl, nmake, link, ...)
REM into the current cmd session. Visual Studio is located the canonical way
REM with vswhere, so no install path or VS version is hardcoded here.
REM
REM   setup.bat               - prepare environment, keep the session
REM   setup.bat <command...>  - prepare environment, then run the command
REM
REM Run it from an existing terminal so the environment persists for the
REM session; double-clicking applies it only to the window that opens.

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo ERROR: vswhere.exe not found - is the Visual Studio Installer present?
    exit /b 1
)

REM newest installation that has the C++ toolset
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -utf8 -property installationPath`) do set "VS_INSTALL_DIR=%%i"

if not defined VS_INSTALL_DIR (
    echo ERROR: no Visual Studio installation with the C++ toolset found.
    echo Install "Desktop development with C++" via the Visual Studio Installer.
    exit /b 1
)

call "%VS_INSTALL_DIR%\VC\Auxiliary\Build\vcvars64.bat"

where cl >nul 2>&1
if errorlevel 1 (
    echo ERROR: cl.exe is not on PATH after vcvars64.bat.
    exit /b 1
)

echo.
echo MSVC x64 environment ready: cl, nmake, link are on PATH.
echo Visual Studio: %VS_INSTALL_DIR%
echo Next: cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

if not "%~1"=="" (
    echo.
    echo Running: %*
    %*
)
