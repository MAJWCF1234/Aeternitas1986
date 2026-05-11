#!/usr/bin/env python3
"""Validate generated world table consistency (room count vs slugs, duplicate slugs).

Runs with `py -3 tools/validate_world_data.py` from the repo root. Exit 0 on success.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))
from repo_paths import ROOT, world_tables_recovered_json  # noqa: E402


def _read_world_room_count(header_text: str) -> int:
    m = re.search(r"^\s*#define\s+WORLD_ROOM_COUNT\s+(\d+)\s*$", header_text, re.MULTILINE)
    if not m:
        print("error: could not find #define WORLD_ROOM_COUNT in header", file=sys.stderr)
        sys.exit(1)
    return int(m.group(1))


def _extract_world_slugs_from_c(c_text: str) -> list[str]:
    marker = "static const char *const WORLD_SLUGS_REC"
    if marker not in c_text:
        print("error: WORLD_SLUGS_REC array not found in generated C source", file=sys.stderr)
        sys.exit(1)
    lines = c_text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if marker in line and "[" in line:
            start = i + 1
            break
    if start is None:
        print("error: could not locate start of WORLD_SLUGS_REC initializer", file=sys.stderr)
        sys.exit(1)

    slugs: list[str] = []
    slug_line = re.compile(r'^\s*"([^"]*)"\s*,?\s*$')
    for line in lines[start:]:
        s = line.strip()
        if s == "};":
            break
        m = slug_line.match(line)
        if not m:
            print(f"error: unexpected line in WORLD_SLUGS_REC block: {line!r}", file=sys.stderr)
            sys.exit(1)
        slugs.append(m.group(1))
    return slugs


def _dup_indices(values: list[str]) -> dict[str, list[int]]:
    seen: dict[str, list[int]] = {}
    for i, v in enumerate(values):
        seen.setdefault(v, []).append(i)
    return {k: idx for k, idx in seen.items() if len(idx) > 1}


def main() -> None:
    ap = argparse.ArgumentParser(description="Validate world slug tables vs WORLD_ROOM_COUNT.")
    ap.add_argument(
        "--header",
        type=Path,
        default=ROOT / "aeternitas_world_generated.h",
        help="Path to aeternitas_world_generated.h",
    )
    ap.add_argument(
        "--generated-c",
        type=Path,
        default=ROOT / "aeternitas_world_generated.c",
        help="Path to aeternitas_world_generated.c",
    )
    ap.add_argument(
        "--json",
        type=Path,
        default=None,
        help="Optional world_tables_recovered.json (default: recovery path if present)",
    )
    args = ap.parse_args()

    header_path = args.header
    c_path = args.generated_c
    json_path = args.json if args.json is not None else world_tables_recovered_json()

    if not header_path.is_file():
        print(f"error: missing header: {header_path}", file=sys.stderr)
        sys.exit(1)
    if not c_path.is_file():
        print(f"error: missing generated C: {c_path}", file=sys.stderr)
        sys.exit(1)

    room_count = _read_world_room_count(header_path.read_text(encoding="utf-8"))
    slugs = _extract_world_slugs_from_c(c_path.read_text(encoding="utf-8"))

    if len(slugs) != room_count:
        print(
            f"error: WORLD_ROOM_COUNT is {room_count} but WORLD_SLUGS_REC has {len(slugs)} entries",
            file=sys.stderr,
        )
        sys.exit(1)

    dups = _dup_indices(slugs)
    if dups:
        for slug, idxs in sorted(dups.items()):
            print(f"error: duplicate slug {slug!r} at indices {idxs}", file=sys.stderr)
        sys.exit(1)

    if json_path.is_file():
        data = json.loads(json_path.read_text(encoding="utf-8"))
        rc = int(data.get("room_count", -1))
        js_slugs = data.get("slugs")
        if not isinstance(js_slugs, list) or not all(isinstance(x, str) for x in js_slugs):
            print("error: JSON 'slugs' must be a list of strings", file=sys.stderr)
            sys.exit(1)
        if rc != room_count:
            print(
                f"error: JSON room_count {rc} != header WORLD_ROOM_COUNT {room_count}",
                file=sys.stderr,
            )
            sys.exit(1)
        if len(js_slugs) != room_count:
            print(
                f"error: JSON has {len(js_slugs)} slugs but room_count is {room_count}",
                file=sys.stderr,
            )
            sys.exit(1)
        jdups = _dup_indices(js_slugs)
        if jdups:
            for slug, idxs in sorted(jdups.items()):
                print(f"error: duplicate slug in JSON {slug!r} at indices {idxs}", file=sys.stderr)
            sys.exit(1)
        if js_slugs != slugs:
            for i, (a, b) in enumerate(zip(slugs, js_slugs)):
                if a != b:
                    print(
                        f"error: slug mismatch at index {i}: C has {a!r}, JSON has {b!r}",
                        file=sys.stderr,
                    )
                    sys.exit(1)
            # zip shorter — already caught length
        print(
            f"ok: {room_count} rooms, slugs unique; JSON {json_path.name} matches generated C"
        )
    else:
        print(f"ok: {room_count} rooms, slugs unique (no {json_path.name} to cross-check)")


if __name__ == "__main__":
    main()
