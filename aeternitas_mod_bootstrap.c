#include "aeternitas_mod_bootstrap.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/types.h>
#endif

static int mkdir_one(const char *path) {
  if (!path || !path[0]) return 0;
#if defined(_WIN32)
  if (_mkdir(path) == 0) return 1;
#else
  if (mkdir(path, 0755) == 0) return 1;
#endif
  return errno == EEXIST;
}

static int dir_exists(const char *path) {
  struct stat st;
  return path && stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int path_join(char *out, size_t cap, const char *a, const char *b) {
  size_t la = a ? strlen(a) : 0, lb = b ? strlen(b) : 0;
  int need_sep = (la > 0 && a[la - 1] != '/' && a[la - 1] != '\\');
  if (la + lb + (need_sep ? 2 : 1) > cap) return 0;
  memcpy(out, a, la);
  if (need_sep) {
#if defined(_WIN32)
    out[la++] = '\\';
#else
    out[la++] = '/';
#endif
  }
  memcpy(out + la, b, lb + 1);
  return 1;
}

static void ensure_parent_dirs(const char *filepath) {
  char buf[700];
  char *p;
  strncpy(buf, filepath, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  p = strrchr(buf, '/');
  {
    char *b = strrchr(buf, '\\');
    if (b > p) p = b;
  }
  if (!p) return;
  *p = '\0';
  if (!buf[0]) return;
#if defined(_WIN32)
  if (buf[1] == ':')
    p = buf + 2;
  else
    p = buf;
#else
  p = buf;
#endif
  while (*p == '/' || *p == '\\') p++;
  for (; *p; p++) {
    if (*p == '/' || *p == '\\') {
      char sv = *p;
      *p = '\0';
      (void)mkdir_one(buf);
      *p = sv;
    }
  }
  (void)mkdir_one(buf);
}

static int write_text(const char *path, const char *text) {
  FILE *f;
  if (!path || !text) return 0;
  ensure_parent_dirs(path);
  f = fopen(path, "wb");
  if (!f) return 0;
  fputs(text, f);
  fclose(f);
  return 1;
}

static int write_text_if_missing(const char *path, const char *text, int *wrote) {
  FILE *f;
  if (wrote) *wrote = 0;
  if (!path || !text) return 0;
  f = fopen(path, "rb");
  if (f) {
    fclose(f);
    return 1;
  }
  if (!write_text(path, text)) return 0;
  if (wrote) *wrote = 1;
  return 1;
}

#define SAMPLE_MANIFEST                                                  \
  "# Aeternitas64 sample pack (tutorial). Safe to delete.\n"           \
  "priority=-100\n"                                                    \
  "id=aeternitas_sample\n"                                             \
  "title=Built-in tutorial / mod smoke test\n"

#define SAMPLE_README                                                    \
  "000_aeternitas_sample\n"                                            \
  "----------------------\n"                                           \
  "Installed automatically on first run if this folder was missing.\n" \
  "See PACK_GUIDE.txt and in-game:  help modding\n"

#define SAMPLE_PACK_GUIDE                                                \
  "PACK_GUIDE — 000_aeternitas_sample\n"                               \
  "====================================\n"                             \
  "This pack demonstrates data-only mods:\n"                         \
  "  • rooms/west_of_house.txt — full blurb replacement\n"           \
  "  • rooms/west_of_house.append.txt — text appended after blurb\n" \
  "  • titles/west_of_house.txt — title tweak\n"                       \
  "  • items/mailbox.txt, leaflet.txt — examine text\n"                \
  "  • npcs/miller.greeting.txt, miller__modding.txt — NPC lines (optional\n"    \
  "    placeholders %NAME% %RACE% %CLASS% %ROLE% %PRONOUNS% in greetings/topics)\n"    \
  "  • character/sheet_append.txt, portrait_append.txt,\n"             \
  "    aptitudes_append.txt, reputation_append.txt,\n"             \
  "    loadout_append.txt, traits_append.txt,\n"             \
  "    momentum_append.txt, perks_append.txt,\n"             \
  "    voice_append.txt, bio_append.txt,\n"             \
  "    tainting_append.txt, rapport_append.txt,\n"             \
  "    objectives_append.txt, journal_append.txt, progress_append.txt,\n"             \
  "    inventory_append.txt, waypoints_append.txt, status_append.txt,\n"             \
  "    score_append.txt, time_append.txt, weather_append.txt, room_append.txt,\n"             \
  "    recap_append.txt, help_append.txt, about_append.txt, lights_append.txt,\n"             \
  "    exits_append.txt, scan_append.txt, trail_append.txt, nearby_append.txt,\n"             \
  "    lockcheck_append.txt, noise_append.txt, nav_append.txt, route_append.txt,\n"             \
  "    loot_append.txt, compare_append.txt, people_append.txt,\n"             \
  "    diagnostics_append.txt, wares_append.txt, saves_append.txt,\n"             \
  "    vitals_append.txt, examine_append.txt, notes_append.txt, hints_append.txt,\n"             \
  "    notes_panel_append.txt, hints_panel_append.txt — PC overlays\n" \
  "\n"                                                                 \
  "For DLC: copy this tree, rename the folder, raise manifest priority,\n" \
  "and replace slugs/ids with your content. Full reference:\n"       \
  "  help modding   mods list   mods doctor   (in-game)\n"

#define SAMPLE_ROOM_APPEND                                               \
  "— Appended by west_of_house.append.txt (layered blurbs for DLC).\n"

#define SAMPLE_ROOM_WEST                                                 \
  "You stand west of a modest house that seems to have misplaced its\n" \
  "century. The path is honest dirt; the mailbox is stubborn metal.\n" \
  "\n"                                                                 \
  "[000_aeternitas_sample] This blurb comes from your mods folder — a\n" \
  "tutorial pack the game created on first run. Delete mods/"          \
  "000_aeternitas_sample to restore the original description, or\n"  \
  "add a higher-priority DLC folder to override this one.\n"

#define SAMPLE_TITLE_WEST "West of House — sample mod active"

#define SAMPLE_MAILBOX                                                     \
  "[Sample mod] The mailbox is still dented, but now it carries a tiny\n" \
  "sticker: \"Mods live here.\" Proof your DLC path is wired.\n"

#define SAMPLE_LEAFLET                                                   \
  "[Sample mod] The leaflet now mentions arrow-key help: type\n"       \
  "  help modding  after closing the main help screen.\n"

#define SAMPLE_MILLER_GR                                                 \
  "Ah — %NAME%, you've found the sample pack (%ROLE%). Modders use manifests\n" \
  "for load order; try  talk about modding  if the topic file loaded.\n"

#define SAMPLE_MILLER_TOPIC                                              \
  "Every serious DLC drops a folder under mods/ with a manifest.txt.\n" \
  "Higher priority loads later and wins. Ship content, not patches\n"  \
  "to the .exe.\n"

#define SAMPLE_CHAR_SHEET                                                \
  "[Sample] %NAME% %RACEID%/%CLASSID% | POW %POWER% RES %RESOLVE% CUN %CUNNING% PRE %PRESENCE% | %ARCHETYPE%, %TEMPER%, risk %RISK% (%RISKLABEL%), cor %CORTIER%.\n"

#define SAMPLE_CHAR_PORTRAIT                                             \
  "[Sample] Epilogue text, quest flags as flavor, or translator notes —\n" \
  "anything you want after the generated portrait (character / sheet).\n"

#define SAMPLE_CHAR_APTITUDES                                            \
  "[Sample] Extra training notes, guild ranks, or perk reminders for the\n" \
  "aptitudes panel (skills / aptitudes).\n"

#define SAMPLE_CHAR_REPUTATION                                           \
  "[Sample] Faction blurbs or rumor hooks for the reputation screen.\n"

#define SAMPLE_CHAR_LOADOUT                                              \
  "[Sample] Stamina cost fiction, secondary weapons, or heirloom lines for\n" \
  "the loadout panel.\n"

#define SAMPLE_CHAR_TRAITS                                               \
  "[Sample] Signature perks, cult affiliations, or story flags as readable\n" \
  "traits.\n"

#define SAMPLE_CHAR_MOMENTUM                                             \
  "[Sample] Act titles, episode recaps, or DM notes for the momentum screen.\n"

#define SAMPLE_CHAR_PERKS                                                \
  "[Sample] Named perks for DLC (one line each reads well in the roster).\n"

#define SAMPLE_CHAR_VOICE                                                \
  "[Sample] Catchphrases, accent notes, or languages for the voice panel.\n"

#define SAMPLE_CHAR_BIO                                                  \
  "[Sample] Two or three paragraphs of DLC canon — origins, debts, kin.\n"

#define SAMPLE_CHAR_TAINTING                                             \
  "[Sample] Cult ranks, relapse clocks, or resistance fiction for COR.\n"

#define SAMPLE_CHAR_RAPPORT                                              \
  "[Sample] %SOCIALSTANCE% stance with locals — e.g. Miller: stranger -> ally lines.\n"

#define SAMPLE_CHAR_OBJECTIVES                                           \
  "[Sample] Episode beats or chapter goals to show under Objectives.\n"

#define SAMPLE_CHAR_JOURNAL                                              \
  "[Sample] Lore tabs, translator notes, or quest flavor after the journal list.\n"

#define SAMPLE_CHAR_PROGRESS                                             \
  "[Sample] %NAME% — %VISITED%/%WORLDROOMS% rooms (%EXPLORE%%), %TRAVELMOOD% routes, scene %SCENETONE%.\n"

#define SAMPLE_CHAR_INVENTORY                                            \
  "[Sample] %NAME%, pack fiction: curses, quest items, weight notes; social read %NPCPRESENCE% with %NPCDISPLAY% (%NPCROLE/%NPCDANGER%), last contact %LASTNPCDISPLAY% (%LASTNPCROLE%), stance %LASTNPCATTITUDE% (%LASTNPCDANGER%).\n"

#define SAMPLE_CHAR_WAYPOINTS                                            \
  "[Sample] DLC travel rules, network lore, or nexus hazards after Waypoints.\n"

#define SAMPLE_CHAR_STATUS                                               \
  "[Sample] %NAME% at %ROOMTITLE% — %THREATPOSTURE% stance, %MORALVECTOR% vector, %VOICESTYLE% voice.\n"

#define SAMPLE_CHAR_SCORE                                                \
  "[Sample] Episode point hooks or seasonal multipliers after Score.\n"

#define SAMPLE_CHAR_TIME                                                 \
  "[Sample] %TIME% %PERIOD% — festival schedules or DLC clocks after Time.\n"

#define SAMPLE_CHAR_WEATHER                                              \
  "[Sample] %WEATHER% at %TEMPC%C in %REGION% — seasonal DLC after Weather.\n"

#define SAMPLE_CHAR_ROOM                                                 \
  "[Sample] %ROOMTITLE%: purse %PURSE% (%PURSESHORT%), pack %PACK%, %READIED% readied — room panel DLC.\n"

#define SAMPLE_CHAR_RECAP                                                \
  "[Sample] %NAME% — last NPC %LASTNPCDISPLAY% [%LASTNPCROLE%/%LASTNPCATTITUDE%/%LASTNPCDANGER%/%LASTNPCTRUST%/%LASTNPCLEVERAGE%] (%LASTNPCHERE%), here: %NPCDISPLAY% [%NPCROLE%/%NPCDANGER%/%NPCTRUST%/%NPCLEVERAGE%], presence %NPCPRESENCE%, focus %LASTFOCUS%, topic %LASTTOPIC% (%LASTTOPICMOOD%, heat %TOPICHEAT%), blurbs %ROOMMODE%.\n"

#define SAMPLE_CHAR_HELP                                                 \
  "[Sample] DLC command cheatsheet lines after Help — episode controls, mod hotkeys.\n"

#define SAMPLE_CHAR_ABOUT                                                \
  "[Sample] Credits rider: studio name, license, community links after About.\n"

#define SAMPLE_CHAR_LIGHTS                                               \
  "[Sample] %ROOMTITLE% — torch policy, dark-room fiction after Lights.\n"

#define SAMPLE_CHAR_EXITS                                                \
  "[Sample] %REGION% — signpost lore or route warnings after Exits.\n"

#define SAMPLE_CHAR_SCAN                                                 \
  "[Sample] Tactical gloss: hazard tags, stealth hints after Scan.\n"

#define SAMPLE_CHAR_TRAIL                                                \
  "[Sample] DLC breadcrumbs fiction after Trail (back-track, lost-in-fog jokes).\n"

#define SAMPLE_CHAR_NEARBY                                               \
  "[Sample] Region color commentary after Nearby / map (%REGION%, %ROOM%).\n"

#define SAMPLE_CHAR_LOCKCHECK                                            \
  "[Sample] Security briefing after Lockcheck — guild seals, alarm clocks.\n"

#define SAMPLE_CHAR_NOISE                                                \
  "[Sample] Stealth episode notes after Noise / suspicion panel.\n"

#define SAMPLE_CHAR_NAV                                                  \
  "[Sample] Wayfinding DLC after Where / Locate / Find (same hook for all three).\n"

#define SAMPLE_CHAR_ROUTE                                                \
  "[Sample] Travel warnings after Route — bandits on the path, tolls.\n"

#define SAMPLE_CHAR_LOOT                                                 \
  "[Sample] Salvage tables or fence contacts after Loot.\n"

#define SAMPLE_CHAR_COMPARE                                              \
  "[Sample] Appraiser gossip after Compare.\n"

#define SAMPLE_CHAR_PEOPLE                                               \
  "[Sample] Crowd flavor after Who — festival masks, witness lists.\n"

#define SAMPLE_CHAR_DIAGNOSTICS                                          \
  "[Sample] Support footer after Diagnostics — log paths, known issues.\n"

#define SAMPLE_CHAR_WARES                                                \
  "[Sample] %LASTNPC% — seasonal prices, black-market footnotes after Wares.\n"

#define SAMPLE_CHAR_SAVES                                                \
  "[Sample] Cloud-save fiction or slot labels after Saves (no real cloud — flavor).\n"

#define SAMPLE_CHAR_VITALS                                               \
  "[Sample] Wound clocks, poison, fatigue fiction for the vitals panel.\n"

#define SAMPLE_CHAR_EXAMINE                                              \
  "[Sample] Extra lines after  x me / examine self  (compact self-inspection).\n"

#define SAMPLE_CHAR_NOTES                                                \
  "[Sample] DLC reminders after your notes list — trackers, debts, episode beats.\n"

#define SAMPLE_CHAR_HINTS                                                \
  "[Sample] Extra lines after contextual hints — chapter guides, route spoilers OK.\n"

#define SAMPLE_CHAR_HINTS_PANEL                                          \
  "[Sample] Second DLC block after Hints fullscreen (--- DLC / mod --- banner).\n"

#define SAMPLE_CHAR_NOTES_PANEL                                          \
  "[Sample] Second DLC block after Notes / todo / done / stats / find fullscreen.\n"

static int ensure_sample_pack(const char *mods_root, int *repaired_files) {
  char pack[700], sub[700], file[700];
  int wrote = 0;

  if (!mods_root || !mods_root[0]) return 0;
  if (!dir_exists(mods_root)) {
    if (!mkdir_one(mods_root)) return 0;
  }

  if (!path_join(pack, sizeof pack, mods_root, "000_aeternitas_sample")) return 0;
  if (!dir_exists(pack) && !mkdir_one(pack)) return 0;

  if (!path_join(file, sizeof file, pack, "manifest.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_MANIFEST, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;
  if (!path_join(file, sizeof file, pack, "README.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_README, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;
  if (!path_join(file, sizeof file, pack, "PACK_GUIDE.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_PACK_GUIDE, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;

  if (!path_join(sub, sizeof sub, pack, "rooms")) return 0;
  if (!mkdir_one(sub)) return 0;
  if (!path_join(file, sizeof file, sub, "west_of_house.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_ROOM_WEST, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;
  if (!path_join(file, sizeof file, sub, "west_of_house.append.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_ROOM_APPEND, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;

  if (!path_join(sub, sizeof sub, pack, "titles")) return 0;
  if (!mkdir_one(sub)) return 0;
  if (!path_join(file, sizeof file, sub, "west_of_house.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_TITLE_WEST, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;

  if (!path_join(sub, sizeof sub, pack, "items")) return 0;
  if (!mkdir_one(sub)) return 0;
  if (!path_join(file, sizeof file, sub, "mailbox.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_MAILBOX, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;
  if (!path_join(file, sizeof file, sub, "leaflet.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_LEAFLET, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;

  if (!path_join(sub, sizeof sub, pack, "npcs")) return 0;
  if (!mkdir_one(sub)) return 0;
  if (!path_join(file, sizeof file, sub, "miller.greeting.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_MILLER_GR, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;
  if (!path_join(file, sizeof file, sub, "miller__modding.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_MILLER_TOPIC, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;

  if (!path_join(sub, sizeof sub, pack, "character")) return 0;
  if (!mkdir_one(sub)) return 0;
  if (!path_join(file, sizeof file, sub, "sheet_append.txt")) return 0;
  if (!write_text_if_missing(file, SAMPLE_CHAR_SHEET, &wrote)) return 0;
  if (wrote && repaired_files) (*repaired_files)++;
  #define ENSURE_CHAR_FILE(name, text)                                  \
    do {                                                                 \
      if (!path_join(file, sizeof file, sub, name)) return 0;           \
      if (!write_text_if_missing(file, text, &wrote)) return 0;         \
      if (wrote && repaired_files) (*repaired_files)++;                 \
    } while (0)
  ENSURE_CHAR_FILE("portrait_append.txt", SAMPLE_CHAR_PORTRAIT);
  ENSURE_CHAR_FILE("aptitudes_append.txt", SAMPLE_CHAR_APTITUDES);
  ENSURE_CHAR_FILE("reputation_append.txt", SAMPLE_CHAR_REPUTATION);
  ENSURE_CHAR_FILE("loadout_append.txt", SAMPLE_CHAR_LOADOUT);
  ENSURE_CHAR_FILE("traits_append.txt", SAMPLE_CHAR_TRAITS);
  ENSURE_CHAR_FILE("momentum_append.txt", SAMPLE_CHAR_MOMENTUM);
  ENSURE_CHAR_FILE("perks_append.txt", SAMPLE_CHAR_PERKS);
  ENSURE_CHAR_FILE("voice_append.txt", SAMPLE_CHAR_VOICE);
  ENSURE_CHAR_FILE("bio_append.txt", SAMPLE_CHAR_BIO);
  ENSURE_CHAR_FILE("tainting_append.txt", SAMPLE_CHAR_TAINTING);
  ENSURE_CHAR_FILE("rapport_append.txt", SAMPLE_CHAR_RAPPORT);
  ENSURE_CHAR_FILE("objectives_append.txt", SAMPLE_CHAR_OBJECTIVES);
  ENSURE_CHAR_FILE("journal_append.txt", SAMPLE_CHAR_JOURNAL);
  ENSURE_CHAR_FILE("progress_append.txt", SAMPLE_CHAR_PROGRESS);
  ENSURE_CHAR_FILE("inventory_append.txt", SAMPLE_CHAR_INVENTORY);
  ENSURE_CHAR_FILE("waypoints_append.txt", SAMPLE_CHAR_WAYPOINTS);
  ENSURE_CHAR_FILE("status_append.txt", SAMPLE_CHAR_STATUS);
  ENSURE_CHAR_FILE("score_append.txt", SAMPLE_CHAR_SCORE);
  ENSURE_CHAR_FILE("time_append.txt", SAMPLE_CHAR_TIME);
  ENSURE_CHAR_FILE("weather_append.txt", SAMPLE_CHAR_WEATHER);
  ENSURE_CHAR_FILE("room_append.txt", SAMPLE_CHAR_ROOM);
  ENSURE_CHAR_FILE("recap_append.txt", SAMPLE_CHAR_RECAP);
  ENSURE_CHAR_FILE("help_append.txt", SAMPLE_CHAR_HELP);
  ENSURE_CHAR_FILE("about_append.txt", SAMPLE_CHAR_ABOUT);
  ENSURE_CHAR_FILE("lights_append.txt", SAMPLE_CHAR_LIGHTS);
  ENSURE_CHAR_FILE("exits_append.txt", SAMPLE_CHAR_EXITS);
  ENSURE_CHAR_FILE("scan_append.txt", SAMPLE_CHAR_SCAN);
  ENSURE_CHAR_FILE("trail_append.txt", SAMPLE_CHAR_TRAIL);
  ENSURE_CHAR_FILE("nearby_append.txt", SAMPLE_CHAR_NEARBY);
  ENSURE_CHAR_FILE("lockcheck_append.txt", SAMPLE_CHAR_LOCKCHECK);
  ENSURE_CHAR_FILE("noise_append.txt", SAMPLE_CHAR_NOISE);
  ENSURE_CHAR_FILE("nav_append.txt", SAMPLE_CHAR_NAV);
  ENSURE_CHAR_FILE("route_append.txt", SAMPLE_CHAR_ROUTE);
  ENSURE_CHAR_FILE("loot_append.txt", SAMPLE_CHAR_LOOT);
  ENSURE_CHAR_FILE("compare_append.txt", SAMPLE_CHAR_COMPARE);
  ENSURE_CHAR_FILE("people_append.txt", SAMPLE_CHAR_PEOPLE);
  ENSURE_CHAR_FILE("diagnostics_append.txt", SAMPLE_CHAR_DIAGNOSTICS);
  ENSURE_CHAR_FILE("wares_append.txt", SAMPLE_CHAR_WARES);
  ENSURE_CHAR_FILE("saves_append.txt", SAMPLE_CHAR_SAVES);
  ENSURE_CHAR_FILE("vitals_append.txt", SAMPLE_CHAR_VITALS);
  ENSURE_CHAR_FILE("examine_append.txt", SAMPLE_CHAR_EXAMINE);
  ENSURE_CHAR_FILE("notes_append.txt", SAMPLE_CHAR_NOTES);
  ENSURE_CHAR_FILE("hints_append.txt", SAMPLE_CHAR_HINTS);
  ENSURE_CHAR_FILE("hints_panel_append.txt", SAMPLE_CHAR_HINTS_PANEL);
  ENSURE_CHAR_FILE("notes_panel_append.txt", SAMPLE_CHAR_NOTES_PANEL);
  #undef ENSURE_CHAR_FILE
  return 1;
}

void aet_mod_bootstrap_sample_pack(const char *mods_root) {
  (void)ensure_sample_pack(mods_root, NULL);
}

void aet_mod_bootstrap_prepare_runtime(const char *save_file_path,
                                       const char *mods_root,
                                       AetRuntimeBootstrapStatus *out_status) {
  AetRuntimeBootstrapStatus st;
  char marker[700];
  int wrote = 0;
  memset(&st, 0, sizeof st);
  if (save_file_path && save_file_path[0]) {
    ensure_parent_dirs(save_file_path);
    st.save_parent_ready = 1;
  }
  if (mods_root && mods_root[0]) {
    if (dir_exists(mods_root) || mkdir_one(mods_root)) {
      st.mods_root_ready = 1;
      if (path_join(marker, sizeof marker, mods_root, ".aet_bootstrap_v1.marker")) {
        if (write_text_if_missing(
                marker,
                "Aeternitas64 runtime bootstrap marker v1.\n"
                "Safe to delete; it will be recreated automatically.\n",
                &wrote)) {
          st.marker_present = 1;
        }
      }
      if (ensure_sample_pack(mods_root, &st.repaired_files))
        st.sample_pack_present = 1;
    }
  }
  if (out_status) *out_status = st;
}
