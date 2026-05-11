#!/usr/bin/env python3
"""Compare WebEdition item slugs with the C item catalog (maintenance aid; no network)."""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))
from repo_paths import ROOT, TOOLS, require_under_root, web_edition_index  # noqa: E402

CATALOG = ROOT / "aeternitas_item_catalog.c"


def extract_web_items(html: str) -> dict[str, str]:
    start = html.find("this.items = {")
    end = html.find("this.entities = {", start)
    if start < 0 or end < 0:
        raise SystemExit("Could not find this.items / this.entities bounds")
    chunk = html[start:end]
    out: dict[str, str] = {}
    current_slug: str | None = None
    depth = 0
    for line in chunk.splitlines():
        m = re.match(r'^(\s*)"([a-zA-Z0-9_]+)":\s*\{\s*$', line)
        if m and m.group(1).count(" ") >= 8:
            current_slug = m.group(2)
            depth = 1
            continue
        if current_slug is None:
            continue
        depth += line.count("{") - line.count("}")
        dm = re.search(r'description:\s*"((?:[^"\\]|\\.)*)"', line)
        if dm and current_slug not in out:
            raw = dm.group(1)
            out[current_slug] = bytes(raw, "utf-8").decode("unicode_escape")
        if depth <= 0:
            current_slug = None
            depth = 0
    return out


def extract_catalog_slug_set(c_src: str) -> set[str]:
    slugs: set[str] = set()
    for m in re.finditer(
        r'\{\s*(\d+)\s*,\s*"([a-zA-Z0-9_]+)"\s*,', c_src
    ):
        slugs.add(m.group(2))
    return slugs


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Audit item slug coverage between WebEdition index.html and the C catalog."
    )
    ap.add_argument(
        "--web",
        type=Path,
        default=web_edition_index(),
        help="Path to AeternitasWebEdition/index.html (or set AETERNITAS_WEB_INDEX).",
    )
    ap.add_argument(
        "--report",
        type=Path,
        default=TOOLS / "_web_item_audit_out.txt",
        help="Repo-local report path for web-only sample slugs.",
    )
    ap.add_argument(
        "--world-missing",
        action="store_true",
        help="Write world item slugs missing from the C catalog instead of web audit.",
    )
    args = ap.parse_args()

    if args.world_missing:
        return world_items_missing_catalog()

    web_path = args.web.expanduser()
    if not web_path.is_file():
        print(
            f"Missing web edition index: {web_path}\n"
            "Pass --web PATH or set AETERNITAS_WEB_INDEX.",
            file=sys.stderr,
        )
        return 1
    if not CATALOG.is_file():
        print(f"Missing C catalog: {CATALOG}", file=sys.stderr)
        return 1

    report = require_under_root(args.report, label="--report")
    web = extract_web_items(web_path.read_text(encoding="utf-8", errors="replace"))
    c = CATALOG.read_text(encoding="utf-8", errors="replace")
    c_slugs = extract_catalog_slug_set(c)

    only_web = sorted(web.keys() - c_slugs)
    only_c = sorted(c_slugs - web.keys())
    shared = sorted(web.keys() & c_slugs)

    print(f"Web items with string description: {len(web)}")
    print(f"C catalog slugs: {len(c_slugs)}")
    print(f"Shared: {len(shared)}")
    print(f"In web only (first 40): {only_web[:40]}{' ...' if len(only_web) > 40 else ''}")
    print(f"In C only (first 40): {only_c[:40]}{' ...' if len(only_c) > 40 else ''}")

    # Suggest additions: web items referenced in shared world food list etc.
    lines = []
    for s in only_web[:200]:
        desc = web[s][:200].replace("\n", " ")
        lines.append(f"{s}\t{desc}")
    report.write_text("\n".join(lines), encoding="utf-8")
    print(f"Wrote sample web-only list to {report}")
    return 0


def world_items_missing_catalog() -> int:
    if not CATALOG.is_file():
        print(f"Missing C catalog: {CATALOG}", file=sys.stderr)
        return 1
    world_path = ROOT / "aeternitas_world_generated.c"
    if not world_path.is_file():
        print(f"Missing generated world source: {world_path}", file=sys.stderr)
        return 1
    c = CATALOG.read_text(encoding="utf-8", errors="replace")
    world = world_path.read_text(encoding="utf-8", errors="replace")
    csl = set(re.findall(r"\{\s*\d+\s*,\s*\"([a-zA-Z0-9_]+)\"", c))
    sl: set[str] = set()
    for block in re.findall(
        r"WORLD_ITEM_LISTS_REC_\d+\[\]\s*=\s*\{([^}]+)\}", world, re.S
    ):
        sl |= set(re.findall(r"\"([a-z0-9_]+)\"", block))
    missing = sorted(sl - csl)
    p = ROOT / "tools" / "_world_items_missing_catalog.txt"
    p.write_text("\n".join(missing), encoding="utf-8")
    print(f"World item slugs: {len(sl)}; missing from catalog: {len(missing)} -> {p}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
