
#ifndef AETER_MODS_H
#define AETER_MODS_H

#include <stddef.h>

typedef struct {
  char name[48];
  int req_hrd, req_shp, req_dur, req_bnd, req_grp, req_flx;
} AetCraftArchetype;

void aet_mods_build_default_path(const char *save_file_path, char *out, size_t outcap);

void aet_mods_init(const char *mods_directory);
void aet_mods_shutdown(void);
void aet_mods_reload(const char *mods_directory);

const char *aet_mods_room_blurb(int room_index);
const char *aet_mods_room_title(int room_index);

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

void aet_mods_format_load_warnings(char *buf, size_t cap);

void aet_mods_format_load_order(char *buf, size_t cap);

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
