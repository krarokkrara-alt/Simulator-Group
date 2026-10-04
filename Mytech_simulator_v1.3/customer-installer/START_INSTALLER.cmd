@echo off
setlocal
powershell.exe -NoLogo -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Installer.ps1"
set "installer_result=%errorlevel%"
if not "%installer_result%"=="0" echo Installation did not complete. Exit code: %installer_result%
exit /b %installer_result%
