@echo off
setlocal & cd /d "%~dp0"
set "T=%~dp0" & set "MG=%T%..\testing" & set "MINI=%T%..\..\minigames" & set "ROOT=%T%..\.."
set "M=%~1" & if "%M%"=="" set "M=all"
if /i "%M%"=="help" goto help
if /i "%M%"=="-h" goto help
if /i "%M%"=="?" goto help
if /i "%M%"=="bench" goto bench
if /i "%M%"=="adventure" goto adventure
if /i "%M%"=="craft" goto craft
if /i "%M%"=="parser" goto parser_only
if /i "%M%"=="nlstress" goto nlstress_only
if /i not "%M%"=="all" if /i not "%M%"=="smoke" if /i not "%M%"=="embed" if /i not "%M%"=="launch" if /i not "%M%"=="outcomes" (
  echo Unknown: %M% & goto help
)
call :build & if errorlevel 1 exit /b 1
if /i "%M%"=="all" goto run_all
if /i "%M%"=="craft" goto craft_only
if /i "%M%"=="parser" goto parser_only
call :run & exit /b %errorlevel%

:run_all
call :run & if errorlevel 1 exit /b 1
call :adventure & if errorlevel 1 exit /b 1
call :craft & if errorlevel 1 exit /b 1
call :parser & if errorlevel 1 exit /b 1
call :nlstress & if errorlevel 1 exit /b 1
echo. & echo === tester: ALL PASS ===
exit /b 0

:help
echo tester.bat [all^|smoke^|embed^|outcomes^|launch^|adventure^|craft^|parser^|nlstress^|bench]
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
echo --- native ---
"%T%tester.exe" native
exit /b %errorlevel%

:adventure
echo --- adventure ---
set AETER_AUTOTEST=1 & set MGT_AUTOTEST=1
if exist "%ROOT%\harness_save.mgt" del /q "%ROOT%\harness_save.mgt"
if exist "%T%harness_save.mgt" del /q "%T%harness_save.mgt"
cd /d "%ROOT%" & call build_aeternitas64.bat & if errorlevel 1 exit /b 1
cd /d "%T%"
< "%T%adventure_hooks.in" "%ROOT%\aeternitas64.exe" > "%T%adventure_hooks.out" 2>&1
if errorlevel 1 exit /b 1
findstr /i /c:"That activity is not available" /c:"Could not start that activity" "%T%adventure_hooks.out" >nul
if not errorlevel 1 exit /b 1
findstr /i /c:"Lock opened" "%T%adventure_hooks.out" >nul
if errorlevel 1 exit /b 1
findstr /i /c:"Last lockpick noise" "%T%adventure_hooks.out" >nul
if errorlevel 1 exit /b 1
findstr /i /c:"Clean shot" "%T%adventure_hooks.out" >nul
if errorlevel 1 exit /b 1
echo PASS: adventure hooks
exit /b 0

:craft_only
call :build & if errorlevel 1 exit /b 1
:craft
echo --- craft save ---
set AETER_AUTOTEST=1 & set MGT_AUTOTEST=1
if exist "%ROOT%\harness_save.mgt" del /q "%ROOT%\harness_save.mgt"
if exist "%T%harness_save.mgt" del /q "%T%harness_save.mgt"
cd /d "%ROOT%" & call build_aeternitas64.bat & if errorlevel 1 exit /b 1
cd /d "%T%"
< "%T%adventure_craft.in" "%ROOT%\aeternitas64.exe" > "%T%adventure_craft.out" 2>&1
if errorlevel 1 exit /b 1
findstr /i /c:"FORGE COMPLETE" "%T%adventure_craft.out" >nul
if errorlevel 1 exit /b 1
findstr /i /c:"CRAFTPROF" "%ROOT%\aeternitas64_save.txt" >nul
if errorlevel 1 exit /b 1
findstr /i /c:"Crude" "%ROOT%\aeternitas64_save.txt" >nul
if errorlevel 1 exit /b 1
echo PASS: craft profile in save
exit /b 0

:bench
cd /d "%MG%" & call build.bat
exit /b %errorlevel%

:parser_only
call :build & if errorlevel 1 exit /b 1
:parser
echo --- parser regression ---
cd /d "%ROOT%" & call build_aeternitas64.bat & if errorlevel 1 exit /b 1
cd /d "%T%.."
py -3 parser_regression_aeternitas64.py 2>nul || python parser_regression_aeternitas64.py
exit /b %errorlevel%

:nlstress_only
call :build & if errorlevel 1 exit /b 1
:nlstress
echo --- parser natural-language stress ---
cd /d "%ROOT%" & call build_aeternitas64.bat & if errorlevel 1 exit /b 1
cd /d "%T%.."
py -3 parser_nl_stress.py 2>nul || python parser_nl_stress.py
exit /b %errorlevel%
