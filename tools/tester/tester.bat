@echo off
setlocal & cd /d "%~dp0"
set "T=%~dp0" & set "MG=%T%..\testing" & set "MINI=%T%..\..\minigames" & set "ROOT=%T%..\.."
set "M=%~1" & if "%M%"=="" set "M=all"
if /i "%M%"=="help" goto help
if /i "%M%"=="-h" goto help
if /i "%M%"=="?" goto help
if /i "%M%"=="bench" goto bench
if /i "%M%"=="adventure" goto adventure
if /i not "%M%"=="all" if /i not "%M%"=="smoke" if /i not "%M%"=="embed" if /i not "%M%"=="launch" (
  echo Unknown: %M% & goto help
)
call :build & if errorlevel 1 exit /b 1
if /i "%M%"=="all" goto run_all
call :run & exit /b %errorlevel%

:run_all
call :run & if errorlevel 1 exit /b 1
goto adventure

:help
echo tester.bat [all^|smoke^|embed^|launch^|adventure^|bench]
exit /b 0

:build
gcc -std=c11 -Wall -Wextra -O2 -DAETER_MINIGAMES -I"%MG%" -I"%MINI%" -o "%T%tester.exe" ^
  "%T%tester.c" "%MG%\mgt_platform.c" "%MG%\mgt_state.c" "%MG%\mgt_game_sim.c" ^
  "%MG%\mgt_sync.c" "%MG%\mgt_bus.c" "%MG%\mgt_read_bridge.c" "%MG%\mgt_host.c" ^
  "%MG%\mgt_game_bridge.c" "%MINI%\mg_registry.c" "%MINI%\mg_lockpick.c" "%MINI%\mg_piano.c" ^
  "%MINI%\mg_piano_data.c" "%MINI%\mg_piano_audio.c" "%MINI%\mg_fishing.c" "%MINI%\mg_farming.c" ^
  "%MINI%\mg_cooking.c" "%MINI%\mg_writing.c" "%MINI%\mg_reading.c" "%MINI%\mg_gambling.c" ^
  "%MINI%\mg_hunting.c" -lwinmm
exit /b %errorlevel%

:run
echo --- %M% ---
if /i "%M%"=="all" ("%T%tester.exe" native) else ("%T%tester.exe" %M%)
exit /b %errorlevel%

:adventure
echo --- adventure ---
set AETER_AUTOTEST=1 & set MGT_AUTOTEST=1
cd /d "%ROOT%" & call build_aeternitas64.bat & if errorlevel 1 exit /b 1
cd /d "%T%"
< "%T%adventure_hooks.in" "%ROOT%\aeternitas64.exe" > "%T%adventure_hooks.out" 2>&1
if errorlevel 1 exit /b 1
findstr /i /c:"That activity is not available" /c:"Could not start that activity" "%T%adventure_hooks.out" >nul
if not errorlevel 1 exit /b 1
echo PASS: adventure hooks
if /i "%M%"=="all" echo. & echo === tester: ALL PASS ===
exit /b 0

:bench
cd /d "%MG%" & call build.bat
exit /b %errorlevel%
