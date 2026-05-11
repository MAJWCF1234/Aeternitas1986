#!/usr/bin/env python3
"""List or remove ephemeral recovery logs and scratch transcripts (see patterns in main).

Canonical data lives under ``recovery_artifacts/txts/`` (JSON, symbol dumps). This script
does not delete those. It does not touch baseline EXEs or world_tables_recovered.json*.

Dry-run is the default; pass ``--apply`` to delete matched paths.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))
from repo_paths import ROOT, require_under_root  # noqa: E402

KEEP_NAMES = frozenset(
    {
        "world_tables_recovered.json",
        "world_tables_recovered.json.bak",
        "exe_symbols.txt",
        "exe_strings.txt",
        "exe_disasm_all.txt",
        "dwarf_decoded_lines.txt",
        "dwarf_info_index.txt",
        "char_creation_disasm.txt",
        "char_creation_symbols.txt",
        "char_description_disasm.txt",
        "char_description_symbols.txt",
        "recovery_index.json",
        "RECOVERY_INDEX.md",
        "help_modding_orig.txt",
        "aeternitas64_save.txt",
    }
)


def _safe_path(path: Path) -> Path:
    return require_under_root(path, label="cleanup target")


def _delete_file(path: Path, *, apply: bool) -> None:
    target = _safe_path(path)
    if apply:
        target.unlink()
        print("deleted", target.relative_to(ROOT))
    else:
        print("would delete", target.relative_to(ROOT))


def _delete_dir(path: Path, *, apply: bool) -> None:
    target = _safe_path(path)
    if apply:
        shutil.rmtree(target)
        print("removed dir", target.relative_to(ROOT))
    else:
        print("would remove dir", target.relative_to(ROOT))


def main() -> int:
    ap = argparse.ArgumentParser(description="Clean ephemeral recovery scratch files.")
    ap.add_argument(
        "--apply",
        action="store_true",
        help="Actually delete matched paths. Without this, only prints what would be removed.",
    )
    args = ap.parse_args()

    removed = 0
    for p in ROOT.glob("tools/_describe_*.txt"):
        _delete_file(p, apply=args.apply)
        removed += 1

    art = ROOT / "recovery_artifacts"
    for d in sorted(art.glob("assistant_temp_recovery_*")):
        if d.is_dir():
            _delete_dir(d, apply=args.apply)
            removed += 1

    txts = art / "txts"
    if txts.is_dir():
        for p in sorted(txts.iterdir()):
            if not p.is_file():
                continue
            name = p.name
            if name in KEEP_NAMES:
                continue
            if (
                name.startswith("smoke_")
                or name.startswith("tmp_")
                or name.startswith("trace_")
                or name.startswith("raw_original_")
                or name.startswith("raw_recovered_")
                or name
                in (
                    "raw_turns_input.txt",
                    "raw_char_defaults_input.txt",
                    "recovered_exits_crash_stdout.txt",
                    "trace_run_log.txt",
                    "help_modding_rec.txt",
                    "exe_rdata_dump.txt",
                )
            ):
                _delete_file(p, apply=args.apply)
                removed += 1

    verb = "removed" if args.apply else "matched"
    print(f"OK: {verb} {removed} path(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
