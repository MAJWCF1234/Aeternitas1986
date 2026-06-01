#!/usr/bin/env python3
"""Deeper end-to-end playthrough evaluator for aeternitas64.exe."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "aeternitas64.exe"
OUT = Path(__file__).with_name("deep_playthrough.out")
INP = Path(__file__).with_name("deep_playthrough.in")

COMMANDS = [
    "1",
    "scan",
    "open mailbox",
    "take leaflet from mailbox",
    "drop leaflet",
    "take dropped",
    "unlock locked chest with skeleton key",
    "open it",
    "take all from it",
    "go north",
    "go east",
    "go west",
    "go north",
    "go north",
    "who",
    "talk to him about work",
    "where miller",
    "qs",
    "ql",
    "status",
    "inventory list",
    "quit",
]

CHECKS = [
    ("intro room", "West of House"),
    ("mailbox open", "You open the Mailbox"),
    ("mailbox take", "Taken from Mailbox: Orientation Leaflet"),
    ("dropped memory", "Taken: Orientation Leaflet"),
    ("unlock chest", "You unlock the Locked Chest with the Skeleton Key"),
    ("open pronoun", "You open the Locked Chest"),
    ("npc present", "Present:"),
    ("where output", "[miller]"),
    ("save/restore", "restored from"),
    ("inventory panel", "Pack list"),
    ("clean exit", "Thanks for playing."),
]


def run() -> int:
    if not EXE.is_file():
        print(f"FAIL: missing {EXE} (build first)", file=sys.stderr)
        return 1

    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env["MGT_AUTOTEST"] = "1"

    INP.write_text("\n".join(COMMANDS) + "\n", encoding="utf-8")
    try:
        with INP.open("rb") as stdin_f:
            proc = subprocess.run(
                [str(EXE)],
                stdin=stdin_f,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                cwd=str(ROOT),
                env=env,
                timeout=180,
                check=False,
            )
        OUT.write_bytes(proc.stdout)
        text = proc.stdout.decode("utf-8", errors="replace")
    except subprocess.TimeoutExpired as ex:
        OUT.write_bytes(ex.stdout or b"")
        print("FAIL: deep playthrough timed out", file=sys.stderr)
        print(f"Transcript: {OUT}", file=sys.stderr)
        return 1

    if proc.returncode != 0:
        print(f"FAIL: game exited {proc.returncode}", file=sys.stderr)
        print(f"Transcript: {OUT}", file=sys.stderr)
        return 1

    missing = [name for name, needle in CHECKS if needle not in text]
    hard_fail = []
    for bad in ("Unknown command.", "Go where?"):
        if bad in text:
            hard_fail.append(bad)

    if missing or hard_fail:
        print("FAIL: deep playthrough")
        if missing:
            print("Missing milestones:")
            for m in missing:
                print(f"  - {m}")
        if hard_fail:
            print("Hard parser failures observed:")
            for h in hard_fail:
                print(f"  - {h}")
        print(f"\nTranscript: {OUT}", file=sys.stderr)
        return 1

    print(f"PASS: deep playthrough ({len(COMMANDS)} commands, {len(CHECKS)} milestones)")
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
