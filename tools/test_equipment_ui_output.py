#!/usr/bin/env python3
from __future__ import annotations

import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path


ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")


def strip_ansi(s: str) -> str:
    return ANSI_RE.sub("", s)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def default_exe(root: Path) -> Path:
    for name in ("aeternitas64.exe", "aeternitas64"):
        p = root / name
        if p.is_file():
            return p
    return root / "aeternitas64.exe"


def load_autotest_script(root: Path) -> str:
    sys.path.insert(0, str(root / "tools"))
    import autotest_aeternitas64  # type: ignore

    return autotest_aeternitas64.build_script()


def main() -> int:
    root = repo_root()
    exe = default_exe(root)
    if not exe.is_file():
        print(f"FAIL: executable not found: {exe}", file=sys.stderr)
        return 1

    script = load_autotest_script(root)
    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"

    with tempfile.TemporaryDirectory(prefix="aet-eq-ui-") as td:
        save_path = Path(td) / "ui_test_save.txt"
        try:
            proc = subprocess.run(
                [str(exe), "--save", str(save_path)],
                input=script,
                text=True,
                capture_output=True,
                cwd=root,
                env=env,
                timeout=120,
            )
        except subprocess.TimeoutExpired:
            print("FAIL: execution timed out", file=sys.stderr)
            return 2

    if proc.returncode != 0:
        print(f"FAIL: game exited with {proc.returncode}", file=sys.stderr)
        return 3

    out = strip_ansi(proc.stdout.replace("\r", ""))
    lines = out.split("\n")

    title_ix = -1
    for i, ln in enumerate(lines):
        if "E Q U I P M E N T   &   I N V E N T O R Y" in ln:
            title_ix = i
            break
    if title_ix < 0:
        print("FAIL: equipment screen title not found", file=sys.stderr)
        return 4

    window = lines[title_ix : min(len(lines), title_ix + 64)]
    width_violations = []
    exact_120_rules = 0
    has_commands = False
    has_slot_header = False
    has_inventory_header = False

    for ln in window:
        v = len(ln)
        if v > 120:
            width_violations.append(v)
        if "=" * 120 in ln:
            exact_120_rules += 1
        if "COMMANDS:[EQUIP #]" in ln:
            has_commands = True
        if "[ EQUIPMENT SLOTS ]" in ln:
            has_slot_header = True
        if "[ INVENTORY DATA ]" in ln:
            has_inventory_header = True

    if width_violations:
        print(
            "FAIL: equipment window exceeded width 120; lengths: "
            + ", ".join(str(v) for v in width_violations[:10]),
            file=sys.stderr,
        )
        return 5
    if exact_120_rules < 2:
        print("FAIL: did not detect expected 120-char rule lines", file=sys.stderr)
        return 6
    if not has_commands:
        print("FAIL: command strip not found", file=sys.stderr)
        return 7
    if not (has_slot_header and has_inventory_header):
        print("FAIL: panel headers missing", file=sys.stderr)
        return 8

    print("PASS: equipment UI output looks width-safe and structurally correct.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
