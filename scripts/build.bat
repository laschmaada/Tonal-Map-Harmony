@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 (
  echo vcvars64 failed
  exit /b 1
)
cd /d "J:\DevOps\MappingTonalHarmonyPro"
cmake --build build --config Release --target MidiChordPad_VST3 ChordTests
exit /b %errorlevel%