@echo off
cd /d "%~dp0"
gcc -std=c11 -Wall -Wextra -Wpedantic -O2 -o minigame_tester.exe ^
  minigame_tester.c mgt_platform.c mgt_state.c mgt_game_sim.c mgt_sync.c ^
  mgt_bus.c mgt_read_bridge.c mgt_host.c mgt_game_bridge.c mg_registry.c ^
  mg_test_cube.c
if errorlevel 1 exit /b 1
echo Built minigame_tester.exe with TESTCUBE module (verify_bench.bat).
