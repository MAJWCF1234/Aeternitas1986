#!/usr/bin/env python3
"""
List or delete known multi-hundred-MB recovery trace files that inflate the tree.

Default: print paths and sizes (dry-run). Use --delete to remove after review.
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parent.parent


DEFAULT_RELATIVE_BULK = (
    "recovery_artifacts/raw_original_path2.txt",
    "recovery_artifacts/raw_original_turns.txt",
)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--delete",
        action="store_true",
        help="Delete listed files that exist (irreversible).",
    )
    args = ap.parse_args()
    root = repo_root()
    total = 0
    for rel in DEFAULT_RELATIVE_BULK:
        p = root / rel
        if not p.is_file():
            print(f"missing: {rel}")
            continue
        sz = p.stat().st_size
        total += sz
        print(f"{sz:>14}  {rel}")
        if args.delete:
            p.unlink()
            print(f"  deleted: {rel}")
    if total:
        print(f"total listed bytes: {total}")
    if args.delete and total == 0:
        print("nothing deleted (no files present).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
