#!/usr/bin/env python3
from __future__ import annotations

import argparse
import difflib
import json
import os
import shutil
import subprocess
from pathlib import Path

from harvest_original_playthrough import normalize_output, repo_root


def seed_golden_saves(golden: Path, out_dir: Path, label: str) -> None:
    """Copy harvested save/slot files so load-based scripts see the same state as golden."""
    for src in sorted(golden.glob(f"{label}.save*.txt")):
        shutil.copy2(src, out_dir / src.name)


def run_candidate(exe: Path, input_path: Path, save_path: Path, timeout: int) -> tuple[int, str, str]:
    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env.setdefault("NO_COLOR", "1")
    p = subprocess.run(
        [str(exe), "--save", str(save_path)],
        input=input_path.read_text(encoding="utf-8"),
        text=True,
        capture_output=True,
        cwd=exe.parent,
        env=env,
        timeout=timeout,
    )
    return p.returncode, normalize_output(p.stdout), p.stderr


def collect_jobs(golden: Path) -> list[dict[str, str]]:
    manifest = json.loads((golden / "manifest.json").read_text(encoding="utf-8"))
    jobs = list(manifest["jobs"])
    seen = {job["label"] for job in jobs}
    for input_path in sorted(golden.glob("*.input.txt")):
        label = input_path.name.removesuffix(".input.txt")
        expected_path = golden / f"{label}.stdout.normalized.txt"
        if label in seen or not expected_path.is_file():
            continue
        jobs.append({"label": label})
        seen.add(label)
    return jobs


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(description="Diff a rebuilt EXE against golden playthrough transcripts.")
    parser.add_argument("--exe", default=str(root / "aeternitas64_recovered.exe"))
    parser.add_argument("--golden", default=str(root / "recovery_artifacts" / "golden_playthrough"))
    parser.add_argument("--out", default=str(root / "recovery_artifacts" / "playthrough_diff"))
    parser.add_argument("--timeout", type=int, default=360)
    args = parser.parse_args()

    exe_arg = Path(args.exe)
    exe = exe_arg.resolve() if exe_arg.is_absolute() else (root / exe_arg).resolve()
    golden = Path(args.golden).resolve()
    out_dir = Path(args.out).resolve()
    if not exe.is_file():
        raise SystemExit(f"missing candidate executable: {exe}")
    out_dir.mkdir(parents=True, exist_ok=True)
    jobs = collect_jobs(golden)

    summary = {"exe": str(exe), "jobs": []}
    for job in jobs:
        label = job["label"]
        input_path = golden / f"{label}.input.txt"
        expected_path = golden / f"{label}.stdout.normalized.txt"
        save_path = out_dir / f"{label}.save.txt"
        print(f"diffing {label}")
        seed_golden_saves(golden, out_dir, label)
        rc, actual, stderr = run_candidate(exe, input_path, save_path, args.timeout)
        expected = expected_path.read_text(encoding="utf-8")
        actual_path = out_dir / f"{label}.actual.normalized.txt"
        diff_path = out_dir / f"{label}.diff"
        stderr_path = out_dir / f"{label}.stderr.txt"
        actual_path.write_text(actual, encoding="utf-8")
        stderr_path.write_text(stderr, encoding="utf-8", errors="ignore")
        diff = list(
            difflib.unified_diff(
                expected.splitlines(keepends=True),
                actual.splitlines(keepends=True),
                fromfile=f"golden/{label}",
                tofile=f"candidate/{label}",
                n=3,
            )
        )
        diff_path.write_text("".join(diff), encoding="utf-8")
        entry = {
            "label": label,
            "returncode": rc,
            "diff_lines": len(diff),
            "actual_path": str(actual_path),
            "diff_path": str(diff_path),
        }
        summary["jobs"].append(entry)
        print(f"  rc={rc} diff_lines={len(diff)}")

    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    print(f"wrote {out_dir / 'summary.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
