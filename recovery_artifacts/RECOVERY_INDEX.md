# Aeternitas64 EXE Recovery Index

## Binary
- Original: `aeternitas64.exe` (622028 bytes)
- Entry point: `0x14000105f`
- Format: PE32+ x86-64 console executable
- Toolchain evidence: MinGW-W64 GCC 15.2.0, C23, `-g`, `-O2`

## Recovery Inputs
- `recovery_artifacts/exe_symbols.txt`
- `recovery_artifacts/exe_disasm_all.txt`
- `recovery_artifacts/dwarf_info_index.txt`
- `recovery_artifacts/dwarf_decoded_lines.txt`
- `recovery_artifacts/exe_strings.txt`

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
- Original defined symbols: `2465`
- Recovered defined symbols: `2279`
- Game-like original symbols indexed: `958`

## Build recovered EXE (parity vs original)
From repo root (requires MinGW `gcc` on PATH):

```powershell
powershell -File tools\build_recovered.ps1
```

Produces `aeternitas64_recovered.exe`. Refresh world tables from the web snapshot when needed:

```text
py tools\merge_web_world_into_recovered_json.py
py tools\build_world_c_from_recovered_json.py
```

Then rebuild the EXE. Compare stdout to the shipped binary:

```text
py tools\dual_first_diff.py --corpus recovery_artifacts\golden_playthrough --label global --full-normalize
py tools\dual_exe_compare.py
```

Golden transcripts live under `recovery_artifacts/golden_playthrough/` (see README there).

## Keeping the repo lean (hand-maintained vs soup)

Most growth comes from **recovery traces and duplicates**, not from the real game sources:

| Kind | Typical path | Notes |
|------|----------------|------|
| Hand-maintained C | `aeternitas64_ascii.c`, `aeternitas_char_creation.c`, `aeternitas_world_generated.c` | Normal source sizes (tens–low hundreds of KB). |
| World data | `world_tables_recovered.json` | Single JSON source of truth; optional compact pass if editing with tooling. |
| **Huge trace dumps** | `recovery_artifacts/raw_original_path2.txt`, `raw_original_turns.txt` | Often **hundreds of MB**; safe to delete locally; excluded from git via `.gitignore`. |
| **Embedded PE in C** | `aeternitas64_binary_rehost.c` | Default is a **thin launcher** next to `aeternitas64.exe`. Fat embed (optional): `py tools/generate_binary_rehost_c.py` → `aeternitas64_binary_rehost_fat.c`. |
| Duplicate snapshot trees | `recovery_artifacts/recovered_*/` | Full-tree copies; excluded from git; do not treat as the canonical source tree. |

To list or remove known bulk paths under `recovery_artifacts/` (dry-run by default):

```text
py tools\prune_recovery_bulk.py
py tools\prune_recovery_bulk.py --delete
```

## Next Recovery Targets
- Build parity scripts for NPC topics, merchant wares, and room loot against the original EXE.
- Use symbol-address anchored disassembly for large `cmd_*` and `run_*` functions when behavior differs from the original.
- Keep parity tests against the original EXE for command output, save/load serialization, world navigation, merchant flows, and character creation.
