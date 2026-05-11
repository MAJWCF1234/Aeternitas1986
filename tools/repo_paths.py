#!/usr/bin/env python3
"""Canonical repo paths for local maintenance and recovery artifacts.

Tools should resolve paths through this module instead of carrying stale drive
letters or sibling checkout assumptions.
"""

from __future__ import annotations

import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"


def require_under_root(path: Path, *, label: str = "path") -> Path:
    """Return an absolute path if it stays inside the repository root."""
    resolved = path.resolve()
    try:
        resolved.relative_to(ROOT.resolve())
    except ValueError as exc:
        raise ValueError(f"{label} must stay inside repo root: {resolved}") from exc
    return resolved


def recovery_txts_dir() -> Path:
    return ROOT / "recovery_artifacts" / "txts"


def world_tables_recovered_json() -> Path:
    primary = recovery_txts_dir() / "world_tables_recovered.json"
    legacy = ROOT / "recovery_artifacts" / "world_tables_recovered.json"
    if primary.is_file():
        return primary
    if legacy.is_file():
        return legacy
    return primary


def golden_playthrough_dir() -> Path:
    return ROOT / "recovery_artifacts" / "golden_playthrough"


def web_edition_index() -> Path:
    """Best-effort local WebEdition index path for comparison tools.

    Honors AETERNITAS_WEB_INDEX first, then checks common sibling checkout names.
    The returned path may not exist; callers decide whether that is an error.
    """
    env_path = os.environ.get("AETERNITAS_WEB_INDEX")
    if env_path:
        return Path(env_path).expanduser()
    for name in ("AeternitasWebEdition", "aeternitas-web-edition", "web"):
        candidate = ROOT.parent / name / "index.html"
        if candidate.is_file():
            return candidate
    return ROOT.parent / "AeternitasWebEdition" / "index.html"
