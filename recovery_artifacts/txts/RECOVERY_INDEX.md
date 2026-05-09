# Aeternitas64 EXE Recovery Index

## Binary
- Original: `aeternitas64.exe` (482304 bytes)
- Entry point: `0x14000105f`
- Format: PE32+ x86-64 console executable
- Toolchain evidence: MinGW-W64 GCC 15.2.0, C23, `-g`, `-O2`

## Recovery Inputs
- `recovery_artifacts/txts/exe_symbols.txt`
- `recovery_artifacts/txts/exe_disasm_all.txt`
- `recovery_artifacts/txts/dwarf_info_index.txt`
- `recovery_artifacts/txts/dwarf_decoded_lines.txt`
- `recovery_artifacts/txts/exe_strings.txt`

## Source Units Named By The EXE
- `CRT_fp10.c`
- `__initenv.c`
- `__p___initenv.c`
- `_newmode.c`
- `aeternitas64_ascii.c`
- `aeternitas_char_creation.c`
- `aeternitas_char_description.c`
- `aeternitas_mod_bootstrap.c`
- `aeternitas_mod_guide.c`
- `aeternitas_mods.c`
- `aeternitas_world_generated.c`
- `cinitexe.c`
- `crt_handler.c`
- `crtexe.c`
- `cygming-crtbegin.c`
- `cygming-crtend.c`
- `dllargv.c`
- `fake`
- `gccmain.c`
- `libgcc2.c`
- `merr.c`
- `mingw_helpers.c`
- `mingw_matherr.c`
- `natstart.c`
- `pesect.c`
- `pseudo-reloc-list.c`
- `pseudo-reloc.c`
- `strtok_r.c`
- `tlsmcrt.c`
- `tlssup.c`
- `tlsthrd.c`
- `ucrt___local_stdio_printf_options.c`
- `ucrt___local_stdio_scanf_options.c`
- `ucrt__getmainargs.c`
- `ucrt_amsg_exit.c`
- `ucrt_fprintf.c`
- `ucrt_printf.c`
- `ucrt_snprintf.c`
- `ucrt_sscanf.c`
- `ucrt_vfprintf.c`
- `ucrt_vsnprintf.c`
- `usermatherr.c`
- `wildcard.c`
- `xncommod.c`
- `xthdloc.c`
- `xtxtmode.c`

## Recovered World Tables
- Rooms: `161`
- Directions per room: `19`
- Recovered arrays: slugs, titles, blurbs, regions, entities, darkness flags, exits
- Visible room item lists: `160` rooms, `512` entries
- Hidden room item lists: `16` rooms, `19` entries
- NPC line sets: `10`
- Merchant tables: `7`

## Symbol Parity
- Original defined symbols: `0`
- Recovered defined symbols: `0`
- Game-like original symbols indexed: `0`

## Consolidated (canonical copy lives in repo root)
Byte-identical duplicates were removed from `recovery_artifacts` so this folder tracks **unique** recovery inputs only.

- **`recovered_2026-05-07_1444/recycle_exes/mods/`** — deleted; same tree as **`mods/`** at repository root (sample DLC pack).
- **`recovered_2026-05-07_1444/sources/`** — deleted files that matched repo root exactly:
  `aeternitas_mod_bootstrap.c/.h`, `aeternitas_mod_guide.h`, `aeternitas_mods.c/.h`,
  `aeternitas_world_generated.h`, `skin_tsv_fragment.inc`.

**Still present there:** snapshots that **differ** from today’s sources (e.g. `aeternitas64_ascii.c`,
`aeternitas_world_generated.c`, character sources), reference **`aeternitas64.exe`**, and
**`aeternitas64_rebuilt_*.exe`** for historical comparison.

## Next Recovery Targets
- Build parity scripts for NPC topics, merchant wares, and room loot against the original EXE.
- Use symbol-address anchored disassembly for large `cmd_*` and `run_*` functions when behavior differs from the original.
- Keep parity tests against the original EXE for command output, save/load serialization, world navigation, merchant flows, and character creation.
