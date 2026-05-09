#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import struct
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))
from repo_paths import recovery_txts_dir, world_tables_recovered_json  # noqa: E402

ART = ROOT / "recovery_artifacts"
IMAGE_BASE_DEFAULT = 0x140000000


def read_u16(b: bytes, o: int) -> int:
    return struct.unpack_from("<H", b, o)[0]


def read_u32(b: bytes, o: int) -> int:
    return struct.unpack_from("<I", b, o)[0]


def read_u64(b: bytes, o: int) -> int:
    return struct.unpack_from("<Q", b, o)[0]


def parse_pe(path: Path) -> dict:
    exe = path.read_bytes()
    pe_off = read_u32(exe, 0x3C)
    if exe[pe_off : pe_off + 4] != b"PE\x00\x00":
        raise RuntimeError(f"{path} is not a PE executable")

    coff = pe_off + 4
    machine = read_u16(exe, coff)
    nsects = read_u16(exe, coff + 2)
    opt_size = read_u16(exe, coff + 16)
    opt_off = coff + 20
    image_base = read_u64(exe, opt_off + 24)
    entry_rva = read_u32(exe, opt_off + 16)
    size_image = read_u32(exe, opt_off + 56)
    subsystem = read_u16(exe, opt_off + 68)

    sect_off = opt_off + opt_size
    sections = []
    for i in range(nsects):
        s = sect_off + i * 40
        name = exe[s : s + 8].rstrip(b"\x00").decode("ascii", errors="ignore")
        sections.append(
            {
                "name": name,
                "virtual_size": read_u32(exe, s + 8),
                "virtual_address": read_u32(exe, s + 12),
                "raw_size": read_u32(exe, s + 16),
                "raw_pointer": read_u32(exe, s + 20),
            }
        )
    return {
        "path": str(path),
        "size": path.stat().st_size,
        "machine": f"0x{machine:04x}",
        "image_base": f"0x{image_base:x}",
        "entry_point": f"0x{image_base + entry_rva:x}",
        "size_of_image": size_image,
        "subsystem": subsystem,
        "sections": sections,
    }


def run(args: list[str]) -> str:
    return subprocess.check_output(args, cwd=ROOT, text=True, errors="ignore")


def parse_symbols(path: Path) -> tuple[list[dict], list[str]]:
    text = run(["nm", "-C", "--defined-only", str(path)])
    symbols = []
    source_units: list[str] = []
    for line in text.splitlines():
        m = re.match(r"^([0-9a-fA-F]{16})\s+([A-Za-z])\s+(.+)$", line.strip())
        if not m:
            continue
        addr, kind, name = m.groups()
        symbols.append({"address": f"0x{int(addr, 16):x}", "kind": kind, "name": name})
        if name.endswith((".c", ".S", ".s")) and name not in source_units:
            source_units.append(name)
    return symbols, source_units


def parse_coff_file_records(symbol_dump: Path) -> list[str]:
    if not symbol_dump.exists():
        return []
    out: list[str] = []
    lines = symbol_dump.read_text(encoding="utf-8", errors="ignore").splitlines()
    for i, line in enumerate(lines):
        if "(scl 103)" in line and i + 1 < len(lines):
            m = re.search(r"\)\s+0x[0-9a-fA-F]+\s+(.+)$", line)
            if m:
                name = m.group(1).strip()
                if name and name not in out:
                    out.append(name)
    return out


def main() -> int:
    ART.mkdir(exist_ok=True)
    TXTS = recovery_txts_dir()
    TXTS.mkdir(parents=True, exist_ok=True)
    original = ROOT / "aeternitas64.exe"
    recovered = ROOT / "aeternitas64_recovered.exe"

    original_symbols, source_units_from_nm = parse_symbols(original)
    recovered_symbols, _ = parse_symbols(recovered) if recovered.exists() else ([], [])
    original_names = {s["name"] for s in original_symbols}
    recovered_names = {s["name"] for s in recovered_symbols}

    world_path = world_tables_recovered_json()
    world = json.loads(world_path.read_text(encoding="utf-8")) if world_path.exists() else {}

    public_game_symbols = [
        s
        for s in original_symbols
        if not s["name"].startswith((".", "__", "_", "imp_"))
        and not s["name"].startswith("lib")
        and s["kind"].lower() in {"t", "d", "r", "b"}
    ]

    index = {
        "original_pe": parse_pe(original),
        "recovered_pe": parse_pe(recovered) if recovered.exists() else None,
        "toolchain": {
            "compiler_hint": "GNU C23 / MinGW-W64 GCC 15.2.0; debug and COFF symbols present",
            "evidence_files": [
                "recovery_artifacts/txts/exe_symbols.txt",
                "recovery_artifacts/txts/exe_disasm_all.txt",
                "recovery_artifacts/txts/dwarf_info_index.txt",
                "recovery_artifacts/txts/dwarf_decoded_lines.txt",
                "recovery_artifacts/txts/exe_strings.txt",
            ],
        },
        "source_units": sorted(
            set(source_units_from_nm + parse_coff_file_records(TXTS / "exe_symbols.txt"))
        ),
        "symbol_counts": {
            "original_defined": len(original_symbols),
            "recovered_defined": len(recovered_symbols),
            "public_game_like_original": len(public_game_symbols),
            "missing_from_recovered_by_name": len(original_names - recovered_names),
            "extra_in_recovered_by_name": len(recovered_names - original_names),
        },
        "world_tables": {
            "room_count": world.get("room_count"),
            "dir_count": world.get("dir_count"),
            "has_slugs": bool(world.get("slugs")),
            "has_titles": bool(world.get("titles")),
            "has_blurbs": bool(world.get("blurbs")),
            "has_regions": bool(world.get("regions")),
            "has_entities": bool(world.get("entities")),
            "visible_item_room_count": sum(1 for x in world.get("item_lists", []) if x),
            "visible_item_count": sum(len(x) for x in world.get("item_lists", [])),
            "hidden_item_room_count": sum(1 for x in world.get("hidden_item_lists", []) if x),
            "hidden_item_count": sum(len(x) for x in world.get("hidden_item_lists", [])),
            "npc_count": len(world.get("npc_lines", [])),
            "merchant_count": len(world.get("merchants", [])),
            "has_exits": bool(world.get("exits")),
            "known_table_symbols": [
                "WORLD_SLUGS",
                "WORLD_TITLES",
                "WORLD_BLURBS",
                "WORLD_REGIONS",
                "WORLD_ROOM_DARK",
                "WORLD_ROOM_ENTITIES",
                "WORLD_EXITS",
                "WORLD_ITEM_LISTS",
                "WORLD_HIDDEN_ITEM_LISTS",
            ],
        },
        "notable_symbols": [
            s for s in public_game_symbols if re.search(r"^(main|cmd_|run_|format_|load_|save_|aet_|pc_|WORLD_)", s["name"])
        ][:220],
    }

    (TXTS / "recovery_index.json").write_text(json.dumps(index, indent=2), encoding="utf-8")

    md = []
    md.append("# Aeternitas64 EXE Recovery Index")
    md.append("")
    md.append("## Binary")
    md.append(f"- Original: `{original.name}` ({index['original_pe']['size']} bytes)")
    md.append(f"- Entry point: `{index['original_pe']['entry_point']}`")
    md.append("- Format: PE32+ x86-64 console executable")
    md.append("- Toolchain evidence: MinGW-W64 GCC 15.2.0, C23, `-g`, `-O2`")
    md.append("")
    md.append("## Recovery Inputs")
    for f in index["toolchain"]["evidence_files"]:
        md.append(f"- `{f}`")
    md.append("")
    md.append("## Source Units Named By The EXE")
    for unit in index["source_units"]:
        md.append(f"- `{unit}`")
    md.append("")
    md.append("## Recovered World Tables")
    wt = index["world_tables"]
    md.append(f"- Rooms: `{wt['room_count']}`")
    md.append(f"- Directions per room: `{wt['dir_count']}`")
    md.append("- Recovered arrays: slugs, titles, blurbs, regions, entities, darkness flags, exits")
    md.append(
        f"- Visible room item lists: `{wt['visible_item_room_count']}` rooms, `{wt['visible_item_count']}` entries"
    )
    md.append(
        f"- Hidden room item lists: `{wt['hidden_item_room_count']}` rooms, `{wt['hidden_item_count']}` entries"
    )
    md.append(f"- NPC line sets: `{wt['npc_count']}`")
    md.append(f"- Merchant tables: `{wt['merchant_count']}`")
    md.append("")
    md.append("## Symbol Parity")
    sc = index["symbol_counts"]
    md.append(f"- Original defined symbols: `{sc['original_defined']}`")
    md.append(f"- Recovered defined symbols: `{sc['recovered_defined']}`")
    md.append(f"- Game-like original symbols indexed: `{sc['public_game_like_original']}`")
    md.append("")
    md.append("## Next Recovery Targets")
    md.append("- Build parity scripts for NPC topics, merchant wares, and room loot against the original EXE.")
    md.append("- Use symbol-address anchored disassembly for large `cmd_*` and `run_*` functions when behavior differs from the original.")
    md.append("- Keep parity tests against the original EXE for command output, save/load serialization, world navigation, merchant flows, and character creation.")
    md.append("")
    (TXTS / "RECOVERY_INDEX.md").write_text("\n".join(md), encoding="utf-8")
    print(f"Wrote {TXTS / 'recovery_index.json'}")
    print(f"Wrote {TXTS / 'RECOVERY_INDEX.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
