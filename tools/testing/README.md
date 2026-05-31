# Module Bench (`minigame_tester`)

A **bus simulator** for minigame modules — no `aeternitas64.exe` required. The harness auto-wires purse, pack, room, weather, and turn counter for each linked cube, then runs it in a fullscreen viewport.

When a cube tests clean on the bench, copy the same source into `minigames/` and link it in `build_aeternitas64.bat`.

## Build

```bat
cd tools\testing
build.bat
minigame_tester.exe
```

## Keys

| Key | Action |
|-----|--------|
| Up/Down | Select a **linked** module (only cubes in this build's registry) |
| Enter | Auto-wire bus + run module |
| Q | Quit (saves `harness_save.mgt`) |

Inside a module: **ESC** returns to the bench.

The screen redraws only when you press a key — no idle animation spam.

## Verify the bench (TESTCUBE)

```bat
verify_bench.bat
```

1. **Test Cube** is the only linked module in the default harness build.
2. **Enter** — seats the cube and opens the pulse viewport.
3. **SPACE** a few times, then **ESC** — banner confirms bus sync.

Source: `tools/testing/mg_test_cube.c` (harness-only, not in the main game).

## Plug in a new cube (develop a minigame)

1. Create `minigames/mg_yourgame.c` with `int mg_run_yourgame(MgtSession *session);`
2. Add pin spec to `tools/testing/mgt_bus.c` `g_modules[]` (which contacts the cube needs).
3. Register in `tools/testing/mg_registry.c` and add the `.c` file to `tools/testing/build.bat`.
4. Bench: select cube → **Enter** to run.
5. Native: register in `minigames/mg_registry.c` + `build_aeternitas64.bat`.

## Bus pins

| Pin | Simulates |
|-----|-----------|
| PWR | Harness power (always on) |
| COIN | Adventure purse |
| PACK | Inventory |
| ROOM | Room slug gate |
| SKIL | Stats / craft proficiency |
| WTHR | Weather string |
| TURN | Adventure turn counter |
| MGST | Minigame persistent profile bus |

Sync uses the same `mgt_sync_from_world` / `mgt_sync_to_world` path as the real game.

## Main game

```bat
build_aeternitas64.bat
```

All cubes in `minigames/` — the live game **is** the native socket; quicksave embeds the MGST bus (`MGT` section).

## Tests

Automated suites: `tools\tester\tester.bat` (all / smoke / embed / launch / adventure / bench).
