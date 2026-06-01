#!/usr/bin/env python3
"""Natural-language parser stress test for aeternitas64.exe.

Focus: realistic player wording, filler text, typos, pronouns, and indirect asks.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "aeternitas64.exe"
OUT = Path(__file__).with_name("parser_nl_stress.out")

COMMANDS = [
    "1",
    "hey can you show me what's in the mailbox please?",
    "could you open the mailbox for me",
    "can you check what is in mailbox",
    "grab the leaflet from there",
    "drop it",
    "take the thing i dropped",
    "where can i go from here?",
    "go north",
    "can you tell me who is here",
    "talk to him",
    "what should i do next",
    "go south",
    "i wanna look around",
    "what can i see",
    "is there a lockpick here?",
    "pick up the lockpick",
    "head east",
    "please pick lock",
    "who noticed me",
    "go west",
    "show me what is in mailbox",
    "put lockpick in the mailbox",
    "show me what's in mailbox",
    "take lockpick from mailbox",
    "could you tell me who is here?",
    "where am i now",
    "show exits",
    "go norrth",
    "go sout",
    "look at the old box",
    "open old box",
    "take reed from old box",
    "inventory please",
    "what do i have on me",
    "quick save",
    "quick load",
    "quit",
]

REQUIRED = [
    "West of House",
    "Mailbox",
    "Exits",
    "Taken",
    "Present",
]

BAD = [
    "Unknown command.",
    "Go where?",
]


def run() -> int:
    if not EXE.is_file():
        print(f"FAIL: missing {EXE} — run build_aeternitas64.bat first", file=sys.stderr)
        return 1

    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env["MGT_AUTOTEST"] = "1"

    script = ("\n".join(COMMANDS) + "\n").encode("utf-8")
    proc = subprocess.run(
        [str(EXE)],
        input=script,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=str(ROOT),
        env=env,
        timeout=240,
        check=False,
    )
    OUT.write_bytes(proc.stdout)
    text = proc.stdout.decode("utf-8", errors="replace")

    if proc.returncode != 0:
        print(f"FAIL: game exited {proc.returncode}", file=sys.stderr)
        print(f"Transcript: {OUT}", file=sys.stderr)
        return 1

    missing = [needle for needle in REQUIRED if needle not in text]
    bad_hits = [needle for needle in BAD if needle in text]
    menu_unknown = text.count("Unknown choice. Use 1")

    if missing or bad_hits or menu_unknown > 0:
        print("FAIL: parser natural-language stress")
        if missing:
            print("Missing expected behaviors:")
            for m in missing:
                print(f"  - {m}")
        if bad_hits:
            print("Parser hard-fail markers seen:")
            for b in bad_hits:
                print(f"  - {b}")
        if menu_unknown > 0:
            print(f"Menu-state mismatch detected: {menu_unknown} unknown menu choice lines")
        print(f"\nTranscript: {OUT}", file=sys.stderr)
        return 1

    print(f"PASS: parser natural-language stress ({len(COMMANDS)} commands)")
    return 0


if __name__ == "__main__":
    raise SystemExit(run())
