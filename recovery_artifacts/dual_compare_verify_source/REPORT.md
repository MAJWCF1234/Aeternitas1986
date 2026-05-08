# Dual EXE compare (original vs recovered)

- Original: `D:\light\Aeternitas1986\aeternitas64.exe`
- Recovered: `D:\light\Aeternitas1986\recovery_artifacts\aeternitas64_verify_build.exe`
- Identical normalized stdout (all jobs): **yes**

## global

- return codes: original=0 recovered=0
- identical: **True** (diff hunks ~ 0 lines in unified diff)

## rooms

- return codes: original=0 recovered=0
- identical: **True** (diff hunks ~ 0 lines in unified diff)

## npcs_merchants

- return codes: original=0 recovered=0
- identical: **True** (diff hunks ~ 0 lines in unified diff)

## items

- return codes: original=0 recovered=0
- identical: **True** (diff hunks ~ 0 lines in unified diff)

## character_creation

- return codes: original=0 recovered=0
- identical: **True** (diff hunks ~ 0 lines in unified diff)

## surfaces

- return codes: original=0 recovered=0
- identical: **True** (diff hunks ~ 0 lines in unified diff)

## Interactive / non-stdout gaps

- Arrow-key pagers (`help modding`), splash timing, and some `ui_block_pause` flows only appear in autotest as `[CI autotest: …]` or shortened paths.
- Forge `craft` may bump proficiency via `rand() % 3`, so stdout can diverge even when logic matches.
