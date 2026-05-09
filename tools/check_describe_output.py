#!/usr/bin/env python3
"""Run describe_pc_test and assert narrative invariants (no crash, key markers).

Build the harness first: ``py -3 tools/run_describe_pc_test.py --build-only``.
"""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXE = ROOT / ("describe_pc_test.exe" if sys.platform == "win32" else "describe_pc_test")

# Phrases that must appear somewhere in the full harness transcript.
REQUIRED_SUBSTRINGS = (
    "You are centaur-built",
    "You are serpent from the waist down",
    "You are drider-shaped",
    "You are harpy-formed",
    "Winged bearing stacks muscle along wing roots",
    "Where a human pelvis would frame stride, serpent girth",
    "Horse morph lengthens flank and neck",
    # Stat ties at max: pick two alphabetically among tied peaks (harness case).
    "especially agility and charisma",
    # Gorgon / Lizard: carriage + voice branches (non-default doorway prose).
    "stone myths borrow your reflection",
    "It puts hiss and heat ahead of pretty apologies",
    # Orc / Goblin / Elf / Dwarf-stock voice & coda paths.
    "tunnel-width and war-camp bluntness",
    "slips through gaps polite rooms pretend were never there",
    "braids cool vowels through chambers built for shorter memories",
    "rings hearth and stone closer than tallfolk bother to hear",
    # Constructs: voice path (Android / Cyborg / Golem harness cases).
    "It couples through seams and borrowed lung-space differently",
    # Biology-neutral carriage openers + identity + face for construct / slime.
    "torque budgets and seam allowance shaped",
    "yielding mass and surface tension shaped",
    "by artifice",
    "by strange chemistry",
    "finish rides chassis lines",
    # Short stature lines for non-biological races (harness cases).
    "short for your manufacture band",
    "strain prefers density over span",
    # Silhouette scan line + Slime race gloss (construct / slime vs linen / meat).
    "cant of casing where linen lies about torque paths",
    "gel rehearses ribs under veil",
    "charts drew meat alone",
    # Face framing: construct vs slime vs bone (Android + Slime harness males).
    "Your faceplate reads masculine",
    "Your surface hints masculine",
    # Construct / Slime: feminine + they face lines + bald crown prose.
    "Your mask reads feminine",
    "facial calibration hedges",
    "Your surface hints feminine",
    "mask keeps taxonomy unstable",
    "sensor rings and bare laminate",
    "meniscus offers combs nothing to purchase",
    # Scarless face + marks ledger + corruption for constructs / slime.
    "laminate stays honest",
    "surface stays rumor-smooth",
    "coolant traces and stress bloom",
    "drip-edge sheen and idle shimmer",
    "thermal creep across housing",
    "viscosity gone rude",
    # Construct / slime scar ledgers + scarred face clauses.
    "free of obvious scoring",
    "free of obvious tear scars",
    "hairline faults that catch hazard-light",
    "mapped by heavy relay scoring",
    "cross-hatched with healed seams",
    "mapped by turbulent scarring",
    "one seam reopened by blunt luck",
    "relay tracks stacked across plating",
    "turbulent planes where insults stacked",
    # Tattoo ledgers (construct / slime vs skin ink).
    "live in ROM or in silence",
    "gel memory or in silence",
    "routing inked into laminate",
    "routing becomes second housing",
    "stain that clings with intent",
    "second tide",
    # Voice braid (construct / slime vs breath/cord/habit).
    "exciter mesh, and mapped habit braided into sound",
    "meniscus pull, and stubborn habit braided into sound",
    # Intimate anatomy: construct / slime vs flesh plumbing.
    "interface stays smooth and deliberate",
    "viscosity hoards interest elsewhere",
    "thermal mass banked close",
    "hardware the world refuses to mythologize",
    "hydraulic rise when demand insists",
    "tracks specification and bearing",
    "housing stays spare — no paired reservoir",
    "Your sack carries compliance",
    "thermal bloom banks behind curls",
    "warmth pools behind curls",
    "neither subroutine cancels the other",
    "neither tide cancels the other",
    "restless stack gathers behind your root",
    "without reservoirs to share the blame",
    # Labeled cock + ball fullness with sack (construct / slime vs seed/torque).
    "mass enough to crowd routing",
    "heft enough to crowd shear",
    "reservoir pressure simmering",
    "gel tide simmering",
    "spine remembers torque",
    "meniscus remembers appetite",
    "standing still feels like calibration",
    "mercy measured in inches and swollen tide",
    # Construct / slime rank-1 lactation (breast CC path).
    "White coolant gathers",
    "tide remembers appetite",
    # No-balls fullness tier 2 + human tier 3 (weight not ballast).
    "spine keeps torque tally",
    "gel keeps tide tally",
    "without weight to share the blame",
    # Explicit balls labels + gear weapon-only closer (construct / slime).
    "another torque datum",
    "another swell reading",
    "Your chassis reads light",
    "Your mantle reads light",
    # Gear closer: both-none / armor-only / full kit (construct / slime).
    "lantern-light across your chassis",
    "lacquered cuirass catches",
    "specification and stubborn uptime",
    "measured viscosity beneath",
    "alloy and leather shifting",
    "slick resin shifting",
    "catches ripple instead of only curve",
    "routing straps",
    "borrowed shells",
    # Muscle/softness clause + slime race fallback + infer dedupe safety.
    "Training carved torque into your silhouette",
    "calibration holds from scalp to heel",
    "torque stacks under",
    "Training carved strain into your silhouette",
    "gel folded tight where joints demand",
    "traits lace through surface and stance",
    # Low stat carriage + vagina-only default anatomy (construct / slime).
    "Your chassis does not yet advertise",
    "Your mantle does not yet advertise",
    "seams warming wherever linen pretends",
    "meniscus film climbing wherever cloth",
    # High stat carriage (Wear and torque / tide vs habit).
    "Wear and torque have marked you",
    "Wear and tide have marked you",
    "telegraph first in motion",
    "surface first in motion",
    "parse through plating",
    "read through sheen",
    # infer_modification_prose: scales + slime-token gleam (construct / slime).
    "cling to veil and profile",
    "tide alone draws the limit",
    # Stature Tall/Gigantic + tags cant + dragon morph gloss (construct / slime).
    "by manufacture intent",
    "where mass pooled upward",
    "tall by engineered reach",
    "tall where tide drew you up",
    "That first read rides junction cant",
    "That first read rides surface cant",
    "heat banks quiet under veil",
    "heat banks quiet under housing",
    # Hips + space carriage coda for slime-like edge races (Goomurali / tide hips).
    "substrate pretends it voted",
    "hips steer tide before linen earns its excuse",
    "outline trades honesty with every threshold",
    "strangers finish pretending rooms stay dry",
    # Height parsing: CC parentheses, empty string, custom tokens (construct/slime).
    "lintels read as suggestions",
    "doorways learn patience",
    "charts assume what calibration never promised",
    "tide levels where gossip hoped for drama",
    "reach rated without apology",
    "span borrowed from appetite",
    # Average token stature (construct / slime).
    "nominal where the chart pretends neutrality",
    "quiet until tide argues",
    # Beast-kin infer (infer_modification_prose): slime strain vs construct routing.
    "Beast strain shows in ear-set",
    "Beast routing shows in ear-set",
    # Composite construct morphs (race_extra_clause fallback vs blood bend).
    "routing bends your outline away from the human mean",
)

FORBIDDEN_SUBSTRINGS = (
    "human template",  # removed generic morph filler
)


def main() -> int:
    if not EXE.is_file():
        print(f"Missing {EXE}; build with tools/run_describe_pc_test.py first.", file=sys.stderr)
        return 2
    p = subprocess.run([str(EXE)], cwd=ROOT, text=True, capture_output=True, encoding="utf-8")
    if p.returncode != 0:
        print(p.stderr or p.stdout, file=sys.stderr)
        return p.returncode
    text = p.stdout
    if len(text) < 50_000:
        print(f"Output unexpectedly short ({len(text)} chars).", file=sys.stderr)
        return 1
    missing = [s for s in REQUIRED_SUBSTRINGS if s not in text]
    if missing:
        print("Missing required fragments:", file=sys.stderr)
        for m in missing:
            print(f"  - {m}", file=sys.stderr)
        return 1
    bad = [s for s in FORBIDDEN_SUBSTRINGS if s in text]
    if bad:
        print("Forbidden fragments present:", file=sys.stderr)
        for b in bad:
            print(f"  - {b}", file=sys.stderr)
        return 1
    cases = len(re.findall(r"(?m)^# ", text))
    print(f"OK: describe harness ({cases} cases, {len(text)} chars)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
