#!/usr/bin/env python3
"""Run two Aeternitas64 EXEs with identical env/input and report the first
mismatch line index plus a short context window.

Quick smoke (PowerShell):
    py tools\dual_first_diff.py
    py tools\dual_first_diff.py --input "1`nlook`nquit`n5`n"
    py tools\dual_first_diff.py --hex --input-file recovery_artifacts/golden_playthrough/global.input.txt
    py tools\dual_first_diff.py --corpus recovery_artifacts/golden_playthrough --label global --hex
"""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))

from harvest_original_playthrough import (  # noqa: E402
    normalize_output,
    repo_root,
    stable_gate_line_index,
)
from repo_paths import golden_playthrough_dir  # noqa: E402

ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
TMP_PATH_RE = re.compile(
    r"[A-Za-z]:(?:\\|/)[^\n]*?\\aet-(?:dual|probe|first)-[a-z0-9_-]+(?:\\|/)\S+",
    re.IGNORECASE,
)


def strip_ansi(text: str) -> str:
    text = ANSI_RE.sub("", text)
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    return text


def normalize_for_compare(text: str, *, anonymize_temps: bool, full_normalize: bool) -> str:
    """Normalize stdout the way the dual-compare harness does, but optionally
    skip the splash/save-path stripping so callers can see early-line diffs."""
    if full_normalize:
        text = normalize_output(text)
    else:
        text = strip_ansi(text)
    if anonymize_temps:
        text = TMP_PATH_RE.sub("<AETER_TMP>", text)
    return text


def run_exe(
    exe: Path,
    input_text: str,
    save_path: Path,
    cwd: Path,
    env: dict[str, str],
    timeout: int,
    extra_args: list[str],
) -> tuple[int, str, str]:
    cmd = [str(exe), "--save", str(save_path), *extra_args]
    p = subprocess.run(
        cmd,
        input=input_text,
        text=True,
        capture_output=True,
        cwd=cwd,
        env=env,
        timeout=timeout,
    )
    return p.returncode, p.stdout, p.stderr


def hexdump_line(line: str, width: int = 32) -> str:
    raw = line.encode("utf-8", errors="replace")
    chunks = []
    for i in range(0, len(raw), width):
        chunk = raw[i : i + width]
        hex_part = " ".join(f"{b:02x}" for b in chunk)
        ascii_part = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        chunks.append(f"  {i:04x}  {hex_part:<{width * 3}}  {ascii_part}")
    return "\n".join(chunks) if chunks else "  <empty line>"


def first_diff(a_lines: list[str], b_lines: list[str]) -> int:
    for i, (la, lb) in enumerate(zip(a_lines, b_lines)):
        if la != lb:
            return i
    if len(a_lines) != len(b_lines):
        return min(len(a_lines), len(b_lines))
    return -1


def context_window(lines: list[str], idx: int, before: int, after: int) -> tuple[int, list[str]]:
    start = max(0, idx - before)
    stop = min(len(lines), idx + after + 1)
    return start, lines[start:stop]


def resolve_input(args) -> tuple[str, str]:
    """Return (label, input_text)."""
    if args.input is not None:
        # PowerShell users can pass `\n` literally — accept both.
        text = args.input.replace("\\n", "\n")
        if not text.endswith("\n"):
            text += "\n"
        return ("inline", text)
    if args.input_file:
        p = Path(args.input_file).resolve()
        if not p.is_file():
            raise SystemExit(f"missing --input-file: {p}")
        return (p.stem, p.read_text(encoding="utf-8"))
    if args.corpus and args.label:
        p = Path(args.corpus).resolve() / f"{args.label}.input.txt"
        if not p.is_file():
            raise SystemExit(f"missing corpus input: {p}")
        return (args.label, p.read_text(encoding="utf-8"))
    return ("default", "1\nlook\nquit\n5\n")


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(
        description=(
            "Run golden + recovered Aeternitas64 EXEs with identical env/input "
            "and print the first diff line index with a context window."
        )
    )
    parser.add_argument("--original", default=str(root / "aeternitas64.exe"))
    parser.add_argument(
        "--recovered",
        default=str(root / "aeternitas64_recovered.exe"),
        help=(
            "Recovered GCC build (default: aeternitas64_recovered.exe; see "
            "tools/build_recovered.ps1)."
        ),
    )
    parser.add_argument(
        "--input",
        default=None,
        help="Inline stdin script, e.g. \"1\\nlook\\nquit\\n5\\n\". Default: minimal new-game-look-quit.",
    )
    parser.add_argument(
        "--input-file",
        default=None,
        help="Path to a stdin script file (one command per line).",
    )
    parser.add_argument(
        "--corpus",
        default=str(golden_playthrough_dir()),
        help="Directory holding <label>.input.txt scripts (used with --label).",
    )
    parser.add_argument(
        "--label",
        default=None,
        help="Pick <label>.input.txt from --corpus (e.g. global, rooms, surfaces).",
    )
    parser.add_argument("--timeout", type=int, default=420)
    parser.add_argument("--before", type=int, default=3, help="Context lines before first diff.")
    parser.add_argument("--after", type=int, default=10, help="Context lines after first diff.")
    parser.add_argument(
        "--hex",
        action="store_true",
        help="Print hexdump of the diverging line(s).",
    )
    parser.add_argument(
        "--repr",
        action="store_true",
        help="Print Python repr() of the diverging line(s) so escapes are visible.",
    )
    parser.add_argument(
        "--full-normalize",
        action="store_true",
        help=(
            "Apply the dual-compare normalize_output (anonymize tempfile paths, "
            "strip splash, collapse blank runs). Default keeps splash/randomized "
            "intro intact so early divergence (boot text, dissolve) is visible."
        ),
    )
    parser.add_argument(
        "--no-anonymize-temps",
        action="store_true",
        help="Keep absolute Windows tempfile paths in compared output.",
    )
    parser.add_argument(
        "--extra-args",
        nargs=argparse.REMAINDER,
        default=[],
        help="Extra CLI args appended to both exes (after --).",
    )
    parser.add_argument(
        "--out",
        default=None,
        help="Optional directory to dump normalized stdout/stderr/diff for archival.",
    )
    args = parser.parse_args()

    orig = Path(args.original).resolve()
    rec = Path(args.recovered).resolve()
    if not orig.is_file():
        raise SystemExit(f"missing original exe: {orig}")
    if not rec.is_file():
        raise SystemExit(f"missing recovered exe: {rec}")

    label, input_text = resolve_input(args)

    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env.setdefault("NO_COLOR", "1")

    extra_args: list[str] = []
    if args.extra_args:
        extra_args = [a for a in args.extra_args if a != "--"]

    with tempfile.TemporaryDirectory(prefix=f"aet-first-{label}-orig-") as tdo:
        with tempfile.TemporaryDirectory(prefix=f"aet-first-{label}-rec-") as tdr:
            save_o = Path(tdo) / "save.txt"
            save_r = Path(tdr) / "save.txt"
            rc_o, raw_o, err_o = run_exe(
                orig, input_text, save_o, orig.parent, env, args.timeout, extra_args
            )
            rc_r, raw_r, err_r = run_exe(
                rec, input_text, save_r, rec.parent, env, args.timeout, extra_args
            )

    norm_o = normalize_for_compare(
        raw_o,
        anonymize_temps=not args.no_anonymize_temps,
        full_normalize=args.full_normalize,
    )
    norm_r = normalize_for_compare(
        raw_r,
        anonymize_temps=not args.no_anonymize_temps,
        full_normalize=args.full_normalize,
    )

    lines_o = norm_o.splitlines()
    lines_r = norm_r.splitlines()

    print(f"label                : {label}")
    print(f"original  exe        : {orig}")
    print(f"recovered exe        : {rec}")
    print(f"original  rc / lines : {rc_o} / {len(lines_o)}")
    print(f"recovered rc / lines : {rc_r} / {len(lines_r)}")

    idx = first_diff(lines_o, lines_r)
    if idx < 0:
        print("PARITY: identical normalized stdout (line-for-line).")
        if args.out:
            out_dir = Path(args.out).resolve()
            out_dir.mkdir(parents=True, exist_ok=True)
            (out_dir / f"{label}.original.txt").write_text(norm_o, encoding="utf-8")
            (out_dir / f"{label}.recovered.txt").write_text(norm_r, encoding="utf-8")
        return 0

    print(f"FIRST DIFF at line index {idx}")
    if not args.full_normalize:
        gate_o = stable_gate_line_index(lines_o)
        gate_r = stable_gate_line_index(lines_r)
        if gate_o >= 0 and gate_r >= 0 and idx < min(gate_o, gate_r):
            print()
            print(
                "NOTE: Mismatch is before the stable '18+ ADULT CONTENT' gate (boot / dissolve). "
                "Golden and recovered are launched sequentially, so srand(time(NULL)) "
                "usually differs and the VGA dissolve frames will not match byte-for-byte. "
                "For menu/gameplay parity use --full-normalize."
            )
    print()
    start_o, ctx_o = context_window(lines_o, idx, args.before, args.after)
    start_r, ctx_r = context_window(lines_r, idx, args.before, args.after)

    print("--- original ---")
    for i, ln in enumerate(ctx_o):
        marker = ">>" if (start_o + i) == idx else "  "
        print(f"{marker} {start_o + i:5d}: {ln}")
    print()
    print("--- recovered ---")
    for i, ln in enumerate(ctx_r):
        marker = ">>" if (start_r + i) == idx else "  "
        print(f"{marker} {start_r + i:5d}: {ln}")

    if args.repr or args.hex:
        oa = lines_o[idx] if idx < len(lines_o) else ""
        ra = lines_r[idx] if idx < len(lines_r) else ""
        print()
        if args.repr:
            print(f"repr original  [{idx}] : {oa!r}")
            print(f"repr recovered [{idx}] : {ra!r}")
        if args.hex:
            print(f"hex original  [{idx}] :")
            print(hexdump_line(oa))
            print(f"hex recovered [{idx}] :")
            print(hexdump_line(ra))

    if args.out:
        out_dir = Path(args.out).resolve()
        out_dir.mkdir(parents=True, exist_ok=True)
        (out_dir / f"{label}.original.txt").write_text(norm_o, encoding="utf-8")
        (out_dir / f"{label}.recovered.txt").write_text(norm_r, encoding="utf-8")
        (out_dir / f"{label}.original.stderr.txt").write_text(err_o, encoding="utf-8", errors="ignore")
        (out_dir / f"{label}.recovered.stderr.txt").write_text(err_r, encoding="utf-8", errors="ignore")

    return 1 if idx >= 0 else 0


if __name__ == "__main__":
    raise SystemExit(main())
