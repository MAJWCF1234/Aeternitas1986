#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / "aeternitas64.exe"

ANSI_RE = re.compile(r"\x1b\[[0-9;?]*[A-Za-z]")


@dataclass(frozen=True)
class ParserCase:
    name: str
    commands: tuple[str, ...]
    must_contain: tuple[str, ...] = ()
    must_not_contain: tuple[str, ...] = ("unknown-command", "You do not recognize")
    pre_commands: tuple[str, ...] = ()
    save_edits: tuple[tuple[str, str], ...] = ()


CASES: tuple[ParserCase, ...] = (
    ParserCase(
        name="first-person look",
        commands=("new", "I look around", "causality parser", "quit"),
        must_contain=("parser-normalize", "i look around -> look around"),
    ),
    ParserCase(
        name="plain inventory wrapper",
        commands=("new", "check my inventory", "done", "quit"),
        must_contain=("EQUIPMENT", "INVENTORY"),
    ),
    ParserCase(
        name="plain chain movement",
        commands=("new", "I go north then south", "causality parser", "quit"),
        must_contain=("parser-normalize", "i go north -> north", "Ok.\nOk.", "Turn: 2"),
    ),
    ParserCase(
        name="polite status",
        commands=("new", "could you please show me my status", "quit"),
        must_contain=("Location:", "Health:", "Turns:"),
    ),
    ParserCase(
        name="where-am-i query",
        commands=("new", "can you tell me where I am", "quit"),
        must_contain=("You are at:", "west_of_house"),
    ),
    ParserCase(
        name="typo movement suggestion",
        commands=("new", "nort", "quit"),
        must_contain=("North of House",),
        must_not_contain=(),
    ),
    ParserCase(
        name="chain movement",
        commands=("new", "north then south", "quit"),
        must_contain=("Ok.\nOk.", "Turn: 2"),
    ),
    ParserCase(
        name="repeat chain",
        commands=("new", "north then south", "again", "quit"),
        must_contain=("Ok.\nOk.", "Turn: 4"),
    ),
    ParserCase(
        name="fullscreen chain continues",
        commands=("new", "help; brief", "quit"),
        must_contain=(
            "Command groups:",
            "Brief room blurbs on",
        ),
    ),
    ParserCase(
        name="exits then inline command",
        commands=("new", "exits; brief", "quit"),
        must_contain=("EXITS", "Brief room blurbs on"),
    ),
    ParserCase(
        name="npc validation clean on new",
        commands=("new", "errors", "quit"),
        must_contain=(
            "NPC world refs:",
            "0 warning(s)",
            "no failed commands logged yet.",
        ),
        must_not_contain=("npc-world:", "unknown-command", "You do not recognize"),
    ),
    ParserCase(
        name="npc validation clean on load",
        commands=("new", "qs", "ql", "errors", "quit"),
        must_contain=("NPC world refs:", "0 warning(s)"),
        must_not_contain=("npc-world:", "unknown-command", "You do not recognize"),
    ),
    ParserCase(
        name="global who scan",
        commands=("new", "who all", "quit"),
        must_contain=(
            "Global NPC presence scan",
            "miller",
            "Blacksmith's Forge",
            "activity:",
            "Village Inn",
            "Total placements:",
        ),
    ),
    ParserCase(
        name="whereami alias",
        commands=("new", "whereami", "quit"),
        must_contain=("You are at:", "[west_of_house]"),
    ),
    ParserCase(
        name="locked route query",
        commands=("new", "where locks", "quit"),
        must_contain=("Lockcheck", "Best tool:", "Current:"),
    ),
    ParserCase(
        name="nearby detailed alias",
        commands=("new", "nearby detailed", "quit"),
        must_contain=('From "West of House', "(detailed)", "region="),
    ),
    ParserCase(
        name="filtered exits view",
        commands=("new", "exits locked", "quit"),
        must_contain=("Filter: locked", "No exits match this filter."),
    ),
    ParserCase(
        name="exit lock indicators locked",
        commands=("new", "north", "east", "exits", "quit"),
        must_contain=("East of House", "shed", "[locked]"),
    ),
    ParserCase(
        name="exit lock indicators open",
        commands=("new", "take lockpick", "north", "east", "east", "west", "exits", "quit"),
        must_contain=("East of House", "shed", "[open]"),
        must_not_contain=("unknown-command", "You do not recognize", "[locked]"),
    ),
    ParserCase(
        name="filtered locked exits present",
        commands=("new", "north", "east", "exits locked", "quit"),
        must_contain=("Filter: locked", "shed", "[locked]"),
    ),
    ParserCase(
        name="targeted look alias",
        commands=("new", "look door", "quit"),
        must_contain=(
            "The white house dominates the east",
            "A weathered mailbox waits nearby.",
        ),
    ),
    ParserCase(
        name="targeted look short alias",
        commands=("new", "l door", "quit"),
        must_contain=(
            "The white house dominates the east",
            "A weathered mailbox waits nearby.",
        ),
    ),
    ParserCase(
        name="look helper phrase cleanup",
        commands=("new", "take a look at mailbox", "causality parser", "quit"),
        must_contain=("parser-normalize", "take a look at mailbox -> examine mailbox"),
    ),
    ParserCase(
        name="talk helper phrase cleanup",
        commands=("new", "talk with miller", "causality parser", "quit"),
        must_contain=("parser-normalize", "talk with miller -> talk to miller"),
    ),
    ParserCase(
        name="plain english movement bridge",
        commands=("new", "i want to go north", "causality parser", "quit"),
        must_contain=("North of House", "parser-normalize", "i want to go north -> north"),
    ),
    ParserCase(
        name="plain english inventory bridge",
        commands=("new", "can i check my inventory", "done", "causality parser", "quit"),
        must_contain=(
            "EQUIPMENT",
            "INVENTORY",
            "parser-normalize",
            "can i check my inventory -> inventory",
        ),
    ),
    ParserCase(
        name="plain english inspect bridge",
        commands=("new", "let me look at mailbox", "causality parser", "quit"),
        must_contain=("parser-normalize", "let me look at mailbox -> look at mailbox"),
    ),
    ParserCase(
        name="npc non collectable enforcement",
        commands=(
            "new",
            "north",
            "north",
            "northeast",
            "east",
            "east",
            "take blacksmith",
            "grab blacksmith",
            "pick up blacksmith",
            "quit",
        ),
        must_contain=(
            "Blacksmith's Forge",
            "Also here: blacksmith",
            "You cannot take blacksmith - people are not inventory.",
        ),
        must_not_contain=("Taken: blacksmith",),
    ),
    ParserCase(
        name="protective grab intent path",
        commands=(
            "new",
            "north",
            "north",
            "northeast",
            "east",
            "east",
            "grab her hand and pull her away from danger gently",
            "causality parser",
            "quit",
        ),
        must_contain=(
            "Blacksmith's Forge",
            "You read \"her\" as blacksmith here.",
            "The parser reads it as a rescue move, not a pickup attempt.",
            "parser-normalize",
            "-> protective grab her hand and pull her away from danger",
        ),
        must_not_contain=("Taken: blacksmith", "You do not see that here."),
    ),
    ParserCase(
        name="bulk take except parsing",
        commands=(
            "new",
            "take all except the lockpick and the wood scrap",
            "take wood_scrap",
            "take lockpick",
            "take mailbox",
            "quit",
        ),
        must_contain=(
            "Taken: Wood Scrap",
            "Taken: Lockpick",
            "You do not see that here.",
        ),
    ),
    ParserCase(
        name="bulk drop except parsing",
        commands=(
            "new",
            "take all",
            "drop all except the bandage and flint",
            "drop bandage",
            "drop flint",
            "drop club",
            "quit",
        ),
        must_contain=(
            "Dropped: Bandage",
            "Dropped: Flint",
            "You are not carrying that.",
        ),
    ),
    ParserCase(
        name="bulk action state consistency",
        commands=("new", "hold club", "drop all", "status", "quit"),
        must_contain=(
            "You ready club.",
            "You set everything down.",
            "Readied:",
            "Inventory slots: 0",
        ),
        must_not_contain=("Readied: club",),
    ),
    ParserCase(
        name="merchant reputation discounts",
        commands=(
            "new",
            "north",
            "north",
            "northeast",
            "east",
            "east",
            "wares",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "talk",
            "wares",
            "rapport",
            "quit",
        ),
        must_contain=(
            "Patron: Stranger (0 pts)",
            "Patron: Regular (26 pts)",
            "Steel Ingot",
            "2b 4c",
            "list 2b 5c",
            "blacksmith",
            "patron 26 (Regular)",
        ),
    ),
    ParserCase(
        name="trading skill prices",
        pre_commands=("new", "qs", "quit"),
        save_edits=(("cha 10", "cha 18"),),
        commands=("load", "north", "north", "northeast", "east", "east", "wares", "quit"),
        must_contain=(
            "Trade knack: silver-tongued (CHA 18)",
            "buy edge 8%, sell edge 15%.",
            "Patron: Stranger (0 pts)",
            "Iron Ingot",
            "1b 4c",
            "Steel Ingot",
            "2b 3c",
            "list 2b 5c",
        ),
    ),
    ParserCase(
        name="currency variety",
        pre_commands=("new", "qs", "quit"),
        save_edits=(("coins 50", "coins 173"),),
        commands=("load", "purse", "north", "north", "northeast", "east", "east", "wares", "quit"),
        must_contain=(
            "purse: 1 gold, 1 silver, 2 bronze, 3 copper.",
            "Iron Ingot",
            "1b 5c",
            "Steel Ingot",
            "2b 5c",
            "Purse: 1g 1s 2b 3c.",
        ),
    ),
    ParserCase(
        name="currency pickups",
        commands=(
            "new",
            "search",
            "find hidden cash",
            "take hidden cash",
            "search",
            "find buried coin",
            "take buried coin",
            "coins",
            "quit",
        ),
        must_contain=(
            'Matches for "hidden cash"',
            "Hidden Cash",
            "[west_of_house]",
            "You uncover a concealed stash: Hidden Cash.",
            "You uncover a concealed stash: Buried Coin.",
            "You take Hidden Cash (1 bronze, 2 copper go into your purse).",
            "You take Buried Coin (6 copper go into your purse).",
            "purse: 1 silver, 1 bronze, 8 copper.",
        ),
    ),
    ParserCase(
        name="bartering system",
        commands=(
            "new",
            "take scrap_metal",
            "take lockpick",
            "north",
            "east",
            "pick lock",
            "east",
            "east",
            "haggle buy rope",
            "buy rope",
            "haggle sell scrap metal",
            "sell scrap metal",
            "trade history",
            "quit",
        ),
        must_contain=(
            "You haggle Rope down to 9 copper (was 1 bronze).",
            "Bought Rope for 9 copper (listed 1 bronze",
            "You talk Scrap Metal up to 6 copper (was 5 copper).",
            "Sold Scrap Metal for 6 copper (listed 4 copper",
            "[buy] bought Rope from miller for 9 copper (list 1 bronze)",
            "[sell] sold Scrap Metal to miller for 6 copper (list 4 copper)",
        ),
    ),
    ParserCase(
        name="bulk trading",
        commands=(
            "new",
            "take scrap_metal",
            "take wood_scrap",
            "take lockpick",
            "north",
            "east",
            "pick lock",
            "east",
            "east",
            "buy all except rope",
            "sell all except bread",
            "trade history",
            "quit",
        ),
        must_contain=(
            "You buy 2 listed goods for 1 bronze, 3 copper total.",
            "You sell 2 carried goods for 7 copper total.",
            "[buy] bought Flour from miller for 5 copper",
            "[buy] bought Bread from miller for 8 copper",
            "[sell] sold Scrap Metal to miller for 5 copper",
            "[sell] sold Wood Scrap to miller for 2 copper",
        ),
    ),
    ParserCase(
        name="trade history ledger",
        commands=(
            "new",
            "take scrap_metal",
            "north",
            "north",
            "northeast",
            "east",
            "east",
            "buy metal scrap",
            "sell scrap metal",
            "trade history",
            "quick save",
            "quick load",
            "trade history",
            "quit",
        ),
        must_contain=(
            "Bought Metal Scrap for 3 copper.",
            "Sold Scrap Metal for 2 copper.",
            "TRADE HISTORY",
            "[buy] bought Metal Scrap from blacksmith for 3 copper",
            "[sell] sold Scrap Metal to blacksmith for 2 copper",
            "saved with your game",
        ),
    ),
    ParserCase(
        name="exit npc density tags",
        commands=("new", "north", "north", "exits", "quit"),
        must_contain=("Forest Path", "deep_forest", "[npc:1]"),
    ),
    ParserCase(
        name="encumbrance aware hints",
        commands=(
            "new",
            "take scrap_metal",
            "take wood_scrap",
            "take lockpick",
            "hint",
            "quit",
        ),
        must_contain=(
            "Pack pressure is building",
            "loot weight",
            "Heaviest right now:",
            "Scrap Metal",
            "Wood Scrap",
        ),
    ),
    ParserCase(
        name="compare labels",
        commands=(
            "new",
            "take scrap metal",
            "take lockpick",
            "compare scrap metal / lockpick",
            "quit",
        ),
        must_contain=(
            "=== APPRAISAL ===",
            "Scrap Metal",
            "Lockpick",
            "Est. value:",
        ),
    ),
    ParserCase(
        name="trail depth view",
        commands=("new", "north", "south", "north", "trail 2", "quit"),
        must_contain=(
            "Back-trail (oldest to newest, showing last 2 of 3 step(s)):",
            "  1. ",
            "  2. ",
            "Use  back  or  back <n>  to retrace.",
        ),
    ),
    ParserCase(
        name="trail depth bounds",
        commands=("new", "trail 99", "quit"),
        must_contain=("Use: trail <1-25>  or  trail.",),
    ),
    ParserCase(
        name="trail depth available cap",
        commands=("new", "north", "trail 5", "quit"),
        must_contain=(
            "Back-trail (oldest to newest, showing all 1 available step(s) (requested 5)):",
            "Use  back  or  back <n>  to retrace.",
        ),
    ),
    ParserCase(
        name="npc routine relocation",
        commands=("new", "where tavern_keeper", "wait until night", "where tavern_keeper", "quit"),
        must_contain=(
            "The Rusty Anchor - Common Room",
            "afternoon routine: hosting the common room",
            "Tavern Back Room",
            "night routine: handling quiet business in the back room",
        ),
    ),
)


def clean_output(text: str) -> str:
    text = ANSI_RE.sub("", text)
    return text.replace("\r\n", "\n").replace("\r", "\n")


def build() -> None:
    p = subprocess.run(
        [str(ROOT / "build_win_gcc.bat")],
        cwd=ROOT,
        text=True,
        capture_output=True,
        shell=False,
    )
    if p.returncode != 0:
        print("BUILD FAILED")
        print(p.stdout)
        print(p.stderr, file=sys.stderr)
        raise SystemExit(p.returncode)


def apply_save_edits(save_path: Path, edits: tuple[tuple[str, str], ...]) -> None:
    text = save_path.read_text(encoding="utf-8")
    for old, new in edits:
        if old not in text:
            raise AssertionError(f"save edit needle not found: {old!r}")
        text = text.replace(old, new, 1)
    save_path.write_text(text, encoding="utf-8")


def run_case(case: ParserCase, *, exe: Path, timeout: int) -> tuple[bool, str]:
    env = os.environ.copy()
    env["AETER_AUTOTEST"] = "1"
    env.setdefault("NO_COLOR", "1")
    script = "\n".join(case.commands) + "\n"
    with tempfile.TemporaryDirectory(prefix="aet-parser-reg-") as td:
        save_path = Path(td) / "save.txt"
        if case.pre_commands:
            pre_script = "\n".join(case.pre_commands) + "\n"
            pre = subprocess.run(
                [str(exe), "--save", str(save_path)],
                input=pre_script,
                cwd=ROOT,
                env=env,
                text=True,
                capture_output=True,
                timeout=timeout,
            )
            if pre.returncode != 0:
                pre_out = clean_output((pre.stdout or "") + "\n" + (pre.stderr or ""))
                return False, f"pre-run returned {pre.returncode}\n{pre_out}"
        if case.save_edits:
            try:
                apply_save_edits(save_path, case.save_edits)
            except Exception as exc:
                return False, f"save edit failed: {exc}"
        p = subprocess.run(
            [str(exe), "--save", str(save_path)],
            input=script,
            cwd=ROOT,
            env=env,
            text=True,
            capture_output=True,
            timeout=timeout,
        )
    out = clean_output((p.stdout or "") + "\n" + (p.stderr or ""))
    if p.returncode != 0:
        return False, f"process returned {p.returncode}\n{out}"

    missing = [needle for needle in case.must_contain if needle not in out]
    forbidden = [needle for needle in case.must_not_contain if needle in out]
    if missing or forbidden:
        lines = []
        if missing:
            lines.append("missing: " + ", ".join(repr(x) for x in missing))
        if forbidden:
            lines.append("forbidden: " + ", ".join(repr(x) for x in forbidden))
        lines.append("--- output tail ---")
        lines.extend(out.splitlines()[-80:])
        return False, "\n".join(lines)
    return True, ""


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Run native aeternitas64 parser regression cases."
    )
    ap.add_argument("--exe", default=str(EXE), help="Executable to test.")
    ap.add_argument("--build", action="store_true", help="Build before running.")
    ap.add_argument("--timeout", type=int, default=120, help="Per-case timeout seconds.")
    ap.add_argument("--list", action="store_true", help="List cases and exit.")
    args = ap.parse_args()

    exe = Path(args.exe)

    if args.list:
        for case in CASES:
            print(case.name)
        return 0

    if args.build:
        build()
    if not exe.is_file():
        print(f"Missing executable: {exe}", file=sys.stderr)
        print("Run .\\build_win_gcc.bat or pass --build.", file=sys.stderr)
        return 2

    failures = 0
    for case in CASES:
        ok, detail = run_case(case, exe=exe, timeout=args.timeout)
        if ok:
            print(f"OK  {case.name}")
        else:
            failures += 1
            print(f"FAIL {case.name}")
            print(detail)
    if failures:
        print(f"\nFAIL: {failures}/{len(CASES)} parser regression case(s) failed.")
        return 1
    print(f"\nOK: {len(CASES)} parser regression case(s) passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
