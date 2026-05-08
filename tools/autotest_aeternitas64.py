#!/usr/bin/env python3
from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "aeternitas64.exe"


def build() -> None:
    sources = [
        "aeternitas64_ascii.c",
        "aeternitas_item_catalog.c",
        "aeternitas_world_generated.c",
        "aeternitas_char_creation.c",
        "aeternitas_mod_guide.c",
        "aeternitas_char_description.c",
        "aeternitas_mods.c",
        "aeternitas_mod_bootstrap.c",
    ]
    cmd = [
        "gcc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Os",
        "-s",
        "-o",
        str(EXE),
        *sources,
    ]
    p = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True)
    if p.returncode != 0:
        print("BUILD FAILED\n")
        print(p.stdout)
        print(p.stderr)
        print(
            "Hint: if enabling pickers on Windows, compile with "
            "-DAETER_WIN_PICKERS -lshell32 -lcomdlg32 -lole32"
        )
        raise SystemExit(p.returncode)


def run_smoke() -> None:
    lines = [
        "new",
        "name Test",
        "help",
        "who is here",
        "who did i last meet",
        "last npc attitude",
        "npc danger",
        "npc trust",
        "npc leverage",
        "topic",
        "topic mood",
        "topic heat",
        "inventory",
        "status",
        "save",
        "quit",
        "y",
    ]
    script = "\n".join(lines) + "\n"
    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env.setdefault("NO_COLOR", "1")
    with tempfile.TemporaryDirectory(prefix="aet-autotest-") as td:
        save_path = Path(td) / "save.txt"
        p = subprocess.run(
            [str(EXE), "--save", str(save_path)],
            input=script,
            cwd=ROOT,
            env=env,
            text=True,
            capture_output=True,
            timeout=180,
        )
    if p.returncode != 0:
        print("RUNTIME SMOKE FAILED\n")
        print(p.stdout)
        print(p.stderr)
        raise SystemExit(p.returncode)


def main() -> int:
    build()
    run_smoke()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
