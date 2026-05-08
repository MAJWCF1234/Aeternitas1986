/* Data-driven mods for the stdin C port (no script VM — text overlays in RAM). */
#ifndef AETER_MODS_H
#define AETER_MODS_H

#include <stddef.h>

typedef struct {
  char name[48];
  int req_hrd, req_shp, req_dur, req_bnd, req_grp, req_flx;
} AetCraftArchetype;

/** Derive "<save_dir>/mods" from the quicksave path (same dir as executable default). */
void aet_mods_build_default_path(const char *save_file_path, char *out, size_t outcap);

/**
 * Scan mods_directory for subfolders (packs). Each pack may contain:
 *   manifest.txt         — optional keys: priority=<int>; enabled=0 or
 *                          disabled=1 skips the whole pack; id=/title= notes.
 *   rooms/<slug>.txt     — full room blurb replace (slug must exist in world)
 *   rooms/<slug>.prepend.txt — text prepended to current blurb (after replaces)
 *   rooms/<slug>.append.txt  — text appended to current blurb
 *   titles/<slug>.txt    — one-line display title override
 *   items/<item_id>.txt  — examine / x text (item id as in game, e.g. lockpick)
 *   crafting/profiles.txt — item craft profile overrides:
 *                           item_id|class|is_base|hrd|shp|flx|dur|wgt|grp|bnd|utl
 *   crafting/archetypes.txt — craft result targets:
 *                           name|req_hrd|req_shp|req_dur|req_bnd|req_grp|req_flx
 *   npcs/<entity>.greeting.txt — replaces NPC greeting (may use %NAME% %RACE%
 *                          %CLASS% %ROLE% %PRONOUNS% — from current character)
 *   npcs/<entity>__<keyword>.txt — topic line (same placeholders supported)
 *   character/sheet_append.txt — appended to compact character / sheet brief
 *   character/portrait_append.txt — appended after full portrait (sheet command)
 *   character/aptitudes_append.txt — appended to skills / aptitudes panel
 *   character/reputation_append.txt — appended to reputation overview panel
 *   character/loadout_append.txt — appended to loadout / gear panel
 *   character/traits_append.txt — appended to traits / personality panel
 *   character/momentum_append.txt — appended to momentum / arc panel
 *   character/perks_append.txt — appended to perks roster panel
 *   character/voice_append.txt — appended to voice / pronouns panel
 *   character/bio_append.txt — appended to biography / backstory panel
 *   character/tainting_append.txt — appended to tainting / corruption (COR) panel
 *   character/rapport_append.txt — appended to rapport / social anchors panel
 *   character/objectives_append.txt — appended after objectives / goals panel
 *   character/vitals_append.txt — appended to vitals / wellness panel
 *   character/examine_append.txt — appended after  examine me / x self  compact summary
 *   character/notes_append.txt — merged into notes list body (before panel footer)
 *   character/notes_panel_append.txt — extra --- DLC / mod --- block after any notes
 *                          fullscreen (notes, todo, done, stats, find)
 *   character/hints_append.txt — merged into contextual hints body (inline)
 *   character/hints_panel_append.txt — extra --- DLC / mod --- after hints / unstick / hint
 *                          fullscreen (after the generated hint text)
 *   character/journal_append.txt — appended after journal / quests fullscreen
 *   character/progress_append.txt — appended after progress / visited / seen fullscreen
 *   character/inventory_append.txt — appended after inventory / inv / pack fullscreen
 *   character/waypoints_append.txt — appended after waypoints / nexus / fasttravel list
 *   character/status_append.txt — appended after status / stat fullscreen
 *   character/score_append.txt — appended after score fullscreen
 *   character/time_append.txt — appended after time / clock fullscreen
 *   character/weather_append.txt — appended after weather fullscreen
 *   character/room_append.txt — appended after describe / blurb / room fullscreen
 *   character/recap_append.txt — appended after recap / transcript fullscreen
 *   character/help_append.txt — appended after the main help (?) fullscreen
 *   character/about_append.txt — appended after about / credits fullscreen
 *   character/lights_append.txt — appended after lights / lighting fullscreen
 *   character/exits_append.txt — appended after exits fullscreen
 *   character/scan_append.txt — appended after scan fullscreen
 *   character/trail_append.txt — after movement trail (trail command) fullscreen
 *   character/nearby_append.txt — after nearby / map fullscreen
 *   character/lockcheck_append.txt — after lockcheck fullscreen
 *   character/noise_append.txt — after noise / stealth / suspicion fullscreen
 *   character/nav_append.txt — after where, locate, and find-item fullscreen panels
 *   character/route_append.txt — after route <place> path fullscreen
 *   character/loot_append.txt — after loot fullscreen
 *   character/compare_append.txt — after compare items fullscreen
 *   character/people_append.txt — after who / people here fullscreen
 *   character/diagnostics_append.txt — after errors / diagnostics / diag fullscreen
 *   character/wares_append.txt — after wares / shop / buy / sell price list fullscreen
 *   character/saves_append.txt — after saves / save slots fullscreen
 * Character overlay files may include %NAME% %RACE% %CLASS% %ROLE% %PRONOUNS% — expanded
 * when the panel is shown (same tokens as NPC greetings). Also, when the game has a
 * current room: %ROOMTITLE% %ROOMSLUG% %ROOM% (slug) %REGION%.
 * World clock when expanding: %TIME% %PERIOD% %DAY% %SEASON% %WEATHER% %TEMPC%.
 * Live play state: %COINS% %HP% %MAXHP% %SCORE% %TURNS% %INVCOUNT% %INVMAX%
 * %PACK% (filled/total slots) %READIED% (item id or em dash).
 * World progress: %WORLDROOMS% (locations in build) %VISITED% (rooms seen this run)
 * %NOTECOUNT% (player notes) %EXPLORE% (visited/world as percent 0–100).
 * Character sheet fields: %STR% %AGI% %INT% %WIS% %CHA% %COR% %AGE% %GENDER%
 * %CLASSID% %RACEID% %BUILD% %MUSCLE%.
 * Derived profile fields: %POWER% %RESOLVE% %CUNNING% %PRESENCE% %HPPCT%
 * %RISK% %RISKLABEL% %TEMPER% %ARCHETYPE% %CORTIER% (alias %CORSTATE%).
 * Narrative-tone helpers: %VOICESTYLE% %MORALVECTOR% %THREATPOSTURE%.
 * Scene/state helpers: %SCENETONE% %TRAVELMOOD% %SOCIALSTANCE%.
 * NPC-memory helpers: %NPCHERE% %NPCDISPLAY% %NPCROLE% %NPCDANGER% %NPCTRUST% %NPCLEVERAGE%
 * %NPCPRESENCE% %LASTNPCHERE% %LASTNPCDISPLAY% %LASTNPCROLE% %LASTNPCATTITUDE%
 * %LASTNPCDANGER% %LASTNPCTRUST% %LASTNPCLEVERAGE%.
 * Session context: %LASTNPC% %LASTFOCUS% (last examined pack item) %LASTTOPIC%
 * (last successful talk-about phrase) %LASTTOPICMOOD% (topic category)
 * %TOPICHEAT% (low/medium/high/none) %ROOMMODE% (verbose or brief).
 * Packs load in ascending priority, then folder name; later load wins overlaps.
 */
void aet_mods_init(const char *mods_directory);
void aet_mods_shutdown(void);
void aet_mods_reload(const char *mods_directory);

const char *aet_mods_room_blurb(int room_index);
const char *aet_mods_room_title(int room_index);
/** Copies mod examine text into out; returns 1 if a mod line exists. */
int aet_mods_item_description(const char *item_id, char *out, size_t outcap);
const char *aet_mods_npc_greeting(const char *entity_slug);
const char *aet_mods_npc_topic_response(const char *entity_slug,
                                        const char *topic_phrase_lc);
int aet_mods_crafting_profile(const char *item_id, char *mat_class,
                              size_t mat_class_cap, int *is_base, int *hrd,
                              int *shp, int *flx, int *dur, int *wgt, int *grp,
                              int *bnd, int *utl);
int aet_mods_crafting_archetype_count(void);
int aet_mods_crafting_archetype_get(int idx, AetCraftArchetype *out);
void aet_mods_format_status(char *buf, size_t cap);
/** Pack load order (folder names and priorities); empty if not initialized. */
void aet_mods_format_load_order(char *buf, size_t cap);

/** NULL if no mod text; else pointer to loaded overlay (last pack wins). */
const char *aet_mods_character_sheet_suffix(void);
const char *aet_mods_character_portrait_suffix(void);
const char *aet_mods_character_aptitudes_suffix(void);
const char *aet_mods_character_reputation_suffix(void);
const char *aet_mods_character_loadout_suffix(void);
const char *aet_mods_character_traits_suffix(void);
const char *aet_mods_character_momentum_suffix(void);
const char *aet_mods_character_perks_suffix(void);
const char *aet_mods_character_voice_suffix(void);
const char *aet_mods_character_bio_suffix(void);
const char *aet_mods_character_tainting_suffix(void);
const char *aet_mods_character_rapport_suffix(void);
const char *aet_mods_character_objectives_suffix(void);
const char *aet_mods_character_vitals_suffix(void);
const char *aet_mods_character_examine_suffix(void);
const char *aet_mods_character_notes_suffix(void);
const char *aet_mods_character_hints_suffix(void);
const char *aet_mods_character_hints_panel_suffix(void);
const char *aet_mods_character_notes_panel_suffix(void);
const char *aet_mods_character_journal_suffix(void);
const char *aet_mods_character_progress_suffix(void);
const char *aet_mods_character_inventory_suffix(void);
const char *aet_mods_character_waypoints_suffix(void);
const char *aet_mods_character_status_suffix(void);
const char *aet_mods_character_score_suffix(void);
const char *aet_mods_character_time_suffix(void);
const char *aet_mods_character_weather_suffix(void);
const char *aet_mods_character_room_suffix(void);
const char *aet_mods_character_recap_suffix(void);
const char *aet_mods_character_help_suffix(void);
const char *aet_mods_character_about_suffix(void);
const char *aet_mods_character_lights_suffix(void);
const char *aet_mods_character_exits_suffix(void);
const char *aet_mods_character_scan_suffix(void);
const char *aet_mods_character_trail_suffix(void);
const char *aet_mods_character_nearby_suffix(void);
const char *aet_mods_character_lockcheck_suffix(void);
const char *aet_mods_character_noise_suffix(void);
const char *aet_mods_character_nav_suffix(void);
const char *aet_mods_character_route_suffix(void);
const char *aet_mods_character_loot_suffix(void);
const char *aet_mods_character_compare_suffix(void);
const char *aet_mods_character_people_suffix(void);
const char *aet_mods_character_diagnostics_suffix(void);
const char *aet_mods_character_wares_suffix(void);
const char *aet_mods_character_saves_suffix(void);

#endif
