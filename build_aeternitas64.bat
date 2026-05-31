@echo off
cd /d "%~dp0"
set MG=tools\testing
set MINI=minigames
gcc -std=c11 -Wall -Wextra -O2 -DAETER_MINIGAMES -I. -I%MG% -I%MINI% -o aeternitas64.exe ^
  aeternitas64_ascii.c aeternitas_world_generated.c aeternitas_item_catalog.c ^
  aeternitas_char_creation.c aeternitas_char_description.c ^
  aeternitas_mods.c aeternitas_mod_bootstrap.c aeternitas_mod_guide.c ^
  %MG%\mgt_platform.c %MG%\mgt_state.c %MG%\mgt_game_sim.c %MG%\mgt_sync.c ^
  %MG%\mgt_bus.c %MG%\mgt_read_bridge.c %MG%\mgt_host.c %MG%\mgt_game_bridge.c ^
  %MINI%\mg_registry.c %MINI%\mg_lockpick.c %MINI%\mg_piano.c %MINI%\mg_piano_data.c ^
  %MINI%\mg_piano_audio.c %MINI%\mg_fishing.c %MINI%\mg_farming.c %MINI%\mg_cooking.c ^
  %MINI%\mg_writing.c %MINI%\mg_reading.c %MINI%\mg_gambling.c %MINI%\mg_hunting.c ^
  -lwinmm
if errorlevel 1 exit /b 1
echo Built aeternitas64.exe with minigames (AETER_MINIGAMES).
