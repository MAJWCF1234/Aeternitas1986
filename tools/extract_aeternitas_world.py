#!/usr/bin/env python3
"""Deprecated entry point (older edition). Use the pipeline below."""

from __future__ import annotations

import sys


def main() -> int:
    sys.stderr.write(
        "extract_aeternitas_world.py is deprecated.\n\n"
        "  py -3 tools/extract_world_tables_from_exe.py\n"
        "  py -3 tools/build_world_c_from_recovered_json.py\n\n"
        "JSON output: recovery_artifacts/txts/world_tables_recovered.json\n"
    )
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
