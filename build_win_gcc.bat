@echo off
setlocal EnableExtensions
cd /d "%~dp0"

rem Do not use "..." in the gcc line: the linker will look for a file literally named "...".

set "OPTS=-std=c11 -Wall -Wextra -Os -s -finput-charset=UTF-8 -fexec-charset=UTF-8"

windres aeternitas64_manifest.rc -O coff -o aeternitas64_manifest.o
if errorlevel 1 exit /b 1

gcc %OPTS% aeternitas64_manifest.o -o aeternitas64.exe ^
  aeternitas64_ascii.c ^
  aeternitas_item_catalog.c ^
  aeternitas_world_generated.c ^
  aeternitas_char_creation.c ^
  aeternitas_mod_guide.c ^
  aeternitas_char_description.c ^
  aeternitas_mods.c ^
  aeternitas_mod_bootstrap.c
if errorlevel 1 exit /b 1

echo OK: aeternitas64.exe
exit /b 0
