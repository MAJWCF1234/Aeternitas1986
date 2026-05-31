# Lore sources for the C port

When adding or revising in-game text (room blurbs, examine/read strings, NPC topics, `about` / `lore`, quest journal lines), prefer **canon from the original web edition and world bible** over invention.

## Primary paths (on this machine)

| Source | Path |
|--------|------|
| **Web edition + Complete Lore guide** | `D:\light\Veritasfurtum Guides and Docs\AeternitasWebEdition\` |
| **Complete Lore guide (plain text)** | `D:\light\Veritasfurtum Guides and Docs\AeternitasWebEdition\Complete Lore guide.txt` |
| **Story / behavior bible (large)** | `D:\light\Veritasfurtum Guides and Docs\GAME_BRANCHING_TREE.md` |
| **Obsidian world wiki** | `D:\light\Veritasfurtum Guides and Docs\Veritasfurtum World (Obsidian Vault)\` |
| **Named characters index** | `...\90 Reference and Sources\Lore guide — named characters index.md` |
| **Engine world snapshot** | `...\90 Reference and Sources\Aeternitas engine world snapshot.md` |
| **Hollow Ridge region** | `...\40 Locations\Hollow Ridge\Hollow Ridge.md` |

## Canon anchors (quick reference)

- **Veritasfurtum** — Maddeline's universe (canonical name); reality can fray (entropy, dimensional rifts, overlapping fragments from other worlds).
- **The Architect** — Creator who later walked creation as **Empress Maddeline of Amethystus**.
- **Hollow Ridge** — A **region within Veritasfurtum**; the C build compiles ~161 explorable locations here (manor, village, temple, wilds), not the whole multiverse.
- **Universe Drops** — Cosmology: layered realities adrift in the Void; **Nexus Points** are rare crossings.
- **CSA** — Cosmic Sentinel Agency; monitors dimensional/time/multiversal travel (see lore guide for departments).
- **Currency (Amethystus lore)** — Royal gold → copper ladder; the C port uses a single copper-backed purse with gold/silver/bronze/copper display.

## C port scope

The native executable does **not** implement the full web quest graph or all 19+ NPC dialogue trees. Lore text should **flavor** what exists (items, regions, hints) without implying features that are not in `aeternitas64.exe`.

## In-game

- `lore` — setting primer (fullscreen)
- `lore <topic>` — `veritasfurtum`, `architect`, `hollow`, `cosmology`, `csa`, `currency`
- Reading `lore_scroll` / related scrolls — web-aligned Veritasfurtum snippet when read
- NPC dialogue tables in `aeternitas_world_generated.c` (`AetNpcLineSet`) — sync topics/chatter from `GAME_BRANCHING_TREE.md` when expanding speech
- `talk` / `talk about <topic>` — relationship-stage approach lines and time-of-day hooks for miller, forest hermit, general store (see `npc_stage_approach` / `npc_period_talk` in `aeternitas64_ascii.c`)
