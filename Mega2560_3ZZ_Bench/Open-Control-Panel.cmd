@echo off
setlocal
if not exist "%~dp0Mega3ZZ-Control.exe" (
  echo Mega3ZZ-Control.exe is missing. Build it from the project folder with:
  echo powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-ControlPanel.ps1
  pause
  exit /b 1
)
start "Mega Bench Control" "%~dp0Mega3ZZ-Control.exe"
endlocal
