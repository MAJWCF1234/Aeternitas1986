@echo off
cd /d "%~dp0"
call build.bat
if errorlevel 1 exit /b 1
echo.
echo === Module bench (linked modules only) ===
echo   Up/Down  Enter  Q quit — redraws only on input
echo.
start "" minigame_tester.exe
