#!/usr/bin/env python3
"""Run original and recovered Aeternitas64 EXEs with identical stdin and diff normalized stdout."""

from __future__ import annotations

import argparse
import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

from harvest_original_playthrough import normalize_output, repo_root
from repo_paths import golden_playthrough_dir  # noqa: E402

# Strip per-run Temp paths so dual runs (different tempfile roots) still diff cleanly.
AET_TMP_PATH_RE = re.compile(
    r"[A-Za-z]:(?:\\|/)[^\n]*?\\aet-(?:dual|probe)-[a-z0-9_-]+(?:\\|/)\S+",
    re.IGNORECASE,
)


def for_diff_stdout(text: str, anonymize_temps: bool) -> str:
    if not anonymize_temps:
        return text
    return AET_TMP_PATH_RE.sub("<AETER_TMP>", text)


def run_normalized(
    exe: Path,
    input_text: str,
    save_path: Path,
    cwd: Path,
    env: dict[str, str],
    timeout: int,
) -> tuple[int, str, str]:
    p = subprocess.run(
        [str(exe), "--save", str(save_path)],
        input=input_text,
        text=True,
        capture_output=True,
        cwd=cwd,
        env=env,
        timeout=timeout,
    )
    return p.returncode, normalize_output(p.stdout), p.stderr


def collect_input_jobs(corpus: Path, extra_dirs: list[Path]) -> list[tuple[str, Path]]:
    jobs: list[tuple[str, Path]] = []
    seen: set[str] = set()
    manifest = corpus / "manifest.json"
    if manifest.is_file():
        data = json.loads(manifest.read_text(encoding="utf-8"))
        for j in data.get("jobs", []):
            label = j["label"]
            p = corpus / f"{label}.input.txt"
            if p.is_file():
                jobs.append((label, p))
                seen.add(label)
            else:
                raise SystemExit(f"manifest references missing input: {p}")
        # Corpus may contain extra *.input.txt scripts (e.g. character_creation,
        # surfaces) not listed in manifest.json — include them so post-menu flows
        # are not silently skipped when comparing binaries.
        for p in sorted(corpus.glob("*.input.txt")):
            label = p.stem.replace(".input", "")
            if label in seen:
                continue
            jobs.append((label, p))
            seen.add(label)
    for d in extra_dirs:
        if not d.is_dir():
            raise SystemExit(f"extra corpus dir not found: {d}")
        for p in sorted(d.glob("*.input.txt")):
            label = p.stem.replace(".input", "")
            if label in seen:
                continue
            jobs.append((label, p))
            seen.add(label)
    if not jobs:
        for p in sorted(corpus.glob("*.input.txt")):
            label = p.stem.replace(".input", "")
            jobs.append((label, p))
    return jobs


def trim_diff_hunks(diff_lines: list[str], max_hunks: int) -> tuple[list[str], int]:
    if max_hunks <= 0 or not diff_lines:
        return diff_lines, 0
    out: list[str] = []
    hunks = 0
    for ln in diff_lines:
        if ln.startswith("@@ "):
            hunks += 1
            if hunks > max_hunks:
                break
        out.append(ln)
    truncated = max(0, sum(1 for x in diff_lines if x.startswith("@@ ")) - max_hunks)
    return out, truncated


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(
        description="Compare normalized stdout from original vs recovered exe (same stdin, temp saves).",
        epilog="PowerShell: use --only=global (not --only global); `global` is a scope keyword.",
    )
    parser.add_argument(
        "--original",
        default=str(root / "aeternitas64.exe"),
        help="Known-good executable",
    )
    parser.add_argument(
        "--recovered",
        default=str(root / "aeternitas64_recovered.exe"),
        help="Rebuilt / recovered executable",
    )
    parser.add_argument(
        "--corpus",
        default=str(golden_playthrough_dir()),
        help="Directory with manifest.json and *.input.txt (unlisted *.input.txt next to manifest are included automatically)",
    )
    parser.add_argument(
        "--extra-corpus",
        action="append",
        default=[],
        help="Additional directory(ies) with *.input.txt to merge (e.g. surfaces probe)",
    )
    parser.add_argument(
        "--out",
        default=str(root / "recovery_artifacts" / "dual_compare"),
        help="Output directory for transcripts and diffs",
    )
    parser.add_argument("--timeout", type=int, default=420)
    parser.add_argument(
        "--max-diff-hunks",
        type=int,
        default=12,
        help="Cap unified diff output per job (0 = full diff)",
    )
    parser.add_argument(
        "--only",
        action="append",
        default=[],
        metavar="LABEL",
        help="Run just these job labels (repeatable). Default: all from manifest + extra dirs.",
    )
    parser.add_argument(
        "--no-anonymize-temps",
        action="store_true",
        help="Keep absolute tempfile paths in output (default: replace with <AETER_TMP>)",
    )
    args = parser.parse_args()

    orig = Path(args.original).resolve()
    rec = Path(args.recovered).resolve()
    corpus = Path(args.corpus).resolve()
    out_dir = Path(args.out).resolve()
    extra = [Path(p).resolve() for p in args.extra_corpus]

    if not orig.is_file():
        raise SystemExit(f"missing original exe: {orig}")
    if not rec.is_file():
        raise SystemExit(f"missing recovered exe: {rec}")

    jobs = collect_input_jobs(corpus, extra)
    if args.only:
        allow = {x.strip() for x in args.only if x.strip()}
        jobs = [(a, b) for a, b in jobs if a in allow]
        missing = allow - {a for a, _ in jobs}
        if missing:
            raise SystemExit(f"--only labels not found: {sorted(missing)}")
    if not jobs:
        raise SystemExit(f"no input jobs under {corpus}")

    out_dir.mkdir(parents=True, exist_ok=True)
    env_base = os.environ.copy()
    env_base["AETER_AUTOTEST"] = "1"
    env_base.setdefault("NO_COLOR", "1")

    summary = {
        "original": str(orig),
        "recovered": str(rec),
        "corpus": str(corpus),
        "extra_corpus": [str(p) for p in extra],
        "jobs": [],
    }

    any_diff = False
    for label, input_path in jobs:
        input_text = input_path.read_text(encoding="utf-8")
        print(f"compare {label} ({input_path.name})")
        with tempfile.TemporaryDirectory(prefix=f"aet-dual-{label}-orig-") as tdo:
            with tempfile.TemporaryDirectory(prefix=f"aet-dual-{label}-rec-") as tdr:
                save_o = Path(tdo) / "save.txt"
                save_r = Path(tdr) / "save.txt"
                rc_o, norm_o, err_o = run_normalized(
                    orig, input_text, save_o, orig.parent, env_base, args.timeout
                )
                rc_r, norm_r, err_r = run_normalized(
                    rec, input_text, save_r, rec.parent, env_base, args.timeout
                )

        anon = not args.no_anonymize_temps
        compare_o = for_diff_stdout(norm_o, anon)
        compare_r = for_diff_stdout(norm_r, anon)

        job_out = out_dir / label
        job_out.mkdir(parents=True, exist_ok=True)
        (job_out / "input.txt").write_text(input_text, encoding="utf-8")
        (job_out / "original.normalized.txt").write_text(norm_o, encoding="utf-8")
        (job_out / "recovered.normalized.txt").write_text(norm_r, encoding="utf-8")
        (job_out / "original.for_diff.txt").write_text(compare_o, encoding="utf-8")
        (job_out / "recovered.for_diff.txt").write_text(compare_r, encoding="utf-8")
        (job_out / "original.stderr.txt").write_text(err_o, encoding="utf-8", errors="ignore")
        (job_out / "recovered.stderr.txt").write_text(err_r, encoding="utf-8", errors="ignore")

        diff_lines = list(
            __import__("difflib").unified_diff(
                compare_o.splitlines(keepends=True),
                compare_r.splitlines(keepends=True),
                fromfile=f"original/{label}",
                tofile=f"recovered/{label}",
                n=3,
            )
        )
        trimmed, truncated_hunks = trim_diff_hunks(diff_lines, args.max_diff_hunks)
        diff_text = "".join(trimmed)
        if truncated_hunks > 0:
            diff_text += f"\n... ({truncated_hunks} more diff hunks truncated; use --max-diff-hunks 0)\n"
        (job_out / "stdout.diff").write_text(diff_text, encoding="utf-8")

        identical = compare_o == compare_r
        if not identical:
            any_diff = True
        entry = {
            "label": label,
            "original_rc": rc_o,
            "recovered_rc": rc_r,
            "identical_normalized_stdout": identical,
            "diff_line_count": len(diff_lines),
            "paths": {
                "original_out": str(job_out / "original.normalized.txt"),
                "recovered_out": str(job_out / "recovered.normalized.txt"),
                "diff": str(job_out / "stdout.diff"),
            },
        }
        summary["jobs"].append(entry)
        print(
            f"  original_rc={rc_o} recovered_rc={rc_r} identical={identical} diff_lines={len(diff_lines)}"
        )

    summary["any_differences"] = any_diff
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")

    report_lines = [
        "# Dual EXE compare (original vs recovered)",
        "",
        f"- Original: `{orig}`",
        f"- Recovered: `{rec}`",
        f"- Identical normalized stdout (all jobs): **{'no' if any_diff else 'yes'}**",
        "",
    ]
    for j in summary["jobs"]:
        report_lines.append(f"## {j['label']}")
        report_lines.append("")
        report_lines.append(
            f"- return codes: original={j['original_rc']} recovered={j['recovered_rc']}"
        )
        report_lines.append(
            f"- identical: **{j['identical_normalized_stdout']}** "
            f"(diff hunks ~ {j['diff_line_count']} lines in unified diff)"
        )
        if not j["identical_normalized_stdout"]:
            diff_path = Path(j["paths"]["diff"])
            snippet = diff_path.read_text(encoding="utf-8", errors="ignore")
            snippet = snippet[:8000]
            if len(snippet) >= 8000:
                snippet += "\n\n… (snippet truncated for report; see stdout.diff file)\n"
            report_lines.append("")
            report_lines.append("```diff")
            report_lines.append(snippet.rstrip())
            report_lines.append("```")
        report_lines.append("")

    report_lines.extend(
        [
            "## Interactive / non-stdout gaps",
            "",
            "- Arrow-key pagers (`help modding`), splash timing, and some `ui_block_pause` flows only appear in autotest as `[CI autotest: …]` or shortened paths.",
            "- Forge `craft` may bump proficiency via `rand() % 3`, so stdout can diverge even when logic matches.",
            "",
        ]
    )
    (out_dir / "REPORT.md").write_text("\n".join(report_lines), encoding="utf-8")
    print(f"wrote {out_dir / 'summary.json'} and {out_dir / 'REPORT.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
