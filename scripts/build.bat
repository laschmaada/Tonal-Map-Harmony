@echo off
setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
pushd "%REPO_ROOT%" >nul
if errorlevel 1 (
    echo [build] ERROR: cannot enter repository root: %REPO_ROOT%
    exit /b 1
)
set "REPO_ROOT=%CD%"
popd >nul

REM =============================================================================
REM Locate a Visual Studio installation that has the C++ x64 tools.
REM Same logic as scripts\configure.bat. See comments there for why this
REM is as convoluted as it is.
REM =============================================================================
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_LIST=%TEMP%\vs_list_%RANDOM%.txt"

if exist "%VSWHERE%" (
    echo [build] Querying vswhere...
    set "VSWRAP=%TEMP%\vswhere_runner_%RANDOM%.bat"
    >  "%VSWRAP%" echo @echo off
    >> "%VSWRAP%" echo call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    call "%VSWRAP%" > "%VS_LIST%" 2>nul
    del "%VSWRAP%" >nul 2>&1
    for /f "usebackq delims=" %%I in ("%VS_LIST%") do (
        if not defined VCVARS if exist "%%I\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=%%I\VC\Auxiliary\Build\vcvars64.bat"
    )
)

if "%VCVARS%"=="" (
    echo [build] vswhere unavailable or returned no install; trying common VS install paths...
    for %%R in (
        "C:\Program Files\Microsoft Visual Studio"
        "C:\Program Files (x86)\Microsoft Visual Studio"
        "C:\BuildTools"
    ) do (
        if not defined VCVARS (
            echo [build]   searching %%R
            where /R %%R vcvars64.bat > "%VS_LIST%" 2>nul
            for /f "usebackq delims=" %%I in ("%VS_LIST%") do (
                if not defined VCVARS if exist "%%I" set "VCVARS=%%I"
            )
        )
    )
)
del "%VS_LIST%" >nul 2>&1

if "%VCVARS%"=="" (
    echo [build] ERROR: no Visual Studio found. Install VS 2019/2022/2026 Build Tools.
    exit /b 1
)

echo [build] Using: %VCVARS%
call "%VCVARS%" >nul
if errorlevel 1 (
    echo [build] ERROR: vcvars64.bat failed
    exit /b 1
)

cd /d "%REPO_ROOT%"
cmake --build build --config Release
exit /b %errorlevel%
