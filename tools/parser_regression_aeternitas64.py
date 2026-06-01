#!/usr/bin/env python3
"""Scripted parser + NPC routine regression for aeternitas64.exe."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "aeternitas64.exe"
SCRIPT = Path(__file__).with_name("parser_regression.in")
OUT = Path(__file__).with_name("parser_regression.out")

# Each entry: (label, substring that must appear in full transcript)
CHECKS = [
    ("unlock locked chest with key", "You unlock the Locked Chest with the Skeleton Key"),
    ("open locked chest", "You open the Locked Chest"),
    ("take all from locked chest", "Taken from Locked Chest: House Key"),
    ("pronoun take it without focus", "Nothing in mind"),
    ("take scrap metal from ground", "Taken: Scrap Metal"),
    ("closed container take hint", "closed Mailbox"),
    ("open mailbox", "You open the Mailbox"),
    ("scan lists containers", "Containers:"),
    ("what is in mailbox examine", "Mailbox"),
    ("take leaflet from mailbox", "Taken from Mailbox: Orientation Leaflet"),
    ("nested open old box", "You open the Old Box"),
    ("take reed from nested box", "Taken from Old Box: Reed"),
    ("give off-room to miller", "leave Bandage at"),
    ("put scrap in mailbox", "You put Scrap Metal in the Mailbox"),
    ("take scrap from mailbox", "Taken from Mailbox: Scrap Metal"),
    ("dropped item take", "Taken: Scrap Metal"),
    ("scope query whats here", "AREA SCAN"),
    ("is there find rewrite", "lockpick"),
    ("ordinal take second pick", "Taken: Lockpick"),
    ("other pick after lockpick focus", "Taken: Rusty"),
    ("compare ordinal picks", "Likelier payday"),
    ("multi-word wood scrap + take it", "Taken: Wood"),
    ("who all next-period hints", "next ("),
    ("who all bartender routine", "• bartender"),
    ("npc schedule bartender", "Daily routine"),
    ("npc schedule miller", "Daily routine — miller"),
    ("where miller live lookup", "[miller]"),
    ("where blacksmith live lookup", "[blacksmith]"),
    ("container save quickload", "Inside: Old Box, Lockpick"),
]

def main() -> int:
    if not EXE.is_file():
        print(f"FAIL: missing {EXE} — run build_aeternitas64.bat first", file=sys.stderr)
        return 1
    if not SCRIPT.is_file():
        print(f"FAIL: missing {SCRIPT}", file=sys.stderr)
        return 1

    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env["MGT_AUTOTEST"] = "1"

    save_path = ROOT / "aeternitas64_save.txt"
    if save_path.is_file():
        save_path.unlink()

    with SCRIPT.open("rb") as stdin_f, OUT.open("wb") as out_f:
        proc = subprocess.run(
            [str(EXE)],
            stdin=stdin_f,
            stdout=out_f,
            stderr=subprocess.STDOUT,
            cwd=str(ROOT),
            env=env,
            timeout=180,
            check=False,
        )
    if proc.returncode != 0:
        print(f"FAIL: aeternitas64.exe exit {proc.returncode}", file=sys.stderr)
        return 1

    text = OUT.read_text(encoding="utf-8", errors="replace")
    failed: list[str] = []

    for name, needle in CHECKS:
        if needle not in text:
            failed.append(name)

    if failed:
        print("FAIL: parser regression")
        for name in failed:
            print(f"  - {name}")
        print(f"\nTranscript: {OUT}", file=sys.stderr)
        return 1

    print(f"PASS: parser regression ({len(CHECKS)} checks)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
