# Tavern Piano — Minigame Designer Brief

**Product:** Aeternitas64 (harness: `minigame_tester.exe` → Piano)  
**Web reference:** `startPianoMinigame` / `parsePianoNotationToSteps`  
**Sheet format:** **Virtual Piano (VP) keyboard notation** — same letters as [virtualpiano.net](https://virtualpiano.net/how-to-play/) and standard “VP sheets” / Roblox piano paste sheets.

**Playable build:** `tools/testing/minigame_tester.exe` → **Piano Performance** — **96×30** canvas (`MGT_CANVAS_W` × `MGT_CANVAS_H`), falling-note lanes, hit line, command mode, GM MIDI (or beep fallback).

This document is the **authoritative key map and notation grammar** for anyone writing sheets or tuning gameplay. Absolute octave names (C3 vs C2) do **not** matter for sheet compatibility; **keyboard letters** do.

Regenerate embedded sheet strings from the web edition: `python tools/testing/gen_piano_data.py` (optional env `AETERNITAS_WEB_INDEX`).

---

## 1. Harness quick start

```bat
cd tools\testing
build.bat
minigame_tester.exe
```

Select **Piano Performance**. **Esc** from the idle screen returns to the harness menu; while playing, **Esc** pauses/resumes.

| Input | Action |
|-------|--------|
| **Up / Down** | Change song (when idle) |
| **Enter** | Start performance |
| **Esc** | Pause / resume while playing; exit when idle |
| **:** | Command mode (`HELP`, `START`, `SONG 1`, `SPEED fast`, …) |
| **b** / **B** | Bot toggle |
| **VP keys** | Play notes (timing at hit line) |

---

## 2. Layout (96×30)

| Rows | Panel |
|------|--------|
| 0–5 | Header: state, song, speed, score, current step, controls |
| 6–19 | Falling notes (12 lanes), hit line at row 18 |
| 20–25 | VP keyboard map + recent key presses |
| 26–29 | Prompt / command line |

**Designer note:** **`b` / `B` cannot be used as piano notes during performance** — reserved for bot (matches web). In VP sheets, the note **B** on the keyboard is **`n`** (white key before `m`). Use **`B`** (shift+b) for A#6 if needed.

---

## 3. Virtual Piano keyboard map (full)

### 3.1 Physical layout on QWERTY (use for UI “Key Assist”)

Black keys are **Shift + the key shown below** on the number row; on letter rows, **uppercase** is the black key above/right of the lowercase white key (standard VP).

```
  BLACK (shift):  !   @       $   %   ^       *   (           [group 2+3 like a piano]
  WHITE:          1   2   3   4   5   6   7   8   9   0

  BLACK:          Q   W   E       T   Y       I   O   P
  WHITE:          q   w   e   r   t   y   u   i   o   p

  BLACK:          S   D       G   H   J
  WHITE:          a   s   d   f   g   h   j   k   l

  BLACK:          Z       C   V
  WHITE:          z   x   c   v   b   n   m
```

**Range:** `1` (lowest) through `m` (highest) — **61 keys**, same as VP “1 to m”.

### 3.2 Complete character table (sheet authoring)

Use this table when writing or validating VP sheets. **Order** is chromatic from low to high.

| Key char | Type | Key char | Type | Key char | Type |
|----------|------|----------|------|----------|------|
| `1` | white | `!` | black | `2` | white |
| `@` | black | `3` | white | `4` | white |
| `$` | black | `5` | white | `%` | black |
| `6` | white | `^` | black | `7` | white |
| `8` | white | `*` | black | `9` | white |
| `(` | black | `0` | white | | |
| `q` | white | `Q` | black | `w` | white |
| `W` | black | `e` | white | `E` | black |
| `r` | white | `t` | white | `T` | black |
| `y` | white | `Y` | black | `u` | white |
| `i` | white | `I` | black | `o` | white |
| `O` | black | `p` | white | `P` | black |
| `a` | white | `A` | black | `s` | white |
| `S` | black | `d` | white | `D` | black |
| `f` | white | `g` | white | `G` | black |
| `h` | white | `H` | black | `j` | white |
| `J` | black | `k` | white | `l` | white |
| `L` | black | `z` | white | `Z` | black |
| `x` | white | `c` | white | `C` | black |
| `v` | white | `V` | black | `n` | white |
| `m` | white | | | | |

**Invalid for sheets:** any character not in the table above (letters outside this set, `#`, `&`, etc.) is skipped by the parser unless added to the engine later.

**Reserved in performance UI:** lowercase/uppercase `b` = bot toggle (not a note).

---

## 4. VP sheet notation grammar

### 4.1 What becomes a “step”

The parser scans left-to-right, top-to-bottom (each line is a row).

| Syntax | Meaning in VP | Meaning in minigame |
|--------|----------------|---------------------|
| `[abc]` | Play a, b, c together | **One step**, chord `abc` — player must hit **all** listed keys (any order) before the step clears |
| `y` | Single key | **One step**, single key `y` |
| `[5tu]` | Chord | **One step**, keys `5`, `t`, `u` |
| ` - ` | Short pause | **Ignored** (no step) |
| `\|` | Longer pause | **Ignored** (no step) |
| spaces | Spacing / rhythm | **Ignored** between steps (only chars inside `[...]` or lone key chars count) |
| newline | Next row | Continues parsing (no extra step by itself) |
| `{` `}` | VP extended | **Ignored** in C harness / web parser today |

**Inside `[...]`:** only valid piano characters are kept; spaces inside brackets are ignored (`[a s d]` → chord `asd`).

### 4.2 Example

VP line:

```text
[3e] - 0 - [5w] t y r
```

Becomes **6 steps:**

1. Chord `3` + `e`
2. Single `0`
3. Chord `5` + `w`
4. Single `t`
5. Single `y`
6. Single `r`

### 4.3 Sheet IDs in catalog (web / target content)

| ID | Title | Difficulty | Notes |
|----|--------|------------|--------|
| `barkeep_lesson` | The Barkeep's First Lesson | easy | Full notation in web |
| `lantern_waltz` | Candlelight Reel of Hollow Ridge | medium | Harness may ship truncated; web has full VP string |
| `last_call_etude` | Last Call at the Copper Cup | hard | Harness may ship truncated; web has full VP string |

**Web BPM metadata (for future rhythm mode, not enforced in harness):**

| Sheet | Label | BPM | Speed multiplier |
|-------|--------|-----|------------------|
| `barkeep_lesson` | Easy | 200 | 2.1 |
| `lantern_waltz` | Medium | 230 | 2.6 |
| `last_call_etude` | Hard | 255 | 3.0 |

---

## 5. Gameplay rules (performance mode)

| Rule | Behavior |
|------|----------|
| **Display** | Shows current step (`Current: [...]`) and keys still required (`Remaining: ...`) |
| **Correct key** | If pressed key is in **Remaining**, it is removed; sound plays |
| **Chord complete** | When **Remaining** empty → advance to next step, combo++ , score += 100 + combo |
| **Wrong key** | Pressing a valid piano key **not** in Remaining (or after chord logic): **miss**, combo = 0, **skip to next step** (web-aligned) |
| **Any piano key** | Valid piano character always plays **sound**, even on miss |
| **End** | After last step → “Performance complete” |
| **Bot** | Each bot tick: plays sound for **first character** of current step string, then advances whole step (harness simplification; web bot removes one key at a time from chords) |

---

## 6. Audio spec (implementation reference)

| Parameter | Value |
|-----------|--------|
| Layout | Same as §3 (`pianoKeyToSemitone`) |
| Lowest key `1` | MIDI note **48** (engine label: C3) |
| Highest key `m` | MIDI note **108** (C8) |
| Program | GM **0** Acoustic Grand Piano |
| Note length | **210 ms** (note on → note off) |
| Velocity | **92** |
| Backend | Windows: **MIDI Mapper** (system GM / Sound Blaster / MPU-401 route); fallback **PC speaker** |

VP community charts sometimes label `1` as **C2**; sheets still paste the **same letters** — only global transpose differs.

---

## 7. UI copy hooks (suggested)

**Music book**

- Title: `TAVERN PIANO` / `COLLECTED SHEETS`
- Hint: `Up/Down select · ENTER perform · ESC exit`

**Performance**

- `Sheet: {title}` · `Difficulty: {easy|medium|hard}`
- `Score / Hits / Misses / Combo / Max`
- `Current: [{step}]` · `Remaining: {chars}`
- `Audio: GM MIDI (210ms)` or fallback label
- `B bot · ESC book`

**Help overlay (recommended for designer mockups)**

- Show §3.1 ASCII keyboard
- “Sheets use Virtual Piano letters — copy from VP sheets as-is”
- “`[chord]` = play keys together”

---

## 8. Harness vs shipping game

| Item | Harness (`tools/testing`) | Main game (`aeternitas64.exe`) |
|------|---------------------------|--------------------------------|
| Piano minigame | **Yes** | **Not wired** (narrative stub only) |
| Sheet source | Embedded `k_sheets[]` | Should use `pianoSheetCatalog` / items |
| Save | `harness_save.mgt` | Future profile fields |

---

## 9. Quick validation checklist for new sheets

1. Only use characters from §3.2.  
2. Chords wrapped in `[...]`.  
3. Avoid lowercase `b` in chords if players need that pitch in performance (bot conflict).  
4. Test step count: paste notation into harness piano and step through.  
5. For full VP rhythm playback later, keep `-` and `|` in source text for when timing is implemented.

---

## 10. Machine-readable key list (copy/paste)

Valid piano characters (61 + 2 reserved):

```text
12!@345$%^89(0qQwWeErRtTyYuUiIoOpPaAsSdDfFgGhHjJkKlLzZxXcCvVnNmM
```

**Playable in performance (61):** exclude `b` and `B` from performance input — use `n` / `M` etc. for those pitches.

```text
12!@345$%^89(0qQwWeErRtTyYuUiIoOpPaAsSdDfFgGhHjJkKlLzZxXcCvVnNm
```

(Uppercase `B` still toggles bot; lowercase `b` toggles bot.)

---

*Source of truth in code: `tools/testing/mg_piano_audio.c` (map), `tools/testing/mg_piano.c` (parser + rules), web `index.html` `pianoKeyToSemitone` + `parsePianoNotationToSteps`.*
