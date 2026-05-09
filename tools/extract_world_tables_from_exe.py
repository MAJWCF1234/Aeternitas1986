#!/usr/bin/env python3
from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

_TOOLS = Path(__file__).resolve().parent
if str(_TOOLS) not in sys.path:
    sys.path.insert(0, str(_TOOLS))
from repo_paths import recovery_txts_dir, world_tables_recovered_json  # noqa: E402


ROOM_COUNT = 161
DIR_COUNT = 19


def read_u16(b: bytes, o: int) -> int:
    return struct.unpack_from("<H", b, o)[0]


def read_u32(b: bytes, o: int) -> int:
    return struct.unpack_from("<I", b, o)[0]


def read_u64(b: bytes, o: int) -> int:
    return struct.unpack_from("<Q", b, o)[0]


def parse_pe_sections(exe: bytes):
    pe_off = read_u32(exe, 0x3C)
    if exe[pe_off : pe_off + 4] != b"PE\x00\x00":
        raise RuntimeError("Not a PE file")
    coff = pe_off + 4
    nsects = read_u16(exe, coff + 2)
    opt_size = read_u16(exe, coff + 16)
    opt_off = coff + 20
    magic = read_u16(exe, opt_off)
    if magic != 0x20B:
        raise RuntimeError("Only PE32+ supported")
    image_base = read_u64(exe, opt_off + 24)
    sect_off = opt_off + opt_size
    sections = []
    for i in range(nsects):
        s = sect_off + i * 40
        name = exe[s : s + 8].rstrip(b"\x00").decode("ascii", errors="ignore")
        vsize = read_u32(exe, s + 8)
        vaddr = read_u32(exe, s + 12)
        raw_size = read_u32(exe, s + 16)
        raw_ptr = read_u32(exe, s + 20)
        sections.append((name, vaddr, vsize, raw_ptr, raw_size))
    return image_base, sections


def rva_to_off(rva: int, sections) -> int | None:
    for _, vaddr, vsize, raw_ptr, raw_size in sections:
        span = max(vsize, raw_size)
        if vaddr <= rva < vaddr + span:
            return raw_ptr + (rva - vaddr)
    return None


def read_cstr(exe: bytes, off: int, cap: int = 8192) -> str:
    out = bytearray()
    for i in range(cap):
        if off + i >= len(exe):
            break
        c = exe[off + i]
        if c == 0:
            break
        out.append(c)
    return out.decode("utf-8", errors="ignore")


def read_ptr_array_strings(exe: bytes, sections, image_base: int, rva: int, n: int):
    out: list[str] = []
    base_off = rva_to_off(rva, sections)
    if base_off is None:
        return out
    for i in range(n):
        p = read_u64(exe, base_off + i * 8)
        prva = p - image_base if p >= image_base else p
        soff = rva_to_off(prva, sections)
        if soff is None:
            out.append("")
            continue
        out.append(read_cstr(exe, soff))
    return out


def read_string_list_at_ptr(exe: bytes, sections, image_base: int, ptr: int, cap: int = 128):
    prva = ptr - image_base if ptr >= image_base else ptr
    off = rva_to_off(prva, sections)
    if off is None:
        return []
    out: list[str] = []
    for i in range(cap):
        p = read_u64(exe, off + i * 8)
        if p == 0:
            break
        srva = p - image_base if p >= image_base else p
        soff = rva_to_off(srva, sections)
        if soff is None:
            break
        out.append(read_cstr(exe, soff))
    return out


def read_ptr_array_string_lists(exe: bytes, sections, image_base: int, rva: int, n: int):
    out: list[list[str]] = []
    base_off = rva_to_off(rva, sections)
    if base_off is None:
        return out
    for i in range(n):
        p = read_u64(exe, base_off + i * 8)
        if p == 0:
            out.append([])
            continue
        out.append(read_string_list_at_ptr(exe, sections, image_base, p))
    return out


def read_ptr_array_until_null(exe: bytes, sections, image_base: int, rva: int, cap: int = 512):
    out: list[str] = []
    base_off = rva_to_off(rva, sections)
    if base_off is None:
        return out
    for i in range(cap):
        p = read_u64(exe, base_off + i * 8)
        if p == 0:
            break
        prva = p - image_base if p >= image_base else p
        soff = rva_to_off(prva, sections)
        if soff is None:
            break
        out.append(read_cstr(exe, soff))
    return out


def read_offer_list_at_ptr(exe: bytes, sections, image_base: int, ptr: int, cap: int = 128):
    prva = ptr - image_base if ptr >= image_base else ptr
    off = rva_to_off(prva, sections)
    if off is None:
        return []
    out: list[dict[str, int | str]] = []
    for i in range(cap):
        p = read_u64(exe, off + i * 16)
        price = struct.unpack_from("<i", exe, off + i * 16 + 8)[0]
        if p == 0:
            break
        srva = p - image_base if p >= image_base else p
        soff = rva_to_off(srva, sections)
        if soff is None:
            break
        out.append({"item": read_cstr(exe, soff), "price": price})
    return out


def read_topic_list_at_ptr(exe: bytes, sections, image_base: int, ptr: int, cap: int = 128):
    prva = ptr - image_base if ptr >= image_base else ptr
    off = rva_to_off(prva, sections)
    if off is None:
        return []
    out: list[dict[str, str]] = []
    for i in range(cap):
        kp = read_u64(exe, off + i * 16)
        rp = read_u64(exe, off + i * 16 + 8)
        if kp == 0 or rp == 0:
            break
        krva = kp - image_base if kp >= image_base else kp
        rrva = rp - image_base if rp >= image_base else rp
        koff = rva_to_off(krva, sections)
        roff = rva_to_off(rrva, sections)
        if koff is None or roff is None:
            break
        out.append({"keywords": read_cstr(exe, koff), "response": read_cstr(exe, roff)})
    return out


def read_npc_lines(exe: bytes, sections, image_base: int, rva: int, n: int):
    base_off = rva_to_off(rva, sections)
    if base_off is None:
        return []
    out = []
    for i in range(n):
        off = base_off + i * 32
        slug_p = read_u64(exe, off)
        greeting_p = read_u64(exe, off + 8)
        chatter_p = read_u64(exe, off + 16)
        topics_p = read_u64(exe, off + 24)
        if slug_p == 0:
            break
        slug_off = rva_to_off(slug_p - image_base if slug_p >= image_base else slug_p, sections)
        greeting_off = rva_to_off(
            greeting_p - image_base if greeting_p >= image_base else greeting_p, sections
        )
        if slug_off is None or greeting_off is None:
            continue
        out.append(
            {
                "slug": read_cstr(exe, slug_off),
                "greeting": read_cstr(exe, greeting_off),
                "chatter": read_string_list_at_ptr(exe, sections, image_base, chatter_p),
                "topics": read_topic_list_at_ptr(exe, sections, image_base, topics_p),
            }
        )
    return out


def read_merchants(exe: bytes, sections, image_base: int, rva: int, n: int):
    base_off = rva_to_off(rva, sections)
    if base_off is None:
        return []
    out = []
    for i in range(n):
        off = base_off + i * 24
        slug_p = read_u64(exe, off)
        stock_p = read_u64(exe, off + 8)
        buys_p = read_u64(exe, off + 16)
        if slug_p == 0:
            break
        slug_off = rva_to_off(slug_p - image_base if slug_p >= image_base else slug_p, sections)
        if slug_off is None:
            continue
        out.append(
            {
                "slug": read_cstr(exe, slug_off),
                "stock": read_offer_list_at_ptr(exe, sections, image_base, stock_p),
                "buys": read_offer_list_at_ptr(exe, sections, image_base, buys_p),
            }
        )
    return out


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    exe_path = root / "aeternitas64.exe"
    recovery_txts_dir().mkdir(parents=True, exist_ok=True)
    out_path = world_tables_recovered_json()
    out_path.parent.mkdir(parents=True, exist_ok=True)

    exe = exe_path.read_bytes()
    image_base, sections = parse_pe_sections(exe)

    rdata_rva = next((va for name, va, *_ in sections if name == ".rdata"), None)
    if rdata_rva is None:
        raise RuntimeError(".rdata section not found")

    # From objdump symbols (section-relative offsets in .rdata).
    rva_world_slugs = rdata_rva + 0x0030A40
    rva_world_titles = rdata_rva + 0x002FCE0
    rva_world_blurbs = rdata_rva + 0x002EE20
    rva_world_regions = rdata_rva + 0x0023AE0
    rva_world_entities = rdata_rva + 0x0023500
    rva_world_dark = rdata_rva + 0x0023A20
    rva_world_exits = rdata_rva + 0x0021D00
    rva_world_items = rdata_rva + 0x001DEE0
    rva_world_hidden_items = rdata_rva + 0x001D9C0
    rva_consume_food = rdata_rva + 0x0011200
    rva_consume_drink = rdata_rva + 0x0011180
    rva_quest_hints = rdata_rva + 0x0016C00
    rva_npc_lines = rdata_rva + 0x0017EA0
    rva_merchants = rdata_rva + 0x0017040

    slugs = read_ptr_array_strings(exe, sections, image_base, rva_world_slugs, ROOM_COUNT)
    titles = read_ptr_array_strings(exe, sections, image_base, rva_world_titles, ROOM_COUNT)
    blurbs = read_ptr_array_strings(exe, sections, image_base, rva_world_blurbs, ROOM_COUNT)
    regions = read_ptr_array_strings(exe, sections, image_base, rva_world_regions, ROOM_COUNT)
    entities = read_ptr_array_strings(exe, sections, image_base, rva_world_entities, ROOM_COUNT)
    item_lists = read_ptr_array_string_lists(exe, sections, image_base, rva_world_items, ROOM_COUNT)
    hidden_item_lists = read_ptr_array_string_lists(
        exe, sections, image_base, rva_world_hidden_items, ROOM_COUNT
    )
    consume_food_ids = read_ptr_array_until_null(exe, sections, image_base, rva_consume_food)
    consume_drink_ids = read_ptr_array_until_null(exe, sections, image_base, rva_consume_drink)
    quest_hints = read_ptr_array_until_null(exe, sections, image_base, rva_quest_hints)
    npc_lines = read_npc_lines(exe, sections, image_base, rva_npc_lines, 10)
    merchants = read_merchants(exe, sections, image_base, rva_merchants, 7)

    dark: list[int] = []
    dark_off = rva_to_off(rva_world_dark, sections)
    if dark_off is not None:
        dark = list(exe[dark_off : dark_off + ROOM_COUNT])

    exits: list[list[int]] = []
    ex_off = rva_to_off(rva_world_exits, sections)
    if ex_off is not None:
        for r in range(ROOM_COUNT):
            row = []
            for d in range(DIR_COUNT):
                v = struct.unpack_from("<h", exe, ex_off + (r * DIR_COUNT + d) * 2)[0]
                row.append(v)
            exits.append(row)

    payload = {
        "room_count": ROOM_COUNT,
        "dir_count": DIR_COUNT,
        "slugs": slugs,
        "titles": titles,
        "blurbs": blurbs,
        "regions": regions,
        "entities": entities,
        "item_lists": item_lists,
        "hidden_item_lists": hidden_item_lists,
        "consume_food_ids": consume_food_ids,
        "consume_drink_ids": consume_drink_ids,
        "quest_hints": quest_hints,
        "npc_lines": npc_lines,
        "merchants": merchants,
        "dark": dark,
        "exits": exits,
        "image_base": image_base,
    }
    out_path.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"Wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
