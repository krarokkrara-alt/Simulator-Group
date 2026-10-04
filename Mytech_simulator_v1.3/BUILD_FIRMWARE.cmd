@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Firmware.ps1"
set "build_result=%errorlevel%"
echo Build exit code: %build_result%
pause
exit /b %build_result%
