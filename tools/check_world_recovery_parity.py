#!/usr/bin/env python3
"""Compare NPC line-set and merchant slug tables vs recovered JSON (when present).

Primary path (``world_tables_recovered.json`` present): ordered slug lists in
``aeternitas_world_generated.c`` for ``AET_NPC_LINES_REC`` and ``AET_MERCHANTS_REC``
must match ``npc_lines`` and ``merchants`` in the JSON.

Fallback when JSON is absent: ``WORLD_ROOM_COUNT`` in ``aeternitas_world_generated.h``
must equal the number of string entries in ``WORLD_SLUGS_REC`` in the generated C.
(``world_slug(`` occurs only once in that file as the accessor; the slug table drives room parity.)

Read-only. Missing header or generated C: print a skip message and exit 0. No JSON (or
unreadable ``WORLD_SLUGS_REC`` in fallback): skip with exit 0 where noted below.
NPC/merchant mismatch vs JSON: exit 1.

Run: ``py -3 tools/check_world_recovery_parity.py`` from repo root.
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


def _read_world_room_count(header_text: str) -> int | None:
    m = re.search(r"^\s*#define\s+WORLD_ROOM_COUNT\s+(\d+)\s*$", header_text, re.MULTILINE)
    return int(m.group(1)) if m else None


def _count_world_slugs_rec(c_text: str) -> int | None:
    marker = "static const char *const WORLD_SLUGS_REC"
    if marker not in c_text:
        return None
    lines = c_text.splitlines()
    start = None
    for i, line in enumerate(lines):
        if marker in line and "[" in line:
            start = i + 1
            break
    if start is None:
        return None
    n = 0
    slug_line = re.compile(r'^\s*"([^"]*)"\s*,?\s*$')
    for line in lines[start:]:
        if line.strip() == "};":
            break
        if slug_line.match(line):
            n += 1
        else:
            return None
    return n


def _extract_table_slugs_line_based(c_text: str, array_marker: str) -> list[str] | None:
    """Parse ``... ARRAY[] = {`` rows ``{"slug", ...},`` through closing ``};``."""
    if array_marker not in c_text:
        return None
    lines = c_text.splitlines()
    in_array = False
    slugs: list[str] = []
    row_re = re.compile(r'^\s*\{\s*"([^"]+)"\s*,')
    for line in lines:
        if not in_array:
            if array_marker in line and "[" in line and "=" in line and "{" in line:
                in_array = True
            continue
        st = line.strip()
        if st.startswith("};"):
            break
        if not st or st.startswith("//"):
            continue
        m = row_re.match(line)
        if m:
            slugs.append(m.group(1))
    return slugs if slugs else None


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Parity check: NPC/merchant slug tables vs recovered JSON; "
        "fallback room slug count vs WORLD_ROOM_COUNT."
    )
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
        help="world_tables_recovered.json (default: recovery path if present)",
    )
    args = ap.parse_args()

    header_path = args.header
    c_path = args.generated_c
    json_path = args.json if args.json is not None else world_tables_recovered_json()

    if not header_path.is_file():
        print(f"skip: missing header (expected generated world header): {header_path}")
        sys.exit(0)
    if not c_path.is_file():
        print(f"skip: missing generated world C: {c_path}")
        sys.exit(0)

    c_text = c_path.read_text(encoding="utf-8")

    if json_path.is_file():
        try:
            data = json.loads(json_path.read_text(encoding="utf-8"))
        except json.JSONDecodeError as e:
            print(f"error: invalid JSON in {json_path}: {e}", file=sys.stderr)
            sys.exit(1)
        if "npc_lines" not in data or "merchants" not in data:
            print(
                f"error: JSON missing 'npc_lines' or 'merchants': {json_path}",
                file=sys.stderr,
            )
            sys.exit(1)

        npc_json = [str(x["slug"]) for x in data["npc_lines"]]
        mer_json = [str(x["slug"]) for x in data["merchants"]]

        npc_c = _extract_table_slugs_line_based(c_text, "AET_NPC_LINES_REC[]")
        mer_c = _extract_table_slugs_line_based(c_text, "AET_MERCHANTS_REC[]")

        if npc_c is None:
            print("error: could not parse AET_NPC_LINES_REC[] in generated C", file=sys.stderr)
            sys.exit(1)
        if mer_c is None:
            print("error: could not parse AET_MERCHANTS_REC[] in generated C", file=sys.stderr)
            sys.exit(1)

        ok = True
        if npc_json != npc_c:
            ok = False
            print("error: NPC line-set slug list mismatch (JSON order vs generated C)", file=sys.stderr)
            for i, (a, b) in enumerate(zip(npc_json, npc_c)):
                if a != b:
                    print(f"  index {i}: JSON={a!r} C={b!r}", file=sys.stderr)
                    break
            lj, lc = len(npc_json), len(npc_c)
            if lj != lc:
                print(f"  length: JSON={lj} C={lc}", file=sys.stderr)

        if mer_json != mer_c:
            ok = False
            print("error: merchant slug list mismatch (JSON order vs generated C)", file=sys.stderr)
            for i, (a, b) in enumerate(zip(mer_json, mer_c)):
                if a != b:
                    print(f"  index {i}: JSON={a!r} C={b!r}", file=sys.stderr)
                    break
            lj, lc = len(mer_json), len(mer_c)
            if lj != lc:
                print(f"  length: JSON={lj} C={lc}", file=sys.stderr)

        if not ok:
            sys.exit(1)

        print(
            f"ok: npc_lines ({len(npc_json)}), merchants ({len(mer_json)}) slug order matches "
            f"{json_path.name}"
        )
        sys.exit(0)

    header_text = header_path.read_text(encoding="utf-8")
    room_count = _read_world_room_count(header_text)
    if room_count is None:
        print("skip: WORLD_ROOM_COUNT not found in header", file=sys.stderr)
        sys.exit(0)

    # Fallback: no JSON — room slug table vs WORLD_ROOM_COUNT
    print(f"note: no {json_path.name}; using fallback room slug count vs WORLD_ROOM_COUNT")
    n_slugs = _count_world_slugs_rec(c_text)
    if n_slugs is None:
        print("skip: could not parse WORLD_SLUGS_REC in generated C")
        sys.exit(0)

    world_slug_calls = len(re.findall(r"\bworld_slug\s*\(", c_text))
    if world_slug_calls != 1:
        print(
            f"note: expected exactly one world_slug( definition in generated C, found {world_slug_calls}",
            file=sys.stderr,
        )

    if n_slugs != room_count:
        print(
            f"error: WORLD_ROOM_COUNT is {room_count} but WORLD_SLUGS_REC has {n_slugs} entries",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"ok: WORLD_ROOM_COUNT {room_count} matches WORLD_SLUGS_REC ({n_slugs} slugs)")
    sys.exit(0)


if __name__ == "__main__":
    main()
