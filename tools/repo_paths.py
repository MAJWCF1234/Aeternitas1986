#!/usr/bin/env python3
"""Canonical repo paths for recovery artifacts (layout under recovery_artifacts/txts/)."""

from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


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
