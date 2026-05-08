#!/usr/bin/env python3
"""Run suggest_typo vocab tokens not present in golden global.input.txt (smoke-test gaps)."""

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

from harvest_original_playthrough import normalize_output, repo_root  # noqa: E402

VOCAB_BLOCK_RE = re.compile(r"vocab\[\] = \{(.*?)\s*NULL\};", re.DOTALL)
AET_TMP_PATH_RE = re.compile(
    r"[A-Za-z]:(?:\\|/)[^\n]*?\\aet-(?:dual|probe)-[a-z0-9_-]+(?:\\|/)\S+",
    re.I,
)


def parse_vocab(ascii_c: Path) -> list[str]:
    text = ascii_c.read_text(encoding="utf-8", errors="ignore")
    m = VOCAB_BLOCK_RE.search(text)
    if not m:
        return []
    return re.findall(r'"([a-z_][a-z0-9_]*)"', m.group(1))


def global_commands_used(golden_global_input: Path) -> set[str]:
    s: set[str] = set()
    for ln in golden_global_input.read_text(encoding="utf-8").splitlines():
        t = ln.strip().lower()
        if not t or t.isdigit():
            continue
        s.add(t.split()[0])
    return s


def anonymize(text: str) -> str:
    return AET_TMP_PATH_RE.sub("<AETER_TMP>", text)


def run_both(root: Path, script: str, timeout: int) -> tuple[int, int, str, str]:
    orig = root / "aeternitas64.exe"
    rec = root / "aeternitas64_recovered.exe"
    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env.setdefault("NO_COLOR", "1")
    out_o = out_r = ""
    rc_o = rc_r = 1
    with tempfile.TemporaryDirectory(prefix="aet-probe-o-") as tdo:
        with tempfile.TemporaryDirectory(prefix="aet-probe-r-") as tdr:
            so = Path(tdo) / "s.txt"
            sr = Path(tdr) / "s.txt"
            p1 = subprocess.run(
                [str(orig), "--save", str(so)],
                input=script,
                text=True,
                capture_output=True,
                cwd=str(orig.parent),
                env=env,
                timeout=timeout,
            )
            rc_o = p1.returncode
            out_o = anonymize(normalize_output(p1.stdout))
            p2 = subprocess.run(
                [str(rec), "--save", str(sr)],
                input=script,
                text=True,
                capture_output=True,
                cwd=str(rec.parent),
                env=env,
                timeout=timeout,
            )
            rc_r = p2.returncode
            out_r = anonymize(normalize_output(p2.stdout))
    return rc_o, rc_r, out_o, out_r


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--max", type=int, default=48, help="Max extra vocab commands to try")
    parser.add_argument("--timeout", type=int, default=180)
    args = parser.parse_args()

    ascii_c = root / "aeternitas64_ascii.c"
    global_in = root / "recovery_artifacts" / "golden_playthrough" / "global.input.txt"
    if not ascii_c.is_file():
        print("missing aeternitas64_ascii.c", file=sys.stderr)
        return 1
    if not global_in.is_file():
        print("missing golden global.input.txt", file=sys.stderr)
        return 1

    vocab = parse_vocab(ascii_c)
    used = global_commands_used(global_in)
    gap = [w for w in vocab if w not in used]
    gap = gap[: args.max]
    print(f"vocab_terms={len(vocab)} global_distinct_first_tokens={len(used)} gap_trying={len(gap)}")

    lines = ["1"] + gap + ["quit", "5"]
    script = "\n".join(lines) + "\n"
    rc_o, rc_r, o, r = run_both(root, script, args.timeout)
    same = o == r
    print(f"original_rc={rc_o} recovered_rc={rc_r} identical_stdout={same}")
    if not same:
        diff = list(
            __import__("difflib").unified_diff(
                o.splitlines(keepends=True),
                r.splitlines(keepends=True),
                fromfile="original",
                tofile="recovered",
                n=2,
            )
        )
        cap = 120
        for i, ln in enumerate(diff[:cap]):
            print(ln, end="")
        if len(diff) > cap:
            print(f"... ({len(diff) - cap} more diff lines)")
    outd = root / "recovery_artifacts" / "probe_vocab_gap"
    outd.mkdir(parents=True, exist_ok=True)
    (outd / "probe.input.txt").write_text(script, encoding="utf-8")
    (outd / "original.normalized.txt").write_text(o, encoding="utf-8")
    (outd / "recovered.normalized.txt").write_text(r, encoding="utf-8")
    print(f"wrote {outd}/")
    return 0 if same else 2


if __name__ == "__main__":
    raise SystemExit(main())
