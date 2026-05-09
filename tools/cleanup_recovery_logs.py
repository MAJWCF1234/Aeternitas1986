#!/usr/bin/env python3
"""Remove ephemeral recovery logs and scratch transcripts (see patterns in main).

Canonical data lives under ``recovery_artifacts/txts/`` (JSON, symbol dumps). This script
does not delete those. It does not touch baseline EXEs or world_tables_recovered.json*."""

from __future__ import annotations

import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

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


def main() -> int:
    removed = 0
    for p in ROOT.glob("tools/_describe_*.txt"):
        p.unlink()
        removed += 1
        print("deleted", p.relative_to(ROOT))

    art = ROOT / "recovery_artifacts"
    for d in sorted(art.glob("assistant_temp_recovery_*")):
        if d.is_dir():
            shutil.rmtree(d)
            removed += 1
            print("removed dir", d.relative_to(ROOT))

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
                p.unlink()
                removed += 1
                print("deleted", p.relative_to(ROOT))

    print(f"OK: removed {removed} path(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
