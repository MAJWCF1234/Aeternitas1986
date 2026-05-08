#!/usr/bin/env python3
"""Extract AET64SAVE1 ROOMS block from aeternitas_world_generated.c (visible + hidden items)."""
from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD_C = ROOT / "aeternitas_world_generated.c"


def _parse_rec_arrays(text: str, prefix: str) -> dict[str, list[str]]:
    """prefix e.g. 'WORLD_ITEM_LISTS_REC_' or 'WORLD_HIDDEN_ITEM_LISTS_REC_'."""
    out: dict[str, list[str]] = {}
    # Non-greedy block up to closing };
    pat = re.compile(
        rf"static const char \*const {prefix}(\d{{3}})\[\] = \{{(.*?)}}\s*;",
        re.DOTALL,
    )
    for m in pat.finditer(text):
        idx, body = m.group(1), m.group(2)
        out[idx] = re.findall(r'"([^"]+)"', body)
    return out


def _parse_main_table(text: str, name: str, rec_prefix: str) -> list[list[str]]:
    """Parse WORLD_*[WORLD_ROOM_COUNT] = { ... }; table into list per room."""
    rec_map = _parse_rec_arrays(text, rec_prefix)
    start = text.find(f"static const char *const *const {name}[WORLD_ROOM_COUNT] = {{")
    if start < 0:
        raise SystemExit(f"Could not find {name} table in {WORLD_C}")
    start = text.find("{", start) + 1
    depth = 1
    end = start
    while end < len(text) and depth:
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
        end += 1
    body = text[start : end - 1]
    rooms: list[list[str]] = []
    for raw in body.split(","):
        tok = raw.strip()
        if not tok:
            continue
        if tok == "EMPTY_LIST":
            rooms.append([])
        else:
            m = re.match(rf"{re.escape(rec_prefix)}(\d{{3}})\s*$", tok)
            if not m:
                raise SystemExit(f"Unexpected token in {name}: {tok!r}")
            key = m.group(1)
            if key not in rec_map:
                raise SystemExit(f"Missing array {rec_prefix}{key} for {name}")
            rooms.append(list(rec_map[key]))
    return rooms


def emit_rooms_block() -> str:
    text = WORLD_C.read_text(encoding="utf-8")
    vis = _parse_main_table(text, "WORLD_ITEM_LISTS_REC", "WORLD_ITEM_LISTS_REC_")
    hid = _parse_main_table(
        text, "WORLD_HIDDEN_ITEM_LISTS_REC", "WORLD_HIDDEN_ITEM_LISTS_REC_"
    )
    if len(vis) != len(hid):
        raise SystemExit("visible/hidden room count mismatch")
    lines = ["ROOMS"]
    for r, (vitems, hitems) in enumerate(zip(vis, hid)):
        n, h = len(vitems), len(hitems)
        lines.append(f"R {r} {n} {h}")
        lines.extend(vitems)
        lines.extend(hitems)
    return "\n".join(lines) + "\n"


def main() -> None:
    if not WORLD_C.is_file():
        print(f"Missing {WORLD_C}", file=sys.stderr)
        sys.exit(1)
    sys.stdout.write(emit_rooms_block())


if __name__ == "__main__":
    main()
