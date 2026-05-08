#!/usr/bin/env python3
"""Build aeternitas64_dev_save.txt (ROOMS from aeternitas_world_generated.c).

Dev save: blacksmith forge (room 15), full catalog slugs in pack, 18 rows (>12) for
equipment UI, partial EQUIP, high coins/craft.

  py -3 tools/mk_dev_save.py

ROOMS block: ``tools/export_save_rooms_from_world.py`` (keeps saves aligned with the
compiled world). CHARACTER tail is still copied from ``aeternitas64_save.txt``.

Load in-game: aeternitas64 --save aeternitas64_dev_save.txt  then choose  Restore (2) at title.
"""
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
_TOOLS = Path(__file__).resolve().parent
SRC = ROOT / "aeternitas64_save.txt"
DST = ROOT / "aeternitas64_dev_save.txt"
WORLD_ROOM_COUNT = 161


def main() -> None:
    if str(_TOOLS) not in sys.path:
        sys.path.insert(0, str(_TOOLS))
    from export_save_rooms_from_world import emit_rooms_block

    lines = SRC.read_text(encoding="utf-8").splitlines()
    char_i = lines.index("CHARACTER")
    endchar_i = lines.index("ENDCHAR")
    rooms_block = emit_rooms_block().strip().splitlines()
    char_block = lines[char_i : endchar_i + 1]
    for j, ln in enumerate(char_block):
        if ln.startswith("name "):
            char_block[j] = "name DevTester"
            break

    vis = "1" * WORLD_ROOM_COUNT
    if len(vis) != WORLD_ROOM_COUNT:
        raise SystemExit("WORLD_ROOM_COUNT mismatch")

    inv = [
        "steel_shortsword",
        "wooden_buckler",
        "leather_cowl",
        "rusted_iron_mail",
        "woven_grass_shirt",
        "bone_ring",
        "heavy_scrap_axe",
        "leather_boots",
        "torn_trousers",
        "terrible_long_item_name",
        "sword",
        "bandage",
        "flint",
        "lockpick",
        "scrap_metal",
        "wood_scrap",
        "rusty_pick",
        "house_key",
    ]

    preamble = [
        "AET64SAVE1",
        "worldcnt 161",
        "room 15",
        "turns 0",
        "score 50",
        "coins 9999",
        "hp 100",
        "hpmax 100",
        "front 1",
        "shed 1",
        "roomv 1",
        "craftprof 9",
        "histn 0",
        vis,
        f"invn {len(inv)}",
        *inv,
    ]

    epilogue = [
        "NOTES",
        "1",
        "Dev save: r15 forge, 18-pack gear UI. aeternitas64 --save aeternitas64_dev_save.txt then restore",
        "READIED",
        "steel_shortsword",
        "EQUIP",
        "leather_cowl",
        "rusted_iron_mail",
        "",
        "torn_trousers",
        "leather_boots",
        "steel_shortsword",
        "wooden_buckler",
        "bone_ring",
        "REP 0 0 0 0 0 0 0",
        *char_block,
        "FOCUS",
        "",
        "TOPIC",
        "",
    ]

    out = "\n".join(preamble + rooms_block + epilogue) + "\n"
    DST.write_text(out, encoding="utf-8")
    print(f"Wrote {DST} ({len(out.splitlines())} lines)")


if __name__ == "__main__":
    main()
