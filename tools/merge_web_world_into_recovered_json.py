#!/usr/bin/env python3
"""Merge Aeternitas Web Edition map extract into recovery_artifacts/world_tables_recovered.json.

Reads ``world_map_data.js`` (``window.WORLD_MAP_DATA = { ... };``), matches rooms by slug to
the existing recovered JSON (same 161-room order and indices), and overwrites titles, blurbs,
regions, per-direction exits, and visible item lists. Preserves npc_lines, merchants,
hidden_item_lists, consume IDs, quest_hints, and dark flags unless you add logic later.

Typical flow after updating web ``index.html``::

    cd D:\\light\\AeternitasWebEdition && node generate_world_map_data.js
    py tools\\merge_web_world_into_recovered_json.py --web-root ..\\AeternitasWebEdition
    py tools\\build_world_c_from_recovered_json.py

If the sibling ``AeternitasWebEdition`` repo is absent, use the bundled snapshot
(``recovery_artifacts/web_edition_snapshot/world_map_data.js``) — it is tried
automatically when ``--web-root`` is omitted.

Direction labels match ``DIR_LABELS`` in ``aeternitas64_ascii.c`` (DIR_COUNT == 19).
Extra exit keys in the web map that do not match a known label are skipped with a warning.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

# Must stay aligned with aeternitas64_ascii.c DIR_LABELS / enum order.
DIR_LABELS: tuple[str, ...] = (
    "north",
    "south",
    "east",
    "west",
    "up",
    "down",
    "northeast",
    "northwest",
    "southeast",
    "southwest",
    "in",
    "out",
    "deeper",
    "upstream",
    "downstream",
    "fountain",
    "stage",
    "board",
    "square",
)

LABEL_TO_DIR: dict[str, int] = {lab: i for i, lab in enumerate(DIR_LABELS)}

# Web/source aliases occasionally seen in HTML bundles.
ALIAS_EXIT_KEYS: dict[str, str] = {
    "n": "north",
    "s": "south",
    "e": "east",
    "w": "west",
    "u": "up",
    "d": "down",
    "enter": "in",
    "inside": "in",
    "outside": "out",
    "leave": "out",
    "exit": "out",
}


def load_world_map_js(path: Path) -> dict:
    """``world_map_data.js`` is ``window.WORLD_MAP_DATA = <JSON>;`` — JSON-only payload."""
    raw = path.read_text(encoding="utf-8", errors="replace")
    eq = raw.index("=")
    sub = raw[eq + 1 :].strip()
    if sub.endswith(";"):
        sub = sub[:-1].strip()
    return json.loads(sub)


def normalize_exit_key(k: str) -> str | None:
    k2 = k.strip().lower().replace(" ", "_")
    k2 = ALIAS_EXIT_KEYS.get(k2, k2)
    if k2 in LABEL_TO_DIR:
        return k2
    return None


def pick_web_root(repo_root: Path, explicit: Path | None) -> Path:
    """Resolve folder containing ``world_map_data.js`` (live web or snapshot)."""
    if explicit is not None:
        p = explicit.resolve()
        if not (p / "world_map_data.js").is_file():
            raise SystemExit(f"missing {p / 'world_map_data.js'}")
        return p
    sibling = repo_root.parent / "AeternitasWebEdition"
    if (sibling / "world_map_data.js").is_file():
        return sibling.resolve()
    snap = repo_root / "recovery_artifacts" / "web_edition_snapshot"
    if (snap / "world_map_data.js").is_file():
        return snap.resolve()
    raise SystemExit(
        "world_map_data.js not found. Pass --web-root, clone AeternitasWebEdition "
        "next to this repo, or keep recovery_artifacts/web_edition_snapshot/."
    )


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument(
        "--web-root",
        type=Path,
        default=None,
        help="Folder containing world_map_data.js (default: sibling web edition or snapshot)",
    )
    ap.add_argument(
        "--dry-run",
        action="store_true",
        help="Report counts only; do not write JSON.",
    )
    ap.add_argument(
        "--compact",
        action="store_true",
        help="Write minified JSON (smaller on disk; harder to diff by hand).",
    )
    args = ap.parse_args()

    root = Path(__file__).resolve().parents[1]
    web_root = pick_web_root(root, args.web_root)
    js_path = web_root / "world_map_data.js"

    data_path = root / "recovery_artifacts" / "world_tables_recovered.json"
    backup_path = data_path.with_suffix(".json.bak")
    d = json.loads(data_path.read_text(encoding="utf-8"))

    slugs: list[str] = d["slugs"]
    n = len(slugs)
    idx_of: dict[str, int] = {s: i for i, s in enumerate(slugs)}

    wm = load_world_map_js(js_path)
    rooms: list[dict] = wm.get("rooms") or []
    by_id: dict[str, dict] = {}
    for r in rooms:
        rid = r.get("id")
        if isinstance(rid, str) and rid:
            by_id[rid] = r

    updated = 0
    missing_web = 0
    bad_exit = 0
    warn_keys: dict[str, int] = {}

    titles = list(d["titles"])
    blurbs = list(d["blurbs"])
    regions = list(d["regions"])
    exits: list[list[int]] = [list(row) for row in d["exits"]]
    item_lists = [list(x) for x in d.get("item_lists") or [[] for _ in slugs]]

    if len(titles) != n or len(blurbs) != n or len(regions) != n or len(exits) != n:
        raise SystemExit("corrupted JSON: array length mismatch vs slugs")

    for i, slug in enumerate(slugs):
        wr = by_id.get(slug)
        if not wr:
            missing_web += 1
            continue
        updated += 1
        name = wr.get("name")
        if isinstance(name, str) and name.strip():
            titles[i] = name.strip()
        desc = wr.get("description")
        if isinstance(desc, str):
            blurbs[i] = desc
        reg = wr.get("region")
        if isinstance(reg, str) and reg.strip():
            regions[i] = reg.strip()
        elif wr.get("region") is None:
            pass

        row = [-1] * len(DIR_LABELS)
        ex = wr.get("exits")
        if isinstance(ex, dict):
            for key, target_slug in ex.items():
                if not isinstance(target_slug, str) or not target_slug.strip():
                    continue
                nk = normalize_exit_key(str(key))
                if nk is None:
                    warn_keys[str(key)] = warn_keys.get(str(key), 0) + 1
                    continue
                di = LABEL_TO_DIR[nk]
                ti = idx_of.get(target_slug.strip())
                if ti is None:
                    bad_exit += 1
                    continue
                row[di] = ti
        exits[i] = row

        items = wr.get("items")
        if isinstance(items, list):
            item_lists[i] = [str(x) for x in items if str(x).strip()]

    report = (
        f"merge_web_world: web_root={web_root}, rooms in JSON={n}, "
        f"matched web rooms={updated}, missing in web={missing_web}, "
        f"bad_exit_targets={bad_exit}, unknown_exit_keys={len(warn_keys)}"
    )
    print(report, flush=True)
    if warn_keys:
        sample = sorted(warn_keys.items(), key=lambda x: -x[1])[:12]
        print("top unknown exit keys:", sample, flush=True)

    if args.dry_run:
        return 0

    backup_path.write_text(
        data_path.read_text(encoding="utf-8"), encoding="utf-8"
    )
    print(f"wrote backup {backup_path}", flush=True)

    d["titles"] = titles
    d["blurbs"] = blurbs
    d["regions"] = regions
    d["exits"] = exits
    d["item_lists"] = item_lists
    d["room_count"] = n

    if args.compact:
        blob = json.dumps(d, ensure_ascii=False, separators=(",", ":")) + "\n"
    else:
        blob = json.dumps(d, indent=2, ensure_ascii=False) + "\n"
    data_path.write_text(blob, encoding="utf-8")
    print(f"updated {data_path} ({len(blob)} bytes)", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
