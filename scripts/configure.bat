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

REM Locate a Visual Studio installation that can host the JUCE 8 build.
set "VCVARS="
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"      set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"     set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VCVARS=C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if "%VCVARS%"=="" (
    echo [configure] ERROR: no Visual Studio found. Install VS 2019 or 2022 Build Tools.
    exit /b 1
)

echo [configure] Using: %VCVARS%
call "%VCVARS%" >nul
if errorlevel 1 (
    echo [configure] ERROR: vcvars64.bat failed
    exit /b 1
)

cd /d "%REPO_ROOT%"
cmake -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
exit /b %errorlevel%