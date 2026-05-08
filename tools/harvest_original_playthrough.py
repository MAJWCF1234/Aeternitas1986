#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import subprocess
import tempfile
from pathlib import Path


ANSI_RE = re.compile(r"\x1b\[[0-9;]*[A-Za-z]")
PROMPT_RE = re.compile(r"(?m)^>\s*")
# VGA boot text + randomized ASCII dissolve differ per run; age gate banner is stable.
_BOOT_GATE_RE = re.compile(
    r"(?ms)^={30,}\s*\n\s*18\+\s*ADULT\s*CONTENT\s*\n={30,}\s*\n",
)
_WIN_ABS_SAVE_QUICK_RE = re.compile(
    r"(?i)\b[A-Za-z]:\\(?:[^\\\r\n]|\\)+?\.save\.txt\b"
)
_WIN_ABS_SAVE_SLOT_RE = re.compile(
    r"(?i)\b[A-Za-z]:\\(?:[^\\\r\n]|\\)+?\.save_slot(\d+)\.txt\b"
)


def repo_root() -> Path:
    return Path(__file__).resolve().parents[1]


def load_world_data(root: Path) -> dict:
    data_path = root / "recovery_artifacts" / "world_tables_recovered.json"
    if not data_path.is_file():
        raise SystemExit(f"missing {data_path}; run extract_world_tables_from_exe.py first")
    return json.loads(data_path.read_text(encoding="utf-8"))


def strip_boot_splash_prefix(text: str) -> str:
    """Drop VGA boot + title dissolve; keep from the 18+ disclaimer block onward."""
    m = _BOOT_GATE_RE.search(text)
    if not m:
        return text
    return text[m.start() :]


def canonicalize_abs_save_paths(text: str) -> str:
    """Make save/load UI lines independent of --save basename and drive letter."""
    text = _WIN_ABS_SAVE_QUICK_RE.sub("<SAVE_QUICK.txt>", text)
    text = _WIN_ABS_SAVE_SLOT_RE.sub(
        lambda m: f"<SAVE_SLOT{m.group(1)}.txt>", text
    )
    return text


def normalize_output(text: str) -> str:
    text = ANSI_RE.sub("", text)
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    text = "\n".join(line.rstrip() for line in text.splitlines())
    text = re.sub(r"\n{4,}", "\n\n\n", text)
    text = canonicalize_abs_save_paths(text)
    text = strip_boot_splash_prefix(text)
    return text.strip() + "\n"


def stable_gate_line_index(lines: list[str]) -> int:
    """0-based line index of the stable 18+ banner block start, or -1 if missing."""
    text = "\n".join(lines)
    m = _BOOT_GATE_RE.search(text)
    if not m:
        return -1
    return text[: m.start()].count("\n")


def run_exe(exe: Path, script: list[str], out_dir: Path, label: str, timeout: int) -> dict:
    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env.setdefault("NO_COLOR", "1")
    out_dir.mkdir(parents=True, exist_ok=True)
    save_path = out_dir / f"{label}.save.txt"
    script_text = "\n".join(script) + "\n"

    with tempfile.TemporaryDirectory(prefix=f"aet-{label}-") as td:
        p = subprocess.run(
            [str(exe), "--save", str(save_path)],
            input=script_text,
            text=True,
            capture_output=True,
            cwd=exe.parent,
            env=env,
            timeout=timeout,
        )

    raw_stdout = p.stdout
    raw_stderr = p.stderr
    norm_stdout = normalize_output(raw_stdout)
    (out_dir / f"{label}.input.txt").write_text(script_text, encoding="utf-8")
    (out_dir / f"{label}.stdout.txt").write_text(raw_stdout, encoding="utf-8", errors="ignore")
    (out_dir / f"{label}.stdout.normalized.txt").write_text(norm_stdout, encoding="utf-8")
    (out_dir / f"{label}.stderr.txt").write_text(raw_stderr, encoding="utf-8", errors="ignore")
    return {
        "label": label,
        "returncode": p.returncode,
        "commands": len(script),
        "stdout_bytes": len(raw_stdout.encode("utf-8", errors="ignore")),
        "normalized_sha256": hashlib.sha256(norm_stdout.encode("utf-8")).hexdigest(),
    }


def base_start() -> list[str]:
    return ["1"]


def clean_exit() -> list[str]:
    return ["quit", "5"]


def build_global_script() -> list[str]:
    commands = [
        "whereami",
        "look",
        "exits",
        "exits locked",
        "nearby",
        "nearby detail",
        "inventory",
        "i",
        "loadout",
        "status",
        "character brief",
        "sheet brief",
        "skills",
        "traits",
        "perks",
        "vitals",
        "reputation",
        "rapport",
        "momentum",
        "voice",
        "bio",
        "tainting",
        "progress",
        "score",
        "journal",
        "objectives",
        "notes show",
        "hints",
        "lights",
        "time",
        "weather",
        "temperature",
        "lockcheck",
        "noise",
        "causality recent",
        "why blocked",
        "help",
        "about",
        "mods list",
        "mods doctor",
    ]
    return base_start() + commands + clean_exit()


def build_room_script(slugs: list[str], per_room: bool) -> list[str]:
    script = base_start()
    for slug in slugs:
        script.append(f"route {slug}")
        script.append("whereami")
        script.append("look")
        script.append("exits")
        script.append("nearby detail")
        script.append("scan")
        script.append("loot")
        if per_room:
            script.extend(
                [
                    "search",
                    "find key",
                    "find coin",
                    "listen",
                    "smell",
                    "who",
                    "wares",
                ]
            )
    script.extend(["progress", "score"])
    script.extend(clean_exit())
    return script


def build_npc_script(npcs: list[dict], merchants: list[dict]) -> list[str]:
    topics = [
        "work",
        "trade",
        "rumor",
        "help",
        "guard",
        "faith",
        "food",
        "room",
        "mill",
        "forge",
        "prices",
    ]
    script = base_start()
    for npc in npcs:
        slug = npc["slug"]
        script.append(f"where {slug}")
        script.append(f"talk to {slug}")
        for topic in topics:
            script.append(f"talk to {slug} about {topic}")
        for topic in npc.get("topics", []):
            kws = str(topic.get("keywords", "")).split()
            if kws:
                script.append(f"talk to {slug} about {' '.join(kws[:3])}")
    for merchant in merchants:
        slug = merchant["slug"]
        script.append(f"where {slug}")
        script.append(f"talk to {slug} about trade")
        script.append("wares")
        for offer in merchant.get("stock", [])[:5]:
            script.append(f"buy {offer['item']}")
        for offer in merchant.get("buys", [])[:5]:
            script.append(f"sell {offer['item']}")
    script.extend(["reputation", "people", "rapport"])
    script.extend(clean_exit())
    return script


def build_item_script(data: dict) -> list[str]:
    seen: list[str] = []
    for lists_key in ("item_lists", "hidden_item_lists"):
        for items in data.get(lists_key, []):
            for item in items:
                if item and item not in seen:
                    seen.append(item)
    script = base_start()
    for item in seen[:220]:
        script.append(f"find {item}")
        script.append(f"examine {item}")
        script.append(f"compare {item} / bread")
    script.extend(clean_exit())
    return script


def build_character_script() -> list[str]:
    # This intentionally accepts defaults through the character creator in autotest mode.
    # Extra newlines make it resilient if a later build asks one more setup question.
    return ["2"] + ["1"] * 80 + clean_exit()


def build_ui_surfaces_script() -> list[str]:
    """Equipment UI, material forge, save/load and slot save — all stdin-driven."""
    return base_start() + [
        "loadout",
        "equipment",
        "done",
        "forge",
        "clear",
        "done",
        "saves",
        "save",
        "load",
        "save 1",
        "load 1",
    ] + clean_exit()


def main() -> int:
    root = repo_root()
    parser = argparse.ArgumentParser(
        description="Autoplay the known-good Aeternitas64 EXE and harvest golden transcripts."
    )
    parser.add_argument("--exe", default=str(root / "aeternitas64.exe"))
    parser.add_argument(
        "--out",
        default=str(root / "recovery_artifacts" / "golden_playthrough"),
    )
    parser.add_argument(
        "--mode",
        choices=[
            "all",
            "global",
            "rooms",
            "rooms-deep",
            "npcs",
            "items",
            "character",
            "surfaces",
        ],
        default="all",
    )
    parser.add_argument("--timeout", type=int, default=240)
    args = parser.parse_args()

    exe = Path(args.exe).resolve()
    if not exe.is_file():
        raise SystemExit(f"missing executable: {exe}")
    out_dir = Path(args.out).resolve()
    data = load_world_data(root)

    jobs: list[tuple[str, list[str], int]] = []
    if args.mode in {"all", "global"}:
        jobs.append(("global", build_global_script(), args.timeout))
    if args.mode in {"all", "rooms"}:
        jobs.append(("rooms", build_room_script(data["slugs"], per_room=False), max(args.timeout, 360)))
    if args.mode == "rooms-deep":
        jobs.append(("rooms_deep", build_room_script(data["slugs"], per_room=True), max(args.timeout, 600)))
    if args.mode in {"all", "npcs"}:
        jobs.append(("npcs_merchants", build_npc_script(data.get("npc_lines", []), data.get("merchants", [])), args.timeout))
    if args.mode in {"all", "items"}:
        jobs.append(("items", build_item_script(data), max(args.timeout, 360)))
    if args.mode in {"character"}:
        jobs.append(
            ("character_creation", build_character_script(), args.timeout)
        )
    if args.mode in {"surfaces"}:
        jobs.append(
            ("surfaces", build_ui_surfaces_script(), max(args.timeout, 300))
        )

    manifest = {
        "exe": str(exe),
        "exe_sha256": hashlib.sha256(exe.read_bytes()).hexdigest(),
        "out_dir": str(out_dir),
        "mode": args.mode,
        "world_room_count": len(data.get("slugs", [])),
        "npc_count": len(data.get("npc_lines", [])),
        "merchant_count": len(data.get("merchants", [])),
        "jobs": [],
    }

    for label, script, timeout in jobs:
        print(f"harvesting {label}: {len(script)} commands")
        result = run_exe(exe, script, out_dir, label, timeout)
        manifest["jobs"].append(result)
        print(
            f"  rc={result['returncode']} bytes={result['stdout_bytes']} sha={result['normalized_sha256'][:12]}"
        )

    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (out_dir / "README.md").write_text(
        "# Golden Playthrough Harvest\n\n"
        "These files were generated by `tools/harvest_original_playthrough.py` from the known-good EXE.\n"
        "Use `*.stdout.normalized.txt` for stable diffs against rebuilt executables.\n"
        "Normalization strips ANSI, normalizes newlines, collapses long blank runs, "
        "replaces absolute Windows quicksave / slot paths with stable placeholders, then "
        "removes the nondeterministic VGA/splash title dissolve up to the stable `18+ ADULT CONTENT` gate.\n\n"
        "Main passes:\n"
        "- `global`: common panels and help/status surfaces.\n"
        "- `rooms`: route/look/exits/nearby/scan/loot for every recovered room slug.\n"
        "- `npcs_merchants`: NPC greeting/topic attempts and merchant wares/buy/sell attempts.\n"
        "- `items`: find/examine/compare for recovered world item ids.\n"
        "Optional (`--mode character` / `--mode surfaces`): autotest character builder and equipment/forge/save UI smoke "
        "(may still differ from golden until CC/save-loop disasm parity).\n",
        encoding="utf-8",
    )
    print(f"wrote manifest: {out_dir / 'manifest.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
