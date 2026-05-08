#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import subprocess
import tempfile
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def load_slugs_from_world_source(world_c: Path) -> list[str]:
    text = world_c.read_text(encoding="utf-8", errors="ignore")
    m = re.search(
        r"(?:ROOM_SLUGS|WORLD_SLUGS_REC)\[WORLD_ROOM_COUNT\]\s*=\s*\{(.*?)\};",
        text,
        re.S,
    )
    if not m:
        return []
    body = m.group(1)
    return re.findall(r'"([a-z0-9_]+)"', body)


def build_script(slugs: list[str]) -> str:
    lines: list[str] = []
    lines.append("1")  # new game
    lines.extend(
        [
            "whereami",
            "exits",
            "inventory",
            "loadout",
        ]
    )
    for slug in slugs:
        lines.append(f"route {slug}")
    lines.extend(["progress", "score", "quit", "5"])
    return "\n".join(lines) + "\n"


def parse_route_lines(output: str) -> dict[str, str]:
    routes: dict[str, str] = {}
    cur: str | None = None
    for line in output.splitlines():
        line = line.strip()
        m = re.search(r"route\s+([a-z0-9_]+)", line)
        if m:
            cur = m.group(1)
            continue
        if cur and line:
            if "Route (" in line or "not one step away" in line or "Unknown place" in line:
                routes[cur] = line
                cur = None
    return routes


def main() -> int:
    root = repo_root()
    exe = root / "aeternitas64.exe"
    world_c = root / "aeternitas_world_generated.c"
    out_dir = root / "recovery_artifacts"
    out_dir.mkdir(exist_ok=True)

    if not exe.is_file():
        print("FAIL: original exe missing")
        return 1
    if not world_c.is_file():
        print("FAIL: world source missing")
        return 2

    slugs = load_slugs_from_world_source(world_c)
    if not slugs:
        print("FAIL: no slugs found in world source")
        return 3

    script = build_script(slugs)
    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"

    with tempfile.TemporaryDirectory(prefix="aet-trace-") as td:
        save_path = Path(td) / "trace_save.txt"
        p = subprocess.run(
            [str(exe), "--save", str(save_path)],
            input=script,
            text=True,
            capture_output=True,
            cwd=root,
            env=env,
            timeout=180,
        )

    (out_dir / "trace_original_stdout.txt").write_text(
        p.stdout, encoding="utf-8", errors="ignore"
    )
    (out_dir / "trace_original_stderr.txt").write_text(
        p.stderr, encoding="utf-8", errors="ignore"
    )

    routes = parse_route_lines(p.stdout)
    summary = {
        "returncode": p.returncode,
        "slug_count": len(slugs),
        "routes_detected": len(routes),
        "routes": routes,
    }
    (out_dir / "trace_original_summary.json").write_text(
        json.dumps(summary, indent=2), encoding="utf-8"
    )

    print(f"returncode={p.returncode}")
    print(f"slug_count={len(slugs)}")
    print(f"routes_detected={len(routes)}")
    return 0 if p.returncode == 0 else p.returncode


if __name__ == "__main__":
    raise SystemExit(main())
