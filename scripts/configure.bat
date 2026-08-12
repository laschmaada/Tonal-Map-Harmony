@echo off
setlocal enabledelayedexpansion

REM Derive the repository root from the script location so the script is
REM portable to any checkout path.
set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
pushd "%REPO_ROOT%" >nul
if errorlevel 1 (
    echo [configure] ERROR: cannot enter repository root: %REPO_ROOT%
    exit /b 1
)
set "REPO_ROOT=%CD%"
popd >nul

REM =============================================================================
REM Locate a Visual Studio installation that has the C++ x64 tools.
REM
REM The GitHub Actions 'windows-latest' runner image was updated to
REM 'windows-2025-vs2026' which installs Visual Studio 2026 under
REM 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\'. Older
REM enumeration of known paths misses this install and the script would
REM exit with 'no Visual Studio found' before CMake could even start.
REM
REM We try, in order:
REM   1. vswhere.exe (ships with the VS Installer; robust on hosts where
REM      it's installed).
REM   2. Recursive 'where /R' over the most common install roots.
REM
REM Each branch redirects its output to a temp file, then reads the first
REM line with for/f usebackq (the inline-command variant of for/f trips
REM the parser when the command emits nothing). The vswhere argument list
REM contains a dotted package name (Microsoft.VisualStudio.Component.VC.
REM Tools.x86.x64) which the cmd parser treats as command separators, so
REM the vswhere call is wrapped in a one-line .bat that holds the dotted
REM argument.
REM =============================================================================
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_LIST=%TEMP%\vs_list_%RANDOM%.txt"

if exist "%VSWHERE%" (
    echo [configure] Querying vswhere...
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
    echo [configure] vswhere unavailable or returned no install; trying common VS install paths...
    for %%R in (
        "C:\Program Files\Microsoft Visual Studio"
        "C:\Program Files (x86)\Microsoft Visual Studio"
        "C:\BuildTools"
    ) do (
        if not defined VCVARS (
            echo [configure]   searching %%R
            where /R %%R vcvars64.bat > "%VS_LIST%" 2>nul
            for /f "usebackq delims=" %%I in ("%VS_LIST%") do (
                if not defined VCVARS if exist "%%I" set "VCVARS=%%I"
            )
        )
    )
)
del "%VS_LIST%" >nul 2>&1

if "%VCVARS%"=="" (
    echo [configure] ERROR: no Visual Studio found. Install VS 2019/2022/2026 Build Tools.
    exit /b 1
)

echo [configure] Using: %VCVARS%
call "%VCVARS%" >nul
if errorlevel 1 (
    echo [configure] ERROR: vcvars64.bat failed
    exit /b 1
)

cd /d "%REPO_ROOT%"
REM With vcvars64.bat sourced, cmake can auto-detect the newest installed
REM Visual Studio generator. We do NOT fall through to a different
REM generator on error: if the auto-detect fails the build is broken
REM and a different -G would just leave a half-configured build dir.
REM
REM VST3_SDK_DIR is passed as a CMake cache variable (not just an env
REM var) so the build is self-contained and works both from the workflow
REM (which clones C:\vst3sdk) and from a local dev box (where the SDK
REM may be elsewhere). The default falls back to the standard install
REM location on Windows.
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DVST3_SDK_DIR="%VST3_SDK_DIR%"
exit /b %errorlevel%
