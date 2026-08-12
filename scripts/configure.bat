@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
  echo vcvars64 failed
  exit /b 1
)
cd /d "J:\DevOps\MappingTonalHarmonyPro"
cmake -B build -G "Visual Studio 16 2019" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
exit /b %errorlevel%