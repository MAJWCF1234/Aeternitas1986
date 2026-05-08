#!/usr/bin/env python3
from __future__ import annotations

import json
from pathlib import Path


def cstr(s: str) -> str:
    s = s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    return f'"{s}"'


def emit_array_str(name: str, vals: list[str]) -> str:
    lines = [f"static const char *const {name}[WORLD_ROOM_COUNT] = {{"]
    for i, v in enumerate(vals):
        sep = "," if i < len(vals) - 1 else ""
        lines.append(f"  {cstr(v)}{sep}")
    lines.append("};")
    return "\n".join(lines)


def emit_array_u8(name: str, vals: list[int]) -> str:
    lines = [f"static const unsigned char {name}[WORLD_ROOM_COUNT] = {{"]
    row = []
    for i, v in enumerate(vals):
        row.append(str(v))
        if len(row) == 24 or i == len(vals) - 1:
            lines.append("  " + ", ".join(row) + ("," if i < len(vals) - 1 else ""))
            row = []
    lines.append("};")
    return "\n".join(lines)


def emit_array_exits(name: str, vals: list[list[int]]) -> str:
    lines = [f"static const int {name}[WORLD_ROOM_COUNT][DIR_COUNT] = {{"]
    for r in vals:
        lines.append("  {" + ", ".join(str(x) for x in r) + "},")
    lines.append("};")
    return "\n".join(lines)


def emit_room_item_lists(prefix: str, vals: list[list[str]]) -> str:
    lines: list[str] = []
    for i, items in enumerate(vals):
        if not items:
            continue
        lines.append(f"static const char *const {prefix}_{i:03d}[] = {{")
        for item in items:
            lines.append(f"  {cstr(item)},")
        lines.append("  NULL")
        lines.append("};")
    lines.append(f"static const char *const *const {prefix}[WORLD_ROOM_COUNT] = {{")
    for i, items in enumerate(vals):
        ref = f"{prefix}_{i:03d}" if items else "EMPTY_LIST"
        sep = "," if i < len(vals) - 1 else ""
        lines.append(f"  {ref}{sep}")
    lines.append("};")
    return "\n".join(lines)


def emit_null_str_list(name: str, vals: list[str]) -> str:
    lines = [f"static const char *const {name}[] = {{"]
    for v in vals:
        lines.append(f"  {cstr(v)},")
    lines.append("  NULL")
    lines.append("};")
    return "\n".join(lines)


def safe_ident(s: str) -> str:
    return "".join(ch if ch.isalnum() else "_" for ch in s)


def emit_offer_list(name: str, vals: list[dict]) -> str:
    lines = [f"static const AetMerchantOffer {name}[] = {{"]
    for row in vals:
        lines.append(f"  {{{cstr(row['item'])}, {int(row['price'])}}},")
    lines.append("  {NULL, 0}")
    lines.append("};")
    return "\n".join(lines)


def emit_npc_tables(npcs: list[dict]) -> str:
    lines: list[str] = []
    for npc in npcs:
        ident = safe_ident(npc["slug"])
        lines.append(emit_null_str_list(f"NPC_{ident}_CHATTER", npc.get("chatter", [])))
        lines.append(f"static const AetNpcTopic NPC_{ident}_TOPICS[] = {{")
        for topic in npc.get("topics", []):
            lines.append(
                f"  {{{cstr(topic['keywords'])}, {cstr(topic['response'])}}},"
            )
        lines.append("  {NULL, NULL}")
        lines.append("};")
    lines.append("static const AetNpcLineSet AET_NPC_LINES_REC[] = {")
    for npc in npcs:
        ident = safe_ident(npc["slug"])
        lines.append(
            f"  {{{cstr(npc['slug'])}, {cstr(npc['greeting'])}, NPC_{ident}_CHATTER, NPC_{ident}_TOPICS}},"
        )
    lines.append("};")
    return "\n".join(lines)


def emit_merchant_tables(merchants: list[dict]) -> str:
    lines: list[str] = []
    for merchant in merchants:
        ident = safe_ident(merchant["slug"])
        lines.append(emit_offer_list(f"MERCHANT_{ident}_STOCK", merchant.get("stock", [])))
        lines.append(emit_offer_list(f"MERCHANT_{ident}_BUYS", merchant.get("buys", [])))
    lines.append("static const AetMerchantTable AET_MERCHANTS_REC[] = {")
    for merchant in merchants:
        ident = safe_ident(merchant["slug"])
        lines.append(
            f"  {{{cstr(merchant['slug'])}, MERCHANT_{ident}_STOCK, MERCHANT_{ident}_BUYS}},"
        )
    lines.append("};")
    return "\n".join(lines)


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    data_path = root / "recovery_artifacts" / "world_tables_recovered.json"
    out_path = root / "aeternitas_world_generated.c"
    d = json.loads(data_path.read_text(encoding="utf-8"))

    slugs = d["slugs"]
    titles = d["titles"]
    blurbs = d["blurbs"]
    regions = d["regions"]
    entities = d["entities"]
    item_lists = d.get("item_lists") or [[] for _ in slugs]
    hidden_item_lists = d.get("hidden_item_lists") or [[] for _ in slugs]
    consume_food_ids = d.get("consume_food_ids") or ["bread", "fresh_bread", "cheese", "fish", "stew"]
    consume_drink_ids = d.get("consume_drink_ids") or ["water", "ale", "tea", "wine"]
    quest_hints = d.get("quest_hints") or [
        "Search the yard and mailbox for useful tools.",
        "Talk to locals near the mill for trade and rumors.",
        "Use route <place> to reach known locations.",
    ]
    npcs = d.get("npc_lines") or []
    merchants = d.get("merchants") or []
    dark = d["dark"]
    exits = d["exits"]

    s = []
    s.append('#include "aeternitas_world_generated.h"')
    s.append("#include <ctype.h>")
    s.append("#include <string.h>")
    s.append("")
    s.append("static const char *const EMPTY_LIST[] = {NULL};")
    s.append(emit_null_str_list("FOOD_IDS", consume_food_ids))
    s.append(emit_null_str_list("DRINK_IDS", consume_drink_ids))
    s.append(emit_null_str_list("QUEST_HINTS", quest_hints))
    s.append("")
    s.append(emit_array_str("WORLD_SLUGS_REC", slugs))
    s.append("")
    s.append(emit_array_str("WORLD_TITLES_REC", titles))
    s.append("")
    s.append(emit_array_str("WORLD_BLURBS_REC", blurbs))
    s.append("")
    s.append(emit_array_str("WORLD_REGIONS_REC", regions))
    s.append("")
    s.append(emit_array_str("WORLD_ENTITIES_REC", entities))
    s.append("")
    s.append(emit_array_u8("WORLD_DARK_REC", dark))
    s.append("")
    s.append(emit_array_exits("WORLD_EXITS_REC", exits))
    s.append("")
    s.append(emit_room_item_lists("WORLD_ITEM_LISTS_REC", item_lists))
    s.append("")
    s.append(emit_room_item_lists("WORLD_HIDDEN_ITEM_LISTS_REC", hidden_item_lists))
    s.append("")
    s.append("static int ieq(const char *a, const char *b) {")
    s.append("  unsigned char ca, cb;")
    s.append("  if (!a || !b) return 0;")
    s.append("  while (*a && *b) { ca=(unsigned char)*a++; cb=(unsigned char)*b++; if (tolower(ca)!=tolower(cb)) return 0; }")
    s.append("  return *a=='\\0' && *b=='\\0';")
    s.append("}")
    s.append("")
    s.append("const char *const *world_consume_food_ids(void) { return FOOD_IDS; }")
    s.append("const char *const *world_consume_drink_ids(void) { return DRINK_IDS; }")
    s.append("const char *const *world_quest_hints(void) { return QUEST_HINTS; }")
    s.append("const char *world_slug(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_SLUGS_REC[room] : \"unknown\"; }")
    s.append("const char *world_title(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_TITLES_REC[room] : \"Unknown\"; }")
    s.append("const char *world_blurb(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_BLURBS_REC[room] : \"\"; }")
    s.append("const char *world_region(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_REGIONS_REC[room] : \"\"; }")
    s.append("int world_room_is_dark(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? (int)WORLD_DARK_REC[room] : 0; }")
    s.append("const char *world_room_entity(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_ENTITIES_REC[room] : \"\"; }")
    s.append("int world_exit(int room, int dir) { int v; if (room<0||room>=WORLD_ROOM_COUNT||dir<0||dir>=DIR_COUNT) return -1; v=WORLD_EXITS_REC[room][dir]; return (v>=0 && v<WORLD_ROOM_COUNT) ? v : -1; }")
    s.append("const char *const *world_item_list(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_ITEM_LISTS_REC[room] : EMPTY_LIST; }")
    s.append("const char *const *world_hidden_item_list(int room) { return (room>=0 && room<WORLD_ROOM_COUNT) ? WORLD_HIDDEN_ITEM_LISTS_REC[room] : EMPTY_LIST; }")
    s.append("int world_room_index(const char *slug) { int i; if (!slug||!slug[0]) return -1; for (i=0;i<WORLD_ROOM_COUNT;i++) if (ieq(WORLD_SLUGS_REC[i], slug)) return i; return -1; }")
    s.append("")
    s.append(emit_npc_tables(npcs))
    s.append(emit_merchant_tables(merchants))
    s.append("const AetNpcLineSet *aet_npc_lines(const char *entity_slug) { int i; if(!entity_slug||!entity_slug[0]) return NULL; for(i=0;i<(int)(sizeof AET_NPC_LINES_REC/sizeof AET_NPC_LINES_REC[0]);i++) if(ieq(AET_NPC_LINES_REC[i].slug,entity_slug)) return &AET_NPC_LINES_REC[i]; return NULL; }")
    s.append("int aet_merchant_count(void) { return (int)(sizeof AET_MERCHANTS_REC/sizeof AET_MERCHANTS_REC[0]); }")
    s.append("int aet_merchant_index(const char *slug) { int i; if(!slug||!slug[0]) return -1; for(i=0;i<aet_merchant_count();i++) if(ieq(AET_MERCHANTS_REC[i].slug,slug)) return i; return -1; }")
    s.append("const char *aet_merchant_slug_at(int idx) { if(idx<0||idx>=aet_merchant_count()) return \"\"; return AET_MERCHANTS_REC[idx].slug; }")
    s.append("const AetMerchantTable *aet_merchant_trades(const char *slug) { int i=aet_merchant_index(slug); if(i<0) return NULL; return &AET_MERCHANTS_REC[i]; }")
    s.append("")

    out_path.write_text("\n".join(s), encoding="utf-8")
    print(f"Wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
