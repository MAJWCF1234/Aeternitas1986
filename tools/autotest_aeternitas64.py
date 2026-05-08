#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
_TOOLS = Path(__file__).resolve().parent
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
    extra_objs: list[str] = []
    if platform.system() == "Windows":
        windres = shutil.which("windres")
        manifest_o = ROOT / "aeternitas64_manifest.o"
        rc = ROOT / "aeternitas64_manifest.rc"
        if windres and rc.is_file():
            wr = subprocess.run(
                [
                    windres,
                    str(rc),
                    "-O",
                    "coff",
                    "-o",
                    str(manifest_o),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
            )
            if wr.returncode == 0 and manifest_o.is_file():
                extra_objs.append(str(manifest_o))
            else:
                print(
                    "NOTE: windres failed; exe built without UTF-8 manifest "
                    "(console mojibake may persist).",
                    file=sys.stderr,
                )
                if wr.stderr:
                    print(wr.stderr, file=sys.stderr)
        else:
            print(
                "NOTE: windres not found or missing aeternitas64_manifest.rc; "
                "linking without UTF-8 manifest.",
                file=sys.stderr,
            )

    cmd = [
        "gcc",
        "-std=c11",
        "-Wall",
        "-Wextra",
        "-Os",
        "-s",
        "-finput-charset=UTF-8",
        "-fexec-charset=UTF-8",
        *extra_objs,
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


def run_script(lines: list[str], *, timeout: int, label: str) -> None:
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
            timeout=timeout,
        )
    if p.returncode != 0:
        print(f"RUNTIME FAILED ({label}, {len(lines)} commands)\n")
        print(p.stdout)
        print(p.stderr)
        raise SystemExit(p.returncode)
    print(f"OK: {label} ({len(lines)} commands, rc=0)")


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
        "done",
        "status",
        "save",
        "quit",
        "y",
    ]
    run_script(lines, timeout=180, label="smoke")


def run_full_playthrough(*, include_all_rooms: bool) -> None:
    """Global command surface + UI surfaces (forge / equipment / save menus).

    With ``include_all_rooms``, also runs a shallow visit of every room slug from
    ``recovery_artifacts/world_tables_recovered.json`` (same script as golden harvest).
    """
    if str(_TOOLS) not in sys.path:
        sys.path.insert(0, str(_TOOLS))
    from harvest_original_playthrough import (
        build_global_script,
        build_room_script,
        build_ui_surfaces_script,
    )

    run_script(build_global_script(), timeout=360, label="global playthrough")
    run_script(build_ui_surfaces_script(), timeout=360, label="ui surfaces")
    if include_all_rooms:
        data_path = ROOT / "recovery_artifacts" / "world_tables_recovered.json"
        if not data_path.is_file():
            print(f"SKIP: all-rooms (missing {data_path})", file=sys.stderr)
            return
        data = json.loads(data_path.read_text(encoding="utf-8"))
        slugs = data.get("slugs") or []
        run_script(
            build_room_script(slugs, per_room=False),
            timeout=900,
            label=f"all rooms ({len(slugs)} slugs)",
        )


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Compile aeternitas64.exe and run stdin playthrough tests."
    )
    ap.add_argument(
        "--quick",
        action="store_true",
        help="Short smoke only (skip global / UI / rooms)",
    )
    ap.add_argument(
        "--no-rooms",
        action="store_true",
        help="Skip the all-room route/look sweep (faster; still runs global + UI surfaces)",
    )
    args = ap.parse_args()

    build()
    if args.quick:
        run_smoke()
    else:
        run_full_playthrough(include_all_rooms=not args.no_rooms)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
