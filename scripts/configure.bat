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
REM We try, in order:
REM   1. vswhere.exe (ships with the VS Installer; finds the latest install
REM      that has the C++ workload). Robust on hosts where it's installed.
REM   2. Recursive 'where /R' for vcvars64.bat under both Program Files roots.
REM      Fallback for hosts that lack vswhere (some BuildTools installs).
REM
REM Both branches redirect their output to a temp file, then we read the
REM first line with for/f. The for/f MUST use usebackq + delims= (no tokens=*)
REM so the quoted arg is interpreted as a filename, not a literal string.
REM =============================================================================
set "VCVARS="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VS_LIST=%TEMP%\vs_list_%RANDOM%.txt"

if exist "%VSWHERE%" (
    echo [configure] Querying vswhere...
    REM The vswhere argument list contains dots
    REM (Microsoft.VisualStudio.Component.VC.Tools.x86.x64) which the cmd
    REM parser treats as command separators. Wrap the call in a temp .bat
    REM so the outer for/f never sees the dotted string.
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
    REM 'where /R C:\' sometimes returns 'system cannot find the path
    REM specified' on the windows-2025-vs2026 runner image even though C:\
    REM is valid - some interaction with the read-only C:\ snapshot. Try
    REM the most common install roots explicitly instead. The recursive
    REM 'where /R' works under specific subdirs as proven by the diagnostic
    REM on a previous run that found vcvars at
    REM 'C:\Program Files\Microsoft Visual Studio\18\Enterprise\...'.
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
REM PR #2 review C2: with vcvars64.bat sourced, cmake can auto-detect the
REM newest Visual Studio generator. We pin a fallback generator string only
REM if auto-detection fails (older CMake on older hosts).
cmake -B build -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if errorlevel 1 (
    cmake -B build -G "Visual Studio 18 2026" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
if errorlevel 1 (
    cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
if errorlevel 1 (
    cmake -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)
exit /b %errorlevel%
