#include "aeternitas_char_creation.h"
#include "aeternitas_char_description.h"
#include "aeternitas_mod_bootstrap.h"
#include "aeternitas_mod_guide.h"
#include "aeternitas_mods.h"
#include "aeternitas_world_generated.h"
#include "aeternitas_item_catalog.h"
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <windows.h>
#include <conio.h>
#if defined(AETER_WIN_PICKERS)
#include <prsht.h>
#include <shlobj.h>
#include <commdlg.h>
#endif
#else
#include <errno.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif
#if defined(_WIN32)
#include <io.h>
#endif

#ifdef AETER_MINIGAMES
#include "tools/testing/mgt_game_bridge.h"
#include "tools/testing/mgt_game_sim.h"
#include "tools/testing/mgt_host.h"
#include "tools/testing/mgt_read.h"
#include "tools/testing/mgt_state.h"
#include "tools/testing/mgt_sync.h"
#include "mgt.h"
#endif

static const char *base_world_room_entity(int room) { return world_room_entity(room); }
static const char *runtime_room_entity(int room);
#define world_room_entity runtime_room_entity

static int aet_autotest(void) {
  const char *e = getenv("AETER_AUTOTEST");
  return e != NULL && e[0] != '\0' && strcmp(e, "0") != 0;
}

enum {
  DIR_N = 0,
  DIR_S = 1,
  DIR_E = 2,
  DIR_W = 3,
  DIR_U = 4,
  DIR_D = 5,
  DIR_NE = 6,
  DIR_NW = 7,
  DIR_SE = 8,
  DIR_SW = 9,
  DIR_IN = 10,
  DIR_OUT = 11,
  DIR_DEEPER = 12,
  DIR_UPSTREAM = 13,
  DIR_DOWNSTREAM = 14,
  DIR_FOUNTAIN = 15,
  DIR_STAGE = 16,
  DIR_BOARD = 17,
  DIR_SQUARE = 18
};

_Static_assert(18 + 1 == DIR_COUNT, "sync enum with aeternitas_world_generated.h");

static const char *const DIR_LABELS[DIR_COUNT] = {
    "north",     "south",     "east",      "west",      "up",        "down",
    "northeast", "northwest", "southeast", "southwest", "in",        "out",
    "deeper",    "upstream",  "downstream", "fountain", "stage",
    "board",     "square"};

#define MAX_WORLD_ROOMS 256
#define MAX_ITEMS_ROOM 64
#define MAX_ITEM_LEN 48
#define MAX_INV 128
#define MAX_NOTES 32
#define NOTE_LEN 192
#define INPUT_LINE_MAX 512

#define AETER_HELP_BODY_CAP 8192
#define TRANSCRIPT_CAP 4096
#define PROCESS_MSG_CAP 1536
#define SAVE_BASENAME "aeternitas64_save.txt"
#define SAVE_SLOT_COUNT 10
#define AETER_START_HP 100

static char g_save_path[520];

static char g_mods_override[520];
static AetRuntimeBootstrapStatus g_bootstrap_status;
typedef struct {
  int quiet;
  int loud;
  int friendly;
  int harsh;
  int careful;
  int present;
} AetIntentCtx;
static AetIntentCtx g_intent;
static AetIntentCtx g_last_intent;
static char g_transcript[TRANSCRIPT_CAP];
static int g_return_to_menu;

enum {
  DISAMBIG_ACT_NONE = 0,
  DISAMBIG_ACT_TAKE,
  DISAMBIG_ACT_DROP,
  DISAMBIG_ACT_EXAMINE,
  DISAMBIG_ACT_EQUIP,
  DISAMBIG_ACT_BUY,
  DISAMBIG_ACT_SELL
};
#define DISAMBIG_PICK_MAX 10
#define DISAMBIG_MEM_MAX 24
static int g_disambig_next_act;
static int g_disambig_act;
static int g_disambig_inv;
static char g_disambig_query[MAX_ITEM_LEN];
static char g_disambig_picks[DISAMBIG_PICK_MAX][MAX_ITEM_LEN];
static int g_disambig_pick_n;
typedef struct {
  char q[MAX_ITEM_LEN];
  char slug[MAX_ITEM_LEN];
} DisambigMemEnt;
static DisambigMemEnt g_disambig_mem[DISAMBIG_MEM_MAX];
static int g_disambig_mem_n;

static const char *UI_RULE =
    "===============================================================================";
static const char *UI_RULE_LIGHT =
    "--------------------------------------------------------------------------------";

static const char AETER_RULE_120[] =
    "==============================================================================="
    "=========================================";
static int g_use_color;
static const char *C_RESET = "";
static const char *C_BORDER = "";
static const char *C_TITLE = "";
static const char *C_REGION = "";
static const char *C_HEADING = "";
static const char *C_EXIT = "";
static const char *C_ITEM = "";
static const char *C_STATUS = "";
static const char *C_PROMPT = "";
static const char *C_BOOT_OK = "";
static const char *C_BOOT_HI = "";
static const char *C_MUTED = "";

static const char AETER_MENU_RULE[] =
    "==========================================================================================";
#define AETER_MAIN_MENU_VER "[ v1.3.8 ]"

static void init_save_path(const char *argv0) {
  const char *slash;
  size_t dirlen;
  size_t baselen = strlen(SAVE_BASENAME);
  if (!argv0 || !argv0[0]) {
    memcpy(g_save_path, SAVE_BASENAME, baselen + 1);
    return;
  }
  slash = strrchr(argv0, '\\');
  if (!slash) slash = strrchr(argv0, '/');
  if (!slash) {
    memcpy(g_save_path, SAVE_BASENAME, baselen + 1);
    return;
  }
  dirlen = (size_t)(slash - argv0 + 1);
  if (dirlen + baselen + 1 > sizeof g_save_path) {
    memcpy(g_save_path, SAVE_BASENAME, baselen + 1);
    return;
  }
  memcpy(g_save_path, argv0, dirlen);
  memcpy(g_save_path + dirlen, SAVE_BASENAME, baselen + 1);
}

#if defined(_WIN32) && defined(AETER_WIN_PICKERS)
static int win32_browse_mods_directory(char *out, size_t cap) {
  BROWSEINFOA bi;
  PIDLIST_ABSOLUTE pidl;
  char display[MAX_PATH];
  if (!out || cap < 4) return 0;
  out[0] = '\0';
  memset(&bi, 0, sizeof bi);
  bi.hwndOwner = GetForegroundWindow();
  bi.lpszTitle =
      "Select folder that contains mod packs (each pack is a subfolder).";
  bi.ulFlags = BIF_RETURNONLYFSDIRS;
  pidl = SHBrowseForFolderA(&bi);
  if (!pidl) return 0;
  if (!SHGetPathFromIDListA(pidl, display)) {
    CoTaskMemFree(pidl);
    return 0;
  }
  CoTaskMemFree(pidl);
  strncpy(out, display, cap - 1);
  out[cap - 1] = '\0';
  return 1;
}

static int win32_browse_save_file(char *out, size_t cap) {
  OPENFILENAMEA ofn;
  char filter[] = "Text / save\0*.txt\0All files\0*.*\0\0";
  char path[MAX_PATH] = "aeternitas64_save.txt";
  if (!out || cap < 4) return 0;
  memset(&ofn, 0, sizeof ofn);
  ofn.lStructSize = sizeof ofn;
  ofn.hwndOwner = GetForegroundWindow();
  ofn.lpstrFilter = filter;
  ofn.lpstrFile = path;
  ofn.nMaxFile = (DWORD)sizeof path;
  ofn.lpstrTitle = "Choose quicksave file (use a writable drive)";
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
  if (!GetSaveFileNameA(&ofn)) return 0;
  strncpy(out, path, cap - 1);
  out[cap - 1] = '\0';
  return 1;
}
#endif

static void mods_resolve_root(char *out, size_t cap) {
  const char *e;
  size_t n;
  if (!out || cap < 2) return;
  if (g_mods_override[0]) {
    n = strnlen(g_mods_override, cap - 1);
    memcpy(out, g_mods_override, n);
    out[n] = '\0';
    return;
  }
  e = getenv("AETER_MODS");
  if (e && e[0]) {
    n = strnlen(e, cap - 1);
    memcpy(out, e, n);
    out[n] = '\0';
    return;
  }
  aet_mods_build_default_path(g_save_path, out, cap);
}

static void mods_init_from_env(void) {
  char mp[520];
  mods_resolve_root(mp, sizeof mp);
  aet_mod_bootstrap_prepare_runtime(g_save_path, mp, &g_bootstrap_status);
  aet_mods_init(mp);
}

static void mods_reload_same_rules(void) {
  char mp[520];
  mods_resolve_root(mp, sizeof mp);
  aet_mod_bootstrap_prepare_runtime(g_save_path, mp, &g_bootstrap_status);
  aet_mods_reload(mp);
}

static void format_bootstrap_status(char *out, size_t cap, const char *mods_root) {
  snprintf(out, cap,
           "Runtime bootstrap\n\n"
           "  save parent ready: %s\n"
           "  mods root ready:   %s\n"
           "  sample pack ready: %s\n"
           "  marker present:    %s\n"
           "  repaired files:    %d\n\n"
           "Active mods root:\n  %s\n",
           g_bootstrap_status.save_parent_ready ? "yes" : "no",
           g_bootstrap_status.mods_root_ready ? "yes" : "no",
           g_bootstrap_status.sample_pack_present ? "yes" : "no",
           g_bootstrap_status.marker_present ? "yes" : "no",
           g_bootstrap_status.repaired_files,
           (mods_root && mods_root[0]) ? mods_root : "(none)");
}

static const char *resolve_world_blurb(int r) {
  const char *o = aet_mods_room_blurb(r);
  if (o && o[0]) return o;
  return world_blurb(r);
}

static const char *resolve_world_title(int r) {
  const char *o = aet_mods_room_title(r);
  if (o && o[0]) return o;
  return world_title(r);
}

static char g_room_items[MAX_WORLD_ROOMS][MAX_ITEMS_ROOM][MAX_ITEM_LEN];
static int g_room_item_n[MAX_WORLD_ROOMS];
static char g_hidden_items[MAX_WORLD_ROOMS][MAX_ITEMS_ROOM][MAX_ITEM_LEN];
static int g_hidden_n[MAX_WORLD_ROOMS];
static char g_inv[MAX_INV][MAX_ITEM_LEN];
static int g_inv_n;
static int g_room;
static int g_turns;
static int g_score;
static int g_coins;
static int g_health;
static int g_max_health;
static int g_front_unlocked;
static int g_shed_unlocked;

#define BACK_HIST 32
static int g_hist[BACK_HIST];
static int g_hist_n;
static unsigned char g_visited[MAX_WORLD_ROOMS];
static char g_last_focus[MAX_ITEM_LEN];
static char g_last_dropped[MAX_ITEM_LEN];
static char g_last_npc[MAX_ITEM_LEN];

#define AET_CONTAINER_MAX 48
#define AET_CONTAINER_SLOTS 6
#define AET_CONTAINER_ROOM_INV (-1)
#define AET_CONTAINER_NESTED_BASE (-1000)

typedef struct {
  int room;
  char id[MAX_ITEM_LEN];
  unsigned char open;
  unsigned char locked;
  char contents[AET_CONTAINER_SLOTS][MAX_ITEM_LEN];
  int content_n;
} AetContainerRec;

static AetContainerRec g_containers[AET_CONTAINER_MAX];
static int g_container_n;
#define AETER_LAST_TOPIC_CAP 256
static char g_last_topic[AETER_LAST_TOPIC_CAP];
static char g_ready_item[MAX_ITEM_LEN];
#define EQ_SLOT_COUNT 8
enum {
  EQ_HEAD = 0,
  EQ_CHEST,
  EQ_HANDS,
  EQ_LEGS,
  EQ_FEET,
  EQ_WEAPON,
  EQ_OFFHAND,
  EQ_ACCESSORY
};
static const char *const EQ_SLOT_NAME[EQ_SLOT_COUNT] = {
    "head", "chest", "hands", "legs",
    "feet", "weapon", "offhand", "accessory"};
static char g_eq_slots[EQ_SLOT_COUNT][MAX_ITEM_LEN];
static char g_last_cmd[INPUT_LINE_MAX];
#define CMD_HIST_RING 32
static char g_cmd_hist[CMD_HIST_RING][INPUT_LINE_MAX];
static int g_cmd_hist_count;
static int g_cmd_hist_head;
#define DIAG_RING 8
#define DIAG_W 168
static char g_diag_ring[DIAG_RING][DIAG_W];
static int g_diag_head;
static int g_diag_count;
static char g_notes[MAX_NOTES][NOTE_LEN];
static int g_note_n;
#define AETER_REP_MAX 32

#define AETER_SOC_NPC_MAX 32
static int g_merchant_rep[AETER_REP_MAX];

static unsigned char g_npc_friendship[AETER_REP_MAX];
static unsigned char g_npc_romance[AETER_REP_MAX];

static int g_npc_last_social_turn[AETER_REP_MAX];
static char g_soc_npc_slug[AETER_SOC_NPC_MAX][MAX_ITEM_LEN];
static unsigned char g_soc_npc_friendship[AETER_SOC_NPC_MAX];
static unsigned char g_soc_npc_romance[AETER_SOC_NPC_MAX];
static int g_soc_npc_last_turn[AETER_SOC_NPC_MAX];

#define CONV_MENU_MAX 6
#define CONV_TOPIC_MEM_SZ 56
enum {
  CONV_PICK_TOPIC = 0,
  CONV_PICK_MORE = 1,
  CONV_PICK_GOODBYE = 2
};
static int g_conv_active;
static char g_conv_npc[MAX_ITEM_LEN];
static int g_conv_pick_n;
static int g_conv_pick_kind[CONV_MENU_MAX];
static char g_conv_pick_label[CONV_MENU_MAX][72];
static char g_conv_pick_phrase[CONV_MENU_MAX][128];
static char g_npc_topic_mem[AETER_REP_MAX][CONV_TOPIC_MEM_SZ];
static char g_npc_topic_last[AETER_REP_MAX][CONV_TOPIC_MEM_SZ];
static char g_soc_topic_mem[AETER_SOC_NPC_MAX][CONV_TOPIC_MEM_SZ];
static char g_soc_topic_last[AETER_SOC_NPC_MAX][CONV_TOPIC_MEM_SZ];

static int g_verbose_room;
static int g_npc_validation_checked;
static int g_npc_validation_warnings;

static int g_settings_color_ov = -1;
static int g_hints_pref = 1;
static int g_autosave_enabled = 1;
static int g_ironman_stub = 0;
#define RECAP_MAX 16
#define RECAP_W 320
static char g_recap[RECAP_MAX][RECAP_W];
static int g_recap_n;
static int g_craft_proficiency = 1;
#define MAX_CRAFT_PROFILES 64
typedef struct {
  char name[MAX_ITEM_LEN];
  int hrd, shp, flx, dur, wgt, grp, bnd, utl;
  int quality;
  int disp_dur, disp_shp, disp_hnd, disp_wgt;
} CraftSavedProfile;
static int g_craft_prof_n;
static CraftSavedProfile g_craft_prof[MAX_CRAFT_PROFILES];
static CraftSavedProfile *craft_profile_lookup(const char *item);
static int g_lockpick_last_noise;
static int g_lockpick_suspicion;
#define CAUSAL_RING 32
#define CAUSAL_W 224
static char g_causal_ring[CAUSAL_RING][CAUSAL_W];
static int g_causal_head;
static int g_causal_count;

#define TRADE_RING 32
#define TRADE_W 320
static char g_trade_ring[TRADE_RING][TRADE_W];
static int g_trade_head;
static int g_trade_count;

#define REL_HIST_RING 32
#define REL_HIST_ACT 20
typedef struct {
  int turn;
  char slug[MAX_ITEM_LEN];
  char act[REL_HIST_ACT];
  signed char df;
  signed char dr;
  signed char dp;
} RelHistRow;
static RelHistRow g_rel_hist[REL_HIST_RING];
static int g_rel_hist_head;
static int g_rel_hist_count;

enum {
  BARTER_NONE = 0,
  BARTER_BUY = 1,
  BARTER_SELL = 2
};
static int g_barter_mode;
static int g_barter_price;
static int g_barter_list_price;
static int g_barter_expire_turn;
static char g_barter_merchant[MAX_ITEM_LEN];
static char g_barter_item[MAX_ITEM_LEN];

typedef struct {
  const char *slug;
  const char *morning_room;
  const char *afternoon_room;
  const char *evening_room;
  const char *night_room;
} AetNpcRoutine;

static int npc_routine_count(void);
static const AetNpcRoutine *npc_routine_at(int idx);

typedef struct {
  int verbose_room;
  int recap_n;
  char recap[RECAP_MAX][RECAP_W];
  char last_focus[MAX_ITEM_LEN];
  char last_npc[MAX_ITEM_LEN];
  char last_topic[AETER_LAST_TOPIC_CAP];
  char ready_item[MAX_ITEM_LEN];
  int room, turns, score, coins;
  int health, max_health;
  int front_unlocked, shed_unlocked;
  int hist_n;
  int hist[BACK_HIST];
  unsigned char visited[MAX_WORLD_ROOMS];
  int inv_n;
  char inv[MAX_INV][MAX_ITEM_LEN];
  int room_item_n[MAX_WORLD_ROOMS];
  char room_items[MAX_WORLD_ROOMS][MAX_ITEMS_ROOM][MAX_ITEM_LEN];
  int hidden_n[MAX_WORLD_ROOMS];
  char hidden_items[MAX_WORLD_ROOMS][MAX_ITEMS_ROOM][MAX_ITEM_LEN];
  int note_n;
  char notes[MAX_NOTES][NOTE_LEN];
  int merchant_rep[AETER_REP_MAX];
  unsigned char npc_friendship[AETER_REP_MAX];
  unsigned char npc_romance[AETER_REP_MAX];
  int npc_last_social_turn[AETER_REP_MAX];
  char soc_npc_slug[AETER_SOC_NPC_MAX][MAX_ITEM_LEN];
  unsigned char soc_npc_friendship[AETER_SOC_NPC_MAX];
  unsigned char soc_npc_romance[AETER_SOC_NPC_MAX];
  int soc_npc_last_turn[AETER_SOC_NPC_MAX];
  int trade_head;
  int trade_count;
  char trade_ring[TRADE_RING][TRADE_W];
  int barter_mode;
  int barter_price;
  int barter_list_price;
  int barter_expire_turn;
  char barter_merchant[MAX_ITEM_LEN];
  char barter_item[MAX_ITEM_LEN];
  int craft_proficiency;
  int hints_pref;
  int settings_color_ov;
  int autosave_enabled;
  char eq_slots[EQ_SLOT_COUNT][MAX_ITEM_LEN];
  AetPcSave pc;
} AetGameSnapshot;

static AetGameSnapshot g_load_rollback;

static void snapshot_capture(AetGameSnapshot *dst) {
  dst->verbose_room = g_verbose_room;
  dst->recap_n = g_recap_n;
  memcpy(dst->recap, g_recap, sizeof g_recap);
  memcpy(dst->last_focus, g_last_focus, sizeof g_last_focus);
  memcpy(dst->last_npc, g_last_npc, sizeof g_last_npc);
  memcpy(dst->last_topic, g_last_topic, sizeof g_last_topic);
  memcpy(dst->ready_item, g_ready_item, sizeof g_ready_item);
  dst->room = g_room;
  dst->turns = g_turns;
  dst->score = g_score;
  dst->coins = g_coins;
  dst->health = g_health;
  dst->max_health = g_max_health;
  dst->front_unlocked = g_front_unlocked;
  dst->shed_unlocked = g_shed_unlocked;
  dst->hist_n = g_hist_n;
  memcpy(dst->hist, g_hist, sizeof g_hist);
  memcpy(dst->visited, g_visited, sizeof g_visited);
  dst->inv_n = g_inv_n;
  memcpy(dst->inv, g_inv, sizeof g_inv);
  memcpy(dst->room_item_n, g_room_item_n, sizeof g_room_item_n);
  memcpy(dst->room_items, g_room_items, sizeof g_room_items);
  memcpy(dst->hidden_n, g_hidden_n, sizeof g_hidden_n);
  memcpy(dst->hidden_items, g_hidden_items, sizeof g_hidden_items);
  dst->note_n = g_note_n;
  memcpy(dst->notes, g_notes, sizeof g_notes);
  memcpy(dst->merchant_rep, g_merchant_rep, sizeof g_merchant_rep);
  memcpy(dst->npc_friendship, g_npc_friendship, sizeof g_npc_friendship);
  memcpy(dst->npc_romance, g_npc_romance, sizeof g_npc_romance);
  memcpy(dst->npc_last_social_turn, g_npc_last_social_turn,
         sizeof g_npc_last_social_turn);
  memcpy(dst->soc_npc_slug, g_soc_npc_slug, sizeof g_soc_npc_slug);
  memcpy(dst->soc_npc_friendship, g_soc_npc_friendship,
         sizeof g_soc_npc_friendship);
  memcpy(dst->soc_npc_romance, g_soc_npc_romance, sizeof g_soc_npc_romance);
  memcpy(dst->soc_npc_last_turn, g_soc_npc_last_turn,
         sizeof g_soc_npc_last_turn);
  dst->trade_head = g_trade_head;
  dst->trade_count = g_trade_count;
  memcpy(dst->trade_ring, g_trade_ring, sizeof g_trade_ring);
  dst->barter_mode = g_barter_mode;
  dst->barter_price = g_barter_price;
  dst->barter_list_price = g_barter_list_price;
  dst->barter_expire_turn = g_barter_expire_turn;
  memcpy(dst->barter_merchant, g_barter_merchant, sizeof g_barter_merchant);
  memcpy(dst->barter_item, g_barter_item, sizeof g_barter_item);
  dst->craft_proficiency = g_craft_proficiency;
  dst->hints_pref = g_hints_pref;
  dst->settings_color_ov = g_settings_color_ov;
  dst->autosave_enabled = g_autosave_enabled;
  memcpy(dst->eq_slots, g_eq_slots, sizeof g_eq_slots);
  pc_capture(&dst->pc);
}

static void snapshot_restore(const AetGameSnapshot *src) {
  g_verbose_room = src->verbose_room;
  g_recap_n = src->recap_n;
  memcpy(g_recap, src->recap, sizeof g_recap);
  memcpy(g_last_focus, src->last_focus, sizeof g_last_focus);
  memcpy(g_last_npc, src->last_npc, sizeof g_last_npc);
  memcpy(g_last_topic, src->last_topic, sizeof g_last_topic);
  memcpy(g_ready_item, src->ready_item, sizeof g_ready_item);
  g_room = src->room;
  g_turns = src->turns;
  g_score = src->score;
  g_coins = src->coins;
  g_health = src->health;
  g_max_health = src->max_health;
  g_front_unlocked = src->front_unlocked;
  g_shed_unlocked = src->shed_unlocked;
  g_hist_n = src->hist_n;
  memcpy(g_hist, src->hist, sizeof g_hist);
  memcpy(g_visited, src->visited, sizeof g_visited);
  g_inv_n = src->inv_n;
  memcpy(g_inv, src->inv, sizeof g_inv);
  memcpy(g_room_item_n, src->room_item_n, sizeof g_room_item_n);
  memcpy(g_room_items, src->room_items, sizeof g_room_items);
  memcpy(g_hidden_n, src->hidden_n, sizeof g_hidden_n);
  memcpy(g_hidden_items, src->hidden_items, sizeof g_hidden_items);
  g_note_n = src->note_n;
  memcpy(g_notes, src->notes, sizeof g_notes);
  memcpy(g_merchant_rep, src->merchant_rep, sizeof g_merchant_rep);
  memcpy(g_npc_friendship, src->npc_friendship, sizeof g_npc_friendship);
  memcpy(g_npc_romance, src->npc_romance, sizeof g_npc_romance);
  memcpy(g_npc_last_social_turn, src->npc_last_social_turn,
         sizeof g_npc_last_social_turn);
  memcpy(g_soc_npc_slug, src->soc_npc_slug, sizeof g_soc_npc_slug);
  memcpy(g_soc_npc_friendship, src->soc_npc_friendship,
         sizeof g_soc_npc_friendship);
  memcpy(g_soc_npc_romance, src->soc_npc_romance, sizeof g_soc_npc_romance);
  memcpy(g_soc_npc_last_turn, src->soc_npc_last_turn,
         sizeof g_soc_npc_last_turn);
  g_trade_head = src->trade_head;
  g_trade_count = src->trade_count;
  memcpy(g_trade_ring, src->trade_ring, sizeof g_trade_ring);
  g_barter_mode = src->barter_mode;
  g_barter_price = src->barter_price;
  g_barter_list_price = src->barter_list_price;
  g_barter_expire_turn = src->barter_expire_turn;
  memcpy(g_barter_merchant, src->barter_merchant, sizeof g_barter_merchant);
  memcpy(g_barter_item, src->barter_item, sizeof g_barter_item);
  g_craft_proficiency = src->craft_proficiency;
  g_hints_pref = src->hints_pref;
  g_settings_color_ov = src->settings_color_ov;
  g_autosave_enabled = src->autosave_enabled ? 1 : 0;
  memcpy(g_eq_slots, src->eq_slots, sizeof g_eq_slots);
  pc_restore(&src->pc);
}

static int count_visited(void);
static int count_waypoints(int only_visited);
static void format_objectives_body(char *body, size_t cap);
static void format_progress_body(char *body, size_t cap);
static void format_waypoints_body(char *body, size_t cap);
static void format_trade_history_body(char *body, size_t cap);
static int write_save_file_path(const char *path);
static void cmd_trade_buy(const char *rest, char *msg, size_t msgcap);
static void cmd_trade_sell(const char *rest, char *msg, size_t msgcap);
static void item_pretty(const char *item, char *out, size_t cap);
static void entity_pretty(const char *ent, char *out, size_t cap);
static int str_ieq(const char *a, const char *b);
static void strip_trailing_space(char *s);
static void query_norm_underscore(char *dst, size_t cap, const char *src);
static int cmd_waypoint_travel(const char *raw, char *msg, size_t msgcap);
static void run_game_menu(void);
static void run_settings_ui(void);
static void return_to_game_screen(void);
static void ui_block_pause(const char *title, const char *body);
static int guide_line_count(const char *s);
static void ui_scrollable_panel(const char *title, const char *body,
                              const char *pending_acc, int *did_fullscreen);
static void cmd_play_piano(char *msg, size_t msgcap);
static int try_minigame(const char *id, char *msg, size_t msgcap);
static int has_lockpick_tool(void);
static int resolve_inv_item_query(const char *name, int *ix_out, char *msg,
                                  size_t msgcap);
static void game_mgt_apply_lockpick_stealth(MgtPersistentState *st, char *msg,
                                            size_t msgcap);
static int room_can_fish(void);
static int room_can_farm(void);
static int room_can_cook(void);
static int room_can_gamble(void);
static int run_save_manager_ui(int *did_fullscreen, int esc_menu);
static void run_material_forge(const char *pending_acc, int *did_fullscreen);
static void run_equipment_inventory_ui(const char *pending_acc, int *did_fullscreen);
static int inv_find(const char *name);
static int soc_friendship_for_slug(const char *slug);
static int inv_take_out(int ix, char *taken, size_t taken_cap);
static void eq_clear_all(void);
static void eq_sync_ready_item(void);
static void eq_sync_pc_sheet(void);
static void eq_remove_item_from_slots(const char *slug);
static void merchant_rep_bump_slug(const char *slug, int delta);
static const char *causal_latest_row_matching(const char *needle);
static int room_item_matches_query(const char *item, const char *q,
                                   const char *qnorm);
static int resolve_room_item_query(const char *name, char *resolved,
                                   size_t resolved_cap, char *msg,
                                   size_t msgcap);
static int resolve_inv_item_query(const char *name, int *ix_out, char *msg,
                                  size_t msgcap);
static void disambig_reset_all(void);
static int disambig_resolve_ambiguous(int act, int inv, const char *query,
                                      char picks[][MAX_ITEM_LEN], int n,
                                      char *resolved, size_t resolved_cap,
                                      char *msg, size_t msgcap);
static int disambig_try_followup(const char *line, char *msg, size_t msgcap,
                                 int *turn_advance);
static void cmd_talk(const char *target, const char *topic, char *msg,
                     size_t msgcap);
static void conv_clear(void);
static int conv_try_followup(const char *line, char *msg, size_t msgcap,
                             int *turn_advance);
static void cmd_price_compare(const char *rest, char *body, size_t cap);
static void examine_target(const char *raw, char *msg, size_t msgcap);
static int item_has_read_text(const char *id);
static int resolve_visible_item(const char *raw, char *id_out, size_t idcap,
                                char *msg, size_t msgcap);
static int room_entity_matches_query_here(const char *q, char *pretty_out,
                                          size_t pretty_cap);
static int inv_has(const char *name);
static void inv_add(const char *name);
static int inv_take_out(int ix, char *out, size_t outcap);
static int resolve_inv_item_query(const char *name, int *ix_out, char *msg,
                                  size_t msgcap);
static int resolve_room_item_query(const char *name, char *resolved,
                                   size_t resolved_cap, char *msg,
                                   size_t msgcap);
static int room_item_matches_query(const char *item, const char *q,
                                   const char *qnorm);
static void copy_capped(char *dst, size_t cap, const char *src);
static void chomp_line(char *s);
#ifdef AETER_MINIGAMES
static int game_open_reader(const char *item_id, char *msg, size_t msgcap);
static int game_read_resolve(const char *item_id, MgtReadDocument *doc);
#endif
static void strip_leading_articles(char *s);
static int parser_query_is_object_pronoun(const char *q);
static int parser_query_is_npc_pronoun(const char *q);
static int parser_expand_object_pronoun(const char *q, char *out, size_t cap,
                                        char *msg, size_t msgcap);
static int parser_expand_npc_pronoun(const char *q, char *out, size_t cap,
                                     char *msg, size_t msgcap);
static int parser_qualify_noun_query(char *q, int *ordinal, int *exclude_last);
static int str_contains_ci(const char *haystack, const char *needle);
static int text_has_word_ci(const char *text, const char *word);
static int protective_phrase_has_danger_context(const char *text);
static int protective_phrase_is_candidate(const char *text);
static const char *protective_phrase_pronoun(const char *text);
static int line_equals_one_of(const char *line, const char *const *candidates);

static void hist_push(int from_room) {
  if (g_hist_n >= BACK_HIST) {
    memmove(g_hist, g_hist + 1, (size_t)(BACK_HIST - 1) * sizeof g_hist[0]);
    g_hist_n = BACK_HIST - 1;
  }
  g_hist[g_hist_n++] = from_room;
}

static void remember_focus_item(const char *item_id) {
  size_t n;
  if (!item_id || !item_id[0]) return;
  n = strlen(item_id);
  if (n >= sizeof g_last_focus) n = sizeof g_last_focus - 1;
  memcpy(g_last_focus, item_id, n);
  g_last_focus[n] = '\0';
}

static void clear_focus(void) { g_last_focus[0] = '\0'; }

static void remember_dropped_item(const char *item_id) {
  size_t n;
  if (!item_id || !item_id[0]) return;
  n = strlen(item_id);
  if (n >= sizeof g_last_dropped) n = sizeof g_last_dropped - 1;
  memcpy(g_last_dropped, item_id, n);
  g_last_dropped[n] = '\0';
}

static int parser_rhs_is_surface(const char *rhs) {
  char buf[64];
  if (!rhs || !rhs[0]) return 0;
  strncpy(buf, rhs, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  strip_trailing_space(buf);
  strip_leading_articles(buf);
  return str_ieq(buf, "ground") || str_ieq(buf, "floor") || str_ieq(buf, "here") ||
         str_ieq(buf, "room") || str_ieq(buf, "the room");
}

static int parser_expand_dropped_ref(char *q, size_t cap) {
  if (!q || !q[0] || !g_last_dropped[0]) return 0;
  if (str_ieq(q, "dropped") || str_ieq(q, "the dropped") ||
      strstr(q, "i dropped") != NULL || strstr(q, "I dropped") != NULL ||
      (strstr(q, "dropped") != NULL &&
       (strstr(q, "what") != NULL || strstr(q, "one") != NULL ||
        strstr(q, "thing") != NULL || strstr(q, "item") != NULL))) {
    snprintf(q, cap, "%s", g_last_dropped);
    return 1;
  }
  return 0;
}

static int parser_query_is_object_pronoun(const char *q) {
  if (!q || !q[0]) return 0;
  return str_ieq(q, "it") || str_ieq(q, "that") || str_ieq(q, "this");
}

static int parser_query_is_npc_pronoun(const char *q) {
  if (!q || !q[0]) return 0;
  return str_ieq(q, "him") || str_ieq(q, "her") || str_ieq(q, "them") ||
         str_ieq(q, "he") || str_ieq(q, "she");
}

static int parser_expand_object_pronoun(const char *q, char *out, size_t cap,
                                        char *msg, size_t msgcap) {
  if (!parser_query_is_object_pronoun(q)) return 0;
  if (!g_last_focus[0]) {
    if (msg && msgcap)
      snprintf(msg, msgcap,
               "Nothing in mind — examine, take, or search something first.");
    return -1;
  }
  if (out && cap >= 2) snprintf(out, cap, "%s", g_last_focus);
  return 1;
}

static int parser_expand_npc_pronoun(const char *q, char *out, size_t cap,
                                     char *msg, size_t msgcap) {
  const char *ent;
  if (!parser_query_is_npc_pronoun(q)) return 0;
  ent = world_room_entity(g_room);
  if (g_last_npc[0] && ent && ent[0] && str_ieq(ent, g_last_npc)) {
    if (out && cap >= 2) snprintf(out, cap, "%s", g_last_npc);
    return 1;
  }
  if (ent && ent[0]) {
    if (out && cap >= 2) snprintf(out, cap, "%s", ent);
    return 1;
  }
  if (g_last_npc[0]) {
    char pretty[64];
    entity_pretty(g_last_npc, pretty, sizeof pretty);
    if (msg && msgcap)
      snprintf(msg, msgcap,
               "%s is not here now. Try  where %s  or  who all.", pretty,
               pretty);
    return -1;
  }
  if (msg && msgcap)
    snprintf(msg, msgcap,
             "No one in mind — try  who  or  talk to <name>.");
  return -1;
}

static int parser_qualify_noun_query(char *q, int *ordinal, int *exclude_last) {
  static const struct {
    const char *word;
    int n;
  } ord[] = {{"first ", 1},  {"second ", 2}, {"third ", 3},
             {"1st ", 1},    {"2nd ", 2},    {"3rd ", 3},
             {"fourth ", 4}, {"4th ", 4},    {NULL, 0}};
  size_t i;
  int o = 0, ex = 0;
  if (!q) return 0;
  strip_trailing_space(q);
  strip_leading_articles(q);
  if (!strncmp(q, "other ", 6)) {
    ex = 1;
    memmove(q, q + 6, strlen(q + 6) + 1);
    strip_leading_articles(q);
  }
  for (i = 0; ord[i].word; i++) {
    size_t wl = strlen(ord[i].word);
    if (!strncmp(q, ord[i].word, wl)) {
      o = ord[i].n;
      memmove(q, q + wl, strlen(q + wl) + 1);
      break;
    }
  }
  if (!o && q[0] >= '1' && q[0] <= '9') {
    char *p = q;
    o = 0;
    while (*p >= '0' && *p <= '9') {
      o = o * 10 + (int)(*p - '0');
      p++;
    }
    if (*p == ' ' || *p == '\0') {
      while (*p == ' ') p++;
      memmove(q, p, strlen(p) + 1);
    } else {
      o = 0;
    }
  }
  strip_leading_articles(q);
  strip_trailing_space(q);
  if (ordinal) *ordinal = o;
  if (exclude_last) *exclude_last = ex;
  return 1;
}

static int parser_token_matches_item(const char *tok, const char *item) {
  char inorm[MAX_ITEM_LEN];
  char pretty[96];
  char pnorm[96];
  char tnorm[MAX_ITEM_LEN];
  if (!tok || !tok[0] || !item || !item[0]) return 0;
  query_norm_underscore(tnorm, sizeof tnorm, tok);
  query_norm_underscore(inorm, sizeof inorm, item);
  item_pretty(item, pretty, sizeof pretty);
  query_norm_underscore(pnorm, sizeof pnorm, pretty);
  if (str_ieq(item, tok) || str_ieq(inorm, tnorm)) return 1;
  if (str_ieq(pretty, tok) || str_ieq(pnorm, tnorm)) return 1;
  if (strstr(item, tnorm) != NULL) return 1;
  if (strstr(inorm, tnorm) != NULL) return 1;
  if (strstr(pnorm, tnorm) != NULL) return 1;
  return 0;
}

static int parser_prepare_object_query(const char *raw, char *work, size_t wcap,
                                       int *ordinal, int *exclude_last,
                                       char *msg, size_t msgcap) {
  int pr;
  if (!raw || !work || wcap < 2) return 0;
  strncpy(work, raw, wcap - 1);
  work[wcap - 1] = '\0';
  strip_trailing_space(work);
  strip_leading_articles(work);
  pr = parser_expand_object_pronoun(work, work, wcap, msg, msgcap);
  if (pr < 0) return -1;
  (void)parser_expand_dropped_ref(work, wcap);
  if (!parser_qualify_noun_query(work, ordinal, exclude_last)) return 0;
  return work[0] ? 1 : 0;
}

static int min3_int(int a, int b, int c) {
  int m = a;
  if (b < m) m = b;
  if (c < m) m = c;
  return m;
}

static int levenshtein_upto(const char *a, const char *b, int maxd) {
  int la = (int)strlen(a);
  int lb = (int)strlen(b);
  int i, j;
  int prev, tmp, cost;
  int row[28];
  (void)maxd;
  if (la > 26 || lb > 26) return 999;
  for (j = 0; j <= lb; j++) row[j] = j;
  for (i = 1; i <= la; i++) {
    prev = row[0];
    row[0] = i;
    for (j = 1; j <= lb; j++) {
      tmp = row[j];
      cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
      row[j] = min3_int(prev + cost, row[j] + 1, row[j - 1] + 1);
      prev = tmp;
    }
  }
  return row[lb] > maxd ? maxd + 1 : row[lb];
}

static void suggest_typo(const char *word, char *out, size_t outcap) {
  static const char *const vocab[] = {
      "help",      "inventory", "look",      "exits",    "take",      "drop",
      "go",        "north",     "south",     "east",     "west",      "open",
      "unlock",    "search",    "wait",      "save",     "load",      "back",
      "get",       "grab",
      "trail",     "progress",  "status",    "stat",      "scan",     "loot",      "compare",
      "talk",
      "pick",      "use",       "where",     "who",      "menu",      "clear",
      "restore",   "reload",    "quicksave", "quickload", "inspect",  "examine",
      "read",
      "score",     "i",         "inv",       "pack",      "unstick",   "hints",     "errors",    "healthcheck", "diagnostics",
      "causality", "because",
      "nearby",
      "route",     "journal",   "quests",    "notes",     "saves",     "slots",
      "give",      "locate",
      "map",       "hint",      "listen",    "smell",     "objectives", "goals",
      "find",
      "describe",  "blurb",     "shout",     "say",        "brief",
      "verbose",   "recap",     "transcript", "touch",      "lights",
      "again",     "repeat",    "history",   "utilities",
      "about",     "credits",   "lore",      "version",   "ver",       "approach",
      "equip",     "goto",
      "wield",     "stow",      "visited",   "seen",      "takeoff",
      "close",     "buy",       "sell",      "purchase",  "wares",
      "shop",      "coins",     "wallet",    "eat",       "time",
      "weather",   "temperature", "temp",
      "skills",    "aptitudes", "reputation", "standing",
      "loadout",   "gear",      "equipment", "traits",
      "momentum",  "arc",       "perks",
      "pronouns",  "bio",       "backstory",
      "tainting",  "corruption", "rapport",
      "vitals",    "wellness",
      "drink",     "taste",     "sip",       "waypoints", "waystone",
      "waystones", "nexus",     "fasttravel", "lockcheck", "noise",
      "stealth",   "suspicion", NULL};
  int k, bestd = 4, d;
  const char *best = NULL;
  if (!word || !word[0] || strlen(word) < 3) {
    if (outcap) out[0] = '\0';
    return;
  }
  for (k = 0; vocab[k]; k++) {
    d = levenshtein_upto(word, vocab[k], 2);
    if (d < bestd) {
      bestd = d;
      best = vocab[k];
    }
  }
  if (best && bestd <= 2 && bestd > 0)
    snprintf(out, outcap, " Did you mean '%s'?", best);
  else if (outcap)
    out[0] = '\0';
}

static const char *const PARSER_TOPVERBS[] = {
    "about",     "again",     "approach",  "aptitudes", "back",
    "lore",
    "because",   "brief",     "buy",       "causality", "clear",
    "compare",   "describe",  "diagnostics", "done",    "drop",
    "drink",     "east",      "eat",       "equip",     "equipment",
    "errors",    "examine",   "exits",     "fasttravel", "find",
    "forge",     "gear",      "give",      "go",        "healthcheck",
    "help",      "hints",     "hold",      "inventory", "inv",
    "journal",   "listen",    "load",      "loadout",   "locate",
    "look",      "loot",      "map",       "menu",      "mods",
    "momentum",  "nexus",     "noise",     "north",     "notes",
    "objectives", "open",     "perks",     "progress",  "purchase",
    "quit",      "rapport",   "read",      "recap",     "repeat",
    "reputation", "route",    "save",      "say",       "scan",
    "score",     "search",    "sell",      "shout",     "smell",
    "south",     "status",    "take",      "talk",      "tainting",
    "time",      "touch",     "trail",     "unstick",   "unlock",
    "use",       "verbose",   "vitals",    "wait",      "wallet",
    "wares",     "waypoints", "weather",   "west",      "where",
    "who",       NULL};

static int parser_topverb_min_dist(const char *word, int maxd, int *ties_out,
                                   const char **best_out) {
  int k;
  int bestd = maxd + 1;
  int ties = 0;
  const char *best = NULL;
  if (!word || strlen(word) < 3) {
    if (ties_out) *ties_out = 0;
    if (best_out) *best_out = NULL;
    return maxd + 1;
  }
  for (k = 0; PARSER_TOPVERBS[k]; k++) {
    int d = levenshtein_upto(word, PARSER_TOPVERBS[k], maxd);
    if (d > maxd) continue;
    if (d < bestd) {
      bestd = d;
      best = PARSER_TOPVERBS[k];
      ties = 1;
    } else if (d == bestd) {
      ties++;
      if (best && strcmp(PARSER_TOPVERBS[k], best) < 0) best = PARSER_TOPVERBS[k];
    }
  }
  if (ties_out) *ties_out = ties;
  if (best_out) *best_out = best;
  return bestd;
}

static void rewrite_command(char *line, const char *verb, const char *rest);

static int parser_autofix_first_token_unique_typo(char *line) {
  char tok[64];
  const char *sp;
  size_t wl;
  int ties;
  const char *best;
  int bestd;
  if (!line || !line[0]) return 0;
  sp = strchr(line, ' ');
  wl = sp ? (size_t)(sp - line) : strlen(line);
  if (wl < 4 || wl >= sizeof tok) return 0;
  memcpy(tok, line, wl);
  tok[wl] = '\0';
  bestd = parser_topverb_min_dist(tok, 2, &ties, &best);
  if (bestd > 2 || ties != 1 || !best) return 0;
  if (bestd == 2 && wl < 6) return 0;
  if (strcmp(tok, best) == 0) return 0;
  if (sp) {
    while (*sp == ' ') sp++;
    rewrite_command(line, best, sp);
  } else {
    rewrite_command(line, best, "");
  }
  return 1;
}

static const char *const kNaturalLanguageStrip[] = {
    "would it be possible to ", "would you mind if i ", "do you mind if i ",
    "if you could please ", "could you please ",
    "would you please ",    "i would like to ",     "i would love to ",
    "can you please ",      "could i please ",      "may i please ",
    "let me please ",
    "i'm trying to ",       "im trying to ",        "go ahead and ",
    "i'm going to ",        "can i please ",        "im going to ",
    "i'd like to ",         "i want to ",           "i wanna ",
    "gonna ",               "could you ",
    "would you ",           "i need to ",           "i should ",
    "might as well ",       "i have to ",           "i've got to ",
    "ive got to ",          "i gotta ",             "i'm gonna ",
    "im gonna ",            "i will ",              "i'll ",
    "ill ",                 "i can ",               "i am going to ",
    "i am trying to ",      "i'm about to ",        "im about to ",
    "try and ",             "could i ",             "let me ",
    "please ",              "try to ",              "can i ",
    "lemme ",               "i ",
    NULL};

static void strip_natural_prefixes(char *line) {
  int guard = 0;
  while (guard++ < 10) {
    int i;
    int stripped = 0;
    while (line[0] == ' ')
      memmove(line, line + 1, strlen(line + 1) + 1);
    for (i = 0; kNaturalLanguageStrip[i]; i++) {
      size_t n = strlen(kNaturalLanguageStrip[i]);
      if (!strncmp(line, kNaturalLanguageStrip[i], n)) {
        memmove(line, line + n, strlen(line + n) + 1);
        stripped = 1;
        break;
      }
    }
    if (!stripped) break;
  }
}

static int is_intent_token(const char *tok, AetIntentCtx *ctx) {
  if (!tok || !tok[0] || !ctx) return 0;
  if (!strcmp(tok, "quiet") || !strcmp(tok, "quietly") ||
      !strcmp(tok, "silently") || !strcmp(tok, "softly")) {
    ctx->quiet = 1;
    ctx->present = 1;
    return 1;
  }
  if (!strcmp(tok, "loud") || !strcmp(tok, "loudly") || !strcmp(tok, "noisy")) {
    ctx->loud = 1;
    ctx->present = 1;
    return 1;
  }
  if (!strcmp(tok, "friendly") || !strcmp(tok, "kindly") ||
      !strcmp(tok, "politely") || !strcmp(tok, "respectfully") ||
      !strcmp(tok, "gently")) {
    ctx->friendly = 1;
    ctx->present = 1;
    return 1;
  }
  if (!strcmp(tok, "aggressively") || !strcmp(tok, "harshly") ||
      !strcmp(tok, "rudely") || !strcmp(tok, "coldly")) {
    ctx->harsh = 1;
    ctx->present = 1;
    return 1;
  }
  if (!strcmp(tok, "carefully") || !strcmp(tok, "cautiously") ||
      !strcmp(tok, "deliberately")) {
    ctx->careful = 1;
    ctx->quiet = 1;
    ctx->present = 1;
    return 1;
  }
  return 0;
}

static void extract_intent_modifiers(char *line, AetIntentCtx *ctx) {
  char out[INPUT_LINE_MAX];
  char tok[96];
  const char *p;
  size_t len = 0;
  if (!line || !ctx) return;
  memset(ctx, 0, sizeof *ctx);
  out[0] = '\0';
  p = line;
  while (*p) {
    size_t tn = 0;
    while (*p == ' ') p++;
    if (!*p) break;
    while (p[tn] && p[tn] != ' ' && tn + 1 < sizeof tok) {
      tok[tn] = p[tn];
      tn++;
    }
    tok[tn] = '\0';
    if (tok[0] && !is_intent_token(tok, ctx)) {
      if (len && len + 1 < sizeof out) out[len++] = ' ';
      if (len + tn < sizeof out) {
        memcpy(out + len, tok, tn);
        len += tn;
        out[len] = '\0';
      }
    }
    p += tn;
    while (*p && *p != ' ') p++;
  }
  if (out[0]) {
    strncpy(line, out, INPUT_LINE_MAX - 1);
    line[INPUT_LINE_MAX - 1] = '\0';
  }
}

static void format_intent_suffix(char *out, size_t cap, const AetIntentCtx *ctx) {
  int wrote = 0;
  if (!out || cap < 2 || !ctx || !ctx->present) {
    if (out && cap) out[0] = '\0';
    return;
  }
  out[0] = '\0';
  strncat(out, " [Intent:", cap - strlen(out) - 1);
  if (ctx->friendly) {
    strncat(out, wrote ? ", friendly" : " friendly", cap - strlen(out) - 1);
    wrote = 1;
  }
  if (ctx->harsh) {
    strncat(out, wrote ? ", harsh" : " harsh", cap - strlen(out) - 1);
    wrote = 1;
  }
  if (ctx->quiet) {
    strncat(out, wrote ? ", quiet" : " quiet", cap - strlen(out) - 1);
    wrote = 1;
  }
  if (ctx->loud) {
    strncat(out, wrote ? ", loud" : " loud", cap - strlen(out) - 1);
    wrote = 1;
  }
  if (ctx->careful) {
    strncat(out, wrote ? ", careful" : " careful", cap - strlen(out) - 1);
  }
  strncat(out, "]", cap - strlen(out) - 1);
}

static void apply_intent_to_social_feedback(char *msg, size_t msgcap) {
  const char *ent;
  char suf[80];
  int delta = 0;
  if (!msg || !msg[0] || !g_intent.present) return;
  ent = world_room_entity(g_room);
  if (g_intent.friendly) delta += 1;
  if (g_intent.harsh) delta -= 1;
  if (ent && ent[0] && delta != 0) merchant_rep_bump_slug(ent, delta);
  format_intent_suffix(suf, sizeof suf, &g_intent);
  if (suf[0] && strlen(msg) + strlen(suf) + 1 < msgcap) {
    strncat(msg, suf, msgcap - strlen(msg) - 1);
  }
}

static void clear_frame(void) {
  if (aet_autotest()) return;
#if defined(_WIN32)
  system("cls");
#else
  if (system("clear") != 0) {
    printf("\033[2J\033[H");
    fflush(stdout);
  }
#endif
}

static void ui_print_title(const char *title) {
  printf("%s%s%s\n", C_BORDER, UI_RULE, C_RESET);
  if (title && title[0])
    printf("  %s%s%s\n", C_TITLE, title, C_RESET);
  else
    printf("  %sAETERNITAS64%s\n", C_TITLE, C_RESET);
  printf("%s%s%s\n\n", C_BORDER, UI_RULE, C_RESET);
}

static void ui_print_panel_footer(const char *hint) {
  printf("\n%s%s%s\n", C_BORDER, UI_RULE, C_RESET);
  if (hint && hint[0]) printf("%s%s%s\n", C_PROMPT, hint, C_RESET);
}

static void ui_delay_ms(int ms) {
  if (aet_autotest() || ms <= 0) return;
#if defined(_WIN32)
  Sleep((DWORD)ms);
#else
  usleep((useconds_t)ms * 1000U);
#endif
}

typedef struct {
  int row;
  int col;
} RevealCell;

static void ui_rewind_lines(int lines) {
  if (lines <= 0) return;
#if defined(_WIN32)
  {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD pos;
    if (h == INVALID_HANDLE_VALUE) return;
    if (!GetConsoleScreenBufferInfo(h, &info)) return;
    pos = info.dwCursorPosition;
    if (pos.Y >= lines)
      pos.Y = (SHORT)(pos.Y - lines);
    else
      pos.Y = 0;
    pos.X = 0;
    SetConsoleCursorPosition(h, pos);
  }
#else
  printf("\033[%dA", lines);
  printf("\r");
#endif
}

static void ui_flash_screen(void) {
#if defined(_WIN32)
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO info;
  DWORD cells, written;
  COORD home = {0, 0};
  WORD old_attr = 0;
  if (h != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(h, &info)) {
    old_attr = info.wAttributes;
    cells = (DWORD)info.dwSize.X * (DWORD)info.dwSize.Y;
    SetConsoleTextAttribute(h, BACKGROUND_RED | BACKGROUND_GREEN |
                                   BACKGROUND_BLUE);
    FillConsoleOutputCharacterA(h, ' ', cells, home, &written);
    FillConsoleOutputAttribute(h, BACKGROUND_RED | BACKGROUND_GREEN |
                                      BACKGROUND_BLUE,
                               cells, home, &written);
    SetConsoleCursorPosition(h, home);
    fflush(stdout);
    ui_delay_ms(70);
    SetConsoleTextAttribute(h, old_attr);
  } else {
    ui_delay_ms(70);
  }
#else
  printf("\033[0;30;47m\033[2J\033[H");
  fflush(stdout);
  ui_delay_ms(70);
  printf("\033[0m");
#endif
  clear_frame();
}

#if defined(_WIN32)

static void win32_console_utf8_begin(void) {
  (void)SetConsoleOutputCP(CP_UTF8);
  (void)SetConsoleCP(CP_UTF8);
  
  if (setlocale(LC_ALL, "C.UTF-8"))
    return;
  if (setlocale(LC_ALL, ".UTF8"))
    return;
  if (setlocale(LC_ALL, ".UTF-8"))
    return;
  (void)setlocale(LC_CTYPE, "C.UTF-8");
}
#endif

static void ui_clear_color_codes(void) {
  C_RESET = "";
  C_BORDER = "";
  C_TITLE = "";
  C_REGION = "";
  C_HEADING = "";
  C_EXIT = "";
  C_ITEM = "";
  C_STATUS = "";
  C_PROMPT = "";
  C_BOOT_OK = "";
  C_BOOT_HI = "";
  C_MUTED = "";
}

static void ui_assign_color_codes(void) {
  C_RESET = "\x1b[0m";
  C_BORDER = "\x1b[90m";
  C_TITLE = "\x1b[96;1m";
  C_REGION = "\x1b[36m";
  C_HEADING = "\x1b[97;1m";
  C_EXIT = "\x1b[92;1m";
  C_ITEM = "\x1b[93;1m";
  C_STATUS = "\x1b[30;47;1m";
  C_PROMPT = "\x1b[97;1m";
  C_BOOT_OK = "\x1b[92;1m";
  C_BOOT_HI = "\x1b[97;1m";
  C_MUTED = "\x1b[37m";
}

static void ui_init_color(void) {
  const char *force = getenv("AETER_COLOR");
  g_use_color = 0;
#if defined(_WIN32)
  {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode) &&
        SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
      g_use_color = 1;
    }
  }
#else
  g_use_color = 1;
#endif
  if (force) {
    if (!strcmp(force, "0") || !strcmp(force, "off") || !strcmp(force, "false"))
      g_use_color = 0;
    else if (!strcmp(force, "1") || !strcmp(force, "on") ||
             !strcmp(force, "true"))
      g_use_color = 1;
  }
  if (g_settings_color_ov == 1)
    g_use_color = 1;
  else if (g_settings_color_ov == 0)
    g_use_color = 0;

  if (!g_use_color) {
    ui_clear_color_codes();
    return;
  }
  ui_assign_color_codes();
}

static void ui_dissolve_logo(const char *const *logo, int lines) {
  char frame[24][200];
  RevealCell cells[4096];
  int reveal_n = 0;
  int r, c, i;
  int frames = 40;
  int chars_per_frame;

  if (!logo || lines <= 0) return;
  if (lines > 24) lines = 24;

  for (r = 0; r < lines; r++) {
    int L = (int)strlen(logo[r]);
    if (L > 199) L = 199;
    for (c = 0; c < L; c++) {
      char ch = logo[r][c];
      if (ch != ' ' && ch != '\n' && ch != '\r' && reveal_n < 4096) {
        cells[reveal_n].row = r;
        cells[reveal_n].col = c;
        reveal_n++;
        frame[r][c] = ' ';
      } else {
        frame[r][c] = ch;
      }
    }
    frame[r][L] = '\0';
  }

  for (i = reveal_n - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    RevealCell tmp = cells[i];
    cells[i] = cells[j];
    cells[j] = tmp;
  }

  chars_per_frame = reveal_n / frames;
  if (chars_per_frame < 1) chars_per_frame = 1;

  for (r = 0; r < lines; r++) printf("%s\n", frame[r]);
  fflush(stdout);

  i = 0;
  while (i < reveal_n) {
    int k;
    for (k = 0; k < chars_per_frame && i < reveal_n; k++, i++) {
      int rr = cells[i].row;
      int cc = cells[i].col;
      frame[rr][cc] = logo[rr][cc];
    }
    ui_rewind_lines(lines);
    for (r = 0; r < lines; r++) printf("%s\n", frame[r]);
    fflush(stdout);
    ui_delay_ms(30);
  }
}

static const char *dir_short_name(int d) {
  switch (d) {
    case DIR_N:
      return "N";
    case DIR_S:
      return "S";
    case DIR_E:
      return "E";
    case DIR_W:
      return "W";
    case DIR_U:
      return "U";
    case DIR_D:
      return "D";
    case DIR_NE:
      return "NE";
    case DIR_NW:
      return "NW";
    case DIR_SE:
      return "SE";
    case DIR_SW:
      return "SW";
    case DIR_IN:
      return "IN";
    case DIR_OUT:
      return "OUT";
    default:
      if (d >= 0 && d < DIR_COUNT) return DIR_LABELS[d];
      return "?";
  }
}

static void print_wrapped_paragraph(const char *text, int indent, int width) {
  int col = 0;
  int i;
  const char *p = text ? text : "";
  if (width < indent + 8) width = indent + 8;
  for (i = 0; i < indent; i++) putchar(' ');
  col = indent;
  while (*p) {
    while (*p == ' ') p++;
    if (*p == '\n' || *p == '\r') {
      while (*p == '\n' || *p == '\r') p++;
      putchar('\n');
      for (i = 0; i < indent; i++) putchar(' ');
      col = indent;
      continue;
    }
    if (!*p) break;
    {
      const char *w = p;
      int wlen = 0;
      while (w[wlen] && w[wlen] != ' ' && w[wlen] != '\n' && w[wlen] != '\r')
        wlen++;
      if (col > indent && col + 1 + wlen > width) {
        putchar('\n');
        for (i = 0; i < indent; i++) putchar(' ');
        col = indent;
      } else if (col > indent) {
        putchar(' ');
        col++;
      }
      for (i = 0; i < wlen; i++) putchar(w[i]);
      col += wlen;
      p = w + wlen;
    }
  }
  putchar('\n');
}

static void seed_rng_once(void) {
  static int seeded = 0;
  unsigned seed;
  if (seeded) return;
  seed = (unsigned)time(NULL) ^ (unsigned)clock();
#if defined(_WIN32)
  seed ^= (unsigned)GetTickCount();
  seed ^= (unsigned)GetCurrentProcessId();
#else
  seed ^= (unsigned)getpid();
#endif
  srand(seed);
  seeded = 1;
}

static void splash_wait(void) {
  static const char *const ascii_title[] = {
      "                            ,;                 ,;           L.               "
      "                               .",
      "                          f#i                f#i j.         EW:        ,ft t"
      "                               ;W",
      "                 ..     .E#t  GEEEEEEEL    .E#t  EW,        E##;       t#E "
      "Ej GEEEEEEEL         ..        f#E",
      "                ;W,    i#W,   ,;;L#K;;.   i#W,   E##j       E###t      t#E "
      "E#,,;;L#K;;.        ;W,      .E#f",
      "               j##,   L#D.       t#E     L#D.    E###D.     E#fE#f     t#E "
      "E#t   t#E          j##,     iWW;  ",
      "              G###, :K#Wfff;     t#E   :K#Wfff;  E#jG#W;    E#t D#G    t#E "
      "E#t   t#E         G###,    L##Lffi",
      "            :E####, i##WLLLLt    t#E   i##WLLLLt E#t t##f   E#t  f#E.  t#E "
      "E#t   t#E       :E####,   tLLG##L ",
      "           ;W#DG##,  .E#L        t#E    .E#L     E#t  :K#E: E#t   t#K: t#E "
      "E#t   t#E      ;W#DG##,     ,W#i  ",
      "          j###DW##,    f#E:      t#E      f#E:   E#KDDDD###iE#t    ;#W,t#E "
      "E#t   t#E     j###DW##,    j#E.   ",
      "         G##i,,G##,     ,WW;     t#E       ,WW;  E#f,t#Wi,,,E#t     :K#D#E "
      "E#t   t#E    G##i,,G##,  .D#j     ",
      "       :K#K:   L##,      .D#;    t#E        .D#; E#t  ;#W:  E#t      .E##E "
      "E#t   t#E  :K#K:   L##, ,WK,      ",
      "      ;##D.    L##,        tt     fE          tt DWi   ,KK: ..         G#E "
      "E#t    fE ;##D.    L##, EG.       ",
      "      ,,,      .,,                 :                                    fE "
      ",;.     : ,,,      .,,  ,         ",
      "                                                                         ,   "
      "                                 "};
  char buf[32];
  int blink;
  seed_rng_once();
  clear_frame();
  printf("%sA:\\> AETERNITAS.EXE%s\n", C_BOOT_HI, C_RESET);
  fflush(stdout);
  ui_delay_ms(800);
  printf("Initializing Extended VGA Mode (120x40)... %sOK%s\n", C_BOOT_OK, C_RESET);
  fflush(stdout);
  ui_delay_ms(500);
  printf("Checking Base Memory (640K)... %sOK%s\n", C_BOOT_OK, C_RESET);
  fflush(stdout);
  ui_delay_ms(600);
  printf("Mounting [FLOPPY_DRIVE:A]...\n");
  fflush(stdout);
  ui_delay_ms(400);
  printf("  Reading Sector 0x01");
  fflush(stdout);
  ui_delay_ms(300);
  printf(".");
  fflush(stdout);
  ui_delay_ms(300);
  printf(".");
  fflush(stdout);
  ui_delay_ms(350);
  printf(". %sDONE%s\n", C_BOOT_OK, C_RESET);
  fflush(stdout);
  ui_delay_ms(250);
  printf("  Loading Core Engine... %sOK%s\n\n", C_BOOT_OK, C_RESET);
  fflush(stdout);
  ui_delay_ms(700);
  printf("%sSYSTEM READY.%s\n", C_BOOT_HI, C_RESET);
  fflush(stdout);
  ui_delay_ms(1000);
  ui_flash_screen();
  ui_dissolve_logo(ascii_title, 14);
  printf("\n");
  for (blink = 0; blink < 3; blink++) {
    printf("Press [ENTER] to start...\r");
    fflush(stdout);
    ui_delay_ms(260);
    printf("                         \r");
    fflush(stdout);
    ui_delay_ms(220);
  }
  printf("%sPress [ENTER] to start...%s\n", C_PROMPT, C_RESET);
  fflush(stdout);
  if (!aet_autotest()) (void)fgets(buf, sizeof buf, stdin);
}

static void init_items_from_world(void) {
  int r;
  for (r = 0; r < WORLD_ROOM_COUNT; r++) {
    const char *const *lst = world_item_list(r);
    const char *const *hid = world_hidden_item_list(r);
    g_room_item_n[r] = 0;
    g_hidden_n[r] = 0;
    if (lst) {
      while (*lst && g_room_item_n[r] < MAX_ITEMS_ROOM) {
        strncpy(g_room_items[r][g_room_item_n[r]], *lst, MAX_ITEM_LEN - 1);
        g_room_items[r][g_room_item_n[r]][MAX_ITEM_LEN - 1] = '\0';
        g_room_item_n[r]++;
        lst++;
      }
    }
    if (hid) {
      while (*hid && g_hidden_n[r] < MAX_ITEMS_ROOM) {
        strncpy(g_hidden_items[r][g_hidden_n[r]], *hid, MAX_ITEM_LEN - 1);
        g_hidden_items[r][g_hidden_n[r]][MAX_ITEM_LEN - 1] = '\0';
        g_hidden_n[r]++;
        hid++;
      }
    }
  }
}

static int str_ieq(const char *a, const char *b) {
  if (!a || !b) return 0;
  for (; *a && *b; a++, b++) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
  }
  return *a == *b;
}

static void apply_reference_new_game_bootstrap(void) { g_coins = 50; }

static int room_has_visible_item(int room, const char *id) {
  int i;
  if (room < 0 || room >= WORLD_ROOM_COUNT || !id || !id[0]) return 0;
  for (i = 0; i < g_room_item_n[room]; i++)
    if (str_ieq(g_room_items[room][i], id)) return 1;
  return 0;
}

static void room_add_visible_item(int room, const char *id) {
  if (room < 0 || room >= WORLD_ROOM_COUNT || !id || !id[0]) return;
  if (room_has_visible_item(room, id)) return;
  if (g_room_item_n[room] >= MAX_ITEMS_ROOM) return;
  strncpy(g_room_items[room][g_room_item_n[room]], id, MAX_ITEM_LEN - 1);
  g_room_items[room][g_room_item_n[room]][MAX_ITEM_LEN - 1] = '\0';
  g_room_item_n[room]++;
}

static void place_persistent_story_items(void) {
  int r = world_room_index("nexus_point_1");
  if (r >= 0) room_add_visible_item(r, "bucket");
}

static int item_is_container(const char *slug) {
  if (!slug || !slug[0]) return 0;
  return str_ieq(slug, "mailbox") || str_ieq(slug, "chest") ||
         str_ieq(slug, "old_box") || str_ieq(slug, "locked_chest") ||
         str_ieq(slug, "storage_chests") || str_ieq(slug, "crates");
}

static void containers_clear(void) {
  g_container_n = 0;
  memset(g_containers, 0, sizeof g_containers);
}

static int container_carried_here(const char *id) {
  return inv_has(id);
}

static int container_find_idx(int room, const char *id) {
  int i;
  if (!id || !id[0]) return -1;
  for (i = 0; i < g_container_n; i++) {
    if (g_containers[i].room != room) continue;
    if (str_ieq(g_containers[i].id, id)) return i;
  }
  return -1;
}

static int container_find_idx_any(const char *id) {
  int i;
  if (!id || !id[0]) return -1;
  for (i = 0; i < g_container_n; i++)
    if (str_ieq(g_containers[i].id, id)) return i;
  return -1;
}

static int container_room_is_nested(int room) {
  return room <= AET_CONTAINER_NESTED_BASE &&
         room > AET_CONTAINER_NESTED_BASE - AET_CONTAINER_MAX;
}

static int container_nested_parent_cix(int room) {
  return AET_CONTAINER_NESTED_BASE - room;
}

static int container_make_nested_room(int parent_cix) {
  return AET_CONTAINER_NESTED_BASE - parent_cix;
}

static int container_room_valid(int room) {
  if (room == AET_CONTAINER_ROOM_INV) return 1;
  if (room >= 0 && room < WORLD_ROOM_COUNT) return 1;
  return container_room_is_nested(room);
}

static int container_parent_lists_item(int parent_cix, const char *id) {
  int i;
  if (parent_cix < 0 || parent_cix >= g_container_n || !id || !id[0])
    return 0;
  for (i = 0; i < g_containers[parent_cix].content_n; i++)
    if (str_ieq(g_containers[parent_cix].contents[i], id)) return 1;
  return 0;
}

static int container_nested_accessible(int cix) {
  int pcix;
  if (cix < 0 || cix >= g_container_n) return 0;
  if (!container_room_is_nested(g_containers[cix].room)) return 0;
  pcix = container_nested_parent_cix(g_containers[cix].room);
  if (pcix < 0 || pcix >= g_container_n) return 0;
  if (!g_containers[pcix].open) return 0;
  return container_parent_lists_item(pcix, g_containers[cix].id);
}

static int container_accessible_here(int room, int cix) {
  const AetContainerRec *c;
  if (cix < 0 || cix >= g_container_n) return 0;
  c = &g_containers[cix];
  if (c->room == AET_CONTAINER_ROOM_INV) return container_carried_here(c->id);
  if (container_room_is_nested(c->room)) return container_nested_accessible(cix);
  return c->room == room && room_has_visible_item(room, c->id);
}

static void container_set_location(const char *id, int room) {
  int cix = container_find_idx_any(id);
  if (cix >= 0) g_containers[cix].room = room;
}

static int container_ensure(int room, const char *id) {
  int ix;
  if (!id || !id[0]) return -1;
  ix = container_find_idx(room, id);
  if (ix >= 0) return ix;
  if (g_container_n >= AET_CONTAINER_MAX) return -1;
  ix = g_container_n++;
  g_containers[ix].room = room;
  copy_capped(g_containers[ix].id, sizeof g_containers[ix].id, id);
  return ix;
}

static int container_content_add(int cix, const char *item) {
  int nix;
  if (cix < 0 || cix >= g_container_n || !item || !item[0]) return 0;
  if (g_containers[cix].content_n >= AET_CONTAINER_SLOTS) return 0;
  copy_capped(g_containers[cix].contents[g_containers[cix].content_n],
              sizeof g_containers[cix].contents[0], item);
  g_containers[cix].content_n++;
  if (item_is_container(item)) {
    nix = container_find_idx_any(item);
    if (nix < 0) {
      if (g_container_n >= AET_CONTAINER_MAX) {
        g_containers[cix].content_n--;
        g_containers[cix].contents[g_containers[cix].content_n][0] = '\0';
        return 0;
      }
      nix = g_container_n++;
      g_containers[nix].room = container_make_nested_room(cix);
      copy_capped(g_containers[nix].id, sizeof g_containers[nix].id, item);
    } else {
      g_containers[nix].room = container_make_nested_room(cix);
    }
  }
  return 1;
}

static int container_content_remove(int cix, const char *item) {
  int i, j;
  if (cix < 0 || cix >= g_container_n || !item || !item[0]) return 0;
  for (i = 0; i < g_containers[cix].content_n; i++) {
    if (!str_ieq(g_containers[cix].contents[i], item)) continue;
    for (j = i; j < g_containers[cix].content_n - 1; j++)
      copy_capped(g_containers[cix].contents[j],
                  sizeof g_containers[cix].contents[0],
                  g_containers[cix].contents[j + 1]);
    g_containers[cix].content_n--;
    return 1;
  }
  return 0;
}

static void hidden_remove_item(int room, const char *id) {
  int i, j;
  if (room < 0 || room >= WORLD_ROOM_COUNT || !id || !id[0]) return;
  for (i = 0; i < g_hidden_n[room]; i++) {
    if (!str_ieq(g_hidden_items[room][i], id)) continue;
    for (j = i; j < g_hidden_n[room] - 1; j++)
      memcpy(g_hidden_items[room][j], g_hidden_items[room][j + 1], MAX_ITEM_LEN);
    g_hidden_n[room]--;
    return;
  }
}

static const char *container_unlock_key_slug(const char *container_id) {
  if (!container_id || !container_id[0]) return NULL;
  if (str_ieq(container_id, "locked_chest")) return "skeleton_key";
  return NULL;
}

static int container_key_item_ok(const char *key_item,
                                 const char *container_id) {
  const char *need;
  if (!key_item || !key_item[0] || !container_id || !container_id[0]) return 0;
  if (str_ieq(key_item, "skeleton_key")) return 1;
  need = container_unlock_key_slug(container_id);
  return need && str_ieq(key_item, need);
}

static int container_inv_has_key(const char *container_id, char *key_out,
                                 size_t keycap) {
  int i;
  if (key_out && keycap > 0) key_out[0] = '\0';
  for (i = 0; i < g_inv_n; i++) {
    if (!container_key_item_ok(g_inv[i], container_id)) continue;
    if (key_out && keycap > 1)
      copy_capped(key_out, keycap, g_inv[i]);
    return 1;
  }
  return 0;
}

static int container_resolve_here(int room, const char *query, char *id_out,
                                size_t idcap, int *cix_out) {
  char work[256], qnorm[MAX_ITEM_LEN];
  int i;
  if (!query || !query[0] || !id_out || idcap < 2) return 0;
  strncpy(work, query, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_leading_articles(work);
  strip_trailing_space(work);
  query_norm_underscore(qnorm, sizeof qnorm, work);
  for (i = 0; i < g_container_n; i++) {
    char inorm[MAX_ITEM_LEN], pretty[96], pnorm[96];
    const AetContainerRec *c = &g_containers[i];
    if (!container_accessible_here(room, i)) continue;
    query_norm_underscore(inorm, sizeof inorm, c->id);
    item_pretty(c->id, pretty, sizeof pretty);
    query_norm_underscore(pnorm, sizeof pnorm, pretty);
    if (!(str_ieq(c->id, work) || str_ieq(inorm, qnorm) || str_ieq(pretty, work) ||
          str_ieq(pnorm, qnorm) || strstr(inorm, qnorm) != NULL))
      continue;
    copy_capped(id_out, idcap, c->id);
    if (cix_out) *cix_out = i;
    return 1;
  }
  return 0;
}

static void container_format_contents(int cix, char *msg, size_t msgcap) {
  int i;
  char pretty[96];
  if (cix < 0 || cix >= g_container_n) return;
  if (g_containers[cix].content_n == 0) {
    strncat(msg, "It is empty.", msgcap - strlen(msg) - 1);
    return;
  }
  strncat(msg, "Inside: ", msgcap - strlen(msg) - 1);
  for (i = 0; i < g_containers[cix].content_n; i++) {
    item_pretty(g_containers[cix].contents[i], pretty, sizeof pretty);
    if (i) strncat(msg, ", ", msgcap - strlen(msg) - 1);
    strncat(msg, pretty, msgcap - strlen(msg) - 1);
  }
  strncat(msg, ".", msgcap - strlen(msg) - 1);
}

static void containers_init_new_game(void) {
  int ix, west, attic;
  containers_clear();
  west = world_room_index("west_of_house");
  attic = world_room_index("attic");
  if (west >= 0) {
    ix = container_ensure(west, "mailbox");
    if (ix >= 0) {
      g_containers[ix].open = 0;
      g_containers[ix].locked = 0;
      container_content_add(ix, "leaflet");
      hidden_remove_item(west, "leaflet");
      if (container_content_add(ix, "old_box")) {
        int box_ix = container_find_idx_any("old_box");
        if (box_ix >= 0) container_content_add(box_ix, "reed");
      }
    }
  }
  if (attic >= 0) {
    ix = container_ensure(attic, "chest");
    if (ix >= 0) {
      g_containers[ix].open = 0;
      g_containers[ix].locked = 0;
      container_content_add(ix, "ancient_coin");
    }
  }
  if (west >= 0) {
    room_add_visible_item(west, "locked_chest");
    ix = container_ensure(west, "locked_chest");
    if (ix >= 0) {
      g_containers[ix].open = 0;
      g_containers[ix].locked = 1;
      container_content_add(ix, "house_key");
    }
  }
}

static int containers_write_section(FILE *fp) {
  int i, j;
  fprintf(fp, "CONTAINERS\n%d\n", g_container_n);
  for (i = 0; i < g_container_n; i++) {
    const AetContainerRec *c = &g_containers[i];
    fprintf(fp, "%d %s %d %d %d\n", c->room, c->id, (int)c->open,
            (int)c->locked, c->content_n);
    for (j = 0; j < c->content_n; j++) fprintf(fp, "%s\n", c->contents[j]);
  }
  return 1;
}

static int containers_read_section(FILE *fp) {
  char buf[512];
  int n, i, k;
  if (!fgets(buf, sizeof buf, fp)) return 0;
  chomp_line(buf);
  n = atoi(buf);
  if (n < 0 || n > AET_CONTAINER_MAX) return 0;
  containers_clear();
  for (i = 0; i < n; i++) {
    int room, open, locked, cn;
    char id[MAX_ITEM_LEN];
    if (!fgets(buf, sizeof buf, fp)) return 0;
    chomp_line(buf);
    if (sscanf(buf, "%d %47s %d %d %d", &room, id, &open, &locked, &cn) != 5)
      return 0;
    if (!container_room_valid(room)) return 0;
    if (cn < 0 || cn > AET_CONTAINER_SLOTS) return 0;
    if (g_container_n >= AET_CONTAINER_MAX) return 0;
    g_containers[g_container_n].room = room;
    copy_capped(g_containers[g_container_n].id, MAX_ITEM_LEN, id);
    g_containers[g_container_n].open = (unsigned char)(open ? 1 : 0);
    g_containers[g_container_n].locked = (unsigned char)(locked ? 1 : 0);
    g_containers[g_container_n].content_n = cn;
    for (k = 0; k < cn; k++) {
      if (!fgets(buf, sizeof buf, fp)) return 0;
      chomp_line(buf);
      copy_capped(g_containers[g_container_n].contents[k],
                  sizeof g_containers[g_container_n].contents[0], buf);
    }
    g_container_n++;
  }
  return 1;
}

static void container_sync_carried(int cix, int to_room) {
  if (cix < 0 || cix >= g_container_n) return;
  g_containers[cix].room = to_room;
}

static void container_on_item_taken(int room, const char *id) {
  int cix = container_find_idx(room, id);
  if (cix < 0) return;
  container_sync_carried(cix, AET_CONTAINER_ROOM_INV);
}

static void container_on_item_dropped(int room, const char *id) {
  int cix = container_find_idx(AET_CONTAINER_ROOM_INV, id);
  if (cix < 0) return;
  container_sync_carried(cix, room);
}

static int container_open_cmd(int room, const char *query, char *msg,
                              size_t msgcap) {
  char cid[MAX_ITEM_LEN];
  int cix = -1;
  char pretty[96];
  if (!container_resolve_here(room, query, cid, sizeof cid, &cix) || cix < 0) {
    snprintf(msg, msgcap, "Nothing like that to open here.");
    return 0;
  }
  item_pretty(cid, pretty, sizeof pretty);
  if (g_containers[cix].locked) {
    const char *need = container_unlock_key_slug(cid);
    if (need)
      snprintf(msg, msgcap,
               "The %s is locked shut. You need the right key (try  unlock %s  "
               "or  use key on %s).",
               pretty, cid, cid);
    else
      snprintf(msg, msgcap, "The %s is locked shut.", pretty);
    return 0;
  }
  g_containers[cix].open = 1;
  remember_focus_item(cid);
  if (g_containers[cix].content_n == 0) {
    snprintf(msg, msgcap, "You open the %s. It is empty.", pretty);
  } else {
    snprintf(msg, msgcap, "You open the %s. ", pretty);
    container_format_contents(cix, msg, msgcap);
  }
  return 1;
}

static int container_unlock_cmd(int room, const char *cont_q, const char *key_q,
                              char *msg, size_t msgcap) {
  char cont_id[MAX_ITEM_LEN], key_id[MAX_ITEM_LEN];
  char cont_pretty[96], key_pretty[96];
  int cix = -1, kix = -1, kr;
  const char *need;
  if (!cont_q || !cont_q[0]) return 0;
  if (!container_resolve_here(room, cont_q, cont_id, sizeof cont_id, &cix) ||
      cix < 0)
    return 0;
  item_pretty(cont_id, cont_pretty, sizeof cont_pretty);
  if (!g_containers[cix].locked) {
    snprintf(msg, msgcap, "The %s is not locked.", cont_pretty);
    return 1;
  }
  if (key_q && key_q[0]) {
    kr = resolve_inv_item_query(key_q, &kix, msg, msgcap);
    if (kr < 0) return 1;
    if (kr <= 0 || kix < 0) {
      snprintf(msg, msgcap, "You are not carrying that key.");
      return 1;
    }
    copy_capped(key_id, sizeof key_id, g_inv[kix]);
  } else if (!container_inv_has_key(cont_id, key_id, sizeof key_id)) {
    need = container_unlock_key_slug(cont_id);
    if (need) {
      item_pretty(need, key_pretty, sizeof key_pretty);
      snprintf(msg, msgcap,
               "The %s is locked. You need something like a %s — or try  use "
               "%s on %s.",
               cont_pretty, key_pretty, need, cont_id);
    } else if (has_lockpick_tool())
      snprintf(msg, msgcap,
               "The %s is locked. Try a key or  use lockpick on %s.", cont_pretty,
               cont_id);
    else
      snprintf(msg, msgcap, "The %s is locked, and you have no key that fits.",
               cont_pretty);
    return 1;
  }
  if (!container_key_item_ok(key_id, cont_id)) {
    item_pretty(key_id, key_pretty, sizeof key_pretty);
    snprintf(msg, msgcap, "The %s does not fit the %s.", key_pretty,
             cont_pretty);
    return 1;
  }
  g_containers[cix].locked = 0;
  remember_focus_item(cont_id);
  item_pretty(key_id, key_pretty, sizeof key_pretty);
  snprintf(msg, msgcap, "You unlock the %s with the %s.", cont_pretty,
           key_pretty);
  g_score += 3;
  return 1;
}

static int container_lockpick_cmd(int room, const char *cont_q, char *msg,
                                  size_t msgcap) {
  char cont_id[MAX_ITEM_LEN], cont_pretty[96];
  int cix = -1;
  if (!has_lockpick_tool()) return 0;
  if (!container_resolve_here(room, cont_q, cont_id, sizeof cont_id, &cix) ||
      cix < 0)
    return 0;
  if (!g_containers[cix].locked) return 0;
  item_pretty(cont_id, cont_pretty, sizeof cont_pretty);
#ifdef AETER_MINIGAMES
  if (try_minigame("lockpick", msg, msgcap)) {
    MgtPersistentState *st = mgt_host_state();
    game_mgt_apply_lockpick_stealth(st, msg, msgcap);
    if (st && st->last_success == 1) {
      g_containers[cix].locked = 0;
      snprintf(msg, msgcap, "The %s gives way under your tools.", cont_pretty);
      remember_focus_item(cont_id);
      g_score += 5;
    }
    return 1;
  }
#endif
  g_containers[cix].locked = 0;
  snprintf(msg, msgcap, "You coax the %s lock until it yields.", cont_pretty);
  remember_focus_item(cont_id);
  g_score += 5;
  return 1;
}

static int container_take_all_from(int room, const char *cont_q, char *msg,
                                   size_t msgcap) {
  char cont_id[MAX_ITEM_LEN], item_id[MAX_ITEM_LEN];
  char cont_pretty[96], item_pretty_buf[96], line[512];
  int cix = -1, n = 0;
  size_t len;
  if (!container_resolve_here(room, cont_q, cont_id, sizeof cont_id, &cix) ||
      cix < 0) {
    snprintf(msg, msgcap, "You do not see that container here.");
    return 0;
  }
  item_pretty(cont_id, cont_pretty, sizeof cont_pretty);
  if (!g_containers[cix].open) {
    snprintf(msg, msgcap, "The %s is closed.", cont_pretty);
    return 0;
  }
  if (g_containers[cix].content_n == 0) {
    snprintf(msg, msgcap, "There is nothing to take from the %s.", cont_pretty);
    return 0;
  }
  line[0] = '\0';
  while (g_containers[cix].content_n > 0) {
    if (g_inv_n >= MAX_INV) {
      if (n == 0)
        snprintf(msg, msgcap,
                 "You cannot carry more (%d-item limit). Drop something first.",
                 MAX_INV);
      else
        snprintf(msg, msgcap,
                 "You take what you can from the %s; something remains inside.\n%s",
                 cont_pretty, line);
      return n > 0;
    }
    copy_capped(item_id, sizeof item_id, g_containers[cix].contents[0]);
    container_content_remove(cix, item_id);
    if (item_is_container(item_id))
      container_set_location(item_id, AET_CONTAINER_ROOM_INV);
    inv_add(item_id);
    remember_focus_item(item_id);
    item_pretty(item_id, item_pretty_buf, sizeof item_pretty_buf);
    if (n == 0)
      snprintf(line, sizeof line, "Taken from %s: %s", cont_pretty,
               item_pretty_buf);
    else {
      len = strlen(line);
      snprintf(line + len, sizeof line - len, ", %s", item_pretty_buf);
    }
    n++;
    g_score += 5;
  }
  snprintf(msg, msgcap, "%s", line);
  return 1;
}

static int container_close_cmd(int room, const char *query, char *msg,
                               size_t msgcap) {
  char cid[MAX_ITEM_LEN];
  int cix = -1;
  char pretty[96];
  if (!container_resolve_here(room, query, cid, sizeof cid, &cix) || cix < 0) {
    snprintf(msg, msgcap, "Nothing like that to close here.");
    return 0;
  }
  item_pretty(cid, pretty, sizeof pretty);
  if (!g_containers[cix].open) {
    snprintf(msg, msgcap, "The %s is already closed.", pretty);
    return 0;
  }
  g_containers[cix].open = 0;
  snprintf(msg, msgcap, "You close the %s.", pretty);
  return 1;
}

static int container_take_from(int room, const char *item_q, const char *cont_q,
                               char *msg, size_t msgcap) {
  char cont_id[MAX_ITEM_LEN], work[256], qnorm[MAX_ITEM_LEN];
  char picks[6][MAX_ITEM_LEN], item_id[MAX_ITEM_LEN];
  char item_pretty_buf[96], cont_pretty[96];
  int cix = -1, pn = 0, ordinal = 0, exclude_last = 0, prep, ii;
  if (!container_resolve_here(room, cont_q, cont_id, sizeof cont_id, &cix) ||
      cix < 0) {
    snprintf(msg, msgcap, "You do not see that container here.");
    return 0;
  }
  item_pretty(cont_id, cont_pretty, sizeof cont_pretty);
  if (!g_containers[cix].open) {
    snprintf(msg, msgcap, "The %s is closed.", cont_pretty);
    return 0;
  }
  prep = parser_prepare_object_query(item_q, work, sizeof work, &ordinal,
                                     &exclude_last, msg, msgcap);
  if (prep < 0) return 0;
  if (!work[0]) {
    snprintf(msg, msgcap, "Take what from the %s?", cont_pretty);
    return 0;
  }
  query_norm_underscore(qnorm, sizeof qnorm, work);
  for (ii = 0; ii < g_containers[cix].content_n; ii++) {
    if (exclude_last && g_last_focus[0] &&
        str_ieq(g_containers[cix].contents[ii], g_last_focus))
      continue;
    if (!room_item_matches_query(g_containers[cix].contents[ii], work, qnorm))
      continue;
    if (pn < 6)
      copy_capped(picks[pn++], MAX_ITEM_LEN, g_containers[cix].contents[ii]);
  }
  if (pn == 0) {
    snprintf(msg, msgcap, "That is not inside the %s.", cont_pretty);
    return 0;
  }
  if (pn > 1 && ordinal >= 1 && ordinal <= pn)
    copy_capped(item_id, sizeof item_id, picks[ordinal - 1]);
  else if (pn == 1)
    copy_capped(item_id, sizeof item_id, picks[0]);
  else {
    snprintf(msg, msgcap,
             "Which one inside the %s? (%d matches — try an ordinal.)",
             cont_pretty, pn);
    return 0;
  }
  if (g_inv_n >= MAX_INV) {
    snprintf(msg, msgcap,
             "You cannot carry more (%d-item limit). Drop something first.",
             MAX_INV);
    return 0;
  }
  container_content_remove(cix, item_id);
  if (item_is_container(item_id))
    container_set_location(item_id, AET_CONTAINER_ROOM_INV);
  inv_add(item_id);
  remember_focus_item(item_id);
  item_pretty(item_id, item_pretty_buf, sizeof item_pretty_buf);
  snprintf(msg, msgcap, "Taken from %s: %s", cont_pretty, item_pretty_buf);
  g_score += 5;
  return 1;
}

static int container_put_in(int room, const char *item_q, const char *cont_q,
                            char *msg, size_t msgcap) {
  char cont_id[MAX_ITEM_LEN], taken[MAX_ITEM_LEN];
  char item_pretty_buf[96], cont_pretty[96];
  int cix = -1, ix = -1, r;
  if (!container_resolve_here(room, cont_q, cont_id, sizeof cont_id, &cix) ||
      cix < 0) {
    snprintf(msg, msgcap, "You do not see that container here.");
    return 0;
  }
  item_pretty(cont_id, cont_pretty, sizeof cont_pretty);
  if (!g_containers[cix].open) {
    snprintf(msg, msgcap, "The %s is closed.", cont_pretty);
    return 0;
  }
  if (g_containers[cix].content_n >= AET_CONTAINER_SLOTS) {
    snprintf(msg, msgcap, "There is no room left inside the %s.", cont_pretty);
    return 0;
  }
  g_disambig_next_act = DISAMBIG_ACT_DROP;
  r = resolve_inv_item_query(item_q, &ix, msg, msgcap);
  if (r < 0) return 0;
  if (r <= 0 || ix < 0) {
    snprintf(msg, msgcap, "You are not carrying that.");
    return 0;
  }
  if (str_ieq(g_inv[ix], cont_id)) {
    snprintf(msg, msgcap, "You cannot put a container inside itself.");
    return 0;
  }
  inv_take_out(ix, taken, sizeof taken);
  if (str_ieq(g_ready_item, taken)) g_ready_item[0] = '\0';
  eq_remove_item_from_slots(taken);
  if (str_ieq(g_last_focus, taken)) clear_focus();
  container_content_add(cix, taken);
  remember_focus_item(cont_id);
  item_pretty(taken, item_pretty_buf, sizeof item_pretty_buf);
  snprintf(msg, msgcap, "You put %s in the %s.", item_pretty_buf, cont_pretty);
  g_score += 1;
  return 1;
}

static void container_append_open_contents_picks(int room, const char *name,
                                                 const char *qnorm,
                                                 char picks[][MAX_ITEM_LEN],
                                                 int *pn, int exclude_last) {
  int ci, ii, j, n = pn ? *pn : 0;
  if (!pn || n >= DISAMBIG_PICK_MAX) return;
  for (ci = 0; ci < g_container_n; ci++) {
    if (!g_containers[ci].open) continue;
    if (!container_accessible_here(room, ci)) continue;
    for (ii = 0; ii < g_containers[ci].content_n; ii++) {
      if (exclude_last && g_last_focus[0] &&
          str_ieq(g_containers[ci].contents[ii], g_last_focus))
        continue;
      if (!room_item_matches_query(g_containers[ci].contents[ii], name, qnorm))
        continue;
      for (j = 0; j < n; j++)
        if (str_ieq(picks[j], g_containers[ci].contents[ii])) break;
      if (j < n) continue;
      if (n < DISAMBIG_PICK_MAX) {
        copy_capped(picks[n], MAX_ITEM_LEN, g_containers[ci].contents[ii]);
        n++;
      }
    }
  }
  *pn = n;
}

static int container_take_by_query(int room, const char *name, char *msg,
                                   size_t msgcap) {
  char work[256], qnorm[MAX_ITEM_LEN], picks[6][MAX_ITEM_LEN], slug[MAX_ITEM_LEN];
  char pretty[96], ip[96];
  int pn = 0, ordinal = 0, exclude_last = 0, prep, ci, ii;
  prep = parser_prepare_object_query(name, work, sizeof work, &ordinal,
                                     &exclude_last, msg, msgcap);
  if (prep < 0) return 0;
  if (!work[0]) return 0;
  query_norm_underscore(qnorm, sizeof qnorm, work);
  container_append_open_contents_picks(room, work, qnorm, picks, &pn,
                                       exclude_last);
  if (pn == 0) return 0;
  if (pn > 1 && ordinal >= 1 && ordinal <= pn)
    copy_capped(slug, sizeof slug, picks[ordinal - 1]);
  else if (pn == 1)
    copy_capped(slug, sizeof slug, picks[0]);
  else {
    snprintf(msg, msgcap,
             "Which one? (%d matches in open containers — try an ordinal.)", pn);
    return 0;
  }
  for (ci = 0; ci < g_container_n; ci++) {
    if (!g_containers[ci].open) continue;
    if (!container_accessible_here(room, ci)) continue;
    for (ii = 0; ii < g_containers[ci].content_n; ii++) {
      if (!str_ieq(g_containers[ci].contents[ii], slug)) continue;
      if (g_inv_n >= MAX_INV) {
        snprintf(msg, msgcap,
                 "You cannot carry more (%d-item limit). Drop something first.",
                 MAX_INV);
        return 0;
      }
      container_content_remove(ci, slug);
      if (item_is_container(slug))
        container_set_location(slug, AET_CONTAINER_ROOM_INV);
      inv_add(slug);
      item_pretty(g_containers[ci].id, pretty, sizeof pretty);
      item_pretty(slug, ip, sizeof ip);
      snprintf(msg, msgcap, "Taken from %s: %s", pretty, ip);
      remember_focus_item(slug);
      g_score += 5;
      return 1;
    }
  }
  return 0;
}

static void container_examine(int cix, char *msg, size_t msgcap) {
  char pretty[96], body[512];
  const AetContainerRec *c;
  if (cix < 0 || cix >= g_container_n) return;
  c = &g_containers[cix];
  item_pretty(c->id, pretty, sizeof pretty);
  body[0] = '\0';
  if (aet_item_catalog_description_for_slug(c->id))
    snprintf(body, sizeof body, "%s",
             aet_item_catalog_description_for_slug(c->id));
  snprintf(msg, msgcap, "%s%s%s", pretty, body[0] ? "\n\n" : "", body);
  if (c->locked) {
    const char *need = container_unlock_key_slug(c->id);
    if (need) {
      char kp[96];
      item_pretty(need, kp, sizeof kp);
      snprintf(body, sizeof body, "\n\nIt is locked. A %s might fit.", kp);
      strncat(msg, body, msgcap - strlen(msg) - 1);
    } else
      strncat(msg, "\n\nIt is locked.", msgcap - strlen(msg) - 1);
  } else if (c->open) {
    strncat(msg, "\n\nIt is open. ", msgcap - strlen(msg) - 1);
    container_format_contents(cix, msg, msgcap);
  } else {
    if (c->content_n > 0)
      strncat(msg,
              "\n\nIt is closed. You would need to open it to see or reach "
              "what is inside.",
              msgcap - strlen(msg) - 1);
    else
      strncat(msg, "\n\nIt is closed.", msgcap - strlen(msg) - 1);
  }
  remember_focus_item(c->id);
}

static void container_scan_append(char *msg, size_t msgcap, int room) {
  int ci, any = 0;
  char line[320], pretty[96], ip[96];
  for (ci = 0; ci < g_container_n; ci++) {
    const AetContainerRec *c = &g_containers[ci];
    size_t len;
    char parent_pretty[96];
    int pcix;
    if (!container_accessible_here(room, ci)) continue;
    if (container_room_is_nested(c->room)) {
      pcix = container_nested_parent_cix(c->room);
      if (pcix < 0 || pcix >= g_container_n) continue;
      item_pretty(g_containers[pcix].id, parent_pretty, sizeof parent_pretty);
    } else if (c->room != room) {
      continue;
    }
    if (!any) {
      strncat(msg, "\nContainers:\n", msgcap - strlen(msg) - 1);
      any = 1;
    }
    item_pretty(c->id, pretty, sizeof pretty);
    if (container_room_is_nested(c->room)) {
      if (c->locked)
        snprintf(line, sizeof line, "  %s (in %s) — locked closed", pretty,
                 parent_pretty);
      else if (c->open) {
        if (c->content_n == 0)
          snprintf(line, sizeof line, "  %s (in %s) — open, empty", pretty,
                   parent_pretty);
        else if (c->content_n == 1) {
          item_pretty(c->contents[0], ip, sizeof ip);
          snprintf(line, sizeof line, "  %s (in %s) — open; inside: %s", pretty,
                   parent_pretty, ip);
        } else
          snprintf(line, sizeof line, "  %s (in %s) — open; %d items inside",
                   pretty, parent_pretty, c->content_n);
      } else if (c->content_n > 0)
        snprintf(line, sizeof line, "  %s (in %s) — closed (%d item(s) within)",
                 pretty, parent_pretty, c->content_n);
      else
        snprintf(line, sizeof line, "  %s (in %s) — closed", pretty,
                 parent_pretty);
    } else if (c->locked)
      snprintf(line, sizeof line, "  %s — locked closed", pretty);
    else if (c->open) {
      if (c->content_n == 0)
        snprintf(line, sizeof line, "  %s — open, empty", pretty);
      else if (c->content_n == 1) {
        item_pretty(c->contents[0], ip, sizeof ip);
        snprintf(line, sizeof line, "  %s — open; inside: %s", pretty, ip);
      } else
        snprintf(line, sizeof line, "  %s — open; %d items inside", pretty,
                 c->content_n);
    } else if (c->content_n > 0)
      snprintf(line, sizeof line, "  %s — closed (%d item(s) within)", pretty,
               c->content_n);
    else
      snprintf(line, sizeof line, "  %s — closed", pretty);
    len = strlen(msg);
    if (len + 2 < msgcap) strncat(msg, line, msgcap - len - 1);
    if (len + 2 < msgcap) strncat(msg, "\n", msgcap - strlen(msg) - 1);
  }
}

static int container_hint_closed_item(int room, const char *work,
                                      const char *qnorm, char *msg,
                                      size_t msgcap) {
  int ci, ii;
  char cp[96], ip[96];
  for (ci = 0; ci < g_container_n; ci++) {
    const AetContainerRec *c = &g_containers[ci];
    if (c->open) continue;
    if (!container_accessible_here(room, ci)) continue;
    for (ii = 0; ii < c->content_n; ii++) {
      if (!room_item_matches_query(c->contents[ii], work, qnorm)) continue;
      item_pretty(c->id, cp, sizeof cp);
      item_pretty(c->contents[ii], ip, sizeof ip);
      snprintf(msg, msgcap,
               "The %s is inside the closed %s. Try  open %s  first.", ip, cp,
               c->id);
      return 1;
    }
  }
  return 0;
}

static void body_append(char *body, size_t cap, const char *fmt, ...) {
  size_t len;
  va_list ap;
  if (!body || cap == 0) return;
  len = strlen(body);
  if (len >= cap - 1) {
    body[cap - 1] = '\0';
    return;
  }
  va_start(ap, fmt);
  (void)vsnprintf(body + len, cap - len, fmt, ap);
  va_end(ap);
  body[cap - 1] = '\0';
}

static size_t mod_replace_token_pass(const char *src, char *dst, size_t dstcap,
                                     const char *tok, const char *rep) {
  size_t tl = strlen(tok);
  const char *p = src;
  char *d = dst;
  char *dend = dst + dstcap - 1;
  const char *r;
  if (!rep) rep = "";
  while (*p && d < dend) {
    if (tl > 0 && strncmp(p, tok, tl) == 0) {
      for (r = rep; *r && d < dend; r++) *d++ = *r;
      p += tl;
    } else
      *d++ = *p++;
  }
  *d = '\0';
  return (size_t)(d - dst);
}

static void expand_scene_mod_text(const char *src, char *dst, size_t dstcap) {
  char a[4096], b[4096];
  const char *slug, *title, *reg;
  if (!dst || dstcap < 2) return;
  dst[0] = '\0';
  if (!src || !src[0]) return;
  if (strlen(src) >= sizeof a) {
    (void)snprintf(dst, dstcap, "%.*s", (int)(dstcap - 1), src);
    return;
  }
  if (g_room < 0 || g_room >= WORLD_ROOM_COUNT) {
    slug = "?";
    title = "(unknown)";
    reg = "(unknown)";
  } else {
    slug = world_slug(g_room);
    title = resolve_world_title(g_room);
    reg = world_region(g_room);
    if (!slug) slug = "";
    if (!title) title = "";
    if (!reg || !reg[0]) reg = "(unspecified)";
  }
  memcpy(a, src, strlen(src) + 1);
  mod_replace_token_pass(a, b, sizeof b, "%ROOMTITLE%", title);
  mod_replace_token_pass(b, a, sizeof a, "%ROOMSLUG%", slug);
  mod_replace_token_pass(a, b, sizeof b, "%ROOM%", slug);
  mod_replace_token_pass(b, a, sizeof a, "%REGION%", reg);
  (void)snprintf(dst, dstcap, "%s", a);
}

static const char *const LIGHT_SOURCE_IDS[] = {
    "torch",         "lit_torch",   "lantern",       "candle",
    "candlestick",   "matchbox",    "matches",       "flashlight",
    "glowstone",     "ember",       "firefly_jar",   "magic_light",
    "brazier",       "oil_lamp",    "moon_orb",      "phosphor_moss",
    NULL};

static int item_id_is_light(const char *id) {
  int s;
  if (!id || !id[0]) return 0;
  for (s = 0; LIGHT_SOURCE_IDS[s]; s++)
    if (str_ieq(id, LIGHT_SOURCE_IDS[s])) return 1;
  return 0;
}

static int player_has_light_source(void) {
  int i;
  for (i = 0; i < g_inv_n; i++)
    if (item_id_is_light(g_inv[i])) return 1;
  return 0;
}

static int room_too_dark_to_see(void) {
  return world_room_is_dark(g_room) && !player_has_light_source();
}

static int inv_has(const char *name) {
  int i;
  for (i = 0; i < g_inv_n; i++)
    if (str_ieq(g_inv[i], name)) return 1;
  return 0;
}

static int inv_count(const char *name) {
  int i, n = 0;
  if (!name || !name[0]) return 0;
  for (i = 0; i < g_inv_n; i++)
    if (str_ieq(g_inv[i], name)) n++;
  return n;
}

#ifdef AETER_MINIGAMES
static const char *game_mgt_weather_string(void);
static void inv_add(const char *name);
static int inv_remove_exact_one(const char *name);

static int resolve_visible_item(const char *raw, char *id_out, size_t idcap,
                                char *msg, size_t msgcap);
static int game_read_resolve(const char *item_id, MgtReadDocument *doc);

static int g_mgt_ready;
static MgtPersistentState g_mgt_quicksave_profile;
static int g_mgt_quicksave_valid;

static void game_fill_sim(MgtGameSim *sim);

static void game_mgt_ensure_profile(void) {
  MgtPersistentState *pst = mgt_host_state();
  MgtGameSim sim;

  if (g_mgt_ready) {
    game_fill_sim(&sim);
    mgt_sync_from_world(pst, &sim);
    return;
  }
  if (g_mgt_quicksave_valid) {
    *pst = g_mgt_quicksave_profile;
  } else {
    memset(&sim, 0, sizeof sim);
    if (!mgt_harness_load(&sim, pst, NULL))
      mgt_profile_fresh_adventure(pst, (unsigned)time(NULL) | 1u);
  }
  game_fill_sim(&sim);
  mgt_sync_from_world(pst, &sim);
}

static void game_mgt_capture_profile(MgtPersistentState *out) {
  if (!out) return;
  game_mgt_ensure_profile();
  *out = *mgt_host_state();
  g_mgt_quicksave_profile = *out;
  g_mgt_quicksave_valid = 1;
}

static void game_mgt_bootstrap_profile(void) { game_mgt_ensure_profile(); }

static void game_fill_sim(MgtGameSim *sim) {
  AetPcSave pc;
  const char *slug;
  int i;
  if (!sim) return;
  memset(sim, 0, sizeof *sim);
  pc_capture(&pc);
  sim->coins = g_coins;
  sim->craft_proficiency = g_craft_proficiency;
  sim->cha = pc.cha;
  sim->wis = pc.wis;
  sim->intl = pc.intl;
  sim->adventure_turn = g_turns;
  sim->inv_n = g_inv_n;
  if (sim->inv_n > MGT_SIM_INV_MAX) sim->inv_n = MGT_SIM_INV_MAX;
  for (i = 0; i < sim->inv_n; i++) {
    snprintf(sim->inv[i], sizeof sim->inv[i], "%s", g_inv[i]);
  }
  slug = world_slug(g_room);
  if (slug)
    snprintf(sim->room_slug, sizeof sim->room_slug, "%s", slug);
  else
    sim->room_slug[0] = '\0';
  {
    const char *wx = game_mgt_weather_string();
    snprintf(sim->weather, sizeof sim->weather, "%s", wx ? wx : "clear");
  }
}

static void game_apply_sim(const MgtGameSim *sim) {
  int i;
  if (!sim) return;
  g_coins = sim->coins;
  if (sim->craft_proficiency > g_craft_proficiency)
    g_craft_proficiency = sim->craft_proficiency;
  if (g_craft_proficiency > 100) g_craft_proficiency = 100;
  g_inv_n = 0;
  for (i = 0; i < sim->inv_n && i < MGT_SIM_INV_MAX; i++) inv_add(sim->inv[i]);
}

static void game_mgt_sync_from(MgtPersistentState *st, void *ctx) {
  MgtGameSim sim;
  (void)ctx;
  if (!st) return;
  game_fill_sim(&sim);
  mgt_sync_from_world(st, &sim);
}

static void game_mgt_give_item(const char *id, void *ctx) {
  (void)ctx;
  if (id && id[0]) inv_add(id);
}

static void game_mgt_sync_to(const MgtPersistentState *st_in, void *ctx) {
  MgtGameSim sim;
  (void)ctx;
  if (!st_in) return;
  game_fill_sim(&sim);
  mgt_sync_to_world((MgtPersistentState *)st_in, &sim);
  game_apply_sim(&sim);
}

static void game_mgt_redraw(void *ctx) {
  (void)ctx;
  return_to_game_screen();
}

static int try_minigame(const char *id, char *msg, size_t msgcap) {
  MgtPersistentState *st;
  MgtGameSim sim;
  int rc;
  if (!id || !id[0]) return 0;
  if (!mgt_registry_find(id)) {
    if (msg && msgcap)
      snprintf(msg, msgcap, "That activity is not available.");
    return 0;
  }
  if (!g_mgt_ready) {
    aet_minigames_register_sync(game_mgt_sync_from, game_mgt_sync_to,
                                game_mgt_redraw, NULL);
    aet_minigames_register_give(game_mgt_give_item, NULL);
    mgt_read_register_resolver(game_read_resolve);
    game_mgt_bootstrap_profile();
    g_mgt_ready = 1;
  }
  if (aet_autotest()) {
#if defined(_WIN32)
    _putenv("MGT_AUTOTEST=1");
    if (!strcmp(id, "lockpick"))
      _putenv("MGT_AUTOTEST_SCRIPT=lock_win");
    else if (!strcmp(id, "hunting"))
      _putenv("MGT_AUTOTEST_SCRIPT=hunt_win");
    else
      _putenv("MGT_AUTOTEST_SCRIPT=");
#else
    setenv("MGT_AUTOTEST", "1", 1);
    if (!strcmp(id, "lockpick"))
      setenv("MGT_AUTOTEST_SCRIPT", "lock_win", 1);
    else if (!strcmp(id, "hunting"))
      setenv("MGT_AUTOTEST_SCRIPT", "hunt_win", 1);
    else
      unsetenv("MGT_AUTOTEST_SCRIPT");
#endif
  }
  rc = aet_minigame_takeover(id);
  if (aet_autotest()) {
#if defined(_WIN32)
    _putenv("MGT_AUTOTEST_SCRIPT=");
#else
    unsetenv("MGT_AUTOTEST_SCRIPT");
#endif
  }
  if (rc == (int)MGT_HOST_ABORT) {
    if (msg && msgcap)
      snprintf(msg, msgcap, "Could not start that activity.");
    return 0;
  }
  st = mgt_host_state();
  game_fill_sim(&sim);
  g_mgt_quicksave_profile = *st;
  g_mgt_quicksave_valid = 1;
  mgt_harness_save(&sim, st, NULL);
  if (msg && msgcap) {
    if (st && st->last_banner[0])
      snprintf(msg, msgcap, "%s", st->last_banner);
    else
      snprintf(msg, msgcap, "You finish and return to the adventure.");
  }
  return 1;
}
#else
static int try_minigame(const char *id, char *msg, size_t msgcap) {
  (void)id;
  if (msg && msgcap) msg[0] = '\0';
  return 0;
}
#endif

static int room_can_fish(void) {
  const char *slug = world_slug(g_room);
  if (room_has_visible_item(g_room, "fishing_spot")) return 1;
  if (slug && (!strcmp(slug, "pond") || !strcmp(slug, "fishing_piers") ||
               !strcmp(slug, "river_shore") || !strcmp(slug, "hollowridge_docks") ||
               !strcmp(slug, "stream") || !strcmp(slug, "ferry_crossing")))
    return 1;
  return 0;
}

static int room_can_farm(void) {
  const char *slug = world_slug(g_room);
  return slug && !strcmp(slug, "farm");
}

static int room_can_cook(void) {
  const char *slug = world_slug(g_room);
  if (!slug) return 0;
  if (strstr(slug, "kitchen") != NULL) return 1;
  return !strcmp(slug, "roasting_room");
}

static int room_can_gamble(void) {
  const char *slug = world_slug(g_room);
  return slug && (!strcmp(slug, "tavern_common_room") ||
                  !strcmp(slug, "the_hidden_gem_tavern") ||
                  !strcmp(slug, "cozy_booths"));
}

static void meter_bar_pct(char *out, size_t cap, int pct) {
  int filled, i;
  if (!out || cap < 12) return;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  filled = (pct * 10 + 99) / 100;
  for (i = 0; i < 10; i++) out[i] = (i < filled) ? '#' : '-';
  out[10] = '\0';
}

static int room_floor_has_id(const char *id) {
  int i;
  if (!id || !id[0]) return 0;
  for (i = 0; i < g_room_item_n[g_room]; i++)
    if (str_ieq(g_room_items[g_room][i], id)) return 1;
  return 0;
}

static void cmd_play_piano(char *msg, size_t msgcap) {
  if (!msg || msgcap < 32) return;
  if (!room_has_visible_item(g_room, "tavern_piano")) {
    snprintf(msg, msgcap, "There is no piano here to play.");
    return;
  }
#ifdef AETER_MINIGAMES
  if (try_minigame("piano", msg, msgcap)) return;
#endif
  snprintf(
      msg, msgcap,
      "You pick out a hesitant melody on the worn keys. A few regulars glance "
      "over, then return to their drinks.");
}

static int topic_mentions_bucket(const char *s) {
  if (!s || !s[0]) return 0;
  return strstr(s, "bucket") != NULL;
}

static void player_heal(int amt) {
  long sum;
  if (amt <= 0 || g_health >= g_max_health) return;
  sum = (long)g_health + amt;
  g_health = sum > g_max_health ? g_max_health : (int)sum;
}

#define CURRENCY_BRONZE_VALUE 10
#define CURRENCY_SILVER_VALUE 50
#define CURRENCY_GOLD_VALUE 100

static void currency_split(int amount, int *gold, int *silver, int *bronze,
                           int *copper) {
  int v = amount < 0 ? 0 : amount;
  if (gold) *gold = v / CURRENCY_GOLD_VALUE;
  v %= CURRENCY_GOLD_VALUE;
  if (silver) *silver = v / CURRENCY_SILVER_VALUE;
  v %= CURRENCY_SILVER_VALUE;
  if (bronze) *bronze = v / CURRENCY_BRONZE_VALUE;
  if (copper) *copper = v % CURRENCY_BRONZE_VALUE;
}

static void currency_format_long(int amount, char *out, size_t cap) {
  char parts[4][32];
  int gold, silver, bronze, copper;
  int n = 0, i;
  if (!out || cap < 2) return;
  currency_split(amount, &gold, &silver, &bronze, &copper);
  if (gold > 0) snprintf(parts[n++], sizeof parts[0], "%d gold", gold);
  if (silver > 0) snprintf(parts[n++], sizeof parts[0], "%d silver", silver);
  if (bronze > 0) snprintf(parts[n++], sizeof parts[0], "%d bronze", bronze);
  if (copper > 0 || n == 0)
    snprintf(parts[n++], sizeof parts[0], "%d copper", copper);
  out[0] = '\0';
  for (i = 0; i < n; i++) {
    if (i > 0) strncat(out, ", ", cap - strlen(out) - 1);
    strncat(out, parts[i], cap - strlen(out) - 1);
  }
}

static void currency_format_compact(int amount, char *out, size_t cap) {
  char parts[4][16];
  int gold, silver, bronze, copper;
  int n = 0, i;
  if (!out || cap < 2) return;
  currency_split(amount, &gold, &silver, &bronze, &copper);
  if (gold > 0) snprintf(parts[n++], sizeof parts[0], "%dg", gold);
  if (silver > 0) snprintf(parts[n++], sizeof parts[0], "%ds", silver);
  if (bronze > 0) snprintf(parts[n++], sizeof parts[0], "%db", bronze);
  if (copper > 0 || n == 0)
    snprintf(parts[n++], sizeof parts[0], "%dc", copper);
  out[0] = '\0';
  for (i = 0; i < n; i++) {
    if (i > 0) strncat(out, " ", cap - strlen(out) - 1);
    strncat(out, parts[i], cap - strlen(out) - 1);
  }
}

static int tender_pickup_coins(const char *item_id) {
  if (!item_id || !item_id[0]) return 0;
  if (str_ieq(item_id, "copper_coin")) return 1;
  if (str_ieq(item_id, "bronze_coin")) return CURRENCY_BRONZE_VALUE;
  if (str_ieq(item_id, "silver_coin")) return CURRENCY_SILVER_VALUE;
  if (str_ieq(item_id, "gold_coin")) return CURRENCY_GOLD_VALUE;
  if (str_ieq(item_id, "hidden_cash")) return 12;
  if (str_ieq(item_id, "buried_coin")) return 6;
  if (str_ieq(item_id, "ancient_coin")) return 15;
  return 0;
}

static void inv_add(const char *name) {
  size_t n;
  if (g_inv_n >= MAX_INV || !name) return;
  n = strlen(name);
  if (n >= (size_t)MAX_ITEM_LEN) n = (size_t)MAX_ITEM_LEN - 1;
  memcpy(g_inv[g_inv_n], name, n);
  g_inv[g_inv_n][n] = '\0';
  g_inv_n++;
}

static void strip_trailing_space(char *s) {
  char *e = s + strlen(s);
  while (e > s && isspace((unsigned char)e[-1])) *--e = '\0';
}

static void chomp_line(char *s) {
  size_t n = strlen(s);
  while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

static void diag_clear(void) {
  g_diag_head = 0;
  g_diag_count = 0;
}

static void causal_clear(void) {
  g_causal_head = 0;
  g_causal_count = 0;
}

static void trade_history_clear(void) {
  g_trade_head = 0;
  g_trade_count = 0;
}

static void rel_hist_clear(void) {
  g_rel_hist_head = 0;
  g_rel_hist_count = 0;
}

static void rel_hist_push(const char *slug, const char *act, int df, int dr,
                          int dp) {
  RelHistRow *row;
  if (!slug || !slug[0] || !act || !act[0]) return;
  if (df == 0 && dr == 0 && dp == 0) return;
  row = &g_rel_hist[g_rel_hist_head];
  row->turn = g_turns;
  snprintf(row->slug, sizeof row->slug, "%s", slug);
  snprintf(row->act, sizeof row->act, "%s", act);
  if (df < -99) df = -99;
  if (df > 99) df = 99;
  if (dr < -99) dr = -99;
  if (dr > 99) dr = 99;
  if (dp < -99) dp = -99;
  if (dp > 99) dp = 99;
  row->df = (signed char)df;
  row->dr = (signed char)dr;
  row->dp = (signed char)dp;
  g_rel_hist_head = (g_rel_hist_head + 1) % REL_HIST_RING;
  if (g_rel_hist_count < REL_HIST_RING) g_rel_hist_count++;
}

static void rel_hist_load_row(int turn, const char *slug, const char *act,
                             int df, int dr, int dp) {
  RelHistRow *row;
  if (!slug || !slug[0] || !act || !act[0]) return;
  if (df == 0 && dr == 0 && dp == 0) return;
  row = &g_rel_hist[g_rel_hist_head];
  row->turn = turn >= 0 ? turn : 0;
  snprintf(row->slug, sizeof row->slug, "%s", slug);
  snprintf(row->act, sizeof row->act, "%s", act);
  row->df = (signed char)df;
  row->dr = (signed char)dr;
  row->dp = (signed char)dp;
  g_rel_hist_head = (g_rel_hist_head + 1) % REL_HIST_RING;
  if (g_rel_hist_count < REL_HIST_RING) g_rel_hist_count++;
}

static void barter_clear(void) {
  g_barter_mode = BARTER_NONE;
  g_barter_price = 0;
  g_barter_list_price = 0;
  g_barter_expire_turn = 0;
  g_barter_merchant[0] = '\0';
  g_barter_item[0] = '\0';
}

static void barter_sanitize_loaded(void) {
  if (g_barter_mode != BARTER_NONE && g_barter_mode != BARTER_BUY &&
      g_barter_mode != BARTER_SELL) {
    barter_clear();
    return;
  }
  if (g_barter_mode == BARTER_NONE) return;
  if (!g_barter_merchant[0] || !g_barter_item[0] ||
      !aet_merchant_trades(g_barter_merchant) ||
      g_barter_expire_turn < g_turns) {
    barter_clear();
  }
}

static void trade_history_push_row(const char *row) {
  size_t n;
  const char *src = row ? row : "";
  n = strnlen(src, TRADE_W - 1);
  memcpy(g_trade_ring[g_trade_head], src, n);
  g_trade_ring[g_trade_head][n] = '\0';
  g_trade_head = (g_trade_head + 1) % TRADE_RING;
  if (g_trade_count < TRADE_RING) g_trade_count++;
}

static void copy_capped(char *dst, size_t cap, const char *src) {
  size_t n;
  if (!dst || cap == 0) return;
  if (!src) src = "";
  n = strnlen(src, cap - 1);
  memcpy(dst, src, n);
  dst[n] = '\0';
}

static void cmd_hist_clear(void) {
  g_cmd_hist_count = 0;
  g_cmd_hist_head = 0;
}

static void cmd_hist_push(const char *line) {
  size_t n;
  int prev;
  if (!line || !line[0]) return;
  if (!strcmp(line, "g") || !strcmp(line, "again") || !strcmp(line, "repeat"))
    return;
  if (g_cmd_hist_count > 0) {
    prev = (g_cmd_hist_head - 1 + CMD_HIST_RING) % CMD_HIST_RING;
    if (str_ieq(g_cmd_hist[prev], line)) return;
  }
  n = strnlen(line, INPUT_LINE_MAX - 1);
  memcpy(g_cmd_hist[g_cmd_hist_head], line, n);
  g_cmd_hist[g_cmd_hist_head][n] = '\0';
  g_cmd_hist_head = (g_cmd_hist_head + 1) % CMD_HIST_RING;
  if (g_cmd_hist_count < CMD_HIST_RING) g_cmd_hist_count++;
}

static int cmd_hist_get_from_latest(int n, char *out, size_t cap) {
  int idx;
  if (!out || cap < 2 || n < 1 || n > g_cmd_hist_count) return 0;
  idx = (g_cmd_hist_head - n + CMD_HIST_RING) % CMD_HIST_RING;
  copy_capped(out, cap, g_cmd_hist[idx]);
  return 1;
}

static void format_cmd_history_body(char *body, size_t cap) {
  int i, start;
  if (!body || cap < 64) return;
  snprintf(body, cap,
           "Command history\n\n"
           "Recent inputs (newest last). Re-run with  history <n>  or  !<n>  "
           "(e.g. !3).\n"
           "  again | g | repeat  — last full chain (including ; / then).\n\n");
  if (g_cmd_hist_count == 0) {
    body_append(body, cap, "  (empty — enter a few commands first.)\n");
    return;
  }
  start = (g_cmd_hist_head - g_cmd_hist_count + CMD_HIST_RING) % CMD_HIST_RING;
  for (i = 0; i < g_cmd_hist_count; i++) {
    int idx = (start + i) % CMD_HIST_RING;
    body_append(body, cap, "  %2d. %s\n", i + 1, g_cmd_hist[idx]);
  }
}

static void trade_history_push(const char *kind, const char *merchant,
                               const char *item, int amount, int list_price,
                               int coin_total) {
  char scratch[TRADE_W];
  char whatb[96];
  char amountb[48], listb[48], purseb[48];
  const char *k = kind ? kind : "";
  const char *who = merchant && merchant[0] ? merchant : "merchant";
  const char *what = item && item[0] ? item : "item";
  item_pretty(what, whatb, sizeof whatb);
  if (amount < 0) amount = 0;
  if (list_price < 0) list_price = amount;
  currency_format_long(amount, amountb, sizeof amountb);
  currency_format_long(list_price, listb, sizeof listb);
  currency_format_long(coin_total, purseb, sizeof purseb);
  if (str_ieq(k, "buy")) {
    if (list_price != amount)
      (void)snprintf(scratch, sizeof scratch,
                     "T%04d [buy] bought %s from %s for %s (list %s) -> purse %s",
                     g_turns, whatb, who, amountb, listb, purseb);
    else
      (void)snprintf(scratch, sizeof scratch,
                     "T%04d [buy] bought %s from %s for %s -> purse %s", g_turns,
                     whatb, who, amountb, purseb);
  } else if (str_ieq(k, "sell")) {
    if (list_price != amount)
      (void)snprintf(scratch, sizeof scratch,
                     "T%04d [sell] sold %s to %s for %s (list %s) -> purse %s",
                     g_turns, whatb, who, amountb, listb, purseb);
    else
      (void)snprintf(scratch, sizeof scratch,
                     "T%04d [sell] sold %s to %s for %s -> purse %s", g_turns,
                     whatb, who, amountb, purseb);
  } else {
    (void)snprintf(scratch, sizeof scratch,
                   "T%04d [trade] %s with %s for %s -> purse %s", g_turns, whatb,
                   who, amountb, purseb);
  }
  scratch[sizeof scratch - 1] = '\0';
  trade_history_push_row(scratch);
}

static void diag_push(const char *tag, const char *detail) {
  char scratch[DIAG_W];
  char trunc[136];
  const char *d = detail ? detail : "";
  const char *t = tag ? tag : "?";
  if (strlen(d) > sizeof trunc - 1) {
    memcpy(trunc, d, sizeof trunc - 1);
    trunc[sizeof trunc - 1] = '\0';
    d = trunc;
  }
  (void)snprintf(scratch, sizeof scratch, "%s: %s", t, d);
  scratch[sizeof scratch - 1] = '\0';
  memcpy(g_diag_ring[g_diag_head], scratch, DIAG_W);
  g_diag_ring[g_diag_head][DIAG_W - 1] = '\0';
  g_diag_head = (g_diag_head + 1) % DIAG_RING;
  if (g_diag_count < DIAG_RING) g_diag_count++;
}

static void causal_push(const char *tag, const char *detail) {
  char scratch[CAUSAL_W];
  const char *d = detail ? detail : "";
  const char *t = tag ? tag : "?";
  (void)snprintf(scratch, sizeof scratch, "T%04d [%s] %s", g_turns, t, d);
  scratch[sizeof scratch - 1] = '\0';
  memcpy(g_causal_ring[g_causal_head], scratch, CAUSAL_W);
  g_causal_ring[g_causal_head][CAUSAL_W - 1] = '\0';
  g_causal_head = (g_causal_head + 1) % CAUSAL_RING;
  if (g_causal_count < CAUSAL_RING) g_causal_count++;
}

static void format_causality_body(char *body, size_t cap, const char *term) {
  int i, start, shown = 0;
  char q[96];
  if (!body || cap < 64) return;
  body[0] = '\0';
  if (term && term[0]) {
    size_t n = strnlen(term, sizeof q - 1);
    size_t k;
    memcpy(q, term, n);
    q[n] = '\0';
    for (k = 0; q[k]; k++) q[k] = (char)tolower((unsigned char)q[k]);
    while (n > 0 && q[n - 1] == ' ') q[--n] = '\0';
  } else
    q[0] = '\0';
  (void)snprintf(
      body, cap,
      "=== CAUSALITY TRACE ===\n\n"
      "Recent cause/effect records (newest last; max %d).\n"
      "%s%s%s\n\n",
      CAUSAL_RING, q[0] ? "Filter: \"" : "", q[0] ? q : "", q[0] ? "\"" : "");
  if (g_causal_count == 0) {
    strncat(body, "  (none yet — play a few actions first.)\n",
            cap - strlen(body) - 1);
    return;
  }
  start = (g_causal_head - g_causal_count + CAUSAL_RING) % CAUSAL_RING;
  for (i = 0; i < g_causal_count; i++) {
    int idx = (start + i) % CAUSAL_RING;
    const char *row = g_causal_ring[idx];
    char row_lc[CAUSAL_W];
    char out[CAUSAL_W + 8];
    size_t n = strnlen(row, sizeof row_lc - 1);
    size_t k;
    memcpy(row_lc, row, n);
    row_lc[n] = '\0';
    for (k = 0; row_lc[k]; k++) row_lc[k] = (char)tolower((unsigned char)row_lc[k]);
    if (q[0] && strstr(row_lc, q) == NULL) continue;
    (void)snprintf(out, sizeof out, "  • %s\n", row);
    strncat(body, out, cap - strlen(body) - 1);
    shown++;
  }
  if (shown == 0)
    strncat(body, "  (no events matched this filter)\n",
            cap - strlen(body) - 1);
}

static void format_trade_history_body(char *body, size_t cap) {
  int i, start, buys = 0, sells = 0, recent_n;
  if (!body || cap < 64) return;
  body[0] = '\0';
  (void)snprintf(body, cap,
                 "=== TRADE HISTORY ===\n\n"
                 "Completed merchant transactions (saved with your game; max %d).\n\n",
                 TRADE_RING);
  if (g_trade_count == 0) {
    strncat(body, "  (none yet — buy or sell with a merchant first.)\n",
            cap - strlen(body) - 1);
    return;
  }
  start = (g_trade_head - g_trade_count + TRADE_RING) % TRADE_RING;
  for (i = 0; i < g_trade_count; i++) {
    int idx = (start + i) % TRADE_RING;
    const char *row = g_trade_ring[idx];
    if (strstr(row, "[buy]")) buys++;
    if (strstr(row, "[sell]")) sells++;
  }
  body_append(body, cap,
              "Summary: %d transaction%s (%d buy%s, %d sell%s)\n\n",
              g_trade_count, g_trade_count == 1 ? "" : "s", buys, buys == 1 ? "" : "s",
              sells, sells == 1 ? "" : "s");
  recent_n = g_trade_count < 5 ? g_trade_count : 5;
  body_append(body, cap, "Recent (newest first)\n");
  for (i = g_trade_count - 1; i >= g_trade_count - recent_n; i--) {
    int idx = (start + i) % TRADE_RING;
    char out[TRADE_W + 8];
    (void)snprintf(out, sizeof out, "  • %s\n", g_trade_ring[idx]);
    strncat(body, out, cap - strlen(body) - 1);
  }
  if (g_trade_count > recent_n) {
    body_append(body, cap, "\nFull ledger (oldest → newest)\n");
    for (i = 0; i < g_trade_count; i++) {
      int idx = (start + i) % TRADE_RING;
      char out[TRADE_W + 8];
      if (strlen(body) + sizeof out + 4 >= cap) {
        body_append(body, cap, "  ... ledger truncated (screen cap).\n");
        break;
      }
      (void)snprintf(out, sizeof out, "  • %s\n", g_trade_ring[idx]);
      strncat(body, out, cap - strlen(body) - 1);
    }
  }
}

static void format_causality_turn_body(char *body, size_t cap, int turn_value) {
  int i, start, shown = 0;
  char needle[24];
  if (!body || cap < 64) return;
  (void)snprintf(needle, sizeof needle, "T%04d ", turn_value);
  body[0] = '\0';
  (void)snprintf(body, cap,
                 "=== CAUSALITY TURN VIEW ===\n\nEvents for turn %d:\n\n",
                 turn_value);
  if (g_causal_count == 0) {
    strncat(body, "  (none yet)\n", cap - strlen(body) - 1);
    return;
  }
  start = (g_causal_head - g_causal_count + CAUSAL_RING) % CAUSAL_RING;
  for (i = 0; i < g_causal_count; i++) {
    int idx = (start + i) % CAUSAL_RING;
    const char *row = g_causal_ring[idx];
    char out[CAUSAL_W + 8];
    if (strncmp(row, needle, strlen(needle)) != 0) continue;
    (void)snprintf(out, sizeof out, "  • %s\n", row);
    strncat(body, out, cap - strlen(body) - 1);
    shown++;
  }
  if (shown == 0)
    strncat(body, "  (no events recorded for that turn)\n",
            cap - strlen(body) - 1);
}

static void format_causality_explain_body(char *body, size_t cap) {
  const char *block = causal_latest_row_matching("[move-blocked]");
  const char *unknown = causal_latest_row_matching("[unknown-command]");
  const char *social = causal_latest_row_matching("[talk");
  const char *speech = causal_latest_row_matching("[say");
  const char *save = causal_latest_row_matching("[save");
  const char *load = causal_latest_row_matching("[load");
  if (!body || cap < 64) return;
  body[0] = '\0';
  strncat(body, "=== CAUSALITY EXPLAINER ===\n\n", cap - strlen(body) - 1);
  if (block) {
    strncat(body, "Primary blocker:\n  ", cap - strlen(body) - 1);
    strncat(body, block, cap - strlen(body) - 1);
    strncat(body, "\n\nAction: try  why blocked  or  lockcheck.\n\n",
            cap - strlen(body) - 1);
  } else if (unknown) {
    strncat(body, "Parser fallback:\n  ", cap - strlen(body) - 1);
    strncat(body, unknown, cap - strlen(body) - 1);
    strncat(body, "\n\nAction: try  causality parser  and  help.\n\n",
            cap - strlen(body) - 1);
  } else if (save || load) {
    strncat(body, "Persistence state:\n  ", cap - strlen(body) - 1);
    strncat(body, load ? load : save, cap - strlen(body) - 1);
    strncat(body, "\n\nAction: try  saves  and  mods doctor.\n\n",
            cap - strlen(body) - 1);
  } else {
    strncat(body, "No hard blocker detected in recent events.\n\n",
            cap - strlen(body) - 1);
  }
  if (social || speech) {
    strncat(body, "Recent social signal:\n  ", cap - strlen(body) - 1);
    strncat(body, social ? social : speech, cap - strlen(body) - 1);
    strncat(body, "\n", cap - strlen(body) - 1);
  }
}

static int causal_row_has_category(const char *row_lc, const char *cat) {
  if (!row_lc || !cat || !cat[0]) return 1;
  if (!strcmp(cat, "movement"))
    return strstr(row_lc, "[move") != NULL;
  if (!strcmp(cat, "social"))
    return strstr(row_lc, "[talk") != NULL || strstr(row_lc, "[say") != NULL;
  if (!strcmp(cat, "save"))
    return strstr(row_lc, "[save") != NULL || strstr(row_lc, "[load") != NULL;
  if (!strcmp(cat, "parser"))
    return strstr(row_lc, "[unknown-command]") != NULL;
  return 1;
}

static void format_causality_recent_body(char *body, size_t cap) {
  int i;
  const char *cats[] = {"movement", "social", "save", "parser", NULL};
  if (!body || cap < 64) return;
  body[0] = '\0';
  strncat(body,
          "=== CAUSALITY SUMMARY (V2-LITE) ===\n\n"
          "Most recent notable event by category:\n\n",
          cap - strlen(body) - 1);
  for (i = 0; cats[i]; i++) {
    int j, start;
    int found = 0;
    char line[300];
    if (g_causal_count <= 0) break;
    start = (g_causal_head - g_causal_count + CAUSAL_RING) % CAUSAL_RING;
    for (j = g_causal_count - 1; j >= 0; j--) {
      int idx = (start + j) % CAUSAL_RING;
      const char *row = g_causal_ring[idx];
      char row_lc[CAUSAL_W];
      size_t n = strnlen(row, sizeof row_lc - 1);
      size_t k;
      memcpy(row_lc, row, n);
      row_lc[n] = '\0';
      for (k = 0; row_lc[k]; k++)
        row_lc[k] = (char)tolower((unsigned char)row_lc[k]);
      if (!causal_row_has_category(row_lc, cats[i])) continue;
      (void)snprintf(line, sizeof line, "  %-8s %s\n", cats[i], row);
      strncat(body, line, cap - strlen(body) - 1);
      found = 1;
      break;
    }
    if (!found) {
      (void)snprintf(line, sizeof line, "  %-8s (none)\n", cats[i]);
      strncat(body, line, cap - strlen(body) - 1);
    }
  }
}

static void format_why_blocked_body(char *body, size_t cap) {
  int j, start;
  if (!body || cap < 64) return;
  body[0] = '\0';
  strncat(body, "=== WHY BLOCKED ===\n\n", cap - strlen(body) - 1);
  if (g_causal_count <= 0) {
    strncat(body, "No causal records yet.\n", cap - strlen(body) - 1);
    return;
  }
  start = (g_causal_head - g_causal_count + CAUSAL_RING) % CAUSAL_RING;
  for (j = g_causal_count - 1; j >= 0; j--) {
    int idx = (start + j) % CAUSAL_RING;
    const char *row = g_causal_ring[idx];
    if (strstr(row, "[move-blocked]") != NULL) {
      strncat(body, "Most recent movement blocker:\n\n", cap - strlen(body) - 1);
      strncat(body, "  ", cap - strlen(body) - 1);
      strncat(body, row, cap - strlen(body) - 1);
      strncat(body,
              "\n\nTry: lockcheck, route <place>, nearby locked, or acquire the required tool/key.",
              cap - strlen(body) - 1);
      return;
    }
  }
  strncat(body, "No recent movement-blocked event found.\n",
          cap - strlen(body) - 1);
}

static const char *causal_latest_row_matching(const char *needle) {
  static char hit[CAUSAL_W];
  int j, start;
  if (!needle || !needle[0] || g_causal_count <= 0) return NULL;
  start = (g_causal_head - g_causal_count + CAUSAL_RING) % CAUSAL_RING;
  for (j = g_causal_count - 1; j >= 0; j--) {
    int idx = (start + j) % CAUSAL_RING;
    const char *row = g_causal_ring[idx];
    if (strstr(row, needle) == NULL) continue;
    snprintf(hit, sizeof hit, "%s", row);
    return hit;
  }
  return NULL;
}

static void causal_row_compact(const char *row, char *out, size_t cap) {
  const char *p;
  if (!out || cap < 2) return;
  out[0] = '\0';
  if (!row || !row[0]) {
    snprintf(out, cap, "none");
    return;
  }
  p = strstr(row, "] ");
  if (p && p[2]) p += 2;
  else p = row;
  snprintf(out, cap, "%s", p);
}

static void append_causal_status_overlay(char *body, size_t cap) {
  char b[96], p[96], s[96], sv[96];
  causal_row_compact(causal_latest_row_matching("[move-blocked]"), b, sizeof b);
  causal_row_compact(causal_latest_row_matching("[unknown-command]"), p, sizeof p);
  if (!strcmp(p, "none"))
    causal_row_compact(causal_latest_row_matching("[intent]"), p, sizeof p);
  causal_row_compact(causal_latest_row_matching("[talk"), s, sizeof s);
  if (!strcmp(s, "none"))
    causal_row_compact(causal_latest_row_matching("[say"), s, sizeof s);
  causal_row_compact(causal_latest_row_matching("[save"), sv, sizeof sv);
  if (!strcmp(sv, "none"))
    causal_row_compact(causal_latest_row_matching("[load"), sv, sizeof sv);
  body_append(body, cap,
              "\nCausal overlay:\n"
              "  blocker: %s\n"
              "  parser:  %s\n"
              "  social:  %s\n"
              "  save:    %s\n",
              b, p, s, sv);
}

static void append_auto_causal_hint(const char *cmd, char *msg, size_t msgcap) {
  const char *row = NULL;
  if (!msg || !msg[0] || msgcap < 8) return;
  if (strstr(msg, "You can't go that way.") != NULL ||
      strstr(msg, "locked tight") != NULL || strstr(msg, "will not budge") != NULL) {
    row = causal_latest_row_matching("[move-blocked]");
    if (row && strlen(msg) + strlen(row) + 64 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nBecause: %s\nTry: why blocked", row);
    return;
  }
  if (strstr(msg, "You do not recognize that command.") != NULL) {
    row = causal_latest_row_matching("[unknown-command]");
    if (row && strlen(msg) + strlen(row) + 64 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nBecause: %s\nTry: causality parser", row);
    return;
  }
  if (strstr(msg, "You are not carrying that.") != NULL ||
      strstr(msg, "You do not see that here.") != NULL) {
    if (strlen(msg) + 84 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nHint: try 'find <item>' or 'scan' for visible context.");
    return;
  }
  if (strstr(msg, "No one is here") != NULL || strstr(msg, "Nobody to talk to") != NULL) {
    if (strlen(msg) + 84 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nHint: try 'who', 'nearby npc', or 'route <npc>'.");
    return;
  }
  if (cmd && (!strncmp(cmd, "save", 4) || !strncmp(cmd, "load", 4)) &&
      (strstr(msg, "Could not write") != NULL || strstr(msg, "No save at") != NULL ||
       strstr(msg, "Save corrupt") != NULL)) {
    row = causal_latest_row_matching("[load");
    if (!row) row = causal_latest_row_matching("[save");
    if (row && strlen(msg) + strlen(row) + 64 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nBecause: %s\nTry: saves  or  mods doctor", row);
  }
}

static void format_diag_health_body(char *body, size_t cap) {
  int start, i;
  int wok = (WORLD_ROOM_COUNT > 0 && WORLD_ROOM_COUNT <= MAX_WORLD_ROOMS);
  char banner[256];

  if (cap < 128) return;
  body[0] = '\0';
  pc_format_identity_banner(banner, sizeof banner);
  (void)snprintf(
      body, cap,
      "=== SESSION LOG & HEALTH ===\n\n"
      "%s\n\n"
      "Recent issues (newest last; max %d):\n",
      banner, DIAG_RING);
  if (g_diag_count == 0) {
    strncat(body, "  (none — no failed commands logged yet.)\n", cap - strlen(body) - 1);
  } else {
    start = (g_diag_head - g_diag_count + DIAG_RING) % DIAG_RING;
    for (i = 0; i < g_diag_count; i++) {
      char row[DIAG_W + 8];
      int idx = (start + i) % DIAG_RING;
      snprintf(row, sizeof row, "  • %s\n", g_diag_ring[idx]);
      strncat(body, row, cap - strlen(body) - 1);
    }
  }
  strncat(body, "\nEngine snapshot:\n", cap - strlen(body) - 1);
  {
    char chunk[768];
    (void)snprintf(
        chunk, sizeof chunk,
        "  Rooms loaded: %d (cap %d) %s\n"
        "  Save file: %s\n"
        "  Here: %s [%s]\n"
        "  Turns: %d   Pack: %d / %d   HP: %d / %d\n"
        "  NPC world refs: %d checked, %d warning(s)\n"
        "  Dark (no light): %s\n"
        "  Autotest mode: %s\n",
        WORLD_ROOM_COUNT, MAX_WORLD_ROOMS, wok ? "OK" : "CHECK",
        g_save_path[0] ? g_save_path : "(default)",
        resolve_world_title(g_room), world_slug(g_room), g_turns, g_inv_n, MAX_INV,
        g_health, g_max_health, g_npc_validation_checked,
        g_npc_validation_warnings, room_too_dark_to_see() ? "yes" : "no",
        aet_autotest() ? "on" : "off");
    strncat(body, chunk, cap - strlen(body) - 1);
  }
  strncat(
      body,
      "\nTip: errors clear  —  empties the issue list for this run.\n",
      cap - strlen(body) - 1);
}

static int npc_world_entity_count(const char *slug) {
  int i, c = 0;
  if (!slug || !slug[0]) return 0;
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    const char *ent = base_world_room_entity(i);
    if (ent && ent[0] && str_ieq(ent, slug)) c++;
  }
  return c;
}

static void validate_npc_world_refs(void) {
  int i;
  int checked = 0;
  int warnings = 0;

  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    const char *ent = base_world_room_entity(i);
    const AetNpcLineSet *dlg;
    const AetMerchantTable *mt;
    char detail[196];
    if (!ent || !ent[0]) continue;
    checked++;
    dlg = aet_npc_lines(ent);
    mt = aet_merchant_trades(ent);
    if (!dlg && !mt) {
      snprintf(detail, sizeof detail,
               "room %s [%s] references unknown npc/entity slug \"%s\"",
               world_slug(i), resolve_world_title(i), ent);
      diag_push("npc-world", detail);
      warnings++;
      continue;
    }
    if (mt && !dlg) {
      snprintf(detail, sizeof detail,
               "merchant slug \"%s\" is placed at %s [%s] but has no dialogue roster",
               ent, world_slug(i), resolve_world_title(i));
      diag_push("npc-world", detail);
      warnings++;
    }
  }

  for (i = 0; i < npc_routine_count(); i++) {
    const AetNpcRoutine *rt = npc_routine_at(i);
    const char *rooms[4] = {rt->morning_room, rt->afternoon_room, rt->evening_room,
                            rt->night_room};
    int k;
    checked++;
    if (!aet_npc_lines(rt->slug) && !aet_merchant_trades(rt->slug)) {
      char detail[196];
      snprintf(detail, sizeof detail,
               "routine slug \"%s\" has no npc dialogue or merchant data", rt->slug);
      diag_push("npc-world", detail);
      warnings++;
    }
    for (k = 0; k < 4; k++) {
      if (!rooms[k] || !rooms[k][0] || world_room_index(rooms[k]) >= 0) continue;
      {
        char detail[196];
        snprintf(detail, sizeof detail,
                 "routine for \"%s\" references unknown room slug \"%s\"", rt->slug,
                 rooms[k]);
        diag_push("npc-world", detail);
        warnings++;
      }
    }
  }

  for (i = 0; i < aet_npc_line_count(); i++) {
    const char *slug = aet_npc_line_slug_at(i);
    if (!slug || !slug[0]) continue;
    if (npc_world_entity_count(slug) <= 0) {
      char detail[196];
      snprintf(detail, sizeof detail,
               "dialogue roster \"%s\" has no room.entity placement in this build",
               slug);
      diag_push("npc-world", detail);
      warnings++;
      checked++;
    }
  }

  for (i = 0; i < AETER_SOC_NPC_MAX; i++) {
    const char *slug = g_soc_npc_slug[i];
    if (!slug[0]) continue;
    checked++;
    if (aet_npc_lines(slug) || npc_world_entity_count(slug) > 0) continue;
    {
      char detail[196];
      snprintf(detail, sizeof detail,
               "loaded social state references unknown npc slug \"%.96s\"",
               slug);
      diag_push("npc-world", detail);
      warnings++;
    }
  }

  g_npc_validation_checked = checked;
  g_npc_validation_warnings = warnings;
}

static int parse_save_slot(const char *s, int *slot) {
  char *end = NULL;
  long n;
  if (!s) return 0;
  while (*s == ' ') s++;
  if (!strncmp(s, "slot ", 5)) {
    s += 5;
    while (*s == ' ') s++;
  }
  if (!*s) return 0;
  n = strtol(s, &end, 10);
  while (end && *end == ' ') end++;
  if (!end || end == s || *end != '\0' || n < 1 || n > SAVE_SLOT_COUNT)
    return 0;
  *slot = (int)n;
  return 1;
}

static void make_slot_save_path(int slot, char *out, size_t cap) {
  const char *dot;
  const char *slash;
  const char *ext;
  char stem[520];
  char suffix[32];
  size_t stem_len;
  size_t suffix_len;
  size_t ext_len;
  if (!out || cap == 0) return;
  out[0] = '\0';
  if (slot < 1 || slot > SAVE_SLOT_COUNT) {
    snprintf(out, cap, "%s", g_save_path);
    return;
  }
  slash = strrchr(g_save_path, '\\');
  if (!slash) slash = strrchr(g_save_path, '/');
  dot = strrchr(g_save_path, '.');
  if (!dot || (slash && dot < slash)) dot = g_save_path + strlen(g_save_path);
  stem_len = (size_t)(dot - g_save_path);
  if (stem_len >= sizeof stem) stem_len = sizeof stem - 1;
  memcpy(stem, g_save_path, stem_len);
  stem[stem_len] = '\0';
  snprintf(suffix, sizeof suffix, "_slot%d", slot);
  suffix_len = strlen(suffix);
  ext = *dot ? dot : ".txt";
  ext_len = strlen(ext);
  if (stem_len + suffix_len + ext_len + 1 > cap) {
    snprintf(out, cap, "aeternitas64_slot%d.txt", slot);
    return;
  }
  memcpy(out, stem, stem_len);
  memcpy(out + stem_len, suffix, suffix_len);
  memcpy(out + stem_len + suffix_len, ext, ext_len + 1);
}

static void make_autosave_path(char *out, size_t cap) {
  const char *dot;
  const char *slash;
  const char *ext;
  char stem[520];
  size_t stem_len;
  size_t ext_len;
  if (!out || cap == 0) return;
  out[0] = '\0';
  slash = strrchr(g_save_path, '\\');
  if (!slash) slash = strrchr(g_save_path, '/');
  dot = strrchr(g_save_path, '.');
  if (!dot || (slash && dot < slash)) dot = g_save_path + strlen(g_save_path);
  stem_len = (size_t)(dot - g_save_path);
  if (stem_len >= sizeof stem) stem_len = sizeof stem - 1;
  memcpy(stem, g_save_path, stem_len);
  stem[stem_len] = '\0';
  ext = *dot ? dot : ".txt";
  ext_len = strlen(ext);
  if (stem_len + 9 + ext_len + 1 > cap) {
    snprintf(out, cap, "aeternitas64_autosave.txt");
    return;
  }
  memcpy(out, stem, stem_len);
  memcpy(out + stem_len, "_autosave", 9);
  memcpy(out + stem_len + 9, ext, ext_len + 1);
}

static void autosave_write_shadow(void) {
  char path[520];
  if (!g_autosave_enabled) return;
  make_autosave_path(path, sizeof path);
  (void)write_save_file_path(path);
}

static void entity_pretty(const char *ent, char *out, size_t cap) {
  size_t i = 0;
  if (!ent || !ent[0] || cap < 2) {
    if (cap) out[0] = '\0';
    return;
  }
  for (; *ent && i + 1 < cap; ent++)
    out[i++] = (char)(*ent == '_' ? ' ' : *ent);
  out[i] = '\0';
}

static void item_pretty(const char *item, char *out, size_t cap) {
  const char *label;
  size_t i = 0;
  int new_word = 1;
  if (!out || cap < 2) return;
  out[0] = '\0';
  if (!item || !item[0]) return;
  label = aet_item_catalog_label_for_slug(item);
  if (label && label[0]) {
    snprintf(out, cap, "%s", label);
    return;
  }
  for (; *item && i + 1 < cap; item++) {
    unsigned char c = (unsigned char)*item;
    if (c == '_') {
      out[i++] = ' ';
      new_word = 1;
      continue;
    }
    out[i++] = (char)(new_word ? toupper(c) : c);
    new_word = 0;
  }
  out[i] = '\0';
}

static void soc_npc_clear(void) {
  memset(g_soc_npc_slug, 0, sizeof g_soc_npc_slug);
  memset(g_soc_npc_friendship, 0, sizeof g_soc_npc_friendship);
  memset(g_soc_npc_romance, 0, sizeof g_soc_npc_romance);
  memset(g_soc_npc_last_turn, 0, sizeof g_soc_npc_last_turn);
  memset(g_soc_topic_mem, 0, sizeof g_soc_topic_mem);
  memset(g_soc_topic_last, 0, sizeof g_soc_topic_last);
}

static int soc_npc_find(const char *slug) {
  int i;
  if (!slug || !slug[0]) return -1;
  for (i = 0; i < AETER_SOC_NPC_MAX; i++) {
    if (g_soc_npc_slug[i][0] && str_ieq(g_soc_npc_slug[i], slug)) return i;
  }
  return -1;
}

static int soc_npc_alloc(const char *slug) {
  int i;
  size_t L;
  if (!slug || !slug[0]) return -1;
  if (aet_merchant_index(slug) >= 0) return -1;
  for (i = 0; i < AETER_SOC_NPC_MAX; i++) {
    if (!g_soc_npc_slug[i][0]) {
      L = strnlen(slug, MAX_ITEM_LEN - 1);
      memcpy(g_soc_npc_slug[i], slug, L);
      g_soc_npc_slug[i][L] = '\0';
      return i;
    }
  }
  return -1;
}

static int soc_npc_ensure(const char *slug) {
  int ix = soc_npc_find(slug);
  if (ix >= 0) return ix;
  return soc_npc_alloc(slug);
}

static void merchant_rep_clear(void) {
  memset(g_merchant_rep, 0, sizeof g_merchant_rep);
  memset(g_npc_friendship, 0, sizeof g_npc_friendship);
  memset(g_npc_romance, 0, sizeof g_npc_romance);
  memset(g_npc_last_social_turn, 0, sizeof g_npc_last_social_turn);
  memset(g_npc_topic_mem, 0, sizeof g_npc_topic_mem);
  memset(g_npc_topic_last, 0, sizeof g_npc_topic_last);
  soc_npc_clear();
}

static void merchant_rep_load_line(const char *line) {
  int mc = aet_merchant_count();
  int i = 0;
  const char *p = line;
  memset(g_merchant_rep, 0, sizeof g_merchant_rep);
  if (mc > AETER_REP_MAX) mc = AETER_REP_MAX;
  while (i < mc && p && *p) {
    char *end = NULL;
    long v = strtol(p, &end, 10);
    if (!end || end == p) break;
    if (v < 0) v = 0;
    if (v > 220) v = 220;
    g_merchant_rep[i++] = (int)v;
    p = end;
    while (*p == ' ' || *p == '\t') p++;
  }
}

static void merchant_rep_bump_slug(const char *slug, int delta) {
  int ix;
  int before;
  if (!slug || !slug[0] || delta == 0) return;
  ix = aet_merchant_index(slug);
  if (ix < 0 || ix >= AETER_REP_MAX) return;
  before = g_merchant_rep[ix];
  g_merchant_rep[ix] += delta;
  if (g_merchant_rep[ix] < 0) g_merchant_rep[ix] = 0;
  if (g_merchant_rep[ix] > 220) g_merchant_rep[ix] = 220;
  rel_hist_push(slug, "patron", 0, 0, g_merchant_rep[ix] - before);
}

static void soc_clamp_byte(unsigned char *p) {
  if (*p > 100u) *p = 100u;
}

static void relationship_decay_tick(void) {
  int i;
  if (g_turns < 1 || (g_turns % 20) != 0) return;
  for (i = 0; i < AETER_REP_MAX; i++) {
    if ((int)g_npc_friendship[i] < 10) continue;
    if (g_npc_last_social_turn[i] <= 0 ||
        g_turns - g_npc_last_social_turn[i] < 50)
      continue;
    g_npc_friendship[i] =
        (unsigned char)((int)g_npc_friendship[i] > 0 ? (int)g_npc_friendship[i] - 1 : 0);
    {
      const char *slug = aet_merchant_slug_at(i);
      if (slug && slug[0]) rel_hist_push(slug, "idle decay", -1, 0, 0);
    }
  }
  for (i = 0; i < AETER_SOC_NPC_MAX; i++) {
    if (!g_soc_npc_slug[i][0]) continue;
    if ((int)g_soc_npc_friendship[i] < 10) continue;
    if (g_soc_npc_last_turn[i] <= 0 || g_turns - g_soc_npc_last_turn[i] < 50)
      continue;
    g_soc_npc_friendship[i] =
        (unsigned char)((int)g_soc_npc_friendship[i] > 0
                            ? (int)g_soc_npc_friendship[i] - 1
                            : 0);
    rel_hist_push(g_soc_npc_slug[i], "idle decay", -1, 0, 0);
  }
}

static int merchant_rep_score(int ix);

static unsigned char soc_derive_stage_values(int friendship, int romance,
                                           int patron_rep) {
  int f = friendship;
  int r = romance;
  int rep = patron_rep;
  if (r >= 75 && f >= 50) return 6;
  if (r >= 55 && f >= 35) return 5;
  if (r >= 28) return 4;
  if (f >= 58 || rep >= 85) return 3;
  if (f >= 30 || rep >= 45) return 2;
  if (f >= 10 || rep >= 12) return 1;
  return 0;
}

static unsigned char soc_derive_stage(int ix) {
  return soc_derive_stage_values((int)g_npc_friendship[ix],
                                 (int)g_npc_romance[ix], merchant_rep_score(ix));
}

static unsigned char soc_derive_stage_npc_slug(const char *slug) {
  int mix = aet_merchant_index(slug);
  int si;
  if (mix >= 0 && mix < AETER_REP_MAX)
    return soc_derive_stage(mix);
  si = soc_npc_find(slug);
  if (si < 0)
    return soc_derive_stage_values(0, 0, 0);
  return soc_derive_stage_values((int)g_soc_npc_friendship[si],
                                 (int)g_soc_npc_romance[si], 0);
}

static const char *soc_stage_name(unsigned char st) {
  switch (st) {
    case 0:
      return "Stranger";
    case 1:
      return "Acquainted";
    case 2:
      return "Friend";
    case 3:
      return "Close friend";
    case 4:
      return "Romantic interest";
    case 5:
      return "Lover";
    case 6:
      return "Partner";
    default:
      return "Stranger";
  }
}

static void soc_bump_talk(const char *slug) {
  int ix;
  int si;
  AetPcSave p;
  int bonus = 0;
  unsigned char f0, r0;
  if (!slug || !slug[0]) return;
  ix = aet_merchant_index(slug);
  pc_capture(&p);
  pc_fill_narrative_defaults(&p);
  if (p.cha >= 15) bonus += 1;
  if (p.cha >= 18) bonus += 1;
  if (ix >= 0 && ix < AETER_REP_MAX) {
    f0 = g_npc_friendship[ix];
    r0 = g_npc_romance[ix];
    g_npc_friendship[ix] =
        (unsigned char)((int)g_npc_friendship[ix] + 2 + bonus);
    soc_clamp_byte(&g_npc_friendship[ix]);
    if (p.cha >= 17)
      g_npc_romance[ix] = (unsigned char)((int)g_npc_romance[ix] + 1);
    soc_clamp_byte(&g_npc_romance[ix]);
    g_npc_last_social_turn[ix] = g_turns;
    rel_hist_push(slug, "talk", (int)g_npc_friendship[ix] - (int)f0,
                  (int)g_npc_romance[ix] - (int)r0, 0);
    return;
  }
  si = soc_npc_ensure(slug);
  if (si < 0) return;
  f0 = g_soc_npc_friendship[si];
  r0 = g_soc_npc_romance[si];
  g_soc_npc_friendship[si] =
      (unsigned char)((int)g_soc_npc_friendship[si] + 2 + bonus);
  soc_clamp_byte(&g_soc_npc_friendship[si]);
  if (p.cha >= 17)
    g_soc_npc_romance[si] =
        (unsigned char)((int)g_soc_npc_romance[si] + 1);
  soc_clamp_byte(&g_soc_npc_romance[si]);
  g_soc_npc_last_turn[si] = g_turns;
  rel_hist_push(slug, "talk", (int)g_soc_npc_friendship[si] - (int)f0,
                (int)g_soc_npc_romance[si] - (int)r0, 0);
}

static void soc_bump_gift(const char *slug) {
  int ix;
  int si;
  unsigned char f0, r0;
  if (!slug || !slug[0]) return;
  ix = aet_merchant_index(slug);
  if (ix >= 0 && ix < AETER_REP_MAX) {
    f0 = g_npc_friendship[ix];
    r0 = g_npc_romance[ix];
    g_npc_friendship[ix] = (unsigned char)((int)g_npc_friendship[ix] + 8);
    g_npc_romance[ix] = (unsigned char)((int)g_npc_romance[ix] + 5);
    soc_clamp_byte(&g_npc_friendship[ix]);
    soc_clamp_byte(&g_npc_romance[ix]);
    g_npc_last_social_turn[ix] = g_turns;
    rel_hist_push(slug, "gift", (int)g_npc_friendship[ix] - (int)f0,
                  (int)g_npc_romance[ix] - (int)r0, 0);
    return;
  }
  si = soc_npc_ensure(slug);
  if (si < 0) return;
  f0 = g_soc_npc_friendship[si];
  r0 = g_soc_npc_romance[si];
  g_soc_npc_friendship[si] =
      (unsigned char)((int)g_soc_npc_friendship[si] + 8);
  g_soc_npc_romance[si] = (unsigned char)((int)g_soc_npc_romance[si] + 5);
  soc_clamp_byte(&g_soc_npc_friendship[si]);
  soc_clamp_byte(&g_soc_npc_romance[si]);
  g_soc_npc_last_turn[si] = g_turns;
  rel_hist_push(slug, "gift", (int)g_soc_npc_friendship[si] - (int)f0,
                (int)g_soc_npc_romance[si] - (int)r0, 0);
}

static int soc_parse_int_line(const char *line, int *dst, int maxn) {
  char work[800];
  char *ctx = NULL;
  char *tok;
  int n = 0;
  if (!line || !dst || maxn <= 0) return 0;
  strncpy(work, line, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  for (tok = strtok_r(work, " \t", &ctx); tok && n < maxn;
       tok = strtok_r(NULL, " \t", &ctx)) {
    long v = strtol(tok, NULL, 10);
    if (v < 0) v = 0;
    if (v > 1000000) v = 1000000;
    dst[n++] = (int)v;
  }
  return n;
}

static int soc_parse_uchar_line(const char *line, unsigned char *dst, int maxn) {
  char work[800];
  char *ctx = NULL;
  char *tok;
  int n = 0;
  if (!line || !dst || maxn <= 0) return 0;
  strncpy(work, line, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  for (tok = strtok_r(work, " \t", &ctx); tok && n < maxn;
       tok = strtok_r(NULL, " \t", &ctx)) {
    long v = strtol(tok, NULL, 10);
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    dst[n++] = (unsigned char)v;
  }
  return n;
}

static void merchant_rep_bump_conversation(const char *slug, int base_delta) {
  AetPcSave p;
  int d = base_delta;
  if (!slug || !slug[0] || base_delta == 0) return;
  pc_capture(&p);
  if (p.cha >= 15) d += 1;
  if (p.cha >= 18) d += 1;
  merchant_rep_bump_slug(slug, d);
  soc_bump_talk(slug);
}

static void merchant_rep_bump_gift(const char *slug) {
  AetPcSave p;
  int d = 3;
  if (!slug || !slug[0]) return;
  pc_capture(&p);
  if (p.cha >= 14) d += 1;
  merchant_rep_bump_slug(slug, d);
  soc_bump_gift(slug);
}

static int merchant_rep_score(int ix) {
  int v;
  if (ix < 0 || ix >= AETER_REP_MAX) return 0;
  v = g_merchant_rep[ix];
  if (v < 0) return 0;
  if (v > 220) return 220;
  return v;
}

static const char *merchant_rep_tier_label(int score) {
  if (score >= 90) return "Renowned";
  if (score >= 65) return "Trusted";
  if (score >= 45) return "Favored";
  if (score >= 25) return "Regular";
  if (score >= 10) return "Acquainted";
  return "Stranger";
}

static int merchant_buy_discount_pct(int score) {
  if (score >= 90) return 15;
  if (score >= 65) return 12;
  if (score >= 45) return 8;
  if (score >= 25) return 5;
  if (score >= 10) return 2;
  return 0;
}

static int merchant_sell_bonus_pct(int score) {
  if (score >= 90) return 12;
  if (score >= 65) return 9;
  if (score >= 45) return 6;
  if (score >= 25) return 3;
  if (score >= 10) return 1;
  return 0;
}

static const char *merchant_trade_skill_label(int cha) {
  if (cha >= 18) return "silver-tongued";
  if (cha >= 16) return "keen trader";
  if (cha >= 14) return "confident bargainer";
  if (cha >= 12) return "watchful buyer";
  if (cha >= 10) return "practical haggler";
  return "unguarded";
}

static int merchant_trade_buy_skill_pct(int cha) {
  if (cha >= 18) return 8;
  if (cha >= 16) return 6;
  if (cha >= 14) return 4;
  if (cha >= 12) return 2;
  return 0;
}

static int merchant_trade_sell_skill_pct(int cha) {
  if (cha >= 18) return 15;
  if (cha >= 16) return 12;
  if (cha >= 14) return 10;
  if (cha >= 12) return 7;
  if (cha >= 10) return 5;
  return 0;
}

static int merchant_adjust_buy_price(int base, int score, int cha) {
  int pct = merchant_buy_discount_pct(score) + merchant_trade_buy_skill_pct(cha);
  int n = base - (base * pct) / 100;
  return n < 1 ? 1 : n;
}

static int merchant_adjust_sell_price(int base, int score, int cha) {
  int pct = merchant_sell_bonus_pct(score) + merchant_trade_sell_skill_pct(cha);
  int bonus = (base * pct) / 100;
  if (pct > 0 && base >= 4 && bonus < 1) bonus = 1;
  return base + bonus;
}

static int merchant_friendship_score(int ix) {
  if (ix < 0 || ix >= AETER_REP_MAX) return 0;
  return (int)g_npc_friendship[ix];
}

static int merchant_haggle_buy_pct(int score, int friendship, int cha) {
  int pct = 0;
  if (cha >= 10) pct += 4;
  if (cha >= 14) pct += 2;
  if (cha >= 18) pct += 2;
  if (score >= 10) pct += 1;
  if (score >= 25) pct += 1;
  if (friendship >= 20) pct += 1;
  if (friendship >= 40) pct += 1;
  if (pct > 12) pct = 12;
  return pct;
}

static int merchant_haggle_sell_pct(int score, int friendship, int cha) {
  int pct = 0;
  if (cha >= 10) pct += 6;
  if (cha >= 14) pct += 3;
  if (cha >= 18) pct += 3;
  if (score >= 10) pct += 1;
  if (score >= 25) pct += 1;
  if (friendship >= 20) pct += 1;
  if (friendship >= 40) pct += 1;
  if (pct > 18) pct = 18;
  return pct;
}

static int merchant_haggle_buy_price(int current, int score, int friendship,
                                     int cha) {
  int pct = merchant_haggle_buy_pct(score, friendship, cha);
  int delta = (current * pct) / 100;
  if (pct > 0 && current >= 10 && delta < 1) delta = 1;
  current -= delta;
  return current < 1 ? 1 : current;
}

static int merchant_haggle_sell_price(int current, int score, int friendship,
                                      int cha) {
  int pct = merchant_haggle_sell_pct(score, friendship, cha);
  int delta = (current * pct) / 100;
  if (pct > 0 && current >= 4 && delta < 1) delta = 1;
  return current + delta;
}

static void barter_set(int mode, const char *merchant, const char *item,
                       int quoted_price, int list_price, int expire_turn) {
  g_barter_mode = mode;
  g_barter_price = quoted_price;
  g_barter_list_price = list_price;
  g_barter_expire_turn = expire_turn;
  copy_capped(g_barter_merchant, sizeof g_barter_merchant, merchant);
  copy_capped(g_barter_item, sizeof g_barter_item, item);
}

static int barter_quote_sync(void) {
  const char *ent;
  if (g_barter_mode == BARTER_NONE) return 0;
  if (g_turns > g_barter_expire_turn) {
    barter_clear();
    return 0;
  }
  ent = world_room_entity(g_room);
  if (!ent[0] || !str_ieq(ent, g_barter_merchant)) {
    barter_clear();
    return 0;
  }
  return 1;
}

static int barter_quote_matches(int mode, const char *merchant, const char *item,
                                int *price_out) {
  if (!barter_quote_sync()) return 0;
  if (g_barter_mode != mode) return 0;
  if (!merchant || !merchant[0] || !str_ieq(g_barter_merchant, merchant)) return 0;
  if (!item || !item[0] || !str_ieq(g_barter_item, item)) return 0;
  if (price_out) *price_out = g_barter_price;
  return 1;
}

static int read_kv_int(FILE *fp, const char *expect_key, int *v) {
  char buf[160];
  char key[40];
  if (!fgets(buf, sizeof buf, fp)) return 0;
  chomp_line(buf);
  if (sscanf(buf, "%39s %d", key, v) != 2) return 0;
  return strcmp(key, expect_key) == 0;
}

static int write_save_file_path(const char *path) {
  FILE *fp;
  int r, i;
  fp = fopen(path, "w");
  if (!fp) return 0;
  fprintf(fp, "AET64SAVE1\n");
  fprintf(fp, "worldcnt %d\n", WORLD_ROOM_COUNT);
  fprintf(fp, "room %d\n", g_room);
  fprintf(fp, "turns %d\n", g_turns);
  fprintf(fp, "score %d\n", g_score);
  fprintf(fp, "coins %d\n", g_coins);
  fprintf(fp, "hp %d\n", g_health);
  fprintf(fp, "hpmax %d\n", g_max_health);
  fprintf(fp, "front %d\n", g_front_unlocked);
  fprintf(fp, "shed %d\n", g_shed_unlocked);
  fprintf(fp, "roomv %d\n", g_verbose_room);
  fprintf(fp, "craftprof %d\n", g_craft_proficiency);
  fprintf(fp, "hintena %d\n", g_hints_pref ? 1 : 0);
  fprintf(fp, "colorov %d\n", g_settings_color_ov);
  fprintf(fp, "autosave %d\n", g_autosave_enabled ? 1 : 0);
  fprintf(fp, "histn %d\n", g_hist_n);
  for (i = 0; i < g_hist_n; i++) fprintf(fp, "%d\n", g_hist[i]);
  for (r = 0; r < WORLD_ROOM_COUNT; r++)
    fputc(g_visited[r] ? '1' : '0', fp);
  fputc('\n', fp);
  fprintf(fp, "invn %d\n", g_inv_n);
  for (i = 0; i < g_inv_n; i++) fprintf(fp, "%s\n", g_inv[i]);
  fprintf(fp, "ROOMS\n");
  for (r = 0; r < WORLD_ROOM_COUNT; r++) {
    fprintf(fp, "R %d %d %d\n", r, g_room_item_n[r], g_hidden_n[r]);
    for (i = 0; i < g_room_item_n[r]; i++)
      fprintf(fp, "%s\n", g_room_items[r][i]);
    for (i = 0; i < g_hidden_n[r]; i++)
      fprintf(fp, "%s\n", g_hidden_items[r][i]);
  }
  if (!containers_write_section(fp)) return 0;
  fprintf(fp, "NOTES\n%d\n", g_note_n);
  for (i = 0; i < g_note_n; i++) fprintf(fp, "%s\n", g_notes[i]);
  fprintf(fp, "READIED\n");
  fprintf(fp, "%s\n", g_ready_item[0] ? g_ready_item : "");
  fprintf(fp, "EQUIP\n");
  for (i = 0; i < EQ_SLOT_COUNT; i++)
    fprintf(fp, "%s\n", g_eq_slots[i][0] ? g_eq_slots[i] : "");
  {
    int mc = aet_merchant_count();
    int ri;
    if (mc > AETER_REP_MAX) mc = AETER_REP_MAX;
    fprintf(fp, "REP");
    for (ri = 0; ri < mc; ri++)
      fprintf(fp, " %d", merchant_rep_score(ri));
    fputc('\n', fp);
    fprintf(fp, "SOC\n");
    fprintf(fp, "%d\n", mc);
    for (ri = 0; ri < mc; ri++)
      fprintf(fp, "%s%u", ri ? " " : "", (unsigned)g_npc_friendship[ri]);
    fputc('\n', fp);
    for (ri = 0; ri < mc; ri++)
      fprintf(fp, "%s%u", ri ? " " : "", (unsigned)g_npc_romance[ri]);
    fputc('\n', fp);
    for (ri = 0; ri < mc; ri++)
      fprintf(fp, "%s%d", ri ? " " : "", g_npc_last_social_turn[ri]);
    fputc('\n', fp);
  }
  
  {
    int si, nout = 0;
    for (si = 0; si < AETER_SOC_NPC_MAX; si++) {
      if (g_soc_npc_slug[si][0]) nout++;
    }
    fprintf(fp, "SOC2\n%d\n", nout);
    for (si = 0; si < AETER_SOC_NPC_MAX; si++) {
      if (!g_soc_npc_slug[si][0]) continue;
      fprintf(fp, "%s %u %u %d\n", g_soc_npc_slug[si],
              (unsigned)g_soc_npc_friendship[si],
              (unsigned)g_soc_npc_romance[si], g_soc_npc_last_turn[si]);
    }
  }
  fprintf(fp, "TRADELOG\n%d\n", g_trade_count);
  {
    int start = (g_trade_head - g_trade_count + TRADE_RING) % TRADE_RING;
    for (i = 0; i < g_trade_count; i++) {
      int idx = (start + i) % TRADE_RING;
      fprintf(fp, "%s\n", g_trade_ring[idx]);
    }
  }
  fprintf(fp, "RELHIST\n%d\n", g_rel_hist_count);
  {
    int rh, rstart =
        (g_rel_hist_head - g_rel_hist_count + REL_HIST_RING) % REL_HIST_RING;
    for (rh = 0; rh < g_rel_hist_count; rh++) {
      int ridx = (rstart + rh) % REL_HIST_RING;
      const RelHistRow *row = &g_rel_hist[ridx];
      fprintf(fp, "%d %s %s %d %d %d\n", row->turn, row->slug, row->act,
              (int)row->df, (int)row->dr, (int)row->dp);
    }
  }
  fprintf(fp, "BARTER\n");
  fprintf(fp, "%d %d %d %d\n", g_barter_mode, g_barter_price, g_barter_list_price,
          g_barter_expire_turn);
  fprintf(fp, "%s\n", g_barter_merchant[0] ? g_barter_merchant : "");
  fprintf(fp, "%s\n", g_barter_item[0] ? g_barter_item : "");
  pc_write_save(fp);
  fprintf(fp, "FOCUS\n");
  fprintf(fp, "%s\n", g_last_focus[0] ? g_last_focus : "");
  fprintf(fp, "DROPPED\n");
  fprintf(fp, "%s\n", g_last_dropped[0] ? g_last_dropped : "");
  fprintf(fp, "TOPIC\n");
  fprintf(fp, "%s\n", g_last_topic[0] ? g_last_topic : "");
  fprintf(fp, "LASTNPC\n");
  fprintf(fp, "%s\n", g_last_npc[0] ? g_last_npc : "");
  {
    int mc = aet_merchant_count();
    int ri, si, nout = 0;
    if (mc > AETER_REP_MAX) mc = AETER_REP_MAX;
    for (ri = 0; ri < mc; ri++) {
      if (g_npc_topic_mem[ri][0] || g_npc_topic_last[ri][0]) nout++;
    }
    for (si = 0; si < AETER_SOC_NPC_MAX; si++) {
      if (g_soc_npc_slug[si][0] &&
          (g_soc_topic_mem[si][0] || g_soc_topic_last[si][0]))
        nout++;
    }
    fprintf(fp, "CONVMEM\n%d\n", nout);
    for (ri = 0; ri < mc; ri++) {
      const char *slug = aet_merchant_slug_at(ri);
      if (!slug || (!g_npc_topic_mem[ri][0] && !g_npc_topic_last[ri][0]))
        continue;
      fprintf(fp, "%s\t%s\t%s\n", slug, g_npc_topic_mem[ri],
              g_npc_topic_last[ri]);
    }
    for (si = 0; si < AETER_SOC_NPC_MAX; si++) {
      if (!g_soc_npc_slug[si][0]) continue;
      if (!g_soc_topic_mem[si][0] && !g_soc_topic_last[si][0]) continue;
      fprintf(fp, "%s\t%s\t%s\n", g_soc_npc_slug[si], g_soc_topic_mem[si],
              g_soc_topic_last[si]);
    }
  }
#ifdef AETER_MINIGAMES
  {
    MgtPersistentState mgst;
    fprintf(fp, "CRAFTPROF\n%d\n", g_craft_prof_n);
    for (i = 0; i < g_craft_prof_n; i++) {
      CraftSavedProfile *cp = &g_craft_prof[i];
      fprintf(fp, "%s\n", cp->name);
      fprintf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d\n", cp->hrd, cp->shp,
              cp->flx, cp->dur, cp->wgt, cp->grp, cp->bnd, cp->utl, cp->quality,
              cp->disp_dur, cp->disp_shp, cp->disp_hnd, cp->disp_wgt);
    }
    fprintf(fp, "LOCKPICK\n%d %d\n", g_lockpick_last_noise, g_lockpick_suspicion);
    game_mgt_capture_profile(&mgst);
    if (!mgt_profile_write_embedded(fp, &mgst)) {
      (void)fclose(fp);
      return 0;
    }
  }
#else
  fprintf(fp, "CRAFTPROF\n%d\n", g_craft_prof_n);
  for (i = 0; i < g_craft_prof_n; i++) {
    CraftSavedProfile *cp = &g_craft_prof[i];
    fprintf(fp, "%s\n", cp->name);
    fprintf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d\n", cp->hrd, cp->shp,
            cp->flx, cp->dur, cp->wgt, cp->grp, cp->bnd, cp->utl, cp->quality,
            cp->disp_dur, cp->disp_shp, cp->disp_hnd, cp->disp_wgt);
  }
  fprintf(fp, "LOCKPICK\n%d %d\n", g_lockpick_last_noise, g_lockpick_suspicion);
#endif
  if (fflush(fp) != 0 || ferror(fp)) {
    (void)fclose(fp);
    return 0;
  }
  if (fclose(fp) != 0)
    return 0;
  return 1;
}

static int write_save_file(void) { return write_save_file_path(g_save_path); }

static int save_game(char *msg, size_t msgcap) {
  if (!write_save_file()) {
    snprintf(msg, msgcap, "Could not write %s.", g_save_path);
    causal_push("save-failed", g_save_path);
    return 0;
  }
  snprintf(msg, msgcap, "%s — saved %s (%d rooms of item state).",
           pc_display_name(), g_save_path, WORLD_ROOM_COUNT);
  causal_push("save", g_save_path);
  return 1;
}

static int save_game_slot(int slot, char *msg, size_t msgcap) {
  char path[520];
  make_slot_save_path(slot, path, sizeof path);
  if (!write_save_file_path(path)) {
    snprintf(msg, msgcap, "Could not write slot %d (%s).", slot, path);
    causal_push("save-slot-failed", path);
    return 0;
  }
  snprintf(msg, msgcap, "%s — saved to slot %d: %s.", pc_display_name(), slot,
           path);
  causal_push("save-slot", path);
  return 1;
}

static int load_game_path(const char *path, char *msg, size_t msgcap) {
  FILE *fp = NULL;
  char buf[512];
  char vis[MAX_WORLD_ROOMS + 8];
  int wc, r, i, ri, n, h;
  int focus_loaded = 0;
  fp = fopen(path, "r");
  if (!fp) {
    snprintf(msg, msgcap, "No save at %s.", path);
    causal_push("load-miss", path);
    return 0;
  }
  snapshot_capture(&g_load_rollback);
  disambig_reset_all();
  conv_clear();
  g_verbose_room = 1;
  g_recap_n = 0;
  g_last_npc[0] = '\0';
  g_last_topic[0] = '\0';
  memset(g_npc_topic_mem, 0, sizeof g_npc_topic_mem);
  memset(g_npc_topic_last, 0, sizeof g_npc_topic_last);
  memset(g_soc_topic_mem, 0, sizeof g_soc_topic_mem);
  memset(g_soc_topic_last, 0, sizeof g_soc_topic_last);
  g_ready_item[0] = '\0';
  memset(g_npc_friendship, 0, sizeof g_npc_friendship);
  memset(g_npc_romance, 0, sizeof g_npc_romance);
  memset(g_npc_last_social_turn, 0, sizeof g_npc_last_social_turn);
  soc_npc_clear();
  trade_history_clear();
  rel_hist_clear();
  barter_clear();
  if (!fgets(buf, sizeof buf, fp)) goto bad;
  chomp_line(buf);
  if (strcmp(buf, "AET64SAVE1") != 0) goto bad;
  if (!read_kv_int(fp, "worldcnt", &wc) || wc != WORLD_ROOM_COUNT) goto bad;
  if (!read_kv_int(fp, "room", &g_room) || g_room < 0 ||
      g_room >= WORLD_ROOM_COUNT)
    goto bad;
  if (!read_kv_int(fp, "turns", &g_turns) || g_turns < 0) goto bad;
  if (!read_kv_int(fp, "score", &g_score)) goto bad;
  g_coins = 0;
  g_health = AETER_START_HP;
  g_max_health = AETER_START_HP;
  {
    long pos = ftell(fp);
    if (pos >= 0 && fgets(buf, sizeof buf, fp)) {
      int c;
      chomp_line(buf);
      if (sscanf(buf, "coins %d", &c) == 1 && c >= 0)
        g_coins = c;
      else if (fseek(fp, pos, SEEK_SET) != 0)
        goto bad;
    } else if (pos >= 0 && fseek(fp, pos, SEEK_SET) != 0)
      goto bad;
  }
  {
    long pos = ftell(fp);
    int th, tm;
    if (pos >= 0 && fgets(buf, sizeof buf, fp)) {
      chomp_line(buf);
      if (sscanf(buf, "hp %d", &th) == 1 && th >= 0 && th <= 99999 &&
          fgets(buf, sizeof buf, fp)) {
        chomp_line(buf);
        if (sscanf(buf, "hpmax %d", &tm) == 1 && tm >= 1 && tm <= 99999) {
          g_health = th > tm ? tm : th;
          g_max_health = tm;
        } else if (fseek(fp, pos, SEEK_SET) != 0)
          goto bad;
      }       else if (fseek(fp, pos, SEEK_SET) != 0)
        goto bad;
    } else if (pos >= 0 && fseek(fp, pos, SEEK_SET) != 0)
      goto bad;
  }
  if (!read_kv_int(fp, "front", &g_front_unlocked)) goto bad;
  if (!read_kv_int(fp, "shed", &g_shed_unlocked)) goto bad;
  if ((g_front_unlocked | g_shed_unlocked) & ~1) goto bad;
  {
    long pos = ftell(fp);
    if (pos >= 0 && fgets(buf, sizeof buf, fp)) {
      int rv;
      chomp_line(buf);
      if (sscanf(buf, "roomv %d", &rv) == 1 && (rv == 0 || rv == 1))
        g_verbose_room = rv;
      else if (fseek(fp, pos, SEEK_SET) != 0)
        goto bad;
    } else if (pos >= 0 && fseek(fp, pos, SEEK_SET) != 0)
      goto bad;
  }
  {
    long pos = ftell(fp);
    if (pos >= 0 && fgets(buf, sizeof buf, fp)) {
      int cv;
      chomp_line(buf);
      if (sscanf(buf, "craftprof %d", &cv) == 1 && cv >= 1 && cv <= 10)
        g_craft_proficiency = cv;
      else if (fseek(fp, pos, SEEK_SET) != 0)
        goto bad;
    } else if (pos >= 0 && fseek(fp, pos, SEEK_SET) != 0)
      goto bad;
  }
  {
    long pos = ftell(fp);
    if (pos >= 0 && fgets(buf, sizeof buf, fp)) {
      int h;
      chomp_line(buf);
      if (sscanf(buf, "hintena %d", &h) == 1 && (h == 0 || h == 1))
        g_hints_pref = h;
      else if (fseek(fp, pos, SEEK_SET) != 0)
        goto bad;
    } else if (pos >= 0 && fseek(fp, pos, SEEK_SET) != 0)
      goto bad;
  }
  {
    long pos = ftell(fp);
    if (pos >= 0 && fgets(buf, sizeof buf, fp)) {
      int co;
      chomp_line(buf);
      if (sscanf(buf, "colorov %d", &co) == 1 && co >= -1 && co <= 1)
        g_settings_color_ov = co;
      else if (fseek(fp, pos, SEEK_SET) != 0)
        goto bad;
    } else if (pos >= 0 && fseek(fp, pos, SEEK_SET) != 0)
      goto bad;
  }
  {
    long pos = ftell(fp);
    if (pos >= 0 && fgets(buf, sizeof buf, fp)) {
      int au;
      chomp_line(buf);
      if (sscanf(buf, "autosave %d", &au) == 1 && (au == 0 || au == 1))
        g_autosave_enabled = au;
      else if (fseek(fp, pos, SEEK_SET) != 0)
        goto bad;
    } else if (pos >= 0 && fseek(fp, pos, SEEK_SET) != 0)
      goto bad;
  }
  if (!read_kv_int(fp, "histn", &g_hist_n) || g_hist_n < 0 ||
      g_hist_n > BACK_HIST)
    goto bad;
  for (i = 0; i < g_hist_n; i++) {
    if (!fgets(buf, sizeof buf, fp)) goto bad;
    chomp_line(buf);
    g_hist[i] = atoi(buf);
    if (g_hist[i] < 0 || g_hist[i] >= WORLD_ROOM_COUNT) goto bad;
  }
  if (!fgets(vis, sizeof vis, fp)) goto bad;
  chomp_line(vis);
  if ((int)strlen(vis) < WORLD_ROOM_COUNT) goto bad;
  for (r = 0; r < WORLD_ROOM_COUNT; r++) {
    if (vis[r] != '0' && vis[r] != '1') goto bad;
    g_visited[r] = (unsigned char)(vis[r] - '0');
  }
  if (!read_kv_int(fp, "invn", &g_inv_n) || g_inv_n < 0 || g_inv_n > MAX_INV)
    goto bad;
  for (i = 0; i < g_inv_n; i++) {
    size_t L;
    if (!fgets(buf, sizeof buf, fp)) goto bad;
    chomp_line(buf);
    L = strnlen(buf, (size_t)MAX_ITEM_LEN - 1);
    memcpy(g_inv[i], buf, L);
    g_inv[i][L] = '\0';
  }
  if (!fgets(buf, sizeof buf, fp)) goto bad;
  chomp_line(buf);
  if (strcmp(buf, "ROOMS") != 0) goto bad;
  for (r = 0; r < WORLD_ROOM_COUNT; r++) {
    if (!fgets(buf, sizeof buf, fp)) goto bad;
    chomp_line(buf);
    if (sscanf(buf, "R %d %d %d", &ri, &n, &h) != 3 || ri != r || n < 0 ||
        n > MAX_ITEMS_ROOM || h < 0 || h > MAX_ITEMS_ROOM)
      goto bad;
    g_room_item_n[r] = 0;
    for (i = 0; i < n; i++) {
      size_t L;
      if (!fgets(buf, sizeof buf, fp)) goto bad;
      chomp_line(buf);
      L = strnlen(buf, (size_t)MAX_ITEM_LEN - 1);
      memcpy(g_room_items[r][i], buf, L);
      g_room_items[r][i][L] = '\0';
    }
    g_room_item_n[r] = n;
    g_hidden_n[r] = 0;
    for (i = 0; i < h; i++) {
      size_t L;
      if (!fgets(buf, sizeof buf, fp)) goto bad;
      chomp_line(buf);
      L = strnlen(buf, (size_t)MAX_ITEM_LEN - 1);
      memcpy(g_hidden_items[r][i], buf, L);
      g_hidden_items[r][i][L] = '\0';
    }
    g_hidden_n[r] = h;
  }
  g_note_n = 0;
  if (!fgets(buf, sizeof buf, fp)) goto bad;
  chomp_line(buf);
  if (strcmp(buf, "CONTAINERS") == 0) {
    if (!containers_read_section(fp)) goto bad;
    if (!fgets(buf, sizeof buf, fp)) goto bad;
    chomp_line(buf);
  } else {
    containers_init_new_game();
  }
  if (strcmp(buf, "NOTES") != 0) goto bad;
  if (!fgets(buf, sizeof buf, fp)) goto bad;
  {
    int nn, k;
    chomp_line(buf);
    nn = atoi(buf);
    if (nn < 0 || nn > MAX_NOTES) goto bad;
    for (k = 0; k < nn; k++) {
      size_t noteL;
      if (!fgets(buf, sizeof buf, fp)) goto bad;
      chomp_line(buf);
      noteL = strnlen(buf, NOTE_LEN - 1);
      memcpy(g_notes[g_note_n], buf, noteL);
      g_notes[g_note_n][noteL] = '\0';
      g_note_n++;
    }
    if (!fgets(buf, sizeof buf, fp)) goto bad;
    chomp_line(buf);
    if (strcmp(buf, "READIED") != 0) goto bad;
    if (!fgets(buf, sizeof buf, fp)) goto bad;
    {
      size_t rl;
      int eq_loaded = 0;
      int pc_loaded = 0;
      chomp_line(buf);
      rl = strnlen(buf, sizeof g_ready_item - 1);
      memcpy(g_ready_item, buf, rl);
      g_ready_item[rl] = '\0';
      if (!inv_has(g_ready_item)) g_ready_item[0] = '\0';
      if (!fgets(buf, sizeof buf, fp)) goto bad;
      chomp_line(buf);
      if (strcmp(buf, "EQUIP") != 0) goto bad;
      {
        int esi;
        eq_clear_all();
        for (esi = 0; esi < EQ_SLOT_COUNT; esi++) {
          size_t el;
          if (!fgets(buf, sizeof buf, fp)) goto bad;
          chomp_line(buf);
          el = strnlen(buf, MAX_ITEM_LEN - 1);
          memcpy(g_eq_slots[esi], buf, el);
          g_eq_slots[esi][el] = '\0';
          if (g_eq_slots[esi][0] && !inv_has(g_eq_slots[esi]))
            g_eq_slots[esi][0] = '\0';
        }
        eq_loaded = 1;
      }
      if (!fgets(buf, sizeof buf, fp)) goto bad;
      chomp_line(buf);
      if (!strncmp(buf, "REP ", 4)) {
        merchant_rep_load_line(buf + 4);
        if (!fgets(buf, sizeof buf, fp)) goto bad;
        chomp_line(buf);
      } else {
        memset(g_merchant_rep, 0, sizeof g_merchant_rep);
      }
      if (!strcmp(buf, "SOC")) {
              char ln[800];
              int mc_file2 = aet_merchant_count();
              int nf, nr, jj;
              if (mc_file2 > AETER_REP_MAX) mc_file2 = AETER_REP_MAX;
              if (!fgets(ln, sizeof ln, fp)) goto bad;
              chomp_line(ln);
              (void)atoi(ln);
              if (!fgets(ln, sizeof ln, fp)) goto bad;
              chomp_line(ln);
              nf = soc_parse_uchar_line(ln, g_npc_friendship, AETER_REP_MAX);
              if (!fgets(ln, sizeof ln, fp)) goto bad;
              chomp_line(ln);
              nr = soc_parse_uchar_line(ln, g_npc_romance, AETER_REP_MAX);
              for (jj = nf; jj < mc_file2; jj++) g_npc_friendship[jj] = 0;
              for (jj = nr; jj < mc_file2; jj++) g_npc_romance[jj] = 0;
              if (!fgets(ln, sizeof ln, fp)) goto bad;
              chomp_line(ln);
              if (ln[0] && strcmp(ln, "SOC2") != 0 &&
                  strcmp(ln, "TRADELOG") != 0 && strcmp(ln, "RELHIST") != 0 &&
                  strcmp(ln, "BARTER") != 0 &&
                  strcmp(ln, "CHARACTER") != 0 && strncmp(ln, "REP ", 4) != 0) {
                int nl;
                nl = soc_parse_int_line(ln, g_npc_last_social_turn, AETER_REP_MAX);
                for (jj = nl; jj < mc_file2; jj++) g_npc_last_social_turn[jj] = 0;
                if (!fgets(buf, sizeof buf, fp)) goto bad;
                chomp_line(buf);
              } else {
                strncpy(buf, ln, sizeof buf - 1);
                buf[sizeof buf - 1] = '\0';
              }
            }
            if (!strcmp(buf, "SOC2")) {
              char ln2[512];
              int rowwant, rr;
              if (!fgets(ln2, sizeof ln2, fp)) goto bad;
              chomp_line(ln2);
              rowwant = atoi(ln2);
              if (rowwant < 0) rowwant = 0;
              if (rowwant > 512) rowwant = 512;
              soc_npc_clear();
              for (rr = 0; rr < rowwant; rr++) {
                unsigned uf, ur;
                int ut;
                char slugbuf[MAX_ITEM_LEN];
                int slot;
                if (!fgets(ln2, sizeof ln2, fp)) goto bad;
                chomp_line(ln2);
                if (sscanf(ln2, "%47s %u %u %d", slugbuf, &uf, &ur, &ut) != 4)
                  goto bad;
                if (uf > 100u) uf = 100u;
                if (ur > 100u) ur = 100u;
                if (!slugbuf[0] || aet_merchant_index(slugbuf) >= 0) continue;
                slot = soc_npc_ensure(slugbuf);
                if (slot < 0) continue;
                g_soc_npc_friendship[slot] = (unsigned char)uf;
                g_soc_npc_romance[slot] = (unsigned char)ur;
                g_soc_npc_last_turn[slot] = ut;
              }
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
            }
            if (!strcmp(buf, "TRADELOG")) {
              int rowwant, rr;
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
              rowwant = atoi(buf);
              if (rowwant < 0) rowwant = 0;
              if (rowwant > TRADE_RING) rowwant = TRADE_RING;
              trade_history_clear();
              for (rr = 0; rr < rowwant; rr++) {
                if (!fgets(buf, sizeof buf, fp)) goto bad;
                chomp_line(buf);
                trade_history_push_row(buf);
              }
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
            }
            if (!strcmp(buf, "RELHIST")) {
              int rowwant, rr;
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
              rowwant = atoi(buf);
              if (rowwant < 0) rowwant = 0;
              if (rowwant > REL_HIST_RING) rowwant = REL_HIST_RING;
              rel_hist_clear();
              for (rr = 0; rr < rowwant; rr++) {
                int rt, rdf, rdr, rdp;
                char slugbuf[MAX_ITEM_LEN];
                char actbuf[REL_HIST_ACT];
                if (!fgets(buf, sizeof buf, fp)) goto bad;
                chomp_line(buf);
                slugbuf[0] = actbuf[0] = '\0';
                if (sscanf(buf, "%d %47s %19s %d %d %d", &rt, slugbuf, actbuf,
                           &rdf, &rdr, &rdp) >= 6)
                  rel_hist_load_row(rt, slugbuf, actbuf, rdf, rdr, rdp);
              }
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
            }
            if (!strcmp(buf, "BARTER")) {
              int bm, bp, bl, be;
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
              barter_clear();
              if (sscanf(buf, "%d %d %d %d", &bm, &bp, &bl, &be) == 4) {
                g_barter_mode = bm;
                g_barter_price = bp;
                g_barter_list_price = bl;
                g_barter_expire_turn = be;
              }
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
              if (strlen(buf) >= sizeof g_barter_merchant) {
                barter_clear();
                goto bad;
              }
              copy_capped(g_barter_merchant, sizeof g_barter_merchant, buf);
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
              if (strlen(buf) >= sizeof g_barter_item) {
                barter_clear();
                goto bad;
              }
              copy_capped(g_barter_item, sizeof g_barter_item, buf);
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
              barter_sanitize_loaded();
            }
            if (strcmp(buf, "CHARACTER") == 0) {
              if (!pc_read_save(fp, buf, sizeof buf)) goto bad;
              pc_loaded = 1;
              if (!fgets(buf, sizeof buf, fp)) goto bad;
              chomp_line(buf);
            }
            if (!pc_loaded) pc_set_default_adventurer();
            if (!eq_loaded) {
              eq_clear_all();
              if (g_ready_item[0]) {
                snprintf(g_eq_slots[EQ_WEAPON], sizeof g_eq_slots[EQ_WEAPON], "%s",
                         g_ready_item);
              }
            }
      eq_sync_ready_item();
      if (strcmp(buf, "FOCUS") == 0 && fgets(buf, sizeof buf, fp)) {
        size_t fl;
        chomp_line(buf);
        focus_loaded = 1;
        fl = strnlen(buf, sizeof g_last_focus - 1);
        memcpy(g_last_focus, buf, fl);
        g_last_focus[fl] = '\0';
        if (!g_last_focus[0] || !inv_has(g_last_focus)) clear_focus();
        if (fgets(buf, sizeof buf, fp)) {
          chomp_line(buf);
          if (strcmp(buf, "DROPPED") == 0 && fgets(buf, sizeof buf, fp)) {
            chomp_line(buf);
            fl = strnlen(buf, sizeof g_last_dropped - 1);
            memcpy(g_last_dropped, buf, fl);
            g_last_dropped[fl] = '\0';
            if (fgets(buf, sizeof buf, fp)) chomp_line(buf);
          }
          if (strcmp(buf, "TOPIC") == 0 && fgets(buf, sizeof buf, fp)) {
            chomp_line(buf);
            fl = strnlen(buf, sizeof g_last_topic - 1);
            memcpy(g_last_topic, buf, fl);
            g_last_topic[fl] = '\0';
            if (fgets(buf, sizeof buf, fp)) {
              chomp_line(buf);
              if (strcmp(buf, "LASTNPC") == 0 && fgets(buf, sizeof buf, fp)) {
                chomp_line(buf);
                if (buf[0]) copy_capped(g_last_npc, sizeof g_last_npc, buf);
                if (fgets(buf, sizeof buf, fp)) {
                  chomp_line(buf);
                  if (strcmp(buf, "CONVMEM") == 0 && fgets(buf, sizeof buf, fp)) {
                    int rowwant, rr;
                    chomp_line(buf);
                    rowwant = atoi(buf);
                    if (rowwant < 0) rowwant = 0;
                    if (rowwant > 256) rowwant = 256;
                    memset(g_npc_topic_mem, 0, sizeof g_npc_topic_mem);
                    memset(g_npc_topic_last, 0, sizeof g_npc_topic_last);
                    memset(g_soc_topic_mem, 0, sizeof g_soc_topic_mem);
                    memset(g_soc_topic_last, 0, sizeof g_soc_topic_last);
                    for (rr = 0; rr < rowwant; rr++) {
                      char slugbuf[MAX_ITEM_LEN];
                      char tagsbuf[CONV_TOPIC_MEM_SZ];
                      char lastbuf[CONV_TOPIC_MEM_SZ];
                      char *tab, *tab2;
                      int ix, si;
                      if (!fgets(buf, sizeof buf, fp)) goto bad;
                      chomp_line(buf);
                      tab = strchr(buf, '\t');
                      if (!tab) continue;
                      *tab++ = '\0';
                      tab2 = strchr(tab, '\t');
                      if (!tab2) continue;
                      *tab2++ = '\0';
                      copy_capped(slugbuf, sizeof slugbuf, buf);
                      copy_capped(tagsbuf, sizeof tagsbuf, tab);
                      copy_capped(lastbuf, sizeof lastbuf, tab2);
                      ix = aet_merchant_index(slugbuf);
                      if (ix >= 0 && ix < AETER_REP_MAX) {
                        copy_capped(g_npc_topic_mem[ix], sizeof g_npc_topic_mem[0],
                                    tagsbuf);
                        copy_capped(g_npc_topic_last[ix], sizeof g_npc_topic_last[0],
                                    lastbuf);
                        continue;
                      }
                      si = soc_npc_find(slugbuf);
                      if (si < 0) si = soc_npc_ensure(slugbuf);
                      if (si >= 0) {
                        copy_capped(g_soc_topic_mem[si], sizeof g_soc_topic_mem[0],
                                    tagsbuf);
                        copy_capped(g_soc_topic_last[si], sizeof g_soc_topic_last[0],
                                    lastbuf);
                      }
                    }
                    if (!fgets(buf, sizeof buf, fp)) goto bad;
                    chomp_line(buf);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
#ifdef AETER_MINIGAMES
  g_mgt_quicksave_valid = 0;
  g_mgt_ready = 0;
  if (strcmp(buf, "MGT") == 0) {
    if (mgt_profile_read_embedded(fp, &g_mgt_quicksave_profile))
      g_mgt_quicksave_valid = 1;
  } else {
    while (fgets(buf, sizeof buf, fp)) {
      chomp_line(buf);
      if (strcmp(buf, "CRAFTPROF") == 0) {
        int cn, ci;
        g_craft_prof_n = 0;
        if (!fgets(buf, sizeof buf, fp)) break;
        chomp_line(buf);
        cn = atoi(buf);
        if (cn < 0) cn = 0;
        if (cn > MAX_CRAFT_PROFILES) cn = MAX_CRAFT_PROFILES;
        for (ci = 0; ci < cn; ci++) {
          CraftSavedProfile *cp;
          if (!fgets(buf, sizeof buf, fp)) break;
          chomp_line(buf);
          if (g_craft_prof_n >= MAX_CRAFT_PROFILES) continue;
          cp = &g_craft_prof[g_craft_prof_n++];
          copy_capped(cp->name, sizeof cp->name, buf);
          if (!fgets(buf, sizeof buf, fp)) {
            g_craft_prof_n--;
            break;
          }
          chomp_line(buf);
          if (sscanf(buf, "%d %d %d %d %d %d %d %d %d %d %d %d %d", &cp->hrd,
                     &cp->shp, &cp->flx, &cp->dur, &cp->wgt, &cp->grp, &cp->bnd,
                     &cp->utl, &cp->quality, &cp->disp_dur, &cp->disp_shp,
                     &cp->disp_hnd, &cp->disp_wgt) < 13)
            g_craft_prof_n--;
        }
        continue;
      }
      if (strcmp(buf, "LOCKPICK") == 0) {
        if (fgets(buf, sizeof buf, fp)) {
          chomp_line(buf);
          sscanf(buf, "%d %d", &g_lockpick_last_noise, &g_lockpick_suspicion);
        }
        continue;
      }
      if (strcmp(buf, "MGT") == 0) {
        if (mgt_profile_read_embedded(fp, &g_mgt_quicksave_profile))
          g_mgt_quicksave_valid = 1;
        break;
      }
    }
  }
#endif
  fclose(fp);
  if (!focus_loaded)
    clear_focus();
  eq_sync_pc_sheet();
  ui_init_color();
  validate_npc_world_refs();
  snprintf(msg, msgcap, "%s — restored from %s.", pc_display_name(), path);
  causal_push("load", path);
  return 1;
bad:
  snapshot_restore(&g_load_rollback);
  eq_sync_ready_item();
  if (fp) fclose(fp);
  snprintf(msg, msgcap,
           "Save corrupt or built for a different world size. Regenerate and "
           "start fresh.");
  causal_push("load-invalid", path);
  return 0;
}

static int load_game(char *msg, size_t msgcap) {
  return load_game_path(g_save_path, msg, msgcap);
}

static int load_game_slot(int slot, char *msg, size_t msgcap) {
  char path[520];
  make_slot_save_path(slot, path, sizeof path);
  return load_game_path(path, msg, msgcap);
}

#define SAVE_MGR_FLOPPY_BYTES (1474560UL)
#define SAVE_MGR_BAR_LEN 28

typedef struct {
  int slot;
  int ok_header;
  char path[520];
  char dosname[16];
  char datestr[16];
  char sizelab[16];
  char desc[56];
  unsigned long bytes;
  int room;
  int turns;
} AetSaveSlotInfo;

static unsigned long save_mgr_total_bytes_on_disk(void) {
  struct stat st;
  unsigned long t = 0;
  int s;
  char apath[520];
  if (g_save_path[0] && stat(g_save_path, &st) == 0)
    t += (unsigned long)st.st_size;
  make_autosave_path(apath, sizeof apath);
  if (apath[0] && stat(apath, &st) == 0) t += (unsigned long)st.st_size;
  for (s = 1; s <= SAVE_SLOT_COUNT; s++) {
    char path[520];
    make_slot_save_path(s, path, sizeof path);
    if (stat(path, &st) == 0) t += (unsigned long)st.st_size;
  }
  return t;
}

static void save_mgr_probe_slot(int slot, AetSaveSlotInfo *o) {
  struct stat st;
  FILE *fp;
  char line[512];
  int room = -1, turns = -1, v;

  memset(o, 0, sizeof *o);
  o->slot = slot;
  o->room = -1;
  snprintf(o->dosname, sizeof o->dosname, "SLOT%02d.SAV", slot);
  make_slot_save_path(slot, o->path, sizeof o->path);
  snprintf(o->datestr, sizeof o->datestr, "--/--/--");
  snprintf(o->sizelab, sizeof o->sizelab, "---   ");
  snprintf(o->desc, sizeof o->desc, "Empty Slot");

  if (stat(o->path, &st) != 0) return;

  o->bytes = (unsigned long)st.st_size;
  {
    struct tm *tm = localtime(&st.st_mtime);
    if (tm)
      (void)strftime(o->datestr, sizeof o->datestr, "%m-%d-%y", tm);
  }
  if (o->bytes < 1024)
    snprintf(o->sizelab, sizeof o->sizelab, "%luB", o->bytes);
  else
    snprintf(o->sizelab, sizeof o->sizelab, "%lukB",
             (unsigned long)(o->bytes / 1024));

  fp = fopen(o->path, "r");
  if (!fp) {
    snprintf(o->desc, sizeof o->desc, "Read error");
    return;
  }
  if (!fgets(line, sizeof line, fp)) {
    fclose(fp);
    snprintf(o->desc, sizeof o->desc, "Unreadable");
    return;
  }
  chomp_line(line);
  if (strcmp(line, "AET64SAVE1") != 0) {
    fclose(fp);
    snprintf(o->desc, sizeof o->desc, "Not a save file");
    return;
  }
  o->ok_header = 1;
  while (fgets(line, sizeof line, fp)) {
    chomp_line(line);
    if (strcmp(line, "ROOMS") == 0) break;
    if (sscanf(line, "room %d", &v) == 1) room = v;
    else if (sscanf(line, "turns %d", &v) == 1) turns = v;
  }
  fclose(fp);
  o->room = room;
  o->turns = turns < 0 ? 0 : turns;
  if (room >= 0 && room < WORLD_ROOM_COUNT)
    snprintf(o->desc, sizeof o->desc, "%s - T%d", resolve_world_title(room),
             o->turns);
  else if (room >= 0)
    snprintf(o->desc, sizeof o->desc, "Room %d - T%d", room, o->turns);
  else
    snprintf(o->desc, sizeof o->desc, "Incomplete header");
}

static int save_mgr_delete_slot(int slot, char *msg, size_t msgcap) {
  char path[520];
  struct stat st;
  if (slot < 1 || slot > SAVE_SLOT_COUNT) {
    snprintf(msg, msgcap, "Slot must be 1-%d.", SAVE_SLOT_COUNT);
    return 0;
  }
  make_slot_save_path(slot, path, sizeof path);
  if (stat(path, &st) != 0) {
    snprintf(msg, msgcap, "Slot %d is already empty.", slot);
    return 0;
  }
  if (remove(path) != 0) {
    snprintf(msg, msgcap, "Could not delete %s.", path);
    return 0;
  }
  snprintf(msg, msgcap, "Deleted slot %d.", slot);
  return 1;
}

static int run_save_manager_ui(int *did_fullscreen, int esc_menu) {
  static const char *const kExit[] = {"exit", "done", "back", "resume", NULL};
  AetSaveSlotInfo rows[SAVE_SLOT_COUNT];
  char line[INPUT_LINE_MAX];
  char msg[1024];
  int s, i, filled, pct;
  unsigned long used;
  const char *suf;

  for (;;) {
    clear_frame();
    used = save_mgr_total_bytes_on_disk();
    pct = (int)((used * 100UL) / SAVE_MGR_FLOPPY_BYTES);
    if (pct > 100) pct = 100;
    for (s = 1; s <= SAVE_SLOT_COUNT; s++) save_mgr_probe_slot(s, &rows[s - 1]);

    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(" %sSAVE MANAGER%s                                          %sDRIVE: "
           "A:\\%s\n",
           C_TITLE, C_RESET, C_HEADING, C_RESET);
    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(" %sID   FILENAME      DATE      SIZE    DESCRIPTION%s\n", C_HEADING,
           C_RESET);
    printf(" %s", C_BORDER);
    for (i = 0; i < 118; i++) putchar('-');
    printf("%s\n", C_RESET);
    for (s = 0; s < SAVE_SLOT_COUNT; s++) {
      char descvis[52];
      const AetSaveSlotInfo *r = &rows[s];
      snprintf(descvis, sizeof descvis, "%.40s", r->desc);
      printf(" %s%2d%s    %-12s  %8s   %-6s  %s%s%s\n", C_ITEM, r->slot, C_RESET,
             r->dosname, r->datestr, r->sizelab, C_HEADING, descvis, C_RESET);
    }
    printf("\n%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(" %sDISK SPACE:%s [", C_HEADING, C_RESET);
    filled = (pct * SAVE_MGR_BAR_LEN + 50) / 100;
    if (filled > SAVE_MGR_BAR_LEN) filled = SAVE_MGR_BAR_LEN;
    printf("%s", C_EXIT);
    for (i = 0; i < filled; i++) putchar('#');
    printf("%s", C_MUTED);
    for (; i < SAVE_MGR_BAR_LEN; i++) putchar('.');
    printf("%s] %s%d%% USED%s\n", C_RESET, C_ITEM, pct, C_RESET);
    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(
        " %sCOMMANDS:%s [SAVE #]  [LOAD #]  [DEL #]  [EXIT]   %sQuick save "
        "file:%s\n",
        C_MUTED, C_RESET, C_MUTED, C_RESET);
    printf(" %s%s%s\n", C_MUTED, g_save_path[0] ? g_save_path : "(default)",
           C_RESET);
    {
      char apath[520];
      struct stat ast;
      make_autosave_path(apath, sizeof apath);
      if (apath[0] && stat(apath, &ast) == 0)
        printf(" %sAutosave shadow:%s %s (%lukB)  %sload autosave%s\n", C_MUTED,
               C_RESET, apath, (unsigned long)(ast.st_size / 1024), C_ITEM,
               C_RESET);
      else if (g_autosave_enabled)
        printf(" %sAutosave shadow:%s (none yet — advance a turn)%s\n", C_MUTED,
               C_RESET, C_RESET);
    }
    suf = aet_mods_character_saves_suffix();
    if (suf && suf[0]) printf(" %s%s%s\n", C_MUTED, suf, C_RESET);
    printf("\n %s>>%s ", C_TITLE, C_RESET);
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) {
      if (did_fullscreen) *did_fullscreen = 1;
      if (!esc_menu) return_to_game_screen();
      return 0;
    }
    chomp_line(line);
    strip_trailing_space(line);
    for (i = 0; line[i]; i++) line[i] = (char)tolower((unsigned char)line[i]);
    if (line_equals_one_of(line, kExit)) {
      if (did_fullscreen) *did_fullscreen = 1;
      if (!esc_menu) return_to_game_screen();
      return 0;
    }
    {
      char a0[32], a1[32];
      int ns = sscanf(line, "%31s %31s", a0, a1);
      int slotnum = 0;
      if (ns >= 2) slotnum = (int)strtol(a1, NULL, 10);
      if (ns >= 2 && (!strcmp(a0, "save") || !strcmp(a0, "s"))) {
        if (slotnum < 1 || slotnum > SAVE_SLOT_COUNT) {
          ui_block_pause("SAVE MANAGER", "Use: save <1-10>.");
          continue;
        }
        save_game_slot(slotnum, msg, sizeof msg);
        ui_block_pause("SAVE", msg);
        continue;
      }
      if (ns >= 2 && (!strcmp(a0, "load") || !strcmp(a0, "l"))) {
        if (slotnum < 1 || slotnum > SAVE_SLOT_COUNT) {
          ui_block_pause("SAVE MANAGER", "Use: load <1-10>.");
          continue;
        }
        if (load_game_slot(slotnum, msg, sizeof msg)) {
          if (did_fullscreen) *did_fullscreen = 1;
          return_to_game_screen();
          return 1;
        }
        ui_block_pause("LOAD FAILED", msg);
        continue;
      }
      if (ns >= 2 &&
          (!strcmp(a0, "del") || !strcmp(a0, "delete") || !strcmp(a0, "rm"))) {
        if (slotnum < 1 || slotnum > SAVE_SLOT_COUNT) {
          ui_block_pause("SAVE MANAGER", "Use: del <1-10>.");
          continue;
        }
        (void)save_mgr_delete_slot(slotnum, msg, sizeof msg);
        ui_block_pause("DELETE", msg);
        continue;
      }
    }
    ui_block_pause(
        "SAVE MANAGER",
        "Commands: save N, load N, del N, exit (done/back/resume).\n");
  }
}

typedef struct {
  int day;
  int hour;
  int minute;
  const char *period;
  const char *season;
  const char *weather;
  int temp_c;
  int forecast_hours;
} AetWorldClock;

static const char *time_period_for_hour(int hour) {
  if (hour >= 6 && hour < 12) return "morning";
  if (hour >= 12 && hour < 18) return "afternoon";
  if (hour >= 18 && hour < 21) return "evening";
  return "night";
}

static int period_start_hour(const char *period) {
  if (str_ieq(period, "morning")) return 6;
  if (str_ieq(period, "afternoon")) return 12;
  if (str_ieq(period, "evening")) return 18;
  if (str_ieq(period, "night")) return 21;
  return -1;
}

static const char *season_for_day(int day) {
  int doy = ((day - 1) % 360) + 1;
  if (doy <= 90) return "spring";
  if (doy <= 180) return "summer";
  if (doy <= 270) return "autumn";
  return "winter";
}

static const char *weather_for_slice(const char *season, int roll) {
  if (str_ieq(season, "spring")) {
    if (roll < 40) return "clear";
    if (roll < 75) return "rain";
    if (roll < 90) return "fog";
    if (roll < 95) return "snow";
    return "storm";
  }
  if (str_ieq(season, "summer")) {
    if (roll < 60) return "clear";
    if (roll < 75) return "rain";
    if (roll < 90) return "storm";
    if (roll < 95) return "fog";
    return "snow";
  }
  if (str_ieq(season, "autumn")) {
    if (roll < 45) return "clear";
    if (roll < 75) return "rain";
    if (roll < 95) return "fog";
    return "snow";
  }
  if (roll < 30) return "clear";
  if (roll < 70) return "snow";
  if (roll < 90) return "fog";
  return "rain";
}

static void world_clock_for_turn(int turns, AetWorldClock *out) {
  int total_minutes = 12 * 60 + turns * 10;
  int day_zero = total_minutes / (24 * 60);
  int minute_of_day = total_minutes % (24 * 60);
  int base_temp;
  int roll;
  out->day = day_zero + 1;
  out->hour = minute_of_day / 60;
  out->minute = minute_of_day % 60;
  out->period = time_period_for_hour(out->hour);
  out->season = season_for_day(out->day);
  base_temp = str_ieq(out->season, "summer")   ? 25
              : str_ieq(out->season, "winter") ? 5
                                                : 15;
  if (out->hour >= 12 && out->hour < 17)
    base_temp += 8;
  else if (out->hour >= 17 && out->hour < 21)
    base_temp += 2;
  else if (out->hour < 6 || out->hour >= 22)
    base_temp -= 6;
  roll = (out->day * 37 + (out->hour / 6) * 19 + g_room * 3) % 100;
  out->weather = weather_for_slice(out->season, roll);
  if (str_ieq(out->weather, "snow"))
    base_temp -= 5;
  else if (str_ieq(out->weather, "storm"))
    base_temp -= 3;
  else if (str_ieq(out->weather, "rain"))
    base_temp -= 2;
  else if (str_ieq(out->weather, "fog"))
    base_temp -= 1;
  out->temp_c = base_temp;
  out->forecast_hours = 6 - (out->hour % 6);
}

static const AetNpcRoutine NPC_ROUTINES[] = {
    {"miller", "abandoned_mill", "river_shore", "river_shore",
     "mill_upper_level"},
    {"blacksmith", "blacksmith", "blacksmith", "blacksmith_waystone",
     "blacksmith_waystone"},
    {"forest_hermit", "hermit_hut", "deep_forest", "hermit_hut", "hermit_hut"},
    {"general_store_owner", "general_store", "village_square", "general_store",
     "general_store"},
    {"tavern_keeper", "tavern_kitchen", "tavern_common_room",
     "tavern_common_room", "tavern_back_room"},
    {"priestess", "temple_garden", "temple_of_architect", "temple_garden",
     "temple_of_architect"},
    {"paladin_marcus", "temple_training_grounds", "temple_training_grounds",
     "temple_of_architect", "temple_training_grounds"},
    {"village_innkeeper", "village_inn", "village_inn", "village_inn",
     "inn_rooms"},
    {"village_guard", "village_square", "village_square", "town_square",
     "town_square"},
    {"traveling_bard", "village_square", "bard_stage", "village_square",
     "bard_stage"},
    {"traveling_merchant", "west_of_house", "tavern_exterior", "village_road",
     "west_of_house"},
    {"bartender", "tavern_common_room", "tavern_common_room",
     "tavern_common_room", "tavern_back_room"},
    {"farmer", "farm", "farm", "village_square", "farm"},
    {"missionary_elena", "temple_of_architect", "village_square",
     "temple_garden", "temple_of_architect"},
    {"missionary_kira", "temple_garden", "village_road", "village_square",
     "temple_garden"},
    {"missionary_yuki", "village_square", "temple_garden",
     "temple_of_architect", "village_square"},
};

static int npc_routine_count(void) {
  return (int)(sizeof NPC_ROUTINES / sizeof NPC_ROUTINES[0]);
}

static const AetNpcRoutine *npc_routine_at(int idx) {
  return (idx >= 0 && idx < npc_routine_count()) ? &NPC_ROUTINES[idx] : NULL;
}

static const AetNpcRoutine *npc_routine_for(const char *slug) {
  size_t i;
  if (!slug || !slug[0]) return NULL;
  for (i = 0; i < sizeof NPC_ROUTINES / sizeof NPC_ROUTINES[0]; i++) {
    if (str_ieq(NPC_ROUTINES[i].slug, slug)) return &NPC_ROUTINES[i];
  }
  return NULL;
}

static int npc_resolve_routine_query(const char *q, char *slug_out, size_t slugcap,
                                     char *pretty_out, size_t prettycap) {
  char buf[128], qn[MAX_ITEM_LEN], pretty[96];
  size_t i;
  if (!q || !q[0] || !slug_out || slugcap < 2) return 0;
  strncpy(buf, q, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  strip_leading_articles(buf);
  strip_trailing_space(buf);
  query_norm_underscore(qn, sizeof qn, buf);
  for (i = 0; i < (size_t)npc_routine_count(); i++) {
    const AetNpcRoutine *rt = npc_routine_at(i);
    if (!rt) continue;
    entity_pretty(rt->slug, pretty, sizeof pretty);
    if (!(str_ieq(rt->slug, buf) || strstr(rt->slug, qn) != NULL ||
          str_ieq(pretty, buf) || strstr(pretty, buf) != NULL))
      continue;
    copy_capped(slug_out, slugcap, rt->slug);
    if (pretty_out && prettycap > 1)
      copy_capped(pretty_out, prettycap, pretty);
    return 1;
  }
  return 0;
}

static const char *npc_routine_room_for_period(const AetNpcRoutine *rt,
                                               const char *period) {
  if (!rt) return NULL;
  if (str_ieq(period, "morning")) return rt->morning_room;
  if (str_ieq(period, "afternoon")) return rt->afternoon_room;
  if (str_ieq(period, "evening")) return rt->evening_room;
  return rt->night_room;
}

static const char *npc_routine_next_period(const char *period) {
  if (str_ieq(period, "morning")) return "afternoon";
  if (str_ieq(period, "afternoon")) return "evening";
  if (str_ieq(period, "evening")) return "night";
  return "morning";
}

static const char *npc_routine_room_special(const AetNpcRoutine *rt,
                                            const AetWorldClock *wc) {
  if (!rt || !wc) return NULL;
  if (str_ieq(rt->slug, "farmer")) {
    int dow = wc->day % 7;
    if ((dow == 0 || dow == 3) && str_ieq(wc->period, "afternoon"))
      return "village_square";
  }
  if (str_ieq(rt->slug, "traveling_bard")) {
    if (wc->day % 7 == 6 && str_ieq(wc->period, "evening"))
      return "festival_stage";
  }
  if (str_ieq(rt->slug, "traveling_merchant")) {
    if (wc->day % 7 == 5 && str_ieq(wc->period, "afternoon"))
      return "village_square";
  }
  return npc_routine_room_for_period(rt, wc->period);
}

static const char *npc_room_slug_for_turn(const char *slug, int turns) {
  AetWorldClock wc;
  const AetNpcRoutine *rt = npc_routine_for(slug);
  if (!rt) return NULL;
  world_clock_for_turn(turns, &wc);
  return npc_routine_room_special(rt, &wc);
}

static const char *npc_activity_for_period(const char *slug,
                                           const char *period) {
  if (!slug || !slug[0]) return "";
  if (str_ieq(slug, "miller")) {
    if (str_ieq(period, "morning")) return "minding the millstones";
    if (str_ieq(period, "afternoon") || str_ieq(period, "evening"))
      return "watching the river";
    return "checking the upper works";
  }
  if (str_ieq(slug, "blacksmith")) {
    if (str_ieq(period, "morning") || str_ieq(period, "afternoon"))
      return "working the forge";
    return "banking the fire near the waystone";
  }
  if (str_ieq(slug, "forest_hermit")) {
    if (str_ieq(period, "afternoon")) return "walking the deep forest";
    return "keeping to the hermit's hut";
  }
  if (str_ieq(slug, "general_store_owner")) {
    if (str_ieq(period, "afternoon")) return "chatting in the village square";
    if (str_ieq(period, "evening")) return "closing ledgers at the counter";
    if (str_ieq(period, "night")) return "counting stock behind the counter";
    return "keeping the shop floor";
  }
  if (str_ieq(slug, "tavern_keeper")) {
    if (str_ieq(period, "morning")) return "working the tavern kitchen";
    if (str_ieq(period, "night")) return "handling quiet business in the back room";
    return "hosting the common room";
  }
  if (str_ieq(slug, "priestess")) {
    if (str_ieq(period, "morning") || str_ieq(period, "evening"))
      return "walking the temple garden";
    return "hearing petitions in the temple";
  }
  if (str_ieq(slug, "paladin_marcus")) {
    if (str_ieq(period, "evening")) return "standing watch in the temple hall";
    return "drilling in the training grounds";
  }
  if (str_ieq(slug, "village_innkeeper")) {
    if (str_ieq(period, "night")) return "checking the upstairs rooms";
    return "keeping the inn";
  }
  if (str_ieq(slug, "village_guard")) {
    if (str_ieq(period, "evening") || str_ieq(period, "night"))
      return "watching the town square";
    return "patrolling the village square";
  }
  if (str_ieq(slug, "traveling_bard")) {
    if (str_ieq(period, "morning") || str_ieq(period, "evening"))
      return "performing in the square";
    return "holding court in the tavern";
  }
  if (str_ieq(slug, "traveling_merchant")) {
    if (str_ieq(period, "morning") || str_ieq(period, "night"))
      return "unpacking wares by the road";
    if (str_ieq(period, "afternoon"))
      return "pitching wares outside the Rusty Anchor";
    return "counting coin and packing up";
  }
  if (str_ieq(slug, "farmer")) {
    if (str_ieq(period, "evening")) return "shyly offering produce in the square";
    return "working the fields";
  }
  if (str_ieq(slug, "bartender")) return "polishing glasses behind the bar";
  if (str_ieq(slug, "missionary_elena"))
    return str_ieq(period, "afternoon") ? "spreading love in the square"
                                        : "tending temple rites";
  if (str_ieq(slug, "missionary_kira"))
    return str_ieq(period, "evening") ? "flirting with passersby in the square"
                                      : "practicing playful devotion";
  if (str_ieq(slug, "missionary_yuki"))
    return str_ieq(period, "morning") ? "counseling in the village square"
                                      : "meditating on temple wisdom";
  return "";
}

static const char *runtime_room_entity(int room) {
  AetWorldClock wc;
  const char *base;
  const AetNpcRoutine *base_rt;
  const char *scheduled;
  size_t i;
  if (room < 0 || room >= WORLD_ROOM_COUNT) return "";
  world_clock_for_turn(g_turns, &wc);
  base = base_world_room_entity(room);
  if (base && base[0]) {
    base_rt = npc_routine_for(base);
    if (!base_rt) return base;
    scheduled = npc_routine_room_special(base_rt, &wc);
    if (scheduled && world_room_index(scheduled) == room) return base;
    return "";
  }
  for (i = 0; i < sizeof NPC_ROUTINES / sizeof NPC_ROUTINES[0]; i++) {
    const char *room_slug = npc_routine_room_special(&NPC_ROUTINES[i], &wc);
    int rr = room_slug ? world_room_index(room_slug) : -1;
    if (rr == room) return NPC_ROUTINES[i].slug;
  }
  return "";
}

static void get_world_clock(AetWorldClock *out) { world_clock_for_turn(g_turns, out); }

static const char *game_mgt_weather_string(void) {
  AetWorldClock wc;
  get_world_clock(&wc);
  return wc.weather && wc.weather[0] ? wc.weather : "clear";
}

static void note_npc_routine_changes(int prev_turn, int new_turn) {
  size_t i;
  if (new_turn <= prev_turn) return;
  for (i = 0; i < sizeof NPC_ROUTINES / sizeof NPC_ROUTINES[0]; i++) {
    const char *was = npc_room_slug_for_turn(NPC_ROUTINES[i].slug, prev_turn);
    const char *now = npc_room_slug_for_turn(NPC_ROUTINES[i].slug, new_turn);
    if (was && now && !str_ieq(was, now)) {
      char detail[200];
      AetWorldClock wc;
      world_clock_for_turn(new_turn, &wc);
      snprintf(detail, sizeof detail, "%s: %s -> %s (%s)", NPC_ROUTINES[i].slug,
               was, now, wc.period);
      causal_push("npc-routine", detail);
    }
  }
}

static void format_clock_time(char *out, size_t cap, const AetWorldClock *wc) {
  int h12 = wc->hour % 12;
  const char *ampm = wc->hour < 12 ? "AM" : "PM";
  if (h12 == 0) h12 = 12;
  snprintf(out, cap, "%d:%02d %s", h12, wc->minute, ampm);
}

static void expand_temporal_mod_text(const char *src, char *dst, size_t dstcap) {
  AetWorldClock wc;
  char timestr[40];
  char daystr[24];
  char degstr[16];
  char a[4096], b[4096];
  const char *per, *seas, *wth;
  if (!dst || dstcap < 2) return;
  dst[0] = '\0';
  if (!src || !src[0]) return;
  if (strlen(src) >= sizeof a) {
    (void)snprintf(dst, dstcap, "%.*s", (int)(dstcap - 1), src);
    return;
  }
  get_world_clock(&wc);
  format_clock_time(timestr, sizeof timestr, &wc);
  (void)snprintf(daystr, sizeof daystr, "%d", wc.day);
  (void)snprintf(degstr, sizeof degstr, "%d", wc.temp_c);
  per = wc.period ? wc.period : "";
  seas = wc.season ? wc.season : "";
  wth = wc.weather ? wc.weather : "";
  memcpy(a, src, strlen(src) + 1);
  mod_replace_token_pass(a, b, sizeof b, "%TIME%", timestr);
  mod_replace_token_pass(b, a, sizeof a, "%PERIOD%", per);
  mod_replace_token_pass(a, b, sizeof b, "%DAY%", daystr);
  mod_replace_token_pass(b, a, sizeof a, "%SEASON%", seas);
  mod_replace_token_pass(a, b, sizeof b, "%WEATHER%", wth);
  mod_replace_token_pass(b, a, sizeof a, "%TEMPC%", degstr);
  (void)snprintf(dst, dstcap, "%s", a);
}

static const char *topic_mood_for(const char *topic);
static const char *topic_heat_for(const char *topic_mood, int risk);
static const char *npc_trust_for(const char *slug, const char *role, int risk);
static const char *npc_leverage_for(const char *slug, const char *role, int risk);
static void expand_state_mod_text(const char *src, char *dst, size_t dstcap) {
  char coinsb[24], purseb[48], purseshortb[24], hpbuf[16], maxhpbuf[16],
      scorebuf[16], turnsbuf[16];
  char invcbuf[16], invmaxbuf[16], packbuf[40];
  char readybuf[MAX_ITEM_LEN + 4];
  char npcbuf[MAX_ITEM_LEN + 4], focusbuf[MAX_ITEM_LEN + 4];
  char lasttopicb[AETER_LAST_TOPIC_CAP + 8], lasttopicmoodb[32], topicheatb[16];
  char worldroomsb[16], visitedb[16], notesb[16], exploreb[16];
  char strb[12], agib[12], intb[12], wisb[12], chab[12], corb[12];
  char powerb[12], resolveb[12], cunningb[12], presenceb[12], hppctb[12];
  char riskb[12], temperb[12], archetypeb[40], risklabelb[20];
  char voicestyleb[24], moralb[24], threatb[24];
  char scenetoneb[32], travelmoodb[32], socialstanceb[32];
  char npchereb[MAX_ITEM_LEN + 4], npcpresenceb[16], lastnpchereb[8];
  char npcdisplayb[64], npcroleb[24], npcdangerb[20], npctrustb[24], npcleverageb[24];
  char npcpretty[64], lastnpcpretty[64];
  char lastnpcdisplayb[64], lastnpcroleb[24], lastnpcattitudeb[24], lastnpcdangerb[20], lastnpctrustb[24], lastnpcleverageb[24];
  char ageb[32], genderb[24], classb[40], raceb[48], buildb[32], muscleb[32];
  char vq[32];
  const char *cortier;
  int visited_n, explore_pct, hp_pct;
  int power, resolve, cunning, presence, risk;
  const char *temper;
  const char *archetype;
  const char *risk_label;
  const char *voice_style;
  const char *moral_vector;
  const char *threat_posture;
  const char *scene_tone;
  const char *travel_mood;
  const char *social_stance;
  const char *npc_here;
  const char *npc_presence;
  const char *last_npc_here;
  const char *npc_role;
  const char *npc_danger;
  const char *npc_trust;
  const char *npc_leverage;
  const char *last_npc_role;
  const char *last_npc_attitude;
  const char *last_npc_danger;
  const char *last_npc_trust;
  const char *last_npc_leverage;
  const char *topic_mood;
  const char *topic_heat;
  const char *modestr;
  AetWorldClock wc;
  AetPcSave ps;
  char a[4096], b[4096];
  if (!dst || dstcap < 2) return;
  dst[0] = '\0';
  if (!src || !src[0]) return;
  if (strlen(src) >= sizeof a) {
    (void)snprintf(dst, dstcap, "%.*s", (int)(dstcap - 1), src);
    return;
  }
  (void)snprintf(coinsb, sizeof coinsb, "%d", g_coins);
  currency_format_compact(g_coins, purseb, sizeof purseb);
  currency_format_compact(g_coins, purseshortb, sizeof purseshortb);
  (void)snprintf(hpbuf, sizeof hpbuf, "%d", g_health);
  (void)snprintf(maxhpbuf, sizeof maxhpbuf, "%d", g_max_health);
  (void)snprintf(scorebuf, sizeof scorebuf, "%d", g_score);
  (void)snprintf(turnsbuf, sizeof turnsbuf, "%d", g_turns);
  (void)snprintf(invcbuf, sizeof invcbuf, "%d", g_inv_n);
  (void)snprintf(invmaxbuf, sizeof invmaxbuf, "%d", MAX_INV);
  (void)snprintf(packbuf, sizeof packbuf, "%d/%d", g_inv_n, MAX_INV);
  if (g_ready_item[0] && inv_has(g_ready_item))
    (void)snprintf(readybuf, sizeof readybuf, "%s", g_ready_item);
  else
    (void)snprintf(readybuf, sizeof readybuf, "%s", "—");
  if (g_last_npc[0])
    (void)snprintf(npcbuf, sizeof npcbuf, "%s", g_last_npc);
  else
    (void)snprintf(npcbuf, sizeof npcbuf, "%s", "—");
  if (g_last_focus[0] && inv_has(g_last_focus))
    (void)snprintf(focusbuf, sizeof focusbuf, "%s", g_last_focus);
  else
    (void)snprintf(focusbuf, sizeof focusbuf, "%s", "—");
  if (g_last_topic[0])
    (void)snprintf(lasttopicb, sizeof lasttopicb, "%s", g_last_topic);
  else
    (void)snprintf(lasttopicb, sizeof lasttopicb, "%s", "—");
  npc_here = world_room_entity(g_room);
  if (!npc_here || !npc_here[0]) npc_here = "—";
  if (strcmp(npc_here, "—") != 0)
    entity_pretty(npc_here, npcpretty, sizeof npcpretty);
  else
    (void)snprintf(npcpretty, sizeof npcpretty, "%s", "—");
  if (strcmp(npc_here, "—") == 0)
    npc_role = "none";
  else if (str_ieq(npc_here, "miller") || str_ieq(npc_here, "blacksmith") ||
           str_ieq(npc_here, "merchant") || str_ieq(npc_here, "trader") ||
           str_ieq(npc_here, "shopkeeper"))
    npc_role = "merchant";
  else if (strstr(npc_here, "guard") || strstr(npc_here, "sentinel"))
    npc_role = "guard";
  else
    npc_role = "local";
  if (g_last_npc[0])
    entity_pretty(g_last_npc, lastnpcpretty, sizeof lastnpcpretty);
  else
    (void)snprintf(lastnpcpretty, sizeof lastnpcpretty, "%s", "—");
  if (!g_last_npc[0])
    last_npc_role = "none";
  else if (str_ieq(g_last_npc, "miller") || str_ieq(g_last_npc, "blacksmith") ||
           str_ieq(g_last_npc, "merchant") || str_ieq(g_last_npc, "trader") ||
           str_ieq(g_last_npc, "shopkeeper"))
    last_npc_role = "merchant";
  else if (strstr(g_last_npc, "guard") || strstr(g_last_npc, "sentinel"))
    last_npc_role = "guard";
  else
    last_npc_role = "local";
  if (g_last_npc[0] && strcmp(npc_here, "—") != 0 && str_ieq(g_last_npc, npc_here)) {
    npc_presence = "present";
    last_npc_here = "yes";
  } else if (g_last_npc[0]) {
    npc_presence = (strcmp(npc_here, "—") != 0) ? "absent" : "none";
    last_npc_here = "no";
  } else {
    npc_presence = "unknown";
    last_npc_here = "no";
  }
  modestr = g_verbose_room ? "verbose" : "brief";
  visited_n = count_visited();
  explore_pct = WORLD_ROOM_COUNT > 0 ? (visited_n * 100) / WORLD_ROOM_COUNT : 0;
  pc_capture(&ps);
  pc_fill_narrative_defaults(&ps);
  (void)snprintf(worldroomsb, sizeof worldroomsb, "%d", WORLD_ROOM_COUNT);
  (void)snprintf(visitedb, sizeof visitedb, "%d", visited_n);
  (void)snprintf(notesb, sizeof notesb, "%d", g_note_n);
  (void)snprintf(exploreb, sizeof exploreb, "%d", explore_pct);
  (void)snprintf(strb, sizeof strb, "%d", ps.str);
  (void)snprintf(agib, sizeof agib, "%d", ps.agi);
  (void)snprintf(intb, sizeof intb, "%d", ps.intl);
  (void)snprintf(wisb, sizeof wisb, "%d", ps.wis);
  (void)snprintf(chab, sizeof chab, "%d", ps.cha);
  (void)snprintf(corb, sizeof corb, "%d", ps.cor);
  power = (ps.str + ps.agi + ps.tou) / 3;
  resolve = (ps.will + ps.wis + ps.tou) / 3;
  cunning = (ps.intl + ps.per + ps.agi) / 3;
  presence = (ps.cha + ps.per + ps.will) / 3;
  hp_pct = g_max_health > 0 ? (g_health * 100) / g_max_health : 0;
  if (hp_pct < 0) hp_pct = 0;
  if (hp_pct > 100) hp_pct = 100;
  risk = ps.cor + (100 - hp_pct) / 10;
  topic_mood = topic_mood_for(g_last_topic);
  topic_heat = topic_heat_for(topic_mood, risk);
  (void)snprintf(lasttopicmoodb, sizeof lasttopicmoodb, "%s", topic_mood);
  (void)snprintf(topicheatb, sizeof topicheatb, "%s", topic_heat);
  if (ps.cor <= 5)
    cortier = "clear";
  else if (ps.cor <= 10)
    cortier = "touched";
  else if (ps.cor <= 15)
    cortier = "stained";
  else
    cortier = "fallen";
  temper = resolve >= 15   ? "steady"
           : resolve >= 11 ? "driven"
                           : "volatile";
  if (power >= 14 && resolve >= 13)
    archetype = "vanguard";
  else if (cunning >= 14 && presence >= 12)
    archetype = "schemer";
  else if (presence >= 14)
    archetype = "envoy";
  else if (cunning >= 14)
    archetype = "seer";
  else
    archetype = "drifter";
  risk_label = risk <= 8    ? "low"
               : risk <= 16 ? "medium"
                            : "high";
  get_world_clock(&wc);
  {
    size_t i = 0;
    while (ps.voice_quality[i] && i + 1 < sizeof vq) {
      vq[i] = (char)tolower((unsigned char)ps.voice_quality[i]);
      i++;
    }
    vq[i] = '\0';
  }
  if (strstr(vq, "rasp") || strstr(vq, "gruff"))
    voice_style = "rough";
  else if (strstr(vq, "breath"))
    voice_style = "soft";
  else if (strstr(vq, "melod"))
    voice_style = "lyrical";
  else
    voice_style = "plain";
  moral_vector = ps.cor <= 5    ? "upright"
                 : ps.cor <= 10 ? "tempted"
                 : ps.cor <= 15 ? "compromised"
                                : "predatory";
  if (hp_pct < 35)
    threat_posture = "fragile";
  else if (power >= 14 && resolve >= 13)
    threat_posture = "dominant";
  else if (cunning >= 14)
    threat_posture = "opportunistic";
  else if (risk >= 17)
    threat_posture = "desperate";
  else
    threat_posture = "measured";
  if (str_ieq(wc.weather, "storm"))
    scene_tone = "ominous";
  else if (str_ieq(wc.weather, "rain"))
    scene_tone = "somber";
  else if (str_ieq(wc.weather, "fog"))
    scene_tone = "uncertain";
  else if (str_ieq(wc.weather, "snow"))
    scene_tone = "hushed";
  else if (str_ieq(wc.period, "night"))
    scene_tone = "tense";
  else
    scene_tone = "clear";
  if (explore_pct >= 80)
    travel_mood = "well-mapped";
  else if (explore_pct >= 45)
    travel_mood = "seasoned";
  else if (visited_n >= 8)
    travel_mood = "probing";
  else
    travel_mood = "uncharted";
  if (presence >= 14 && !str_ieq(moral_vector, "predatory"))
    social_stance = "diplomatic";
  else if (str_ieq(moral_vector, "predatory"))
    social_stance = "coercive";
  else if (str_ieq(threat_posture, "fragile"))
    social_stance = "guarded";
  else
    social_stance = "transactional";
  if (str_ieq(npc_role, "none"))
    npc_danger = "none";
  else if (str_ieq(npc_role, "guard"))
    npc_danger = risk >= 14 ? "high" : "medium";
  else if (str_ieq(npc_role, "merchant"))
    npc_danger = (risk >= 18 || str_ieq(moral_vector, "predatory")) ? "medium" : "low";
  else
    npc_danger = risk >= 17 ? "medium" : "low";
  npc_trust = npc_trust_for(npc_here, npc_role, risk);
  npc_leverage = npc_leverage_for(npc_here, npc_role, risk);
  if (!g_last_npc[0])
    last_npc_attitude = "unknown";
  else if (strcmp(last_npc_here, "yes") == 0 && str_ieq(last_npc_role, "merchant"))
    last_npc_attitude = "open";
  else if (str_ieq(last_npc_role, "guard"))
    last_npc_attitude = risk >= 16 ? "hostile" : "wary";
  else if (risk >= 18 || str_ieq(moral_vector, "predatory"))
    last_npc_attitude = "suspicious";
  else if (strcmp(last_npc_here, "yes") == 0)
    last_npc_attitude = "engaged";
  else
    last_npc_attitude = "distant";
  if (str_ieq(last_npc_role, "none"))
    last_npc_danger = "none";
  else if (str_ieq(last_npc_role, "guard"))
    last_npc_danger = risk >= 14 ? "high" : "medium";
  else if (str_ieq(last_npc_role, "merchant"))
    last_npc_danger = (risk >= 18 || str_ieq(moral_vector, "predatory")) ? "medium" : "low";
  else
    last_npc_danger = risk >= 17 ? "medium" : "low";
  last_npc_trust = npc_trust_for(g_last_npc, last_npc_role, risk);
  last_npc_leverage = npc_leverage_for(g_last_npc, last_npc_role, risk);
  (void)snprintf(powerb, sizeof powerb, "%d", power);
  (void)snprintf(resolveb, sizeof resolveb, "%d", resolve);
  (void)snprintf(cunningb, sizeof cunningb, "%d", cunning);
  (void)snprintf(presenceb, sizeof presenceb, "%d", presence);
  (void)snprintf(hppctb, sizeof hppctb, "%d", hp_pct);
  (void)snprintf(riskb, sizeof riskb, "%d", risk);
  (void)snprintf(temperb, sizeof temperb, "%s", temper);
  (void)snprintf(archetypeb, sizeof archetypeb, "%s", archetype);
  (void)snprintf(risklabelb, sizeof risklabelb, "%s", risk_label);
  (void)snprintf(voicestyleb, sizeof voicestyleb, "%s", voice_style);
  (void)snprintf(moralb, sizeof moralb, "%s", moral_vector);
  (void)snprintf(threatb, sizeof threatb, "%s", threat_posture);
  (void)snprintf(scenetoneb, sizeof scenetoneb, "%s", scene_tone);
  (void)snprintf(travelmoodb, sizeof travelmoodb, "%s", travel_mood);
  (void)snprintf(socialstanceb, sizeof socialstanceb, "%s", social_stance);
  (void)snprintf(npchereb, sizeof npchereb, "%s", npc_here);
  (void)snprintf(npcpresenceb, sizeof npcpresenceb, "%s", npc_presence);
  (void)snprintf(lastnpchereb, sizeof lastnpchereb, "%s", last_npc_here);
  (void)snprintf(npcdisplayb, sizeof npcdisplayb, "%s", npcpretty);
  (void)snprintf(npcroleb, sizeof npcroleb, "%s", npc_role);
  (void)snprintf(npcdangerb, sizeof npcdangerb, "%s", npc_danger);
  (void)snprintf(npctrustb, sizeof npctrustb, "%s", npc_trust);
  (void)snprintf(npcleverageb, sizeof npcleverageb, "%s", npc_leverage);
  (void)snprintf(lastnpcdisplayb, sizeof lastnpcdisplayb, "%s", lastnpcpretty);
  (void)snprintf(lastnpcroleb, sizeof lastnpcroleb, "%s", last_npc_role);
  (void)snprintf(lastnpcattitudeb, sizeof lastnpcattitudeb, "%s", last_npc_attitude);
  (void)snprintf(lastnpcdangerb, sizeof lastnpcdangerb, "%s", last_npc_danger);
  (void)snprintf(lastnpctrustb, sizeof lastnpctrustb, "%s", last_npc_trust);
  (void)snprintf(lastnpcleverageb, sizeof lastnpcleverageb, "%s", last_npc_leverage);
  (void)snprintf(ageb, sizeof ageb, "%s", ps.age[0] ? ps.age : "—");
  (void)snprintf(genderb, sizeof genderb, "%s", ps.gender[0] ? ps.gender : "they");
  (void)snprintf(classb, sizeof classb, "%s",
                 ps.class_[0] ? ps.class_ : "adventurer");
  (void)snprintf(raceb, sizeof raceb, "%s", ps.race[0] ? ps.race : "Human");
  (void)snprintf(buildb, sizeof buildb, "%s", ps.build[0] ? ps.build : "average");
  (void)snprintf(muscleb, sizeof muscleb, "%s",
                 ps.muscle_tone[0] ? ps.muscle_tone : "average");
  memcpy(a, src, strlen(src) + 1);
  mod_replace_token_pass(a, b, sizeof b, "%WORLDROOMS%", worldroomsb);
  mod_replace_token_pass(b, a, sizeof a, "%VISITED%", visitedb);
  mod_replace_token_pass(a, b, sizeof b, "%NOTECOUNT%", notesb);
  mod_replace_token_pass(b, a, sizeof a, "%EXPLORE%", exploreb);
  mod_replace_token_pass(a, b, sizeof b, "%STR%", strb);
  mod_replace_token_pass(b, a, sizeof a, "%AGI%", agib);
  mod_replace_token_pass(a, b, sizeof b, "%INT%", intb);
  mod_replace_token_pass(b, a, sizeof a, "%WIS%", wisb);
  mod_replace_token_pass(a, b, sizeof b, "%CHA%", chab);
  mod_replace_token_pass(b, a, sizeof a, "%COR%", corb);
  mod_replace_token_pass(a, b, sizeof b, "%POWER%", powerb);
  mod_replace_token_pass(b, a, sizeof a, "%RESOLVE%", resolveb);
  mod_replace_token_pass(a, b, sizeof b, "%CUNNING%", cunningb);
  mod_replace_token_pass(b, a, sizeof a, "%PRESENCE%", presenceb);
  mod_replace_token_pass(a, b, sizeof b, "%HPPCT%", hppctb);
  mod_replace_token_pass(b, a, sizeof a, "%CORTIER%", cortier);
  mod_replace_token_pass(a, b, sizeof b, "%CORSTATE%", cortier);
  mod_replace_token_pass(b, a, sizeof a, "%RISK%", riskb);
  mod_replace_token_pass(a, b, sizeof b, "%TEMPER%", temperb);
  mod_replace_token_pass(b, a, sizeof a, "%ARCHETYPE%", archetypeb);
  mod_replace_token_pass(a, b, sizeof b, "%RISKLABEL%", risklabelb);
  mod_replace_token_pass(b, a, sizeof a, "%VOICESTYLE%", voicestyleb);
  mod_replace_token_pass(a, b, sizeof b, "%MORALVECTOR%", moralb);
  mod_replace_token_pass(b, a, sizeof a, "%THREATPOSTURE%", threatb);
  mod_replace_token_pass(a, b, sizeof b, "%SCENETONE%", scenetoneb);
  mod_replace_token_pass(b, a, sizeof a, "%TRAVELMOOD%", travelmoodb);
  mod_replace_token_pass(a, b, sizeof b, "%SOCIALSTANCE%", socialstanceb);
  mod_replace_token_pass(b, a, sizeof a, "%NPCHERE%", npchereb);
  mod_replace_token_pass(a, b, sizeof b, "%NPCPRESENCE%", npcpresenceb);
  mod_replace_token_pass(b, a, sizeof a, "%LASTNPCHERE%", lastnpchereb);
  mod_replace_token_pass(a, b, sizeof b, "%NPCDISPLAY%", npcdisplayb);
  mod_replace_token_pass(b, a, sizeof a, "%NPCROLE%", npcroleb);
  mod_replace_token_pass(a, b, sizeof b, "%NPCDANGER%", npcdangerb);
  mod_replace_token_pass(b, a, sizeof a, "%NPCTRUST%", npctrustb);
  mod_replace_token_pass(a, b, sizeof b, "%NPCLEVERAGE%", npcleverageb);
  mod_replace_token_pass(b, a, sizeof a, "%LASTNPCDISPLAY%", lastnpcdisplayb);
  mod_replace_token_pass(a, b, sizeof b, "%LASTNPCROLE%", lastnpcroleb);
  mod_replace_token_pass(b, a, sizeof a, "%LASTNPCATTITUDE%", lastnpcattitudeb);
  mod_replace_token_pass(a, b, sizeof b, "%LASTNPCDANGER%", lastnpcdangerb);
  mod_replace_token_pass(b, a, sizeof a, "%LASTNPCTRUST%", lastnpctrustb);
  mod_replace_token_pass(a, b, sizeof b, "%LASTNPCLEVERAGE%", lastnpcleverageb);
  mod_replace_token_pass(b, a, sizeof a, "%AGE%", ageb);
  mod_replace_token_pass(a, b, sizeof b, "%GENDER%", genderb);
  mod_replace_token_pass(b, a, sizeof a, "%CLASSID%", classb);
  mod_replace_token_pass(a, b, sizeof b, "%RACEID%", raceb);
  mod_replace_token_pass(b, a, sizeof a, "%BUILD%", buildb);
  mod_replace_token_pass(a, b, sizeof b, "%MUSCLE%", muscleb);
  mod_replace_token_pass(b, a, sizeof a, "%LASTFOCUS%", focusbuf);
  mod_replace_token_pass(a, b, sizeof b, "%LASTTOPIC%", lasttopicb);
  mod_replace_token_pass(b, a, sizeof a, "%LASTTOPICMOOD%", lasttopicmoodb);
  mod_replace_token_pass(a, b, sizeof b, "%TOPICHEAT%", topicheatb);
  mod_replace_token_pass(b, a, sizeof a, "%LASTNPC%", npcbuf);
  mod_replace_token_pass(a, b, sizeof b, "%ROOMMODE%", modestr);
  mod_replace_token_pass(b, a, sizeof a, "%MAXHP%", maxhpbuf);
  mod_replace_token_pass(a, b, sizeof b, "%INVMAX%", invmaxbuf);
  mod_replace_token_pass(b, a, sizeof a, "%INVCOUNT%", invcbuf);
  mod_replace_token_pass(a, b, sizeof b, "%COINS%", coinsb);
  mod_replace_token_pass(b, a, sizeof a, "%PURSE%", purseb);
  mod_replace_token_pass(a, b, sizeof b, "%PURSESHORT%", purseshortb);
  mod_replace_token_pass(b, a, sizeof a, "%HP%", hpbuf);
  mod_replace_token_pass(a, b, sizeof b, "%SCORE%", scorebuf);
  mod_replace_token_pass(b, a, sizeof a, "%TURNS%", turnsbuf);
  mod_replace_token_pass(a, b, sizeof b, "%PACK%", packbuf);
  mod_replace_token_pass(b, a, sizeof a, "%READIED%", readybuf);
  (void)snprintf(dst, dstcap, "%s", a);
}

static void expand_mod_overlay_flat(const char *suffix, char *out, size_t outcap) {
  char t1[4096], t2[4096], t3[4096];
  if (!out || outcap < 2) return;
  out[0] = '\0';
  if (!suffix || !suffix[0]) return;
  pc_expand_world_placeholders(suffix, t1, sizeof t1);
  expand_scene_mod_text(t1, t2, sizeof t2);
  expand_temporal_mod_text(t2, t3, sizeof t3);
  expand_state_mod_text(t3, out, outcap);
}

static void append_dlc_mod_to_body(char *body, size_t cap, const char *suffix) {
  char xp[4096];
  if (!body || cap < 64 || !suffix || !suffix[0]) return;
  expand_mod_overlay_flat(suffix, xp, sizeof xp);
  body_append(body, cap, "\n\n--- DLC / mod ---\n%s", xp);
}

static const char *weather_detail(const AetWorldClock *wc) {
  if (str_ieq(wc->weather, "clear")) {
    if (str_ieq(wc->period, "morning")) return "The light is clean and bright.";
    if (str_ieq(wc->period, "night")) return "The sky is open and still.";
    return "The sky is clear over Hollow Ridge.";
  }
  if (str_ieq(wc->weather, "rain"))
    return "Rain darkens the paths and makes old wood smell alive.";
  if (str_ieq(wc->weather, "fog"))
    return "Fog softens the edges of the road and swallows distant shapes.";
  if (str_ieq(wc->weather, "snow"))
    return "Snow hushes the world and gathers in pale seams.";
  return "Storm pressure hangs low; every exposed hinge and branch complains.";
}

static void format_time_body(char *body, size_t cap) {
  AetWorldClock wc;
  char t[32];
  char banner[256];
  get_world_clock(&wc);
  format_clock_time(t, sizeof t, &wc);
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(body, cap,
           "%s\n\n"
           "Time: %s (%s)\nDay: %d\nSeason: %s\nTurns: %d\n",
           banner, t, wc.period, wc.day, wc.season, g_turns);
}

static int parse_clock_hour(const char *raw, int *hour) {
  char buf[64];
  char *p = buf;
  char *colon;
  char *end = NULL;
  long h;
  int pm = 0, am = 0;
  if (!raw) return 0;
  while (*raw == ' ') raw++;
  strncpy(buf, raw, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  strip_trailing_space(buf);
  if (!buf[0]) return 0;
  if (strstr(buf, "pm")) pm = 1;
  if (strstr(buf, "am")) am = 1;
  colon = strchr(buf, ':');
  if (colon) *colon = '\0';
  h = strtol(p, &end, 10);
  if (!end || end == p) return 0;
  if (pm || am) {
    if (h < 1 || h > 12) return 0;
    h %= 12;
    if (pm) h += 12;
  } else {
    if (h < 0 || h > 23) return 0;
  }
  *hour = (int)h;
  return 1;
}

static int hours_until_hour(int now_hour, int target_hour) {
  int d = (target_hour - now_hour + 24) % 24;
  return d;
}

static void format_weather_body(char *body, size_t cap, const char *arg) {
  AetWorldClock wc;
  char t[32];
  char banner[256];
  int f;
  get_world_clock(&wc);
  format_clock_time(t, sizeof t, &wc);
  pc_format_identity_banner(banner, sizeof banner);
  f = wc.temp_c * 9 / 5 + 32;
  snprintf(body, cap,
           "Weather\n\n"
           "%s\n\n"
           "Time: %s (%s)\n",
           banner, t, wc.period);
  body_append(
      body, cap,
      "Conditions: %s\n"
      "Season: %s\n"
      "Temperature: %dC / %dF\n"
      "Detail: %s\n",
      wc.weather, wc.season, wc.temp_c, f, weather_detail(&wc));
  if (arg && (str_ieq(arg, "forecast") || str_ieq(arg, "report") ||
              str_ieq(arg, "full"))) {
    char line[128];
    snprintf(line, sizeof line,
             "\nForecast: this pattern should hold for about %d hour%s.\n",
             wc.forecast_hours, wc.forecast_hours == 1 ? "" : "s");
    strncat(body, line, cap - strlen(body) - 1);
  }
  if (arg && (str_ieq(arg, "impact") || str_ieq(arg, "effects"))) {
    const char *visibility = "normal";
    const char *travel = "normal";
    const char *gathering = "normal";
    if (str_ieq(wc.weather, "fog")) {
      visibility = "reduced";
    } else if (str_ieq(wc.weather, "storm")) {
      visibility = "heavily reduced";
      travel = "hazardous";
      gathering = "reduced";
    } else if (str_ieq(wc.weather, "snow")) {
      visibility = "slightly reduced";
      travel = "slow and cold";
      gathering = "reduced";
    } else if (str_ieq(wc.weather, "rain")) {
      travel = "wet and slippery";
      gathering = "slightly reduced";
    }
    snprintf(body + strlen(body), cap - strlen(body),
             "\nImpact: visibility=%s, travel=%s, gathering=%s.\n",
             visibility, travel, gathering);
  }
}

static void format_temperature_msg(char *msg, size_t msgcap, const char *arg) {
  AetWorldClock wc;
  int f;
  const char *feel = "mild";
  const char *impact = "Conditions are comfortable for travel and work.";
  get_world_clock(&wc);
  f = wc.temp_c * 9 / 5 + 32;
  if (wc.temp_c < 0) {
    feel = "freezing";
    impact = "Extreme cold makes exposed travel harder.";
  } else if (wc.temp_c < 5) {
    feel = "very cold";
    impact = "Cold air slows careful outdoor work.";
  } else if (wc.temp_c < 12) {
    feel = "cool";
    impact = "Cool weather has only minor impact.";
  } else if (wc.temp_c > 35) {
    feel = "extreme heat";
    impact = "Severe heat makes exertion harder.";
  } else if (wc.temp_c > 30) {
    feel = "hot";
    impact = "High heat slightly taxes long travel.";
  } else if (wc.temp_c > 24) {
    feel = "warm";
    impact = "Warm weather has minimal penalties.";
  }
  if (arg && (str_ieq(arg, "f") || str_ieq(arg, "fahrenheit")))
    snprintf(msg, msgcap, "Temperature: %dF (%s). %s", f, feel, impact);
  else if (arg && (str_ieq(arg, "c") || str_ieq(arg, "celsius")))
    snprintf(msg, msgcap, "Temperature: %dC (%s). %s", wc.temp_c, feel,
             impact);
  else
    snprintf(msg, msgcap, "Temperature: %dC / %dF (%s). %s", wc.temp_c, f,
             feel, impact);
}

static const char *dir_name(int dir) {
  return (dir >= 0 && dir < DIR_COUNT) ? DIR_LABELS[dir] : "?";
}

static int has_hunting_weapon(void) {
  return inv_has("bow") || inv_has("shortbow") || inv_has("hunting_bow");
}

static int has_lockpick_tool(void) {
  return inv_has("lockpick") || inv_has("fine_lockpick") ||
         inv_has("rusty_pick") || inv_has("skeleton_key");
}

static const char *best_lock_tool(void) {
  if (inv_has("skeleton_key")) return "skeleton_key";
  if (inv_has("fine_lockpick")) return "fine_lockpick";
  if (inv_has("lockpick")) return "lockpick";
  if (inv_has("rusty_pick")) return "rusty_pick";
  return "none";
}

#ifdef AETER_MINIGAMES
static int count_adjacent_npcs(void);
static int npc_in_adjacent_room(const char *slug) {
  AetWorldClock wc;
  size_t i;
  int d;
  if (!slug || !slug[0]) return 0;
  get_world_clock(&wc);
  for (i = 0; i < sizeof NPC_ROUTINES / sizeof NPC_ROUTINES[0]; i++) {
    const char *rs = npc_routine_room_special(&NPC_ROUTINES[i], &wc);
    int rr;
    if (!NPC_ROUTINES[i].slug || !str_ieq(NPC_ROUTINES[i].slug, slug)) continue;
    if (!rs) continue;
    rr = world_room_index(rs);
    if (rr < 0) continue;
    for (d = 0; d < DIR_COUNT; d++) {
      if (world_exit(g_room, d) == rr) return 1;
    }
  }
  return 0;
}

static void game_mgt_apply_lockpick_stealth(MgtPersistentState *st, char *msg,
                                            size_t cap) {
  char tail[360];
  const char *band;
  int heard;
  int si;
  unsigned char f0;
  if (!st) return;
  tail[0] = '\0';
  band = st->lock_noise_band[0] ? st->lock_noise_band : "LOW";
  g_lockpick_last_noise = st->lock_exit_noise;
  if (st->lock_exit_noise >= 70 || st->lock_exit_misses >= 2)
    g_lockpick_suspicion += (st->lock_exit_noise >= 70 ? 3 : 1);
  else if (st->lock_exit_noise >= 35)
    g_lockpick_suspicion += 1;
  if (g_lockpick_suspicion > 10) g_lockpick_suspicion = 10;

  if (st->lock_pick_broken) {
    const char *tool = best_lock_tool();
    if (tool && strcmp(tool, "skeleton_key") && inv_remove_exact_one(tool)) {
      snprintf(tail, sizeof tail,
               "\n\nYour %s snaps and is ruined — removed from your pack.",
               tool);
    }
  }

  heard = count_adjacent_npcs() > 0 || npc_in_adjacent_room("village_guard");
  if ((st->lock_exit_noise >= 70 || st->lock_exit_misses >= 2) && heard) {
    if (npc_in_adjacent_room("village_guard")) {
      si = soc_npc_ensure("village_guard");
      if (si >= 0) {
        f0 = g_soc_npc_friendship[si];
        if ((int)g_soc_npc_friendship[si] > 2)
          g_soc_npc_friendship[si] =
              (unsigned char)((int)g_soc_npc_friendship[si] - 5);
        rel_hist_push("village_guard", "lockpick alert",
                      (int)g_soc_npc_friendship[si] - (int)f0, 0, 0);
      }
      snprintf(tail + strlen(tail), sizeof tail - strlen(tail),
               "%s%sFrom the square, the village guard calls out: "
               "\"Who's fiddling with that lock?\"",
               tail[0] ? "" : "\n\n", tail[0] ? "" : "");
    } else {
      snprintf(tail + strlen(tail), sizeof tail - strlen(tail),
               "%s%sA muffled voice nearby: \"Did you hear metal on metal?\"",
               tail[0] ? "" : "\n\n", tail[0] ? "" : "");
    }
    causal_push("lockpick-noise", band);
  } else if (st->lock_exit_noise >= 35 && st->last_success != 1) {
    snprintf(tail + strlen(tail), sizeof tail - strlen(tail),
             "%s%sYou freeze — the %s scrape might have carried.",
             tail[0] ? "" : "\n\n", tail[0] ? "" : "", band);
    causal_push("lockpick-noise", band);
  } else if (st->lock_exit_noise >= 35 && st->last_success == 1) {
    snprintf(tail + strlen(tail), sizeof tail - strlen(tail),
             "%s%sThe lock yields with only %s noise.",
             tail[0] ? "" : "\n\n", tail[0] ? "" : "", band);
  }

  if (tail[0] && msg && cap) {
    size_t used = strlen(msg);
    if (used && used + 1 < cap) {
      snprintf(msg + used, cap - used, "%s", tail);
    } else if (cap > 1) {
      snprintf(msg, cap, "%s%s", msg[0] ? msg : st->last_banner, tail);
    }
  }
}
#endif

static int exit_lock_info(int room, int dir, char *name, size_t namecap,
                          int *locked, int *difficulty) {
  const char *slug;
  if (locked) *locked = 0;
  if (difficulty) *difficulty = 0;
  if (name && namecap) name[0] = '\0';
  if (room < 0 || room >= WORLD_ROOM_COUNT) return 0;
  slug = world_slug(room);
  if (!strcmp(slug, "front_door") && dir == DIR_E) {
    if (name && namecap) snprintf(name, namecap, "front door");
    if (locked) *locked = !g_front_unlocked;
    if (difficulty) *difficulty = 2;
    return 1;
  }
  if (!strcmp(slug, "east_of_house") && dir == DIR_E) {
    if (name && namecap) snprintf(name, namecap, "shed lock");
    if (locked) *locked = !g_shed_unlocked;
    if (difficulty) *difficulty = 3;
    return 1;
  }
  return 0;
}

static int count_adjacent_locks(int only_locked) {
  int d, n = 0;
  for (d = 0; d < DIR_COUNT; d++) {
    int locked = 0;
    if (world_exit(g_room, d) < 0) continue;
    if (!exit_lock_info(g_room, d, NULL, 0, &locked, NULL)) continue;
    if (only_locked && !locked) continue;
    n++;
  }
  return n;
}

static int count_adjacent_npcs(void) {
  int d, n = 0;
  for (d = 0; d < DIR_COUNT; d++) {
    int dest = world_exit(g_room, d);
    const char *ent;
    if (dest < 0) continue;
    ent = world_room_entity(dest);
    if (ent && ent[0]) n++;
  }
  return n;
}

static void body_append_people_here(char *body, size_t cap) {
  const char *primary = world_room_entity(g_room);
  char others[320];
  char pretty[96];
  AetWorldClock wc;
  size_t i, olen = 0;
  int n_other = 0;

  others[0] = '\0';
  get_world_clock(&wc);
  for (i = 0; i < sizeof NPC_ROUTINES / sizeof NPC_ROUTINES[0]; i++) {
    const char *slug = NPC_ROUTINES[i].slug;
    const char *rs = npc_routine_room_special(&NPC_ROUTINES[i], &wc);
    int rr;
    char op[64];
    if (!rs) continue;
    rr = world_room_index(rs);
    if (rr != g_room) continue;
    if (primary && primary[0] && str_ieq(slug, primary)) continue;
    entity_pretty(slug, op, sizeof op);
    if (olen + strlen(op) + 4 >= sizeof others) continue;
    if (n_other > 0) {
      others[olen++] = ',';
      others[olen++] = ' ';
    }
    olen += snprintf(others + olen, sizeof others - olen, "%s", op);
    n_other++;
  }
  if (primary && primary[0]) {
    entity_pretty(primary, pretty, sizeof pretty);
    if (n_other > 0)
      body_append(body, cap,
                  "\nPeople here: %s\nAlso scheduled here (one visible in this "
                  "build): %s\n",
                  pretty, others);
    else
      body_append(body, cap, "\nPeople here: %s\n", pretty);
  } else if (n_other > 0) {
    body_append(body, cap,
                "\nPeople here: none visible\nAlso scheduled here: %s\n",
                others);
  } else {
    body_append(body, cap, "\nPeople here: none\n");
  }
}

static void format_exits(char *buf, size_t cap) {
  int d;
  int w;
  size_t len;
  w = snprintf(buf, cap, "%s",
               room_too_dark_to_see() ? "You sense openings:" : "Obvious exits:");
  if (w < 0) {
    if (cap) buf[0] = '\0';
    return;
  }
  len = (size_t)w;
  for (d = 0; d < DIR_COUNT; d++) {
    if (world_exit(g_room, d) >= 0) {
      w = snprintf(buf + len, cap > len ? cap - len : 0, " %s", dir_name(d));
      if (w < 0) return;
      len += (size_t)w;
      if (len >= cap) return;
    }
  }
  if (len + 2 < cap) {
    buf[len++] = '\n';
    buf[len] = '\0';
  } else if (cap)
    buf[cap - 1] = '\0';
}

static int exit_mode_matches(const char *mode, int dest, int lock_known,
                             int locked) {
  const char *ent;
  if (!mode || !mode[0]) return 1;
  if (!strcmp(mode, "locked")) return lock_known && locked;
  if (!strcmp(mode, "open")) return !lock_known || !locked;
  if (!strcmp(mode, "new")) return !g_visited[dest];
  if (!strcmp(mode, "visited")) return g_visited[dest];
  ent = world_room_entity(dest);
  if (!strcmp(mode, "npc")) return ent && ent[0];
  if (!strcmp(mode, "safe")) return (!lock_known || !locked) && g_visited[dest] &&
                                   (!ent || !ent[0]);
  return 1;
}

static void format_exits_screen(char *body, size_t cap, const char *mode) {
  char ex[512];
  char banner[256];
  int d, shown = 0;
  format_exits(ex, sizeof ex);
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(body, cap, "%s\n\n%s\n[%s]\n\n%s\n", banner,
           resolve_world_title(g_room), world_slug(g_room), ex);
  if (mode && mode[0])
    body_append(body, cap, "Filter: %s\n\n", mode);
  else
    body_append(body, cap,
                "Use exits locked/open/new/visited/npc/safe for filters.\n\n");
  for (d = 0; d < DIR_COUNT; d++) {
    int dest = world_exit(g_room, d);
    int locked = 0, diff = 0, lock_known;
    int npc_n = 0;
    char lname[64];
    char npc_tag[16];
    const char *ent;
    if (dest < 0) continue;
    lock_known = exit_lock_info(g_room, d, lname, sizeof lname, &locked, &diff);
    if (!exit_mode_matches(mode, dest, lock_known, locked)) continue;
    ent = world_room_entity(dest);
    npc_n = (ent && ent[0]) ? 1 : 0;
    npc_tag[0] = '\0';
    if (npc_n > 0) snprintf(npc_tag, sizeof npc_tag, " [npc:%d]", npc_n);
    body_append(body, cap, "  %-10s %-28s [%s]  %s%s%s%s\n", dir_name(d),
                resolve_world_title(dest), world_slug(dest),
                lock_known ? (locked ? "[locked]" : "[open]") : "[open]",
                g_visited[dest] ? " [visited]" : " [new]",
                npc_tag,
                (lock_known && locked) ? " [tool?]" : "");
    shown++;
  }
  if (!shown) {
    body_append(body, cap, "  (No exits match this filter.)\n");
  }
}

static void show_room(void) {
  int i, d, ex_n = 0, rows, right_n;
  int ex_dirs[DIR_COUNT];
  const char *reg = world_region(g_room);
  printf("%s%s%s\n", C_BORDER, UI_RULE, C_RESET);
  printf("  %s%s%s\n", C_TITLE, resolve_world_title(g_room), C_RESET);
  printf("%s%s%s\n", C_BORDER, UI_RULE, C_RESET);
  if (reg[0]) printf("  %s[ Region: %s ]%s\n", C_REGION, reg, C_RESET);
  printf("\n");
  if (room_too_dark_to_see()) {
    print_wrapped_paragraph(
        "Pitch black. Without a torch, lantern, candle, or similar you cannot "
        "read the space - only sound, air, and the floor underfoot. You can "
        "still guess where passages lie.",
        2, 80);
  } else {
    const char *bl = resolve_world_blurb(g_room);
    if (g_verbose_room)
      print_wrapped_paragraph(bl && bl[0] ? bl : "", 2, 80);
    else {
      char shorty[200];
      size_t k, maxc = 160;
      if (!bl) bl = "";
      for (k = 0; bl[k] && k < maxc && k + 1 < sizeof shorty; k++) {
        shorty[k] = (bl[k] == '\n' || bl[k] == '\r') ? ' ' : bl[k];
      }
      shorty[k] = '\0';
      if (bl[k]) strncat(shorty, " ...", sizeof shorty - strlen(shorty) - 1);
      print_wrapped_paragraph(shorty, 2, 80);
    }
  }
  {
    const char *ent = world_room_entity(g_room);
    if (ent[0]) {
      if (room_too_dark_to_see())
        print_wrapped_paragraph(
            "Someone else is very close - you hear movement. Try 'talk'.", 2,
            80);
      else {
        char pretty[128];
        char line[220];
        entity_pretty(ent, pretty, sizeof pretty);
        snprintf(line, sizeof line, "Also here: %s", pretty);
        print_wrapped_paragraph(line, 2, 80);
      }
    }
  }

  for (d = 0; d < DIR_COUNT; d++) {
    if (world_exit(g_room, d) >= 0) ex_dirs[ex_n++] = d;
  }
  right_n = room_too_dark_to_see() ? 0 : g_room_item_n[g_room];
  rows = ex_n > right_n ? ex_n : right_n;

  printf("%s%s%s\n", C_BORDER, UI_RULE_LIGHT, C_RESET);
  printf("  %s[ EXITS ]%s                       %s|%s  %s[ VISIBLE ITEMS ]%s\n",
         C_HEADING, C_RESET, C_BORDER, C_RESET, C_HEADING, C_RESET);
  for (i = 0; i < rows; i++) {
    char left[40] = "";
    char right[40] = "";
    if (i < ex_n) {
      int dd = ex_dirs[i];
      snprintf(left, sizeof left, "%-3s- %s", dir_short_name(dd), dir_name(dd));
    }
    if (i < right_n) {
      char pretty[32];
      item_pretty(g_room_items[g_room][i], pretty, sizeof pretty);
      snprintf(right, sizeof right, "* %s", pretty);
    } else if (room_too_dark_to_see() && i == 0) {
      snprintf(right, sizeof right, "(hidden by darkness)");
    }
    printf("  %s%-30s%s %s|%s  %s%-30s%s\n", C_EXIT, left, C_RESET, C_BORDER,
           C_RESET, C_ITEM, right, C_RESET);
  }
  if (rows == 0) {
    printf("  %-30s %s|%s  %-30s\n", "(no obvious exits)", C_BORDER, C_RESET,
           "(no visible items)");
  }
  printf("%s%s%s\n", C_BORDER, UI_RULE, C_RESET);
  printf("%s[ Turn: %d | HP: %d/%d | Score: %d | Light: %s | Rooms: %d | [help] [menu] [recap] ]%s\n",
         C_STATUS,
         g_turns,
         g_health,
         g_max_health,
         g_score,
         room_too_dark_to_see()
             ? "OFF"
             : (world_room_is_dark(g_room) ? "ON (dark room)" : "ON"),
         WORLD_ROOM_COUNT, C_RESET);
  {
    char hud[96];
    pc_format_hud_tag(hud, sizeof hud);
    printf("  %s%s%s\n", C_MUTED, hud, C_RESET);
  }
  printf("%s%s%s\n", C_BORDER, UI_RULE, C_RESET);
}

static void format_room_description_body(char *body, size_t cap) {
  const char *reg = world_region(g_room);
  const char *bl = resolve_world_blurb(g_room);
  size_t len;
  char banner[256];
  int w;
  pc_format_identity_banner(banner, sizeof banner);
  w = snprintf(body, cap, "%s\n\n%s\n[%s]\n", banner, resolve_world_title(g_room),
                     world_slug(g_room));
  if (w < 0) return;
  len = (size_t)w;
  if (reg[0]) {
    w = snprintf(body + len, cap > len ? cap - len : 0, "(%s)\n\n", reg);
    if (w < 0) return;
    len += (size_t)w;
  } else {
    if (len + 2 < cap) {
      body[len++] = '\n';
      body[len] = '\0';
    }
  }
  if (room_too_dark_to_see()) {
    snprintf(body + len, cap > len ? cap - len : 0,
             "[Pitch black. You cannot read the space without a light source. "
             "Exits still work; try listen, search, or move.]\n");
  } else {
    snprintf(body + len, cap > len ? cap - len : 0, "%s", bl ? bl : "");
  }
}

static void paint_normal(void) {
  clear_frame();
  show_room();
  if (g_transcript[0]) printf("%s\n", g_transcript);
  fflush(stdout);
}

static void return_to_game_screen(void) {
  paint_normal();
}

static void recap_push(const char *text) {
  const char *p;
  size_t L;
  if (!text || !text[0]) return;
  if (g_recap_n >= RECAP_MAX) {
    memmove(g_recap[0], g_recap[1],
            (size_t)(RECAP_MAX - 1) * sizeof g_recap[0]);
    g_recap_n = RECAP_MAX - 1;
  }
  L = 0;
  for (p = text; *p && L + 1 < RECAP_W; p++) {
    if (*p == '\n') {
      if (L + 3 < RECAP_W) {
        g_recap[g_recap_n][L++] = ' ';
        g_recap[g_recap_n][L++] = '|';
        g_recap[g_recap_n][L++] = ' ';
      }
    } else
      g_recap[g_recap_n][L++] = (char)*p;
  }
  g_recap[g_recap_n][L] = '\0';
  g_recap_n++;
}

static void format_recap_body(char *body, size_t cap) {
  int i;
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  if (g_recap_n <= 0) {
    snprintf(body, cap,
             "%s\n\n"
             "(No recap yet — commands that produced text are logged here.)\n",
             banner);
    return;
  }
  snprintf(body, cap,
           "%s\n\n"
           "Recent output (oldest first, max %d lines):\n\n",
           banner, RECAP_MAX);
  for (i = 0; i < g_recap_n; i++) {
    char ln[RECAP_W + 48];
    snprintf(ln, sizeof ln, "  [%d] %.*s\n", i + 1, RECAP_W - 1, g_recap[i]);
    strncat(body, ln, cap - strlen(body) - 1);
  }
}

static void ui_block_pause(const char *title, const char *body) {
  char buf[64];
  clear_frame();
  ui_print_title(title);
  printf("%s", body);
  ui_print_panel_footer("[Press Enter]");
  fflush(stdout);
  if (!aet_autotest()) (void)fgets(buf, sizeof buf, stdin);
}

static void age_disclaimer_wait(void) {
  char buf[64];
  static const char text[] =
      "Aeternitas is the 18+ edition of this work.\n\n"
      "It may include mature themes, strong language, sexual content,\n"
      "violence, and other material not suitable for minors.\n\n"
      "By continuing you confirm that you are at least 18 years old (or the age\n"
      "of majority where you live) and that viewing such content is lawful for\n"
      "you. If that is not true, close this program now.\n";
  
  clear_frame();
  ui_print_title("18+ ADULT CONTENT");
  printf("%s", text);
  ui_print_panel_footer("[Press Enter]");
  fflush(stdout);
  if (!aet_autotest()) (void)fgets(buf, sizeof buf, stdin);
}

#define UI_SCROLL_PAGE_LINES 22
#define UI_SCROLL_AUTO_THRESHOLD 24

static void ui_transcript_snapshot(char *snap, size_t cap,
                                   const char *pending_acc) {
  const char *src =
      (pending_acc && pending_acc[0]) ? pending_acc : g_transcript;
  if (!snap || cap < 2) return;
  snprintf(snap, cap, "%s", src ? src : "");
}

static void ui_transcript_restore(const char *snap) {
  if (!snap) return;
  snprintf(g_transcript, sizeof g_transcript, "%s", snap);
}

static void ui_fullscreen_plain(const char *title, const char *body,
                                const char *pending_acc, int *did_fullscreen) {
  char snap[sizeof g_transcript];
  ui_transcript_snapshot(snap, sizeof snap, pending_acc);
  clear_frame();
  ui_print_title(title);
  printf("%s", body ? body : "");
  ui_print_panel_footer("[Press Enter to return]");
  fflush(stdout);
  if (!aet_autotest()) {
    char b[256];
    (void)fgets(b, sizeof b, stdin);
  }
  ui_transcript_restore(snap);
  if (did_fullscreen) *did_fullscreen = 1;
  return_to_game_screen();
}

static void ui_fullscreen(const char *title, const char *body,
                          const char *pending_acc, int *did_fullscreen) {
  if (body && body[0] &&
      guide_line_count(body) > UI_SCROLL_AUTO_THRESHOLD) {
    ui_scrollable_panel(title, body, pending_acc, did_fullscreen);
    return;
  }
  ui_fullscreen_plain(title, body, pending_acc, did_fullscreen);
}

typedef enum {
  GK_QUIT = 0,
  GK_UP,
  GK_DOWN,
  GK_PGUP,
  GK_PGDN,
  GK_HOME,
  GK_END,
  GK_TAB,
  GK_LEFT,
  GK_RIGHT,
  GK_ENTER,
  GK_ESC,
  GK_NONE
} GuideKey;

#if !defined(_WIN32)
static struct termios g_guide_save_termios;
static int g_guide_term_saved;

static int guide_tty_raw_begin(void) {
  struct termios t;
  if (!isatty(STDIN_FILENO)) return 0;
  if (tcgetattr(STDIN_FILENO, &g_guide_save_termios) != 0) return 0;
  t = g_guide_save_termios;
  t.c_lflag &= (tcflag_t)~(ICANON | ECHO);
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &t) != 0) return 0;
  g_guide_term_saved = 1;
  return 1;
}

static void guide_tty_raw_end(void) {
  if (g_guide_term_saved) {
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &g_guide_save_termios);
    g_guide_term_saved = 0;
  }
}

static int guide_read_byte_raw(void) {
  unsigned char c;
  ssize_t n;
  n = read(STDIN_FILENO, &c, 1);
  if (n != 1) return -1;
  return (int)c;
}

static GuideKey guide_read_key_unix(int raw_ok) {
  int c;
  if (!raw_ok) {
    char ln[48];
    if (!fgets(ln, sizeof ln, stdin)) return GK_QUIT;
    if (ln[0] == 'q' || ln[0] == 'Q') return GK_QUIT;
    if (ln[0] == 'j' || ln[0] == 'J') return GK_DOWN;
    if (ln[0] == 'k' || ln[0] == 'K') return GK_UP;
    if (ln[0] == 'n' || ln[0] == 'N') return GK_PGDN;
    if (ln[0] == 'p' || ln[0] == 'P') return GK_PGUP;
    if (ln[0] == '\n' || ln[0] == '\r') return GK_ENTER;
    return GK_NONE;
  }
  c = guide_read_byte_raw();
  if (c < 0) return GK_QUIT;
  if (c == 'q' || c == 'Q' || c == 3 || c == 4) return GK_QUIT;
  if (c == '\r' || c == '\n') return GK_ENTER;
  if (c == 9) return GK_TAB;
  if (c == 'j' || c == 'J') return GK_DOWN;
  if (c == 'k' || c == 'K') return GK_UP;
  if (c == 'n' || c == 'N') return GK_PGDN;
  if (c == 'p' || c == 'P') return GK_PGUP;
  if (c == 27) {
    fd_set rfds;
    struct timeval tv = {0, 50000};
    int b1, b2, last;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    if (select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) <= 0)
      return GK_ESC;
    b1 = guide_read_byte_raw();
    if (b1 < 0) return GK_QUIT;
    if (b1 != (int)'[') return GK_NONE;
    b2 = guide_read_byte_raw();
    if (b2 < 0) return GK_QUIT;
    if (b2 == 'A') return GK_UP;
    if (b2 == 'B') return GK_DOWN;
    if (b2 == 'C') return GK_RIGHT;
    if (b2 == 'D') return GK_LEFT;
    if (b2 == 'H') return GK_HOME;
    if (b2 == 'F') return GK_END;
    if (b2 >= '0' && b2 <= '9') {
      last = b2;
      for (;;) {
        int nx = guide_read_byte_raw();
        if (nx < 0) return GK_QUIT;
        if (nx == '~') {
          if (last == '5') return GK_PGUP;
          if (last == '6') return GK_PGDN;
          return GK_NONE;
        }
        last = nx;
      }
    }
  }
  return GK_NONE;
}
#endif

#if defined(_WIN32)
static GuideKey guide_read_key_win(void) {
  int c = _getch();
  if (c == 0 || c == 224) {
    int c2 = _getch();
    switch (c2) {
      case 72:
        return GK_UP;
      case 80:
        return GK_DOWN;
      case 73:
        return GK_PGUP;
      case 81:
        return GK_PGDN;
      case 71:
        return GK_HOME;
      case 79:
        return GK_END;
      case 75:
        return GK_LEFT;
      case 77:
        return GK_RIGHT;
      default:
        return GK_NONE;
    }
  }
  if (c == 9) return GK_TAB;
  if (c == '\r' || c == '\n') return GK_ENTER;
  if (c == 27) return GK_ESC;
  if (c == 3) return GK_QUIT;
  if (c == 'j' || c == 'J') return GK_DOWN;
  if (c == 'k' || c == 'K') return GK_UP;
  if (c == 'n' || c == 'N') return GK_PGDN;
  if (c == 'p' || c == 'P') return GK_PGUP;
  if (c == 'q' || c == 'Q') return GK_QUIT;
  return GK_NONE;
}
#endif

static int guide_line_count(const char *s) {
  int n = 1;
  if (!s || !s[0]) return 1;
  for (; *s; s++)
    if (*s == '\n') n++;
  return n;
}

static const char *guide_skip_lines(const char *s, int skip) {
  if (!s || skip <= 0) return s;
  for (; *s && skip > 0; s++) {
    if (*s == '\n') skip--;
  }
  return s;
}

static void guide_print_lines(const char *start, int max_lines) {
  int printed = 0;
  if (!start) return;
  for (; *start && printed < max_lines; start++) {
    putchar((unsigned char)*start);
    if (*start == '\n') printed++;
  }
}

static void ui_scrollable_panel(const char *title, const char *body,
                                const char *pending_acc, int *did_fullscreen) {
  char snap[sizeof g_transcript];
  int total;
  int scroll = 0;
  int max_scroll;
#if !defined(_WIN32)
  int raw_ok = 0;
#endif

  if (!body) body = "";
  ui_transcript_snapshot(snap, sizeof snap, pending_acc);
  total = guide_line_count(body);
  max_scroll =
      total > UI_SCROLL_PAGE_LINES ? total - UI_SCROLL_PAGE_LINES : 0;

  if (aet_autotest()) {
    char prev[4800];
    size_t i, lim = sizeof prev - 200;
    for (i = 0; body[i] && i + 1 < lim; i++) prev[i] = body[i];
    prev[i] = '\0';
    strncat(prev,
            "\n\n[CI autotest: scroll pager skipped in this mode.]\n",
            sizeof prev - strlen(prev) - 1);
    ui_fullscreen_plain(title ? title : "SCREEN", prev, pending_acc,
                        did_fullscreen);
    return;
  }

#if !defined(_WIN32)
  raw_ok = guide_tty_raw_begin();
#endif

  for (;;) {
    GuideKey gk;
    int row_first = scroll + 1;
    int row_last = scroll + UI_SCROLL_PAGE_LINES;
    if (row_last > total) row_last = total;
    clear_frame();
    ui_print_title(title ? title : "SCREEN");
    printf("%s", C_MUTED);
    printf("  Up/Down  PgUp/PgDn  Home/End    also j/k n/p    Q or Enter quit\n");
    printf("%s", C_RESET);
    guide_print_lines(guide_skip_lines(body, scroll), UI_SCROLL_PAGE_LINES);
    printf("\n%s--- Lines %d-%d of %d ---%s\n", C_MUTED, row_first, row_last,
           total, C_RESET);
    fflush(stdout);
#if defined(_WIN32)
    gk = guide_read_key_win();
#else
    gk = guide_read_key_unix(raw_ok);
#endif
    if (gk == GK_QUIT || gk == GK_ENTER) break;
    if (gk == GK_UP) {
      if (scroll > 0) scroll--;
    } else if (gk == GK_DOWN) {
      if (scroll < max_scroll) scroll++;
    } else if (gk == GK_PGUP) {
      scroll -= UI_SCROLL_PAGE_LINES;
      if (scroll < 0) scroll = 0;
    } else if (gk == GK_PGDN) {
      scroll += UI_SCROLL_PAGE_LINES;
      if (scroll > max_scroll) scroll = max_scroll;
    } else if (gk == GK_HOME) {
      scroll = 0;
    } else if (gk == GK_END) {
      scroll = max_scroll;
    }
  }
#if !defined(_WIN32)
  guide_tty_raw_end();
#endif
  ui_transcript_restore(snap);
  if (did_fullscreen) *did_fullscreen = 1;
  return_to_game_screen();
}

static void ui_modding_guide_pager(const char *pending_acc, int *did_fullscreen) {
  ui_scrollable_panel("MODDING & DLC GUIDE", aet_mod_guide_full_text(),
                      pending_acc, did_fullscreen);
}

static void fill_help_smart_hint(char *buf, size_t cap) {
  const char *e;
  if (!buf || cap < 32) return;
  if (room_too_dark_to_see()) {
    snprintf(
        buf, cap,
        "Smart hint: You are in darkness without a light source in your pack — "
        "equip or hold a lantern, torch, candle, etc. (see lights), or retreat to "
        "lit rooms.\n\n");
    return;
  }
  e = world_room_entity(g_room);
  if (e && e[0]) {
    snprintf(buf, cap,
             "Smart hint: Someone may be here — try who, listen, or talk about "
             "<topic> / talk to <name>.\n\n");
    return;
  }
  snprintf(buf, cap,
           "Smart hint: Try exits for directions, look or examine for detail, scan "
           "for a room digest, or unstick / hint when stuck.\n\n");
}

static void fill_help_text(char *buf, size_t cap) {
  fill_help_smart_hint(buf, cap);
  body_append(
      buf, cap,
      "Command groups: Movement · Navigation · Look & status · Inventory · "
      "Trade · Craft · Time · Notes · Parser · Debug · Saves · Mods\n\n"
      "Tip: after you close this screen, type  help modding  for the scrollable\n"
      "DLC / modding guide (arrow keys, PgUp/Dn; Q or Enter to leave).\n\n"
      "Aeternitas64 — text adventure (%d locations).\n"
      "  Travel: directions (n/s/e/w…; northward/southwards…; diagonals + -ward forms),\n"
      "          go/walk/run <dir>, go <n> <dir> (max 50),\n"
      "          approach | goto | walk to | enter <place> — one step if adjacent\n"
      "          or <n> <dir> / <dir> <n> — same; special exits as in exits.\n"
      "  back [n] | trail [n] | trail stats | trail clear\n"
      "  waypoints | waystones | nexus  —  attuned travel points (travel network…)\n"
      "  fasttravel <waypoint> | waystone <name> | nexus <name>  —  travel from one\n"
      "  nearby [detail|npc|locked] | map  —  adjacent rooms\n"
      "  exits [locked|open|new|visited|npc|safe]; route <slug/place> — path\n"
      "  scan  —  tactical room readout (recon, survey room, scout…);  loot |\n"
      "          loot value | loot weight\n"
      "  compare <a> / <b>  —  quick appraisal (also: compare <a> vs <b>)\n"
      "  where | whereami  —  here;  where <name> | locate <name>  —  NPC rooms\n"
      "  look | l  — room overview;  look/l <thing> | look at / examine /\n"
      "          inspect / x <thing>  (x me / self — compact sheet)\n"
      "          read <item>  — open literature in pack or room (books, scrolls…)\n"
      "  exits | status | stat (quick status, character status…) | character |\n"
      "          sheet (full portrait) |\n"
      "          character brief | sheet brief | identity (compact sheet); who am i |\n"
      "          appearance | my character | describe me | look at me  (aliases → "
      "character)\n"
      "          skills | skill | aptitudes | aptitude | reputation | rep | standing\n"
      "          loadout | gear | equipment | outfit — 120-col equipment & pack UI\n"
      "          traits | trait | personality\n"
      "          momentum | arc | progression | perks | perk\n"
      "          voice | speech | vocals | pronouns | bio | backstory | biography\n"
      "          tainting | corruption | taint | rapport | relationships | bonds\n"
      "          relationship history [npc] | bond history [npc]\n"
      "          vitals | wellness | hp  (focused health;  status  for full snapshot)\n"
      "          progress | visited | seen (sitrep, world progress…) |\n"
      "          journal (quest log, diary, logbook…) | objectives | goals | quests\n"
      "  inventory | i | inv | pack — equipment UI;  inventory list  for full pack\n"
      "  inventory sort name|weight|type  |  inventory find <text>\n"
      "  score  (my score, game stats…)\n"
      "  take | get | grab <item> | (get|grab|take) all [except a,b…]\n",
      WORLD_ROOM_COUNT);
  body_append(
      buf, cap,
      "  wares | shop | prices  —  merchant lists in g/s/b/c;  buy | purchase | sell <item>\n"
      "          price compare <item> | compare prices <item>  —  cross-shop buy quotes\n"
      "          buy/sell all [except a,b…]; haggle | barter [buy|sell] <item>\n"
      "          patron score and CHA tilt base prices; haggling quotes one item at a time\n"
      "          trade history | trade log | transactions  —  completed buy/sell ledger\n"
      "  coins | wallet | purse  —  purse in gold / silver / bronze / copper; bare buy/sell opens prices\n"
      "  eat | taste <item> | drink | sip <item>  —  consume pack staples\n"
      "  find <item|npc>  —  scan visible items, pack, and known NPC rooms; point me to /\n"
      "          where was / show me where to find … normalize here\n"
      "  describe | blurb | room  —  fullscreen room text (surroundings, full room…)\n"
      "  drop <item> | drop all | drop all except <...> | put down <item>\n"
      "  give <item>  (someone here)  |  listen | smell\n"
      "  unlock door | use key / lockpick / food or drink | open door |\n"
      "          pick lock (shed + lockpick); use bandage | use bucket |\n"
      "          fill bucket (well) | break bucket\n"
      "  lockcheck  —  nearby lock readiness;  noise | stealth | suspicion\n"
      "  intent | tone  —  show parsed modifier profile (quiet/loud/friendly/harsh)\n"
      "  forge | crafting  —  a workbench (experiment; no recipe book)\n"
#ifdef AETER_MINIGAMES
      "  Minigames (ASCII fullscreen; ESC returns):  play piano (tavern);\n"
      "          pick lock (shed);  fish (pond, piers);  farm;  cook (kitchen);\n"
      "          gamble (tavern);  hunt (forest);  write;  read <item>\n"
#endif
      );
  body_append(
      buf, cap,
      "  time | clock (what time is it…) | time until <morning|HH:MM> |\n"
      "  weather (what's the weather, climate…) [forecast|impact] | temperature [c|f]\n"
      "  wait | wait <n> | wait <n> hours | wait until <period> | rest until <period>\n"
      "  z | rest | sleep\n"
      "  notes add/show/delete/done/undone/find/stats/purge done | note … |\n"
      "          jot … | jot down … | memo … | remember that …  —  same as note\n"
      "          notes clear\n"
      "  g | again | repeat  —  repeat last input (full chain);  chain with ; or \"then\"\n"
      "  history | !<n>  —  numbered command recall (session);  history <n>  re-runs entry n\n"
      "  utilities | room objects  —  fixtures here (fireplace, well, forge, lights…)\n"
      "  Parser: deterministic only (no model guessing): fillers/synonyms; bare i→inventory;\n"
      "  go <dir> alone→move; trailing thanks/please; unique near-verb autofix (edit distance,\n"
      "  ties and short tokens rejected; rest preserved); where can i find / point me to /\n"
      "  can i see / tell-style phrases; show me / interact with / discuss / chat about …\n"
      "  Plain English, questions, and prepositions are normalized where sensible\n"
      "  (e.g. instructions, connections, breadcrumbs, shopping list, market,\n"
      "   my saves, my notebook, i need a hint, troubleshoot, map completion,\n"
      "   adjacent rooms, lock status, room overview, light sources, who made this,\n"
      "   who is here, who did i last meet, last npc attitude, npc danger,\n"
      "   topic, topic mood, topic heat, npc trust, npc leverage, conversation topic, salvage)\n"
      "  it | that | this  —  last item (take/drop) or last NPC (after who/talk)\n"
      "  lights  —  light ids; equip/hold/wield <item> | unequip | take off\n"
      "  search | scan | loot [value|weight] | compare <a>/<b> | who |\n"
      "          who all | who global  —  routine-aware world NPC scan | talk |\n"
      "          npc schedule <npc>  —  morning/afternoon/evening/night plan |\n"
      "          talk about <topic> | talk to <who> about <topic>\n"
      "          (after talk, numbered menu — reply 1–5, a label, or  goodbye )\n"
      "  topic | last topic | topic mood | topic heat | npc trust | npc leverage  —  last successful\n"
      "          talk-about phrase (mods: %%LASTTOPIC%% %%LASTTOPICMOOD%% %%TOPICHEAT%%)\n"
      "  say | shout | read | touch <thing>\n");
  body_append(
      buf, cap,
      "  unstick | hints | hint | nudge | give me a hint  —  situational nudges;\n"
      "          errors | healthcheck | diag  —  session log; errors clear\n"
      "          causality [term] | because [term]  —  recent cause/effect traces;\n"
      "          causality movement/social/save/parser | causality recent | why blocked\n"
      "          causality turn <n> | causality lastturn | what changed last turn\n"
      "          causality explain | what triggered that\n"
      "          failed actions now include \"Because\" hints when context is known\n"
      "          causality clear\n");
  body_append(
      buf, cap,
      "  save | quick save | qs   load | quick load | reload | ql | restore\n"
      "  autosave on|off | load autosave  —  shadow backup beside quicksave each turn\n"
      "  saves | slots — fullscreen save manager (save/load/del N); also save <1-10> | "
      "load <1-10>\n"
      "  menu  —  ASCII pause menu; resume clears it and redraws this room\n"
      "  Long panels (help, journal, status, …) scroll with arrows / PgUp/PgDn when "
      "needed.\n"
      "  verbose | brief  —  main-window blurbs; readied item saved in qs\n"
      "  recap | transcript (what just happened, recent messages…) | recap clear\n"
      "  lore [topic]  —  Veritasfurtum setting primer (veritasfurtum, architect, "
      "hollow, …)\n"
      "  about | credits | version | ver  —  port notes / build info\n"
      "  mods reload  —  rescan data packs (path: see help modding / --mods)\n"
      "  mods list     —  show pack load order (priority / DLC debugging)\n"
      "  mods doctor   —  verify/repair runtime mod files and show bootstrap health\n"
      "  mods doctor verbose  —  same, plus full mod status summary\n"
      "  mods directory <path>  —  set mod pack root for this session, then reload\n"
      "  mods may extend the workbench with extra profiles (undocumented in-game)\n"
      "  help modding  —  DLC / modding guide (arrow keys, PgUp/Dn; Q quits)\n"
      "  --------------------------------------------------------------------\n"
      "  DLC drops: folders under mods/ with manifest priority=; last load wins.\n"
      "  First run creates 000_aeternitas_sample (tutorial); see PACK_GUIDE.txt.\n"
      "  Aliases: get/grab/snag/lift/pluck/recover/pocket/scoop/pick up…->take, "
      "put down/deposit/stash…->drop\n"
      "  Dark rooms: light source (see status).  clear | cls\n");
}

static void format_lights_body(char *buf, size_t cap) {
  int i, any = 0;
  size_t len;
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(buf, cap,
           "%s\n\n"
           "Items in your pack that this port treats as light sources:\n\n",
           banner);
  len = strlen(buf);
  for (i = 0; i < g_inv_n; i++) {
    int w;
    if (!item_id_is_light(g_inv[i])) continue;
    char pretty[96];
    item_pretty(g_inv[i], pretty, sizeof pretty);
    w = snprintf(buf + len, cap > len ? cap - len : 0, "  - %s\n", pretty);
    if (w < 0 || (size_t)w >= cap - len) break;
    len += (size_t)w;
    any = 1;
  }
  if (!any) {
    snprintf(buf + len, cap > len ? cap - len : 0,
             "  (none — dark rooms stay unreadable until you find one.)\n");
    len = strlen(buf);
  }
  if (g_ready_item[0] && inv_has(g_ready_item)) {
    snprintf(buf + len, cap > len ? cap - len : 0,
             "\nReadied (flavor): %s\n", g_ready_item);
  }
}

static void format_about_body(char *buf, size_t cap) {
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(
      buf, cap,
      "Aeternitas64 (stdin port)\n\n"
      "%s\n\n"
      "  World data is compiled into fixed C tables for this build — "
      "rooms,\n",
      banner);
  body_append(
      buf, cap,
      "  exits, items, and room entities — not a full scripted quest system.\n"
      "  Verbs are a pragmatic subset: travel, inventory, search, simple locks,\n"
      "  notes, routing hints, light heuristics by item id, and basic health "
      "(see status).\n\n"
      "  Setting: you are in Hollow Ridge, a region of Veritasfurtum — see  lore  "
      "for canon.\n"
      "  World size: %d locations in this build.\n",
      WORLD_ROOM_COUNT);
}

static void format_lore_body(char *body, size_t cap, const char *topic) {
  char t[64];
  if (!body || cap < 256) return;
  t[0] = '\0';
  if (topic && topic[0]) {
    size_t n = strnlen(topic, sizeof t - 1);
    memcpy(t, topic, n);
    t[n] = '\0';
    for (n = 0; t[n]; n++) t[n] = (char)tolower((unsigned char)t[n]);
    strip_leading_articles(t);
    strip_trailing_space(t);
  }
  if (!t[0] || !strcmp(t, "help") || !strcmp(t, "list")) {
    snprintf(body, cap,
             "Lore primer (Veritasfurtum canon)\n\n"
             "Topics:  lore veritasfurtum  |  lore architect  |  lore hollow\n"
             "         lore cosmology  |  lore csa  |  lore currency\n"
             "         lore genetrix  |  lore seraphine  |  lore missionaries\n"
             "         lore waystone  |  lore elysium  |  lore house\n"
             "         lore artifacts  |  lore rift  |  lore thin\n\n"
             "Veritasfurtum is the name of this universe — a creation of the "
             "Architect,\n"
             "now unstable: rifts, stolen fragments of other realities, and "
             "thin places\n"
             "where rules disagree. Your journey in this build centers on "
             "Hollow Ridge,\n"
             "a lived-in region of manor, village, temple, and wild country "
             "(not the whole\n"
             "multiverse at once).\n\n"
             "Find deeper prose on scrolls and in the world; this screen is a "
             "compass, not\n"
             "a spoiler walkthrough.\n");
    return;
  }
  if (!strcmp(t, "veritasfurtum") || !strcmp(t, "setting") ||
      !strcmp(t, "world")) {
    snprintf(body, cap,
             "Veritasfurtum\n\n"
             "The canonical name of Maddeline's universe — a cosmos the "
             "Architect imagined\n"
             "into being, then entered as mortal ruler of Amethystus. Without "
             "constant divine\n"
             "attention the weave frays: entropy, temporal slips, and "
             "dimensional rifts.\n\n"
             "Pieces of other realities sometimes overlap or duplicate into "
             "this world. Travel\n"
             "between unstable layers without authority is dangerous; agencies "
             "like the CSA exist\n"
             "to police the worst crossings.\n\n"
             "Hollow Ridge is one region you can walk in this port — a local "
             "chart, not the\n"
             "entirety of Veritasfurtum.\n");
    return;
  }
  if (!strcmp(t, "architect") || !strcmp(t, "maddeline") ||
      !strcmp(t, "empress")) {
    snprintf(body, cap,
             "The Architect\n\n"
             "A being of unfathomable vision who spoke a universe into shape — "
             "laws, galaxies,\n"
             "and countless lives. Longing to *feel* what had been made, the "
             "Architect took mortal\n"
             "form as Empress Maddeline of Amethystus: kind, progressive, and "
             "beloved, often\n"
             "appearing in her natural form as a symbol of bodily autonomy and "
             "freedom.\n\n"
             "Creation exacts a price. Even Architects answer to cosmic law; "
             "when Maddeline walked\n"
             "among mortals, divine guidance thinned and Veritasfurtum began "
             "its slow unraveling.\n"
             "Temples to the Architect in Hollow Ridge still carry that echo — "
             "reverence mixed with\n"
             "unease at a parent who stepped away.\n");
    return;
  }
  if (!strcmp(t, "hollow") || !strcmp(t, "hollow ridge") ||
      !strcmp(t, "ridge")) {
    snprintf(body, cap,
             "Hollow Ridge\n\n"
             "A region of Veritasfurtum mapped in this build: manor and "
             "outbuildings, village\n"
             "square and shops, tavern life, Temple of the Architect, mills, "
             "forest, ridge, and\n"
             "cave. Room text tags the region as Hollow Ridge throughout.\n\n"
             "Themes from the export: a boarded great house; commerce and "
             "patron NPCs; waystones\n"
             "and Elysium crystals on the ridge; rumors of things that move "
             "between stones without\n"
             "crossing open ground.\n\n"
             "This port compiles %d locations here — enough to explore, not "
             "every land named in\n"
             "the wider lore bible.\n",
             WORLD_ROOM_COUNT);
    return;
  }
  if (!strcmp(t, "rift") || !strcmp(t, "rifts") || !strcmp(t, "thin") ||
      !strcmp(t, "thin places") || !strcmp(t, "fray")) {
    snprintf(body, cap,
             "Rifts and thin places\n\n"
             "When Maddeline walked among mortals, divine attention thinned and "
             "Veritasfurtum\n"
             "began to bruise. Rifts are not random holes — they are arguments "
             "between layers\n"
             "of reality that no longer agree.\n\n"
             "Thin places feel like déjà vu with teeth: wrong weather, doubled "
             "footsteps,\n"
             "memories that belong to someone else's life. Nexus points and "
             "waystones are\n"
             "meant to cross safely; rifts are where safety was never negotiated.\n\n"
             "Hollow Ridge has its share. Treat odd silence and overlapping "
             "scenery as warnings,\n"
             "not scenery.\n");
    return;
  }
  if (!strcmp(t, "cosmology") || !strcmp(t, "void") ||
      !strcmp(t, "drops") || !strcmp(t, "nexus")) {
    snprintf(body, cap,
             "Cosmology — Universe Drops\n\n"
             "In the Void, reality gathers as Universe Drops: whole cosmoses "
             "held by dark matter's\n"
             "tension like droplets on still water. Inside each drop, "
             "dimensions layer — facets of one\n"
             "jewel, each with its own rules, sometimes touching at Nexus "
             "Points where guardians,\n"
             "crystals, or accident keep the borders from tearing.\n\n"
             "Characters may hail from different layers; powers and "
             "perspectives follow the layer\n"
             "they were shaped in. Veritasfurtum is one such drop — presently "
             "bruised, leaking, and\n"
             "occasionally visited by what should not fit.\n");
    return;
  }
  if (!strcmp(t, "csa") || !strcmp(t, "sentinel") ||
      !strcmp(t, "agency")) {
    snprintf(body, cap,
             "Cosmic Sentinel Agency (CSA)\n\n"
             "A high-tech interstellar authority tasked with threats to "
             "galactic stability — invasion,\n"
             "impact events, rogue physics, and misuse of dangerous travel.\n\n"
             "Branches named in the lore guide include Threat Assessment, "
             "Defense Operations,\n"
             "Research and Development, Intelligence, Emergency Response, and "
             "specialized Time\n"
             "Affairs and Multiversal Travel Affairs desks that license "
             "crossings and punish\n"
             "unauthorized jumps that could delete people from history or swap "
             "wrong souls into\n"
             "the wrong reality.\n\n"
             "In Hollow Ridge you feel their absence more than their presence "
             "— until a scroll or\n"
             "rift reminds you someone still watches the thresholds.\n");
    return;
  }
  if (!strcmp(t, "genetrix") || !strcmp(t, "bridge")) {
    snprintf(body, cap,
             "The Genetrix\n\n"
             "A living bridge between designs — named in temple rites and "
             "hermit\n"
             "lore alike. The priestess teaches that genetic exchange can be "
             "sacrament:\n"
             "two blueprints aligning, pleasure as current, union as hymn.\n\n"
             "In this port, ask Seraphina at the temple or read grove offerings "
             "for\n"
             "local color; the wider bible ties the Genetrix to creation-law "
             "and\n"
             "sterile orders who serve without breeding.\n");
    return;
  }
  if (!strcmp(t, "seraphine") || !strcmp(t, "seraphina") ||
      !strcmp(t, "priestess race")) {
    snprintf(body, cap,
             "Seraphines\n\n"
             "A people of the Architect's design: pale, sensitive skin; "
             "ceremonial grace;\n"
             "often four breasts in two rows beneath sheer vestments; velvet "
             "fox-tails;\n"
             "vivid green eyes that assess like a machine and forgive like a "
             "soul.\n\n"
             "High Priestess Seraphina in Hollow Ridge embodies the Blessed "
             "Barren —\n"
             "sterile by intent so devotion can be shared without reproduction's "
             "burden.\n");
    return;
  }
  if (!strcmp(t, "missionaries") || !strcmp(t, "missionary") ||
      !strcmp(t, "elena") || !strcmp(t, "kira") || !strcmp(t, "yuki")) {
    snprintf(body, cap,
             "Temple Missionaries\n\n"
             "Elena (human warmth), Kira (neko play), and Yuki (kitsune wisdom) "
             "travel\n"
             "between temple and village square, spreading the Architect's "
             "message that\n"
             "love has many sacred forms. Marcus the paladin stands as their "
             "shield.\n\n"
             "In this build they follow daily routes — find them by time of day "
             "with\n"
             "who / status, then talk and talk about architect or love.\n");
    return;
  }
  if (!strcmp(t, "waystone") || !strcmp(t, "waystones") ||
      !strcmp(t, "travel stone")) {
    snprintf(body, cap,
             "Waystones\n\n"
             "Monoliths of dark stone set with Elysium crystal. Runes encode "
             "consent,\n"
             "origin, destination, and return — a travel grammar safer than "
             "blind rifts.\n\n"
             "The blacksmith's forge hosts one; Nexus Points in the grove and "
             "square\n"
             "are brighter, formal cousins. Attune before you trust a road that "
             "folds\n"
             "distance.\n");
    return;
  }
  if (!strcmp(t, "house") || !strcmp(t, "white house") ||
      !strcmp(t, "manor")) {
    snprintf(body, cap,
             "The White House\n\n"
             "A boarded great house at the edge of Hollow Ridge — runes on the "
             "door frame,\n"
             "a keyhole that glows, and a journal on the altar grove that warns "
             "about the\n"
             "old well after midnight. The C build lets you explore the foyer, "
             "attic,\n"
             "kitchen, and grounds; deeper house secrets arrive through items "
             "and notes.\n");
    return;
  }
  if (!strcmp(t, "artifacts") || !strcmp(t, "curios") ||
      !strcmp(t, "trader")) {
    snprintf(body, cap,
             "Artifacts & Curios\n\n"
             "The marketplace hosts a mysterious stall: ancient artifacts, "
             "magical items,\n"
             "and rare curiosities — some glowing with residual law, others "
             "beautiful\n"
             "without power. Read the displays in the Artifact Trader room; "
             "merchants\n"
             "like Corbin and Sam trade in stranger stock on other days.\n");
    return;
  }
  if (!strcmp(t, "elysium") || !strcmp(t, "crystal")) {
    snprintf(body, cap,
             "Elysium Crystal\n\n"
             "Crystallized travel-light — holds luminance like water in a cup "
             "and hums\n"
             "when waystones wake. Merchants and temples both prize it; "
             "mishandling\n"
             "at a Nexus can invite attention from agencies that police "
             "crossings.\n\n"
             "Examine crystals at the forge waystone or village Nexus when you "
             "want\n"
             "the local fiction, not the whole cosmic manual.\n");
    return;
  }
  if (!strcmp(t, "currency") || !strcmp(t, "coins") ||
      !strcmp(t, "money") || !strcmp(t, "amethystus")) {
    snprintf(body, cap,
             "Currency (Amethystus ladder)\n\n"
             "Lore denominations in Maddeline's empire (approximate copper "
             "backing):\n"
             "  Royal gold coin 1000 · Gold plate 500 · Gold coin 100\n"
             "  Silver 50 · Bronze 10 · Copper 1\n\n"
             "This port keeps one saved purse total internally and displays "
             "gold / silver / bronze /\n"
             "copper for readability. Merchants, status, and trade history "
             "use the same wallet.\n");
    return;
  }
  snprintf(body, cap,
           "Unknown lore topic \"%s\".\n\n"
           "Try: lore veritasfurtum | architect | hollow | cosmology | csa | "
           "currency | genetrix | seraphine | missionaries | waystone | elysium "
           "| house | artifacts\n",
           t);
}

static void remember_npc_here(void) {
  const char *e = world_room_entity(g_room);
  size_t n;
  if (!e || !e[0]) return;
  n = strlen(e);
  if (n >= sizeof g_last_npc) n = sizeof g_last_npc - 1;
  memcpy(g_last_npc, e, n);
  g_last_npc[n] = '\0';
}

static void remember_talk_topic(const char *topic) {
  size_t n;
  if (!topic || !topic[0]) return;
  n = strnlen(topic, AETER_LAST_TOPIC_CAP);
  if (n >= sizeof g_last_topic) n = sizeof g_last_topic - 1;
  memcpy(g_last_topic, topic, n);
  g_last_topic[n] = '\0';
}

static const char *topic_mood_for(const char *topic) {
  if (!topic || !topic[0]) return "none";
  if (strstr(topic, "buy") || strstr(topic, "sell") || strstr(topic, "wares") ||
      strstr(topic, "price") || strstr(topic, "coin") || strstr(topic, "trade"))
    return "commerce";
  if (strstr(topic, "lock") || strstr(topic, "key") || strstr(topic, "guard") ||
      strstr(topic, "door") || strstr(topic, "watch"))
    return "security";
  if (strstr(topic, "work") || strstr(topic, "mill") || strstr(topic, "craft") ||
      strstr(topic, "forge"))
    return "labor";
  if (strstr(topic, "rumor") || strstr(topic, "gossip") || strstr(topic, "news") ||
      strstr(topic, "story"))
    return "gossip";
  if (strstr(topic, "help") || strstr(topic, "hint") || strstr(topic, "need"))
    return "support";
  return "general";
}

static const char *topic_heat_for(const char *topic_mood, int risk) {
  if (str_ieq(topic_mood, "none")) return "none";
  if (str_ieq(topic_mood, "security")) return risk >= 15 ? "high" : "medium";
  if (str_ieq(topic_mood, "commerce")) return risk >= 17 ? "medium" : "low";
  if (str_ieq(topic_mood, "gossip")) return risk >= 16 ? "medium" : "low";
  if (str_ieq(topic_mood, "support")) return risk >= 18 ? "medium" : "low";
  return risk >= 18 ? "high" : risk >= 12 ? "medium" : "low";
}

static const char *npc_trust_for(const char *slug, const char *role, int risk) {
  int ix, rep;
  if (!slug || !slug[0] || str_ieq(role, "none")) return "unknown";
  ix = aet_merchant_index(slug);
  if (ix >= 0) {
    rep = merchant_rep_score(ix);
    if (rep >= 65) return "trusted";
    if (rep >= 25) return "wary";
    if (risk >= 17) return "hostile";
    return "stranger";
  }
  if (str_ieq(role, "guard")) return risk >= 15 ? "suspicious" : "wary";
  if (risk >= 18) return "suspicious";
  return "neutral";
}

static const char *npc_leverage_for(const char *slug, const char *role, int risk) {
  int ix, rep, edge;
  if (!slug || !slug[0] || str_ieq(role, "none")) return "none";
  edge = 0;
  if (g_coins >= 50) edge += 2;
  else if (g_coins >= 15) edge += 1;
  if (g_ready_item[0] && inv_has(g_ready_item)) edge += 1;
  if (g_inv_n >= (MAX_INV * 3) / 4) edge -= 1;
  if (risk >= 18) edge -= 1;
  if (str_ieq(role, "guard")) {
    if (edge >= 2) return "negotiable";
    if (risk >= 16) return "none";
    return "limited";
  }
  ix = aet_merchant_index(slug);
  if (ix >= 0) {
    rep = merchant_rep_score(ix);
    if (rep >= 65 && edge >= 1) return "strong";
    if (rep >= 25 || edge >= 1) return "moderate";
    if (risk >= 17 && edge <= 0) return "none";
    return "limited";
  }
  if (edge >= 2) return "moderate";
  if (edge <= -1) return "none";
  return "limited";
}

static int try_move(int dir, char *msg, size_t msgcap) {
  int dest = world_exit(g_room, dir);
  const char *slug = world_slug(g_room);

  if (dest < 0 || dest >= WORLD_ROOM_COUNT) {
    snprintf(msg, msgcap, "You can't go that way.");
    causal_push("move-blocked", "no exit in that direction");
    return 0;
  }
  if (slug[0] && strcmp(slug, "front_door") == 0 && dir == DIR_E &&
      !g_front_unlocked) {
    if (!inv_has("house_key")) {
      snprintf(msg, msgcap,
               "The boarded door will not budge. A faint glow from the keyhole "
               "suggests you need a key.");
      causal_push("move-blocked", "front door requires house_key");
      return 0;
    }
    g_front_unlocked = 1;
  }
  if (slug[0] && strcmp(slug, "east_of_house") == 0 && dir == DIR_E &&
      !g_shed_unlocked) {
    snprintf(msg, msgcap,
             "The shed door is locked. Use  pick lock  here with a lockpick "
             "first.");
    causal_push("move-blocked", "shed door locked");
    return 0;
  }
  hist_push(g_room);
  g_room = dest;
  conv_clear();
  if (dest >= 0 && dest < MAX_WORLD_ROOMS) g_visited[dest] = 1;
  if (g_last_npc[0]) {
    const char *ne = world_room_entity(g_room);
    if (!ne || !ne[0] || !str_ieq(ne, g_last_npc)) g_last_npc[0] = '\0';
  }
  snprintf(msg, msgcap, "Ok.");
  {
    char cmsg[160];
    (void)snprintf(cmsg, sizeof cmsg, "%s -> %s", world_slug(g_hist[g_hist_n - 1]),
                   world_slug(g_room));
    causal_push("move", cmsg);
  }
  {
    AetPcSave pr;
    size_t L;
    pc_capture(&pr);
    pc_fill_narrative_defaults(&pr);
    L = strlen(msg);
    if (L + 8 < msgcap && (pr.spe >= 16 || pr.agi >= 16))
      (void)snprintf(msg + L, msgcap - L,
                     " You carry speed without thinking.");
  }
  return 1;
}

#define MAX_AUTO_STEPS 50

static int try_move_n_steps(int dir, int n, char *msg, size_t msgcap,
                            int *ok_steps) {
  int s;
  char sm[512];
  *ok_steps = 0;
  if (n < 1) {
    snprintf(msg, msgcap, "Nothing to do.");
    return 0;
  }
  for (s = 0; s < n; s++) {
    if (!try_move(dir, sm, sizeof sm)) {
      if (*ok_steps == 0)
        snprintf(msg, msgcap, "%s", sm);
      else
        snprintf(msg, msgcap, "Stopped after %d move(s): %s", *ok_steps, sm);
      return 0;
    }
    (*ok_steps)++;
  }
  if (n == 1)
    snprintf(msg, msgcap, "%s", sm);
  else
    snprintf(msg, msgcap, "You move %d times.", n);
  return 1;
}

static int parse_direction_token(const char *tok, int *dir_out) {
  static const struct {
    const char *word;
    int dir;
  } kFuzzyDir[] = {{"north", DIR_N},
                   {"south", DIR_S},
                   {"east", DIR_E},
                   {"west", DIR_W},
                   {"northeast", DIR_NE},
                   {"northwest", DIR_NW},
                   {"southeast", DIR_SE},
                   {"southwest", DIR_SW},
                   {NULL, 0}};
  int i, bestd = 3, besti = -1, ties = 0;
  if (!tok || !tok[0]) return 0;
  if (!strcmp(tok, "n") || !strcmp(tok, "north") || !strcmp(tok, "northward") ||
      !strcmp(tok, "northwards")) {
    *dir_out = DIR_N;
    return 1;
  }
  if (!strcmp(tok, "s") || !strcmp(tok, "south") || !strcmp(tok, "southward") ||
      !strcmp(tok, "southwards")) {
    *dir_out = DIR_S;
    return 1;
  }
  if (!strcmp(tok, "e") || !strcmp(tok, "east") || !strcmp(tok, "eastward") ||
      !strcmp(tok, "eastwards")) {
    *dir_out = DIR_E;
    return 1;
  }
  if (!strcmp(tok, "w") || !strcmp(tok, "west") || !strcmp(tok, "westward") ||
      !strcmp(tok, "westwards")) {
    *dir_out = DIR_W;
    return 1;
  }
  if (!strcmp(tok, "u") || !strcmp(tok, "up")) {
    *dir_out = DIR_U;
    return 1;
  }
  if (!strcmp(tok, "d") || !strcmp(tok, "down")) {
    *dir_out = DIR_D;
    return 1;
  }
  if (!strcmp(tok, "ne") || !strcmp(tok, "northeast") ||
      !strcmp(tok, "northeastward") || !strcmp(tok, "northeastwards")) {
    *dir_out = DIR_NE;
    return 1;
  }
  if (!strcmp(tok, "nw") || !strcmp(tok, "northwest") ||
      !strcmp(tok, "northwestward") || !strcmp(tok, "northwestwards")) {
    *dir_out = DIR_NW;
    return 1;
  }
  if (!strcmp(tok, "se") || !strcmp(tok, "southeast") ||
      !strcmp(tok, "southeastward") || !strcmp(tok, "southeastwards")) {
    *dir_out = DIR_SE;
    return 1;
  }
  if (!strcmp(tok, "sw") || !strcmp(tok, "southwest") ||
      !strcmp(tok, "southwestward") || !strcmp(tok, "southwestwards")) {
    *dir_out = DIR_SW;
    return 1;
  }
  if (!strcmp(tok, "in") || !strcmp(tok, "inside") || !strcmp(tok, "enter")) {
    *dir_out = DIR_IN;
    return 1;
  }
  if (!strcmp(tok, "out") || !strcmp(tok, "outside") ||
      !strcmp(tok, "leave")) {
    *dir_out = DIR_OUT;
    return 1;
  }
  if (!strcmp(tok, "deeper")) {
    *dir_out = DIR_DEEPER;
    return 1;
  }
  if (!strcmp(tok, "upstream")) {
    *dir_out = DIR_UPSTREAM;
    return 1;
  }
  if (!strcmp(tok, "downstream")) {
    *dir_out = DIR_DOWNSTREAM;
    return 1;
  }
  if (!strcmp(tok, "fountain")) {
    *dir_out = DIR_FOUNTAIN;
    return 1;
  }
  if (!strcmp(tok, "stage")) {
    *dir_out = DIR_STAGE;
    return 1;
  }
  if (!strcmp(tok, "board")) {
    *dir_out = DIR_BOARD;
    return 1;
  }
  if (!strcmp(tok, "square")) {
    *dir_out = DIR_SQUARE;
    return 1;
  }
  for (i = 0; kFuzzyDir[i].word; i++) {
    int d = levenshtein_upto(tok, kFuzzyDir[i].word, 2);
    if (d > 2) continue;
    if (d < bestd) {
      bestd = d;
      besti = i;
      ties = 1;
    } else if (d == bestd) {
      ties++;
    }
  }
  if (besti >= 0 && bestd <= 2 && ties == 1) {
    *dir_out = kFuzzyDir[besti].dir;
    return 1;
  }
  return 0;
}

static void query_norm_underscore(char *dst, size_t cap, const char *src) {
  size_t i = 0;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  for (; *src && i + 1 < cap; src++) {
    unsigned char c = (unsigned char)*src;
    dst[i++] = (char)(c == ' ' ? '_' : tolower(c));
  }
  dst[i] = '\0';
}

static int npc_keywords_match(const char *hay, const char *keywords) {
  size_t i = 0, start;
  char word[72];
  size_t wl;
  if (!hay || !hay[0] || !keywords || !keywords[0]) return 0;
  for (;;) {
    while (keywords[i] == ' ') i++;
    if (!keywords[i]) break;
    start = i;
    while (keywords[i] && keywords[i] != ' ') i++;
    wl = (size_t)(i - start);
    if (wl >= sizeof word) wl = sizeof word - 1;
    memcpy(word, keywords + start, wl);
    word[wl] = '\0';
    if (wl >= 2 && strstr(hay, word)) return 1;
  }
  return 0;
}

static const char *npc_stage_approach(const char *slug, unsigned char stage) {
  if (!slug || !slug[0]) return NULL;
  if (str_ieq(slug, "miller")) {
    switch (stage) {
      case 0:
        return "The miller looks up from her work and gives you a nod.";
      case 1:
        return "The miller smiles: \"Hey! Good to see you again.\"";
      case 2:
        return "The miller grins: \"There you are! Come on in!\"";
      case 3:
        return "The miller beams: \"You're here! I was hoping you'd stop by.\"";
      case 4:
        return "The miller blushes slightly. \"Oh, hi! I'm glad you came by.\"";
      case 5:
        return "The miller smiles warmly. \"My love! I missed you.\"";
      case 6:
        return "The miller grins. \"Welcome back, partner.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "forest_hermit")) {
    switch (stage) {
      case 0:
        return "The hermit's ancient eyes study you. \"So... you have found me. "
               "Few do. What brings you to my domain, seeker of secrets?\"";
      case 1:
        return "The hermit nods slowly. \"Ah, you return. Your presence is... "
               "interesting. What knowledge do you seek today?\"";
      case 2:
        return "The hermit's expression softens slightly. \"Friend... it is good "
               "to see you again. The forest welcomes you.\"";
      case 3:
        return "The hermit's eyes gleam with rare warmth. \"Beloved seeker... "
               "you have earned my trust. What ancient secrets shall I share?\"";
      case 4:
        return "The hermit's ancient eyes hold something new — curiosity, "
               "perhaps desire. \"You... intrigue me. The old ways speak of "
               "connections beyond the physical. Perhaps...\"";
      case 5:
        return "The hermit's voice carries rare emotion. \"My love... in all my "
               "years, I have never felt such a connection. The forest itself "
               "blesses our bond.\"";
      case 6:
        return "The hermit's ancient form seems to glow. \"My partner, my "
               "heart... you have awakened something in me I thought long dead. "
               "The forest and I are yours.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "general_store_owner")) {
    switch (stage) {
      case 0:
        return "Sam looks you over with a merchant's smile — warm, measuring.";
      case 1:
        return "Sam leans on the counter. \"Back again? Good. I was hoping you'd "
               "need something.\"";
      case 2:
        return "Sam's smile turns familiar. \"There you are, darling. Tell me "
               "what you need — or what you're hiding.\"";
      case 3:
        return "Sam's eyes warm. \"Back again. I saved the good wine — and a "
               "little privacy.\"";
      case 4:
        return "Sam steps closer, voice low. \"For you? The shop stays open as "
               "long as you want.\"";
      case 5:
        return "Sam brushes your hand. \"I was about to close the curtains... "
               "but for you I'll keep the lights on.\"";
      case 6:
        return "Sam murmurs, \"My shop, my secrets, my bed. You're welcome to "
               "all three.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "blacksmith")) {
    switch (stage) {
      case 0:
        return "Riven wipes soot from their hands and offers a warm, tired smile.";
      case 1:
        return "Riven nods. \"Back again? Good — I was just thinking about your "
               "gear.\"";
      case 2:
        return "Riven grins. \"There you are. Bring me something worth heating.\"";
      case 3:
      case 4:
        return "Riven leans on the anvil. \"Always good to see a familiar face by "
               "the fire.\"";
      case 5:
      case 6:
        return "Riven's smile softens. \"For you I'll keep the forge hot as long "
               "as you need.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "tavern_keeper")) {
    switch (stage) {
      case 0:
        return "Soren straightens his vest, smoothing nonexistent wrinkles. "
               "\"A new face! How exciting! Please, let me get you a menu. Or a "
               "drink. Or a cushion?\"";
      case 1:
        return "Soren beams, his tail wagging slightly. \"You're back! I was "
               "hoping you would be. I saved the good wine just in case.\"";
      case 2:
        return "Soren hurries over, almost tripping. \"Oh, thank goodness you're "
               "here! The bard is off-key and the merchant is boring me to tears. "
               "Save me!\"";
      case 3:
        return "Soren leans on the table, looking up through his lashes. \"You "
               "look exhausted. Let me take care of you tonight. On the house.\"";
      case 4:
        return "Soren grabs your hands, cheeks pink. \"You are my favorite person "
               "in this entire establishment. Don't tell the others.\"";
      case 5:
        return "Soren pulls you into a quick, hidden hug behind the bar. \"I missed "
               "you. Being professional is so hard when you're in the room.\"";
      case 6:
        return "Soren rests his forehead against your shoulder a moment. \"You're "
               "my anchor. Get it? Because of the tavern name? ...Kiss me anyway?\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "village_guard")) {
    switch (stage) {
      case 0:
        return "Garrett blocks your path, chest heaving slightly. \"Halt. I need "
               "to inspect you for contraband. Don't move.\"";
      case 1:
        return "Garrett nods, eyes sweeping over you. \"You're clean. Move along, "
               "but stay within shouting distance.\"";
      case 2:
        return "Garrett smirks. \"Back again? You're starting to like seeing me in "
               "uniform, aren't you?\"";
      case 3:
        return "Garrett leans in close, invading your space. \"I was hoping you'd "
               "walk by. The shift gets boring without someone to keep an eye on.\"";
      case 4:
        return "Garrett relaxes his posture. \"I trust you at my back. I'd prefer "
               "you at my side. Stay safe out there.\"";
      case 5:
        return "Garrett grips your arm, pulling you into an alcove. \"I should "
               "arrest you for what you did to me last night. I can't focus on "
               "patrol.\"";
      case 6:
        return "Garrett's eyes darken. \"You're mine. Anyone touches you, they "
               "answer to my blade.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "paladin_marcus")) {
    switch (stage) {
      case 0:
        return "Marcus steps forward, his massive frame blocking the inner "
               "sanctum. \"Welcome. Leave your malice at the threshold, and you "
               "will find only warmth here.\"";
      case 1:
        return "Marcus smiles, tension leaving his shoulders. \"Good to see you "
               "again. Have you come to rest, or to worship?\"";
      case 2:
        return "Marcus opens his arms for a warm embrace. \"Friend! The temple "
               "feels lighter with you in it. Come, sit with me.\"";
      case 3:
        return "Marcus rests a heavy hand on the back of your neck. \"You look "
               "tense. Let me carry that weight for a while. You are safe with me.\"";
      case 4:
        return "Marcus clasps your hands. \"You are precious to this temple, and "
               "to me. If anyone makes you feel unsafe, tell me immediately.\"";
      case 5:
        return "Marcus pulls you gently against his chestplate, kissing the top "
               "of your head. \"My armor is heavy, but my hands are gentle. Let "
               "me take care of you tonight.\"";
      case 6:
        return "Marcus looks at you with unshakable devotion. \"My partner. My "
               "heart. My greatest honor is being the one you trust when you close "
               "your eyes.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "village_innkeeper")) {
    switch (stage) {
      case 0:
        return "Lydia greets you warmly. \"Welcome! I'm Lydia. Need a room or "
               "something to eat?\"";
      case 1:
        return "Lydia smiles. \"Ah, friend! Good to see you again! Come in, make "
               "yourself comfortable!\"";
      case 2:
        return "Lydia's face lights up. \"My friend! So glad you're here! Let me "
               "get you something to eat!\"";
      case 3:
        return "Lydia embraces you. \"Beloved friend! I've been hoping you'd "
               "visit! Come, let's catch up!\"";
      case 4:
        return "Lydia hugs you tightly. \"Dearest friend! Your visits always "
               "brighten my day! Stay as long as you like!\"";
      case 5:
        return "Lydia pulls you close. \"My love! I've missed you so! Your room "
               "is always ready, and so am I.\"";
      case 6:
        return "Lydia kisses you deeply. \"My partner, my heart! This inn is our "
               "home. Stay with me always.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "traveling_bard")) {
    switch (stage) {
      case 0:
        return "The bard notices you and grins. \"Ah, a new face! Welcome! I'm "
               "Aria! Care to hear a tale or share a story?\"";
      case 1:
        return "The bard's face lights up. \"Friend! Good to see you again! Ready "
               "for another tale?\"";
      case 2:
        return "The bard embraces you. \"My friend! I've got a new story I've "
               "been wanting to share with you!\"";
      case 3:
        return "The bard pulls you close. \"Beloved friend! I've been collecting "
               "stories just for you! Let me tell you one!\"";
      case 4:
        return "The bard's eyes sparkle. \"Dearest friend! Your presence "
               "inspires me! Let me sing you a song about our friendship!\"";
      case 5:
        return "The bard's voice softens. \"Oh, you're here! I... I've been "
               "composing a song about you. Would you like to hear it?\"";
      case 6:
        return "The bard kisses you, voice full of emotion. \"My partner, my "
               "muse! You inspire every song I sing. This one is for you, always.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "traveling_merchant")) {
    switch (stage) {
      case 0:
        return "Corbin gives you a quick, appreciative glance. \"New face. Good. "
               "I like figuring out exactly what a new customer is desperate for.\"";
      case 1:
        return "Corbin nods and shifts their pack strap. \"Back again? Tell me "
               "what problem you're solving today — I'll find the right bottle.\"";
      case 2:
        return "Corbin opens a hidden side pouch and lowers their voice. \"For "
               "you, I can show the stock I keep off the main tarp. The potent stuff.\"";
      case 3:
        return "Corbin offers you a folded route note. \"Read this before you "
               "travel. Safer timings, inns with the softest beds.\"";
      case 4:
        return "Corbin taps their ledger and grins. \"Priority stock and honest "
               "warnings. I don't extend that loyalty to just anyone.\"";
      case 5:
        return "Corbin's grin softens. \"Careful. Keep looking at me like that "
               "and I'll start charging you for the view.\"";
      case 6:
        return "Corbin rests their forehead against yours. \"My roads, my profits, "
               "my bed. You get half of all of it.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "priestess")) {
    switch (stage) {
      case 0:
        return "Seraphina studies your form with clinical intensity. \"A new "
               "vessel. You are welcome. Leave your inhibitions at the threshold.\"";
      case 1:
        return "She inclines her head, tail twitching. \"You returned. Good. "
               "Conviction — and arousal — grows through repetition.\"";
      case 2:
        return "Her voice softens. \"Friend, your energy steadies this temple. "
               "Come, let us speak of what your body craves.\"";
      case 3:
        return "Seraphina takes your hand, pale skin warm. \"You have earned the "
               "right to see the deeper mysteries. I do not offer communion lightly.\"";
      case 4:
        return "A rare, serene smile. \"You are a vital part of this sacred design "
               "now. What brings you heat brings me heat.\"";
      case 5:
        return "Her composure wavers, green eyes darkening. \"There are certain "
               "rites... ceremonies of the flesh... I have wished to perform with you.\"";
      case 6:
        return "Her forehead touches yours, velvet tail coiling around your waist. "
               "\"My partner in witness and in worship. We are the Architect's "
               "perfect union.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "farmer")) {
    switch (stage) {
      case 0:
        return "Jasper wipes sweat from his brow, shirt clinging. \"Howdy. Just "
               "let me catch my breath... hot day, isn't it?\"";
      case 1:
        return "Jasper smiles shyly. \"You came back. I was hoping you would. The "
               "farm's been feeling a little empty.\"";
      case 2:
        return "Jasper's eyes crinkle. \"Hey! I put aside the best strawberries for "
               "you. They're sweet... really sweet.\"";
      case 3:
        return "Jasper leans on his hoe, soft adoration in his eyes. \"Seeing you "
               "walking up that path... best part of my day. Honestly.\"";
      case 4:
        return "Jasper reaches out and squeezes your hand. \"You feel like home to "
               "me. You know that, right?\"";
      case 5:
        return "Jasper blushes, rubbing his neck. \"I was thinking about you. "
               "Maybe... wanted to ask if you'd stay for dinner? I can cook.\"";
      case 6:
        return "Jasper kisses your forehead tenderly. \"Look at this life we're "
               "building. I never thought I could be this happy. I love you.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "bartender")) {
    switch (stage) {
      case 0:
        return "Silas sets down a glass and meets your eyes. \"Evening. Stool's "
               "open. Ale's honest.\"";
      case 1:
        return "Silas gives a small nod. \"Back again. Good. I'll keep the rowdy "
               "ones off your table.\"";
      case 2:
        return "Silas slides a drink your way without being asked. \"For you — "
               "on the house. Don't tell Soren.\"";
      case 3:
      case 4:
        return "Silas leans on the bar. \"You're one of the reasons this place "
               "feels like a sanctuary.\"";
      case 5:
        return "Silas's voice drops to a rumble. \"Stay close tonight. I don't "
               "like how the room looks at you.\"";
      case 6:
        return "Silas rests a scarred hand over yours. \"My bar. My road. "
               "You're half of both now.\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "missionary_elena")) {
    switch (stage) {
      case 0:
        return "Elena turns with a warm smile. \"Hello! I'm Elena. Want to learn "
               "about The Architect's love?\"";
      case 1:
        return "Elena's face lights up. \"Friend! Good to see you! Ready to spread "
               "some love?\"";
      case 2:
        return "Elena embraces you. \"My friend! Let's share some love together!\"";
      case 3:
        return "Elena holds your hands. \"Beloved friend! The Architect's love "
               "flows between us!\"";
      case 4:
        return "Elena hugs you tightly. \"Dearest friend! Your love makes me so "
               "happy!\"";
      case 5:
        return "Elena blushes. \"Oh! You're here! I've been hoping to see you... "
               "want to share some love?\"";
      case 6:
        return "Elena kisses you deeply. \"My partner, my heart! The Architect "
               "has blessed us!\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "missionary_kira")) {
    switch (stage) {
      case 0:
        return "Kira's ears perk up. \"Oh! Hello! I'm Kira! Want to learn about "
               "The Architect's love?\"";
      case 1:
        return "Kira's tail swishes. \"Hey, friend! Ready to spread some love?\"";
      case 2:
        return "Kira bounds over. \"My friend! Let's share some love together!\"";
      case 3:
        return "Kira nuzzles you. \"Beloved friend! The Architect's love flows "
               "between us!\"";
      case 4:
        return "Kira purrs. \"Dearest friend! Your love makes me so happy!\"";
      case 5:
        return "Kira's tail swishes excitedly. \"Oh! You're here! I've been "
               "hoping to see you...\"";
      case 6:
        return "Kira purrs and presses close. \"My partner, my heart! Blessed "
               "love!\"";
      default:
        return NULL;
    }
  }
  if (str_ieq(slug, "missionary_yuki")) {
    switch (stage) {
      case 0:
        return "Yuki's tails sway as she turns. \"Greetings, traveler. I am Yuki. "
               "How may love guide you today?\"";
      case 1:
        return "Yuki's eyes sparkle. \"Ah, friend! The Architect's love shines "
               "on our meeting.\"";
      case 2:
        return "Yuki's tails wrap around you gently. \"My friend! Come, share "
               "the Architect's love.\"";
      case 3:
        return "Yuki nuzzles you with her tails. \"Beloved friend! I've been "
               "thinking of you.\"";
      case 4:
        return "Yuki's tails caress you. \"Dearest friend! The Architect "
               "blesses our bond.\"";
      case 5:
        return "Yuki's tails swish. \"Oh, you're here! I've been hoping to see "
               "you...\"";
      case 6:
        return "Yuki's tails caress you intimately. \"My partner, my heart! "
               "Beautiful love!\"";
      default:
        return NULL;
    }
  }
  return NULL;
}

static const char *npc_period_talk(const char *slug, const char *period,
                                   int want_greeting) {
  if (!slug || !period || !period[0]) return NULL;
  if (str_ieq(slug, "miller")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Morning! Just getting started for the day. Fresh start, "
                   "fresh flour!"
                 : "Mornings are when I'm most productive. There's something "
                   "satisfying about starting the day with hard work.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening! Perfect time to wind down after a day's work. What "
                   "brings you here?"
                 : "Evenings are my favorite time. The mill's quiet, and I can "
                   "just enjoy the peace. Want to sit and chat?";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night visit? Can't sleep either? I'm usually up working "
                   "or just enjoying the quiet."
                 : "Nighttime's peaceful here. Just me, the mill, and the stars. "
                   "Sometimes I think about everything that's happened... want "
                   "to talk?";
  }
  if (str_ieq(slug, "forest_hermit")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Dawn breaks... a time of new beginnings. What secrets call "
                   "to you this morning?"
                 : "Mornings are when the forest's magic is strongest. The old "
                   "powers stir with the light.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening falls... the boundary between day and night, when "
                   "secrets are most easily revealed. What do you seek?"
                 : "Evenings are when the veil between worlds grows thin. "
                   "Ancient knowledge flows more freely.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Night deepens... when the old powers are most active. Few "
                   "dare seek me in the dark. What brings you?"
                 : "Nights are sacred. The forest's true nature reveals itself "
                   "only to those who are not afraid of the dark.";
  }
  if (str_ieq(slug, "general_store_owner")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Morning, sunshine. You look a little rumpled — rough night, "
                   "or just a very good one?"
                 : "Mornings are for organization. I like knowing exactly where "
                   "everything is before the chaos messes it up.";
    if (str_ieq(period, "afternoon"))
      return want_greeting
                 ? "Afternoon. The shop is hot and I was thinking about a break. "
                   "Perfect timing."
                 : "Everyone wants a piece of me in the afternoon. Exhausting "
                   "being this essential, don't you think?";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening. I was about to lock the ledgers. Unless you brought "
                   "something more interesting than coin?"
                 : "When the sun goes down, the requests get much more specific. "
                   "I keep the special inventory ready.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "You're out late. Looking for trouble, or a place to hide from "
                   "it?"
                 : "The shop is quiet at night. Just me, the shadows, and locks "
                   "clicking into place. Very... intimate.";
  }
  if (str_ieq(slug, "blacksmith")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Morning! Just getting the forge hot. Early bird gets the "
                   "best work done."
                 : "Mornings are when I'm sharpest. The fire's fresh, the metal's "
                   "ready, and I'm ready to create something great.";
    if (str_ieq(period, "afternoon"))
      return want_greeting
                 ? "Afternoon heat's perfect for serious work. What needs "
                   "forging or fixing?"
                 : "Afternoons I organize stock and plan the next piece. Good "
                   "time to bring me quality materials.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening! Winding down after a long day at the forge. What "
                   "brings you by?"
                 : "Evenings are for finishing touches and planning tomorrow's "
                   "work. Always something to improve.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? Can't sleep? I'm usually up late on special "
                   "projects or enjoying the quiet."
                 : "Nighttime's peaceful. Just me, the forge, and the stars. "
                   "Sometimes my best work happens when the world's asleep.";
  }
  if (str_ieq(slug, "tavern_keeper")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Morning! The sun is up, the floor is clean, and I have had "
                   "three cups of tea. I am ready for anything!"
                 : "Mornings are for preparation! Check the stock, polish the "
                   "glasses, mentally prepare for the lunch rush. It's a "
                   "performance!";
    if (str_ieq(period, "afternoon"))
      return want_greeting
                 ? "Afternoon! It's getting busy! If I look frantic, it's just "
                   "part of my charm, I promise."
                 : "The lunch crowd is so demanding. I only have two hands — and "
                   "they are very delicate hands!";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Good evening! The lights are dimmed, the mood is set... can "
                   "I get you something to match the atmosphere?"
                 : "This is my favorite time. Everyone is happy, the music is "
                   "playing... I feel like I'm hosting a grand ball every night.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? Oh my. The respectable folk have gone to bed. "
                   "Now it's just us... and the interesting people."
                 : "I should be exhausted, but the nightlife energizes me! Or "
                   "maybe that's just the fear of a bar fight.";
  }
  if (str_ieq(slug, "village_guard")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Morning. Too early for games. Stand straight when you "
                   "address me."
                 : "Morning drills. Sweating out the vices of the night before. "
                   "Care to watch me spar?";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Sun's down. Shadows are getting long. Stick close to me if "
                   "you don't want to get hurt."
                 : "The tavern gets rowdy now. I might have to drag someone out. "
                   "It's good exercise.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "It's late. Why aren't you in bed? Unless you're looking for "
                   "the kind of company decent folk avoid."
                 : "The night watch is lonely. And cold. I could use something — "
                   "or someone — to keep the blood moving.";
  }
  if (str_ieq(slug, "paladin_marcus")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Dawn approaches. I have already finished my physical drills. "
                   "The body must stay strong for the Architect's work."
                 : "The morning light is beautiful on the altar. Are you here for "
                   "morning prayers, or just a quiet place to breathe?";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "The sun sets, and the evening rites begin. I will stand guard "
                   "so you may lose yourself in the moment."
                 : "This is when the temple grows warm. I enjoy watching the "
                   "tension leave the villagers' bodies.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "It is late. The dark brings out our deepest hungers. Speak "
                   "yours freely; I do not judge."
                 : "I take the night watch so the others can sleep — or entwine — "
                   "in peace. You are welcome to keep me company.";
  }
  if (str_ieq(slug, "village_innkeeper")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Good morning! Fresh start to the day! Need breakfast or a "
                   "room?"
                 : "Mornings are busy with guests checking out and preparing for "
                   "the day. But I always have time for friends!";
    if (str_ieq(period, "afternoon"))
      return want_greeting
                 ? "Good afternoon! Lunch is ready, and the inn is lively today!"
                 : "Afternoons are when travelers share their stories. I love "
                   "hearing where everyone's been.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Good evening! Dinner's ready, and the fire's warm. Make "
                   "yourself at home!"
                 : "Evenings are cozy here. Guests gather, stories flow, and the "
                   "inn feels alive.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? The inn is quiet now, but you're always welcome. "
                   "Need a room?"
                 : "Nights are peaceful. Just the crackle of the fire and the "
                   "occasional traveler seeking rest.";
  }
  if (str_ieq(slug, "traveling_bard")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Good morning! Perfect time for a cheerful song to start the "
                   "day! What tale interests you?"
                 : "Mornings are great for upbeat songs and adventure tales! Gets "
                   "everyone's spirits up!";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Good evening! Perfect time for stories around a fire! What "
                   "would you like to hear?"
                 : "Evenings are my favorite — everyone gathers, and I can share "
                   "my best tales and songs!";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? Perfect for mysterious tales and haunting "
                   "melodies! Care to listen?"
                 : "Nights are magical for storytelling. Darkness makes every "
                   "tale more compelling!";
  }
  if (str_ieq(slug, "traveling_merchant")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Morning run. Inventory is fresh and my patience is highest. "
                   "Tell me what you need."
                 : "Morning is for planning route risk. No sentiment — just cold, "
                   "hard survival.";
    if (str_ieq(period, "afternoon"))
      return want_greeting
                 ? "Afternoon rush. Keep requests specific and I can move fast. "
                   "I have places to be."
                 : "By midday I can tell who prepared for their journey, and who "
                   "is just looking for a distraction.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening market energy is volatile. People buy with desire "
                   "instead of their brains."
                 : "Evenings are where rumors inflate and prices follow. Best "
                   "margins of the day.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late trade is expensive trade. If you're here now, I assume "
                   "you need something to keep you warm."
                 : "At night I pack for contingencies. Sometimes that means a "
                   "locked door and a bottle of wine.";
  }
  if (str_ieq(slug, "priestess")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Dawn is for new vows and waking blood. Does your body feel "
                   "the morning's potential?"
                 : "Morning prayer is a physical audit: check every nerve, "
                   "acknowledge every hunger, and prepare to be filled.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening brings the accounting of the day's heat. Who did you "
                   "touch, and who touched you?"
                 : "At dusk we shed vestments and secrets. The night belongs to "
                   "the Architect's deeper designs.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Night visitors usually seek the heat they lack. Sit. The "
                   "lamps are low for a reason."
                 : "In the quiet of the night we hear the body most clearly. No "
                   "noise to hide your desires here.";
  }
  if (str_ieq(slug, "farmer")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Morning... quiet out here. The corn's whispering. Need "
                   "something from the farm?"
                 : "Mornings I watch the sun hit the fields. Good time to think.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening in the square... I brought extra produce. Thought "
                   "you might stop by."
                 : "Evenings in the village are loud. I like selling here — "
                   "easier to see a friendly face.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? I should be in bed... but the stars are clear."
                 : "Nights on the farm are honest. Just crickets and your "
                   "thoughts.";
  }
  if (str_ieq(slug, "bartender")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Early start. Coffee's strong; ale's stronger after noon."
                 : "Mornings I stock shelves before the keeper's drama wakes up.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Evening rush. Sit where I can see the door and your glass."
                 : "Evenings I tell the short versions of the war stories.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night crowd's thinner. Easier to hear what you need."
                 : "Nights I close slow. The quiet ones get the best pours.";
  }
  if (str_ieq(slug, "missionary_elena")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Good morning! The Architect's light shines! Ready to spread "
                   "love today?"
                 : "Mornings are peaceful — prayer, then out to spread love!";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Good evening! What a day of spreading love! How can I help?"
                 : "Evenings are cozy — perfect to share love with friends.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? Perfect for intimate love-sharing! The Architect "
                   "approves!"
                 : "Nights are peaceful — perfect for sharing love in all its "
                   "forms.";
  }
  if (str_ieq(slug, "missionary_kira")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Good morning! Ready to spread some love today?"
                 : "Mornings are energetic — prayer, then mischief in the name "
                   "of love!";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Good evening! How can I help you?"
                 : "Evenings are cozy — perfect to cuddle and share love!";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? Perfect for intimate love-sharing!"
                 : "Nights are my favorite — peaceful, and perfect for love.";
  }
  if (str_ieq(slug, "missionary_yuki")) {
    if (str_ieq(period, "morning"))
      return want_greeting
                 ? "Good morning! May love and wisdom guide you."
                 : "Mornings are perfect for meditation. I feel the Architect's "
                   "wisdom strongest at dawn.";
    if (str_ieq(period, "evening"))
      return want_greeting
                 ? "Good evening! As the day ends, we reflect on love shared."
                 : "Evenings are for reflection and gratitude.";
    if (str_ieq(period, "night"))
      return want_greeting
                 ? "Late night? The Architect's love never sleeps."
                 : "Nights are sacred — quiet enough to feel presence deeply.";
  }
  return NULL;
}

static const char *npc_pick_chatter(const AetNpcLineSet *set) {
  size_t n = 0;
  const char *const *p;
  unsigned h;
  if (!set || !set->chatter) return NULL;
  for (p = set->chatter; *p; p++) n++;
  if (n == 0) return NULL;
  h = (unsigned)(g_turns * 131u + g_room * 17u);
  if (set->slug && set->slug[0])
    h ^= (unsigned)strlen(set->slug) * 7u;
  return set->chatter[h % n];
}

static void cmd_npc_schedule(const char *raw, char *msg, size_t msgcap) {
  char qn[MAX_ITEM_LEN];
  char buf[160];
  AetWorldClock wc;
  const AetNpcRoutine *rt = NULL;
  const char *next_p;
  size_t i;
  if (!raw || !raw[0]) {
    snprintf(msg, msgcap,
             "Name an NPC:  npc schedule miller  or  routine blacksmith");
    return;
  }
  strncpy(buf, raw, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  strip_leading_articles(buf);
  strip_trailing_space(buf);
  query_norm_underscore(qn, sizeof qn, buf);
  rt = npc_routine_for(buf);
  if (!rt) {
    for (i = 0; i < npc_routine_count(); i++) {
      const AetNpcRoutine *c = npc_routine_at(i);
      char pretty[64];
      entity_pretty(c->slug, pretty, sizeof pretty);
      if (str_ieq(c->slug, buf) || str_ieq(pretty, buf) ||
          strstr(c->slug, qn) != NULL || strstr(pretty, buf) != NULL) {
        rt = c;
        break;
      }
    }
  }
  if (!rt) {
    snprintf(msg, msgcap,
             "No daily routine on file for \"%s\". Try  who all  or  where <npc>.",
             raw);
    return;
  }
  get_world_clock(&wc);
  next_p = npc_routine_next_period(wc.period);
  {
    char pretty[64];
    const char *now_room = npc_routine_room_special(rt, &wc);
    const char *next_room = npc_routine_room_for_period(rt, next_p);
    const char *act = npc_activity_for_period(rt->slug, wc.period);
    entity_pretty(rt->slug, pretty, sizeof pretty);
    snprintf(msg, msgcap,
             "Daily routine — %s\n\n"
             "Now (%s): %s  [%s]\n"
             "  %s\n\n"
             "Morning   → %s\n"
             "Afternoon → %s\n"
             "Evening   → %s\n"
             "Night     → %s\n\n"
             "Next period (%s): %s",
             pretty, wc.period, now_room ? resolve_world_title(world_room_index(now_room)) : "?",
             now_room ? now_room : "?", act && act[0] ? act : "(no activity label)",
             rt->morning_room, rt->afternoon_room, rt->evening_room, rt->night_room,
             next_p, next_room ? next_room : "?");
  }
}

static void cmd_who(char *msg, size_t msgcap, int global_mode) {
  const char *ent = world_room_entity(g_room);
  char pretty[128];
  char role[96];
  char banner[256];
  size_t L;
  if (global_mode) {
    int i, shown = 0;
    AetWorldClock wc;
    pc_format_identity_banner(banner, sizeof banner);
    get_world_clock(&wc);
    snprintf(msg, msgcap,
             "Global NPC presence scan\n\n"
             "%s\n\n"
             "Live NPC placements (%s clock):\n\n",
             banner, wc.period);
    for (i = 0; i < npc_routine_count(); i++) {
      const AetNpcRoutine *rt = npc_routine_at(i);
      const char *room_slug = npc_routine_room_special(rt, &wc);
      int rr = room_slug ? world_room_index(room_slug) : -1;
      char who_pretty[96];
      const char *act;
      const char *next_p = npc_routine_next_period(wc.period);
      const char *next_room = npc_routine_room_for_period(rt, next_p);
      const char *tag = (rr == g_room) ? "  [here]" : "";
      if (rr < 0) continue;
      entity_pretty(rt->slug, who_pretty, sizeof who_pretty);
      act = npc_activity_for_period(rt->slug, wc.period);
      if (act && act[0])
        body_append(msg, msgcap,
                    "  • %-20s  —  %s  [%s]%s\n"
                    "      activity: %s\n"
                    "      next (%s): %s\n",
                    who_pretty, resolve_world_title(rr), room_slug, tag, act,
                    next_p, next_room ? next_room : "?");
      else
        body_append(msg, msgcap, "  • %-20s  —  %s  [%s]%s\n"
                                 "      next (%s): %s\n",
                    who_pretty, resolve_world_title(rr), room_slug, tag, next_p,
                    next_room ? next_room : "?");
      shown++;
      if (strlen(msg) + 280 >= msgcap) break;
    }
    for (i = 0; i < WORLD_ROOM_COUNT; i++) {
      const char *e = world_room_entity(i);
      char who_pretty[96];
      if (!e || !e[0] || npc_routine_for(e)) continue;
      entity_pretty(e, who_pretty, sizeof who_pretty);
      body_append(msg, msgcap, "  • %-20s  —  %s  [%s]%s\n", who_pretty,
                  resolve_world_title(i), world_slug(i),
                  (i == g_room) ? "  [here]" : "");
      shown++;
      if (strlen(msg) + 200 >= msgcap) break;
    }
    if (shown == 0) {
      strncat(msg, "  (No NPC placements found in this build.)\n",
              msgcap - strlen(msg) - 1);
    } else {
      body_append(msg, msgcap,
                  "\nTotal placements: %d\n"
                  "Try  npc schedule <name>  for a full day plan, or  where <npc>  for lookup.\n",
                  shown);
    }
    return;
  }
  if (!ent[0]) {
    pc_format_identity_banner(banner, sizeof banner);
    snprintf(msg, msgcap,
             "No one else is here.\n\n"
             "%s\n\n"
             "(NPCs show up as Present: … when a room.entity is set.)",
             banner);
    return;
  }
  entity_pretty(ent, pretty, sizeof pretty);
  remember_npc_here();
  pc_format_role_phrase(role, sizeof role);
  snprintf(msg, msgcap, "Present: %s — try 'talk' or 'talk to %s'.", pretty,
           pretty);
  L = strlen(msg);
  if (L + 4 < msgcap)
    (void)snprintf(msg + L, msgcap - L,
                   "\n\nYou appear to others roughly as %s (%s).", role,
                   pc_display_name());
  {
    AetWorldClock wc;
    const char *act;
    get_world_clock(&wc);
    act = npc_activity_for_period(ent, wc.period);
    if (act && act[0] && strlen(msg) + strlen(act) + 48 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nCurrent routine: %s.", act);
  }
  {
    unsigned char st = soc_derive_stage_npc_slug(ent);
    int fri = soc_friendship_for_slug(ent);
    int mix = aet_merchant_index(ent);
    char fbar[12];
    meter_bar_pct(fbar, sizeof fbar, fri);
    if (strlen(msg) + 96 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nBond: %s — friendship %d [%s].", soc_stage_name(st), fri,
                     fbar);
    if (mix >= 0 && strlen(msg) + 64 < msgcap) {
      int rep = merchant_rep_score(mix);
      char pbar[12];
      int pp = rep > 220 ? 100 : (rep * 100) / 220;
      meter_bar_pct(pbar, sizeof pbar, pp);
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nPatron standing: %s (%d) [%s].",
                     merchant_rep_tier_label(rep), rep, pbar);
    }
    if (strlen(msg) + 48 < msgcap)
      (void)snprintf(msg + strlen(msg), msgcap - strlen(msg),
                     "\nSee  rapport  for the full relationship board.");
  }
}

static void conv_clear(void) {
  g_conv_active = 0;
  g_conv_npc[0] = '\0';
  g_conv_pick_n = 0;
}

static char *conv_mem_tags(const char *slug) {
  int ix, si;
  if (!slug || !slug[0]) return NULL;
  ix = aet_merchant_index(slug);
  if (ix >= 0 && ix < AETER_REP_MAX) return g_npc_topic_mem[ix];
  si = soc_npc_find(slug);
  if (si >= 0) return g_soc_topic_mem[si];
  return NULL;
}

static char *conv_mem_last(const char *slug) {
  int ix, si;
  if (!slug || !slug[0]) return NULL;
  ix = aet_merchant_index(slug);
  if (ix >= 0 && ix < AETER_REP_MAX) return g_npc_topic_last[ix];
  si = soc_npc_find(slug);
  if (si >= 0) return g_soc_topic_last[si];
  return NULL;
}

static int conv_mem_has_tag(const char *slug, const char *tag) {
  char *mem, needle[40];
  char *p;
  size_t tl;
  if (!tag || !tag[0]) return 0;
  mem = conv_mem_tags(slug);
  if (!mem || !mem[0]) return 0;
  snprintf(needle, sizeof needle, ",%s,", tag);
  if (strstr(mem, needle)) return 1;
  tl = strlen(tag);
  if ((size_t)strlen(mem) == tl && str_ieq(mem, tag)) return 1;
  if (!strncmp(mem, tag, tl) && mem[tl] == ',') return 1;
  p = strstr(mem, tag);
  while (p) {
    if ((p == mem || p[-1] == ',') &&
        (p[tl] == '\0' || p[tl] == ','))
      return 1;
    p = strstr(p + 1, tag);
  }
  return 0;
}

static int conv_mem_buf_has(const char *mem, const char *tag) {
  char needle[40];
  size_t tl;
  if (!mem || !mem[0] || !tag || !tag[0]) return 0;
  snprintf(needle, sizeof needle, ",%s,", tag);
  if (strstr(mem, needle)) return 1;
  tl = strlen(tag);
  if ((size_t)strlen(mem) == tl && str_ieq(mem, tag)) return 1;
  if (!strncmp(mem, tag, tl) && (mem[tl] == '\0' || mem[tl] == ',')) return 1;
  return 0;
}

static void conv_mem_add_tag_buf(char *mem, const char *tag) {
  size_t L, tl;
  if (!mem || !tag || !tag[0]) return;
  if (conv_mem_buf_has(mem, tag)) return;
  L = strlen(mem);
  tl = strlen(tag);
  if (L + tl + 2 >= CONV_TOPIC_MEM_SZ) return;
  if (L > 0) {
    mem[L++] = ',';
    mem[L] = '\0';
  }
  strncat(mem, tag, CONV_TOPIC_MEM_SZ - L - 1);
}

static void conv_mem_record(const char *slug, const char *topic_phrase) {
  char tag[32];
  char *tags, *last;
  size_t i, wl, start;
  if (!slug || !slug[0] || !topic_phrase || !topic_phrase[0]) return;
  tags = conv_mem_tags(slug);
  last = conv_mem_last(slug);
  if (!tags || !last) {
    if (aet_merchant_index(slug) < 0) (void)soc_npc_ensure(slug);
    tags = conv_mem_tags(slug);
    last = conv_mem_last(slug);
  }
  if (!tags || !last) return;
  tag[0] = '\0';
  for (i = 0; topic_phrase[i];) {
    while (topic_phrase[i] == ' ') i++;
    if (!topic_phrase[i]) break;
    start = i;
    while (topic_phrase[i] && topic_phrase[i] != ' ') i++;
    wl = (size_t)(i - start);
    if (wl >= sizeof tag) wl = sizeof tag - 1;
    memcpy(tag, topic_phrase + start, wl);
    tag[wl] = '\0';
    for (start = 0; tag[start]; start++)
      tag[start] = (char)tolower((unsigned char)tag[start]);
    if (wl >= 3) break;
  }
  if (!tag[0]) snprintf(tag, sizeof tag, "chat");
  conv_mem_add_tag_buf(tags, tag);
  snprintf(last, CONV_TOPIC_MEM_SZ, "%.55s", topic_phrase);
}

static void conv_topic_label(const char *keywords, char *label, size_t cap) {
  size_t i = 0, start, wl;
  if (!label || cap < 2) return;
  label[0] = '\0';
  if (!keywords || !keywords[0]) {
    snprintf(label, cap, "Something else");
    return;
  }
  for (;;) {
    while (keywords[i] == ' ') i++;
    if (!keywords[i]) break;
    start = i;
    while (keywords[i] && keywords[i] != ' ') i++;
    wl = (size_t)(i - start);
    if (wl < 3) continue;
    if (wl >= cap) wl = cap - 1;
    memcpy(label, keywords + start, wl);
    label[wl] = '\0';
    label[0] = (char)toupper((unsigned char)label[0]);
    return;
  }
  snprintf(label, cap, "Something else");
}

static void conv_primary_tag(const char *keywords, char *tag, size_t cap) {
  size_t i = 0, start, wl;
  if (!tag || cap < 2) return;
  tag[0] = '\0';
  if (!keywords || !keywords[0]) return;
  for (;;) {
    while (keywords[i] == ' ') i++;
    if (!keywords[i]) break;
    start = i;
    while (keywords[i] && keywords[i] != ' ') i++;
    wl = (size_t)(i - start);
    if (wl < 3) continue;
    if (wl >= cap) wl = cap - 1;
    memcpy(tag, keywords + start, wl);
    tag[wl] = '\0';
    for (start = 0; tag[start]; start++)
      tag[start] = (char)tolower((unsigned char)tag[start]);
    return;
  }
}

static void conv_append_menu(char *msg, size_t msgcap) {
  int i;
  size_t L;
  if (!g_conv_active || g_conv_pick_n <= 0 || !msg) return;
  L = strlen(msg);
  if (L + 80 >= msgcap) return;
  snprintf(msg + L, msgcap - L,
           "\n\nContinue (reply with a number, a label, or  talk about … ):\n");
  for (i = 0; i < g_conv_pick_n; i++) {
    L = strlen(msg);
    if (L + 96 >= msgcap) break;
    snprintf(msg + L, msgcap - L, "  %d) %s\n", i + 1, g_conv_pick_label[i]);
  }
  L = strlen(msg);
  if (L + 64 < msgcap)
  snprintf(msg + L, msgcap - L, "  (or type  goodbye  to end the conversation.)");
}

static void conv_open_menu(const char *ent, const AetNpcLineSet *dlg, char *msg,
                           size_t msgcap) {
  const AetNpcTopic *tp;
  char tag[32];
  char label[72];
  int n = 0;
  if (!ent || !ent[0] || !dlg) {
    conv_clear();
    return;
  }
  conv_clear();
  snprintf(g_conv_npc, sizeof g_conv_npc, "%s", ent);
  if (dlg->topics) {
    for (tp = dlg->topics; tp->keywords && tp->response && n < CONV_MENU_MAX - 2;
         tp++) {
      conv_primary_tag(tp->keywords, tag, sizeof tag);
      if (!tag[0] || conv_mem_has_tag(ent, tag)) continue;
      conv_topic_label(tp->keywords, label, sizeof label);
      g_conv_pick_kind[n] = CONV_PICK_TOPIC;
      snprintf(g_conv_pick_label[n], sizeof g_conv_pick_label[n], "%s", label);
      snprintf(g_conv_pick_phrase[n], sizeof g_conv_pick_phrase[n], "%s",
               tp->keywords);
      n++;
      if (n >= 3) break;
    }
  }
  if (n < CONV_MENU_MAX - 1) {
    g_conv_pick_kind[n] = CONV_PICK_MORE;
    snprintf(g_conv_pick_label[n], sizeof g_conv_pick_label[n], "Tell me more");
    if (g_last_topic[0] && str_ieq(g_last_npc, ent))
      copy_capped(g_conv_pick_phrase[n], sizeof g_conv_pick_phrase[n], g_last_topic);
    else
      snprintf(g_conv_pick_phrase[n], sizeof g_conv_pick_phrase[n],
               "tell me more");
    n++;
  }
  if (n < CONV_MENU_MAX) {
    g_conv_pick_kind[n] = CONV_PICK_GOODBYE;
    snprintf(g_conv_pick_label[n], sizeof g_conv_pick_label[n], "Say goodbye");
    g_conv_pick_phrase[n][0] = '\0';
    n++;
  }
  g_conv_pick_n = n;
  if (n > 0) {
    g_conv_active = 1;
    conv_append_menu(msg, msgcap);
  }
}

static void conv_finish_talk(const char *ent, const AetNpcLineSet *dlg,
                             const char *topic_phrase, char *msg, size_t msgcap) {
  if (topic_phrase && topic_phrase[0]) conv_mem_record(ent, topic_phrase);
  if (dlg)
    conv_open_menu(ent, dlg, msg, msgcap);
  else
    conv_clear();
}

static int conv_try_followup(const char *line, char *msg, size_t msgcap,
                             int *turn_advance) {
  char work[INPUT_LINE_MAX];
  const char *p;
  int pick = 0, i;
  const char *ent;
  char pretty[128];
  if (!line || !line[0] || !g_conv_active || g_conv_pick_n <= 0 || !msg ||
      !turn_advance)
    return 0;
  p = line;
  while (*p == ' ') p++;
  if (!strncasecmp(p, "talk about ", 11)) return 0;
  if (!strncasecmp(p, "talk to ", 8)) return 0;
  if (str_ieq(p, "talk") || str_ieq(p, "greet") || str_ieq(p, "hello") ||
      str_ieq(p, "hi"))
    return 0;
  if (str_ieq(p, "goodbye") || str_ieq(p, "bye") || str_ieq(p, "farewell") ||
      str_ieq(p, "done") || str_ieq(p, "leave")) {
    ent = world_room_entity(g_room);
    conv_clear();
    if (ent && ent[0]) {
      entity_pretty(ent, pretty, sizeof pretty);
      snprintf(msg, msgcap, "You take your leave of %s.", pretty);
    } else
      snprintf(msg, msgcap, "You end the conversation.");
    *turn_advance = 1;
    return 1;
  }
  if (*p >= '1' && *p <= '9') {
    pick = (int)(*p - '0');
    p++;
    while (*p == ' ') p++;
    if (*p == ')' || *p == '.' || *p == ':') p++;
    while (*p == ' ') p++;
    if (*p) return 0;
  } else {
    strncpy(work, p, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    for (i = 0; i < g_conv_pick_n; i++) {
      if (str_ieq(work, g_conv_pick_label[i]) ||
          text_has_word_ci(g_conv_pick_label[i], work)) {
        pick = i + 1;
        break;
      }
    }
    if (!pick) return 0;
  }
  if (pick < 1 || pick > g_conv_pick_n) return 0;
  ent = world_room_entity(g_room);
  if (!ent || !ent[0] || !str_ieq(ent, g_conv_npc)) {
    conv_clear();
    snprintf(msg, msgcap,
             "The conversation broke off — %s is no longer here.",
             g_conv_npc);
    *turn_advance = 1;
    return 1;
  }
  if (g_conv_pick_kind[pick - 1] == CONV_PICK_GOODBYE) {
    entity_pretty(ent, pretty, sizeof pretty);
    conv_clear();
    snprintf(msg, msgcap, "You say goodbye to %s.", pretty);
    *turn_advance = 1;
    return 1;
  }
  if (g_conv_pick_kind[pick - 1] == CONV_PICK_MORE) {
    const char *more = g_conv_pick_phrase[pick - 1];
    cmd_talk("", more[0] ? more : "tell me more", msg, msgcap);
    *turn_advance = 1;
    return 1;
  }
  cmd_talk("", g_conv_pick_phrase[pick - 1], msg, msgcap);
  *turn_advance = 1;
  return 1;
}

static void cmd_talk(const char *target, const char *topic, char *msg,
                     size_t msgcap) {
  const char *ent = world_room_entity(g_room);
  char pretty[128];
  char norm[MAX_ITEM_LEN];
  char twork[256];
  char xp[1200];
  const AetNpcLineSet *dlg;
  const AetNpcTopic *tp;

  twork[0] = '\0';
  if (topic && topic[0]) {
    strncpy(twork, topic, sizeof twork - 1);
    twork[sizeof twork - 1] = '\0';
    strip_trailing_space(twork);
  }

  if (twork[0] && topic_mentions_bucket(twork)) {
    static const char *const Vcarry[] = {
        "You make your case to the bucket. It offers no rebuttal, which feels "
        "like agreement from a certain point of view.",
        "You explain the bucket as metaphor. The bucket remains a bucket, "
        "unhelpfully literal.",
        "You attempt small talk. The bucket is better at silence than you are.",
        "You ask whether this is the right story. The bucket declines to "
        "confirm or deny.",
        "You propose alternate routes. The bucket is already on one."};
    static const char *const Vfloor[] = {
        "You address the bucket before picking it up. It listens with the "
        "patience of stage scenery.",
        "You outline your plans involving the bucket. They sound reasonable "
        "until you hear them aloud.",
        "You promise the bucket a better scene. It does not negotiate."};
    int nc = (int)(sizeof Vcarry / sizeof Vcarry[0]);
    int nf = (int)(sizeof Vfloor / sizeof Vfloor[0]);
    if (inv_has("bucket"))
      snprintf(msg, msgcap, "%s", Vcarry[(g_turns + g_room * 3) % nc]);
    else if (room_floor_has_id("bucket"))
      snprintf(msg, msgcap, "%s", Vfloor[(g_turns + g_room) % nf]);
    else if (ent[0]) {
      entity_pretty(ent, pretty, sizeof pretty);
      snprintf(
          msg, msgcap,
          "%s says, \"Buckets? Try the well if you want water. Try the road "
          "if you want trouble. Try both if you want a day.\"",
          pretty);
      remember_npc_here();
    } else {
      snprintf(msg, msgcap,
               "You hold forth on buckets. The room offers no debate team.");
    }
    return;
  }

  if (!ent[0]) {
    conv_clear();
    snprintf(msg, msgcap, "Nobody to talk to — just you, %s, and the scene.",
             pc_display_name());
    return;
  }
  if (g_conv_active && g_conv_npc[0] && !str_ieq(ent, g_conv_npc)) conv_clear();
  entity_pretty(ent, pretty, sizeof pretty);
  if (target && target[0]) {
    char resolved[MAX_ITEM_LEN];
    int pr = parser_expand_npc_pronoun(target, resolved, sizeof resolved, msg,
                                       msgcap);
    if (pr < 0) return;
    if (pr == 1) target = resolved;
    else if (parser_query_is_object_pronoun(target)) {
      if (!g_last_npc[0]) {
        snprintf(msg, msgcap,
                 "No one in mind for \"it\" — try  who  or  talk to <name>.");
        return;
      }
      if (!str_ieq(ent, g_last_npc)) {
        snprintf(msg, msgcap,
                 "Who you had in mind is not the one here now. Try  who.");
        return;
      }
      target = g_last_npc;
    } else {
      query_norm_underscore(norm, sizeof norm, target);
      if (!str_ieq(ent, target) && !str_ieq(pretty, target) &&
          strstr(ent, norm) == NULL && strstr(pretty, target) == NULL) {
        snprintf(msg, msgcap,
                 "You do not see anyone matching \"%s\". Here you only have %s.",
                 target, pretty);
        return;
      }
    }
  }

  dlg = aet_npc_lines(ent);

  if (twork[0]) {
    char tlc[256];
    const char *mt;
    char *p;
    strncpy(tlc, twork, sizeof tlc - 1);
    tlc[sizeof tlc - 1] = '\0';
    for (p = tlc; *p; p++) *p = (char)tolower((unsigned char)*p);
    mt = aet_mods_npc_topic_response(ent, tlc);
    if (mt && mt[0]) {
      char tag[32];
      pc_expand_world_placeholders(mt, xp, sizeof xp);
      conv_primary_tag(twork, tag, sizeof tag);
      if (conv_mem_has_tag(ent, tag))
        snprintf(msg, msgcap,
                 "%s says, \"%s\"\n\n(They nod — you have raised this before.)",
                 pretty, xp);
      else
        snprintf(msg, msgcap, "%s says, \"%s\"", pretty, xp);
      remember_npc_here();
      remember_talk_topic(twork);
      merchant_rep_bump_conversation(ent, 2);
      conv_finish_talk(ent, dlg, twork, msg, msgcap);
      return;
    }
    if (dlg && dlg->topics) {
      for (tp = dlg->topics; tp->keywords && tp->response; tp++) {
        if (npc_keywords_match(twork, tp->keywords)) {
          char tag[32];
          pc_expand_world_placeholders(tp->response, xp, sizeof xp);
          conv_primary_tag(tp->keywords, tag, sizeof tag);
          if (conv_mem_has_tag(ent, tag))
            snprintf(msg, msgcap,
                     "%s says, \"%s\"\n\n(They nod — you have raised this before.)",
                     pretty, xp);
          else
            snprintf(msg, msgcap, "%s says, \"%s\"", pretty, xp);
          remember_npc_here();
          remember_talk_topic(twork);
          merchant_rep_bump_conversation(ent, 2);
          conv_finish_talk(ent, dlg, twork, msg, msgcap);
          return;
        }
      }
    }
    snprintf(
        msg, msgcap,
        "%s listens, but does not pick up on that. Try other words or plain "
        "'talk'.",
        pretty);
    remember_npc_here();
    return;
  }

  if (dlg) {
    AetWorldClock wc;
    unsigned char bond = soc_derive_stage_npc_slug(ent);
    unsigned pick = (unsigned)(g_turns + g_room * 3 + (int)strlen(ent));
    const char *approach;
    const char *period_ln;
    get_world_clock(&wc);
    approach = npc_stage_approach(ent, bond);
    if (!twork[0] && approach && approach[0] && bond >= 1u &&
        (pick % 3u) != 2u) {
      char *prev = conv_mem_last(ent);
      pc_expand_world_placeholders(approach, xp, sizeof xp);
      if (prev && prev[0])
        snprintf(msg, msgcap, "%s still has \"%s\" on their mind.\n\n%s", pretty,
                 prev, xp);
      else
        snprintf(msg, msgcap, "%s", xp);
      remember_npc_here();
      merchant_rep_bump_conversation(ent, 2);
      conv_finish_talk(ent, dlg, NULL, msg, msgcap);
      return;
    }
    period_ln = npc_period_talk(ent, wc.period, (pick % 2u) == 0u);
    if (!twork[0] && period_ln && period_ln[0] && (pick % 5u) < 2u) {
      pc_expand_world_placeholders(period_ln, xp, sizeof xp);
      snprintf(msg, msgcap, "%s says, \"%s\"", pretty, xp);
      remember_npc_here();
      merchant_rep_bump_conversation(ent, 2);
      conv_finish_talk(ent, dlg, NULL, msg, msgcap);
      return;
    }
    {
    const char *mod_gr = aet_mods_npc_greeting(ent);
    const char *use_gr =
        (mod_gr && mod_gr[0]) ? mod_gr
                              : (dlg->greeting && dlg->greeting[0] ? dlg->greeting
                                                                   : NULL);
    if (use_gr && use_gr[0] && (pick % 3u) == 0u) {
      pc_expand_world_placeholders(use_gr, xp, sizeof xp);
      snprintf(msg, msgcap, "%s says, \"%s\"", pretty, xp);
      remember_npc_here();
      merchant_rep_bump_conversation(ent, 2);
      conv_finish_talk(ent, dlg, NULL, msg, msgcap);
      return;
    }
    if (dlg->chatter && dlg->chatter[0]) {
      const char *line = npc_pick_chatter(dlg);
      if (line && line[0]) {
        pc_expand_world_placeholders(line, xp, sizeof xp);
        snprintf(msg, msgcap, "%s says, \"%s\"", pretty, xp);
        remember_npc_here();
        merchant_rep_bump_conversation(ent, 2);
        conv_finish_talk(ent, dlg, NULL, msg, msgcap);
        return;
      }
    }
    if (use_gr && use_gr[0]) {
      pc_expand_world_placeholders(use_gr, xp, sizeof xp);
      snprintf(msg, msgcap, "%s says, \"%s\"", pretty, xp);
      remember_npc_here();
      merchant_rep_bump_conversation(ent, 2);
      conv_finish_talk(ent, dlg, NULL, msg, msgcap);
      return;
    }
    }
  }
  {
    char role[96];
    pc_format_role_phrase(role, sizeof role);
    snprintf(msg, msgcap,
             "You speak with %s (%s). Standing there as %s, you trade a few "
             "words; they nod, then settle back into what they were doing.",
             pretty, ent, role);
  }
  remember_npc_here();
  merchant_rep_bump_conversation(ent, 2);
  conv_finish_talk(ent, dlg, NULL, msg, msgcap);
}

static int take_item(const char *name, char *msg, size_t msgcap) {
  char target[MAX_ITEM_LEN];
  int i, j;
  if (g_inv_n >= MAX_INV) {
    snprintf(msg, msgcap,
             "You cannot carry more (%d-item limit). Drop something first.",
             MAX_INV);
    return 0;
  }
  g_disambig_next_act = DISAMBIG_ACT_TAKE;
  i = resolve_room_item_query(name, target, sizeof target, msg, msgcap);
  if (i < 0) return 0;
  if (i == 0) {
    char work[256], qnorm[MAX_ITEM_LEN];
    int prep;
    if (container_take_by_query(g_room, name, msg, msgcap)) return 1;
    prep = parser_prepare_object_query(name, work, sizeof work, NULL, NULL, NULL, 0);
    if (prep >= 0 && work[0]) {
      query_norm_underscore(qnorm, sizeof qnorm, work);
      if (container_hint_closed_item(g_room, work, qnorm, msg, msgcap)) return 0;
    }
    snprintf(msg, msgcap, "You do not see that here.");
    return 0;
  }
  for (i = 0; i < g_room_item_n[g_room]; i++) {
    if (str_ieq(g_room_items[g_room][i], target)) {
      char taken[MAX_ITEM_LEN];
      char pretty[96];
      strncpy(taken, g_room_items[g_room][i], MAX_ITEM_LEN - 1);
      taken[MAX_ITEM_LEN - 1] = '\0';
      item_pretty(taken, pretty, sizeof pretty);
      {
        int cash = tender_pickup_coins(taken);
        for (j = i; j < g_room_item_n[g_room] - 1; j++)
          memcpy(g_room_items[g_room][j], g_room_items[g_room][j + 1], MAX_ITEM_LEN);
        g_room_item_n[g_room]--;
        g_score += 5;
        if (item_is_container(taken)) container_on_item_taken(g_room, taken);
        if (cash > 0) {
          char cashb[48];
          g_coins += cash;
          currency_format_long(cash, cashb, sizeof cashb);
          snprintf(msg, msgcap,
                   "You take %s (%s go into your purse).", pretty, cashb);
        } else {
          inv_add(taken);
          if (str_ieq(taken, "bucket"))
            snprintf(
                msg, msgcap,
                "Taken: %s\n\n"
                "It settles against you with the polite mass of an object that "
                "knows it is in the wrong genre. Somewhere, a narrator clears "
                "their throat — or maybe that is only wind through the Nexus.",
                pretty);
          else if (item_has_read_text(taken))
            snprintf(msg, msgcap,
                     "Taken: %s\n\n(Try  read %s  when you want the full text.)",
                     pretty, taken);
          else
            snprintf(msg, msgcap, "Taken: %s", pretty);
        }
      }
      return 1;
    }
  }
  snprintf(msg, msgcap, "You do not see that here.");
  return 0;
}

static int cmd_protective_grab(const char *raw, char *msg, size_t msgcap) {
  const char *ent = world_room_entity(g_room);
  const char *pron = protective_phrase_pronoun(raw);
  char pretty[64];
  char detail[240];
  int named_here;
  if (!raw || !raw[0]) {
    snprintf(msg, msgcap,
             "Protect whom? Try a rescue-style phrase like grab their hand and pull them away from danger.");
    return 0;
  }
  if (!ent || !ent[0]) {
    snprintf(msg, msgcap, "No one is here to pull clear of danger.");
    return 0;
  }
  entity_pretty(ent, pretty, sizeof pretty);
  named_here = str_contains_ci(raw, ent) || str_contains_ci(raw, pretty);
  if (!named_here && !pron) {
    snprintf(msg, msgcap,
             "No one here matches that rescue target. Try who or talk to %s.",
             pretty);
    return 0;
  }
  if (g_intent.friendly || g_intent.careful) {
    if (pron)
      snprintf(msg, msgcap,
               "You read \"%s\" as %s here. You catch %s's hand and guide them clear with careful, protective intent. The parser reads it as a rescue move, not a pickup attempt.",
               pron, pretty, pretty);
    else
      snprintf(msg, msgcap,
               "You catch %s's hand and guide them clear with careful, protective intent. The parser reads it as a rescue move, not a pickup attempt.",
               pretty);
  } else if (g_intent.harsh || g_intent.loud) {
    snprintf(msg, msgcap,
             "You seize %s and haul them clear in one urgent motion. The parser still reads the line as protection under pressure, not possession or inventory play.",
             pretty);
  } else {
    snprintf(msg, msgcap,
             "You grab %s and pull them clear of the danger in one decisive motion. The parser reads it as protection, not a pickup attempt.",
             pretty);
  }
  remember_npc_here();
  g_last_intent = g_intent;
  snprintf(detail, sizeof detail, "%s: rescue phrase%s%s", ent,
           protective_phrase_has_danger_context(raw) ? " [danger]" : "",
           pron ? " [pronoun]" : "");
  causal_push("protective-grab", detail);
  return 1;
}

static void take_all(char *msg, size_t msgcap) {
  int n = 0;
  while (g_room_item_n[g_room] > 0) {
    if (!take_item(g_room_items[g_room][0], msg, msgcap)) break;
    n++;
  }
  if (n == 0 && g_room_item_n[g_room] == 0)
    snprintf(msg, msgcap, "Nothing to take here.");
  else if (n == 0) {
  } else if (g_room_item_n[g_room] > 0)
    snprintf(msg, msgcap, "You take what you can; the rest stays put.");
  else
    snprintf(msg, msgcap, "You gather everything here.");
}

static void search_room(char *msg, size_t msgcap) {
  char found[MAX_ITEM_LEN];
  char pretty[96];
  int j;
  AetPcSave pr;
  pc_capture(&pr);
  if (g_hidden_n[g_room] <= 0) {
    if (room_too_dark_to_see())
      snprintf(msg, msgcap,
               "You pat the walls and floor in the dark and find nothing new.%s",
               pr.per >= 14
                   ? " Your fingers map the cracks twice anyway — habit, not hope."
                   : "");
    else
      snprintf(
          msg, msgcap,
          "You search thoroughly but find nothing else concealed here.%s",
          pr.intl >= 16
              ? " Still, your eye keeps catching irregularities — worth another "
                "pass when the room changes."
              : "");
    return;
  }
  if (g_room_item_n[g_room] >= MAX_ITEMS_ROOM) {
    snprintf(msg, msgcap,
             "You brush against a concealed stash, but the floor is too full "
             "to pull it free. Drop something first.");
    return;
  }
  strncpy(found, g_hidden_items[g_room][0], MAX_ITEM_LEN - 1);
  found[MAX_ITEM_LEN - 1] = '\0';
  memcpy(g_room_items[g_room][g_room_item_n[g_room]], found,
         strlen(found) + 1);
  g_room_item_n[g_room]++;
  for (j = 0; j < g_hidden_n[g_room] - 1; j++)
    memcpy(g_hidden_items[g_room][j], g_hidden_items[g_room][j + 1],
           MAX_ITEM_LEN);
  g_hidden_n[g_room]--;
  g_score += 10;
  if (pr.per >= 15) g_score += 1;
  remember_focus_item(found);
  item_pretty(found, pretty, sizeof pretty);
  if (room_too_dark_to_see())
    snprintf(msg, msgcap,
             "Mostly by touch in the dark, you fish out: %s.%s", pretty,
             pr.per >= 15 ? " Your hands knew where to linger." : "");
  else
    snprintf(msg, msgcap,
             "You uncover a concealed stash: %s.%s", pretty,
             pr.per >= 15 ? " Details snap into place — you were built to notice."
                          : "");
}

static unsigned item_est_value(const char *s) {
  CraftSavedProfile *sp = craft_profile_lookup(s);
  unsigned h = 5381u;
  const unsigned char *p;
  char low[MAX_ITEM_LEN];
  size_t i, L;
  if (sp)
    return (unsigned)(12u + (unsigned)sp->quality * 2u +
                      (unsigned)sp->disp_shp * 3u + (unsigned)sp->disp_dur * 2u);
  if (!s || !s[0]) return 8u;
  L = strlen(s);
  if (L >= sizeof low) L = sizeof low - 1u;
  for (i = 0; i < L; i++) low[i] = (char)tolower((unsigned char)s[i]);
  low[L] = '\0';
  for (p = (const unsigned char *)s; *p; p++) h = ((h << 5) + h) + *p;
  h = (h % 160u) + 12u;
  if (strstr(low, "gold") || strstr(low, "coin") || strstr(low, "ruby") ||
      strstr(low, "gem") || strstr(low, "jewel"))
    h += 48u;
  if (strstr(low, "silver")) h += 28u;
  if (strstr(low, "key")) h += 18u;
  if (strstr(low, "lockpick")) h += 20u;
  if (strstr(low, "map") || strstr(low, "book")) h += 10u;
  return h;
}

static unsigned item_est_heft(const char *s) {
  CraftSavedProfile *sp = craft_profile_lookup(s);
  unsigned h = 2166136261u;
  const unsigned char *p;
  char low[MAX_ITEM_LEN];
  size_t i, L;
  if (sp) {
    h = (unsigned)sp->disp_wgt;
    if (h < 1u) h = 1u;
    if (h > 99u) h = 99u;
    return h;
  }
  if (!s || !s[0]) return 5u;
  L = strlen(s);
  if (L >= sizeof low) L = sizeof low - 1u;
  for (i = 0; i < L; i++) low[i] = (char)tolower((unsigned char)s[i]);
  low[L] = '\0';
  for (p = (const unsigned char *)s; *p; p++) h ^= (unsigned)*p, h *= 16777619u;
  h = (h % 42u) + 4u;
  if (strstr(low, "barrel") || strstr(low, "crate") || strstr(low, "anvil"))
    h += 28u;
  if (strstr(low, "bucket") || strstr(low, "sword") || strstr(low, "plate") ||
      strstr(low, "armor"))
    h += 14u;
  if (strstr(low, "leaflet") || strstr(low, "ring") || strstr(low, "key"))
    h -= h > 12u ? 10u : 0u;
  if (h < 1u) h = 1u;
  if (h > 99u) h = 99u;
  return h;
}

static int g_loot_sort_mode;

static int loot_ix_cmp(const void *ap, const void *bp) {
  const int *ia = (const int *)ap, *ib = (const int *)bp;
  const char *sa = g_room_items[g_room][*ia];
  const char *sb = g_room_items[g_room][*ib];
  if (g_loot_sort_mode == 1) {
    unsigned va = item_est_value(sa), vb = item_est_value(sb);
    if (va > vb) return -1;
    if (va < vb) return 1;
  } else if (g_loot_sort_mode == 2) {
    unsigned wa = item_est_heft(sa), wb = item_est_heft(sb);
    if (wa > wb) return -1;
    if (wa < wb) return 1;
  }
  return strcmp(sa, sb);
}

static void cmd_scan(char *msg, size_t msgcap) {
  int i, d, n_ex = 0, n_locked = 0;
  const char *ent;
  char pretty[96];
  char line[256];
  char banner[256];
  size_t len;

  pc_format_identity_banner(banner, sizeof banner);
  snprintf(msg, msgcap,
           "=== AREA SCAN ===\n\n"
           "%s\n\n"
           "Place: %s  [%s]\n"
           "Region: %s\n",
           banner, resolve_world_title(g_room), world_slug(g_room),
           world_region(g_room));

  ent = world_room_entity(g_room);
  if (ent && ent[0]) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(line, sizeof line, "People present: %s\n", pretty);
  } else
    snprintf(line, sizeof line, "People present: (none)\n");
  strncat(msg, line, msgcap - strlen(msg) - 1);

  snprintf(line, sizeof line, "Your pack: %d / %d slots used\n", g_inv_n,
           MAX_INV);
  strncat(msg, line, msgcap - strlen(msg) - 1);

  for (d = 0; d < DIR_COUNT; d++) {
    int dest = world_exit(g_room, d);
    int locked = 0, lk;
    if (dest < 0) continue;
    n_ex++;
    lk = exit_lock_info(g_room, d, NULL, 0, &locked, NULL);
    if (lk && locked) n_locked++;
  }
  snprintf(line, sizeof line,
           "Exits: %d directions open from here\n"
           "Locks on those exits (known): %d\n",
           n_ex, n_locked);
  strncat(msg, line, msgcap - strlen(msg) - 1);

  if (room_too_dark_to_see()) {
    strncat(msg,
            "\nLighting: poor — visual tally unreliable. Listen, search, or "
            "move.\n",
            msgcap - strlen(msg) - 1);
    snprintf(line, sizeof line,
             "Floor items (maybe incomplete): %d  |  Hidden stashes: %d\n",
             g_room_item_n[g_room], g_hidden_n[g_room]);
    strncat(msg, line, msgcap - strlen(msg) - 1);
    return;
  }

  strncat(msg,
          "Lighting: good enough for a full read of the floor.\n",
          msgcap - strlen(msg) - 1);
  snprintf(line, sizeof line, "Visible floor items: %d\n",
           g_room_item_n[g_room]);
  strncat(msg, line, msgcap - strlen(msg) - 1);
  if (g_room_item_n[g_room] > 0) {
    strncat(msg, "  ", msgcap - strlen(msg) - 1);
    for (i = 0; i < g_room_item_n[g_room] && strlen(msg) + 2 < msgcap; i++) {
      char pretty[96];
      if (i) strncat(msg, ", ", msgcap - strlen(msg) - 1);
      item_pretty(g_room_items[g_room][i], pretty, sizeof pretty);
      strncat(msg, pretty, msgcap - strlen(msg) - 1);
    }
    strncat(msg, "\n", msgcap - strlen(msg) - 1);
  }
  if (g_hidden_n[g_room] > 0) {
    snprintf(line, sizeof line,
             "Concealed: ~%d stash slot(s) — use search when you want to dig.\n",
             g_hidden_n[g_room]);
    strncat(msg, line, msgcap - strlen(msg) - 1);
  } else
    strncat(msg, "Concealed: nothing hinted.\n", msgcap - strlen(msg) - 1);

  container_scan_append(msg, msgcap, g_room);

  len = strlen(msg);
  if (len + 80 < msgcap && n_locked > 0)
    snprintf(msg + len, msgcap - len,
             "\nTip: lockcheck  —  noise  —  route <place>  for next moves.\n");
}

static void cmd_loot(char *msg, size_t msgcap, const char *sort) {
  int i, n, ix[64];
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  if (room_too_dark_to_see()) {
    snprintf(msg, msgcap,
             "%s\n\n"
             "Loot triage needs light — you only know *something* is scattered "
             "about.",
             banner);
    return;
  }
  n = g_room_item_n[g_room];
  if (n == 0) {
    snprintf(msg, msgcap,
             "%s\n\nNothing loose to loot in plain sight.",
             banner);
    return;
  }
  if (n > (int)(sizeof ix / sizeof ix[0])) n = (int)(sizeof ix / sizeof ix[0]);
  for (i = 0; i < n; i++) ix[i] = i;

  if (sort && !strcmp(sort, "value"))
    g_loot_sort_mode = 1;
  else if (sort && !strcmp(sort, "weight"))
    g_loot_sort_mode = 2;
  else
    g_loot_sort_mode = 0;
  if (g_loot_sort_mode)
    qsort(ix, (size_t)n, sizeof ix[0], loot_ix_cmp);

  snprintf(msg, msgcap,
           "=== LOOT TRIAGE ===\n\n"
           "%s\n\n"
           "Estimates are heuristic (this port has no full merchant price table).\n"
           "Sort: %s\n\n",
           banner,
           sort && !strcmp(sort, "value")
               ? "by est. barter value (high first)"
               : (sort && !strcmp(sort, "weight")
                      ? "by est. bulk / heft (heavy first)"
                      : "alphabetical"));

  for (i = 0; i < n && strlen(msg) + 120 < msgcap; i++) {
    const char *id = g_room_items[g_room][ix[i]];
    char pretty[96];
    char row[140];
    item_pretty(id, pretty, sizeof pretty);
    snprintf(row, sizeof row,
             "  %-36s  ~%3u coin  |  bulk ~%2u\n", pretty, item_est_value(id),
             item_est_heft(id));
    strncat(msg, row, msgcap - strlen(msg) - 1);
  }
  if (g_hidden_n[g_room] > 0) {
    strncat(msg,
            "\nBuried stock may exist — try search.\n",
            msgcap - strlen(msg) - 1);
  }
}

static int resolve_compare_item(const char *raw, char *out, size_t outcap,
                                char *err, size_t errcap) {
  char work[256], qnorm[MAX_ITEM_LEN];
  char picks[10][MAX_ITEM_LEN];
  int ordinal = 0, exclude_last = 0;
  int i, j, n = 0, prep;
  if (!raw || !raw[0]) {
    snprintf(err, errcap, "Name an object.");
    return 0;
  }
  prep = parser_prepare_object_query(raw, work, sizeof work, &ordinal,
                                     &exclude_last, err, errcap);
  if (prep < 0) return 0;
  if (!work[0]) {
    snprintf(err, errcap, "Name an object.");
    return 0;
  }
  query_norm_underscore(qnorm, sizeof qnorm, work);
  if (!room_too_dark_to_see()) {
    for (i = 0; i < g_room_item_n[g_room]; i++) {
      if (exclude_last && g_last_focus[0] &&
          str_ieq(g_room_items[g_room][i], g_last_focus))
        continue;
      if (!room_item_matches_query(g_room_items[g_room][i], work, qnorm)) continue;
      for (j = 0; j < n; j++)
        if (str_ieq(picks[j], g_room_items[g_room][i])) break;
      if (j < n) continue;
      if (n < 10)
        snprintf(picks[n++], sizeof picks[0], "%s", g_room_items[g_room][i]);
    }
  }
  for (i = 0; i < g_inv_n; i++) {
    if (exclude_last && g_last_focus[0] && str_ieq(g_inv[i], g_last_focus))
      continue;
    if (!room_item_matches_query(g_inv[i], work, qnorm)) continue;
    for (j = 0; j < n; j++)
      if (str_ieq(picks[j], g_inv[i])) break;
    if (j < n) continue;
    if (n < 10) snprintf(picks[n++], sizeof picks[0], "%s", g_inv[i]);
  }
  if (!room_too_dark_to_see())
    container_append_open_contents_picks(g_room, work, qnorm, picks, &n,
                                         exclude_last);
  if (n == 0) {
    snprintf(err, errcap,
             "No \"%s\" on the ground (visible) or in your pack.", raw);
    return 0;
  }
  if (n == 1 || (ordinal >= 1 && ordinal <= n)) {
    const char *slug = picks[ordinal >= 1 ? ordinal - 1 : 0];
    strncpy(out, slug, outcap - 1);
    out[outcap - 1] = '\0';
    return 1;
  }
  snprintf(err, errcap,
           "\"%s\" matches several things — use a clearer id or an ordinal "
           "(second pick, other pick).",
           raw);
  return 0;
}

static void cmd_compare(const char *rest, char *body, size_t cap) {
  char buf[INPUT_LINE_MAX];
  char *div, *a, *b;
  char ia[MAX_ITEM_LEN], ib[MAX_ITEM_LEN];
  char pretty_a[96], pretty_b[96];
  char err[256];
  char banner[256];
  unsigned va, vb, ha, hb;
  const char *la, *lb;

  pc_format_identity_banner(banner, sizeof banner);

  strncpy(buf, rest, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  strip_trailing_space(buf);
  div = strstr(buf, " / ");
  if (div) {
    *div = '\0';
    a = buf;
    b = div + 3;
  } else {
    div = strstr(buf, " vs ");
    if (!div) {
      snprintf(
          body, cap,
          "%s\n\n"
          "Compare what to what?\n\n"
          "Examples:\n"
          "  compare rusty pick / scrap metal\n"
          "  compare bread vs lockpick\n\n"
          "Both objects must be visible here or in your pack.\n",
          banner);
      return;
    }
    *div = '\0';
    a = buf;
    b = div + 4;
  }
  strip_leading_articles(a);
  strip_trailing_space(a);
  strip_leading_articles(b);
  strip_trailing_space(b);
  if (!resolve_compare_item(a, ia, sizeof ia, err, sizeof err)) {
    snprintf(body, cap, "%s\n\n%s", banner, err);
    return;
  }
  if (!resolve_compare_item(b, ib, sizeof ib, err, sizeof err)) {
    snprintf(body, cap, "%s\n\n%s", banner, err);
    return;
  }
  if (str_ieq(ia, ib)) {
    snprintf(body, cap,
             "%s\n\nThat is the same object twice.",
             banner);
    return;
  }
  va = item_est_value(ia);
  vb = item_est_value(ib);
  ha = item_est_heft(ia);
  hb = item_est_heft(ib);
  item_pretty(ia, pretty_a, sizeof pretty_a);
  item_pretty(ib, pretty_b, sizeof pretty_b);
  la = inv_has(ia) ? "in pack" : "on ground here";
  lb = inv_has(ib) ? "in pack" : "on ground here";
  {
    CraftSavedProfile *spa = craft_profile_lookup(ia);
    CraftSavedProfile *spb = craft_profile_lookup(ib);
    if (spa || spb) {
      snprintf(body, cap,
               "=== APPRAISAL ===\n\n"
               "%s\n\n"
               "Forged-item stat profiles (saved with your quicksave).\n\n"
               "%s\n"
               "  Est. value: ~%u coin   Bulk: ~%u   (%s)\n"
               "%s\n"
               "  Est. value: ~%u coin   Bulk: ~%u   (%s)\n\n",
               banner, pretty_a, va, ha, la, pretty_b, vb, hb, lb);
      if (spa)
        body_append(body, cap,
                    "%s\n"
                    "  Forge profile: Q:%d  Dur:%d  Sharp:%d  Handle:%d  Wgt:%d\n",
                    pretty_a, spa->quality, spa->disp_dur, spa->disp_shp,
                    spa->disp_hnd, spa->disp_wgt);
      if (spb)
        body_append(body, cap,
                    "%s\n"
                    "  Forge profile: Q:%d  Dur:%d  Sharp:%d  Handle:%d  Wgt:%d\n",
                    pretty_b, spb->quality, spb->disp_dur, spb->disp_shp,
                    spb->disp_hnd, spb->disp_wgt);
      return;
    }
  }

  snprintf(body, cap,
           "=== APPRAISAL ===\n\n"
           "%s\n\n"
           "Figures are quick estimates for this text edition — haggle in play.\n\n"
           "%s\n"
           "  Est. value: ~%u coin   Bulk: ~%u   (%s)\n\n"
           "%s\n"
           "  Est. value: ~%u coin   Bulk: ~%u   (%s)\n\n",
           banner, pretty_a, va, ha, la, pretty_b, vb, hb, lb);
  if (va > vb)
    strncat(body, "Likelier payday: first item.\n", cap - strlen(body) - 1);
  else if (vb > va)
    strncat(body, "Likelier payday: second item.\n", cap - strlen(body) - 1);
  else
    strncat(body, "Roughly even trade value.\n", cap - strlen(body) - 1);
  if (ha > hb)
    strncat(body, "More awkward to haul: first item.\n",
            cap - strlen(body) - 1);
  else if (hb > ha)
    strncat(body, "More awkward to haul: second item.\n",
            cap - strlen(body) - 1);
}

static int inv_find(const char *name) {
  char norm[MAX_ITEM_LEN];
  int i;
  query_norm_underscore(norm, sizeof norm, name);
  for (i = 0; i < g_inv_n; i++)
    if (str_ieq(g_inv[i], name) || str_ieq(g_inv[i], norm)) return i;
  return -1;
}

static int inv_take_out(int ix, char *taken, size_t taken_cap) {
  int j;
  if (ix < 0 || ix >= g_inv_n || !taken || taken_cap < 2) return 0;
  strncpy(taken, g_inv[ix], taken_cap - 1);
  taken[taken_cap - 1] = '\0';
  for (j = ix; j < g_inv_n - 1; j++)
    memcpy(g_inv[j], g_inv[j + 1], MAX_ITEM_LEN);
  g_inv_n--;
  if (str_ieq(g_ready_item, taken)) g_ready_item[0] = '\0';
  eq_remove_item_from_slots(taken);
  return 1;
}

static int drop_item(const char *name, char *msg, size_t msgcap) {
  int ix, j;
  char dropped[MAX_ITEM_LEN];
  char pretty[96];
  if (g_room_item_n[g_room] >= MAX_ITEMS_ROOM) {
    snprintf(msg, msgcap, "The floor here is too cluttered to drop that.");
    return 0;
  }
  ix = -1;
  g_disambig_next_act = DISAMBIG_ACT_DROP;
  j = resolve_inv_item_query(name, &ix, msg, msgcap);
  if (j < 0) return 0;
  if (j == 0 || ix < 0) {
    snprintf(msg, msgcap, "You are not carrying that.");
    return 0;
  }
  strncpy(dropped, g_inv[ix], MAX_ITEM_LEN - 1);
  dropped[MAX_ITEM_LEN - 1] = '\0';
  memcpy(g_room_items[g_room][g_room_item_n[g_room]], dropped,
         strlen(dropped) + 1);
  g_room_item_n[g_room]++;
  for (j = ix; j < g_inv_n - 1; j++)
    memcpy(g_inv[j], g_inv[j + 1], MAX_ITEM_LEN);
  g_inv_n--;
  g_score += 1;
  if (str_ieq(g_ready_item, dropped)) g_ready_item[0] = '\0';
  eq_remove_item_from_slots(dropped);
  remember_dropped_item(dropped);
  remember_focus_item(dropped);
  if (item_is_container(dropped)) container_on_item_dropped(g_room, dropped);
  item_pretty(dropped, pretty, sizeof pretty);
  if (str_ieq(dropped, "bucket"))
    snprintf(msg, msgcap,
             "Dropped: %s\n\n"
             "It meets the floor with a sound that insists this has happened "
             "before, in other stories, under other rules.",
             pretty);
  else
    snprintf(msg, msgcap, "Dropped: %s", pretty);
  return 1;
}

static void cmd_equip(const char *rest, char *msg, size_t msgcap) {
  char work[256];
  int ix;
  const char *id;
  strncpy(work, rest, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_trailing_space(work);
  strip_leading_articles(work);
  if (!work[0] || str_ieq(work, "nothing")) {
    g_ready_item[0] = '\0';
    g_eq_slots[EQ_WEAPON][0] = '\0';
    eq_sync_pc_sheet();
    snprintf(msg, msgcap, "You relax your grip.");
    return;
  }
  ix = -1;
  {
    int r;
    g_disambig_next_act = DISAMBIG_ACT_EQUIP;
    r = resolve_inv_item_query(work, &ix, msg, msgcap);
    if (r < 0) return;
    if (r == 0 || ix < 0) {
      snprintf(msg, msgcap, "You are not carrying that.");
      return;
    }
  }
  id = g_inv[ix];
  ix = inv_find(id);
  if (ix < 0) {
    snprintf(msg, msgcap, "You are not carrying that.");
    return;
  }
  strncpy(g_ready_item, g_inv[ix], sizeof g_ready_item - 1);
  g_ready_item[sizeof g_ready_item - 1] = '\0';
  snprintf(g_eq_slots[EQ_WEAPON], sizeof g_eq_slots[EQ_WEAPON], "%s", g_ready_item);
  eq_sync_pc_sheet();
  snprintf(msg, msgcap, "You ready %s.", g_ready_item);
}

static void drop_all(char *msg, size_t msgcap) {
  int n = 0;
  while (g_inv_n > 0) {
    if (!drop_item(g_inv[0], msg, msgcap)) break;
    n++;
  }
  if (n == 0 && g_inv_n == 0)
    snprintf(msg, msgcap, "You have nothing to drop.");
  else if (n == 0) {
  } else if (g_inv_n > 0)
    snprintf(msg, msgcap, "You drop what you can; the floor cannot hold more.");
  else
    snprintf(msg, msgcap, "You set everything down.");
}

static void normalize_bulk_exception_list(char *buf) {
  char tmp[320];
  size_t i, o = 0;
  if (!buf || !buf[0]) return;
  for (i = 0; buf[i] && o + 1 < sizeof tmp; i++) {
    if (buf[i] == ';') {
      tmp[o++] = ',';
      continue;
    }
    if (buf[i] == '&') {
      while (o > 0 && tmp[o - 1] == ' ') o--;
      if (o > 0 && tmp[o - 1] != ',') tmp[o++] = ',';
      continue;
    }
    if (i > 0 && buf[i - 1] == ' ' && !strncmp(buf + i, "and ", 4)) {
      while (o > 0 && tmp[o - 1] == ' ') o--;
      if (o > 0 && tmp[o - 1] != ',') tmp[o++] = ',';
      i += 3;
      continue;
    }
    if (i > 0 && buf[i - 1] == ' ' && !strncmp(buf + i, "plus ", 5)) {
      while (o > 0 && tmp[o - 1] == ' ') o--;
      if (o > 0 && tmp[o - 1] != ',') tmp[o++] = ',';
      i += 4;
      continue;
    }
    tmp[o++] = buf[i];
  }
  tmp[o] = '\0';
  snprintf(buf, 320, "%s", tmp);
}

static int item_excluded(const char *item, const char *except_csv) {
  char buf[320];
  char *tok;
  if (!except_csv || !except_csv[0]) return 0;
  strncpy(buf, except_csv, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  normalize_bulk_exception_list(buf);
  for (tok = strtok(buf, ","); tok != NULL; tok = strtok(NULL, ",")) {
    char qn[MAX_ITEM_LEN];
    while (*tok == ' ' || *tok == '\t') tok++;
    strip_leading_articles(tok);
    strip_trailing_space(tok);
    if (!tok[0]) continue;
    query_norm_underscore(qn, sizeof qn, tok);
    if (room_item_matches_query(item, tok, qn)) return 1;
  }
  return 0;
}

static void take_all_except(const char *except_csv, char *msg, size_t msgcap) {
  int guard = 0;
  int took = 0;
  if (!except_csv || !except_csv[0]) {
    snprintf(msg, msgcap, "Take all except what? (comma-separated names ok.)");
    return;
  }
  while (g_room_item_n[g_room] > 0 && guard++ < MAX_ITEMS_ROOM * 4) {
    int i, picked = -1;
    for (i = 0; i < g_room_item_n[g_room]; i++) {
      if (item_excluded(g_room_items[g_room][i], except_csv)) continue;
      picked = i;
      break;
    }
    if (picked < 0) break;
    if (!take_item(g_room_items[g_room][picked], msg, msgcap)) break;
    took++;
  }
  if (took == 0) {
    int i, any = 0;
    for (i = 0; i < g_room_item_n[g_room]; i++) {
      if (!item_excluded(g_room_items[g_room][i], except_csv)) {
        any = 1;
        break;
      }
    }
    if (!any)
      snprintf(msg, msgcap, "Nothing left to take (all visible items may match "
                            "your exception list).");
  } else if (g_room_item_n[g_room] > 0) {
    snprintf(msg, msgcap, "You take what you can; something remains here.");
  }
}

static void drop_all_except(const char *except_csv, char *msg, size_t msgcap) {
  int guard = 0;
  int dropped = 0;
  if (!except_csv || !except_csv[0]) {
    snprintf(msg, msgcap, "Drop all except what?");
    return;
  }
  while (g_inv_n > 0 && guard++ < MAX_INV * 2) {
    int i, picked = -1;
    for (i = 0; i < g_inv_n; i++) {
      if (item_excluded(g_inv[i], except_csv)) continue;
      picked = i;
      break;
    }
    if (picked < 0) break;
    if (!drop_item(g_inv[picked], msg, msgcap)) break;
    dropped++;
  }
  if (dropped == 0) {
    int i, any = 0;
    for (i = 0; i < g_inv_n; i++) {
      if (!item_excluded(g_inv[i], except_csv)) {
        any = 1;
        break;
      }
    }
    if (!any)
      snprintf(msg, msgcap,
               "You kept everything (exception matched all you carry).");
  } else if (g_inv_n > 0) {
    snprintf(msg, msgcap, "You drop what you can; you still carry some.");
  }
}

static void format_nearby_body(char *body, size_t cap, const char *mode) {
  int d;
  size_t len = 0;
  int detailed = mode && (!strcmp(mode, "detail") || !strcmp(mode, "details") ||
                          !strcmp(mode, "detailed"));
  int npc_only = mode && (!strcmp(mode, "npc") || !strcmp(mode, "npcs"));
  int locked_only = mode && (!strcmp(mode, "locked") || !strcmp(mode, "locks"));
  int w;
  {
    AetPcSave p;
    char role[96], pr[64];
    pc_capture(&p);
    pc_fill_narrative_defaults(&p);
    pc_format_role_phrase(role, sizeof role);
    pc_format_pronouns_short(p.gender[0] ? p.gender : "they", pr, sizeof pr);
    w = snprintf(body, cap, "Nearby — %s · %s · %s\n\n",
                 pc_display_name(), role, pr);
    if (w < 0) return;
    len = (size_t)w;
  }
  w = snprintf(body + len, cap > len ? cap - len : 0,
               "From \"%s\" [%s] — adjacent areas",
               resolve_world_title(g_room), world_slug(g_room));
  if (w < 0) return;
  len += (size_t)w;
  if (mode && mode[0]) {
    w = snprintf(body + len, cap > len ? cap - len : 0, " (%s)", mode);
    if (w < 0) return;
    len += (size_t)w;
  }
  w = snprintf(body + len, cap > len ? cap - len : 0, ":\n\n");
  if (w < 0) return;
  len += (size_t)w;
  for (d = 0; d < DIR_COUNT; d++) {
    int dest = world_exit(g_room, d);
    const char *es;
    int locked = 0, diff = 0, lock_known;
    char lname[64];
    if (dest < 0) continue;
    es = world_room_entity(dest);
    lock_known = exit_lock_info(g_room, d, lname, sizeof lname, &locked, &diff);
    if (npc_only && (!es || !es[0])) continue;
    if (locked_only && (!lock_known || !locked)) continue;
    w = snprintf(body + len, cap > len ? cap - len : 0,
                 "  %-10s  %s  [%s]%s%s%s\n", dir_name(d), resolve_world_title(dest),
                 world_slug(dest), g_visited[dest] ? "  (visited)" : "  (new)",
                 lock_known ? (locked ? "  [locked]" : "  [open]") : "",
                 (es && es[0]) ? "  [npc]" : "");
    if (w < 0) return;
    len += (size_t)w;
    if (detailed && len + 96 < cap) {
      const char *reg = world_region(dest);
      w = snprintf(body + len, cap - len,
                   "      region=%s  items=%d  hidden=%d%s%s\n",
                   reg[0] ? reg : "unknown", g_room_item_n[dest],
                   g_hidden_n[dest], lock_known ? "  lock=" : "",
                   lock_known ? lname : "");
      if (w < 0) return;
      len += (size_t)w;
    }
  }
  if (len < 8) {
    snprintf(body, cap, "No mapped exits from here (unexpected).");
  } else if ((npc_only || locked_only) && strstr(body, "\n  ") == NULL) {
    body_append(body, cap, "  (No adjacent areas match this mode.)\n");
  }
}

static void format_lockcheck_body(char *body, size_t cap) {
  int d, n = 0;
  char who[256];
  {
    AetPcSave p;
    char role[96], pr[64];
    pc_capture(&p);
    pc_fill_narrative_defaults(&p);
    pc_format_role_phrase(role, sizeof role);
    pc_format_pronouns_short(p.gender[0] ? p.gender : "they", pr, sizeof pr);
    snprintf(who, sizeof who, "%s · %s · %s", pc_display_name(), role, pr);
  }
  snprintf(body, cap,
           "Lockcheck\n\n"
           "%s\n\n"
           "Current: %s  [%s]\n"
           "Best tool: %s\n\n",
           who, resolve_world_title(g_room), world_slug(g_room),
           best_lock_tool());
  for (d = 0; d < DIR_COUNT; d++) {
    int dest = world_exit(g_room, d);
    int locked = 0, diff = 0;
    char lname[64];
    if (dest < 0) continue;
    if (!exit_lock_info(g_room, d, lname, sizeof lname, &locked, &diff))
      continue;
    body_append(body, cap,
                "  %-10s %-14s -> %-24s [%s]\n"
                "      state=%s  difficulty=%d  tool=%s\n",
                dir_name(d), lname, resolve_world_title(dest), world_slug(dest),
                locked ? "locked" : "open", diff, has_lockpick_tool() ? "ready" : "missing");
    n++;
  }
  if (!n)
    body_append(body, cap,
                "  No lock-aware exits are adjacent. Use where locks or exits "
                "locked from likely doors.\n");
  else if (!has_lockpick_tool())
    body_append(body, cap,
                "\nCarry a lockpick, fine_lockpick, rusty_pick, or skeleton_key "
                "before a real lockpicking minigame starts.\n");
}

static void format_noise_body(char *body, size_t cap) {
  AetWorldClock wc;
  int adjacent_npcs = count_adjacent_npcs();
  int locks = count_adjacent_locks(1);
  int risk = 1;
  const char *ent = world_room_entity(g_room);
  const char *band = "LOW";
  get_world_clock(&wc);
  if (ent && ent[0]) risk += 2;
  if (adjacent_npcs > 0) risk += 1;
  if (locks > 0) risk += 1;
  if (str_ieq(wc.period, "night")) risk += 1;
  if (str_ieq(wc.weather, "storm"))
    risk -= 1;
  else if (str_ieq(wc.weather, "fog"))
    risk += 1;
  else if (str_ieq(wc.weather, "snow"))
    risk += 1;
  if (risk < 1) risk = 1;
  if (risk >= 5)
    band = "HIGH";
  else if (risk >= 3)
    band = "MEDIUM";

  {
    char who[256];
    char isuf[96];
    AetPcSave p;
    char role[96], pr[64];
    pc_capture(&p);
    pc_fill_narrative_defaults(&p);
    format_intent_suffix(isuf, sizeof isuf, &g_last_intent);
    pc_format_role_phrase(role, sizeof role);
    pc_format_pronouns_short(p.gender[0] ? p.gender : "they", pr, sizeof pr);
    snprintf(who, sizeof who, "%s · %s · %s", pc_display_name(), role, pr);
    snprintf(body, cap,
             "Noise / Stealth\n\n"
             "%s\n\n"
             "Current: %s  [%s]\n"
             "Band: %s\n"
             "Last intent: %s\n"
             "Time: %s, %s weather\n"
             "People here: %s\n"
             "NPCs one exit away: %d\n"
             "Adjacent locked exits: %d\n\n",
             who, resolve_world_title(g_room), world_slug(g_room), band,
             isuf[0] ? isuf + 1 : "none",
             wc.period, wc.weather, (ent && ent[0]) ? ent : "none",
             adjacent_npcs, locks);
  }
  if (g_lockpick_last_noise > 0 || g_lockpick_suspicion > 0) {
    body_append(body, cap,
                "Last lockpick noise: %d  Suspicion: %d/10\n",
                g_lockpick_last_noise, g_lockpick_suspicion);
  }
  if (!strcmp(band, "LOW"))
    body_append(body, cap,
                "Quiet actions are unlikely to carry far. Careful lock work "
                "or hunting setup has room to breathe.\n");
  else if (!strcmp(band, "MEDIUM"))
    body_append(body, cap,
                "Sound may carry. Future stealth/minigame checks should reward "
                "careful pacing, waiting, or better tools.\n");
  else
    body_append(body, cap,
                "High pressure. Loud failures should alert nearby people or "
                "raise suspicion once those systems are active.\n");
  {
    char social[96], block[96];
    causal_row_compact(causal_latest_row_matching("[talk"), social, sizeof social);
    if (!strcmp(social, "none"))
      causal_row_compact(causal_latest_row_matching("[say"), social, sizeof social);
    causal_row_compact(causal_latest_row_matching("[move-blocked]"), block, sizeof block);
    body_append(body, cap,
                "\nRecent trigger context:\n"
                "  social/noise: %s\n"
                "  movement block: %s\n",
                social, block);
  }
}

static int bfs_route(int from, int to, int *parent) {
  int q[MAX_WORLD_ROOMS];
  int qh, qt, u, d;
  unsigned char vis[MAX_WORLD_ROOMS];
  if (from < 0 || to < 0 || from >= WORLD_ROOM_COUNT ||
      to >= WORLD_ROOM_COUNT)
    return 0;
  memset(vis, 0, sizeof vis);
  for (u = 0; u < WORLD_ROOM_COUNT; u++) parent[u] = -1;
  qh = 0;
  qt = 0;
  q[qt++] = from;
  vis[from] = 1;
  parent[from] = -1;
  while (qh < qt) {
    u = q[qh++];
    if (u == to) return 1;
    for (d = 0; d < DIR_COUNT; d++) {
      int v = world_exit(u, d);
      if (v >= 0 && v < WORLD_ROOM_COUNT && !vis[v]) {
        vis[v] = 1;
        parent[v] = u;
        if (qt < MAX_WORLD_ROOMS) q[qt++] = v;
      }
    }
  }
  return 0;
}

static int exit_dir_from_to(int from_room, int to_room) {
  int d;
  if (from_room < 0 || to_room < 0 || from_room >= WORLD_ROOM_COUNT ||
      to_room >= WORLD_ROOM_COUNT)
    return -1;
  for (d = 0; d < DIR_COUNT; d++)
    if (world_exit(from_room, d) == to_room) return d;
  return -1;
}

static int resolve_room_index(const char *raw, char *err, size_t errcap) {
  char q[160];
  char qn[MAX_ITEM_LEN];
  int i, best = -1, nmatch = 0;
  if (!raw || !raw[0]) {
    snprintf(err, errcap, "Name a room or place.");
    return -1;
  }
  strncpy(q, raw, sizeof q - 1);
  q[sizeof q - 1] = '\0';
  strip_leading_articles(q);
  strip_trailing_space(q);
  query_norm_underscore(qn, sizeof qn, q);
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    const char *slug = world_slug(i);
    if (!slug[0]) continue;
    if (str_ieq(slug, q) || str_ieq(slug, qn)) return i;
  }
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    const char *slug = world_slug(i);
    const char *tit = resolve_world_title(i);
    const char *ent = world_room_entity(i);
    char pretty[120];
    int hit = 0;
    if (slug[0] && strlen(qn) >= 3 && strstr(slug, qn) != NULL) hit = 1;
    if (!hit && tit[0] && strlen(q) >= 3) {
      char low[200];
      size_t k, L = strlen(tit);
      if (L >= sizeof low) L = sizeof low - 1;
      for (k = 0; k < L; k++)
        low[k] = (char)tolower((unsigned char)tit[k]);
      low[L] = '\0';
      if (strstr(low, q) != NULL) hit = 1;
    }
    if (!hit && ent && ent[0]) {
      if (str_ieq(ent, q) || str_ieq(ent, qn)) hit = 1;
      else if (strlen(q) >= 3) {
        entity_pretty(ent, pretty, sizeof pretty);
        if (str_contains_ci(pretty, q)) hit = 1;
      }
    }
    if (hit) {
      best = i;
      nmatch++;
    }
  }
  if (nmatch == 1) return best;
  if (nmatch == 0) {
    snprintf(err, errcap, "No place matches \"%s\".", raw);
    return -1;
  }
  snprintf(err, errcap,
           "\"%s\" matches more than one area — use a slug from \"where\".",
           raw);
  return -1;
}

static int try_approach_named_place(const char *raw, char *msg, size_t msgcap) {
  char err[256];
  int dest, d;
  dest = resolve_room_index(raw, err, sizeof err);
  if (dest < 0) {
    snprintf(msg, msgcap, "%s", err);
    return 0;
  }
  if (dest == g_room) {
    snprintf(msg, msgcap, "You are already there.");
    return 0;
  }
  d = exit_dir_from_to(g_room, dest);
  if (d < 0) {
    snprintf(
        msg, msgcap,
        "That is not one step away — try route %s for a multi-step path.",
        world_slug(dest));
    return 0;
  }
  return try_move(d, msg, msgcap);
}

static void format_route_body(char *body, size_t cap, int dest) {
  static const char *dir_label[] = {
      "north",     "south",     "east",      "west",      "up",        "down",
      "northeast", "northwest", "southeast", "southwest", "in",        "out",
      "deeper",    "upstream",  "downstream", "fountain", "stage",
      "board",     "square"};
  int parent[MAX_WORLD_ROOMS];
  int path[MAX_WORLD_ROOMS];
  int plen = 0, u, i;
  char step[200];
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  if (dest == g_room) {
    snprintf(body, cap, "%s\n\nYou are already there.\n", banner);
    return;
  }
  if (!bfs_route(g_room, dest, parent)) {
    snprintf(body, cap,
             "%s\n\nNo route found (disconnected graph in this extract).\n",
             banner);
    return;
  }
  u = dest;
  while (u >= 0) {
    if (plen < MAX_WORLD_ROOMS) path[plen++] = u;
    u = parent[u];
  }
  snprintf(body, cap, "%s\n\nRoute (%d steps) — follow edges from here:\n\n",
           banner, plen - 1);
  for (i = plen - 1; i > 0; i--) {
    int here = path[i];
    int nextroom = path[i - 1];
    int d = exit_dir_from_to(here, nextroom);
    const char *dn =
        (d >= 0 && d < DIR_COUNT) ? dir_label[d] : "(?)";
    snprintf(step, sizeof step, "  go %-10s →  %s  [%s]\n", dn,
             resolve_world_title(nextroom), world_slug(nextroom));
    strncat(body, step, cap - strlen(body) - 1);
  }
}

static void format_locate_body(char *body, size_t cap, const char *raw) {
  char qn[MAX_ITEM_LEN];
  char buf[160];
  char banner[256];
  AetWorldClock wc;
  size_t i;
  int hits = 0;
  strncpy(buf, raw, sizeof buf - 1);
  buf[sizeof buf - 1] = '\0';
  strip_leading_articles(buf);
  strip_trailing_space(buf);
  query_norm_underscore(qn, sizeof qn, buf);
  pc_format_identity_banner(banner, sizeof banner);
  get_world_clock(&wc);
  snprintf(body, cap, "%s\n\nSearch for \"%s\":\n\n", banner, raw);
  for (i = 0; i < npc_routine_count(); i++) {
    const AetNpcRoutine *rt = npc_routine_at(i);
    char pretty[96];
    char line[240];
    const char *room_slug;
    int rr;
    const char *act;
    entity_pretty(rt->slug, pretty, sizeof pretty);
    if (!(str_ieq(rt->slug, buf) || strstr(rt->slug, qn) != NULL ||
          str_ieq(pretty, buf) || strstr(pretty, buf) != NULL))
      continue;
    room_slug = npc_routine_room_special(rt, &wc);
    rr = room_slug ? world_room_index(room_slug) : -1;
    act = npc_activity_for_period(rt->slug, wc.period);
    if (rr >= 0 && act && act[0])
      snprintf(line, sizeof line,
               "  %s  [%s]  —  %s  [%s]  (%s: %s)\n", pretty, rt->slug,
               resolve_world_title(rr), room_slug, wc.period, act);
    else if (rr >= 0)
      snprintf(line, sizeof line, "  %s  [%s]  —  %s  [%s]  (%s routine)\n",
               pretty, rt->slug, resolve_world_title(rr), room_slug, wc.period);
    else
      snprintf(line, sizeof line, "  %s  [%s]  —  (unknown room %s)\n", pretty,
               rt->slug, room_slug ? room_slug : "?");
    strncat(body, line, cap - strlen(body) - 1);
    hits++;
  }
  for (i = 0; i < (size_t)WORLD_ROOM_COUNT; i++) {
    const char *ent = world_room_entity((int)i);
    char pretty[96];
    char line[200];
    if (!ent[0] || npc_routine_for(ent)) continue;
    entity_pretty(ent, pretty, sizeof pretty);
    if (str_ieq(ent, buf) || strstr(ent, qn) != NULL || str_ieq(pretty, buf)) {
      snprintf(line, sizeof line, "  %s  [%s]  —  %s\n", pretty, ent,
               resolve_world_title((int)i));
      strncat(body, line, cap - strlen(body) - 1);
      hits++;
    }
  }
  if (hits == 0)
    strncat(body, "  (No NPC matches that name. Try  who all  or a shorter slug.)\n",
            cap - strlen(body) - 1);
}

static int soc_friendship_for_slug(const char *slug) {
  int mix, si;
  if (!slug || !slug[0]) return 0;
  mix = aet_merchant_index(slug);
  if (mix >= 0 && mix < AETER_REP_MAX)
    return (int)g_npc_friendship[mix];
  si = soc_npc_find(slug);
  if (si >= 0 && si < AETER_SOC_NPC_MAX)
    return (int)g_soc_npc_friendship[si];
  return 0;
}

static int quest_journal_hint_state(const char *hint) {
  int n, f, st;
  if (!hint || !hint[0]) return 0;
  if (strstr(hint, "scrap_metal")) {
    n = inv_count("scrap_metal");
    return n >= 10 ? 2 : (n > 0 ? 1 : 0);
  }
  if (strstr(hint, "iron_ingot")) {
    n = inv_count("iron_ingot");
    return n >= 2 ? 2 : (n > 0 ? 1 : 0);
  }
  if (strstr(hint, "flour")) {
    n = inv_count("flour");
    return n >= 3 ? 2 : (n > 0 ? 1 : 0);
  }
  if (strstr(hint, "wood_scrap")) {
    n = inv_count("wood_scrap");
    return n >= 5 ? 2 : (n > 0 ? 1 : 0);
  }
  if (strstr(hint, "meat")) {
    n = inv_count("meat");
    return n >= 3 ? 2 : (n > 0 ? 1 : 0);
  }
  if (strstr(hint, "gold_coin")) {
    n = inv_count("gold_coin");
    return n >= 5 ? 2 : (n > 0 ? 1 : 0);
  }
  if (strstr(hint, "Visit The Blacksmith")) {
    {
      int br = world_room_index("blacksmith");
      if ((br >= 0 && g_visited[br]) ||
          str_ieq(world_slug(g_room), "blacksmith"))
        return 2;
    }
    {
      const char *ent = world_room_entity(g_room);
      if (ent && str_ieq(ent, "blacksmith")) return 1;
    }
    return 0;
  }
  if (strstr(hint, "Help The Miller Today")) {
    f = soc_friendship_for_slug("miller");
    return f >= 35 ? 2 : (f >= 12 ? 1 : 0);
  }
  if (strstr(hint, "Help The Miller") && !strstr(hint, "Today")) {
    f = soc_friendship_for_slug("miller");
    return f >= 28 ? 2 : (f >= 8 ? 1 : 0);
  }
  if (strstr(hint, "Become Partners with The Miller")) {
    st = (int)soc_derive_stage_npc_slug("miller");
    return st >= 6 ? 2 : (st >= 4 ? 1 : 0);
  }
  if (strstr(hint, "Win The Miller's Heart")) {
    st = (int)soc_derive_stage_npc_slug("miller");
    return st >= 5 ? 2 : (st >= 3 ? 1 : 0);
  }
  if (strstr(hint, "Become Friends with The Miller")) {
    st = (int)soc_derive_stage_npc_slug("miller");
    return st >= 3 ? 2 : (st >= 1 ? 1 : 0);
  }
  if (strstr(hint, "Befriend The Tavern Keeper")) {
    st = (int)soc_derive_stage_npc_slug("tavern_keeper");
    return st >= 3 ? 2 : (st >= 1 ? 1 : 0);
  }
  if (strstr(hint, "Befriend The Traveling Merchant")) {
    st = (int)soc_derive_stage_npc_slug("traveling_merchant");
    return st >= 3 ? 2 : (st >= 1 ? 1 : 0);
  }
  if (strstr(hint, "Bartender")) {
    f = soc_friendship_for_slug("bartender");
    if (f < 8) f = soc_friendship_for_slug("tavern_keeper");
    return f >= 45 ? 2 : (f >= 18 ? 1 : 0);
  }
  return 0;
}

static const char *quest_journal_state_label(int st) {
  if (st >= 2) return "done";
  if (st == 1) return "active";
  return "open";
}

static void format_journal_body(char *body, size_t cap) {
  const char *const *qh = world_quest_hints();
  size_t len;
  int w, done_n = 0, active_n = 0, open_n = 0, daily_n = 0;
  char role[96];
  char pr[64];
  char purseb[48];
  AetPcSave jp;
  pc_capture(&jp);
  pc_fill_narrative_defaults(&jp);
  pc_format_role_phrase(role, sizeof role);
  pc_format_pronouns_short(jp.gender[0] ? jp.gender : "they", pr, sizeof pr);
  currency_format_compact(g_coins, purseb, sizeof purseb);
  snprintf(body, cap,
           "Journal\n\n"
           "  Character: %s (%s)\n"
           "  Pronouns: %s\n\n"
           "  Location: %s  [%s]\n"
           "  Explored: %d / %d locations\n"
           "  Health: %d / %d   Purse: %s   Score: %d   Turns: %d\n\n"
           "  Story arc: use  objectives  for the house-to-treasure checklist.\n"
           "  This board tracks village dailies and relationship goals from the "
           "quest list.\n",
           pc_display_name(), role, pr, resolve_world_title(g_room),
           world_slug(g_room), count_visited(), WORLD_ROOM_COUNT, g_health,
           g_max_health, purseb, g_score, g_turns);
  len = strlen(body);
  if (!qh || !*qh || len + 80 >= cap) return;
  for (const char *const *hq = qh; *hq; hq++) {
    int st = quest_journal_hint_state(*hq);
    if (strstr(*hq, "[Daily]")) daily_n++;
    if (st >= 2)
      done_n++;
    else if (st == 1)
      active_n++;
    else
      open_n++;
  }
  w = snprintf(body + len, cap - len,
               "\nQuest board  (done %d · active %d · open %d · %d daily)\n"
               "  [x] met   [~] in progress   [ ] not started\n\n",
               done_n, active_n, open_n, daily_n);
  if (w < 0 || (size_t)w >= cap - len) return;
  len += (size_t)w;
  for (; *qh && len + 12 < cap; qh++) {
    int st = quest_journal_hint_state(*qh);
    char mark = (st >= 2) ? 'x' : (st == 1 ? '~' : ' ');
    w = snprintf(body + len, cap - len, "  [%c] (%s) %s\n", mark,
                 quest_journal_state_label(st), *qh);
    if (w < 0 || (size_t)w >= cap - len) break;
    len += (size_t)w;
  }
  if (len + 48 < cap)
    body_append(body, cap,
                "\nTip: carry delivery goods before visiting; talk and trade to "
                "raise social goals.\n");
}

static void aptitude_format_bar(char *out, size_t cap, int stat) {
  int filled, i;
  if (!out || cap < 12) return;
  if (stat < 1) stat = 1;
  if (stat > 20) stat = 20;
  filled = (stat * 10 + 19) / 20;
  for (i = 0; i < 10; i++) out[i] = (i < filled) ? '#' : '-';
  out[10] = '\0';
}

static const char *aptitude_tier(int stat) {
  if (stat >= 16) return "peak";
  if (stat >= 13) return "solid";
  if (stat >= 10) return "capable";
  if (stat >= 7) return "developing";
  return "feeble";
}

static void aptitude_stat_line(char *body, size_t cap, const char *label, int val) {
  char bar[12];
  aptitude_format_bar(bar, sizeof bar, val);
  body_append(body, cap, "    %-4s %2d  [%s]  %s\n", label, val, bar,
              aptitude_tier(val));
}

static void format_aptitudes_body(char *body, size_t cap) {
  AetPcSave p;
  int any = 0;
  int buy_edge, sell_edge;
  int prof = g_craft_proficiency;
  char prof_bar[12];
  char pr[64];
  if (prof < 1) prof = 1;
  if (prof > 10) prof = 10;
  pc_capture(&p);
  pc_fill_narrative_defaults(&p);
  buy_edge = merchant_trade_buy_skill_pct(p.cha);
  sell_edge = merchant_trade_sell_skill_pct(p.cha);
  pc_format_pronouns_short(p.gender[0] ? p.gender : "they", pr, sizeof pr);
  aptitude_format_bar(prof_bar, sizeof prof_bar, prof * 2);
  snprintf(
      body, cap,
      "Aptitudes\n\n"
      "Saved attributes read as tendencies — each lane shows a ten-block bar "
      "(# = strength).\n\n");
  body_append(body, cap, "Name: %s\nClass: %s\nPronouns: %s\n\n",
              p.name[0] ? p.name : "(unnamed)",
              p.class_[0] ? p.class_ : "adventurer", pr);
  body_append(body, cap, "Physical\n");
  aptitude_stat_line(body, cap, "STR", p.str);
  aptitude_stat_line(body, cap, "TOU", p.tou);
  aptitude_stat_line(body, cap, "SPE", p.spe);
  aptitude_stat_line(body, cap, "AGI", p.agi);
  body_append(body, cap, "\nMental\n");
  aptitude_stat_line(body, cap, "INT", p.intl);
  aptitude_stat_line(body, cap, "WIS", p.wis);
  aptitude_stat_line(body, cap, "WILL", p.will);
  aptitude_stat_line(body, cap, "PER", p.per);
  body_append(body, cap, "\nSocial\n");
  aptitude_stat_line(body, cap, "CHA", p.cha);
  aptitude_stat_line(body, cap, "COR", p.cor);
  body_append(body, cap, "\nWorkbench familiarity: %d / 10  [%s]\n\n", prof, prof_bar);
  body_append(body, cap, "Knacks (when two strengths line up)\n");
  if (p.str >= 10 && p.tou >= 10) {
    any = 1;
    body_append(body, cap,
                "  • Iron tempo — you default to sustained effort, not glass "
                "cannon bursts.\n");
  }
  if (p.agi >= 10 && p.spe >= 10) {
    any = 1;
    body_append(body, cap,
                "  • Edge timing — speed reads as precision, not panic.\n");
  }
  if (p.intl >= 10 && p.per >= 10) {
    any = 1;
    body_append(body, cap,
                "  • Pattern sense — odd details register before conclusions.\n");
  }
  if (p.cha >= 10 && p.wis >= 10) {
    any = 1;
    body_append(body, cap,
                "  • Measured presence — influence without theatrics.\n");
  }
  if (p.will >= 12 && p.wis >= 9) {
    any = 1;
    body_append(body, cap,
                "  • Quiet stubbornness — hard to sway once you commit.\n");
  }
  if (!any)
    body_append(body, cap,
                "  • Balanced or still forming — no pair dominated the chart.\n");
  body_append(body, cap,
              "\nCommerce\n"
              "  • Trading edge — %s. Current read: buy %d%%, sell %d%% "
              "(merchant goods under 4 copper may still round flat).\n",
              merchant_trade_skill_label(p.cha), buy_edge, sell_edge);
  body_append(
      body, cap,
      "\nTry  character  for the portrait,  objectives  for the world checklist.\n"
      "Mods can append DLC callouts below.\n");
}

static void format_reputation_body(char *body, size_t cap) {
  int n = aet_merchant_count();
  int i;
  char banner[256];
  if (cap < 64) return;
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(
      body, cap,
      "Reputation & patron standing\n\n"
      "%s\n\n"
      "Trade and real conversation move a simple score per merchant. Tiers are "
      "plain\n",
      banner);
  body_append(
      body, cap,
      "labels — the fun is in prices and how NPC blurbs feel over time, not "
      "grind.\n\n"
      "How it moves\n"
      "  •  Successful talk topics  (+2 base; CHA 15+ +1, CHA 18+ another +1)\n"
      "  •  Buys and sells here  (+1 each)  — repeat visits matter\n"
      "  •  Gifts (give)  (+3; CHA 14+ +1)  — merchant must be present\n\n"
      "Effects (when trading here)\n"
      "  Stranger → better prices as you climb toward Renowned (caps modest so "
      "the\n"
      "  economy cannot break).\n"
      "  Your CHA also adds a separate trading edge shown in wares and "
      "aptitudes.\n\n");
  if (n <= 0) {
    body_append(body, cap, "(No merchant roster in this world.)\n");
    return;
  }
  body_append(body, cap, "Merchants (sorted as in world data)\n");
  for (i = 0; i < n && i < AETER_REP_MAX; i++) {
    const char *slug = aet_merchant_slug_at(i);
    char pretty[96];
    int sc = merchant_rep_score(i);
    int bd = merchant_buy_discount_pct(sc);
    int sb = merchant_sell_bonus_pct(sc);
    if (!slug || !slug[0]) continue;
    entity_pretty(slug, pretty, sizeof pretty);
    body_append(body, cap, "  • %-22s  %s (%d)  — buy ~%d%% off, sell ~+%d%%\n",
                pretty, merchant_rep_tier_label(sc), sc, bd, sb);
  }
  body_append(body, cap,
              "\nNotes and objectives still track story beats; mods can append "
              "faction\nfiction under  reputation  via reputation_append.txt.\n");
}

static int note_is_done(const char *note) {
  return note && !strncmp(note, "[x] ", 4);
}

static const char *note_text_only(const char *note) {
  if (!note) return "";
  if (!strncmp(note, "[x] ", 4) || !strncmp(note, "[ ] ", 4)) return note + 4;
  return note;
}

static int str_contains_ci(const char *haystack, const char *needle) {
  size_t nl;
  const char *p;
  if (!haystack || !needle || !needle[0]) return 0;
  nl = strlen(needle);
  for (p = haystack; *p; p++) {
    size_t i;
    for (i = 0; i < nl; i++) {
      if (!p[i]) return 0;
      if (tolower((unsigned char)p[i]) !=
          tolower((unsigned char)needle[i]))
        break;
    }
    if (i == nl) return 1;
  }
  return 0;
}

static int text_has_word_ci(const char *text, const char *word) {
  size_t wl;
  const char *p;
  if (!text || !word || !word[0]) return 0;
  wl = strlen(word);
  for (p = text; *p; p++) {
    size_t i = 0;
    if (p != text &&
        (isalnum((unsigned char)p[-1]) || (unsigned char)p[-1] == '_'))
      continue;
    while (i < wl && p[i] &&
           tolower((unsigned char)p[i]) == tolower((unsigned char)word[i]))
      i++;
    if (i == wl &&
        !(isalnum((unsigned char)p[i]) || (unsigned char)p[i] == '_'))
      return 1;
  }
  return 0;
}

static int protective_phrase_has_danger_context(const char *text) {
  return str_contains_ci(text, "away from danger") ||
         str_contains_ci(text, "out of danger") ||
         str_contains_ci(text, "to safety") ||
         text_has_word_ci(text, "danger") || text_has_word_ci(text, "harm") ||
         text_has_word_ci(text, "trouble") || text_has_word_ci(text, "threat") ||
         text_has_word_ci(text, "fire") || text_has_word_ci(text, "safe") ||
         text_has_word_ci(text, "safety");
}

static int protective_phrase_is_candidate(const char *text) {
  int has_hand;
  int has_pull;
  int has_route;
  if (!text || !text[0]) return 0;
  has_hand = text_has_word_ci(text, "hand") || text_has_word_ci(text, "arm") ||
             text_has_word_ci(text, "wrist");
  has_pull = text_has_word_ci(text, "pull") || text_has_word_ci(text, "guide") ||
             text_has_word_ci(text, "lead") || text_has_word_ci(text, "draw");
  has_route = str_contains_ci(text, "away from") ||
              text_has_word_ci(text, "aside") || text_has_word_ci(text, "clear") ||
              str_contains_ci(text, "to safety");
  return has_hand && has_pull && has_route &&
         protective_phrase_has_danger_context(text);
}

static const char *protective_phrase_pronoun(const char *text) {
  if (text_has_word_ci(text, "her")) return "her";
  if (text_has_word_ci(text, "him")) return "him";
  if (text_has_word_ci(text, "them")) return "them";
  if (text_has_word_ci(text, "their")) return "their";
  return NULL;
}

static void format_traits_body(char *body, size_t cap) {
  AetPcSave p;
  const char *cl;
  int any = 0;
  int npath = 0;
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  pc_capture(&p);
  cl = p.class_[0] ? p.class_ : "adventurer";
  snprintf(
      body, cap,
      "Traits\n\n"
      "%s\n\n"
      "Other builds award explicit perk lists and tracked personality axes.\n",
      banner);
  body_append(
      body, cap,
      "This screen summarizes readable tags: a little class color, a little "
      "history\n"
      "from your sheet, and a few stat tells — hooks for dialogue, not a "
      "min-max audit.\n\n");
  body_append(body, cap, "Path & presentation\n");
  if (str_contains_ci(cl, "rogue") || str_contains_ci(cl, "thief") ||
      str_contains_ci(cl, "assassin")) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Shadow habit — you think in angles, noise, and exits.\n");
  }
  if (npath < 4 &&
      (str_contains_ci(cl, "mage") || str_contains_ci(cl, "wizard") ||
       str_contains_ci(cl, "witch") || str_contains_ci(cl, "sorcer"))) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Arcane tilt — patterns and costs sit behind your choices.\n");
  }
  if (npath < 4 && (str_contains_ci(cl, "cleric") || str_contains_ci(cl, "priest") ||
                    str_contains_ci(cl, "druid"))) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Oath or grove — duty reads louder than whim.\n");
  }
  if (npath < 4 &&
      (str_contains_ci(cl, "fighter") || str_contains_ci(cl, "warrior") ||
       str_contains_ci(cl, "knight") || str_contains_ci(cl, "soldier"))) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Front-line default — you solve threats by closing distance.\n");
  }
  if (npath < 4 &&
      (str_contains_ci(cl, "ranger") || str_contains_ci(cl, "scout") ||
       str_contains_ci(cl, "hunter"))) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Tracker's patience — trail, weather, and sign matter.\n");
  }
  if (npath < 4 &&
      (str_contains_ci(cl, "bard") || str_contains_ci(cl, "minstrel"))) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Voice as toolkit — story, rumor, and leverage travel together.\n");
  }
  if (npath < 4 &&
      (str_contains_ci(cl, "monk") || str_contains_ci(cl, "pugilist"))) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Bodily discipline — breath and balance before bravado.\n");
  }
  if (npath < 4 && str_contains_ci(cl, "paladin")) {
    any = 1;
    npath++;
    body_append(body, cap,
                "  • Banner ethic — ideals are not optional flavor.\n");
  }
  if (!any)
    body_append(body, cap,
                "  • Open silhouette — class reads plain; define yourself in play.\n");
  body_append(body, cap, "\nMarks\n");
  any = 0;
  if (p.scars) {
    any = 1;
    body_append(body, cap,
                "  • Scarred — the world has already left its edits on you.\n");
  }
  if (p.tattoos) {
    any = 1;
    body_append(body, cap,
                "  • Inked — symbols, oaths, or aesthetics worn on the skin.\n");
  }
  if (!any)
    body_append(body, cap,
                "  • Unmarked skin — no tattoo or scar flags on the sheet.\n");
  body_append(body, cap, "\nTells (from attributes)\n");
  any = 0;
  if (p.cha >= 12) {
    any = 1;
    body_append(body, cap,
                "  • Presence — you steer rooms without shouting.\n");
  }
  if (p.intl >= 12) {
    any = 1;
    body_append(body, cap,
                "  • Analyst — abstractions feel like home terrain.\n");
  }
  if (p.will >= 12) {
    any = 1;
    body_append(body, cap,
                "  • Iron nerve — fear negotiates with you; it rarely wins.\n");
  }
  if (p.per >= 12) {
    any = 1;
    body_append(body, cap,
                "  • Noticer — small inconsistencies earn your attention first.\n");
  }
  if (p.str >= 12) {
    any = 1;
    body_append(body, cap,
                "  • Raw leverage — force is a vocabulary you speak fluently.\n");
  }
  if (!any)
    body_append(body, cap,
                "  • Even keel — no single stat shouts over the rest yet.\n");
  body_append(
      body, cap,
      "\nPair with  aptitudes  for synergy knacks and  character  for the full "
      "portrait.\n"
      "DLC can append signature perks or story traits below.\n");
}

static void format_momentum_body(char *body, size_t cap) {
  int v = count_visited();
  int pct =
      (WORLD_ROOM_COUNT > 0) ? (v * 100) / WORLD_ROOM_COUNT : 0;
  char banner[256];
  char purseb[32];
  pc_format_identity_banner(banner, sizeof banner);
  currency_format_compact(g_coins, purseb, sizeof purseb);
  snprintf(
      body, cap,
      "Momentum\n\n"
      "%s\n\n"
      "Other editions love level bars and XP pools — clean dopamine, discrete "
      "jumps.\n"
      "This screen is the honest substitute: turns spent, ground covered, purse "
      "and score\n"
      "as rough renown, and whether you are still standing. No silent "
      "multipliers.\n\n",
      banner);
  body_append(body, cap,
              "Raw tallies\n"
              "  Turns taken:     %d\n"
              "  Map footprint:   %d / %d locations (%d%%)\n"
              "  Score:           %d\n"
              "  Purse:           %s\n"
              "  Condition:       %d / %d\n\n",
              g_turns, v, WORLD_ROOM_COUNT, pct, g_score, purseb, g_health,
              g_max_health);
  body_append(body, cap, "Read of the moment\n");
  if (pct < 25)
    body_append(body, cap,
                "  • The atlas still feels episodic — many first visits left.\n");
  else if (pct < 60)
    body_append(body, cap,
                "  • Terrain is knitting together; return trips mean fluency, "
                "not confusion.\n");
  else
    body_append(body, cap,
                "  • You hold most of this volume in working memory — novelty "
                "yields to detail.\n");
  if (g_turns < 40)
    body_append(body, cap,
                "  • Days are crisp; choices still bite with newness.\n");
  else if (g_turns < 120)
    body_append(body, cap,
                "  • Rhythm shows up — you move with opinions, not only "
                "curiosity.\n");
  else
    body_append(body, cap,
                "  • Long mileage — habits, grudges, and shortcuts are tangible.\n");
  if (g_max_health > 0 && g_health * 2 < g_max_health)
    body_append(body, cap,
                "  • You are carrying hurt; patience is a tactical virtue.\n");
  else if (g_max_health > 0 && g_health >= g_max_health)
    body_append(body, cap,
                "  • Whole condition — you are not nursing a crisis.\n");
  body_append(
      body, cap,
      "\nPair with  progress  (checklist),  journal  (templates),  objectives.\n"
      "Mods can append chapter titles, act breaks, or episode recaps below.\n");
}

static void format_perks_body(char *body, size_t cap) {
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(
      body, cap,
      "Perks\n\n"
      "%s\n\n"
      "Some builds hand out named feats with hard math — relationship bonuses, "
      "cooldown\n",
      banner);
  body_append(
      body, cap,
      "tricks, crafting rebates. The stdin port does not smuggle rules inside "
      "hidden\n"
      "modifiers; when engine-backed perks exist, they will print as explicit "
      "lines here.\n\n"
      "Until then\n"
      "  •  aptitudes  — passive knacks from your stat shape\n"
      "  •  traits  — narrative tags from class and presentation\n"
      "  •  notes  — track handshake deals, vows, and player-remembered feats\n\n"
      "DLC: use the block below as a roster (one named perk per line reads "
      "cleanly).\n");
}

static void format_voice_body(char *body, size_t cap) {
  AetPcSave p;
  const char *g;
  const char *subj, *obj, *pos;
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  pc_capture(&p);
  g = p.gender[0] ? p.gender : "they";
  if (str_ieq(g, "he")) {
    subj = "he";
    obj = "him";
    pos = "his";
  } else if (str_ieq(g, "she")) {
    subj = "she";
    obj = "her";
    pos = "her";
  } else {
    subj = "they";
    obj = "them";
    pos = "their";
  }
  snprintf(
      body, cap,
      "Voice & pronouns\n\n"
      "%s\n\n"
      "Full clients often derive a full pronoun bundle from a gender token and "
      "weave it through\n",
      banner);
  body_append(
      body, cap,
      "every paragraph. This port keeps the same token on disk (he / she / they) "
      "and prints\n"
      "a compact cheat-sheet for writers, translators, and DLC authors — no "
      "surprise grammar.\n\n");
  body_append(body, cap, "Identity\n"
                          "  Name:   %s\n"
                          "  Gender: %s  (feeds narration defaults)\n\n",
              p.name[0] ? p.name : "(unnamed)", g);
  body_append(body, cap,
                "Third-person pronouns\n"
                "  Subject:    %s\n"
                "  Object:     %s\n"
                "  Possessive: %s\n\n",
                subj, obj, pos);
  if (str_ieq(subj, "they"))
    body_append(body, cap,
                "Agreement (typical)\n"
                "  • Use plural verbs: they are / they have / they carry.\n\n");
  else
    body_append(body, cap,
                "Agreement (typical)\n"
                "  • Use singular verbs: %s is / %s has / %s carries.\n\n",
                subj, subj, subj);
  body_append(body, cap,
                "Saved voice profile (from creation / save)\n"
                "  Pitch:   %s\n"
                "  Quality: %s\n\n",
                p.voice_pitch[0] ? p.voice_pitch : "—",
                p.voice_quality[0] ? p.voice_quality : "—");
  body_append(body, cap, "Read at a glance\n");
  if (str_contains_ci(p.voice_quality, "rasp") ||
      str_contains_ci(p.voice_quality, "gruff"))
    body_append(body, cap,
                "  • Registers weathered — edges on consonants, smoke or mileage.\n");
  else if (str_contains_ci(p.voice_quality, "breath"))
    body_append(body, cap,
                "  • Soft attack — words lean in before they land.\n");
  else if (str_contains_ci(p.voice_quality, "melod"))
    body_append(body, cap,
                "  • Carries tune — easy to imagine singing even when speaking.\n");
  else
    body_append(body, cap,
                "  • Neutral delivery — room for the scene to color it.\n");
  if (str_contains_ci(p.voice_pitch, "bass") ||
      str_contains_ci(p.voice_pitch, "baritone"))
    body_append(body, cap,
                "  • Weight sits low; quiet lines still occupy space.\n");
  else if (str_contains_ci(p.voice_pitch, "sopr") ||
           str_contains_ci(p.voice_pitch, "alto"))
    body_append(body, cap,
                "  • Brighter register; tension cuts through crowd noise.\n");
  body_append(
      body, cap,
      "\nThe full portrait weaves voice into prose — try  character  or  "
      "sheet brief.\n"
      "Mods can append barks, catchphrases, or language notes below.\n");
}

static void format_bio_body(char *body, size_t cap) {
  AetPcSave p;
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  pc_capture(&p);
  snprintf(
      body, cap,
      "Biography\n\n"
      "%s\n\n"
      "Some editions auto-spin a novel-length past from sliders. Here, biography "
      "is honest:\n",
      banner);
  body_append(
      body, cap,
      "the engine remembers what you declared at creation and what you log in "
      "play; the\n"
      "rest is yours to author — or to ship as DLC text — without rewriting "
      "saves.\n\n");
  body_append(body, cap,
                "Anchors on file\n"
                "  Name:  %s\n"
                "  Class: %s   Race: %s\n"
                "  Age:   %s\n\n",
                p.name[0] ? p.name : "(unnamed)",
                p.class_[0] ? p.class_ : "adventurer",
                p.race[0] ? p.race : "Human", p.age[0] ? p.age : "—");
  body_append(
      body, cap,
      "Player tools\n"
      "  •  notes  — pin facts, debts, and relationships as you invent them\n"
      "  •  objectives  /  journal  — keep the present-tense plot legible\n"
      "  •  reputation  — standing fiction;  traits  — tags from class and "
      "marks\n\n"
      "Drop your canon, timeline, or secrets in the mod section below — last "
      "pack wins.\n");
}

static void format_tainting_body(char *body, size_t cap) {
  AetPcSave p;
  int c;
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  pc_capture(&p);
  c = p.cor;
  snprintf(
      body, cap,
      "Tainting (COR)\n\n"
      "%s\n\n"
      "In heavier builds, corruption can climb from many systems and paint long "
      "paragraphs in real time.\n",
      banner);
  body_append(
      body, cap,
      "This C port keeps the same numeric COR on your save and the same portrait "
      "breakpoints,\n"
      "but it does not simulate hidden drip-feed sources yet — honesty beats a "
      "fake meter.\n\n");
  body_append(body, cap, "Value on file\n  COR = %d\n\n", c);
  body_append(body, cap, "Tier (matches portrait thresholds in this build)\n");
  if (c <= 0)
    body_append(body, cap,
                "  • Clean — the portrait skips corruption prose entirely.\n");
  else if (c < 10)
    body_append(body, cap,
                "  • Drift — the sheet leans otherworldly; optics in play stay "
                "mostly mundane.\n");
  else if (c < 30)
    body_append(body, cap,
                "  • Friction — warmth, glow, and pull (tier shared with full "
                "portrait text).\n");
  else if (c < 60)
    body_append(body, cap,
                "  • Warp — anatomy negotiates with something that is not "
                "quite your birth form.\n");
  else
    body_append(body, cap,
                "  • Severe — mythic taint fully voiced in the portrait layer.\n");
  body_append(
      body, cap,
      "\nWhere to read the sensory version\n"
      "  character  or  sheet  — long-form corruption is woven there when COR "
      "> 0.\n\n"
      "Design note for DLC: raise COR through explicit story beats in your text "
      "packs,\n"
      "or document a target number in manifest notes until scripted sources "
      "land.\n"
      "Mods can append cult ranks, resistances, or relapse hooks below.\n");
}

static void rel_hist_append_delta(char *body, size_t cap, int df, int dr, int dp) {
  int any = 0;
  if (!body || cap < 8) return;
  if (df) {
    body_append(body, cap, "%sfriendship %+d", any ? "  " : "", df);
    any = 1;
  }
  if (dr) {
    body_append(body, cap, "%sromance %+d", any ? "  " : "", dr);
    any = 1;
  }
  if (dp) {
    body_append(body, cap, "%spatron %+d", any ? "  " : "", dp);
    any = 1;
  }
  if (!any) body_append(body, cap, "(no meter change)");
}

static void format_relationship_history_body(char *body, size_t cap,
                                             const char *filter_slug) {
  int i, start, shown = 0;
  char filt[MAX_ITEM_LEN];
  if (!body || cap < 128) return;
  filt[0] = '\0';
  if (filter_slug && filter_slug[0]) {
    size_t n = strnlen(filter_slug, sizeof filt - 1);
    memcpy(filt, filter_slug, n);
    filt[n] = '\0';
    strip_leading_articles(filt);
    strip_trailing_space(filt);
  }
  snprintf(body, cap,
           "Relationship history\n\n"
           "Recent social meter changes (newest last; max %d rows).\n"
           "%s%s%s\n\n",
           REL_HIST_RING, filt[0] ? "Filter: " : "", filt[0] ? filt : "",
           filt[0] ? "\n" : "");
  if (g_rel_hist_count == 0) {
    body_append(body, cap,
                "  (none yet — talk, gift, or trade with someone first.)\n");
    return;
  }
  start = (g_rel_hist_head - g_rel_hist_count + REL_HIST_RING) % REL_HIST_RING;
  for (i = 0; i < g_rel_hist_count && strlen(body) + 96 < cap; i++) {
    int idx = (start + i) % REL_HIST_RING;
    const RelHistRow *row = &g_rel_hist[idx];
    char pretty[96];
    char deltas[80];
    if (filt[0] && !str_ieq(row->slug, filt) &&
        !str_contains_ci(row->slug, filt))
      continue;
    entity_pretty(row->slug, pretty, sizeof pretty);
    deltas[0] = '\0';
    rel_hist_append_delta(deltas, sizeof deltas, row->df, row->dr, row->dp);
    body_append(body, cap, "  T%04d  %-22s  %-12s  %s\n", row->turn, pretty,
                row->act, deltas);
    shown++;
  }
  if (!shown)
    body_append(body, cap, "  (no rows match that name.)\n");
  else
    body_append(body, cap,
                "\nSee  rapport  for current bars;  causality social  for raw "
                "event tags.\n");
}

static const char *rapport_decay_hint(int last_turn, unsigned friendship) {
  int gap;
  if (friendship < 10u || last_turn <= 0) return NULL;
  gap = g_turns - last_turn;
  if (gap >= 50) return "stale — friendship may decay on idle turns";
  if (gap >= 40) return "quiet — check in soon or rapport fades";
  return NULL;
}

static void format_rapport_body(char *body, size_t cap) {
  char ents[96][48];
  int first_room[96];
  int n = 0, r, i, j;
  const char *here;
  char role[96];
  char pr[64];
  AetPcSave me;
  for (r = 0; r < WORLD_ROOM_COUNT; r++) {
    const char *e = world_room_entity(r);
    int known = 0;
    if (!e || !e[0]) continue;
    for (i = 0; i < n; i++) {
      if (str_ieq(ents[i], e)) {
        known = 1;
        break;
      }
    }
    if (known) continue;
    if (n >= 96) break;
    strncpy(ents[n], e, 47);
    ents[n][47] = '\0';
    first_room[n] = r;
    n++;
  }
  for (i = 0; i < n - 1; i++) {
    for (j = i + 1; j < n; j++) {
      if (strcmp(ents[i], ents[j]) > 0) {
        char tmp[48];
        int tr;
        memcpy(tmp, ents[i], sizeof tmp);
        memcpy(ents[i], ents[j], sizeof tmp);
        memcpy(ents[j], tmp, sizeof tmp);
        tr = first_room[i];
        first_room[i] = first_room[j];
        first_room[j] = tr;
      }
    }
  }
  pc_capture(&me);
  pc_fill_narrative_defaults(&me);
  pc_format_role_phrase(role, sizeof role);
  pc_format_pronouns_short(me.gender[0] ? me.gender : "they", pr, sizeof pr);
  snprintf(
      body, cap,
      "Rapport\n\n"
      "%s — %s · %s\n\n"
      "Trading-circle NPCs (same roster as shop patrons) carry friendship and "
      "romance scores,\n"
      "folded into a relationship stage together with patron standing. Talk "
      "topics, gifts,\n"
      "and banter increase those meters; values persist in your save (SOC "
      "block).\n"
      "Other room NPCs (not on the merchant roster) use the SOC2 extension: "
      "same bars,\n"
      "patron rep omitted — rows keyed by entity slug.\n"
      "Friendship 10+ decays 1 point every 20 turns after 50 turns without "
      "contact.\n\n",
      pc_display_name(), role, pr);
  body_append(body, cap,
                "Anchored cast (from room.entity — sorted by id)\n\n");
  if (n <= 0) {
    body_append(body, cap, "  (No NPC entities linked in this world slice.)\n");
  } else {
    for (i = 0; i < n && strlen(body) + 120 < cap; i++) {
      char pretty[112];
      entity_pretty(ents[i], pretty, sizeof pretty);
      body_append(body, cap, "  • %-20s  [%s]\n    usual ground: %s\n",
                  pretty, ents[i], resolve_world_title(first_room[i]));
    }
  }
  here = world_room_entity(g_room);
  if (here && here[0]) {
    char pretty[112];
    entity_pretty(here, pretty, sizeof pretty);
    body_append(body, cap,
                "\nRight now\n"
                "  %s is tied to this room — try  who ,  talk ,  talk about …\n",
                pretty);
  }
  body_append(body, cap,
              "\nTrade-circle ties (merchant roster)\n"
              "Stages derive from friendship · romance · patron rep together "
              "(0–100 bars).\n\n");
  {
    int mc = aet_merchant_count();
    int si;
    if (mc > AETER_REP_MAX) mc = AETER_REP_MAX;
    for (si = 0; si < mc && strlen(body) + 160 < cap; si++) {
      const char *slug = aet_merchant_slug_at(si);
      char pretty_m[112];
      unsigned char st;
      int rep_s;
      if (!slug || !slug[0]) continue;
      entity_pretty(slug, pretty_m, sizeof pretty_m);
      st = soc_derive_stage(si);
      rep_s = merchant_rep_score(si);
      {
        char fbar[12], rbar[12], pbar[12];
        int pp = rep_s > 220 ? 100 : (rep_s * 100) / 220;
        meter_bar_pct(fbar, sizeof fbar, (int)g_npc_friendship[si]);
        meter_bar_pct(rbar, sizeof rbar, (int)g_npc_romance[si]);
        meter_bar_pct(pbar, sizeof pbar, pp);
        body_append(body, cap,
                    "  • %-22s\n"
                    "    stage %-18s\n"
                    "    friendship %3u [%s]   romance %3u [%s]\n"
                    "    patron %3d (%s) [%s]\n",
                    pretty_m, soc_stage_name(st),
                    (unsigned)g_npc_friendship[si], fbar,
                    (unsigned)g_npc_romance[si], rbar, rep_s,
                    merchant_rep_tier_label(rep_s), pbar);
      }
      if (g_npc_last_social_turn[si] > 0)
        body_append(body, cap, "    last social interaction: turn %d\n",
                    g_npc_last_social_turn[si]);
      {
        const char *dh = rapport_decay_hint(g_npc_last_social_turn[si],
                                            g_npc_friendship[si]);
        if (dh) body_append(body, cap, "    %s\n", dh);
      }
    }
  }
  body_append(body, cap,
              "\nOther NPC ties (non-merchant room.entity slugs, SOC2)\n"
              "Stages use friendship and romance only (no patron discount "
              "track).\n\n");
  {
    int si, any = 0;
    for (si = 0; si < AETER_SOC_NPC_MAX; si++) {
      char pretty_o[112];
      unsigned char st;
      if (!g_soc_npc_slug[si][0]) continue;
      any = 1;
      if (strlen(body) + 180 >= cap) break;
      entity_pretty(g_soc_npc_slug[si], pretty_o, sizeof pretty_o);
      st = soc_derive_stage_npc_slug(g_soc_npc_slug[si]);
      {
        char fbar[12], rbar[12];
        meter_bar_pct(fbar, sizeof fbar, (int)g_soc_npc_friendship[si]);
        meter_bar_pct(rbar, sizeof rbar, (int)g_soc_npc_romance[si]);
        body_append(body, cap,
                    "  • %-22s\n"
                    "    stage %-18s\n"
                    "    friendship %3u [%s]   romance %3u [%s]\n",
                    pretty_o, soc_stage_name(st),
                    (unsigned)g_soc_npc_friendship[si], fbar,
                    (unsigned)g_soc_npc_romance[si], rbar);
      }
      if (g_soc_npc_last_turn[si] > 0)
        body_append(body, cap, "    last social interaction: turn %d\n",
                    g_soc_npc_last_turn[si]);
      {
        const char *dh = rapport_decay_hint(g_soc_npc_last_turn[si],
                                            g_soc_npc_friendship[si]);
        if (dh) body_append(body, cap, "    %s\n", dh);
      }
    }
    if (!any)
      body_append(body, cap,
                  "  (None tracked yet — talk while they are present.)\n");
  }
  body_append(
      body, cap,
      "\nPlayer memory\n"
      "  •  notes  — free-form beats the meters miss\n"
      "  •  reputation  — faction blurbs;  bio  — backstory hooks\n\n"
      "Mods can append jealousy arcs, rivalry hooks, or episode ladders below.\n");
}

static void format_vitals_body(char *body, size_t cap) {
  AetPcSave p;
  int pct;
  char role[96];
  char pr[64];
  pc_capture(&p);
  pc_fill_narrative_defaults(&p);
  pc_format_role_phrase(role, sizeof role);
  pc_format_pronouns_short(p.gender[0] ? p.gender : "they", pr, sizeof pr);
  pct = (g_max_health > 0) ? (g_health * 100) / g_max_health : 100;
  if (pct > 100) pct = 100;
  if (pct < 0) pct = 0;
  snprintf(
      body, cap,
      "Vitals\n\n"
      "%s — %s · %s\n\n"
      "Graphical builds love stacked bars — health, mana, stamina, arousal, "
      "hunger — each\n"
      "with its own drip logic. This port keeps the save file honest: health "
      "is the only\n"
      "vital persisted today. Everything else is narration, items, or future "
      "work — no\n"
      "phantom meters ticking down in the background.\n\n",
      pc_display_name(), role, pr);
  body_append(body, cap,
              "Condition\n"
              "  Hit points:  %d / %d  (~%d%% of your current ceiling)\n"
              "  Turns:       %d\n\n",
              g_health, g_max_health, pct, g_turns);
  body_append(body, cap, "Read of the body\n");
  if (pct >= 90)
    body_append(body, cap,
                "  • Whole — you are not limping on empty.\n");
  else if (pct >= 60)
    body_append(body, cap,
                "  • Battered but operational — risk adds spice, not panic.\n");
  else if (pct >= 35)
    body_append(body, cap,
                "  • Meaningfully hurt — clever play beats heroics for a bit.\n");
  else
    body_append(body, cap,
                "  • Critical band — treat recovery as the main quest.\n");
  if (p.cor > 0)
    body_append(body, cap,
                "  • COR is non-zero —  tainting  for corruption tiers; portrait "
                "for sensory detail.\n");
  body_append(
      body, cap,
      "\nRecovery tools in this build\n"
      "  eat / drink / rest / wait  — when the world gives you staples\n"
      "  status  — purse, inventory pressure, room context in one sheet\n\n"
      "When mana, stamina, or sleep debt become real saves, they will surface "
      "here first.\n"
      "Mods can append wound clocks, poison tags, or clinician notes below.\n");
}

static unsigned inventory_item_bulk(const char *slug) {
  const AetItemCatalogEntry *cat;
  unsigned est;
  if (!slug || !slug[0]) return 1u;
  cat = aet_item_catalog_by_slug(slug);
  if (cat && cat->wgt > 0) return (unsigned)cat->wgt;
  est = item_est_heft(slug);
  if (est <= 8u) return 1u;
  if (est <= 16u) return 2u;
  if (est <= 28u) return 3u;
  if (est <= 40u) return 4u;
  if (est <= 52u) return 5u;
  return 6u;
}

static unsigned inventory_total_bulk(void) {
  int i;
  unsigned total = 0;
  for (i = 0; i < g_inv_n; i++) total += inventory_item_bulk(g_inv[i]);
  return total;
}

static int slug_compare_ci(const char *a, const char *b) {
  if (!a) a = "";
  if (!b) b = "";
  while (*a && *b) {
    int ca = tolower((unsigned char)*a);
    int cb = tolower((unsigned char)*b);
    if (ca != cb) return ca - cb;
    a++;
    b++;
  }
  if (*a) return 1;
  if (*b) return -1;
  return 0;
}

static int inv_category_rank(const char *slug) {
  const AetItemCatalogEntry *e;
  const char *const *p;
  if (!slug || !slug[0]) return 9;
  e = aet_item_catalog_by_slug(slug);
  for (p = world_consume_food_ids(); p && *p; p++)
    if (str_ieq(slug, *p)) return 3;
  for (p = world_consume_drink_ids(); p && *p; p++)
    if (str_ieq(slug, *p)) return 4;
  if (e) {
    if (e->slot_index == 6 || e->atk > 0) return 0;
    if (e->def > 0 || (e->slot_index >= 0 && e->slot_index <= 5)) return 1;
    if (e->wgt >= 8) return 2;
  }
  if (strstr(slug, "key") || strstr(slug, "lockpick") || strstr(slug, "pick"))
    return 5;
  if (strstr(slug, "book") || strstr(slug, "tome") || strstr(slug, "scroll") ||
      strstr(slug, "ledger") || strstr(slug, "leaflet") ||
      strstr(slug, "textbook") || strstr(slug, "guide") ||
      strstr(slug, "manual") || strstr(slug, "primer") ||
      strstr(slug, "journal") || strstr(slug, "logbook") ||
      strstr(slug, "sheet") || strstr(slug, "missive"))
    return 7;
  return 6;
}

static const char *inv_category_label(int rank) {
  switch (rank) {
    case 0:
      return "weapon";
    case 1:
      return "armor";
    case 2:
      return "material";
    case 3:
      return "food";
    case 4:
      return "drink";
    case 5:
      return "tool";
    case 7:
      return "literature";
    default:
      return "misc";
  }
}

typedef struct {
  int idx;
} InvSortEntry;

static int inv_sort_name_cmp(const void *a, const void *b) {
  const InvSortEntry *ia = a;
  const InvSortEntry *ib = b;
  return slug_compare_ci(g_inv[ia->idx], g_inv[ib->idx]);
}

static int inv_sort_weight_cmp(const void *a, const void *b) {
  const InvSortEntry *ia = a;
  const InvSortEntry *ib = b;
  unsigned ba = inventory_item_bulk(g_inv[ia->idx]);
  unsigned bb = inventory_item_bulk(g_inv[ib->idx]);
  if (ba != bb) return (bb > ba) - (bb < ba);
  return slug_compare_ci(g_inv[ia->idx], g_inv[ib->idx]);
}

static int inv_sort_type_cmp(const void *a, const void *b) {
  const InvSortEntry *ia = a;
  const InvSortEntry *ib = b;
  int ca = inv_category_rank(g_inv[ia->idx]);
  int cb = inv_category_rank(g_inv[ib->idx]);
  if (ca != cb) return ca - cb;
  return slug_compare_ci(g_inv[ia->idx], g_inv[ib->idx]);
}

static void format_inventory_list_body(char *body, size_t cap, int sort_mode,
                                       const char *filter) {
  InvSortEntry order[MAX_INV];
  int i, shown = 0;
  char banner[256];
  const char *sort_label = "name";
  if (!body || cap < 64) return;
  if (sort_mode == 1) sort_label = "weight (heavy first)";
  else if (sort_mode == 2) sort_label = "category";
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(body, cap,
           "Pack list\n\n"
           "%s\n\n"
           "Items: %d / %d   Sort: %s%s%s\n\n",
           banner, g_inv_n, MAX_INV, sort_label,
           filter && filter[0] ? "   Filter: \"" : "",
           filter && filter[0] ? filter : "");
  if (filter && filter[0] && strlen(body) + strlen(filter) + 4 < cap)
    strncat(body, "\"\n\n", cap - strlen(body) - 1);
  else if (!filter || !filter[0])
    body_append(body, cap, "\n");
  if (g_inv_n == 0) {
    body_append(body, cap, "  (empty pack)\n");
    return;
  }
  for (i = 0; i < g_inv_n; i++) order[i].idx = i;
  if (sort_mode == 1)
    qsort(order, (size_t)g_inv_n, sizeof order[0], inv_sort_weight_cmp);
  else if (sort_mode == 2)
    qsort(order, (size_t)g_inv_n, sizeof order[0], inv_sort_type_cmp);
  else
    qsort(order, (size_t)g_inv_n, sizeof order[0], inv_sort_name_cmp);
  for (i = 0; i < g_inv_n && strlen(body) + 96 < cap; i++) {
    const char *slug = g_inv[order[i].idx];
    char pretty[96];
    unsigned bulk;
    int cat;
    if (filter && filter[0] && !str_contains_ci(slug, filter)) {
      const char *lbl = aet_item_catalog_label_for_slug(slug);
      if (!lbl || !str_contains_ci(lbl, filter)) continue;
    }
    item_pretty(slug, pretty, sizeof pretty);
    bulk = inventory_item_bulk(slug);
    cat = inv_category_rank(slug);
    body_append(body, cap, "  %3d. %-28s [%s]  %s  bulk %u",
                shown + 1, pretty, slug, inv_category_label(cat), bulk);
    if (item_has_read_text(slug))
      body_append(body, cap, "  · readable");
    body_append(body, cap, "\n");
    shown++;
  }
  if (shown == 0)
    body_append(body, cap, "  (no items match that filter)\n");
  else
    body_append(body, cap,
                "\nListed: %d item%s · pack bulk ~%u (heuristic)\n"
                "Readable literature:  read <item>  opens the in-game reader.\n"
                "Equipment UI shows the first 12 rows; use catalog [id] there.\n"
                "Try  inventory sort weight  |  inventory find <text>\n",
                shown, shown == 1 ? "" : "s", inventory_total_bulk());
}

static void util_append_row(char *body, size_t cap, const char *name,
                            const char *state, const char *actions) {
  body_append(body, cap, "  • %-20s\n    state: %s\n    try: %s\n\n", name, state,
              actions);
}

static void format_utilities_body(char *body, size_t cap) {
  const char *slug = world_slug(g_room);
  const char *blurb = resolve_world_blurb(g_room);
  int any = 0;
  int i;
  char banner[256];
  if (!body || cap < 128) return;
  pc_format_identity_banner(banner, sizeof banner);
  snprintf(body, cap,
           "Room utilities\n\n"
           "%s\n\n"
           "Location: %s  [%s]\n\n"
           "Fixtures and interactables the parser knows about here (not every "
           "prop in the blurb).\n\n",
           banner, resolve_world_title(g_room), slug);
  if (blurb && strstr(blurb, "fireplace")) {
    const char *st =
        (strstr(blurb, "cold") || strstr(blurb, "dark") || strstr(blurb, "unlit"))
            ? "cold hearth, unlit"
            : (strstr(blurb, "warm") || strstr(blurb, "embers") ||
               strstr(blurb, "glow"))
                  ? "warm embers"
                  : "hearth present";
    util_append_row(body, cap, "Fireplace", st,
                    "examine fireplace; search mantel; listen");
    any = 1;
  }
  if (blurb && strstr(blurb, "stove")) {
    util_append_row(body, cap, "Kitchen stove",
                    strstr(blurb, "smoke") ? "cool, old smoke smell"
                                           : "cast-iron, idle",
                    "examine stove; search cupboards");
    any = 1;
  }
  if (blurb && strstr(blurb, "fountain")) {
    util_append_row(body, cap, "Fountain", "water running", "examine fountain; listen");
    any = 1;
  }
  if (strstr(slug, "well") || (blurb && strstr(blurb, "well"))) {
    util_append_row(body, cap, "Well",
                    room_floor_has_id("bucket") ? "bucket on rim"
                                                : "stone rim, rope wear",
                    "examine well; search; take bucket (if present)");
    any = 1;
  }
  if (strstr(slug, "blacksmith") || (blurb && strstr(blurb, "forge"))) {
    util_append_row(body, cap, "Forge",
                    (blurb && strstr(blurb, "hot")) ? "working heat"
                                                    : "forge bench ready",
                    "talk; wares; forge");
    any = 1;
  }
  if (strstr(slug, "waystone") || strstr(slug, "nexus")) {
    util_append_row(body, cap, "Travel anchor", "attuned waypoint",
                    "waypoints; waypoint <name>; route <place>");
    any = 1;
  }
  if (strstr(slug, "ferry") || (blurb && strstr(blurb, "ferry"))) {
    util_append_row(body, cap, "Ferry", "dock service", "examine dock; talk; wait");
    any = 1;
  }
  if (strstr(slug, "stage") || (blurb && strstr(blurb, "stage"))) {
    util_append_row(body, cap, "Performance stage", "acoustics tuned",
                    "examine stage; listen; talk");
    any = 1;
  }
  if (room_has_visible_item(g_room, "tavern_piano")) {
    util_append_row(body, cap, "Tavern piano", "upright, playable",
                    "examine piano; play piano; listen; gamble; fish");
    any = 1;
  }
  for (i = 0; i < g_room_item_n[g_room]; i++) {
    const char *id = g_room_items[g_room][i];
    if (!id || !id[0]) continue;
    if (str_ieq(id, "bucket")) {
      util_append_row(body, cap, "Bucket (floor)", "portable", "take bucket; examine bucket");
      any = 1;
    } else if (str_ieq(id, "lantern") || str_ieq(id, "brass_lantern") ||
               str_ieq(id, "torch") || str_ieq(id, "candle")) {
      char pretty[64];
      item_pretty(id, pretty, sizeof pretty);
      util_append_row(body, cap, pretty, "portable light", "take; ready; examine");
      any = 1;
    }
  }
  if (!any)
    body_append(body, cap,
                "  (No utility fixtures tagged for this room — try  look ,  "
                "search ,  examine <thing>.\n"
                "  Mentioned scenery may still respond to plain examine.)\n");
  else
    body_append(body, cap,
                "Tip:  look  for room prose;  utilities  refreshes this panel.\n");
}

static int inventory_heaviest_items(char out[][MAX_ITEM_LEN], unsigned out_bulk[],
                                    int max_out) {
  int i, j, n = 0;
  if (!out || !out_bulk || max_out <= 0) return 0;
  for (i = 0; i < g_inv_n; i++) {
    unsigned bulk = inventory_item_bulk(g_inv[i]);
    int pos = n;
    for (j = 0; j < n; j++) {
      if (bulk > out_bulk[j]) {
        pos = j;
        break;
      }
    }
    if (pos >= max_out) continue;
    if (n < max_out) n++;
    for (j = n - 1; j > pos; j--) {
      out_bulk[j] = out_bulk[j - 1];
      memcpy(out[j], out[j - 1], MAX_ITEM_LEN);
    }
    out_bulk[pos] = bulk;
    strncpy(out[pos], g_inv[i], MAX_ITEM_LEN - 1);
    out[pos][MAX_ITEM_LEN - 1] = '\0';
  }
  return n;
}

static void format_contextual_hints_body(char *body, size_t cap) {
  AetPcSave p;
  const char *ent;
  const char *slug;
  const char *hsfx;
  size_t len0;
  unsigned pack_bulk;

  if (!body || cap < 256) return;
  if (!g_hints_pref) {
    snprintf(body, cap,
             "Context hints are disabled in Settings (pause menu → option 8).\n"
             "Turn them back on to see situational nudges.\n");
    return;
  }
  pc_capture(&p);
  ent = world_room_entity(g_room);
  slug = world_slug(g_room);

  snprintf(body, cap, "Context hints\n\n");
  {
    char role[96], pr[64];
    pc_format_role_phrase(role, sizeof role);
    pc_format_pronouns_short(p.gender[0] ? p.gender : "they", pr, sizeof pr);
    body_append(
        body, cap,
        "%s — %s · %s\n\n"
        "Situational nudges — not a walkthrough. When several apply, skim top "
        "to bottom.\n\n",
        pc_display_name(), role, pr);
  }
  len0 = strlen(body);
  pack_bulk = inventory_total_bulk();

  if (room_too_dark_to_see())
    body_append(body, cap,
                "• Lighting: too dark to see. Ready a torch, lantern, or "
                "another light.\n");

  if (!room_too_dark_to_see() && world_room_is_dark(g_room))
    body_append(body, cap,
                "• This region reads as dim — keep a light handy so text stays "
                "readable.\n");

  if (g_hidden_n[g_room] > 0)
    body_append(body, cap,
                "• Something may be hidden — search (your hands still work in "
                "the dark).\n");

  if (g_max_health > 0 && g_health > 0 && g_health * 4 < g_max_health)
    body_append(body, cap,
                "• Health is critically low — eat, drink, rest, or bail to "
                "safer ground.\n");

  if (g_inv_n >= MAX_INV)
    body_append(body, cap,
                "• Inventory full — drop, sell, or use items before new loot "
                "sticks.\n");

  if ((g_inv_n >= 6 && pack_bulk >= 10u) || pack_bulk >= 12u) {
    char heavy[3][MAX_ITEM_LEN];
    unsigned heavy_bulk[3] = {0u, 0u, 0u};
    int n_heavy, i;
    body_append(body, cap,
                g_inv_n >= MAX_INV
                    ? "• Pack weight is high too — est. bulk %u across %d item(s). "
                      "Use loot weight before taking more, then drop, sell, or use "
                      "the worst anchors first.\n"
                    : "• Pack pressure is building — est. bulk %u across %d item(s). "
                      "Use loot weight before scooping more, compare marginal items, "
                      "and drop or sell the worst anchors first.\n",
                pack_bulk, g_inv_n);
    n_heavy = inventory_heaviest_items(heavy, heavy_bulk, 3);
    if (n_heavy > 0) {
      body_append(body, cap, "  Heaviest right now: ");
      for (i = 0; i < n_heavy; i++) {
        char pretty[96];
        if (i) body_append(body, cap, i + 1 < n_heavy ? ", " : " and ");
        item_pretty(heavy[i], pretty, sizeof pretty);
        body_append(body, cap, "%s (~%u)", pretty, heavy_bulk[i]);
      }
      body_append(body, cap, ".\n");
    }
  }

  if (ent && ent[0]) {
    char pretty[96];
    entity_pretty(ent, pretty, sizeof pretty);
    body_append(body, cap,
                "• %s is here — talk, talk about <topic>; traders use wares, "
                "buy, sell.\n",
                pretty);
  }

  if (slug && slug[0] && !g_front_unlocked &&
      (!strcmp(slug, "west_of_house") || !strcmp(slug, "front_door")))
    body_append(body, cap,
                "• The boarded house: check the mailbox and door — use the "
                "house key when you have it.\n");

  if (strlen(body) == len0)
    body_append(body, cap,
                "• Nothing screams emergency — try exits, nearby, describe, "
                "route <place>, talk.\n");

  if (p.intl >= 15 || p.wis >= 15)
    body_append(body, cap,
                "\n(High INT/WIS: lean on dialogue, search, and routing — not "
                "random wandering.)\n");

  body_append(body, cap,
              "\n---\nStill failing commands?  errors  — recent issues + engine "
              "snapshot.\n");

  hsfx = aet_mods_character_hints_suffix();
  if (hsfx && hsfx[0]) {
    char xhs[4096];
    expand_mod_overlay_flat(hsfx, xhs, sizeof xhs);
    body_append(body, cap, "\n%s", xhs);
  }
}

static void format_notes_filtered_body(char *body, size_t cap,
                                       const char *title,
                                       int filter_mode,
                                       const char *find_text) {
  int i;
  size_t len;
  int shown = 0;
  char banner[256];
  const char *nsfx = aet_mods_character_notes_suffix();
  pc_format_identity_banner(banner, sizeof banner);
  if (g_note_n <= 0) {
    snprintf(body, cap,
             "%s\n\n"
             "(No notes yet. Try: note <text>  or  notes add <text>  —  also  jot "
             "<text>, memo …)\n",
             banner);
    if (nsfx && nsfx[0]) {
      char xn2[4096];
      expand_mod_overlay_flat(nsfx, xn2, sizeof xn2);
      body_append(body, cap, "\n%s", xn2);
    }
    return;
  }
  snprintf(body, cap, "%s\n\n%s:\n\n", banner,
           title ? title : "Your notes");
  len = strlen(body);
  for (i = 0; i < g_note_n && len + 8 < cap; i++) {
    int done = note_is_done(g_notes[i]);
    const char *text = note_text_only(g_notes[i]);
    int w;
    if (filter_mode == 1 && done) continue;
    if (filter_mode == 2 && !done) continue;
    if (find_text && find_text[0] && !str_contains_ci(text, find_text))
      continue;
    w = snprintf(body + len, cap - len, "  %d. [%c] %s\n", i + 1,
                 done ? 'x' : ' ', text);
    if (w < 0 || (size_t)w >= cap - len) break;
    len += (size_t)w;
    shown++;
  }
  if (!shown) {
    snprintf(body + len, cap > len ? cap - len : 0, "  (No matching notes.)\n");
  }
  if (nsfx && nsfx[0]) {
    char xn2[4096];
    expand_mod_overlay_flat(nsfx, xn2, sizeof xn2);
    body_append(body, cap, "\n%s", xn2);
  }
}

static void format_notes_body(char *body, size_t cap) {
  char title[80];
  snprintf(title, sizeof title, "Your notes (%d)", g_note_n);
  format_notes_filtered_body(body, cap, title, 0, NULL);
}

static int notes_add_line(const char *text) {
  size_t L;
  if (g_note_n >= MAX_NOTES) return 0;
  if (!text) return 0;
  while (*text == ' ') text++;
  if (!strncmp(text, "[x] ", 4) || !strncmp(text, "[ ] ", 4)) text += 4;
  L = strlen(text);
  if (L + 4 >= NOTE_LEN) L = NOTE_LEN - 5;
  memcpy(g_notes[g_note_n], "[ ] ", 4);
  memcpy(g_notes[g_note_n] + 4, text, L);
  g_notes[g_note_n][L + 4] = '\0';
  g_note_n++;
  return 1;
}

static int notes_delete_line(int one_based) {
  int i, k;
  if (one_based < 1 || one_based > g_note_n) return 0;
  k = one_based - 1;
  for (i = k; i < g_note_n - 1; i++)
    memcpy(g_notes[i], g_notes[i + 1], NOTE_LEN);
  g_note_n--;
  g_notes[g_note_n][0] = '\0';
  return 1;
}

static int notes_set_done_line(int one_based, int done) {
  char tmp[NOTE_LEN];
  const char *text;
  size_t L;
  if (one_based < 1 || one_based > g_note_n) return 0;
  text = note_text_only(g_notes[one_based - 1]);
  L = strlen(text);
  if (L + 4 >= NOTE_LEN) L = NOTE_LEN - 5;
  memcpy(tmp, done ? "[x] " : "[ ] ", 4);
  memcpy(tmp + 4, text, L);
  tmp[L + 4] = '\0';
  memcpy(g_notes[one_based - 1], tmp, NOTE_LEN);
  return 1;
}

static int notes_set_all_done(int done) {
  int i;
  for (i = 1; i <= g_note_n; i++) (void)notes_set_done_line(i, done);
  return g_note_n;
}

static int notes_purge_done(void) {
  int i = 0;
  int removed = 0;
  while (i < g_note_n) {
    if (note_is_done(g_notes[i])) {
      (void)notes_delete_line(i + 1);
      removed++;
    } else {
      i++;
    }
  }
  return removed;
}

static void format_notes_stats_body(char *body, size_t cap) {
  int i, done = 0, todo, pct;
  char banner[256];
  pc_format_identity_banner(banner, sizeof banner);
  for (i = 0; i < g_note_n; i++)
    if (note_is_done(g_notes[i])) done++;
  todo = g_note_n - done;
  pct = g_note_n ? (done * 100) / g_note_n : 0;
  {
    char bar[12];
    meter_bar_pct(bar, sizeof bar, pct);
    snprintf(body, cap,
             "%s\n\n"
             "Notes progress\n\n"
             "  Total: %d\n"
             "  Todo: %d\n"
             "  Done: %d\n"
             "  Completion: %d%%  [%s]\n\n"
             "  Use  notes purge done  to clear finished lines.\n",
             banner, g_note_n, todo, done, pct, bar);
  }
}

static void cmd_give(const char *rest, char *msg, size_t msgcap) {
  char work[256];
  char taken[MAX_ITEM_LEN];
  char itempretty[96];
  char pretty[128];
  char recipient_slug[MAX_ITEM_LEN];
  char *to_sep;
  const char *ent;
  int ix, dest_room = -1, off_room_gift = 0;
  ent = world_room_entity(g_room);
  strncpy(work, rest, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_trailing_space(work);
  to_sep = strstr(work, " to ");
  if (to_sep) {
    char who[128];
    int pr;
    *to_sep = '\0';
    strip_trailing_space(work);
    strncpy(who, to_sep + 4, sizeof who - 1);
    who[sizeof who - 1] = '\0';
    strip_leading_articles(who);
    pr = parser_expand_npc_pronoun(who, who, sizeof who, msg, msgcap);
    if (pr < 0) return;
    if (room_entity_matches_query_here(who, pretty, sizeof pretty)) {
      ent = world_room_entity(g_room);
      copy_capped(recipient_slug, sizeof recipient_slug, ent);
    } else if (npc_resolve_routine_query(who, recipient_slug,
                                         sizeof recipient_slug, pretty,
                                         sizeof pretty)) {
      const AetNpcRoutine *rt = npc_routine_for(recipient_slug);
      AetWorldClock wc;
      const char *dest_slug;
      get_world_clock(&wc);
      dest_slug = rt ? npc_routine_room_special(rt, &wc) : NULL;
      dest_room = dest_slug ? world_room_index(dest_slug) : -1;
      if (dest_room < 0) {
        snprintf(msg, msgcap,
                 "You are not sure where %s would be right now.", pretty);
        return;
      }
      if (dest_room == g_room) {
        snprintf(msg, msgcap,
                 "%s ought to be here, but you do not see them.", pretty);
        return;
      }
      off_room_gift = 1;
    } else {
      if (who[0])
        snprintf(msg, msgcap, "%s is not here to receive anything.",
                 pretty[0] ? pretty : who);
      else
        snprintf(msg, msgcap, "No one here matches that name.");
      return;
    }
  } else {
    if (!ent[0]) {
      snprintf(msg, msgcap, "No one is here to receive anything.");
      return;
    }
    copy_capped(recipient_slug, sizeof recipient_slug, ent);
  }
  strip_leading_articles(work);
  if (!work[0]) {
    snprintf(msg, msgcap, "Give what? (give <item> or give <item> to <name>.)");
    return;
  }
  ix = -1;
  {
    int r = resolve_inv_item_query(work, &ix, msg, msgcap);
    if (r < 0) return;
    if (r <= 0 || ix < 0) {
      snprintf(msg, msgcap, "You are not carrying \"%s\".", work);
      return;
    }
  }
  entity_pretty(recipient_slug, pretty, sizeof pretty);
  inv_take_out(ix, taken, sizeof taken);
  item_pretty(taken, itempretty, sizeof itempretty);
  if (str_ieq(g_last_focus, taken)) clear_focus();
  if (str_ieq(g_ready_item, taken)) g_ready_item[0] = '\0';
  g_score += 2;
  if (off_room_gift) {
    char place[160];
    if (g_room_item_n[dest_room] >= MAX_ITEMS_ROOM) {
      inv_add(taken);
      snprintf(msg, msgcap,
               "You cannot leave %s at %s — there is no space.",
               itempretty, resolve_world_title(dest_room));
      return;
    }
    room_add_visible_item(dest_room, taken);
    snprintf(place, sizeof place, "%s", resolve_world_title(dest_room));
    if (aet_merchant_index(recipient_slug) >= 0)
      merchant_rep_bump_gift(recipient_slug);
    else
      soc_bump_gift(recipient_slug);
    snprintf(msg, msgcap,
             "You leave %s at %s for %s to find later.",
             itempretty, place, pretty);
    return;
  }
  if (aet_merchant_index(recipient_slug) >= 0)
    merchant_rep_bump_gift(recipient_slug);
  else
    soc_bump_gift(recipient_slug);
  snprintf(msg, msgcap,
           "You hand over %s to %s. They accept it with a cautious nod toward "
           "%s.",
           itempretty, pretty, pc_display_name());
}

static int format_merchant_wares(char *body, size_t cap) {
  const char *ent = world_room_entity(g_room);
  const AetMerchantTable *mt;
  const AetMerchantOffer *o;
  char pretty[96];
  size_t len = 0;
  int w;
  int any;
  int mix;
  int rep0;
  AetPcSave p;
  int buy_edge, sell_edge;
  char purseb[32];
  if (!ent[0]) return 0;
  mt = aet_merchant_trades(ent);
  if (!mt) return 0;
  entity_pretty(ent, pretty, sizeof pretty);
  mix = aet_merchant_index(ent);
  rep0 = merchant_rep_score(mix);
  pc_capture(&p);
  pc_fill_narrative_defaults(&p);
  buy_edge = merchant_trade_buy_skill_pct(p.cha);
  sell_edge = merchant_trade_sell_skill_pct(p.cha);
  currency_format_compact(g_coins, purseb, sizeof purseb);
  w = snprintf(body, cap, "%s — trade list\n\n", pretty);
  if (w < 0 || (size_t)w >= cap) return 1;
  len = (size_t)w;
  if (mix >= 0) {
    w = snprintf(body + len, cap - len,
                 "Patron: %s (%d pts) — your buy prices use the right column; "
                 "sell uses the +%% bump.\n\n",
                 merchant_rep_tier_label(rep0), rep0);
    if (w < 0 || len + (size_t)w >= cap) return 1;
    len += (size_t)w;
  }
  w = snprintf(body + len, cap - len,
               "Trade knack: %s (CHA %d) — buy edge %d%%, sell edge %d%%.\n\n",
               merchant_trade_skill_label(p.cha), p.cha, buy_edge, sell_edge);
  if (w < 0 || len + (size_t)w >= cap) return 1;
  len += (size_t)w;
  if (barter_quote_sync() && str_ieq(g_barter_merchant, ent)) {
    char qb[48], lb[48];
    char itempretty[96];
    currency_format_long(g_barter_price, qb, sizeof qb);
    currency_format_long(g_barter_list_price, lb, sizeof lb);
    item_pretty(g_barter_item, itempretty, sizeof itempretty);
    w = snprintf(body + len, cap - len,
                 "Pending barter quote: %s %s for %s (list %s). "
                 "Use %s %s before turn %d.\n\n",
                 g_barter_mode == BARTER_SELL ? "sell" : "buy", itempretty, qb,
                 lb,
                 g_barter_mode == BARTER_SELL ? "sell" : "buy", itempretty,
                 g_barter_expire_turn);
    if (w < 0 || len + (size_t)w >= cap) return 1;
    len += (size_t)w;
  }
  w = snprintf(body + len, cap - len, "For sale (you pay):\n");
  if (w < 0 || len + (size_t)w >= cap) return 1;
  len += (size_t)w;
  any = 0;
  for (o = mt->stock; o->item && o->item[0]; o++) {
    int ypay = mix >= 0 ? merchant_adjust_buy_price(o->price, rep0, p.cha)
                        : merchant_adjust_buy_price(o->price, 0, p.cha);
    char payb[16], listb[16];
    char itempretty[96];
    currency_format_compact(ypay, payb, sizeof payb);
    currency_format_compact(o->price, listb, sizeof listb);
    item_pretty(o->item, itempretty, sizeof itempretty);
    any = 1;
    if (mix >= 0 && ypay != o->price)
      w = snprintf(body + len, cap - len, "  %-28s  %10s  (list %s)\n",
                   itempretty, payb, listb);
    else
      w = snprintf(body + len, cap - len, "  %-28s  %10s\n", itempretty, payb);
    if (w < 0 || len + (size_t)w >= cap - 32) break;
    len += (size_t)w;
  }
  if (!any) {
    w = snprintf(body + len, cap - len, "  (nothing listed for sale)\n");
    if (w < 0 || len + (size_t)w >= cap) return 1;
    len += (size_t)w;
  }
  w = snprintf(body + len, cap - len, "\nThey buy (you receive):\n");
  if (w < 0 || len + (size_t)w >= cap) return 1;
  len += (size_t)w;
  any = 0;
  for (o = mt->buys; o->item && o->item[0]; o++) {
    int yget = mix >= 0 ? merchant_adjust_sell_price(o->price, rep0, p.cha)
                        : merchant_adjust_sell_price(o->price, 0, p.cha);
    char getb[16], listb[16];
    char itempretty[96];
    currency_format_compact(yget, getb, sizeof getb);
    currency_format_compact(o->price, listb, sizeof listb);
    item_pretty(o->item, itempretty, sizeof itempretty);
    any = 1;
    if (mix >= 0 && yget != o->price)
      w = snprintf(body + len, cap - len, "  %-28s  %10s  (list %s)\n",
                   itempretty, getb, listb);
    else
      w = snprintf(body + len, cap - len, "  %-28s  %10s\n", itempretty, getb);
    if (w < 0 || len + (size_t)w >= cap - 64) break;
    len += (size_t)w;
  }
  if (!any) {
    w = snprintf(body + len, cap - len, "  (not buying anything listed)\n");
    if (w < 0 || len + (size_t)w >= cap) return 1;
    len += (size_t)w;
  }
  (void)snprintf(body + len, cap > len ? cap - len : 0,
                 "\nPurse: %s.  buy <item>  |  sell <item>  |  buy/sell all [except ...]\n"
                 "haggle [buy|sell] <item>  quotes one item for a few turns.\n",
                 purseb);
  return 1;
}

static int merchant_buys_item(const AetMerchantTable *mt, const char *item) {
  const AetMerchantOffer *o;
  if (!mt || !item || !item[0]) return 0;
  for (o = mt->buys; o->item && o->item[0]; o++) {
    if (str_ieq(item, o->item)) return 1;
  }
  return 0;
}

static int merchant_offer_matches_query(const char *item, const char *q,
                                        const char *qnorm) {
  return room_item_matches_query(item, q, qnorm);
}

static void cmd_trade_buy_all_except(const char *except_csv, char *msg,
                                     size_t msgcap) {
  const char *ent;
  const AetMerchantTable *mt;
  const AetMerchantOffer *o;
  char pretty[96];
  char one[256];
  int eligible = 0;
  int bought = 0;
  int total = 0;
  int partial = 0;
  if (!msg || msgcap == 0) return;
  ent = world_room_entity(g_room);
  if (!ent[0]) {
    snprintf(msg, msgcap, "There is no trader here.");
    return;
  }
  mt = aet_merchant_trades(ent);
  if (!mt) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s is not a trader in this port.", pretty);
    return;
  }
  for (o = mt->stock; o->item && o->item[0]; o++) {
    int before;
    if (item_excluded(o->item, except_csv)) continue;
    eligible++;
    before = g_coins;
    snprintf(one, sizeof one, "%s", o->item);
    cmd_trade_buy(one, msg, msgcap);
    if (!strncmp(msg, "Bought ", 7)) {
      bought++;
      total += before - g_coins;
    } else {
      partial = 1;
    }
  }
  if (bought == 0) {
    if (eligible == 0)
      snprintf(msg, msgcap,
               "Your exception list leaves nothing to buy from this merchant.");
    return;
  }
  if (!partial && bought == eligible)
    {
      char totalb[48];
      currency_format_long(total, totalb, sizeof totalb);
      snprintf(msg, msgcap, "You buy %d listed good%s for %s total.", bought,
               bought == 1 ? "" : "s", totalb);
    }
  else
    {
      char totalb[48];
      currency_format_long(total, totalb, sizeof totalb);
      snprintf(msg, msgcap,
               "You buy %d listed good%s for %s total. Some offers remain "
               "(filtered, too expensive, or no pack room).",
               bought, bought == 1 ? "" : "s", totalb);
    }
}

static void cmd_trade_sell_all_except(const char *except_csv, char *msg,
                                      size_t msgcap) {
  const char *ent;
  const AetMerchantTable *mt;
  const AetMerchantOffer *o;
  char pretty[96];
  char one[MAX_ITEM_LEN];
  int eligible = 0;
  int sold = 0;
  int total = 0;
  int partial = 0;
  int guard = 0;
  int i;
  if (!msg || msgcap == 0) return;
  ent = world_room_entity(g_room);
  if (!ent[0]) {
    snprintf(msg, msgcap, "There is no buyer here.");
    return;
  }
  mt = aet_merchant_trades(ent);
  if (!mt) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s is not a trader in this port.", pretty);
    return;
  }
  for (o = mt->buys; o->item && o->item[0]; o++) break;
  if (!o || !o->item || !o->item[0]) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s is not buying goods here.", pretty);
    return;
  }
  for (i = 0; i < g_inv_n; i++) {
    if (item_excluded(g_inv[i], except_csv)) continue;
    if (!merchant_buys_item(mt, g_inv[i])) continue;
    eligible++;
  }
  while (guard++ < MAX_INV * 2) {
    int picked = -1;
    int before;
    for (i = 0; i < g_inv_n; i++) {
      if (item_excluded(g_inv[i], except_csv)) continue;
      if (!merchant_buys_item(mt, g_inv[i])) continue;
      picked = i;
      break;
    }
    if (picked < 0) break;
    before = g_coins;
    memcpy(one, g_inv[picked], sizeof one);
    one[sizeof one - 1] = '\0';
    cmd_trade_sell(one, msg, msgcap);
    if (!strncmp(msg, "Sold ", 5)) {
      sold++;
      total += g_coins - before;
    } else {
      partial = 1;
      break;
    }
  }
  if (sold == 0) {
    if (eligible == 0)
      snprintf(msg, msgcap,
               "You are not carrying anything this merchant will buy%s%s.",
               except_csv && except_csv[0] ? " outside " : "",
               except_csv && except_csv[0] ? "your exception list" : "");
    return;
  }
  if (!partial && sold == eligible)
    {
      char totalb[48];
      currency_format_long(total, totalb, sizeof totalb);
      snprintf(msg, msgcap, "You sell %d carried good%s for %s total.", sold,
               sold == 1 ? "" : "s", totalb);
    }
  else
    {
      char totalb[48];
      currency_format_long(total, totalb, sizeof totalb);
      snprintf(msg, msgcap,
               "You sell %d carried good%s for %s total. Some saleable "
               "items remain.",
               sold, sold == 1 ? "" : "s", totalb);
    }
}

static void cmd_trade_haggle(const char *rest, char *msg, size_t msgcap) {
  char work[256];
  char norm[MAX_ITEM_LEN];
  char pretty[96];
  char detail[160];
  char itembuf[MAX_ITEM_LEN];
  char itempretty[96];
  char invpretty[96];
  char *item = work;
  const char *ent;
  const AetMerchantTable *mt;
  const AetMerchantOffer *buy_offer = NULL;
  const AetMerchantOffer *sell_offer = NULL;
  AetPcSave pc;
  int mix, rep0, friendship;
  int mode = BARTER_NONE;
  int can_buy = 0;
  int can_sell = 0;
  int ix = -1;
  int quote = 0;
  if (!rest) rest = "";
  strncpy(work, rest, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_leading_articles(work);
  strip_trailing_space(work);
  if (!work[0]) {
    snprintf(msg, msgcap,
             "Haggle what? Try: haggle buy <item>  or  haggle sell <item>.");
    return;
  }
  if (!strncmp(work, "buy ", 4)) {
    mode = BARTER_BUY;
    item = work + 4;
  } else if (!strncmp(work, "purchase ", 9)) {
    mode = BARTER_BUY;
    item = work + 9;
  } else if (!strncmp(work, "sell ", 5)) {
    mode = BARTER_SELL;
    item = work + 5;
  }
  while (*item == ' ') item++;
  strip_leading_articles(item);
  strip_trailing_space(item);
  if (!item[0]) {
    snprintf(msg, msgcap,
             "Haggle what? Try: haggle buy <item>  or  haggle sell <item>.");
    return;
  }
  ent = world_room_entity(g_room);
  if (!ent[0]) {
    snprintf(msg, msgcap, "There is no trader here to bargain with.");
    return;
  }
  mt = aet_merchant_trades(ent);
  if (!mt) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s is not a trader in this port.", pretty);
    return;
  }
  pc_capture(&pc);
  pc_fill_narrative_defaults(&pc);
  mix = aet_merchant_index(ent);
  rep0 = merchant_rep_score(mix);
  friendship = merchant_friendship_score(mix);
  query_norm_underscore(norm, sizeof norm, item);
  for (sell_offer = mt->stock; sell_offer->item && sell_offer->item[0]; sell_offer++) {
    if (merchant_offer_matches_query(sell_offer->item, item, norm)) {
      can_buy = 1;
      break;
    }
  }
  {
    int r = resolve_inv_item_query(item, &ix, msg, msgcap);
    if (r < 0) return;
    if (r == 0) {
      r = resolve_inv_item_query(norm, &ix, msg, msgcap);
      if (r < 0) return;
    }
    if (r > 0 && ix >= 0 && merchant_buys_item(mt, g_inv[ix])) {
      can_sell = 1;
      snprintf(itembuf, sizeof itembuf, "%s", g_inv[ix]);
      for (buy_offer = mt->buys; buy_offer->item && buy_offer->item[0]; buy_offer++) {
        if (str_ieq(buy_offer->item, itembuf)) break;
      }
    }
  }
  if (mode == BARTER_NONE) {
    if (can_buy && !can_sell)
      mode = BARTER_BUY;
    else if (can_sell && !can_buy)
      mode = BARTER_SELL;
    else if (can_buy && can_sell) {
      item_pretty(sell_offer->item, itempretty, sizeof itempretty);
      item_pretty(itembuf, invpretty, sizeof invpretty);
      snprintf(msg, msgcap,
               "Be specific here: haggle buy %s  or  haggle sell %s.", itempretty,
               invpretty);
      return;
    }
  }
  if (mode == BARTER_BUY) {
    int current;
    char quoteb[48], currentb[48];
    if (!can_buy || !sell_offer || !sell_offer->item) {
      snprintf(msg, msgcap, "They are not offering that here.");
      return;
    }
    item_pretty(sell_offer->item, itempretty, sizeof itempretty);
    current = merchant_adjust_buy_price(sell_offer->price, mix >= 0 ? rep0 : 0, pc.cha);
    quote = merchant_haggle_buy_price(current, rep0, friendship, pc.cha);
    currency_format_long(quote, quoteb, sizeof quoteb);
    currency_format_long(current, currentb, sizeof currentb);
    if (barter_quote_matches(BARTER_BUY, ent, sell_offer->item, NULL) &&
        g_barter_price == quote) {
      snprintf(msg, msgcap,
               "They already agreed to %s for %s. Use buy %s before turn %d.",
               quoteb, itempretty, itempretty,
               g_barter_expire_turn);
      return;
    }
    if (quote >= current) {
      snprintf(msg, msgcap, "You press for a better rate, but %s holds at %s.",
               itempretty, currentb);
      return;
    }
    barter_set(BARTER_BUY, ent, sell_offer->item, quote, sell_offer->price, g_turns + 3);
    snprintf(detail, sizeof detail, "%s %s %d->%d", ent, sell_offer->item, current,
             quote);
    causal_push("barter-buy", detail);
    snprintf(msg, msgcap,
             "You haggle %s down to %s (was %s). Quote holds through turn %d; "
             "use buy %s.",
             itempretty, quoteb, currentb, g_barter_expire_turn, itempretty);
    return;
  }
  if (mode == BARTER_SELL) {
    int current;
    char quoteb[48], currentb[48];
    if (ix < 0) {
      snprintf(msg, msgcap, "You are not carrying that.");
      return;
    }
    if (!can_sell || !buy_offer || !buy_offer->item) {
      snprintf(msg, msgcap, "They are not interested in that.");
      return;
    }
    current = merchant_adjust_sell_price(buy_offer->price, mix >= 0 ? rep0 : 0, pc.cha);
    quote = merchant_haggle_sell_price(current, rep0, friendship, pc.cha);
    item_pretty(itembuf, invpretty, sizeof invpretty);
    currency_format_long(quote, quoteb, sizeof quoteb);
    currency_format_long(current, currentb, sizeof currentb);
    if (barter_quote_matches(BARTER_SELL, ent, itembuf, NULL) &&
        g_barter_price == quote) {
      snprintf(msg, msgcap,
               "They already agreed to pay %s for %s. Use sell %s before turn %d.",
               quoteb, invpretty, invpretty, g_barter_expire_turn);
      return;
    }
    if (quote <= current) {
      snprintf(msg, msgcap, "They hear you out, but hold at %s for %s.",
               currentb, invpretty);
      return;
    }
    barter_set(BARTER_SELL, ent, itembuf, quote, buy_offer->price, g_turns + 3);
    snprintf(detail, sizeof detail, "%s %s %d->%d", ent, itembuf, current, quote);
    causal_push("barter-sell", detail);
    snprintf(msg, msgcap,
             "You talk %s up to %s (was %s). Quote holds through turn %d; use "
             "sell %s.",
             invpretty, quoteb, currentb, g_barter_expire_turn, invpretty);
    return;
  }
  snprintf(msg, msgcap, "They are not trading that here.");
}

static void cmd_trade_buy(const char *rest, char *msg, size_t msgcap) {
  char work[256];
  char norm[MAX_ITEM_LEN];
  char pretty[96];
  char itempretty[96];
  AetPcSave pc;
  const char *ent;
  const AetMerchantTable *mt;
  const AetMerchantOffer *o;
  strncpy(work, rest, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_leading_articles(work);
  strip_trailing_space(work);
  if (!work[0]) {
    snprintf(msg, msgcap,
             "Buy what? Try: wares  —  or  buy <item>  (needs money in your purse).");
    return;
  }
  ent = world_room_entity(g_room);
  pc_capture(&pc);
  pc_fill_narrative_defaults(&pc);
  if (!ent[0]) {
    snprintf(msg, msgcap, "There is no trader here.");
    return;
  }
  mt = aet_merchant_trades(ent);
  if (!mt) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s is not a trader in this port.", pretty);
    return;
  }
  for (o = mt->stock; o->item && o->item[0]; o++) break;
  if (!o || !o->item || !o->item[0]) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s has nothing for sale here.", pretty);
    return;
  }
  query_norm_underscore(norm, sizeof norm, work);
  {
    const AetMerchantOffer *matches[10];
    int match_n = 0;
    char picks[10][MAX_ITEM_LEN];
    int k;
    for (o = mt->stock; o->item && o->item[0]; o++) {
      if (!merchant_offer_matches_query(o->item, work, norm)) continue;
      if (match_n < (int)(sizeof matches / sizeof matches[0]))
        matches[match_n++] = o;
    }
    if (match_n == 0) {
      snprintf(msg, msgcap, "They are not offering that here.");
      return;
    }
    if (match_n > 1) {
      char slug[MAX_ITEM_LEN];
      int r;
      for (k = 0; k < match_n; k++)
        snprintf(picks[k], sizeof picks[0], "%s", matches[k]->item);
      g_disambig_next_act = DISAMBIG_ACT_BUY;
      r = disambig_resolve_ambiguous(DISAMBIG_ACT_BUY, 0, work, picks, match_n,
                                     slug, sizeof slug, msg, msgcap);
      if (r < 0) return;
      if (r == 1) {
        for (k = 0; k < match_n; k++) {
          if (str_ieq(matches[k]->item, slug)) {
            o = matches[k];
            goto buy_matched_offer;
          }
        }
      }
      return;
    }
    o = matches[0];
buy_matched_offer:
    {
      int mix = aet_merchant_index(ent);
      int rep0 = merchant_rep_score(mix);
      int used_barter = 0;
      int price = merchant_adjust_buy_price(o->price, mix >= 0 ? rep0 : 0, pc.cha);
      int quoted_price = 0;
      char priceb[48], purseb[48], listb[48];
      if (barter_quote_matches(BARTER_BUY, ent, o->item, &quoted_price) &&
          quoted_price < price) {
        price = quoted_price;
        used_barter = 1;
      }
      currency_format_long(price, priceb, sizeof priceb);
      currency_format_long(g_coins, purseb, sizeof purseb);
      currency_format_long(o->price, listb, sizeof listb);
      item_pretty(o->item, itempretty, sizeof itempretty);
      if (g_coins < price) {
        snprintf(msg, msgcap, "You need %s (you have %s).", priceb, purseb);
        return;
      }
      if (g_inv_n >= MAX_INV) {
        snprintf(msg, msgcap, "Your pack is full.");
        return;
      }
      g_coins -= price;
      inv_add(o->item);
      merchant_rep_bump_slug(ent, 1);
      trade_history_push("buy", ent, o->item, price, o->price, g_coins);
      if (used_barter) {
        barter_clear();
        snprintf(msg, msgcap, "Bought %s for %s (listed %s — haggled).",
                 itempretty, priceb, listb);
      } else if (price != o->price)
        snprintf(msg, msgcap, "Bought %s for %s (listed %s — trade rate).",
                 itempretty, priceb, listb);
      else
        snprintf(msg, msgcap, "Bought %s for %s.", itempretty, priceb);
      {
        AetPcSave pb;
        size_t L = strlen(msg);
        pc_capture(&pb);
        pc_fill_narrative_defaults(&pb);
        if (pb.cha >= 15 && L + 8 < msgcap)
          (void)snprintf(msg + L, msgcap - L,
                         " The trade lands smoothly — you've done this before.");
      }
    }
  }
}

static void cmd_trade_sell(const char *rest, char *msg, size_t msgcap) {
  char work[256];
  char norm[MAX_ITEM_LEN];
  char tmp[MAX_ITEM_LEN];
  char pretty[96];
  AetPcSave pc;
  const char *ent;
  const AetMerchantTable *mt;
  const AetMerchantOffer *o;
  int ix;
  strncpy(work, rest, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_leading_articles(work);
  strip_trailing_space(work);
  if (!work[0]) {
    snprintf(msg, msgcap, "Sell what? (they only buy listed goods.)");
    return;
  }
  ent = world_room_entity(g_room);
  pc_capture(&pc);
  pc_fill_narrative_defaults(&pc);
  if (!ent[0]) {
    snprintf(msg, msgcap, "There is no buyer here.");
    return;
  }
  mt = aet_merchant_trades(ent);
  if (!mt) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s is not a trader in this port.", pretty);
    return;
  }
  for (o = mt->buys; o->item && o->item[0]; o++) break;
  if (!o || !o->item || !o->item[0]) {
    entity_pretty(ent, pretty, sizeof pretty);
    snprintf(msg, msgcap, "%s is not buying goods here.", pretty);
    return;
  }
  query_norm_underscore(norm, sizeof norm, work);
  ix = -1;
  {
    int r = resolve_inv_item_query(work, &ix, msg, msgcap);
    if (r < 0) return;
    if (r == 0) {
      r = resolve_inv_item_query(norm, &ix, msg, msgcap);
      if (r < 0) return;
    }
    if (r <= 0 || ix < 0) {
      snprintf(msg, msgcap, "You are not carrying that.");
      return;
    }
  }
  for (o = mt->buys; o->item && o->item[0]; o++) {
    int mix;
    int rep0;
    int pay;
    int used_barter = 0;
    int quoted_price = 0;
    char payb[48], listb[48];
    char itempretty[96];
    if (!str_ieq(g_inv[ix], o->item)) continue;
    mix = aet_merchant_index(ent);
    rep0 = merchant_rep_score(mix);
    pay = merchant_adjust_sell_price(o->price, mix >= 0 ? rep0 : 0, pc.cha);
    if (barter_quote_matches(BARTER_SELL, ent, g_inv[ix], &quoted_price) &&
        quoted_price > pay) {
      pay = quoted_price;
      used_barter = 1;
    }
    currency_format_long(pay, payb, sizeof payb);
    currency_format_long(o->price, listb, sizeof listb);
    inv_take_out(ix, tmp, sizeof tmp);
    item_pretty(tmp, itempretty, sizeof itempretty);
    if (str_ieq(g_ready_item, tmp)) g_ready_item[0] = '\0';
    if (str_ieq(g_last_focus, tmp)) clear_focus();
    g_coins += pay;
    merchant_rep_bump_slug(ent, 1);
    trade_history_push("sell", ent, tmp, pay, o->price, g_coins);
    if (used_barter) {
      barter_clear();
      snprintf(msg, msgcap, "Sold %s for %s (listed %s — haggled).",
               itempretty, payb, listb);
    } else if (pay != o->price)
      snprintf(msg, msgcap, "Sold %s for %s (listed %s — trade rate).",
               itempretty, payb, listb);
    else
      snprintf(msg, msgcap, "Sold %s for %s.", itempretty, payb);
    {
      AetPcSave pb;
      size_t L = strlen(msg);
      pc_capture(&pb);
      pc_fill_narrative_defaults(&pb);
      if (pb.cha >= 15 && L + 8 < msgcap)
        (void)snprintf(msg + L, msgcap - L,
                       " The buyer counts it out like you've done this before.");
    }
    return;
  }
  snprintf(msg, msgcap, "They are not interested in that.");
}

static const char *const CONSUME_FOOD_IDS[] = {
    "bread",       "fresh_bread", "cheese",      "stew",        "flour",
    "eggs",        "rations",       "fish",        "dried_meat",  "roasted_meat",
    "meat",        "fancy_cakes", "coffee_beans", "carrots",     "turnip",
    "pumpkin",     NULL};

static const char *const CONSUME_DRINK_IDS[] = {
    "ale",          "wine",      "wine_bottle", "old_wine", "mead",
    "waterskin",    "stream_water", "clear_water", "dark_water", NULL};

static int consume_id_in_list(const char *id, const char *const *list) {
  const char *const *p;
  if (!id || !id[0] || !list) return 0;
  for (p = list; *p; p++)
    if (str_ieq(id, *p)) return 1;
  return 0;
}

static int consume_is_drink_id(const char *id) {
  return consume_id_in_list(id, CONSUME_DRINK_IDS) ||
         consume_id_in_list(id, world_consume_drink_ids());
}

static int consume_is_food_id(const char *id) {
  return consume_id_in_list(id, CONSUME_FOOD_IDS) ||
         consume_id_in_list(id, world_consume_food_ids());
}

static int consume_resolve_slot(const char *rest, char *work, size_t wcap) {
  int ordinal = 0, exclude_last = 0;
  int prep;
  strncpy(work, rest, wcap - 1);
  work[wcap - 1] = '\0';
  strip_leading_articles(work);
  strip_trailing_space(work);
  if (!work[0]) return -2;
  prep = parser_prepare_object_query(work, work, wcap, &ordinal, &exclude_last,
                                     NULL, 0);
  if (prep < 0) return -3;
  {
    int ix = -1;
    int r = resolve_inv_item_query(work, &ix, NULL, 0);
    if (r <= 0) return -1;
    return ix;
  }
}

static void consume_item_message(const char *id, int drink, char *msg,
                                 size_t msgcap) {
  if (drink) {
    if (str_ieq(id, "dark_water"))
      snprintf(msg, msgcap,
               "Cold iron and peat on the tongue. You swallow anyway.");
    else if (str_ieq(id, "ale") || str_ieq(id, "mead"))
      snprintf(msg, msgcap, "Malt, foam, and honest burn.");
    else if (str_ieq(id, "wine") || str_ieq(id, "wine_bottle") ||
             str_ieq(id, "old_wine"))
      snprintf(msg, msgcap, "Warmth slides down; your cheeks flush.");
    else if (strstr(id, "water") != NULL)
      snprintf(msg, msgcap, "Clean and cold — exactly what you needed.");
    else
      snprintf(msg, msgcap, "You drink it down.");
    return;
  }
  if (str_ieq(id, "flour"))
    snprintf(msg, msgcap,
             "Dry as chalk. You cough and reconsider baking properly.");
  else if (str_ieq(id, "coffee_beans"))
    snprintf(msg, msgcap, "A bitter crunch. Adventurers can't be choosers.");
  else if (str_ieq(id, "bread") || str_ieq(id, "fresh_bread"))
    snprintf(msg, msgcap, "Simple and filling — crumbs everywhere.");
  else if (str_ieq(id, "cheese"))
    snprintf(msg, msgcap, "Sharp, salty, satisfying.");
  else if (str_ieq(id, "stew") || str_ieq(id, "rations"))
    snprintf(msg, msgcap, "Hearty enough to quiet your stomach.");
  else if (str_ieq(id, "fish"))
    snprintf(msg, msgcap, "Flaky. A little grit, a little river.");
  else if (str_ieq(id, "fancy_cakes"))
    snprintf(msg, msgcap, "Sweet enough to forget the road for a moment.");
  else
    snprintf(msg, msgcap, "You eat it. That's enough for now.");
}

static int cmd_consume(const char *rest, int drink, char *msg, size_t msgcap) {
  char work[256];
  char tmp[MAX_ITEM_LEN];
  int ix = consume_resolve_slot(rest, work, sizeof work);
  if (ix == -2) {
    snprintf(msg, msgcap, "%s what?", drink ? "Drink" : "Eat");
    return 0;
  }
  if (ix == -3) {
    snprintf(msg, msgcap,
             "Nothing in mind — name something in your pack, or examine an "
             "item first.");
    return 1;
  }
  if (ix < 0) {
    snprintf(msg, msgcap, "You are not carrying that.");
    return 1;
  }
  if (drink) {
    if (!consume_is_drink_id(g_inv[ix])) {
      if (consume_is_food_id(g_inv[ix]))
        snprintf(msg, msgcap, "That's solid food — try 'eat'.");
      else
        snprintf(msg, msgcap, "You shouldn't drink that.");
      return 1;
    }
  } else {
    if (!consume_is_food_id(g_inv[ix])) {
      if (consume_is_drink_id(g_inv[ix]))
        snprintf(msg, msgcap, "That's a drink — try 'drink'.");
      else
        snprintf(msg, msgcap, "Not something you'd eat.");
      return 1;
    }
  }
  inv_take_out(ix, tmp, sizeof tmp);
  if (str_ieq(g_ready_item, tmp)) g_ready_item[0] = '\0';
  if (str_ieq(g_last_focus, tmp)) clear_focus();
  g_score += 1;
  consume_item_message(tmp, drink, msg, msgcap);
  return 1;
}

static int try_use_consume(const char *rest, char *msg, size_t msgcap,
                           int *turn_advance_out) {
  char work[256];
  int ix = consume_resolve_slot(rest, work, sizeof work);
  if (ix == -2) return 0;
  if (ix == -3) {
    snprintf(msg, msgcap,
             "Nothing in mind — name something in your pack, or examine an "
             "item first.");
    *turn_advance_out = 1;
    return 1;
  }
  if (ix < 0) {
    snprintf(msg, msgcap, "You are not carrying that.");
    *turn_advance_out = 1;
    return 1;
  }
  if (consume_is_drink_id(g_inv[ix])) {
    *turn_advance_out = cmd_consume(rest, 1, msg, msgcap);
    return 1;
  }
  if (consume_is_food_id(g_inv[ix])) {
    *turn_advance_out = cmd_consume(rest, 0, msg, msgcap);
    return 1;
  }
  return 0;
}

static void sensory_append_pc(char *msg, size_t msgcap, int listening) {
  AetPcSave pr;
  size_t L;
  pc_capture(&pr);
  pc_fill_narrative_defaults(&pr);
  L = strlen(msg);
  if (L + 12 >= msgcap) return;
  if (listening) {
    if (pr.per >= 16)
      (void)snprintf(msg + L, msgcap - L,
                     " Your ears sort signal from noise like a habit.");
    else if (pr.per >= 13)
      (void)snprintf(msg + L, msgcap - L,
                     " One detail hangs in the air longer than the rest.");
    else if (pr.wis >= 15)
      (void)snprintf(msg + L, msgcap - L,
                     " You wait until the quiet decides to mean something.");
  } else {
    if (pr.intl >= 16)
      (void)snprintf(msg + L, msgcap - L,
                     " You note ingredients without trying — mind likes lists.");
    else if (pr.per >= 14)
      (void)snprintf(msg + L, msgcap - L,
                     " Something in the mix tugs at memory, faintly.");
  }
}

static void sensory_line(const char *sense, char *msg, size_t msgcap) {
  const char *slug = world_slug(g_room);
  const char *reg = world_region(g_room);
  int listening = sense && strstr(sense, "listen") != NULL;
  if (room_too_dark_to_see()) {
    snprintf(msg, msgcap,
             "In the dark you strain to %s — little beyond your own breathing "
             "and the floor.",
             sense);
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "tavern") != NULL) {
    snprintf(msg, msgcap,
             "Ale, old wood, warm bread, and banked smoke cling to the room. "
             "The tavern smell has layers, like a ledger no one closes.");
    goto sensory_tail;
  }
  if (listening && slug && strstr(slug, "tavern") != NULL) {
    snprintf(msg, msgcap,
             "Conversation rises and falls in pockets: mugs, chairs, a laugh "
             "cut short, and somewhere the patient promise of music.");
    goto sensory_tail;
  }
  if (!listening && slug && (strstr(slug, "forest") || strstr(slug, "grove"))) {
    snprintf(msg, msgcap,
             "Leaf mold, sap, and wet bark. Under it all is a faint mineral "
             "sharpness, as if old stone is close to the surface.");
    goto sensory_tail;
  }
  if (listening && slug && (strstr(slug, "forest") || strstr(slug, "grove"))) {
    snprintf(msg, msgcap,
             "Branches rub together overhead. The forest keeps many small "
             "rhythms, none of them quite willing to become a song.");
    goto sensory_tail;
  }
  if (!listening && slug && (strstr(slug, "river") || strstr(slug, "dock") ||
                             strstr(slug, "pond") || strstr(slug, "lake"))) {
    snprintf(msg, msgcap,
             "Cold water, reeds, mud, and the clean metallic scent of fish. "
             "Good weather would make this place useful.");
    goto sensory_tail;
  }
  if (listening && slug && (strstr(slug, "river") || strstr(slug, "dock") ||
                            strstr(slug, "pond") || strstr(slug, "lake"))) {
    snprintf(msg, msgcap,
             "Water ticks against wood and stone. Every few breaths, something "
             "breaks the surface and vanishes again.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "waystone")) {
    snprintf(msg, msgcap,
             "Hot metal from the forge mixes with dry stone and a clean ozone "
             "note from the crystal.");
    goto sensory_tail;
  }
  if (listening && slug && (strstr(slug, "waystone") || strstr(slug, "nexus"))) {
    snprintf(msg, msgcap,
             "The travel stone hums below hearing. You feel the rhythm more "
             "in your teeth than in your ears.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "temple") != NULL) {
    snprintf(msg, msgcap,
             "Incense, cool stone, and a faint ozone thread from carved runes. "
             "The air tastes like prayer held too long in the throat.");
    goto sensory_tail;
  }
  if (listening && slug && strstr(slug, "temple") != NULL) {
    snprintf(msg, msgcap,
             "Footsteps fade into silence. Somewhere water drips; somewhere "
             "else a crystal ticks like a patient heart. No judgment — only "
             "witness.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "mill") != NULL) {
    snprintf(msg, msgcap,
             "Flour dust, river mud, old grease, and cold wood. The mill "
             "smells like work that refuses to quit.");
    goto sensory_tail;
  }
  if (listening && slug && strstr(slug, "mill") != NULL) {
    snprintf(msg, msgcap,
             "A stuck wheel creaks once and stops. Gears tick when the wind "
             "nudges them — the building still trying to remember motion.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "farm") != NULL) {
    snprintf(msg, msgcap,
             "Warm hay, turned soil, animal musk, and sun on linen. The farm "
             "smells like honest exhaustion.");
    goto sensory_tail;
  }
  if (listening && slug && strstr(slug, "farm") != NULL) {
    snprintf(msg, msgcap,
             "Chickens mutter. Wind combs the crops. Somewhere a gate clicks "
             "and Jasper hums without knowing he is heard.");
    goto sensory_tail;
  }
  if (!listening && slug && (strstr(slug, "village_square") != NULL ||
                             strstr(slug, "town_square") != NULL ||
                             strstr(slug, "market") != NULL)) {
    snprintf(msg, msgcap,
             "Bread, livestock, dust, and a hundred small trades. The square "
             "smells like coin changing hands.");
    goto sensory_tail;
  }
  if (listening && slug && (strstr(slug, "village_square") != NULL ||
                            strstr(slug, "town_square") != NULL ||
                            strstr(slug, "market") != NULL)) {
    snprintf(msg, msgcap,
             "Haggling, cart wheels, a bard's note testing the air, and "
             "missionaries laughing with someone shy.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "cave") != NULL) {
    snprintf(msg, msgcap,
             "Cold stone, wet mineral, and old smoke. The dark has a taste "
             "like forgotten iron.");
    goto sensory_tail;
  }
  if (listening && slug && strstr(slug, "cave") != NULL) {
    snprintf(msg, msgcap,
             "Water ticks far off. Your footstep returns too late, as if the "
             "cave is deciding whether to answer.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "cellar") != NULL) {
    snprintf(msg, msgcap,
             "Oak barrels, spilled ale, and cool stone. The cellar keeps the "
             "tavern's honest secrets.");
    goto sensory_tail;
  }
  if (listening && slug && strstr(slug, "cellar") != NULL) {
    snprintf(msg, msgcap,
             "A barrel tap drips. Somewhere above, muffled laughter; down here, "
             "Silas moves with deliberate quiet.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "bard_stage") != NULL) {
    snprintf(msg, msgcap,
             "Old sawdust, rosin, and perfume from crowds long gone. The stage "
             "still expects applause.");
    goto sensory_tail;
  }
  if (!listening && (strstr(slug, "blacksmith") != NULL ||
                     strstr(slug, "forge") != NULL)) {
    snprintf(msg, msgcap,
             "Hot iron, coal smoke, leather oil, and sweat. The forge smells "
             "like work that refuses to cool down.");
    goto sensory_tail;
  }
  if (listening && (strstr(slug, "blacksmith") != NULL ||
                    strstr(slug, "forge") != NULL)) {
    snprintf(msg, msgcap,
             "Hammer rhythm, bellows sigh, quench hiss — the forge speaks in "
             "metal dialects.");
    goto sensory_tail;
  }
  if (!listening && (strstr(slug, "village_inn") != NULL ||
                     strstr(slug, "inn_") != NULL)) {
    snprintf(msg, msgcap,
             "Clean linen, stew, beeswax, and a quieter hospitality than the "
             "tavern — Lydia keeps this place gentle.");
    goto sensory_tail;
  }
  if (!listening && (strstr(slug, "nexus") != NULL)) {
    snprintf(msg, msgcap,
             "Ozone, warm crystal, and a pressure behind your teeth — as if "
             "distance is stacked nearby, waiting for permission.");
    goto sensory_tail;
  }
  if (listening && (strstr(slug, "nexus") != NULL)) {
    snprintf(msg, msgcap,
             "The monolith hums below hearing. Runes tick like polite "
             "machinery deciding which road may open.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "west_of_house") != NULL) {
    snprintf(msg, msgcap,
             "Boarded wood, grass, and a chill from runes carved in the door "
             "frame — the house holds its breath.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "artifact_trader") != NULL) {
    snprintf(msg, msgcap,
             "Ozone, old velvet, and metal that remembers magic. Each display "
             "hums at a different pitch.");
    goto sensory_tail;
  }
  if (listening && slug && strstr(slug, "artifact_trader") != NULL) {
    snprintf(msg, msgcap,
             "Cloth rustles when no wind blows. Somewhere a crystal ticks like "
             "a patient auctioneer.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "general_store") != NULL) {
    snprintf(msg, msgcap,
             "Soap, rope, lamp oil, and something sweeter from the back room. "
             "Sam keeps the front respectable.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "marketplace") != NULL) {
    snprintf(msg, msgcap,
             "Spices, leather, livestock, and a dozen competing ambitions. "
             "Coin changes hands faster than gossip.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "exotic_pet") != NULL) {
    snprintf(msg, msgcap,
             "Feathers, fur, and exotic musk. Cages click; something trills "
             "a note you cannot place.");
    goto sensory_tail;
  }
  if (!listening && slug && (strstr(slug, "vault") != NULL ||
                             strstr(slug, "underground") != NULL)) {
    snprintf(msg, msgcap,
             "Cold stone, lamp smoke, and locked air. Wealth here smells "
             "like patience.");
    goto sensory_tail;
  }
  if (!listening && slug && strstr(slug, "watchtower") != NULL) {
    snprintf(msg, msgcap,
             "Wind, tar, and far woodsmoke. Height turns the ridge into a "
             "map you can almost read.");
    goto sensory_tail;
  }
  if (!listening && slug && (strstr(slug, "inside_house") != NULL ||
                             strstr(slug, "foyer") != NULL ||
                             strstr(slug, "attic") != NULL)) {
    snprintf(msg, msgcap,
             "Dust, closed rooms, and old varnish. The house keeps stories "
             "in its walls.");
    goto sensory_tail;
  }
  if (!listening && slug && (strstr(slug, "library") || strstr(slug, "study"))) {
    snprintf(msg, msgcap,
             "Old paper, dry glue, candle smoke, and dust. Knowledge has a "
             "smell when nobody has moved it for years.");
    goto sensory_tail;
  }
  if (listening && slug && (strstr(slug, "library") || strstr(slug, "study"))) {
    snprintf(msg, msgcap,
             "The room is quiet enough that paper seems loud: tiny shifts, "
             "settling shelves, a faint draft worrying the pages.");
    goto sensory_tail;
  }
  if (!listening && reg && strstr(reg, "Hollow Ridge") != NULL &&
      slug && strstr(slug, "house") != NULL) {
    snprintf(msg, msgcap,
             "Damp wood, grass, and the sour dust of a house that has been "
             "keeping its breath too long.");
    goto sensory_tail;
  }
  if (listening && reg && strstr(reg, "Hollow Ridge") != NULL &&
      slug && strstr(slug, "house") != NULL) {
    snprintf(msg, msgcap,
             "The house creaks in small private ways. Wind moves around it, "
             "but not through it.");
    goto sensory_tail;
  }
  if (world_room_is_dark(g_room))
    snprintf(msg, msgcap,
             "You %s carefully; shadows shift at the edge of your light.", sense);
  else
    snprintf(msg, msgcap,
             "You %s. The air carries dust, distance, and small signs of life.",
             sense);
sensory_tail:
  sensory_append_pc(msg, msgcap, listening);
}

static void strip_leading_articles(char *s) {
  for (;;) {
    if (!strncmp(s, "the ", 4))
      memmove(s, s + 4, strlen(s + 4) + 1);
    else if (!strncmp(s, "a ", 2))
      memmove(s, s + 2, strlen(s + 2) + 1);
    else if (!strncmp(s, "an ", 3))
      memmove(s, s + 3, strlen(s + 3) + 1);
    else
      break;
  }
}

static int room_item_matches_query(const char *item, const char *q,
                                   const char *qnorm) {
  char inorm[MAX_ITEM_LEN];
  char pretty[96];
  char pnorm[96];
  char work[256];
  char tok[64];
  const char *p;
  int tokens = 0;
  if (!item || !q[0]) return 0;
  query_norm_underscore(inorm, sizeof inorm, item);
  item_pretty(item, pretty, sizeof pretty);
  query_norm_underscore(pnorm, sizeof pnorm, pretty);
  if (str_ieq(item, q) || str_ieq(item, qnorm)) return 1;
  if (str_ieq(inorm, qnorm)) return 1;
  if (str_ieq(pretty, q) || str_ieq(pretty, qnorm)) return 1;
  if (str_ieq(pnorm, qnorm)) return 1;
  strncpy(work, q, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_leading_articles(work);
  strip_trailing_space(work);
  for (p = work; *p;) {
    size_t tn = 0;
    while (*p == ' ') p++;
    if (!*p) break;
    while (p[tn] && p[tn] != ' ' && tn + 1 < sizeof tok) {
      tok[tn] = p[tn];
      tn++;
    }
    tok[tn] = '\0';
    p += tn;
    if (!tok[0]) continue;
    tokens++;
    if (!parser_token_matches_item(tok, item)) return 0;
  }
  if (tokens > 1) return 1;
  if (strstr(item, qnorm) != NULL) return 1;
  if (strstr(inorm, qnorm) != NULL) return 1;
  if (strstr(pnorm, qnorm) != NULL) return 1;
  return 0;
}

static int room_entity_matches_query_here(const char *q, char *pretty_out,
                                          size_t pretty_cap) {
  const char *ent = world_room_entity(g_room);
  char qnorm[MAX_ITEM_LEN];
  char pretty[64];
  char pnorm[64];
  if (!q || !q[0] || !ent || !ent[0]) return 0;
  entity_pretty(ent, pretty, sizeof pretty);
  query_norm_underscore(qnorm, sizeof qnorm, q);
  query_norm_underscore(pnorm, sizeof pnorm, pretty);
  if (!(str_ieq(ent, q) || str_ieq(ent, qnorm) || str_ieq(pretty, q) ||
        str_ieq(pretty, qnorm) || str_ieq(pnorm, qnorm) ||
        strstr(ent, qnorm) != NULL || strstr(pnorm, qnorm) != NULL))
    return 0;
  if (pretty_out && pretty_cap > 0) snprintf(pretty_out, pretty_cap, "%s", pretty);
  return 1;
}

static void disambig_clear(void) {
  g_disambig_act = DISAMBIG_ACT_NONE;
  g_disambig_next_act = DISAMBIG_ACT_NONE;
  g_disambig_inv = 0;
  g_disambig_query[0] = '\0';
  g_disambig_pick_n = 0;
}

static void disambig_reset_all(void) {
  disambig_clear();
  g_disambig_mem_n = 0;
}

static void disambig_remember(const char *query, const char *slug) {
  char qnorm[MAX_ITEM_LEN];
  int i;
  if (!query || !query[0] || !slug || !slug[0]) return;
  query_norm_underscore(qnorm, sizeof qnorm, query);
  for (i = 0; i < g_disambig_mem_n; i++) {
    if (str_ieq(g_disambig_mem[i].q, qnorm)) {
      snprintf(g_disambig_mem[i].slug, sizeof g_disambig_mem[i].slug, "%s",
               slug);
      return;
    }
  }
  if (g_disambig_mem_n >= DISAMBIG_MEM_MAX) {
    memmove(g_disambig_mem, g_disambig_mem + 1,
            (size_t)(DISAMBIG_MEM_MAX - 1) * sizeof g_disambig_mem[0]);
    g_disambig_mem_n = DISAMBIG_MEM_MAX - 1;
  }
  snprintf(g_disambig_mem[g_disambig_mem_n].q, sizeof g_disambig_mem[0].q, "%s",
           qnorm);
  snprintf(g_disambig_mem[g_disambig_mem_n].slug,
           sizeof g_disambig_mem[0].slug, "%s", slug);
  g_disambig_mem_n++;
}

static const char *disambig_recall(const char *query) {
  char qnorm[MAX_ITEM_LEN];
  int i;
  if (!query || !query[0]) return NULL;
  query_norm_underscore(qnorm, sizeof qnorm, query);
  for (i = 0; i < g_disambig_mem_n; i++)
    if (str_ieq(g_disambig_mem[i].q, qnorm)) return g_disambig_mem[i].slug;
  return NULL;
}

static void disambig_set_pending(int act, int inv, const char *query,
                                 char items[][MAX_ITEM_LEN], int n) {
  int i, cap = n > DISAMBIG_PICK_MAX ? DISAMBIG_PICK_MAX : n;
  g_disambig_act = act;
  g_disambig_inv = inv;
  g_disambig_pick_n = cap;
  query_norm_underscore(g_disambig_query, sizeof g_disambig_query, query);
  for (i = 0; i < cap; i++)
    snprintf(g_disambig_picks[i], sizeof g_disambig_picks[i], "%s", items[i]);
}

static void format_disambig_prompt(char *msg, size_t msgcap, int inv,
                                   char items[][MAX_ITEM_LEN], int n) {
  int i;
  char pretty[96];
  size_t L;
  if (!msg || msgcap < 32 || n <= 0) return;
  snprintf(msg, msgcap, "Did you mean");
  for (i = 0; i < n && i < DISAMBIG_PICK_MAX; i++) {
    item_pretty(items[i], pretty, sizeof pretty);
    L = strlen(msg);
    if (L + strlen(pretty) + 16 >= msgcap) break;
    snprintf(msg + L, msgcap - L, "%s (%d) %s", i == 0 ? "" : " or", i + 1,
             pretty);
  }
  L = strlen(msg);
  snprintf(msg + L, msgcap - L,
           "? Reply with the number or name — I'll remember your choice.");
  if (inv)
    strncat(msg, " (in your pack)", msgcap - strlen(msg) - 1);
}

static int disambig_pick_slug(const char *line, char *slug_out, size_t cap) {
  char qnorm[MAX_ITEM_LEN];
  char work[INPUT_LINE_MAX];
  int i, pick = 0;
  const char *p;
  if (!line || !line[0] || g_disambig_pick_n <= 0 || !slug_out || cap < 2)
    return 0;
  p = line;
  while (*p == ' ') p++;
  if (*p >= '1' && *p <= '9') {
    pick = (int)(*p - '0');
    p++;
    while (*p >= '0' && *p <= '9') {
      pick = pick * 10 + (int)(*p - '0');
      p++;
    }
    while (*p == ' ') p++;
    if (*p != '\0') pick = 0;
  }
  if (pick >= 1 && pick <= g_disambig_pick_n) {
    copy_capped(slug_out, cap, g_disambig_picks[pick - 1]);
    return 1;
  }
  strncpy(work, line, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_leading_articles(work);
  query_norm_underscore(qnorm, sizeof qnorm, work);
  for (i = 0; i < g_disambig_pick_n; i++) {
    char pnorm[MAX_ITEM_LEN];
    if (str_ieq(g_disambig_picks[i], work) || str_ieq(g_disambig_picks[i], qnorm)) {
      copy_capped(slug_out, cap, g_disambig_picks[i]);
      return 1;
    }
    item_pretty(g_disambig_picks[i], pnorm, sizeof pnorm);
    if (str_ieq(pnorm, work) || str_ieq(pnorm, qnorm)) {
      copy_capped(slug_out, cap, g_disambig_picks[i]);
      return 1;
    }
    if (room_item_matches_query(g_disambig_picks[i], work, qnorm)) {
      copy_capped(slug_out, cap, g_disambig_picks[i]);
      return 1;
    }
  }
  return 0;
}

static int disambig_resolve_ambiguous(int act, int inv, const char *query,
                                      char picks[][MAX_ITEM_LEN], int n,
                                      char *resolved, size_t resolved_cap,
                                      char *msg, size_t msgcap) {
  const char *mem;
  int i;
  if (n <= 0) return 0;
  mem = disambig_recall(query);
  if (mem && mem[0]) {
    for (i = 0; i < n; i++) {
      if (str_ieq(picks[i], mem)) {
        if (resolved && resolved_cap >= 2)
          snprintf(resolved, resolved_cap, "%s", mem);
        return 1;
      }
    }
  }
  if (act != DISAMBIG_ACT_NONE)
    disambig_set_pending(act, inv, query, picks, n);
  format_disambig_prompt(msg, msgcap, inv, picks, n);
  return -1;
}

static int resolve_room_item_query(const char *name, char *resolved,
                                   size_t resolved_cap, char *msg,
                                   size_t msgcap) {
  char work[256];
  char qnorm[MAX_ITEM_LEN];
  char picks[10][MAX_ITEM_LEN];
  int ordinal = 0, exclude_last = 0;
  int i, n = 0, prep;
  if (!name || !name[0] || !resolved || resolved_cap < 2) return 0;
  prep = parser_prepare_object_query(name, work, sizeof work, &ordinal,
                                     &exclude_last, msg, msgcap);
  if (prep < 0) return -1;
  if (!work[0]) return 0;
  name = work;
  query_norm_underscore(qnorm, sizeof qnorm, name);
  for (i = 0; i < g_room_item_n[g_room]; i++) {
    if (str_ieq(g_room_items[g_room][i], name) ||
        str_ieq(g_room_items[g_room][i], qnorm)) {
      snprintf(resolved, resolved_cap, "%s", g_room_items[g_room][i]);
      remember_focus_item(g_room_items[g_room][i]);
      return 1;
    }
  }
  for (i = 0; i < g_room_item_n[g_room]; i++) {
    if (exclude_last && g_last_focus[0] &&
        str_ieq(g_room_items[g_room][i], g_last_focus))
      continue;
    if (!room_item_matches_query(g_room_items[g_room][i], name, qnorm)) continue;
    if (n < (int)(sizeof picks / sizeof picks[0]))
      snprintf(picks[n], sizeof picks[n], "%s", g_room_items[g_room][i]);
    n++;
  }
  container_append_open_contents_picks(g_room, name, qnorm, picks, &n,
                                       exclude_last);
  if (n == 1) {
    snprintf(resolved, resolved_cap, "%s", picks[0]);
    disambig_remember(name, picks[0]);
    remember_focus_item(picks[0]);
    return 1;
  }
  if (n > 1 && ordinal >= 1 && ordinal <= n) {
    snprintf(resolved, resolved_cap, "%s", picks[ordinal - 1]);
    disambig_remember(name, picks[ordinal - 1]);
    remember_focus_item(picks[ordinal - 1]);
    return 1;
  }
  if (n > 1) {
    int act = g_disambig_next_act;
    g_disambig_next_act = DISAMBIG_ACT_NONE;
    return disambig_resolve_ambiguous(act, 0, name, picks, n, resolved,
                                    resolved_cap, msg, msgcap);
  }
  {
    char pretty[64];
    char work[256], qnorm[MAX_ITEM_LEN];
    if (room_entity_matches_query_here(name, pretty, sizeof pretty)) {
      snprintf(msg, msgcap,
               "You cannot take %s - people are not inventory. Try talk to %s, who, or nearby npc.",
               pretty, pretty);
      return -1;
    }
    query_norm_underscore(qnorm, sizeof qnorm, name);
    strncpy(work, name, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    if (container_hint_closed_item(g_room, work, qnorm, msg, msgcap)) return -1;
  }
  return 0;
}

static int resolve_inv_item_query(const char *name, int *ix_out, char *msg,
                                  size_t msgcap) {
  char work[256];
  char qnorm[MAX_ITEM_LEN];
  int ordinal = 0, exclude_last = 0;
  int i, n = 0, picks[10], prep;
  if (!name || !name[0] || !ix_out) return 0;
  prep = parser_prepare_object_query(name, work, sizeof work, &ordinal,
                                     &exclude_last, msg, msgcap);
  if (prep < 0) return -1;
  if (!work[0]) return 0;
  name = work;
  query_norm_underscore(qnorm, sizeof qnorm, name);
  for (i = 0; i < g_inv_n; i++) {
    if (str_ieq(g_inv[i], name) || str_ieq(g_inv[i], qnorm)) {
      *ix_out = i;
      remember_focus_item(g_inv[i]);
      return 1;
    }
  }
  for (i = 0; i < g_inv_n; i++) {
    if (exclude_last && g_last_focus[0] && str_ieq(g_inv[i], g_last_focus))
      continue;
    if (!room_item_matches_query(g_inv[i], name, qnorm)) continue;
    if (n < (int)(sizeof picks / sizeof picks[0])) picks[n] = i;
    n++;
  }
  if (n == 1) {
    *ix_out = picks[0];
    disambig_remember(name, g_inv[picks[0]]);
    remember_focus_item(g_inv[picks[0]]);
    return 1;
  }
  if (n > 1 && ordinal >= 1 && ordinal <= n) {
    *ix_out = picks[ordinal - 1];
    disambig_remember(name, g_inv[picks[ordinal - 1]]);
    remember_focus_item(g_inv[picks[ordinal - 1]]);
    return 1;
  }
  if (n > 1) {
    char names[10][MAX_ITEM_LEN];
    char slug[MAX_ITEM_LEN];
    int k, shown = n > 10 ? 10 : n;
    int act = g_disambig_next_act;
    int r;
    g_disambig_next_act = DISAMBIG_ACT_NONE;
    for (k = 0; k < shown; k++)
      snprintf(names[k], sizeof names[k], "%s", g_inv[picks[k]]);
    r = disambig_resolve_ambiguous(act, 1, name, names, shown, slug,
                                   sizeof slug, msg, msgcap);
    if (r == 1) {
      for (k = 0; k < shown; k++) {
        if (str_ieq(g_inv[picks[k]], slug)) {
          *ix_out = picks[k];
          return 1;
        }
      }
      r = -1;
    }
    return r;
  }
  return 0;
}

static void format_find_item_body(char *body, size_t cap, const char *raw) {
  char q[256];
  char qnorm[MAX_ITEM_LEN];
  int i, j, found = 0;
  char line[320];
  char banner[256];
  strncpy(q, raw, sizeof q - 1);
  q[sizeof q - 1] = '\0';
  strip_leading_articles(q);
  strip_trailing_space(q);
  pc_format_identity_banner(banner, sizeof banner);
  if (!q[0]) {
    snprintf(body, cap, "Find what? (find <item> — open sight only.)\n");
    return;
  }
  query_norm_underscore(qnorm, sizeof qnorm, q);
  snprintf(body, cap,
           "%s\n\n"
           "Matches for \"%s\" (visible floor items, your pack, and NPCs):\n\n",
           banner, raw);
  for (i = 0; i < g_inv_n; i++) {
    if (!room_item_matches_query(g_inv[i], q, qnorm)) continue;
    char pretty[96];
    item_pretty(g_inv[i], pretty, sizeof pretty);
    snprintf(line, sizeof line, "  (carried)  %s\n", pretty);
    strncat(body, line, cap - strlen(body) - 1);
    found = 1;
  }
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    for (j = 0; j < g_room_item_n[i]; j++) {
      if (!room_item_matches_query(g_room_items[i][j], q, qnorm)) continue;
      char pretty[96];
      item_pretty(g_room_items[i][j], pretty, sizeof pretty);
      snprintf(line, sizeof line, "  %s  —  %s  [%s]\n", pretty,
               resolve_world_title(i), world_slug(i));
      strncat(body, line, cap - strlen(body) - 1);
      found = 1;
    }
  }
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    const char *ent = world_room_entity(i);
    char pretty[96];
    char enorm[MAX_ITEM_LEN];
    if (!ent || !ent[0]) continue;
    entity_pretty(ent, pretty, sizeof pretty);
    query_norm_underscore(enorm, sizeof enorm, ent);
    if (!str_ieq(ent, q) && !str_ieq(pretty, q) && !str_ieq(enorm, qnorm) &&
        strstr(ent, qnorm) == NULL && strstr(enorm, qnorm) == NULL)
      continue;
    snprintf(line, sizeof line, "  %s  —  %s  [%s]  (person)\n", pretty,
             resolve_world_title(i), world_slug(i));
    strncat(body, line, cap - strlen(body) - 1);
    found = 1;
  }
  if (!found) {
    strncat(
        body,
        "  No loose match. Items in hidden stashes are not listed until "
        "search reveals them.\n",
        cap - strlen(body) - 1);
  }
}

static int content_item_response(const char *id, const char *mode, char *msg,
                                 size_t msgcap);

static int item_has_read_text(const char *id) {
  char buf[512];
  if (!id || !id[0]) return 0;
  if (aet_mods_item_description(id, buf, sizeof buf) && buf[0]) return 1;
  return content_item_response(id, "read", buf, sizeof buf);
}

static int reveal_hidden_item(const char *id) {
  int i, j;
  if (!id || !id[0]) return 0;
  for (i = 0; i < g_hidden_n[g_room]; i++) {
    if (!str_ieq(g_hidden_items[g_room][i], id)) continue;
    if (g_room_item_n[g_room] >= MAX_ITEMS_ROOM) return -1;
    memcpy(g_room_items[g_room][g_room_item_n[g_room]], id, strlen(id) + 1);
    g_room_item_n[g_room]++;
    for (j = i; j < g_hidden_n[g_room] - 1; j++)
      memcpy(g_hidden_items[g_room][j], g_hidden_items[g_room][j + 1],
             MAX_ITEM_LEN);
    g_hidden_n[g_room]--;
    remember_focus_item(id);
    return 1;
  }
  return 0;
}

static int content_item_response(const char *id, const char *mode, char *msg,
                                 size_t msgcap) {
  int is_read = mode && !strcmp(mode, "read");
  int is_touch = mode && !strcmp(mode, "touch");
  if (!id || !id[0]) return 0;

  if (aet_mods_item_description(id, msg, msgcap)) return 1;

  if (str_ieq(id, "mailbox")) {
    if (is_touch)
      snprintf(msg, msgcap,
               "The mailbox is cold, dented, and gritty with rust. The little "
               "door complains on its hinge.");
    else
      snprintf(msg, msgcap,
               "A small rusty mailbox with a creaky door. It feels older than "
               "the path around it, like it has been waiting to deliver one "
               "last joke.");
    return 1;
  }
  if (str_ieq(id, "leaflet")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The leaflet reads: \"WELCOME TO AETERNITAS. TAKE ONLY WHAT YOU "
               "CAN CARRY. TRUST DOORS LAST.\" A faded line at the bottom says "
               "\"Printed for the 1986 orientation disk.\"");
    else
      snprintf(msg, msgcap,
               "A brittle leaflet folded into quarters. The ink is cheap, the "
               "paper is stubborn, and the margin carries a tiny floppy-disk "
               "stamp.");
    return 1;
  }
  if (str_ieq(id, "hollow_ridge_primer")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Hollow Ridge Primer — excerpt:\n\n"
               "Veritasfurtum frays where divine attention thins. This ridge is "
               "not a border but a seam: manor, village, temple, and the wild "
               "between. Waystones mark crossings the Architect still recognizes; "
               "rifts are arguments between layers that no longer agree.\n\n"
               "Practical counsel: carry light, read signs twice, and never "
               "mistake a locked door for a wall. When the fog comes in, trust "
               "coin, compass, and the person who knows the exits.");
    else
      snprintf(msg, msgcap,
               "A saddle-stitched primer stamped with the Hollow Ridge seal. It "
               "smells of cheap ink and honest worry.");
    return 1;
  }
  if (str_ieq(id, "ancient_tome")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The tome speaks in layered hands — temple rubric, miller's "
               "marginalia, and a later scribe's warning about waystones. Core "
               "doctrine: the Architect's lattice holds reality together; "
               "Elysium crystals answer when the lattice is stressed; rifts form "
               "where borrowed worlds refuse to leave. One line repeats: \"A "
               "return must be earned before it is trusted.\"");
    else
      snprintf(msg, msgcap,
               "An ancient tome with bowed covers and a spine that crackles like "
               "a warning label.");
    return 1;
  }
  if (str_ieq(id, "scrap_metal")) {
    snprintf(msg, msgcap,
             "A jagged piece of workable metal. Not pretty, but the blacksmith "
             "or a future crafting bench would know what to do with it.");
    return 1;
  }
  if (str_ieq(id, "wood_scrap")) {
    snprintf(msg, msgcap,
             "Dry wood scrap, light enough to carry and straight enough for "
             "repairs, wedges, kindling, or rough crafting.");
    return 1;
  }
  if (str_ieq(id, "lockpick")) {
    if (is_touch)
      snprintf(msg, msgcap,
               "The pick flexes slightly between your fingers. Useful, but it "
               "would punish impatience.");
    else
      snprintf(msg, msgcap,
               "A slender steel lockpick for simple pin locks. This edition "
               "treats it as both a tool and a future minigame key.");
    return 1;
  }
  if (str_ieq(id, "rusty_pick")) {
    snprintf(msg, msgcap,
             "A rough, rust-bitten pick. It might work in a pinch, but any "
             "proper lockpicking game should make it fragile and noisy.");
    return 1;
  }
  if (str_ieq(id, "fine_lockpick")) {
    snprintf(msg, msgcap,
             "A balanced precision pick with polished steel. It feels like a "
             "tool made for patience, not force.");
    return 1;
  }
  if (str_ieq(id, "house_key")) {
    snprintf(msg, msgcap,
             "Old iron with a tired polish and a faint warmth, as if it remembers "
             "the house's keyhole. The bitting looks too deliberate for ordinary "
             "carpentry.");
    return 1;
  }
  if (str_ieq(id, "door")) {
    snprintf(msg, msgcap,
             "A heavy wooden door, boarded over and marked by a small keyhole. "
             "It does not look stuck; it looks selective.");
    return 1;
  }
  if (str_ieq(id, "shed_door")) {
    snprintf(msg, msgcap,
             "A small shed door with an honest mechanical lock. A lockpick "
             "could solve this more cleanly than brute force.");
    return 1;
  }
  if (str_ieq(id, "lantern")) {
    if (is_touch)
      snprintf(msg, msgcap,
               "The brass handle is smooth from years of use. Even unlit, it "
               "feels like a promise against the dark.");
    else
      snprintf(msg, msgcap,
               "A crafted brass lantern with clear glass and a fresh wick. Its "
               "warm light can turn dark rooms back into readable places.");
    return 1;
  }
  if (str_ieq(id, "book") || str_ieq(id, "old_books") ||
      str_ieq(id, "open_books")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The pages discuss artifacts, thresholds, and the danger of "
               "mistaking a locked door for a wall. Several notes mention the "
               "Architect and Elysium crystals.");
    else
      snprintf(msg, msgcap,
               "Leather-bound weight, cover symbols you almost recognize, pages "
               "dry enough to fear an open flame — and cramped marginalia that "
               "does not look entirely friendly.");
    return 1;
  }
  if (str_ieq(id, "engineering_tome")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The tome teaches a practical doctrine: listen for stress, "
               "respect leverage, and never trust a quiet gear that should be "
               "singing.");
    else
      snprintf(msg, msgcap,
               "An engineering tome full of hinges, mills, gear trains, and "
               "small failure diagrams. It quietly argues that every mechanism "
               "has a personality.");
    return 1;
  }
  if (str_ieq(id, "merchant_ledger")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Columns of prices, favors, and old debts. In the margins: "
               "\"Never discount in fog. Never extend credit to a masked poet. "
               "Always know the exits.\"");
    else
      snprintf(msg, msgcap,
               "A ledger of prices, favors, and old debts. The useful lesson is "
               "simple: every village has an economy before it has a hero.");
    return 1;
  }
  if (str_ieq(id, "crafting_manual")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Most pages are obvious; a few margins suggest combinations "
               "without naming them. You close it no wiser, only curious.");
    else
      snprintf(msg, msgcap,
               "A worn manual of joints and repairs. The useful parts seem "
               "to assume you already have a bench.");
    return 1;
  }
  if (str_ieq(id, "armor_crafting_manual")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Layering, rivets, padding, repair seams. The diagrams care "
               "less about glory than surviving the second hit.");
    else
      snprintf(msg, msgcap,
               "A protective-gear manual, heavy with notes on leather, plates, "
               "and why comfort matters in a long fight.");
    return 1;
  }
  if (str_ieq(id, "advanced_crafting_tome")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The advanced diagrams assume you already know the rules, then "
               "show where a master can bend them without breaking the work.");
    else
      snprintf(msg, msgcap,
               "An old tome with careful illustrations and brittle pages. It "
               "looks more like a workshop lineage than a book.");
    return 1;
  }
  if (str_ieq(id, "herbalism_guide")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The guide warns that every useful plant has at least one bad "
               "lookalike. Smell, stem, season, then cut.");
    else
      snprintf(msg, msgcap,
               "A stained field guide to herbs, roots, poultices, and the "
               "quiet discipline of not poisoning yourself.");
    return 1;
  }
  if (str_ieq(id, "linguistics_textbook")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The first lesson is not vocabulary but posture: every language "
               "protects what its speakers fear losing.");
    else
      snprintf(msg, msgcap,
               "A textbook on scripts, dialects, and translation. Several "
               "pages compare old rune families.");
    return 1;
  }
  if (str_ieq(id, "lore_scroll") || str_ieq(id, "ancient_scroll") ||
      str_ieq(id, "hidden_scroll")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The scroll tells of Veritasfurtum, where the Architect's "
               "creations still echo. It names an Empress who rules across "
               "dimensions, Nexus Points where realities converge, and "
               "Dimensional Rifts that can tear through space and time. "
               "The Cosmic Sentinel Agency watches the crossings; Universe "
               "Drops sometimes fall in. One line repeats: \"A return must be "
               "earned before it is trusted.\"");
    else
      snprintf(msg, msgcap,
               "A fragile scroll with ink that has browned at the edges. It "
               "feels ceremonial rather than decorative.");
    return 1;
  }
  if (str_ieq(id, "ancient_artifact")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Under the grime, sigils match temple rubrics for the "
               "Architect and for waystones. A marginal note warns that "
               "Hollow Ridge sits inside Veritasfurtum like a room inside a "
               "storm — stable only until something knocks.");
    else
      snprintf(msg, msgcap,
               "A heavy relic crusted with ridge dust. It hums faintly, as if "
               "remembering a larger sky.");
    return 1;
  }
  if (str_ieq(id, "miller_journal")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Brenna's hand — strong, impatient — tracks grain yields, gear "
               "stress, and river height. Margins note the '67 and '89 floods; "
               "the last pages obsess over the stuck wheel and a dream of the "
               "river \"moving wrong,\" as if Veritasfurtum were borrowing "
               "water from somewhere else.");
    else
      snprintf(msg, msgcap,
               "A leather-bound journal of daily mill operations and uneasy "
               "notes about the river.");
    return 1;
  }
  if (str_ieq(id, "sheet_candlelight_reel")) {
    if (is_read)
      snprintf(msg, msgcap,
               "A pub reel in G major — \"Candlelight Reel of Hollow Ridge.\" "
               "Tempo scratches show it was played often; someone wrote "
               "\"third night, louder\" in the margin.");
    else
      snprintf(msg, msgcap,
               "A folded music sheet marked with pub notations and tempo "
               "scratches from frequent performances.");
    return 1;
  }
  if (str_ieq(id, "tower_logbook")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The watch entries begin ordinary: weather, smoke, road traffic. "
               "Later pages mention lights moving between stones without "
               "crossing the fields.");
    else
      snprintf(msg, msgcap,
               "A leather watchtower logbook, stiff from weather and written "
               "by someone who slowly stopped trusting the horizon.");
    return 1;
  }
  if (str_ieq(id, "survival_handbook") ||
      str_ieq(id, "survival_crafting_book")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The handbook is blunt: stay dry, leave marks, ration pride, "
               "and when the trail goes silent, stop moving first.");
    else
      snprintf(msg, msgcap,
               "A practical survival book with dirt in the spine and useful "
               "corners folded down.");
    return 1;
  }
  if (str_ieq(id, "alchemy_guide")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The guide treats alchemy like cooking with consequences: heat, "
               "sequence, intention, and a clean exit route.");
    else
      snprintf(msg, msgcap,
               "A stained alchemy guide. The margins smell faintly of mint, "
               "iron, and old mistakes.");
    return 1;
  }
  if (str_ieq(id, "weapon_smithing_guide")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The smithing notes insist that a weapon's first duty is not "
               "damage. It is reliability under fear.");
    else
      snprintf(msg, msgcap,
               "A weapons guide full of balance marks, temper colors, and "
               "warning sketches of broken blades.");
    return 1;
  }
  if (str_ieq(id, "chest") || str_ieq(id, "locked_chest")) {
    snprintf(msg, msgcap,
             "A sturdy chest with enough presence to deserve its own little "
             "problem. Future lock, loot, or storage logic can hang here.");
    return 1;
  }
  if (str_ieq(id, "waystone_monolith")) {
    if (is_touch)
      snprintf(msg, msgcap,
               "The monolith hums through your palm. The vibration is not "
               "sound exactly; it is distance remembering how to fold.");
    else
      snprintf(msg, msgcap,
               "A dark monolith set with Elysium crystal. Its runes form a "
               "travel grammar: visit, attune, return.");
    return 1;
  }
  if (str_ieq(id, "nexus_monolith")) {
    snprintf(msg, msgcap,
             "A Nexus monolith, brighter and more formal than a local "
             "waystone. It feels like an address book for impossible roads.");
    return 1;
  }
  if (str_ieq(id, "elysium_crystal") || str_ieq(id, "energy_crystal")) {
    snprintf(msg, msgcap,
             "The crystal holds light the way a cup holds water. Look too long "
             "and it seems to look back through the room.");
    return 1;
  }
  if (str_ieq(id, "dimensional_runes") || str_ieq(id, "dimensional_rune") ||
      str_ieq(id, "glowing_runes")) {
    if (is_read)
      snprintf(msg, msgcap,
               "You can parse only the shape of the runes: origin, consent, "
               "destination, return. The rest resists ordinary language.");
    else
      snprintf(msg, msgcap,
               "The runes glow in measured pulses, like a machine trying to "
               "breathe politely.");
    return 1;
  }
  if (str_ieq(id, "altar_stone")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The runes describe consent, offering, and return — the same "
               "grammar as waystones, but bent toward worship. Moss keeps the "
               "stone cool; the light does not.");
    else
      snprintf(msg, msgcap,
               "An ancient altar stone under moss and runes that pulse with "
               "faint, otherworldly light.");
    return 1;
  }
  if (str_ieq(id, "ancient_carvings")) {
    if (is_read)
      snprintf(msg, msgcap,
               "You cannot read the language, but the story is clear: a "
               "journey into the Void, a Drop held together, and a figure — "
               "the Architect — who walked inside what had been made.");
    else
      snprintf(msg, msgcap,
               "Mysterious symbols carved into the rock. They seem to tell a "
               "story in a tongue long forgotten.");
    return 1;
  }
  if (str_ieq(id, "ancient_map")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The map charts Hollow Ridge and wild country beyond, but "
               "several marks sit in blank parchment — places the cartographer "
               "feared or could not see. One symbol matches temple waystone "
               "rubrics.");
    else
      snprintf(msg, msgcap,
               "A weathered map of wilderness landmarks, ink faded at the "
               "folds.");
    return 1;
  }
  if (str_ieq(id, "carved_stone")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The tablet repeats temple motifs: threshold, witness, return. "
               "A marginal scratch reads: \"Earn the road before you trust it.\"");
    else
      snprintf(msg, msgcap,
               "A small stone tablet bearing the same symbols as the temple "
               "walls.");
    return 1;
  }
  if (str_ieq(id, "cave_painting")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Crude pigments show figures at a monolith, beasts with too "
               "many joints, and a rain of colored lines — as if someone saw "
               "Universe Drops falling through the Void.");
    else
      snprintf(msg, msgcap,
               "Primitive paintings on the cave wall: strange creatures and "
               "spiral symbols.");
    return 1;
  }
  if (str_ieq(id, "grove_offering")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Inside the leaf-wrapped bundle: dried herbs, a small crystal, "
               "and a note — \"For the grove that remembers Maddeline's name. "
               "Leave nothing that cannot rot.\"");
    else
      snprintf(msg, msgcap,
               "A bundle left at the base of an ancient tree — herbs, crystal, "
               "and a folded note inside.");
    return 1;
  }
  if (str_ieq(id, "ancient_prayer_beads")) {
    snprintf(msg, msgcap,
             is_read
                 ? "Each bead warms in turn as you count them, as if approving "
                   "the rhythm of your breath."
                 : "Polished wood and tiny crystals on a string; they pulse with "
                   "gentle heat against your palm.");
    return 1;
  }
  if (str_ieq(id, "ancient_temple_ruins")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Broken columns frame a sermon in stone: the Architect made "
               "Veritasfurtum to be felt, not only ruled; rifts are scars where "
               "feeling outpaced law.");
    else
      snprintf(msg, msgcap,
               "Weathered temple ruins. Moss and runes compete for every "
               "crevice.");
    return 1;
  }
  if (str_ieq(id, "fishing_spot")) {
    snprintf(msg, msgcap,
             "A quiet patch of water where ripples cross against the current. "
             "This is exactly the sort of place that wants a fishing minigame.");
    return 1;
  }
  if (str_ieq(id, "tavern_sign")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The sign reads: THE RUSTY ANCHOR TAVERN. Smaller letters "
               "promise beds, meals, songs, and discretion.");
    else
      snprintf(msg, msgcap,
               "A weathered tavern sign. The painted anchor is rust-red now, "
               "but the place still knows how to invite travelers in.");
    return 1;
  }
  if (str_ieq(id, "signpost")) {
    if (is_read)
      snprintf(msg, msgcap,
               "One arrow points toward The Rusty Anchor Tavern. Another old "
               "cut points toward the village square, nearly swallowed by moss.");
    else
      snprintf(msg, msgcap,
               "A wooden signpost scarred by weather and pocket knives. The "
               "carving is still legible if you lean close.");
    return 1;
  }
  if (str_ieq(id, "tavern_ledger")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Tabs, room numbers, delivery notes, and a few names circled "
               "twice. The tavern survives on memory as much as coin.");
    else
      snprintf(msg, msgcap,
               "A leather tavern ledger with careful handwriting and a cover "
               "worn smooth at the corners.");
    return 1;
  }
  if (str_ieq(id, "holy_symbols") || str_ieq(id, "holy_symbol")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Architect sigils for threshold, witness, and return — the same "
               "grammar as waystones, bent toward worship. A missionary's note "
               "tucked inside reads: \"Shame is the only heresy.\"");
    else
      snprintf(msg, msgcap,
               "Symbols of the Architect on cloth and metal, warm from handling "
               "in the temple.");
    return 1;
  }
  if (str_ieq(id, "protective_charm")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The charm is braided wire and crystal shard. The inscription "
               "promises not invulnerability — only a moment to choose the "
               "wiser road.");
    else
      snprintf(msg, msgcap,
               "A small protective charm merchants prize and temples bless "
               "in passing.");
    return 1;
  }
  if (str_ieq(id, "ancient_coin")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Unknown symbols circle a profile worn smooth by pockets. The "
               "metal is too well preserved — as if Veritasfurtum forgot to "
               "age it.");
    else
      snprintf(msg, msgcap,
               "A golden coin etched with symbols no local mint admits to "
               "knowing.");
    return 1;
  }
  if (str_ieq(id, "stone_marker")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The runes shimmer: threshold, witness, return — and a warning "
               "that Hollow Ridge sits inside a larger storm. Touching them "
               "warms your palm.");
    else
      snprintf(msg, msgcap,
               "A weathered marker covered in ancient runes that seem to "
               "shimmer in the light.");
    return 1;
  }
  if (str_ieq(id, "journal")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Strange diagrams of the white house, notes on boarded doors, "
               "and a recurring sketch of runes that match the front threshold. "
               "The last entry: \"Do not trust the well after midnight.\"");
    else
      snprintf(msg, msgcap,
               "A weathered journal with leather binding and pages full of "
               "diagrams about the house.");
    return 1;
  }
  if (str_ieq(id, "old_maps")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Coastlines, trade routes, and Hollow Ridge marked with pins. "
               "Several destinations sit in blank parchment — places the "
               "cartographer feared or could not see.");
    else
      snprintf(msg, msgcap,
               "Framed maps showing coastlines, trade routes, and distant "
               "lands marked with notes.");
    return 1;
  }
  if (str_ieq(id, "sheet_last_call_etude")) {
    if (is_read)
      snprintf(msg, msgcap,
               "A dense tavern piece in B minor — \"Last Call at the Copper "
               "Cup.\" Margins warn: \"third night, play slower or chairs "
               "start flying.\"");
    else
      snprintf(msg, msgcap,
               "A music sheet copied by a veteran tavern performer — advanced, "
               "crowd-tested, unforgiving.");
    return 1;
  }
  if (str_ieq(id, "tavern_secret")) {
    if (is_read)
      snprintf(msg, msgcap,
               "A folded note: \"If Soren asks, you saw nothing. The back stair "
               "is for friends. — Silas\" A few coins remain as earnest.");
    else
      snprintf(msg, msgcap,
               "A hidden compartment behind a loose board — note and coins "
               "left by a previous patron.");
    return 1;
  }
  if (str_ieq(id, "crystal_heart") || str_ieq(id, "mysterious_gem")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Light lives inside the crystal like a second pulse. It "
               "approves of warmth and honest intent — and grows cold around "
               "malice.");
    else
      snprintf(msg, msgcap,
               "A perfect crystal containing living light. It pulses in your "
               "hand like a heartbeat.");
    return 1;
  }
  if (str_ieq(id, "buried_coin")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Tarnished but legible — mint marks from before the ridge was "
               "named. Someone hid it and never came back.");
    else
      snprintf(msg, msgcap,
               "An old coin half-buried in the dirt, tarnished but still "
               "recognizable.");
    return 1;
  }
  if (str_ieq(id, "shrine_offering")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Dried herbs, a coin, and a prayer: \"For the Architect who "
               "wanted us to feel.\" The cloth smells of rain and incense.");
    else
      snprintf(msg, msgcap,
               "A small offering left at a roadside shrine — herbs, coin, "
               "folded cloth.");
    return 1;
  }
  if (str_ieq(id, "ancient_artifacts")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Shards of pre-ridge civilization: a crown bent by rift heat, "
               "a tablet naming Maddeline before the fall, a nail that still "
               "points north toward a Nexus that is not there yet.");
    else
      snprintf(msg, msgcap,
               "A display of ancient artifacts — some beautiful, some "
               "dangerous, all expensive.");
    return 1;
  }
  if (str_ieq(id, "magical_items")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Charms, charged glass, and a vial that hums when lied to. "
               "The trader's card reads: \"Residual magic is honest magic — "
               "it admits what it was.\"");
    else
      snprintf(msg, msgcap,
               "Magical items arranged on velvet — faint glow, careful labels.");
    return 1;
  }
  if (str_ieq(id, "rare_curiosities")) {
    if (is_read)
      snprintf(msg, msgcap,
               "A music box from another Drop, a comb that remembers hair "
               "color, and a map drawn on skin-thin parchment. None of it "
               "belongs to Hollow Ridge — all of it wants to stay.");
    else
      snprintf(msg, msgcap,
               "Rare curiosities under glass — odd, lovely, and slightly wrong.");
    return 1;
  }
  if (str_ieq(id, "exotic_animals")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Care sheets in three languages. One cage holds a bird "
               "whose song makes listeners confess; the trader keeps it "
               "covered for everyone's safety.");
    else
      snprintf(msg, msgcap,
               "Exotic animals in clean cages — colorful, restless, and "
               "clearly well fed.");
    return 1;
  }
  if (str_ieq(id, "pet_cages") || str_ieq(id, "pet_supplies")) {
    snprintf(msg, msgcap,
             is_read
                 ? "Feed charts, muzzle sizes, and a note: \"Match the pet to "
                   "the owner's courage, not their purse.\""
                 : "Cages and supplies for creatures stranger than village dogs.");
    return 1;
  }
  if (str_ieq(id, "bookshelves") || str_ieq(id, "open_books")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Histories of Amethystus, grammars of old runes, and a romance "
               "novel someone shelved upside down on purpose. The useful "
               "margins point toward the grove altar.");
    else
      snprintf(msg, msgcap,
               "Floor-to-ceiling bookshelves — dust, leather, and the "
               "promise of secrets.");
    return 1;
  }
  if (str_ieq(id, "fountain") || str_ieq(id, "crystal_pool")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The water is clear enough to shame a mirror. Reflected light "
               "has no source; drinking might heal — or invite something "
               "that watches through your eyes.");
    else
      snprintf(msg, msgcap,
               "Water so clear it seems to glow from within, reflecting "
               "lights that have no source.");
    return 1;
  }
  if (str_ieq(id, "number_plates")) {
    if (is_read)
      snprintf(msg, msgcap,
               "House numbers from a village that moved. One plate reads "
               "a street that no longer exists on any map in this build.");
    else
      snprintf(msg, msgcap,
               "Ceramic number plates stacked in a crate — relics of "
               "rearranged roads.");
    return 1;
  }
  if (str_ieq(id, "obsidian_shard")) {
    if (is_read)
      snprintf(msg, msgcap,
               "The shard drinks light. Held long enough, you see a Drop "
               "floating in void — then your reflection returns, older.");
    else
      snprintf(msg, msgcap,
               "A shard of obsidian so black it feels wet. Or a gem that "
               "catches light wrong.");
    return 1;
  }
  if (str_ieq(id, "buried_treasure")) {
    if (is_read)
      snprintf(msg, msgcap,
               "Someone buried coins and a protective charm beneath the "
               "roots. The charm's note: \"For whoever digs with kind hands.\"");
    else
      snprintf(msg, msgcap,
               "A hidden cache beneath tree roots — earth still soft from "
               "recent digging.");
    return 1;
  }
  if (str_ieq(id, "gem") || str_ieq(id, "gold_coin")) {
    if (is_read)
      snprintf(msg, msgcap,
               str_ieq(id, "gem")
                   ? "Facets hold a cold fire. Merchants will haggle; temples "
                     "will bless; thieves will memorize your face."
                   : "Standard Amethystus mint — honest weight, honest metal. "
                     "Still, every coin remembers every hand.");
    else
      snprintf(msg, msgcap,
               str_ieq(id, "gem") ? "A cut gem that catches the light."
                                  : "A gold coin, warm from trade.");
    return 1;
  }
  if (str_ieq(id, "spyglass")) {
    if (is_touch)
      snprintf(msg, msgcap,
               "The brass tube is cold and precisely weighted.");
    else
      snprintf(msg, msgcap,
               "A watchtower spyglass. Through it, roads become arguments "
               "about distance.");
    return 1;
  }
  if (str_ieq(id, "signal_fire_pit") || str_ieq(id, "signal_flags")) {
    snprintf(msg, msgcap,
             "Signal gear for turning danger into distance. With the right "
             "system, this could call help or trouble from far away.");
    return 1;
  }
  if (str_ieq(id, "compass")) {
    snprintf(msg, msgcap,
             "A brass compass with a steady needle. It cannot tell you what is "
             "wise, but it can keep north from becoming a rumor.");
    return 1;
  }
  if (str_ieq(id, "tinderbox")) {
    snprintf(msg, msgcap,
             "Flint, steel, and dry tinder in a tight little box. Fire, if you "
             "respect the order of operations.");
    return 1;
  }
  if (str_ieq(id, "oil_flask")) {
    snprintf(msg, msgcap,
             "Clear lamp oil in thick glass. It smells faintly sharp and would "
             "matter a great deal to a thirsty lantern.");
    return 1;
  }
  if (str_ieq(id, "herbs")) {
    snprintf(msg, msgcap,
             "A small bundle of herbs. Some smell medicinal, some culinary, "
             "and one is probably pretending to be both.");
    return 1;
  }
  if (str_ieq(id, "incense_burner")) {
    if (is_touch)
      snprintf(msg, msgcap,
               "Brass warm from recent use; ash clings in the bowl like grey "
               "snow.");
    else if (is_read)
      snprintf(msg, msgcap,
               "A temple-style burner: vent holes spell witness in an old "
               "script. The web edition kept these near confessionals — smoke "
               "as punctuation, not disguise.");
    else
      snprintf(msg, msgcap,
               "A brass incense burner, recently used. The scent of myrrh and "
               "stone still clings.");
    return 1;
  }
  if (str_ieq(id, "pickled_vegetables")) {
    snprintf(msg, msgcap,
             is_read
                 ? "The brine recipe is village-practical: vinegar, dill, and "
                   "a pinch of temple salt. Someone wrote \"for winter mouths\" "
                   "on the lid."
                 : "A crock of pickled vegetables — sharp, honest, and meant to "
                   "survive long cellar nights.");
    return 1;
  }
  if (str_ieq(id, "dried_meat")) {
    snprintf(msg, msgcap,
             is_read
                 ? "No label — only a butcher's mark and a date scratched in "
                   "grease. Travel rations from the web road menus, adapted here "
                   "as tavern stock."
                 : "Strips of dried meat, salted and smoked. Trail food that "
                   "refuses to apologize.");
    return 1;
  }
  if (str_ieq(id, "tavern_piano")) {
    if (is_touch)
      snprintf(msg, msgcap,
               "The keys dip under your fingers with a tired but honest action.");
    else
      snprintf(msg, msgcap,
               "A sturdy upright piano with worn keys. Use  play piano  for "
               "the rhythm performance.");
    return 1;
  }
  if (strstr(id, "music_sheet") != NULL) {
    if (is_read)
      snprintf(msg, msgcap,
               "The notation is compact and practical: tavern tempo marks, "
               "repeat scratches, and a melody meant to survive noise.");
    else
      snprintf(msg, msgcap,
               "A folded music sheet, creased from use and still ready for a "
               "player with steady hands.");
    return 1;
  }
  if (str_ieq(id, "bucket")) {
    if (is_read)
      snprintf(
          msg, msgcap,
          "There is no inscription. If you stare long enough, you remember "
          "corridors, a confident voice, and an object that refused to be "
          "mere set dressing.");
    else if (is_touch)
      snprintf(msg, msgcap,
               "The handle is cool and ordinary — and entirely too sure of "
               "existing. Your fingers report: bucket.");
    else
      snprintf(
          msg, msgcap,
          "A simple metal bucket. It has a certain presence. You find yourself "
          "inexplicably drawn to it.");
    return 1;
  }
  {
    const char *catalog_desc = aet_item_catalog_description_for_slug(id);
    if (catalog_desc) {
      snprintf(msg, msgcap, "%s", catalog_desc);
      return 1;
    }
  }
  return 0;
}

static int disambig_try_followup(const char *line, char *msg, size_t msgcap,
                                 int *turn_advance) {
  char slug[MAX_ITEM_LEN];
  int act;
  if (!line || !line[0] || g_disambig_pick_n <= 0 || !msg || !turn_advance)
    return 0;
  if (!disambig_pick_slug(line, slug, sizeof slug)) return 0;
  act = g_disambig_act;
  disambig_remember(g_disambig_query, slug);
  disambig_clear();
  switch (act) {
  case DISAMBIG_ACT_TAKE:
    *turn_advance = take_item(slug, msg, msgcap) ? 1 : 0;
    return 1;
  case DISAMBIG_ACT_DROP:
    *turn_advance = drop_item(slug, msg, msgcap) ? 1 : 0;
    return 1;
  case DISAMBIG_ACT_EXAMINE:
    *turn_advance = 0;
    examine_target(slug, msg, msgcap);
    return 1;
  case DISAMBIG_ACT_EQUIP:
    *turn_advance = 0;
    cmd_equip(slug, msg, msgcap);
    return 1;
  case DISAMBIG_ACT_BUY:
    *turn_advance = 0;
    cmd_trade_buy(slug, msg, msgcap);
    return 1;
  case DISAMBIG_ACT_SELL:
    *turn_advance = 0;
    cmd_trade_sell(slug, msg, msgcap);
    return 1;
  default:
    return 0;
  }
}

static void cmd_price_compare(const char *rest, char *body, size_t cap) {
  char work[256];
  char qnorm[MAX_ITEM_LEN];
  char banner[256];
  AetPcSave pc;
  int mc, i, hits = 0;
  int best_buy = -1, worst_buy = -1;
  if (!body || cap < 128) return;
  strncpy(work, rest, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_trailing_space(work);
  strip_leading_articles(work);
  if (!work[0]) {
    snprintf(body, cap,
             "Compare prices for what?\n\n"
             "Examples:\n"
             "  price compare rope\n"
             "  compare prices torch\n"
             "  market compare wheat\n");
    return;
  }
  query_norm_underscore(qnorm, sizeof qnorm, work);
  pc_capture(&pc);
  pc_fill_narrative_defaults(&pc);
  pc_format_identity_banner(banner, sizeof banner);
  mc = aet_merchant_count();
  snprintf(body, cap,
           "=== PRICE COMPARISON ===\n\n"
           "%s\n\n"
           "Buy prices for \"%s\" (your CHA %d and patron standing per shop):\n\n",
           banner, work, pc.cha);
  for (i = 0; i < mc; i++) {
    const char *mslug = aet_merchant_slug_at(i);
    const AetMerchantTable *mt;
    const AetMerchantOffer *o;
    char pretty[96];
    int rep0;
    if (!mslug || !mslug[0]) continue;
    mt = aet_merchant_trades(mslug);
    if (!mt) continue;
    rep0 = merchant_rep_score(i);
    entity_pretty(mslug, pretty, sizeof pretty);
    for (o = mt->stock; o->item && o->item[0]; o++) {
      int pay;
      char payb[24], listb[24], itempretty[96];
      if (!merchant_offer_matches_query(o->item, work, qnorm)) continue;
      pay = merchant_adjust_buy_price(o->price, rep0, pc.cha);
      currency_format_compact(pay, payb, sizeof payb);
      currency_format_compact(o->price, listb, sizeof listb);
      item_pretty(o->item, itempretty, sizeof itempretty);
      body_append(body, cap, "  %-22s  %-24s  pay %s", pretty, itempretty, payb);
      if (pay != o->price)
        body_append(body, cap, "  (list %s)", listb);
      body_append(body, cap, "  — %s\n", merchant_rep_tier_label(rep0));
      hits++;
      if (best_buy < 0 || pay < best_buy) best_buy = pay;
      if (worst_buy < 0 || pay > worst_buy) worst_buy = pay;
    }
  }
  if (!hits) {
    snprintf(body, cap,
             "=== PRICE COMPARISON ===\n\n"
             "%s\n\n"
             "No merchant stocks a match for \"%s\" in world data.\n"
             "Try a shorter name (rope, torch, wheat) or  wares  at a trader.\n",
             banner, work);
    return;
  }
  if (best_buy >= 0 && worst_buy >= 0 && best_buy != worst_buy) {
    char lo[24], hi[24];
    currency_format_compact(best_buy, lo, sizeof lo);
    currency_format_compact(worst_buy, hi, sizeof hi);
    body_append(body, cap,
                "\nSpread: %s (cheapest listed) to %s (priciest listed) for this "
                "query.\n",
                lo, hi);
  }
  body_append(body, cap,
              "\nVisit the trader and use  wares  or  buy <item>  to purchase.\n");
}

static void examine_target_mode(const char *raw, const char *mode, char *msg,
                                size_t msgcap) {
  char q[256];
  char qnorm[MAX_ITEM_LEN];
  int ordinal = 0, exclude_last = 0;
  int i;
  const char *slug;

  strncpy(q, raw, sizeof q - 1);
  q[sizeof q - 1] = '\0';
  strip_trailing_space(q);
  strip_leading_articles(q);
  if (!q[0]) {
    snprintf(msg, msgcap, "Examine what?");
    return;
  }
  if (str_ieq(q, "self") || str_ieq(q, "me") || str_ieq(q, "myself") ||
      str_ieq(q, "player") || str_ieq(q, "pc") || str_ieq(q, "my character")) {
    char sum[1024];
    char exxp[4096];
    const char *ex = aet_mods_character_examine_suffix();
    pc_format_summary(sum, sizeof sum);
    exxp[0] = '\0';
    if (ex && ex[0]) expand_mod_overlay_flat(ex, exxp, sizeof exxp);
    msg[0] = '\0';
    if (world_room_is_dark(g_room) && !player_has_light_source()) {
      body_append(msg, msgcap,
                  "In the dark you take stock by touch and habit — enough to "
                  "remember who you are.\n\n%s",
                  sum);
      if (exxp[0]) body_append(msg, msgcap, "\n\n%s", exxp);
    } else {
      body_append(msg, msgcap,
                  "%s\n\n"
                  "(Go deeper:  identity  or  character brief  — same compact "
                  "sheet;  character  or  sheet  — full portrait.)",
                  sum);
      if (exxp[0]) body_append(msg, msgcap, "\n\n%s", exxp);
    }
    return;
  }
  {
    int prep = parser_prepare_object_query(q, q, sizeof q, &ordinal,
                                           &exclude_last, msg, msgcap);
    if (prep < 0) return;
    if (!q[0]) {
      snprintf(msg, msgcap, "Examine what?");
      return;
    }
  }
  query_norm_underscore(qnorm, sizeof qnorm, q);

  {
    char cid[MAX_ITEM_LEN];
    int cix = -1;
    if (container_resolve_here(g_room, q, cid, sizeof cid, &cix) && cix >= 0) {
      container_examine(cix, msg, msgcap);
      return;
    }
  }

  if (world_room_is_dark(g_room) && !player_has_light_source()) {
    for (i = 0; i < g_inv_n; i++) {
      if (room_item_matches_query(g_inv[i], q, qnorm)) {
        remember_focus_item(g_inv[i]);
        if (content_item_response(g_inv[i], mode, msg, msgcap)) return;
        snprintf(msg, msgcap,
                 "You inspect %.40s by touch in your pack. (Still legible in "
                 "the dark.)",
                 g_inv[i]);
        return;
      }
    }
    snprintf(msg, msgcap,
             "Too dark to make out details here. Try a light source or wait "
             "until you leave.");
    return;
  }

  slug = world_slug(g_room);
  if (slug[0] && strcmp(slug, "west_of_house") == 0 &&
      (str_ieq(qnorm, "door") || strstr(qnorm, "house") != NULL ||
       strstr(qnorm, "front_door") != NULL)) {
    snprintf(msg, msgcap,
             "The white house dominates the east, its entrance boarded tight. "
             "A weathered mailbox waits nearby.");
    return;
  }
  if (slug[0] && strcmp(slug, "front_door") == 0 &&
      (str_ieq(q, "door") || str_ieq(qnorm, "door") ||
       strstr(qnorm, "keyhole") != NULL)) {
    snprintf(msg, msgcap,
             "Heavy planks and faintly glowing runes around the frame. The "
             "keyhole seems to wait for something specific.");
    return;
  }
  if (slug[0] && strcmp(slug, "east_of_house") == 0 &&
      (strstr(qnorm, "shed") != NULL || str_ieq(qnorm, "shed"))) {
    snprintf(msg, msgcap,
             "A small outbuilding. The door looks pickable if you have the "
             "right tools.");
    return;
  }

  {
    char picks[DISAMBIG_PICK_MAX][MAX_ITEM_LEN];
    int pn = 0, j;
    for (i = 0; i < g_room_item_n[g_room]; i++) {
      if (exclude_last && g_last_focus[0] &&
          str_ieq(g_room_items[g_room][i], g_last_focus))
        continue;
      if (!room_item_matches_query(g_room_items[g_room][i], q, qnorm)) continue;
      for (j = 0; j < pn; j++)
        if (str_ieq(picks[j], g_room_items[g_room][i])) break;
      if (j < pn) continue;
      if (pn < DISAMBIG_PICK_MAX) {
        copy_capped(picks[pn], sizeof picks[0], g_room_items[g_room][i]);
        pn++;
      }
    }
    for (i = 0; i < g_inv_n; i++) {
      if (exclude_last && g_last_focus[0] && str_ieq(g_inv[i], g_last_focus))
        continue;
      if (!room_item_matches_query(g_inv[i], q, qnorm)) continue;
      for (j = 0; j < pn; j++)
        if (str_ieq(picks[j], g_inv[i])) break;
      if (j < pn) continue;
      if (pn < DISAMBIG_PICK_MAX) {
        copy_capped(picks[pn], sizeof picks[0], g_inv[i]);
        pn++;
      }
    }
    container_append_open_contents_picks(g_room, q, qnorm, picks, &pn,
                                         exclude_last);
    if (pn == 0) {
      snprintf(msg, msgcap, "You do not see anything like that here.");
      return;
    }
    if (pn == 1) {
      char pretty[96];
      remember_focus_item(picks[0]);
      if (content_item_response(picks[0], mode, msg, msgcap)) return;
      item_pretty(picks[0], pretty, sizeof pretty);
      if (inv_has(picks[0]))
        snprintf(msg, msgcap,
                 "You inspect %.40s in your pack. It is exactly what you are "
                 "carrying.",
                 pretty);
      else
        snprintf(msg, msgcap, "You look closely at %.40s.", pretty);
      disambig_remember(q, picks[0]);
      return;
    }
    if (pn > 1 && ordinal >= 1 && ordinal <= pn) {
      examine_target_mode(picks[ordinal - 1], mode, msg, msgcap);
      return;
    }
    {
      char slug[MAX_ITEM_LEN];
      int r;
      g_disambig_next_act = DISAMBIG_ACT_EXAMINE;
      r = disambig_resolve_ambiguous(DISAMBIG_ACT_EXAMINE, 0, q, picks, pn, slug,
                                     sizeof slug, msg, msgcap);
      if (r == 1) {
        examine_target_mode(slug, mode, msg, msgcap);
        return;
      }
      return;
    }
  }
}

static void examine_target(const char *raw, char *msg, size_t msgcap) {
  examine_target_mode(raw, "examine", msg, msgcap);
}

static void read_target(const char *raw, char *msg, size_t msgcap) {
  char id[MAX_ITEM_LEN];
  int r = resolve_visible_item(raw, id, sizeof id, msg, msgcap);
  if (r != 1) return;
#ifdef AETER_MINIGAMES
  game_open_reader(id, msg, msgcap);
#else
  examine_target_mode(raw, "read", msg, msgcap);
#endif
}

static void touch_target(const char *raw, char *msg, size_t msgcap) {
  examine_target_mode(raw, "touch", msg, msgcap);
}

#ifdef AETER_MINIGAMES
static int content_item_read_body(const char *id, char *out, size_t outcap) {
  return content_item_response(id, "read", out, outcap);
}

static int game_read_resolve(const char *item_id, MgtReadDocument *doc) {
  if (!item_id || !item_id[0] || !doc) return 0;
  memset(doc, 0, sizeof *doc);
  if (aet_mods_item_description(item_id, doc->body, sizeof doc->body) &&
      doc->body[0]) {
    /* mod item text is the readable body */
  } else if (!content_item_read_body(item_id, doc->body, sizeof doc->body) ||
             !doc->body[0]) {
    return 0;
  }
  item_pretty(item_id, doc->title, sizeof doc->title);
  snprintf(doc->source_id, sizeof doc->source_id, "%s", item_id);
  doc->kind = mgt_read_classify_source_id(item_id);
  doc->valid = 1;
  return 1;
}

static int game_open_reader(const char *item_id, char *msg, size_t msgcap) {
  MgtReadDocument doc;
  if (!game_read_resolve(item_id, &doc)) {
    snprintf(msg, msgcap, "You cannot read anything meaningful there.");
    return 0;
  }
  mgt_read_set_document(&doc);
  if (try_minigame("reading", msg, msgcap)) return 1;
  mgt_read_clear_document();
  snprintf(msg, msgcap, "You finish reading %s.", doc.title);
  return 0;
}
#endif

static int resolve_visible_item(const char *raw, char *id_out, size_t idcap,
                                char *msg, size_t msgcap) {
  char work[256];
  char qnorm[MAX_ITEM_LEN];
  char picks[DISAMBIG_PICK_MAX][MAX_ITEM_LEN];
  int ordinal = 0, exclude_last = 0;
  int i, j, pn = 0, prep;

  if (!id_out || idcap < 2) return 0;
  id_out[0] = '\0';
  prep = parser_prepare_object_query(raw, work, sizeof work, &ordinal,
                                     &exclude_last, msg, msgcap);
  if (prep < 0) {
    if (msg && msgcap && !msg[0])
      snprintf(msg, msgcap, "You have nothing in mind to read yet.");
    return 0;
  }
  if (!work[0]) {
    snprintf(msg, msgcap, "Read what?");
    return 0;
  }
  query_norm_underscore(qnorm, sizeof qnorm, work);

  if (world_room_is_dark(g_room) && !player_has_light_source()) {
    for (i = 0; i < g_inv_n; i++) {
      if (!room_item_matches_query(g_inv[i], work, qnorm)) continue;
      remember_focus_item(g_inv[i]);
      copy_capped(id_out, idcap, g_inv[i]);
      return 1;
    }
    snprintf(msg, msgcap,
             "Too dark to read anything here. Try a light source or an item "
             "already in your pack.");
    return 0;
  }

  for (i = 0; i < g_room_item_n[g_room]; i++) {
    if (exclude_last && g_last_focus[0] &&
        str_ieq(g_room_items[g_room][i], g_last_focus))
      continue;
    if (!room_item_matches_query(g_room_items[g_room][i], work, qnorm)) continue;
    for (j = 0; j < pn; j++)
      if (str_ieq(picks[j], g_room_items[g_room][i])) break;
    if (j < pn) continue;
    if (pn < DISAMBIG_PICK_MAX) {
      copy_capped(picks[pn], sizeof picks[0], g_room_items[g_room][i]);
      pn++;
    }
  }
  for (i = 0; i < g_inv_n; i++) {
    if (exclude_last && g_last_focus[0] && str_ieq(g_inv[i], g_last_focus))
      continue;
    if (!room_item_matches_query(g_inv[i], work, qnorm)) continue;
    for (j = 0; j < pn; j++)
      if (str_ieq(picks[j], g_inv[i])) break;
    if (j < pn) continue;
    if (pn < DISAMBIG_PICK_MAX) {
      copy_capped(picks[pn], sizeof picks[0], g_inv[i]);
      pn++;
    }
  }
  if (pn == 0) {
    snprintf(msg, msgcap, "You do not see anything readable like that here.");
    return 0;
  }
  if (pn == 1) {
    remember_focus_item(picks[0]);
    copy_capped(id_out, idcap, picks[0]);
    disambig_remember(work, picks[0]);
    return 1;
  }
  if (ordinal >= 1 && ordinal <= pn) {
    remember_focus_item(picks[ordinal - 1]);
    copy_capped(id_out, idcap, picks[ordinal - 1]);
    disambig_remember(work, picks[ordinal - 1]);
    return 1;
  }
  {
    char slug[MAX_ITEM_LEN];
    int r;
    g_disambig_next_act = DISAMBIG_ACT_EXAMINE;
    r = disambig_resolve_ambiguous(DISAMBIG_ACT_EXAMINE, 0, work, picks, pn,
                                     slug, sizeof slug, msg, msgcap);
    if (r == 1) {
      copy_capped(id_out, idcap, slug);
      remember_focus_item(slug);
      return 1;
    }
    return -1;
  }
}

static int try_direction_parse(const char *tok, char *msg, size_t msgcap) {
  int dir;
  if (!parse_direction_token(tok, &dir)) return 0;
  try_move(dir, msg, msgcap);
  return 1;
}

static int try_parse_two_token_move(char *line, char *msg, size_t msgcap,
                                    int *turn_advance) {
  char *t1, *t2, *t3;
  long n;
  char *e;
  int dir, used;
  t1 = strtok(line, " ");
  t2 = strtok(NULL, " ");
  t3 = strtok(NULL, " ");
  if (!t1 || !t2 || t3) return 0;
  n = strtol(t1, &e, 10);
  if (e > t1 && *e == '\0' && n >= 1 && n <= MAX_AUTO_STEPS &&
      parse_direction_token(t2, &dir)) {
    (void)try_move_n_steps(dir, (int)n, msg, msgcap, &used);
    *turn_advance = used;
    return 1;
  }
  n = strtol(t2, &e, 10);
  if (e > t2 && *e == '\0' && n >= 1 && n <= MAX_AUTO_STEPS &&
      parse_direction_token(t1, &dir)) {
    (void)try_move_n_steps(dir, (int)n, msg, msgcap, &used);
    *turn_advance = used;
    return 1;
  }
  return 0;
}

static void normalize_aliases(char *line) {
  static const struct {
    const char *from;
    size_t from_len;
    const char *to;
    size_t to_len;
  } tab[] = {{"retrieve ", 9, "take ", 5}, {"acquire ", 8, "take ", 5},
             {"obtain ", 7, "take ", 5},     {"snatch ", 7, "take ", 5},
             {"snag ", 5, "take ", 5},       {"lift ", 5, "take ", 5},
             {"pluck ", 6, "take ", 5},      {"recover ", 8, "take ", 5},
             {"pocket ", 7, "take ", 5},
             {"scoop ", 6, "take ", 5},
             {"procure ", 8, "take ", 5},    {"collect ", 8, "take ", 5},
             {"withdraw ", 9, "take ", 5},   {"pick up ", 8, "take ", 5},
             {"pickup ", 7, "take ", 5},    {"fetch ", 6, "take ", 5},
             {"grab ", 5, "take ", 5},       {"get ", 4, "take ", 5},
             {"deposit ", 8, "drop ", 5},    {"stash ", 6, "drop ", 5},
             {"discard ", 8, "drop ", 5},    {"remove ", 7, "drop ", 5},
             {"place ", 6, "drop ", 5},
             {"put down ", 9, "drop ", 5},
             {"taste ", 6, "eat ", 4},       {"sip ", 4, "drink ", 6},
             {NULL, 0, NULL, 0}};
  int k;
  for (k = 0; tab[k].from; k++) {
    if (!strncmp(line, tab[k].from, tab[k].from_len)) {
      size_t rest = strlen(line + tab[k].from_len);
      memmove(line + tab[k].to_len, line + tab[k].from_len, rest + 1);
      memcpy(line, tab[k].to, tab[k].to_len);
      return;
    }
  }
}

static void strip_leading_spaces(char *s) {
  while (s[0] == ' ')
    memmove(s, s + 1, strlen(s + 1) + 1);
}

static void strip_trailing_politeness(char *line) {
  int guard = 0;
  static const char *const tails[] = {
      ", thank you", ", thanks", ", please", " please", " pls", " plz",
      " thanks",    " thank you", " ok",      " okay",  NULL};
  while (guard++ < 8) {
    size_t len = strlen(line);
    int i, trimmed = 0;
    while (len > 0 && line[len - 1] == ' ') line[--len] = '\0';
    for (i = 0; tails[i]; i++) {
      size_t tlen = strlen(tails[i]);
      if (len >= tlen && !strcmp(line + len - tlen, tails[i])) {
        line[len - tlen] = '\0';
        trimmed = 1;
        break;
      }
    }
    if (!trimmed) break;
  }
}

static void strip_motion_verb(char *line) {
  static const char *verbs[] = {"walk ",   "run ",    "step ",   "stroll ",
                                 "hurry ", "rush ",   "sneak ",  "creep ",
                                 "head ",  "march ",  "ascend ", "descend ",
                                 "move ",  "travel ", "proceed ",
                                 NULL};
  int guard = 0;
  while (guard++ < 8) {
    int i;
    int hit = 0;
    strip_leading_spaces(line);
    for (i = 0; verbs[i]; i++) {
      size_t n = strlen(verbs[i]);
      if (!strncmp(line, verbs[i], n)) {
        memmove(line, line + n, strlen(line + n) + 1);
        hit = 1;
        break;
      }
    }
    if (!hit) break;
  }
  strip_leading_spaces(line);
}

static void squeeze_spaces(char *s) {
  char *r = s, *w = s;
  int last_space = 1;
  while (*r) {
    unsigned char c = (unsigned char)*r++;
    if (isspace(c)) {
      if (!last_space) *w++ = ' ';
      last_space = 1;
    } else {
      *w++ = (char)c;
      last_space = 0;
    }
  }
  while (w > s && w[-1] == ' ') w--;
  *w = '\0';
}

static void strip_terminal_punctuation(char *s) {
  size_t n;
  squeeze_spaces(s);
  n = strlen(s);
  while (n > 0 && (s[n - 1] == '?' || s[n - 1] == '!' || s[n - 1] == '.' ||
                   s[n - 1] == '"' || s[n - 1] == '\'')) {
    s[--n] = '\0';
  }
  while (s[0] == '"' || s[0] == '\'')
    memmove(s, s + 1, strlen(s + 1) + 1);
  squeeze_spaces(s);
}

static void rewrite_command(char *line, const char *verb, const char *rest) {
  char tmp[INPUT_LINE_MAX];
  while (rest && *rest == ' ') rest++;
  if (!rest) rest = "";
  if (rest[0])
    snprintf(tmp, sizeof tmp, "%s %.*s", verb, INPUT_LINE_MAX - 64, rest);
  else
    snprintf(tmp, sizeof tmp, "%s", verb);
  strncpy(line, tmp, INPUT_LINE_MAX - 1);
  line[INPUT_LINE_MAX - 1] = '\0';
  strip_terminal_punctuation(line);
}

static int rewrite_to_move_or_approach(char *line, const char *rest) {
  int dir;
  char tmp[INPUT_LINE_MAX];
  while (rest && *rest == ' ') rest++;
  if (!rest || !rest[0]) return 0;
  strncpy(tmp, rest, sizeof tmp - 1);
  tmp[sizeof tmp - 1] = '\0';
  strip_leading_articles(tmp);
  strip_terminal_punctuation(tmp);
  if (parse_direction_token(tmp, &dir))
    rewrite_command(line, "go", tmp);
  else
    rewrite_command(line, "approach", tmp);
  return 1;
}

static void split_preposition_phrase(char *text, const char **prep_out,
                                     char **rhs_out) {
  static const char *preps[] = {" with ", " using ", " on ", " onto ",
                                " into ", " in ",    " at ", " to ", " from ",
                                NULL};
  int i;
  *prep_out = NULL;
  *rhs_out = NULL;
  for (i = 0; preps[i]; i++) {
    char *p = strstr(text, preps[i]);
    if (!p) continue;
    *p = '\0';
    *prep_out = preps[i];
    *rhs_out = p + strlen(preps[i]);
    strip_trailing_space(text);
    while (**rhs_out == ' ') (*rhs_out)++;
    return;
  }
}

typedef struct {
  const char *alias;
  size_t alias_len;
  const char *verb;
  const char *rest;
} ParserExactRewrite;

typedef struct {
  const char *prefix;
  size_t prefix_len;
  const char *verb;
  int require_more_text;
} ParserPrefixRewrite;

#define EXACT_RW(a, v, r) {(a), sizeof(a) - 1, (v), (r)}
#define PREFIX_RW(p, v, req) {(p), sizeof(p) - 1, (v), (req)}

static int parser_apply_exact_rewrites(char *line, const ParserExactRewrite *tab) {
  size_t line_len = strlen(line);
  int i;
  for (i = 0; tab[i].alias; i++) {
    if (line_len != tab[i].alias_len) continue;
    if (!strcmp(line, tab[i].alias)) {
      rewrite_command(line, tab[i].verb, tab[i].rest ? tab[i].rest : "");
      return 1;
    }
  }
  return 0;
}

static int parser_apply_prefix_rewrites(char *line, const ParserPrefixRewrite *tab) {
  int i;
  for (i = 0; tab[i].prefix; i++) {
    size_t n = tab[i].prefix_len;
    if (strncmp(line, tab[i].prefix, n) != 0) continue;
    if (tab[i].require_more_text && !line[n]) continue;
    rewrite_command(line, tab[i].verb, line + n);
    return 1;
  }
  return 0;
}

static int parser_rewrite_question_examine(char *line) {
  static const char *const kQ[] = {"what are ", "what is ", "who are ", "who is ",
                                   "what's ", NULL};
  int i;
  for (i = 0; kQ[i]; i++) {
    size_t n = strlen(kQ[i]);
    char *p;
    if (strncmp(line, kQ[i], n) != 0) continue;
    
    p = line + n;
    while (*p == ' ') p++;
    if (!strncmp(p, "is ", 3))
      p += 3;
    else if (!strncmp(p, "are ", 4))
      p += 4;
    while (*p == ' ') p++;
    rewrite_command(line, "examine", p);
    return 1;
  }
  return 0;
}

static int parser_rewrite_describe_examine(char *line) {
  static const char *const kVerb[] = {"describe ", "observe ", "check ", "study ",
                                      "view ", "peruse ", "survey ", "glance at ",
                                      "skim over ", "skim ", NULL};
  int i;
  for (i = 0; kVerb[i]; i++) {
    size_t n = strlen(kVerb[i]);
    char *p;
    if (strncmp(line, kVerb[i], n) != 0) continue;
    p = line + n;
    while (*p == ' ') p++;
    rewrite_command(line, "examine", p);
    return 1;
  }
  return 0;
}

static int parser_rewrite_look_preposition(char *line) {
  static const char *const kLook[] = {"look through ", "look inside ", "look into ",
                                      "look in ", "peer at ",   "gaze at ",
                                      "stare at ", NULL};
  int i;
  for (i = 0; kLook[i]; i++) {
    size_t n = strlen(kLook[i]);
    char *p;
    if (strncmp(line, kLook[i], n) != 0) continue;
    p = line + n;
    while (*p == ' ') p++;
    rewrite_command(line, "examine", p);
    return 1;
  }
  return 0;
}

static int parser_interaction_rest_dir_or_examine(char *line, const char *rest) {
  char work[INPUT_LINE_MAX];
  char tok[64];
  const char *p;
  size_t tn;
  int dir;
  if (!rest || !rest[0]) return 0;
  strncpy(work, rest, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  strip_leading_articles(work);
  strip_trailing_space(work);
  squeeze_spaces(work);
  p = work;
  while (*p == ' ') p++;
  if (!*p) return 0;
  tn = 0;
  while (p[tn] && p[tn] != ' ' && tn + 1 < sizeof tok) {
    tok[tn] = p[tn];
    tn++;
  }
  tok[tn] = '\0';
  if (p[tn]) {
    size_t j = tn;
    while (p[j] == ' ') j++;
    if (p[j] != '\0') {
      rewrite_command(line, "examine", rest);
      return 1;
    }
  }
  if (parse_direction_token(tok, &dir))
    rewrite_command(line, tok, "");
  else
    rewrite_command(line, "examine", rest);
  return 1;
}

static int parser_rewrite_interaction_phrases(char *line) {
  static const char *const kExamineOnly[] = {
      "give me a look at ",
      "have a look at ",
      "take a look at ",
      "look closer at ",
      "let me see ",
      "show me ",
      "tell me more about ",
      "i want to know about ",
      "get a closer look at ",
      NULL};
  static const char *const kInteract[] = {"interact with ", "engage with ",
                                            "deal with ", NULL};
  int i;
  for (i = 0; kExamineOnly[i]; i++) {
    size_t n = strlen(kExamineOnly[i]);
    const char *r;
    if (strncmp(line, kExamineOnly[i], n) != 0) continue;
    r = line + n;
    while (*r == ' ') r++;
    if (!*r) return 0;
    rewrite_command(line, "examine", r);
    return 1;
  }
  if (!strncmp(line, "handle ", 7)) {
    const char *r = line + 7;
    while (*r == ' ') r++;
    if (!*r) return 0;
    return parser_interaction_rest_dir_or_examine(line, r);
  }
  for (i = 0; kInteract[i]; i++) {
    size_t n = strlen(kInteract[i]);
    const char *r;
    if (strncmp(line, kInteract[i], n) != 0) continue;
    r = line + n;
    while (*r == ' ') r++;
    if (!*r) return 0;
    return parser_interaction_rest_dir_or_examine(line, r);
  }
  if (!strncmp(line, "discuss with ", 13)) {
    const char *r = line + 13;
    while (*r == ' ') r++;
    if (!*r) return 0;
    rewrite_command(line, "talk to", r);
    return 1;
  }
  if (!strncmp(line, "discuss about ", 14)) {
    const char *r = line + 14;
    while (*r == ' ') r++;
    if (!*r) return 0;
    rewrite_command(line, "talk about", r);
    return 1;
  }
  if (!strncmp(line, "discuss ", 8)) {
    const char *r = line + 8;
    while (*r == ' ') r++;
    if (!*r) return 0;
    rewrite_command(line, "talk about", r);
    return 1;
  }
  return 0;
}

static int parser_rewrite_protective_grab(char *line) {
  const char *rest = NULL;
  if (!strncmp(line, "take ", 5))
    rest = line + 5;
  else if (!strncmp(line, "hold ", 5))
    rest = line + 5;
  else if (!strncmp(line, "reach for ", 10))
    rest = line + 10;
  else if (!strncmp(line, "catch ", 6))
    rest = line + 6;
  if (!rest) return 0;
  while (*rest == ' ') rest++;
  if (!protective_phrase_is_candidate(rest)) return 0;
  rewrite_command(line, "protective grab", rest);
  return 1;
}

static int parser_rewrite_talk_bare_pronoun(char *line) {
  if (!strncmp(line, "talk ", 5) && strncmp(line, "talk to ", 8) &&
      strncmp(line, "talk about ", 11)) {
    char *p = line + 5;
    while (*p == ' ') p++;
    strip_leading_articles(p);
    if (parser_query_is_npc_pronoun(p) && !strchr(p, ' ')) {
      rewrite_command(line, "talk to", p);
      return 1;
    }
  }
  return 0;
}

static int parser_rewrite_speak_to(char *line) {
  static const char *const kSpeak[] = {
      "converse with ", "speak with ", "chat with ", "talk with ",
      "speak to ",     "chat to ",    NULL};
  int i;
  for (i = 0; kSpeak[i]; i++) {
    size_t n = strlen(kSpeak[i]);
    char *p;
    if (strncmp(line, kSpeak[i], n) != 0) continue;
    p = line + n;
    while (*p == ' ') p++;
    rewrite_command(line, "talk to", p);
    return 1;
  }
  return 0;
}

static int parser_rewrite_ask_about(char *line) {
  if (!strncmp(line, "ask ", 4)) {
    char work[INPUT_LINE_MAX];
    char *about;
    strncpy(work, line + 4, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    about = strstr(work, " about ");
    if (about) {
      *about = '\0';
      strip_trailing_space(work);
      snprintf(line, INPUT_LINE_MAX, "talk to %.*s about %.*s", 180, work, 240,
               about + 7);
      strip_terminal_punctuation(line);
      return 1;
    }
  }
  return 0;
}

static int parser_rewrite_unlock_open_use(char *line) {
  const char *prep;
  char *rhs;
  size_t ou_sk = 0;
  if (!strncmp(line, "unlock ", 7))
    ou_sk = 7;
  else if (!strncmp(line, "open ", 5))
    ou_sk = 5;
  if (ou_sk) {
    char work[INPUT_LINE_MAX];
    char usebuf[INPUT_LINE_MAX];
    char *target = line + ou_sk;
    strncpy(work, target, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    split_preposition_phrase(work, &prep, &rhs);
    if (rhs && rhs[0]) {
      if (strstr(rhs, "lockpick") || strstr(rhs, "pick"))
        rewrite_command(line, "use", "lockpick");
      else if (work[0])
        snprintf(usebuf, sizeof usebuf, "%s on %s", rhs, work);
      else
        snprintf(usebuf, sizeof usebuf, "%s", rhs);
      rewrite_command(line, "use", usebuf);
      return 1;
    }
  }
  {
    size_t ua_sk = 0;
    if (!strncmp(line, "apply ", 6))
      ua_sk = 6;
    else if (!strncmp(line, "use ", 4))
      ua_sk = 4;
    if (ua_sk) {
      char work[INPUT_LINE_MAX];
      char usebuf[INPUT_LINE_MAX];
      char *tool = line + ua_sk;
      strncpy(work, tool, sizeof work - 1);
      work[sizeof work - 1] = '\0';
      split_preposition_phrase(work, &prep, &rhs);
      if (rhs && rhs[0]) {
        snprintf(usebuf, sizeof usebuf, "%s on %s", work, rhs);
        rewrite_command(line, "use", usebuf);
        return 1;
      }
      if (!strncmp(line, "apply ", 6)) {
        rewrite_command(line, "use", tool);
        return 1;
      }
    }
  }
  if (!strncmp(line, "pick ", 5) && (strstr(line, "lock") || strstr(line, "shed"))) {
    rewrite_command(line, "pick lock", "");
    return 1;
  }
  if (!strncmp(line, "lockpick ", 9)) {
    rewrite_command(line, "use", "lockpick");
    return 1;
  }
  return 0;
}

static int parser_rewrite_take_drop_put(char *line) {
  const char *prep;
  char *rhs;
  {
    const char *verb = NULL;
    if (!strncmp(line, "take ", 5))
      verb = "take";
    else if (!strncmp(line, "drop ", 5))
      verb = "drop";
    if (verb) {
      char work[INPUT_LINE_MAX];
      char *item = line + 5;
      strncpy(work, item, sizeof work - 1);
      work[sizeof work - 1] = '\0';
      split_preposition_phrase(work, &prep, &rhs);
      if (rhs && rhs[0] && parser_rhs_is_surface(rhs)) {
        rewrite_command(line, verb, work);
        return 1;
      }
    }
  }
  if (!strncmp(line, "put ", 4)) {
    char work[INPUT_LINE_MAX];
    strncpy(work, line + 4, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    split_preposition_phrase(work, &prep, &rhs);
    if (rhs && rhs[0]) {
      if (strstr(rhs, "door") || strstr(rhs, "keyhole") || strstr(rhs, "lock"))
        rewrite_command(line, "use", work);
      else if (prep && (strstr(prep, " in ") || strstr(prep, " into ") ||
                        strstr(prep, " on ")))
        return 0;
      else
        rewrite_command(line, "drop", work);
      return 1;
    }
  }
  return 0;
}

static int parser_rewrite_note_prefixes(char *line) {
  static const char *const kNotePrefix[] = {"remember that ", "jot down ", "jot ",
                                            "memo ", NULL};
  int i;
  for (i = 0; kNotePrefix[i]; i++) {
    size_t n = strlen(kNotePrefix[i]);
    if (!strncmp(line, kNotePrefix[i], n)) {
      rewrite_command(line, "note", line + n);
      return 1;
    }
  }
  return 0;
}

static int parser_rewrite_plain_english_queries(char *line) {
  static const struct {
    const char *topic;
    const char *verb;
    const char *rest;
  } topics[] = {
      {"inventory", "inventory", ""}, {"pack", "inventory", ""},
      {"bag", "inventory", ""},       {"stuff", "inventory", ""},
      {"items", "inventory", ""},     {"belongings", "inventory", ""},
      {"inventory list", "inventory list", ""},
      {"pack list", "inventory list", ""},
      {"inventory sort name", "inventory sort", ""},
      {"gear", "gear", ""},           {"equipment", "equipment", ""},
      {"loadout", "loadout", ""},     {"health", "vitals", ""},
      {"hp", "vitals", ""},           {"status", "status", ""},
      {"stats", "aptitudes", ""},     {"skills", "aptitudes", ""},
      {"abilities", "aptitudes", ""}, {"objectives", "objectives", ""},
      {"quests", "journal", ""},      {"journal", "journal", ""},
      {"notes", "notes", ""},         {"map", "nearby", ""},
      {"location", "where", ""},      {"surroundings", "look", ""},
      {"room", "describe", ""},       {"exits", "exits", ""},
      {"weather", "weather", ""},     {"time", "time", ""},
      {NULL, NULL, NULL}};
  static const char *const wrappers[] = {
      "check my ",       "check the ",      "check ",
      "show me my ",    "show my ",        "show me the ",
      "show me ",       "show the ",       "show ",
      "look at my ",    "look at the ",    "look at ",
      "look in my ",    "look in the ",    "look in ",
      "open my ",       "open the ",       "open ",
      "review my ",     "review the ",     "review ",
      "read my ",       "read the ",       "read ",
      "tell me my ",    "tell me the ",    "tell me ",
      "what is my ",    "what's my ",      "whats my ",
      "what are my ",   "where is my ",    "where's my ",
      "wheres my ",     NULL};
  int wi, ti;
  for (wi = 0; wrappers[wi]; wi++) {
    size_t wn = strlen(wrappers[wi]);
    const char *r;
    char topic[96];
    size_t rn, i;
    if (strncmp(line, wrappers[wi], wn) != 0) continue;
    r = line + wn;
    while (!strncmp(r, "own ", 4)) r += 4;
    if (!strncmp(r, "current ", 8)) r += 8;
    while (!strncmp(r, "the ", 4) || !strncmp(r, "my ", 3)) {
      if (!strncmp(r, "the ", 4)) r += 4;
      else r += 3;
    }
    rn = 0;
    while (r[rn] && r[rn] != ' ' && rn + 1 < sizeof topic) {
      topic[rn] = r[rn];
      rn++;
    }
    topic[rn] = '\0';
    if (r[rn]) {
      const char *tail = r + rn;
      while (*tail == ' ') tail++;
      if (!strcmp(tail, "screen") || !strcmp(tail, "menu") ||
          !strcmp(tail, "panel")) {
        
      } else if (rn + 1 < sizeof topic) {
        topic[rn++] = ' ';
        for (i = 0; tail[i] && rn + 1 < sizeof topic; i++) {
          if (tail[i] == ' ') break;
          topic[rn++] = tail[i];
        }
        topic[rn] = '\0';
      }
    }
    for (ti = 0; topics[ti].topic; ti++) {
      if (!strcmp(topic, topics[ti].topic)) {
        rewrite_command(line, topics[ti].verb, topics[ti].rest);
        return 1;
      }
    }
  }
  if (!strcmp(line, "where am i at") || !strcmp(line, "where i am") ||
      !strcmp(line, "where am i standing") ||
      !strcmp(line, "tell me where i am") ||
      !strcmp(line, "can you tell me where i am")) {
    rewrite_command(line, "where", "");
    return 1;
  }
  if (!strcmp(line, "look around here") || !strcmp(line, "look round") ||
      !strcmp(line, "look all around")) {
    rewrite_command(line, "look", "");
    return 1;
  }
  if (!strcmp(line, "what is here") || !strcmp(line, "what's here") ||
      !strcmp(line, "whats here") || !strcmp(line, "what is in the room") ||
      !strcmp(line, "what is in here") || !strcmp(line, "what's in the room") ||
      !strcmp(line, "what's in here")) {
    rewrite_command(line, "scan", "");
    return 1;
  }
  if (!strcmp(line, "what can i take") || !strcmp(line, "what is on the ground") ||
      !strcmp(line, "what's on the ground") ||
      !strcmp(line, "what is on the floor") ||
      !strcmp(line, "what's on the floor")) {
    rewrite_command(line, "loot", "");
    return 1;
  }
  return 0;
}

static int parser_rewrite_what_in_container(char *line) {
  static const char *const pfx[] = {
      "what is inside the ", "what's inside the ", "whats inside the ",
      "what is inside ",     "what's inside ",     "whats inside ",
      "what is in the ",     "what's in the ",     "whats in the ",
      "what is in ",         "what's in ",         "whats in ",
      NULL};
  int pi;
  char rest[INPUT_LINE_MAX];
  for (pi = 0; pfx[pi]; pi++) {
    size_t pn = strlen(pfx[pi]);
    const char *r;
    if (strncmp(line, pfx[pi], pn) != 0) continue;
    r = line + pn;
    while (*r == ' ') r++;
    if (!*r) return 0;
    strncpy(rest, r, sizeof rest - 1);
    rest[sizeof rest - 1] = '\0';
    strip_terminal_punctuation(rest);
    strip_leading_articles(rest);
    if (!rest[0]) return 0;
    rewrite_command(line, "examine", rest);
    return 1;
  }
  return 0;
}

static int parser_rewrite_is_there(char *line) {
  static const char *const pfx[] = {"is there a ", "is there an ", "is there ",
                                    "are there any ", "are there ", NULL};
  static const char *const tails[] = {" here", " in the room", " in here",
                                      " around here", " nearby", NULL};
  int pi, ti;
  for (pi = 0; pfx[pi]; pi++) {
    size_t pn = strlen(pfx[pi]);
    char item[INPUT_LINE_MAX];
    const char *rest;
    if (strncmp(line, pfx[pi], pn) != 0) continue;
    rest = line + pn;
    strncpy(item, rest, sizeof item - 1);
    item[sizeof item - 1] = '\0';
    strip_terminal_punctuation(item);
    for (ti = 0; tails[ti]; ti++) {
      size_t tn = strlen(tails[ti]);
      size_t il = strlen(item);
      if (il >= tn && !strcmp(item + il - tn, tails[ti])) {
        item[il - tn] = '\0';
        strip_trailing_space(item);
        break;
      }
    }
    if (!item[0]) return 0;
    rewrite_command(line, "find", item);
    return 1;
  }
  return 0;
}

static int parser_rewrite_movement_phrases(char *line) {
  static const struct {
    const char *pfx;
    unsigned char len;
  } kDirect[] = {
      {"go to ", 6},   {"go into ", 8}, {"go in ", 6}, {"to ", 3},
  };
  size_t di;
  for (di = 0; di < sizeof kDirect / sizeof kDirect[0]; di++) {
    if (!strncmp(line, kDirect[di].pfx, kDirect[di].len)) {
      (void)rewrite_to_move_or_approach(line, line + kDirect[di].len);
      return 1;
    }
  }
  if (!strncmp(line, "travel to ", 10) || !strncmp(line, "move to ", 8) ||
      !strncmp(line, "head to ", 8) || !strncmp(line, "proceed to ", 11)) {
    char *p = strstr(line, " to ");
    if (p && rewrite_to_move_or_approach(line, p + 4)) return 1;
  }
  return 0;
}

static int parser_rewrite_go_through_where_find(char *line) {
  if (!strncmp(line, "go through ", 11)) {
    rewrite_command(line, "open", line + 11);
    return 1;
  }
  if (!strncmp(line, "where is ", 9) || !strncmp(line, "where are ", 10)) {
    rewrite_command(line, "find", strchr(line, ' ') + 4);
    return 1;
  }
  return 0;
}

static int parser_rewrite_mailbox_search_find(char *line) {
  if (!strncmp(line, "open mailbox", 12) ||
      !strncmp(line, "open the mailbox", 16))
    return 0;
  if (!strncmp(line, "search ", 7) && strcmp(line, "search")) {
    rewrite_command(line, "find", line + 7);
    return 1;
  }
  return 0;
}

static int parser_rewrite_go_single_direction(char *line) {
  const char *p;
  char tok[64];
  size_t tn;
  int dir;
  if (strncmp(line, "go ", 3) != 0) return 0;
  p = line + 3;
  while (*p == ' ') p++;
  if (!*p) return 0;
  tn = 0;
  while (p[tn] && p[tn] != ' ' && tn + 1 < sizeof tok) {
    tok[tn] = p[tn];
    tn++;
  }
  tok[tn] = '\0';
  if (p[tn]) {
    size_t j = tn;
    while (p[j] == ' ') j++;
    if (p[j] != '\0') return 0;
  }
  if (!parse_direction_token(tok, &dir)) return 0;
  rewrite_command(line, tok, "");
  return 1;
}

static void normalize_parser_intent(char *line) {
  static const ParserPrefixRewrite kPrefixRewrite[] = {
      PREFIX_RW("can you show me what's in ", "examine", 1),
      PREFIX_RW("can you show me what is in ", "examine", 1),
      PREFIX_RW("show me what's in ", "examine", 1),
      PREFIX_RW("show me what is in ", "examine", 1),
      PREFIX_RW("could you show me what's in ", "examine", 1),
      PREFIX_RW("could you show me what is in ", "examine", 1),
      PREFIX_RW("would you show me what's in ", "examine", 1),
      PREFIX_RW("would you show me what is in ", "examine", 1),
      PREFIX_RW("can you check what's in ", "examine", 1),
      PREFIX_RW("can you check what is in ", "examine", 1),
      PREFIX_RW("i want to see what's in ", "examine", 1),
      PREFIX_RW("i want to see what is in ", "examine", 1),
      PREFIX_RW("what's in ", "examine", 1),
      PREFIX_RW("whats in ", "examine", 1),
      PREFIX_RW("what is in ", "examine", 1),
      PREFIX_RW("what's on ", "examine", 1),
      PREFIX_RW("whats on ", "examine", 1),
      PREFIX_RW("could you tell me who is here", "who", 0),
      PREFIX_RW("can you tell me who is here", "who", 0),
      PREFIX_RW("who's here", "who", 0),
      PREFIX_RW("whos here", "who", 0),
      PREFIX_RW("where can i go from here", "exits", 0),
      PREFIX_RW("where can i go next", "exits", 0),
      PREFIX_RW("show me where to find ", "find", 1),
      PREFIX_RW("could i get a look at ", "look at", 1),
      PREFIX_RW("can you tell me about ", "examine", 1),
      PREFIX_RW("can i get a look at ", "look at", 1),
      PREFIX_RW("point me towards ", "find", 1),
      PREFIX_RW("where can i find ", "find", 1),
      PREFIX_RW("where do i find ", "find", 1),
      PREFIX_RW("point me to ", "find", 1),
      PREFIX_RW("where were ", "find", 1),
      PREFIX_RW("where was ", "find", 1),
      PREFIX_RW("learn more about ", "examine", 1),
      PREFIX_RW("make conversation with ", "talk to", 1),
      PREFIX_RW("have a word with ", "talk to", 1),
      PREFIX_RW("say hello to ", "talk to", 1),
      PREFIX_RW("negotiate with ", "talk to", 1),
      PREFIX_RW("haggle over ", "haggle buy ", 1),
      PREFIX_RW("barter for ", "haggle buy ", 1),
      PREFIX_RW("negotiate price for ", "haggle ", 1),
      PREFIX_RW("figure out ", "examine", 1),
      PREFIX_RW("work out ", "examine", 1),
      PREFIX_RW("can i see ", "look at", 1),
      PREFIX_RW("peek at ", "look at", 0),
      PREFIX_RW("fast travel to ", "fasttravel", 0),
      PREFIX_RW("fast travel ", "fasttravel", 0),
      PREFIX_RW("fast-travel to ", "fasttravel", 0),
      PREFIX_RW("waystone to ", "waystone", 0),
      PREFIX_RW("nexus to ", "nexus", 0),
      PREFIX_RW("search for ", "find", 0),
      PREFIX_RW("find me ", "find", 0),
      PREFIX_RW("tell me about ", "examine", 0),
      PREFIX_RW("look for ", "find", 0),
      PREFIX_RW("ask about ", "talk about", 0),
      PREFIX_RW("speak about ", "talk about", 1),
      PREFIX_RW("chat about ", "talk about", 1),
      PREFIX_RW("whisper to ", "talk to", 1),
      PREFIX_RW("complain to ", "talk to", 1),
      PREFIX_RW("scream ", "shout", 1),
      {NULL, 0, NULL, 0}};
  static const ParserExactRewrite kExactRewrite[] = {
      EXACT_RW("im stuck", "hints", ""),
      EXACT_RW("i am stuck", "hints", ""),
      EXACT_RW("i'm stuck", "hints", ""),
      EXACT_RW("give me a hint", "hints", ""),
      EXACT_RW("give me hint", "hints", ""),
      EXACT_RW("nudge", "hints", ""),
      EXACT_RW("i need a hint", "hints", ""),
      EXACT_RW("another hint", "hints", ""),
      EXACT_RW("peek", "look", ""),
      EXACT_RW("glance", "look", ""),
      EXACT_RW("what changed last turn", "causality lastturn", ""),
      EXACT_RW("what changed", "causality lastturn", ""),
      EXACT_RW("what happened last turn", "causality lastturn", ""),
      EXACT_RW("why did that happen", "causality recent", ""),
      EXACT_RW("why did this happen", "causality recent", ""),
      EXACT_RW("why did that occur", "causality recent", ""),
      EXACT_RW("why did npc react", "causality social", ""),
      EXACT_RW("why npc reacted", "causality social", ""),
      EXACT_RW("why did they react", "causality social", ""),
      EXACT_RW("what triggered that", "causality explain", ""),
      EXACT_RW("what caused that", "causality explain", ""),
      EXACT_RW("explain what happened", "causality explain", ""),
      EXACT_RW("who heard me", "causality social", ""),
      EXACT_RW("did anyone hear me", "causality social", ""),
      EXACT_RW("who noticed me", "noise", ""),
      EXACT_RW("did anyone notice me", "noise", ""),
      EXACT_RW("what heard that noise", "noise", ""),
      EXACT_RW("why didnt save work", "causality save", ""),
      EXACT_RW("why didn't save work", "causality save", ""),
      EXACT_RW("why load failed", "causality save", ""),
      EXACT_RW("why parser failed", "causality parser", ""),
      EXACT_RW("why command failed to parse", "causality parser", ""),
      EXACT_RW("why did movement fail", "why blocked", ""),
      EXACT_RW("why cant i go", "why blocked", ""),
      EXACT_RW("why can't i go", "why blocked", ""),
      EXACT_RW("what can i do", "help", ""),
      EXACT_RW("what should i do", "help", ""),
      EXACT_RW("what can you do", "help", ""),
      EXACT_RW("what should i do next", "objectives", ""),
      EXACT_RW("what now", "objectives", ""),
      EXACT_RW("commands", "help", ""),
      EXACT_RW("crafting", "forge", ""),
      EXACT_RW("open forge", "forge", ""),
      EXACT_RW("material forge", "forge", ""),
      EXACT_RW("forge menu", "forge", ""),
      EXACT_RW("play piano", "play piano", ""),
      EXACT_RW("play the piano", "play piano", ""),
      EXACT_RW("perform on piano", "play piano", ""),
      EXACT_RW("go fishing", "fish", ""),
      EXACT_RW("tend farm", "farm", ""),
      EXACT_RW("work kitchen", "cook", ""),
      EXACT_RW("play dice", "gamble", ""),
      EXACT_RW("play cards", "gamble", ""),
      EXACT_RW("track game", "hunt", ""),
      EXACT_RW("instructions", "help", ""),
      EXACT_RW("how do i play", "help", ""),
      EXACT_RW("show help", "help", ""),
      EXACT_RW("keyboard shortcuts", "help", ""),
      EXACT_RW("game controls", "help", ""),
      EXACT_RW("modding guide", "help modding", ""),
      EXACT_RW("mod guide", "help modding", ""),
      EXACT_RW("mods guide", "help modding", ""),
      EXACT_RW("dlc guide", "help modding", ""),
      EXACT_RW("dlc help", "help modding", ""),
      EXACT_RW("who made this", "about", ""),
      EXACT_RW("developers", "about", ""),
      EXACT_RW("attribution", "about", ""),
      EXACT_RW("world lore", "lore", ""),
      EXACT_RW("tell me about veritasfurtum", "lore veritasfurtum", ""),
      EXACT_RW("what is veritasfurtum", "lore veritasfurtum", ""),
      EXACT_RW("who is the architect", "lore architect", ""),
      EXACT_RW("what is hollow ridge", "lore hollow", ""),
      EXACT_RW("universe drops", "lore cosmology", ""),
      EXACT_RW("cosmic sentinel agency", "lore csa", ""),
      EXACT_RW("identity", "character brief", ""),
      EXACT_RW("who am i", "character", ""),
      EXACT_RW("whoami", "character", ""),
      EXACT_RW("what do i look like", "character", ""),
      EXACT_RW("describe me", "character", ""),
      EXACT_RW("describe myself", "character", ""),
      EXACT_RW("my appearance", "character", ""),
      EXACT_RW("appearance", "character", ""),
      EXACT_RW("my character", "character", ""),
      EXACT_RW("show character", "character", ""),
      EXACT_RW("show me my character", "character", ""),
      EXACT_RW("look at me", "character", ""),
      EXACT_RW("look at myself", "character", ""),
      EXACT_RW("my skills", "aptitudes", ""),
      EXACT_RW("show skills", "aptitudes", ""),
      EXACT_RW("show my skills", "aptitudes", ""),
      EXACT_RW("what are my skills", "aptitudes", ""),
      EXACT_RW("what skills do i have", "aptitudes", ""),
      EXACT_RW("my aptitudes", "aptitudes", ""),
      EXACT_RW("show aptitudes", "aptitudes", ""),
      EXACT_RW("attribute points", "aptitudes", ""),
      EXACT_RW("my stats", "aptitudes", ""),
      EXACT_RW("stat block", "aptitudes", ""),
      EXACT_RW("power profile", "aptitudes", ""),
      EXACT_RW("combat readiness", "aptitudes", ""),
      EXACT_RW("build summary", "aptitudes", ""),
      EXACT_RW("risk profile", "aptitudes", ""),
      EXACT_RW("character archetype", "aptitudes", ""),
      EXACT_RW("my reputation", "reputation", ""),
      EXACT_RW("show reputation", "reputation", ""),
      EXACT_RW("my standing", "reputation", ""),
      EXACT_RW("show standing", "reputation", ""),
      EXACT_RW("what is my reputation", "reputation", ""),
      EXACT_RW("my rep", "reputation", ""),
      EXACT_RW("show rep", "reputation", ""),
      EXACT_RW("patron standing", "reputation", ""),
      EXACT_RW("merchant standing", "reputation", ""),
      EXACT_RW("my gear", "gear", ""),
      EXACT_RW("show gear", "gear", ""),
      EXACT_RW("show equipment", "equipment", ""),
      EXACT_RW("my equipment", "equipment", ""),
      EXACT_RW("my loadout", "loadout", ""),
      EXACT_RW("show loadout", "loadout", ""),
      EXACT_RW("what am i wearing", "loadout", ""),
      EXACT_RW("my traits", "traits", ""),
      EXACT_RW("show traits", "traits", ""),
      EXACT_RW("my personality", "traits", ""),
      EXACT_RW("my momentum", "momentum", ""),
      EXACT_RW("show momentum", "momentum", ""),
      EXACT_RW("how am i doing", "momentum", ""),
      EXACT_RW("am i making progress", "momentum", ""),
      EXACT_RW("story so far", "momentum", ""),
      EXACT_RW("my arc", "momentum", ""),
      EXACT_RW("how far have i come", "momentum", ""),
      EXACT_RW("my perks", "perks", ""),
      EXACT_RW("show perks", "perks", ""),
      EXACT_RW("list perks", "perks", ""),
      EXACT_RW("what perks do i have", "perks", ""),
      EXACT_RW("my voice", "voice", ""),
      EXACT_RW("show voice", "voice", ""),
      EXACT_RW("how do i sound", "voice", ""),
      EXACT_RW("my pronouns", "voice", ""),
      EXACT_RW("what are my pronouns", "voice", ""),
      EXACT_RW("voice style", "voice", ""),
      EXACT_RW("my bio", "bio", ""),
      EXACT_RW("show bio", "bio", ""),
      EXACT_RW("my backstory", "bio", ""),
      EXACT_RW("tell me my story", "bio", ""),
      EXACT_RW("my biography", "bio", ""),
      EXACT_RW("my corruption", "tainting", ""),
      EXACT_RW("show corruption", "tainting", ""),
      EXACT_RW("am i tainted", "tainting", ""),
      EXACT_RW("how corrupted am i", "tainting", ""),
      EXACT_RW("moral vector", "tainting", ""),
      EXACT_RW("alignment drift", "tainting", ""),
      EXACT_RW("social stance", "rapport", ""),
      EXACT_RW("relationship history", "relationship history", ""),
      EXACT_RW("bond history", "relationship history", ""),
      EXACT_RW("social history", "relationship history", ""),
      EXACT_RW("who matters here", "rapport", ""),
      EXACT_RW("who can i talk to", "rapport", ""),
      EXACT_RW("npc list", "rapport", ""),
      EXACT_RW("list npcs", "rapport", ""),
      EXACT_RW("people in the world", "rapport", ""),
      EXACT_RW("logbook", "journal", ""),
      EXACT_RW("questlog", "journal", ""),
      EXACT_RW("quest log", "journal", ""),
      EXACT_RW("diary", "journal", ""),
      EXACT_RW("adventure log", "journal", ""),
      EXACT_RW("adventure journal", "journal", ""),
      EXACT_RW("my journal", "journal", ""),
      EXACT_RW("story journal", "journal", ""),
      EXACT_RW("my notebook", "notes", ""),
      EXACT_RW("open my notes", "notes", ""),
      EXACT_RW("note list", "notes", ""),
      EXACT_RW("read my notes", "notes", ""),
      EXACT_RW("exploration", "progress", ""),
      EXACT_RW("exploration summary", "progress", ""),
      EXACT_RW("world progress", "progress", ""),
      EXACT_RW("sitrep", "progress", ""),
      EXACT_RW("situation report", "progress", ""),
      EXACT_RW("how far am i", "progress", ""),
      EXACT_RW("story progress", "progress", ""),
      EXACT_RW("where am i in the story", "progress", ""),
      EXACT_RW("what just happened", "recap", ""),
      EXACT_RW("recent messages", "recap", ""),
      EXACT_RW("command recap", "recap", ""),
      EXACT_RW("active quests", "objectives", ""),
      EXACT_RW("current objectives", "objectives", ""),
      EXACT_RW("what are my objectives", "objectives", ""),
      EXACT_RW("story goals", "objectives", ""),
      EXACT_RW("checklist", "objectives", ""),
      EXACT_RW("my checklist", "objectives", ""),
      EXACT_RW("my hp", "vitals", ""),
      EXACT_RW("how hurt am i", "vitals", ""),
      EXACT_RW("am i hurt", "vitals", ""),
      EXACT_RW("my health", "vitals", ""),
      EXACT_RW("mod order", "mods list", ""),
      EXACT_RW("mods order", "mods list", ""),
      EXACT_RW("shopping list", "wares", ""),
      EXACT_RW("merchant stock", "wares", ""),
      EXACT_RW("shop stock", "wares", ""),
      EXACT_RW("whats for sale", "wares", ""),
      EXACT_RW("what is for sale", "wares", ""),
      EXACT_RW("market", "wares", ""),
      EXACT_RW("open the shop", "wares", ""),
      EXACT_RW("what is for sale here", "wares", ""),
      EXACT_RW("bargain", "haggle", ""),
      EXACT_RW("barter", "haggle", ""),
      EXACT_RW("haggle", "haggle", ""),
      EXACT_RW("negotiate price", "haggle", ""),
      EXACT_RW("buy everything", "buy all", ""),
      EXACT_RW("purchase everything", "purchase all", ""),
      EXACT_RW("sell everything", "sell all", ""),
      EXACT_RW("trade log", "trade history", ""),
      EXACT_RW("transaction log", "trade history", ""),
      EXACT_RW("transaction history", "trade history", ""),
      EXACT_RW("history trade", "trade history", ""),
      EXACT_RW("command history", "history", ""),
      EXACT_RW("cmd history", "history", ""),
      EXACT_RW("history list", "history", ""),
      EXACT_RW("room utilities", "utilities", ""),
      EXACT_RW("room objects", "utilities", ""),
      EXACT_RW("utility objects", "utilities", ""),
      EXACT_RW("save game", "save", ""),
      EXACT_RW("load game", "load", ""),
      EXACT_RW("quick save", "qs", ""),
      EXACT_RW("quick load", "ql", ""),
      EXACT_RW("save games", "saves", ""),
      EXACT_RW("my saves", "saves", ""),
      EXACT_RW("load slots", "saves", ""),
      EXACT_RW("troubleshoot", "errors", ""),
      EXACT_RW("engine diagnostics", "errors", ""),
      EXACT_RW("session log", "errors", ""),
      EXACT_RW("what went wrong", "errors", ""),
      EXACT_RW("debug log", "errors", ""),
      EXACT_RW("travel network", "waypoints", ""),
      EXACT_RW("teleport network", "waypoints", ""),
      EXACT_RW("nexus list", "waypoints", ""),
      EXACT_RW("attuned points", "waypoints", ""),
      EXACT_RW("breadcrumbs", "trail", ""),
      EXACT_RW("my path back", "trail", ""),
      EXACT_RW("movement trail", "trail", ""),
      EXACT_RW("adjacent rooms", "nearby", ""),
      EXACT_RW("local map", "nearby", ""),
      EXACT_RW("around here", "nearby", ""),
      EXACT_RW("lock status", "lockcheck", ""),
      EXACT_RW("door locks", "lockcheck", ""),
      EXACT_RW("detection risk", "noise", ""),
      EXACT_RW("how stealthy", "noise", ""),
      EXACT_RW("who is here", "who", ""),
      EXACT_RW("who else is here", "who", ""),
      EXACT_RW("anyone here", "who", ""),
      EXACT_RW("npc role", "who", ""),
      EXACT_RW("who is this npc", "who", ""),
      EXACT_RW("last npc role", "who", ""),
      EXACT_RW("who is the last npc", "who", ""),
      EXACT_RW("last npc attitude", "who", ""),
      EXACT_RW("npc attitude", "who", ""),
      EXACT_RW("npc danger", "who", ""),
      EXACT_RW("last npc danger", "who", ""),
      EXACT_RW("how dangerous is this npc", "who", ""),
      EXACT_RW("npc trust", "who", ""),
      EXACT_RW("last npc trust", "who", ""),
      EXACT_RW("npc leverage", "who", ""),
      EXACT_RW("last npc leverage", "who", ""),
      EXACT_RW("how much leverage do i have", "who", ""),
      EXACT_RW("conversation topic", "topic", ""),
      EXACT_RW("what did we talk about", "topic", ""),
      EXACT_RW("topic mood", "topic", ""),
      EXACT_RW("conversation mood", "topic", ""),
      EXACT_RW("topic heat", "topic", ""),
      EXACT_RW("conversation heat", "topic", ""),
      EXACT_RW("is last npc here", "who", ""),
      EXACT_RW("last npc here", "who", ""),
      EXACT_RW("who did i last meet", "who", ""),
      EXACT_RW("last person i met", "who", ""),
      EXACT_RW("salvage", "loot", ""),
      EXACT_RW("valuables here", "loot", ""),
      EXACT_RW("i", "inventory", ""),
      EXACT_RW("show inventory", "inventory", ""),
      EXACT_RW("show my inventory", "inventory", ""),
      EXACT_RW("show me inventory", "inventory", ""),
      EXACT_RW("show me my inventory", "inventory", ""),
      EXACT_RW("what am i carrying", "inventory", ""),
      EXACT_RW("what do i have", "inventory", ""),
      EXACT_RW("belongings", "inventory", ""),
      EXACT_RW("my stuff", "inventory", ""),
      EXACT_RW("what i have on me", "inventory", ""),
      EXACT_RW("read room", "describe", ""),
      EXACT_RW("read the room", "describe", ""),
      EXACT_RW("describe here", "describe", ""),
      EXACT_RW("surroundings", "describe", ""),
      EXACT_RW("full room", "describe", ""),
      EXACT_RW("room description", "describe", ""),
      EXACT_RW("what do i see", "scan", ""),
      EXACT_RW("what can i see", "scan", ""),
      EXACT_RW("scan room", "scan", ""),
      EXACT_RW("scan the room", "scan", ""),
      EXACT_RW("area scan", "scan", ""),
      EXACT_RW("survey area", "scan", ""),
      EXACT_RW("survey room", "scan", ""),
      EXACT_RW("recon", "scan", ""),
      EXACT_RW("recon pass", "scan", ""),
      EXACT_RW("scout", "scan", ""),
      EXACT_RW("scout area", "scan", ""),
      EXACT_RW("room overview", "scan", ""),
      EXACT_RW("tactical readout", "scan", ""),
      EXACT_RW("show exits", "exits", ""),
      EXACT_RW("list exits", "exits", ""),
      EXACT_RW("where can i go", "exits", ""),
      EXACT_RW("where to", "exits", ""),
      EXACT_RW("connections", "exits", ""),
      EXACT_RW("room connections", "exits", ""),
      EXACT_RW("where am i", "where", ""),
      EXACT_RW("where am i now", "where", ""),
      EXACT_RW("show status", "status", ""),
      EXACT_RW("my status", "status", ""),
      EXACT_RW("quick status", "status", ""),
      EXACT_RW("full status", "status", ""),
      EXACT_RW("character status", "status", ""),
      EXACT_RW("threat posture", "status", ""),
      EXACT_RW("show score", "score", ""),
      EXACT_RW("my score", "score", ""),
      EXACT_RW("my points", "score", ""),
      EXACT_RW("game stats", "score", ""),
      EXACT_RW("what time is it", "time", ""),
      EXACT_RW("what's the time", "time", ""),
      EXACT_RW("whats the time", "time", ""),
      EXACT_RW("what's the weather", "weather", ""),
      EXACT_RW("whats the weather", "weather", ""),
      EXACT_RW("how's the weather", "weather", ""),
      EXACT_RW("hows the weather", "weather", ""),
      EXACT_RW("climate", "weather", ""),
      EXACT_RW("scene tone", "weather", ""),
      EXACT_RW("atmosphere", "weather", ""),
      EXACT_RW("light sources", "lights", ""),
      EXACT_RW("show lights", "lights", ""),
      EXACT_RW("fast travel", "fasttravel", ""),
      EXACT_RW("fast-travel", "fasttravel", ""),
      EXACT_RW("travel mood", "progress", ""),
      {NULL, 0, NULL, NULL}};
  strip_terminal_punctuation(line);
  strip_trailing_politeness(line);
  strip_terminal_punctuation(line);
  if (!line[0]) return;

  if (parser_rewrite_note_prefixes(line)) return;
  if (parser_rewrite_plain_english_queries(line)) return;
  if (parser_rewrite_is_there(line)) return;
  if (parser_apply_exact_rewrites(line, kExactRewrite)) return;
  if (parser_apply_prefix_rewrites(line, kPrefixRewrite)) return;

  if (parser_rewrite_movement_phrases(line)) return;
  if (parser_rewrite_go_through_where_find(line)) return;

  if (parser_rewrite_question_examine(line)) return;
  if (parser_rewrite_describe_examine(line)) return;
  if (parser_rewrite_look_preposition(line)) return;
  if (parser_rewrite_what_in_container(line)) return;
  if (parser_rewrite_interaction_phrases(line)) return;
  if (parser_rewrite_protective_grab(line)) return;
  if (parser_rewrite_mailbox_search_find(line)) return;

  if (parser_rewrite_speak_to(line)) return;
  if (parser_rewrite_talk_bare_pronoun(line)) return;
  if (parser_rewrite_ask_about(line)) return;
  if (parser_rewrite_unlock_open_use(line)) return;
  if (parser_rewrite_take_drop_put(line)) return;
  if (parser_rewrite_go_single_direction(line)) return;

  (void)parser_autofix_first_token_unique_typo(line);
}

static void commas_then_to_semicolon(char *s) {
  char *p = s;
  while ((p = strstr(p, ", then ")) != NULL) {
    *p = ';';
    memmove(p + 1, p + 7, strlen(p + 7) + 1);
    p++;
  }
  p = s;
  while ((p = strstr(p, " then ")) != NULL) {
    *p = ';';
    memmove(p + 1, p + 6, strlen(p + 6) + 1);
    p++;
  }
  p = s;
  while ((p = strstr(p, " after that ")) != NULL) {
    *p = ';';
    memmove(p + 1, p + 12, strlen(p + 12) + 1);
    p++;
  }
}

static int count_visited(void) {
  int i, c = 0;
  for (i = 0; i < WORLD_ROOM_COUNT; i++)
    if (g_visited[i]) c++;
  return c;
}

static int room_is_waypoint(int room) {
  const char *slug;
  const char *const *items;
  if (room < 0 || room >= WORLD_ROOM_COUNT) return 0;
  slug = world_slug(room);
  if (slug && (strstr(slug, "waystone") || strstr(slug, "nexus_point")))
    return 1;
  items = world_item_list(room);
  if (items) {
    for (; *items; items++) {
      if (str_ieq(*items, "waystone_monolith") ||
          str_ieq(*items, "nexus_monolith") || strstr(*items, "waystone") ||
          strstr(*items, "nexus"))
        return 1;
    }
  }
  return 0;
}

static int count_waypoints(int only_visited) {
  int i, c = 0;
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    if (!room_is_waypoint(i)) continue;
    if (only_visited && !g_visited[i]) continue;
    c++;
  }
  return c;
}

static int count_known_npcs(void) {
  int i, c = 0;
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    const char *ent;
    if (!g_visited[i]) continue;
    ent = world_room_entity(i);
    if (ent && ent[0]) c++;
  }
  return c;
}

static int room_visited_by_slug(const char *slug) {
  int r = world_room_index(slug);
  return r >= 0 && g_visited[r];
}

static int objective_done_front_door(void) {
  return g_front_unlocked || room_visited_by_slug("inside_house");
}

static void objective_line(char *body, size_t cap, int done,
                           const char *text) {
  body_append(body, cap, "  [%c] %s\n", done ? 'x' : ' ', text);
}

static void format_objectives_body(char *body, size_t cap) {
  int west = world_room_index("west_of_house");
  int inside = room_visited_by_slug("inside_house");
  int village = room_visited_by_slug("village_square") ||
                room_visited_by_slug("nexus_point_2");
  int treasure = room_visited_by_slug("treasure_room");
  int searched_start =
      west >= 0 ? (g_hidden_n[west] == 0) : (count_visited() > 1);
  int front_done = objective_done_front_door();
  int any_waypoint = count_waypoints(1) > 0;
  char role[96];
  char pr[64];
  char purseb[32];
  AetPcSave ob;
  pc_capture(&ob);
  pc_fill_narrative_defaults(&ob);
  pc_format_role_phrase(role, sizeof role);
  pc_format_pronouns_short(ob.gender[0] ? ob.gender : "they", pr, sizeof pr);
  currency_format_compact(g_coins, purseb, sizeof purseb);

  snprintf(body, cap,
           "Objectives\n\n"
           "Playing: %s (%s)\n"
           "%s\n\n"
           "Current: %s  [%s]\n"
           "Explored: %d / %d   Health: %d/%d   Score: %d   Purse: %s\n\n"
           "Story track\n",
           pc_display_name(), role, pr, resolve_world_title(g_room),
           world_slug(g_room), count_visited(), WORLD_ROOM_COUNT, g_health,
           g_max_health, g_score, purseb);
  objective_line(body, cap, count_visited() > 1,
                 "Leave the first clearing and map the house perimeter.");
  objective_line(body, cap, searched_start,
                 "Search the starting grounds for anything useful.");
  objective_line(body, cap, front_done,
                 "Get through the front door of the house.");
  objective_line(body, cap, inside,
                 "Explore the inside of the house and secure a light source.");
  objective_line(body, cap, g_shed_unlocked,
                 "Open the shed east of the house with the lockpick.");
  objective_line(body, cap, village,
                 "Push beyond the house paths toward the village and Nexus.");
  objective_line(body, cap, any_waypoint,
                 "Attune at least one Waystone or Nexus Point.");
  objective_line(body, cap, treasure,
                 "Find the treasure room.");

  body_append(body, cap, "\nNext best leads\n");
  if (!searched_start) {
    body_append(body, cap,
                "  - Try search at the west side of the house; the old web "
                "logic hid early supplies there.\n");
  } else if (!front_done) {
    body_append(body, cap,
                "  - Inspect the front door area and use the house key there "
                "when you have it.\n");
  } else if (!player_has_light_source()) {
    body_append(body, cap,
                "  - Find or carry a lantern, torch, candle, or similar light "
                "before committing to dark rooms.\n");
  } else if (!g_shed_unlocked) {
    body_append(body, cap,
                "  - The shed lock accepts the lockpick; use it while you are "
                "east of the house.\n");
  } else if (!any_waypoint) {
    body_append(body, cap,
                "  - Watch for Waystone or Nexus rooms. Once visited, they "
                "become travel anchors.\n");
  } else if (!treasure) {
    body_append(body, cap,
                "  - Use route <place> and waypoints to chase deeper named "
                "rooms without losing the thread.\n");
  } else {
    body_append(body, cap,
                "  - Main exploration arc is open-ended now; journal and "
                "notes are your long-form tracker.\n");
  }
}

static void format_progress_body(char *body, size_t cap) {
  int visited = count_visited();
  int wp_seen = count_waypoints(1);
  int wp_total = count_waypoints(0);
  int npc_seen = count_known_npcs();
  int adj_locks = count_adjacent_locks(1);
  int adj_npcs = count_adjacent_npcs();
  int done_notes = 0;
  int quest_done = 0, quest_active = 0, quest_open = 0;
  int i;
  AetWorldClock wc;
  char t[32];
  for (i = 0; i < g_note_n; i++)
    if (note_is_done(g_notes[i])) done_notes++;
  {
    const char *const *qh = world_quest_hints();
    if (qh) {
      for (; *qh; qh++) {
        int st = quest_journal_hint_state(*qh);
        if (st >= 2)
          quest_done++;
        else if (st == 1)
          quest_active++;
        else
          quest_open++;
      }
    }
  }
  get_world_clock(&wc);
  format_clock_time(t, sizeof t, &wc);
  {
    char role[96];
    char pr[64];
    char purseb[48];
    AetPcSave prog;
    pc_capture(&prog);
    pc_fill_narrative_defaults(&prog);
    pc_format_role_phrase(role, sizeof role);
    pc_format_pronouns_short(prog.gender[0] ? prog.gender : "they", pr,
                             sizeof pr);
    currency_format_long(g_coins, purseb, sizeof purseb);
    snprintf(body, cap,
             "Progress\n\n"
             "Character: %s (%s)\n"
             "Pronouns: %s\n\n"
             "Explored: %d / %d locations (%d%%)\n"
             "Current: %s  [%s]\n"
             "Region: %s\n"
             "Time: %s (%s), day %d, %s\n"
             "Weather: %s, %dC\n\n"
             "World state\n"
             "  Front door: %s\n"
             "  Shed lock: %s\n"
             "  Light source: %s\n"
             "  Waypoints attuned: %d / %d\n"
             "  Known NPC rooms: %d\n\n"
             "Local pressure\n"
             "  Adjacent locked exits: %d\n"
             "  Adjacent NPC routes: %d\n"
             "  Best lock tool: %s\n\n"
             "Player state\n"
             "  Pack: %d / %d slots\n"
             "  Health: %d / %d\n"
             "  Purse: %s\n"
             "  Score: %d\n"
             "  Turns: %d\n"
             "  Notes: %d todo, %d done\n"
             "  Quest board: %d done, %d active, %d open\n"
             "  Autosave shadow: %s\n"
             "  Social history rows: %d (relationship history)\n",
             pc_display_name(), role, pr, visited, WORLD_ROOM_COUNT,
             WORLD_ROOM_COUNT ? (visited * 100) / WORLD_ROOM_COUNT : 0,
             resolve_world_title(g_room), world_slug(g_room),
             world_region(g_room)[0] ? world_region(g_room) : "(unspecified)",
             t, wc.period, wc.day, wc.season, wc.weather, wc.temp_c,
             g_front_unlocked ? "open" : "locked",
             g_shed_unlocked ? "open" : "locked",
             player_has_light_source() ? "carried" : "not carried", wp_seen,
             wp_total, npc_seen, adj_locks, adj_npcs, best_lock_tool(),
             g_inv_n, MAX_INV, g_health, g_max_health, purseb, g_score,
             g_turns, g_note_n - done_notes, done_notes, quest_done,
             quest_active, quest_open,
             g_autosave_enabled ? "on" : "off", g_rel_hist_count);
  }
}

static int room_matches_waypoint_query(int room, const char *q,
                                       const char *qnorm) {
  char title_norm[160];
  if (!room_is_waypoint(room)) return 0;
  query_norm_underscore(title_norm, sizeof title_norm, resolve_world_title(room));
  if (str_ieq(world_slug(room), q) || str_ieq(world_slug(room), qnorm))
    return 1;
  if (str_ieq(title_norm, qnorm)) return 1;
  if (strlen(qnorm) >= 3 && strstr(world_slug(room), qnorm)) return 1;
  if (strlen(qnorm) >= 3 && strstr(title_norm, qnorm)) return 1;
  if (strlen(q) >= 3 && str_contains_ci(resolve_world_title(room), q)) return 1;
  return 0;
}

static int resolve_waypoint_destination(const char *raw, char *err,
                                        size_t errcap) {
  char q[160];
  char qnorm[MAX_ITEM_LEN];
  int i, best = -1, matches = 0, hidden_matches = 0;
  if (!raw || !raw[0]) {
    snprintf(err, errcap, "Name an attuned Waystone or Nexus Point.");
    return -1;
  }
  strncpy(q, raw, sizeof q - 1);
  q[sizeof q - 1] = '\0';
  strip_leading_articles(q);
  if (!strncmp(q, "to ", 3)) memmove(q, q + 3, strlen(q + 3) + 1);
  strip_trailing_space(q);
  query_norm_underscore(qnorm, sizeof qnorm, q);

  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    if (!room_matches_waypoint_query(i, q, qnorm)) continue;
    if (!g_visited[i]) {
      hidden_matches++;
      continue;
    }
    best = i;
    matches++;
  }
  if (matches == 1) return best;
  if (matches > 1) {
    snprintf(err, errcap,
             "\"%s\" matches more than one attuned waypoint; use the slug "
             "shown in waypoints.",
             raw);
    return -1;
  }
  if (hidden_matches) {
    snprintf(err, errcap,
             "That waypoint is not attuned yet. Visit it normally first.");
    return -1;
  }
  snprintf(err, errcap,
           "No attuned Waystone or Nexus Point matches \"%s\".", raw);
  return -1;
}

static void format_waypoints_body(char *body, size_t cap) {
  int i, n = 0;
  char role[96];
  char pr[64];
  AetPcSave wp;
  pc_capture(&wp);
  pc_fill_narrative_defaults(&wp);
  pc_format_role_phrase(role, sizeof role);
  pc_format_pronouns_short(wp.gender[0] ? wp.gender : "they", pr, sizeof pr);
  snprintf(body, cap,
           "Waystones & Nexus Points\n\n"
           "Traveler: %s (%s)\n"
           "%s\n\n"
           "Attuned points are rooms you have already visited. Travel only "
           "works while standing at one.\n\n",
           pc_display_name(), role, pr);
  for (i = 0; i < WORLD_ROOM_COUNT; i++) {
    if (!room_is_waypoint(i) || !g_visited[i]) continue;
    body_append(body, cap, "  %s%-28s [%s]  %s\n", i == g_room ? "* " : "  ",
                resolve_world_title(i), world_slug(i),
                world_region(i)[0] ? world_region(i) : "unknown region");
    n++;
  }
  if (!n)
    body_append(body, cap,
                "  None attuned yet. Find a Waystone or Nexus Point in the "
                "world, then it will appear here.\n");
  body_append(body, cap, "\nKnown: %d / %d\n", n, count_waypoints(0));
  if (room_is_waypoint(g_room))
    body_append(body, cap,
                "You are standing at an active travel point. Use fasttravel "
                "<slug>.\n");
  else
    body_append(body, cap,
                "You are not at a travel point right now. Use route <slug> to "
                "walk to one.\n");
}

static int cmd_waypoint_travel(const char *raw, char *msg, size_t msgcap) {
  char err[256];
  int dest;
  if (!room_is_waypoint(g_room)) {
    snprintf(msg, msgcap,
             "You need to be standing at a Waystone or Nexus Point before the "
             "network will carry you.");
    return 0;
  }
  dest = resolve_waypoint_destination(raw, err, sizeof err);
  if (dest < 0) {
    snprintf(msg, msgcap, "%s", err);
    return 0;
  }
  if (dest == g_room) {
    snprintf(msg, msgcap, "The runes are already tuned to this place.");
    return 0;
  }
  hist_push(g_room);
  g_room = dest;
  g_visited[g_room] = 1;
  g_last_npc[0] = '\0';
  clear_focus();
  snprintf(
      msg, msgcap,
      "The monolith answers. Light folds around you, %s, and the world "
      "resolves at %s.",
      pc_display_name(), resolve_world_title(g_room));
  return 1;
}

static void cmd_trail(char *msg, size_t msgcap, int limit) {
  int i, start, requested;
  char banner[256];
  msg[0] = '\0';
  pc_format_identity_banner(banner, sizeof banner);
  if (g_hist_n <= 0) {
    snprintf(msg, msgcap,
             "%s\n\nNo trail yet — move around first.",
             banner);
    return;
  }
  requested = limit;
  if (limit <= 0 || limit > g_hist_n) limit = g_hist_n;
  start = g_hist_n - limit;
  if (requested > g_hist_n)
    snprintf(msg, msgcap,
             "%s\n\nBack-trail (oldest to newest, showing all %d available step(s) "
             "(requested %d)):",
             banner, g_hist_n, requested);
  else if (limit < g_hist_n)
    snprintf(msg, msgcap,
             "%s\n\nBack-trail (oldest to newest, showing last %d of %d step(s)):",
             banner, limit, g_hist_n);
  else
    snprintf(msg, msgcap, "%s\n\nBack-trail (oldest to newest):", banner);
  for (i = start; i < g_hist_n; i++) {
    body_append(msg, msgcap, "\n  %2d. %s", i - start + 1,
                resolve_world_title(g_hist[i]));
  }
  body_append(msg, msgcap, "\n\nUse  back  or  back <n>  to retrace.\n");
}

typedef struct {
  const char *name;
  const char *mat_class;
  int is_base_tool;
  int hrd, shp, flx, dur, wgt, grp, bnd, utl;
} CraftMatProfile;

typedef struct {
  const char *type;
  int req_hrd, req_shp, req_dur, req_bnd, req_grp, req_flx;
} CraftArchetype;

typedef struct {
  int hrd, shp, flx, dur, wgt, grp, bnd, utl;
} CraftAttr;

static const CraftMatProfile CRAFT_PROFILES[] = {
    {"stick", "Wood", 0, 4, 0, 3, 5, 3, 4, 1, 7},
    {"reed", "Plant", 0, 1, 0, 7, 2, 1, 2, 2, 5},
    {"hardwood_branch", "Wood", 0, 6, 0, 2, 8, 5, 5, 1, 8},
    {"wood_scrap", "Wood", 0, 3, 0, 3, 4, 2, 3, 1, 6},
    {"scrap_metal", "Metal", 0, 7, 3, 1, 6, 6, 2, 1, 6},
    {"metal_rod", "Metal", 0, 9, 0, 1, 9, 8, 3, 1, 7},
    {"flint", "Stone", 0, 8, 9, 0, 6, 4, 2, 0, 6},
    {"sharp_stone", "Stone", 0, 6, 5, 0, 4, 5, 1, 0, 5},
    {"vine", "Plant", 0, 1, 0, 8, 3, 1, 2, 9, 6},
    {"leather_cord", "Animal", 0, 2, 0, 9, 7, 2, 3, 7, 8},
    {"grass_fiber", "Plant", 0, 0, 0, 6, 1, 0, 1, 5, 4},
    {"long_stick", "Wood", 0, 4, 0, 4, 5, 4, 4, 1, 7},
    {"bone_shard", "Bone", 0, 5, 6, 1, 3, 2, 1, 0, 4},
    {"stone_blade", "Stone", 0, 7, 7, 0, 5, 5, 1, 0, 5},
    {"iron_spike", "Metal", 0, 9, 8, 0, 8, 3, 1, 0, 6},
    {"leather_wrap", "Animal", 0, 1, 0, 9, 7, 1, 9, 6, 8},
    {"bandage", "Cloth", 0, 0, 0, 7, 5, 0, 4, 8, 6},
    {"cloth", "Cloth", 0, 0, 0, 8, 4, 0, 3, 8, 5},
    {"oil", "Fuel", 0, 0, 0, 2, 1, 1, 1, 2, 9},
    {"resin", "Pitch", 0, 1, 0, 3, 6, 1, 8, 9, 8},
    {"glass_shard", "Glass", 0, 4, 9, 0, 1, 1, 0, 0, 4},
    {"wire", "Metal", 0, 5, 1, 6, 5, 2, 2, 8, 8},
    {"lockpick", "Tool/Metal", 1, 5, 4, 5, 4, 1, 5, 2, 9},
    {"club", "Tool/Wood", 1, 5, 1, 2, 6, 5, 5, 1, 6},
    {"bone-tipped_spear", "Tool/Weapon", 1, 6, 6, 4, 5, 5, 4, 0, 8},
    {NULL, "Mixed", 0, 3, 0, 3, 3, 3, 3, 1, 4}};

static const CraftArchetype CRAFT_ARCHETYPES[] = {
    {"Axe", 10, 6, 7, 4, 4, 1},
    {"Spear", 8, 6, 5, 3, 3, 4},
    {"Knife", 6, 8, 4, 2, 3, 1},
    {"Sword", 8, 8, 6, 3, 5, 2},
    {"Mace", 10, 3, 8, 2, 6, 1},
    {"Shield", 8, 1, 9, 5, 7, 2},
    {"Bow", 4, 2, 7, 7, 8, 9},
    {"Torch", 2, 0, 2, 3, 2, 6},
    {"Bandage", 1, 0, 7, 4, 5, 8},
    {"Snare", 2, 2, 5, 8, 6, 9},
    {"Charm", 3, 1, 5, 6, 5, 5},
    {"Lockpick", 5, 4, 4, 2, 7, 5},
    {NULL, 0, 0, 0, 0, 0, 0}};

static void craft_keyword_boost(const char *text, CraftMatProfile *out) {
  if (!text || !out) return;
  if (str_contains_ci(text, "metal") || str_contains_ci(text, "iron") ||
      str_contains_ci(text, "steel") || str_contains_ci(text, "bronze") ||
      str_contains_ci(text, "copper") || str_contains_ci(text, "scrap")) {
    out->hrd += 3;
    out->dur += 2;
    out->wgt += 2;
  }
  if (str_contains_ci(text, "stone") || str_contains_ci(text, "flint") ||
      str_contains_ci(text, "shard") || str_contains_ci(text, "blade") ||
      str_contains_ci(text, "glass") || str_contains_ci(text, "obsidian")) {
    out->shp += 3;
    out->hrd += 2;
  }
  if (str_contains_ci(text, "wood") || str_contains_ci(text, "stick") ||
      str_contains_ci(text, "branch") || str_contains_ci(text, "handle")) {
    out->utl += 2;
    out->grp += 1;
    out->dur += 1;
  }
  if (str_contains_ci(text, "reed") || str_contains_ci(text, "fiber") ||
      str_contains_ci(text, "cloth") || str_contains_ci(text, "bandage") ||
      str_contains_ci(text, "linen") || str_contains_ci(text, "thread")) {
    out->flx += 3;
    out->bnd += 3;
    out->wgt -= 1;
  }
  if (str_contains_ci(text, "leather") || str_contains_ci(text, "wrap") ||
      str_contains_ci(text, "cord") || str_contains_ci(text, "vine") ||
      str_contains_ci(text, "rope") || str_contains_ci(text, "strap") ||
      str_contains_ci(text, "wire")) {
    out->grp += 3;
    out->bnd += 3;
  }
  if (str_contains_ci(text, "torch") || str_contains_ci(text, "lantern") ||
      str_contains_ci(text, "oil") || str_contains_ci(text, "resin") ||
      str_contains_ci(text, "pitch") || str_contains_ci(text, "wax")) {
    out->utl += 3;
    out->bnd += 1;
  }
  if (str_contains_ci(text, "bone") || str_contains_ci(text, "horn") ||
      str_contains_ci(text, "antler")) {
    out->hrd += 2;
    out->shp += 1;
    out->wgt -= 1;
  }
  if (str_contains_ci(text, "gem") || str_contains_ci(text, "crystal") ||
      str_contains_ci(text, "rune") || str_contains_ci(text, "symbol")) {
    out->utl += 4;
    out->bnd += 2;
    out->shp += 1;
  }
  if (str_contains_ci(text, "key") || str_contains_ci(text, "lockpick")) {
    out->is_base_tool = 1;
    out->utl += 4;
    out->hrd += 2;
    out->shp += 1;
  }
  if (str_contains_ci(text, "spear") || str_contains_ci(text, "knife") ||
      str_contains_ci(text, "axe") || str_contains_ci(text, "sword")) {
    out->is_base_tool = 1;
    out->shp += 3;
    out->dur += 2;
  }
}

static void craft_profile_clamp(CraftMatProfile *p) {
  if (!p) return;
#define CLAMP10(x)      \
  do {                  \
    if ((x) < 0) (x) = 0; \
    if ((x) > 10) (x) = 10; \
  } while (0)
  CLAMP10(p->hrd);
  CLAMP10(p->shp);
  CLAMP10(p->flx);
  CLAMP10(p->dur);
  CLAMP10(p->wgt);
  CLAMP10(p->grp);
  CLAMP10(p->bnd);
  CLAMP10(p->utl);
#undef CLAMP10
}

static CraftSavedProfile *craft_profile_lookup(const char *item) {
  int i;
  if (!item || !item[0]) return NULL;
  for (i = 0; i < g_craft_prof_n; i++)
    if (str_ieq(g_craft_prof[i].name, item)) return &g_craft_prof[i];
  return NULL;
}

static void craft_profile_save(const char *name, const CraftAttr *sum, int bench_n,
                               int quality, int st_dur, int st_shp, int st_hnd,
                               int st_wgt) {
  CraftSavedProfile *p;
  int den = bench_n > 0 ? bench_n : 1;
  int i;
  if (!name || !name[0] || !sum) return;
  p = craft_profile_lookup(name);
  if (!p) {
    if (g_craft_prof_n >= MAX_CRAFT_PROFILES) return;
    p = &g_craft_prof[g_craft_prof_n++];
    snprintf(p->name, sizeof p->name, "%s", name);
  }
  p->hrd = sum->hrd / den;
  p->shp = sum->shp / den;
  p->flx = sum->flx / den;
  p->dur = sum->dur / den;
  p->wgt = sum->wgt / den;
  p->grp = sum->grp / den;
  p->bnd = sum->bnd / den;
  p->utl = sum->utl / den;
  if (p->hrd < 0) p->hrd = 0;
  if (p->hrd > 10) p->hrd = 10;
  if (p->shp < 0) p->shp = 0;
  if (p->shp > 10) p->shp = 10;
  if (p->flx < 0) p->flx = 0;
  if (p->flx > 10) p->flx = 10;
  if (p->dur < 0) p->dur = 0;
  if (p->dur > 10) p->dur = 10;
  if (p->wgt < 0) p->wgt = 0;
  if (p->wgt > 10) p->wgt = 10;
  if (p->grp < 0) p->grp = 0;
  if (p->grp > 10) p->grp = 10;
  if (p->bnd < 0) p->bnd = 0;
  if (p->bnd > 10) p->bnd = 10;
  if (p->utl < 0) p->utl = 0;
  if (p->utl > 10) p->utl = 10;
  p->quality = quality;
  p->disp_dur = st_dur;
  p->disp_shp = st_shp;
  p->disp_hnd = st_hnd;
  p->disp_wgt = st_wgt;
  (void)i;
}

static void craft_profile_for_item(const char *item, CraftMatProfile *out) {
  int i;
  char norm[MAX_ITEM_LEN];
  char mod_class[24];
  int mod_base = 0, hrd = 0, shp = 0, flx = 0, dur = 0, wgt = 0, grp = 0, bnd = 0,
      utl = 0;
  char mod_desc[256];
  if (!out) return;
  query_norm_underscore(norm, sizeof norm, item ? item : "");
  {
    CraftSavedProfile *saved = craft_profile_lookup(item);
    if (saved) {
      out->name = item;
      out->mat_class = "Forged";
      out->is_base_tool = 0;
      out->hrd = saved->hrd;
      out->shp = saved->shp;
      out->flx = saved->flx;
      out->dur = saved->dur;
      out->wgt = saved->wgt;
      out->grp = saved->grp;
      out->bnd = saved->bnd;
      out->utl = saved->utl;
      if (saved->disp_shp >= 7 || saved->disp_dur >= 7) out->is_base_tool = 1;
      return;
    }
  }
  if (aet_mods_crafting_profile(norm, mod_class, sizeof mod_class, &mod_base, &hrd,
                                &shp, &flx, &dur, &wgt, &grp, &bnd, &utl)) {
    out->name = item;
    out->mat_class = "Modded";
    out->is_base_tool = mod_base;
    out->hrd = hrd;
    out->shp = shp;
    out->flx = flx;
    out->dur = dur;
    out->wgt = wgt;
    out->grp = grp;
    out->bnd = bnd;
    out->utl = utl;
    craft_profile_clamp(out);
    return;
  }
  for (i = 0; CRAFT_PROFILES[i].name; i++) {
    if (str_ieq(CRAFT_PROFILES[i].name, norm) || str_ieq(CRAFT_PROFILES[i].name, item)) {
      *out = CRAFT_PROFILES[i];
      return;
    }
  }
  *out = CRAFT_PROFILES[i];
  out->name = item;
  craft_keyword_boost(item, out);
  if (aet_mods_item_description(norm, mod_desc, sizeof mod_desc))
    craft_keyword_boost(mod_desc, out);
  craft_profile_clamp(out);
}

static int inv_remove_exact_one(const char *name) {
  int i;
  char taken[MAX_ITEM_LEN];
  if (!name || !name[0]) return 0;
  for (i = 0; i < g_inv_n; i++) {
    if (!str_ieq(g_inv[i], name)) continue;
    (void)inv_take_out(i, taken, sizeof taken);
    return 1;
  }
  return 0;
}

static int craft_bench_has(char bench[][MAX_ITEM_LEN], int bench_n, const char *name) {
  int i;
  char bn[MAX_ITEM_LEN], nn[MAX_ITEM_LEN];
  query_norm_underscore(nn, sizeof nn, name ? name : "");
  for (i = 0; i < bench_n; i++) {
    query_norm_underscore(bn, sizeof bn, bench[i]);
    if (str_ieq(bench[i], name) || str_ieq(bn, nn)) return 1;
  }
  return 0;
}

static void format_bar10(int v, char *out, size_t cap) {
  int i, n;
  if (!out || cap < 12) return;
  if (v < 0) v = 0;
  if (v > 10) v = 10;
  n = v;
  for (i = 0; i < 10; i++) out[i] = i < n ? '#' : '.';
  out[10] = '\0';
}

static void inv_add_unique(const char *name) {
  if (!name || !name[0]) return;
  if (inv_has(name)) return;
  inv_add(name);
}

static void grant_starting_loadout(void) {
  AetPcSave p;
  char chosen[MAX_ITEM_LEN];
  pc_capture(&p);
  pc_fill_narrative_defaults(&p);

  if (!strcmp(p.class_, "warrior") || !strcmp(p.class_, "barbarian") ||
      !strcmp(p.class_, "knight") || !strcmp(p.class_, "paladin"))
    inv_add_unique("sword");
  else if (!strcmp(p.class_, "rogue") || !strcmp(p.class_, "assassin"))
    inv_add_unique("dagger");
  else if (!strcmp(p.class_, "ranger"))
    inv_add_unique("bow");
  else if (!strcmp(p.class_, "wizard") || !strcmp(p.class_, "mage") ||
           !strcmp(p.class_, "sorcerer") || !strcmp(p.class_, "warlock"))
    inv_add_unique("staff");
  else if (!strcmp(p.class_, "priest") || !strcmp(p.class_, "druid"))
    inv_add_unique("mace");
  else if (!strcmp(p.class_, "bard"))
    inv_add_unique("dagger");
  else if (!strcmp(p.class_, "mercenary")) {
    inv_add_unique("sword");
    inv_add_unique("lockpick");
  }   else
    inv_add_unique("club");

  if (aet_autotest()) {
    inv_add_unique("lockpick");
    inv_add_unique("bow");
    inv_add_unique("leaflet");
    inv_add_unique("stick");
    inv_add_unique("reed");
    inv_add_unique("skeleton_key");
  }

  
  if (p.weapon[0] && !str_ieq(p.weapon, "none")) {
    query_norm_underscore(chosen, sizeof chosen, p.weapon);
    inv_add_unique(chosen);
  }
  
  if (p.armor[0] && !str_ieq(p.armor, "none")) {
    query_norm_underscore(chosen, sizeof chosen, p.armor);
    inv_add_unique(chosen);
    {
      char armored[MAX_ITEM_LEN];
      static const char suffix[] = "_armor";
      size_t base_len = strlen(chosen);
      size_t suffix_len = strlen(suffix);
      if (base_len + suffix_len < sizeof armored) {
        memcpy(armored, chosen, base_len);
        memcpy(armored + base_len, suffix, suffix_len + 1);
        inv_add_unique(armored);
      }
    }
  }

  
  inv_add_unique("bandage");
  inv_add_unique("flint");
}

static void eq_clear_all(void) {
  int i;
  for (i = 0; i < EQ_SLOT_COUNT; i++) g_eq_slots[i][0] = '\0';
}

static void eq_copy_slug(char *dst, size_t cap, const char *src) {
  size_t n;
  if (!dst || cap < 1) return;
  if (!src) src = "";
  n = strnlen(src, cap - 1);
  memcpy(dst, src, n);
  dst[n] = '\0';
}

static void eq_sync_pc_sheet(void) {
  AetPcSave p;
  pc_capture(&p);
  if (g_eq_slots[EQ_WEAPON][0])
    eq_copy_slug(p.weapon, sizeof p.weapon, g_eq_slots[EQ_WEAPON]);
  else
    eq_copy_slug(p.weapon, sizeof p.weapon, "None");
  if (g_eq_slots[EQ_CHEST][0])
    eq_copy_slug(p.armor, sizeof p.armor, g_eq_slots[EQ_CHEST]);
  else
    eq_copy_slug(p.armor, sizeof p.armor, "None");
  pc_restore(&p);
}

static void eq_bootstrap_take_to_slot(const char *field, int slot, char *norm,
                                      size_t norm_cap) {
  int ix;
  if (!field || !field[0] || str_ieq(field, "none")) return;
  query_norm_underscore(norm, norm_cap, field);
  ix = inv_find(norm);
  if (ix >= 0) {
    char taken[MAX_ITEM_LEN];
    (void)inv_take_out(ix, taken, sizeof taken);
    eq_copy_slug(g_eq_slots[slot], sizeof g_eq_slots[slot], taken);
  }
}

static void eq_bootstrap_from_character(void) {
  AetPcSave p;
  char norm[MAX_ITEM_LEN];
  pc_capture(&p);
  eq_bootstrap_take_to_slot(p.weapon, EQ_WEAPON, norm, sizeof norm);
  eq_bootstrap_take_to_slot(p.armor, EQ_CHEST, norm, sizeof norm);
  eq_sync_ready_item();
  eq_sync_pc_sheet();
}

static void craft_short_display_name(const char *slug, char *dst, size_t cap) {
  const AetItemCatalogEntry *cat;
  char tmp[MAX_ITEM_LEN];
  size_t j;
  if (!dst || cap < 2) return;
  dst[0] = '\0';
  if (!slug || !slug[0]) return;
  cat = aet_item_catalog_by_slug(slug);
  if (cat && cat->label && cat->label[0]) {
    snprintf(dst, cap, "%s", cat->label);
    return;
  }
  snprintf(tmp, sizeof tmp, "%s", slug);
  for (j = 0; tmp[j]; j++)
    if (tmp[j] == '_') tmp[j] = ' ';
  snprintf(dst, cap, "%s", tmp);
}

static int eq_slot_index(const char *name) {
  int i;
  if (!name || !name[0]) return -1;
  for (i = 0; i < EQ_SLOT_COUNT; i++) {
    if (str_ieq(name, EQ_SLOT_NAME[i])) return i;
  }
  return -1;
}

static int eq_infer_slot(const char *item) {
  if (!item || !item[0]) return EQ_ACCESSORY;
  if (str_contains_ci(item, "helm") || str_contains_ci(item, "cowl") ||
      str_contains_ci(item, "hood")) return EQ_HEAD;
  if (str_contains_ci(item, "mail") || str_contains_ci(item, "armor") ||
      str_contains_ci(item, "robe") || str_contains_ci(item, "shirt")) return EQ_CHEST;
  if (str_contains_ci(item, "glove") || str_contains_ci(item, "gauntlet")) return EQ_HANDS;
  if (str_contains_ci(item, "trouser") || str_contains_ci(item, "pants") ||
      str_contains_ci(item, "leggings")) return EQ_LEGS;
  if (str_contains_ci(item, "boot") || str_contains_ci(item, "shoe")) return EQ_FEET;
  if (str_contains_ci(item, "shield") || str_contains_ci(item, "buckler")) return EQ_OFFHAND;
  if (str_contains_ci(item, "ring") || str_contains_ci(item, "amulet") ||
      str_contains_ci(item, "charm")) return EQ_ACCESSORY;
  if (str_contains_ci(item, "sword") || str_contains_ci(item, "axe") ||
      str_contains_ci(item, "mace") || str_contains_ci(item, "staff") ||
      str_contains_ci(item, "dagger") || str_contains_ci(item, "spear") ||
      str_contains_ci(item, "bow") || str_contains_ci(item, "lockpick")) return EQ_WEAPON;
  return EQ_ACCESSORY;
}

static void eq_sync_ready_item(void) {
  if (g_eq_slots[EQ_WEAPON][0]) {
    snprintf(g_ready_item, sizeof g_ready_item, "%s", g_eq_slots[EQ_WEAPON]);
    return;
  }
  if (g_eq_slots[EQ_OFFHAND][0]) {
    snprintf(g_ready_item, sizeof g_ready_item, "%s", g_eq_slots[EQ_OFFHAND]);
    return;
  }
  g_ready_item[0] = '\0';
}

static void eq_prune_slots_not_in_inventory(void) {
  int i, changed = 0;
  for (i = 0; i < EQ_SLOT_COUNT; i++) {
    if (g_eq_slots[i][0] && !inv_has(g_eq_slots[i])) {
      g_eq_slots[i][0] = '\0';
      changed = 1;
    }
  }
  if (changed) {
    eq_sync_ready_item();
    eq_sync_pc_sheet();
  }
}

static void eq_remove_item_from_slots(const char *slug) {
  int i, changed = 0;
  if (!slug || !slug[0]) return;
  for (i = 0; i < EQ_SLOT_COUNT; i++) {
    if (str_ieq(g_eq_slots[i], slug)) {
      g_eq_slots[i][0] = '\0';
      changed = 1;
    }
  }
  if (changed) {
    eq_sync_ready_item();
    eq_sync_pc_sheet();
  }
}

static void eq_compute_item_stats(const char *slug, int equipped_slot, int *def_out,
                                  int *atk_out, int *wgt_out, int *slot_out,
                                  const char **label_out) {
  const AetItemCatalogEntry *cat;
  CraftMatProfile p;
  int def = 0, atk = 0, wgt = 0, slot = equipped_slot;
  const char *label = slug ? slug : "";
  if (!slug || !slug[0]) {
    if (def_out) *def_out = 0;
    if (atk_out) *atk_out = 0;
    if (wgt_out) *wgt_out = 0;
    if (slot_out) *slot_out = (equipped_slot >= 0) ? equipped_slot : EQ_ACCESSORY;
    if (label_out) *label_out = "";
    return;
  }
  cat = aet_item_catalog_by_slug(slug);
  if (cat) {
    def = cat->def;
    atk = cat->atk;
    wgt = cat->wgt;
    if (slot < 0) slot = cat->slot_index;
    if (cat->label && cat->label[0]) label = cat->label;
  } else {
    craft_profile_for_item(slug, &p);
    def = (p.hrd / 3) + (p.dur / 4);
    if (slot < 0)
      atk = (p.shp / 2) + (p.hrd / 5);
    else if (slot == EQ_WEAPON)
      atk = (p.shp / 2) + (p.hrd / 4) + 1;
    else if (slot == EQ_OFFHAND)
      atk = p.shp / 6;
    else
      atk = p.shp / 10;
    wgt = p.wgt / 2;
    if (slot < 0) slot = eq_infer_slot(slug);
  }
  if (def_out) *def_out = def;
  if (atk_out) *atk_out = atk;
  if (wgt_out) *wgt_out = wgt;
  if (slot_out) *slot_out = slot;
  if (label_out) *label_out = label;
}

static int eq_equip_from_inventory_index(int inv_ix, int slot, char *msg, size_t msgcap) {
  char item[MAX_ITEM_LEN];
  if (inv_ix < 0 || inv_ix >= g_inv_n || slot < 0 || slot >= EQ_SLOT_COUNT) return 0;
  if (!inv_take_out(inv_ix, item, sizeof item)) return 0;
  if (g_eq_slots[slot][0]) {
    if (g_inv_n >= MAX_INV) {
      inv_add(item);
      snprintf(msg, msgcap, "Cannot swap %s: inventory full.", EQ_SLOT_NAME[slot]);
      return 0;
    }
    inv_add(g_eq_slots[slot]);
  }
  snprintf(g_eq_slots[slot], sizeof g_eq_slots[slot], "%s", item);
  eq_sync_ready_item();
  eq_sync_pc_sheet();
  snprintf(msg, msgcap, "Equipped %s to %s.", item, EQ_SLOT_NAME[slot]);
  return 1;
}

static int eq_unequip_to_inventory(int slot, char *msg, size_t msgcap) {
  char item[MAX_ITEM_LEN];
  if (slot < 0 || slot >= EQ_SLOT_COUNT) return 0;
  if (!g_eq_slots[slot][0]) {
    snprintf(msg, msgcap, "%s is already empty.", EQ_SLOT_NAME[slot]);
    return 0;
  }
  if (g_inv_n >= MAX_INV) {
    snprintf(msg, msgcap, "Inventory full.");
    return 0;
  }
  snprintf(item, sizeof item, "%s", g_eq_slots[slot]);
  inv_add(item);
  g_eq_slots[slot][0] = '\0';
  eq_sync_ready_item();
  eq_sync_pc_sheet();
  snprintf(msg, msgcap, "Unequipped %s from %s.", item, EQ_SLOT_NAME[slot]);
  return 1;
}

static void ui_fit_cell(char *out, size_t cap, const char *src, int width) {
  int i = 0;
  if (!out || cap < 2 || width < 1) return;
  out[0] = '\0';
  if (!src) src = "";
  for (; src[i] && i < width && (size_t)i + 1 < cap; i++) out[i] = src[i];
  while (i < width && (size_t)i + 1 < cap) out[i++] = ' ';
  out[i] = '\0';
}

static int aet_sgr_vis_len(const char *s) {
  int n = 0;
  if (!s) return 0;
  while (*s) {
    if ((unsigned char)*s == '\033' && s[1] == '[') {
      s += 2;
      while (*s && *s != 'm') s++;
      if (*s == 'm') s++;
      continue;
    }
    n++;
    s++;
  }
  return n;
}

static void aet_sgr_pad_to(char *dst, size_t cap, int target_vis) {
  size_t L;
  if (!dst || cap < 2) return;
  while (aet_sgr_vis_len(dst) < target_vis) {
    L = strlen(dst);
    if (L + 2 >= cap) break;
    dst[L] = ' ';
    dst[L + 1] = '\0';
  }
}

enum { FORGE_CELL_L = 38, FORGE_CELL_M = 38, FORGE_CELL_R = 36 };

static void forge_cell_pad(char *dst, size_t cap, const char *src, int visw) {
  if (!dst || cap < 2) return;
  snprintf(dst, cap, "%s", src ? src : "");
  aet_sgr_pad_to(dst, cap, visw);
}

static void forge_print_sep(void) {
  int k;
  printf("  %s+", C_BORDER);
  for (k = 0; k < FORGE_CELL_L; k++) putchar('-');
  printf("+%s+", C_BORDER);
  for (k = 0; k < FORGE_CELL_M; k++) putchar('-');
  printf("+%s+", C_BORDER);
  for (k = 0; k < FORGE_CELL_R; k++) putchar('-');
  printf("+%s\n", C_RESET);
}

static void forge_print_row(const char *l, const char *m, const char *r) {
  char lb[640], mb[640], rb[640];
  forge_cell_pad(lb, sizeof lb, l, FORGE_CELL_L);
  forge_cell_pad(mb, sizeof mb, m, FORGE_CELL_M);
  forge_cell_pad(rb, sizeof rb, r, FORGE_CELL_R);
  fputs("  ", stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputs(lb, stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputc(' ', stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputs(mb, stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputc(' ', stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputs(rb, stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputc('\n', stdout);
}

enum { PAUSE_CELL_W = 55 };

static void pause_print_row(const char *l, const char *r) {
  char lb[512], rb[512];
  forge_cell_pad(lb, sizeof lb, l, PAUSE_CELL_W);
  forge_cell_pad(rb, sizeof rb, r, PAUSE_CELL_W);
  fputs("  ", stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputs(lb, stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs("    ", stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputs(rb, stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputc('\n', stdout);
}

static void pause_print_divider_cell(void) {
  char d[256];
  snprintf(d, sizeof d, "%s%*s%s", C_BORDER, PAUSE_CELL_W, "", C_RESET);
  pause_print_row(d, d);
}

enum { SETTINGS_CELL_L = 40, SETTINGS_CELL_R = 75 };

typedef struct {
  int color_mode;
  int verbose_room;
  int hints;
  int ironman;
  int aud_m;
  int aud_s;
} SettingsWork;

static void settings_print_row(const char *l, const char *r) {
  char lb[512], rb[768];
  forge_cell_pad(lb, sizeof lb, l, SETTINGS_CELL_L);
  forge_cell_pad(rb, sizeof rb, r, SETTINGS_CELL_R);
  fputs("  ", stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputs(lb, stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputs(rb, stdout);
  fputs(C_BORDER, stdout);
  fputc('|', stdout);
  fputs(C_RESET, stdout);
  fputc('\n', stdout);
}

static void settings_from_globals(SettingsWork *w) {
  w->color_mode = g_settings_color_ov;
  if (w->color_mode < -1 || w->color_mode > 1) w->color_mode = -1;
  w->verbose_room = g_verbose_room ? 1 : 0;
  w->hints = g_hints_pref ? 1 : 0;
  w->ironman = g_ironman_stub ? 1 : 0;
  w->aud_m = 0;
  w->aud_s = 0;
}

static void settings_apply_work(const SettingsWork *w) {
  g_settings_color_ov = w->color_mode;
  if (g_settings_color_ov < -1 || g_settings_color_ov > 1) g_settings_color_ov = -1;
  g_verbose_room = w->verbose_room ? 1 : 0;
  g_hints_pref = w->hints ? 1 : 0;
  g_ironman_stub = w->ironman ? 1 : 0;
  ui_init_color();
}

static const char *settings_color_caption(int cm) {
  if (cm <= -1) return "Follow environment";
  if (cm == 0) return "Disabled (mono)";
  return "Enabled (ANSI)";
}

static void settings_cycle_color(int *cm, int dir) {
  int idx =
      (*cm <= -1) ? 0 : (*cm == 0) ? 1 : 2;
  idx = (idx + dir + 3) % 3;
  *cm = (idx == 0) ? -1 : (idx == 1) ? 0 : 1;
}

static int settings_cat_maxopt(int cat) {
  static const int mx[5] = {2, 2, 2, 0, 0};
  if (cat < 0 || cat >= 5) return 0;
  return mx[cat];
}

#if defined(_WIN32)
#define AETER_STDIN_TTY() (_isatty(_fileno(stdin)))
#else
#define AETER_STDIN_TTY() (isatty(STDIN_FILENO))
#endif

static void run_settings_ui(void) {
  static const char *const kCat[5] = {
      "DISPLAY & TERMINAL",
      "AUDIO (TEXT PORT)",
      "GAMEPLAY ASSISTS",
      "SAVE SETTINGS & EXIT",
      "DISCARD CHANGES",
  };
  SettingsWork snap, work;
  int panel = 0;
  int sel_l = 0;
  int sel_r = 0;
  int sel_cat = 0;
#if !defined(_WIN32)
  int raw_ok = guide_tty_raw_begin();
#endif
  GuideKey gk;

  settings_from_globals(&snap);
  work = snap;

  if (aet_autotest() || !AETER_STDIN_TTY()) {
    clear_frame();
    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(" %sSYSTEM CONFIGURATION%s\n\n", C_TITLE, C_RESET);
    printf(
        " %sPipe / CI mode: open Settings from an interactive terminal for the "
        "full UI.%s\n",
        C_MUTED, C_RESET);
    printf(
        " %sPreferences still persist via quicksave after you change them in "
        "game.%s\n",
        C_MUTED, C_RESET);
    ui_block_pause("SETTINGS", "[Press Enter]");
#if !defined(_WIN32)
    guide_tty_raw_end();
#endif
    return;
  }

  for (;;) {
    char ls[512], rs[768];
    int i, mo, row;

    clear_frame();
    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    {
      char hdr[512];
      const char ttl[] = "SYSTEM CONFIGURATION";
      int pad = (120 - (int)(sizeof ttl - 1)) / 2;
      if (pad < 0) pad = 0;
      snprintf(hdr, sizeof hdr, "%s%*s%s%s", C_TITLE, pad, "", ttl, C_RESET);
      aet_sgr_pad_to(hdr, sizeof hdr, 120);
      printf("  %s\n", hdr);
    }
    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);

    for (row = 0; row < 16; row++) {
      int rrow = row;
      ls[0] = rs[0] = '\0';
      if (rrow < 5) {
        int hl = (panel == 0 && sel_l == rrow);
        int cur = (sel_cat == rrow);
        snprintf(ls, sizeof ls, "%s%s%s%s%s", hl ? C_ITEM : "",
                 hl ? "> " : "  ", cur ? C_EXIT : C_HEADING, kCat[rrow],
                 C_RESET);
      }
      if (sel_cat == 0 && rrow >= 1 && rrow <= 5) {
        int sr = rrow - 1;
        int hi = (panel == 1 && sel_cat == 0 && sel_r == sr && sr < 2);
        if (sr == 0) {
          snprintf(rs, sizeof rs,
                   "%s%sANSI colors       : %s%s%s%s",
                   hi ? C_ITEM : "", hi ? "> " : "  ",
                   C_HEADING, settings_color_caption(work.color_mode),
                   C_RESET, hi ? " <" : "");
        } else if (sr == 1) {
          snprintf(rs, sizeof rs,
                   "%s%sRoom blurbs       : %s%s%s%s",
                   hi ? C_ITEM : "", hi ? "> " : "  ",
                   C_HEADING, work.verbose_room ? "Verbose" : "Brief",
                   C_RESET, hi ? " <" : "");
        } else if (sr == 2) {
          snprintf(rs, sizeof rs,
                   "%s%sTip — blurbs follow  verbose / brief  commands too.%s",
                   C_MUTED, "", C_RESET);
        } else if (sr == 3) {
          snprintf(rs, sizeof rs, "%s%s%s", C_MUTED,
                   "(Also respects AETER_COLOR / terminal capabilities.)",
                   C_RESET);
        }
      } else if (sel_cat == 1 && rrow >= 1 && rrow <= 4) {
        char hb[16];
        int sr = rrow - 1;
        int hi = (panel == 1 && sel_cat == 1 && sel_r == sr && sr < 2);
        for (i = 0; i < 10; i++)
          hb[i] = (char)(i < work.aud_m ? '#' : '.');
        hb[10] = '\0';
        if (sr == 0) {
          snprintf(rs, sizeof rs,
                   "%s%sMusic (visual only): %s[%s]%s%s",
                   hi ? C_ITEM : "", hi ? "> " : "  ", C_HEADING, hb,
                   C_RESET, hi ? " <" : "");
        } else if (sr == 1) {
          for (i = 0; i < 10; i++)
            hb[i] = (char)(i < work.aud_s ? '#' : '.');
          hb[10] = '\0';
          snprintf(rs, sizeof rs,
                   "%s%sSFX (visual only)  : %s[%s]%s%s",
                   hi ? C_ITEM : "", hi ? "> " : "  ", C_HEADING, hb,
                   C_RESET, hi ? " <" : "");
        } else if (sr == 2) {
          snprintf(rs, sizeof rs, "%s%s%s", C_MUTED,
                   "No digital audio engine in this text port — sliders are "
                   "cosmetic.",
                   C_RESET);
        }
      } else if (sel_cat == 2 && rrow >= 1 && rrow <= 5) {
        int sr = rrow - 1;
        int hi = (panel == 1 && sel_cat == 2 && sel_r == sr && sr < 2);
        if (sr == 0) {
          snprintf(rs, sizeof rs,
                   "%s%sContext hints cmd : %s%s%s%s",
                   hi ? C_ITEM : "", hi ? "> " : "  ", C_HEADING,
                   work.hints ? "Enabled" : "Disabled", C_RESET,
                   hi ? " <" : "");
        } else if (sr == 1) {
          snprintf(rs, sizeof rs,
                   "%s%sIronman tours      : %s%s%s%s",
                   hi ? C_ITEM : "", hi ? "> " : "  ", C_EXIT,
                   work.ironman ? "Marked (future)" : "Off", C_RESET,
                   hi ? " <" : "");
        } else if (sr == 2) {
          snprintf(rs, sizeof rs, "%s%s%s", C_MUTED,
                   "Ironman does not alter saves yet — reserved for a future "
                   "challenge flag.",
                   C_RESET);
        }
      } else if (sel_cat == 3 && rrow >= 1 && rrow <= 3) {
        if (rrow == 1)
          snprintf(rs, sizeof rs, "%s%s%s", C_HEADING,
                   "Writes hintena / colorov / room blurbs into your quicksave "
                   "file.",
                   C_RESET);
        else if (rrow == 2)
          snprintf(rs, sizeof rs, "%s%s%s", C_MUTED,
                   "Choose this row on the left and press Enter.", C_RESET);
      } else if (sel_cat == 4 && rrow >= 1 && rrow <= 3) {
        if (rrow == 1)
          snprintf(rs, sizeof rs, "%s%s%s", C_HEADING,
                   "Reverts changes made on this screen since you opened it.",
                   C_RESET);
        else if (rrow == 2)
          snprintf(rs, sizeof rs, "%s%s%s", C_MUTED,
                   "Choose this row on the left and press Enter.", C_RESET);
      }
      settings_print_row(ls, rs);
    }

    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(" %s[ARROWS] Navigate   [LEFT/RIGHT] Adjust   [TAB] Switch panel   "
           "[ENTER] Activate   [ESC] Cancel%s\n",
           C_MUTED, C_RESET);
    fflush(stdout);

#if defined(_WIN32)
    gk = guide_read_key_win();
#else
    gk = guide_read_key_unix(raw_ok);
#endif

    if (gk == GK_ESC) {
      work = snap;
      settings_apply_work(&work);
      break;
    }
    if (gk == GK_TAB) {
      panel = !panel;
      if (panel && settings_cat_maxopt(sel_cat) <= 0)
        panel = 0;
      continue;
    }

    if (panel == 0) {
      if (gk == GK_UP)
        sel_l = (sel_l + 4) % 5;
      else if (gk == GK_DOWN)
        sel_l = (sel_l + 1) % 5;
      sel_cat = sel_l;
      if (gk == GK_ENTER) {
        if (sel_l <= 2) {
          panel = 1;
          sel_r = 0;
        } else if (sel_l == 3) {
          settings_apply_work(&work);
          if (write_save_file()) {
            ui_block_pause("SETTINGS",
                           "Preferences saved with your quicksave file.");
          } else
            ui_block_pause("SETTINGS", "Could not write save — prefs kept in "
                                       "memory for this session.");
          break;
        } else {
          work = snap;
          settings_apply_work(&work);
          break;
        }
      }
      continue;
    }

    mo = settings_cat_maxopt(sel_cat);
    if (gk == GK_UP && mo > 0)
      sel_r = (sel_r + mo - 1) % mo;
    else if (gk == GK_DOWN && mo > 0)
      sel_r = (sel_r + 1) % mo;
    else if ((gk == GK_LEFT || gk == GK_RIGHT) && mo > 0) {
      int dir = (gk == GK_RIGHT) ? 1 : -1;
      if (sel_cat == 0) {
        if (sel_r == 0)
          settings_cycle_color(&work.color_mode, dir);
        else if (sel_r == 1)
          work.verbose_room ^= 1;
      } else if (sel_cat == 1) {
        if (sel_r == 0) {
          work.aud_m += dir;
          if (work.aud_m < 0) work.aud_m = 0;
          if (work.aud_m > 10) work.aud_m = 10;
        } else if (sel_r == 1) {
          work.aud_s += dir;
          if (work.aud_s < 0) work.aud_s = 0;
          if (work.aud_s > 10) work.aud_s = 10;
        }
      } else if (sel_cat == 2) {
        if (sel_r == 0)
          work.hints ^= 1;
        else if (sel_r == 1)
          work.ironman ^= 1;
      }
    }
  }
#if !defined(_WIN32)
  guide_tty_raw_end();
#endif
}

typedef struct {
  char *dst;
  size_t cap;
  int maxw;
  int len;
} UiLineBuf;

typedef struct {
  const char *title;
  const char *pending_acc;
  int *did_fullscreen;
  int width;
} AetScreenFrame;

static void ui_line_init(UiLineBuf *b, char *dst, size_t cap, int maxw) {
  if (!b || !dst || cap < 2) return;
  b->dst = dst;
  b->cap = cap;
  b->maxw = maxw;
  b->len = 0;
  b->dst[0] = '\0';
}

static void ui_line_append(UiLineBuf *b, const char *text) {
  int i = 0;
  if (!b || !b->dst || b->cap < 2 || !text) return;
  while (text[i] && b->len < b->maxw && (size_t)(b->len + 1) < b->cap) {
    b->dst[b->len++] = text[i++];
  }
  b->dst[b->len] = '\0';
}

static void ui_line_append_width(UiLineBuf *b, const char *text, int width) {
  char tmp[256];
  if (!b || width < 1) return;
  ui_fit_cell(tmp, sizeof tmp, text ? text : "", width);
  ui_line_append(b, tmp);
}

static void ui_line_pad(UiLineBuf *b) {
  if (!b || !b->dst || b->cap < 2) return;
  while (b->len < b->maxw && (size_t)(b->len + 1) < b->cap) b->dst[b->len++] = ' ';
  b->dst[b->len] = '\0';
}

static void screen_frame_init(AetScreenFrame *f, const char *title,
                              const char *pending_acc, int *did_fullscreen,
                              int width) {
  if (!f) return;
  f->title = title ? title : "";
  f->pending_acc = pending_acc;
  f->did_fullscreen = did_fullscreen;
  f->width = width > 0 ? width : 120;
}

static void screen_frame_rule(const AetScreenFrame *f) {
  int i, w = (f && f->width > 0) ? f->width : 120;
  fputs(C_BORDER, stdout);
  for (i = 0; i < w; i++) putchar('=');
  fputs(C_RESET, stdout);
  putchar('\n');
}

static void screen_frame_begin_status(const AetScreenFrame *f, const char *right) {
  clear_frame();
  screen_frame_rule(f);
  if (f && f->title && f->title[0]) {
    UiLineBuf lb;
    char line[192];
    int title_len = (int)strlen(f->title);
    int right_len = (right && right[0]) ? (int)strlen(right) : 0;
    int pad = (f->width - title_len) / 2;
    if (pad < 0) pad = 0;
    if (right_len > 0 && pad + title_len + 1 + right_len > f->width) {
      pad = f->width - title_len - right_len - 1;
      if (pad < 0) pad = 0;
    }
    ui_line_init(&lb, line, sizeof line, f->width);
    ui_line_append_width(&lb, "", pad);
    ui_line_append(&lb, f->title);
    if (right_len > 0 && lb.len < f->width - right_len)
      ui_line_append_width(&lb, "", f->width - right_len - lb.len);
    if (right_len > 0) ui_line_append(&lb, right);
    ui_line_pad(&lb);
    printf("%s%s%s\n", C_TITLE, line, C_RESET);
  }
  screen_frame_rule(f);
}

static void screen_frame_prompt(const AetScreenFrame *f) {
  if (f && f->pending_acc && f->pending_acc[0]) printf("\n%s", f->pending_acc);
  printf("\n%s>> %s", C_PROMPT, C_RESET);
  fflush(stdout);
}

static int screen_read_command(char *line, size_t cap, int lower) {
  size_t i;
  if (!line || cap < 2) return 0;
  if (!fgets(line, (int)cap, stdin)) return 0;
  chomp_line(line);
  strip_trailing_space(line);
  if (lower) {
    for (i = 0; line[i]; i++) line[i] = (char)tolower((unsigned char)line[i]);
  }
  return 1;
}

static void screen_frame_finish(const AetScreenFrame *f) {
  if (f && f->did_fullscreen) *f->did_fullscreen = 1;
  return_to_game_screen();
}

static void craft_compute_stats(const CraftAttr *a, const char *name, int *dur,
                                int *shp, int *hnd, int *wgt, int bench_n,
                                int *quality) {
  int prof_bonus, den, q;
  int avg_hrd, avg_shp, avg_flx, avg_dur, avg_wgt, avg_grp, avg_bnd, avg_utl;
  int ld, ls, lh, lw;
  if (!a || !dur || !shp || !hnd || !wgt) return;
  den = bench_n > 0 ? bench_n : 1;
  prof_bonus = g_craft_proficiency / 3;
  avg_hrd = a->hrd / den;
  avg_shp = a->shp / den;
  avg_flx = a->flx / den;
  avg_dur = a->dur / den;
  avg_wgt = a->wgt / den;
  avg_grp = a->grp / den;
  avg_bnd = a->bnd / den;
  avg_utl = a->utl / den;

  q = (avg_hrd + avg_shp + avg_dur + avg_grp + avg_bnd + avg_utl) * 2;
  q += (avg_flx / 2) - avg_wgt + (g_craft_proficiency * 3);
  if (str_contains_ci(name ? name : "", "flimsy")) q -= 14;
  if (str_contains_ci(name ? name : "", "worthless")) q -= 30;
  if (q < 1) q = 1;
  if (q > 100) q = 100;

  ld = ((avg_hrd * 2) + avg_dur + avg_bnd) / 2 + prof_bonus;
  ls = avg_shp + (q >= 75 ? 2 : (q >= 55 ? 1 : 0));
  lh = 4 + avg_grp - (avg_wgt / 2) + prof_bonus + (avg_flx / 3);
  lw = avg_wgt;
  if (str_contains_ci(name ? name : "", "flimsy")) ld = ld > 3 ? 3 : ld;
  if (ld < 1) ld = 1;
  if (ls < 0) ls = 0;
  if (lh < 1) lh = 1;
  if (lw < 1) lw = 1;
  if (ld > 10) ld = 10;
  if (ls > 10) ls = 10;
  if (lh > 10) lh = 10;
  if (lw > 10) lw = 10;
  *dur = ld;
  *shp = ls;
  *hnd = lh;
  *wgt = lw;
  if (quality) *quality = q;
}

static int craft_mp_like_binding_wrap(const CraftMatProfile *mp) {
  if (!mp) return 0;
  return mp->bnd >= 6 && mp->flx >= 7 && mp->hrd <= 4;
}

static int craft_mp_like_metal_reinforcement(const CraftMatProfile *mp) {
  if (!mp) return 0;
  return (mp->hrd >= 6 && mp->shp >= 4) || mp->hrd >= 8;
}

static int craft_bench_has_soft_plant_fiber(char bench[][MAX_ITEM_LEN], int bench_n) {
  int i;
  for (i = 0; i < bench_n; i++) {
    CraftMatProfile mp;
    craft_profile_for_item(bench[i], &mp);
    if (mp.flx >= 6 && mp.dur <= 5 && mp.hrd <= 3 && mp.shp <= 5) return 1;
  }
  return 0;
}

static int craft_archetype_distance(int req_hrd, int req_shp, int req_dur,
                                    int req_bnd, int req_grp, int req_flx,
                                    const CraftAttr *sum) {
  int dist = 0;
  if (!sum) return 99999;
  dist += abs(req_hrd - sum->hrd);
  dist += (int)(1.5 * abs(req_shp - sum->shp));
  dist += abs(req_dur - sum->dur) / 2;
  dist += abs(req_bnd - sum->bnd);
  dist += abs(req_grp - sum->grp) / 2;
  dist += abs(req_flx - sum->flx) / 2;
  return dist;
}

static int craft_profile_like_edge(const CraftMatProfile *p) {
  return p && p->shp >= 6 && p->hrd >= 4;
}

static int craft_profile_like_core(const CraftMatProfile *p) {
  return p && p->hrd >= 5 && p->dur >= 4;
}

static int craft_profile_like_handle(const CraftMatProfile *p) {
  return p && p->grp >= 4 && p->utl >= 5 && p->wgt <= 6;
}

static int craft_profile_like_binding(const CraftMatProfile *p) {
  return p && p->bnd >= 6 && p->flx >= 5;
}

static int craft_profile_like_fuel(const CraftMatProfile *p) {
  return p && p->utl >= 8 && p->wgt <= 3;
}

static int craft_profile_like_ward(const CraftMatProfile *p) {
  return p && p->utl >= 7 && p->bnd >= 4 && p->shp <= 4;
}

static int craft_blend_synergy(char bench[][MAX_ITEM_LEN], int bench_n,
                               CraftAttr *sum) {
  int i;
  int edge = 0, core = 0, handle = 0, binding = 0, fuel = 0, ward = 0;
  if (!sum) return 0;
  for (i = 0; i < bench_n; i++) {
    CraftMatProfile p;
    craft_profile_for_item(bench[i], &p);
    if (craft_profile_like_edge(&p)) edge++;
    if (craft_profile_like_core(&p)) core++;
    if (craft_profile_like_handle(&p)) handle++;
    if (craft_profile_like_binding(&p)) binding++;
    if (craft_profile_like_fuel(&p)) fuel++;
    if (craft_profile_like_ward(&p)) ward++;
  }
  if (edge && handle) {
    sum->shp += 2;
    sum->grp += 1;
    sum->utl += 1;
  }
  if (core && binding) {
    sum->dur += 3;
    sum->bnd += 1;
  }
  if (edge && binding) {
    sum->shp += 1;
    sum->bnd += 2;
  }
  if (fuel && binding) {
    sum->utl += 3;
    sum->dur += 1;
  }
  if (ward && binding) {
    sum->utl += 2;
    sum->bnd += 2;
  }
  if (bench_n >= 4 && edge && core && handle && binding) {
    sum->dur += 2;
    sum->grp += 2;
    sum->utl += 1;
  }
  if (sum->wgt >= 12 && binding == 0) sum->grp -= 2;
  return edge + core + handle + binding + fuel + ward;
}

static void craft_predict(char bench[][MAX_ITEM_LEN], int bench_n,
                          char *out_name, size_t out_name_cap, CraftAttr *out_attr,
                          char *d1, size_t d1cap, char *d2, size_t d2cap,
                          char *d3, size_t d3cap, int *ok_out) {
  int i;
  CraftAttr sum = {0, 0, 0, 0, 0, 0, 0, 0};
  int has_tool = 0;
  int synergy = 0;
  char base_tool[MAX_ITEM_LEN] = "";
  if (ok_out) *ok_out = 0;
  if (!out_name || out_name_cap < 2 || !out_attr) return;
  for (i = 0; i < bench_n; i++) {
    CraftMatProfile p;
    craft_profile_for_item(bench[i], &p);
    if (p.is_base_tool && !has_tool) {
      has_tool = 1;
      snprintf(base_tool, sizeof base_tool, "%s", bench[i]);
    }
    sum.hrd += p.hrd;
    sum.shp += p.shp;
    sum.flx += p.flx;
    sum.dur += p.dur;
    sum.wgt += p.wgt;
    sum.grp += p.grp;
    sum.bnd += p.bnd;
    sum.utl += p.utl;
  }
  synergy = craft_blend_synergy(bench, bench_n, &sum);
  *out_attr = sum;
  if (bench_n <= 0) {
    snprintf(out_name, out_name_cap, "Unknown / Debris");
    snprintf(d1, d1cap, "The bench is empty.");
    snprintf(d2, d2cap, "Silence where something might form.");
    if (d3cap) d3[0] = '\0';
    return;
  }
  if (has_tool) {
    int has_wrap = 0;
    int has_reinforce = 0;
    int has_edge = 0;
    int mods_used = 0;
    char built_name[160];
    copy_capped(out_name, out_name_cap, base_tool);
    snprintf(d1, d1cap, "A core piece anchors the pile.");
    snprintf(d2, d2cap, "Other matter presses against it.");
    if (d3cap) d3[0] = '\0';
    for (i = 0; i < bench_n; i++) {
      CraftMatProfile mp;
      if (str_ieq(bench[i], base_tool)) continue;
      craft_profile_for_item(bench[i], &mp);
      if (str_ieq(bench[i], "leather_wrap") || str_ieq(bench[i], "leather_cord") ||
          craft_mp_like_binding_wrap(&mp)) {
        sum.grp += 3;
        sum.dur += 2;
        has_wrap = 1;
        mods_used++;
      }
      if (str_ieq(bench[i], "iron_spike") || str_ieq(bench[i], "metal_rod") ||
          craft_mp_like_metal_reinforcement(&mp)) {
        sum.shp += 2;
        sum.dur += 3;
        if (mp.wgt >= 6) sum.wgt += 2;
        has_reinforce = 1;
        mods_used++;
      }
      if (mp.shp >= 6) {
        sum.shp += 1;
        has_edge = 1;
        mods_used++;
      }
    }
    copy_capped(built_name, sizeof built_name, base_tool);
    if (has_wrap) {
      char t[192];
      snprintf(t, sizeof t, "Wrapped %s", built_name);
      copy_capped(built_name, sizeof built_name, t);
    }
    if (has_reinforce) {
      char t[192];
      snprintf(t, sizeof t, "Reinforced %s", built_name);
      copy_capped(built_name, sizeof built_name, t);
    } else if (has_edge) {
      char t[192];
      snprintf(t, sizeof t, "Honed %s", built_name);
      copy_capped(built_name, sizeof built_name, t);
    }
    copy_capped(out_name, out_name_cap, built_name);
    snprintf(d2, d2cap, "The mix wants to change shape.");
    if (has_reinforce)
      snprintf(d3, d3cap, "Hard matter bites into the frame.");
    else if (has_wrap)
      snprintf(d3, d3cap, "Soft binding creeps along the haft.");
    else if (has_edge)
      snprintf(d3, d3cap, "An edge wakes, hungry and thin.");
    *out_attr = sum;
    if (bench_n <= 1) {
      snprintf(d1, d1cap, "Only the core remains.");
      snprintf(d2, d2cap, "Nothing else leans against it.");
      snprintf(d3, d3cap, "No change worth keeping.");
      if (ok_out) *ok_out = 0;
      return;
    }
    if (ok_out) *ok_out = 1;
    return;
  }
  {
    int best = 9999;
    const char *best_type = "Debris";
    if (aet_mods_crafting_archetype_count() > 0) {
      int ai;
      for (ai = 0; ai < aet_mods_crafting_archetype_count(); ai++) {
        AetCraftArchetype ma;
        int dist;
        if (!aet_mods_crafting_archetype_get(ai, &ma)) continue;
        dist = craft_archetype_distance(ma.req_hrd, ma.req_shp, ma.req_dur,
                                        ma.req_bnd, ma.req_grp, ma.req_flx,
                                        &sum);
        if (synergy >= 4) dist -= 2;
        if (dist < best) {
          best = dist;
          best_type = ma.name;
        }
      }
    } else
      for (i = 0; CRAFT_ARCHETYPES[i].type; i++) {
      int dist;
      const CraftArchetype *a = &CRAFT_ARCHETYPES[i];
      dist = craft_archetype_distance(a->req_hrd, a->req_shp, a->req_dur,
                                      a->req_bnd, a->req_grp, a->req_flx, &sum);
      if (synergy >= 4) dist -= 2;
      if (dist < best) {
        best = dist;
        best_type = a->type;
      }
    }
    if (bench_n < 2 || best > 28) {
      snprintf(out_name, out_name_cap, "Worthless Debris");
      snprintf(d1, d1cap, "The heap collapses into slack scrap.");
      snprintf(d2, d2cap, "No coherent form emerges.");
      if (d3cap) d3[0] = '\0';
      return;
    }
    {
      const char *prefix = "Crude";
      const char *mat = "";
      if ((craft_bench_has(bench, bench_n, "reed") ||
            craft_bench_has_soft_plant_fiber(bench, bench_n)) &&
          sum.flx >= 7 && sum.dur <= 4) {
        prefix = "Flimsy";
        mat = "Reed";
      } else if (sum.utl >= 14 && sum.bnd >= 8 && sum.shp <= 8) {
        prefix = "Odd";
        mat = "Warded";
      } else if (sum.hrd >= 10 && sum.wgt >= 8) {
        prefix = "Heavy Scrap";
      } else if (sum.flx >= 12 && sum.bnd >= 10) {
        prefix = "Tensioned";
      } else if (sum.dur >= 10 && sum.grp >= 6) {
        prefix = "Reliable";
      }
      if (sum.hrd >= 8 && sum.shp >= 7 && sum.bnd >= 3) mat = "Flint";
      else if (sum.utl >= 12 && sum.bnd >= 8) mat = "Bound";
      else if (sum.flx >= 12 && sum.grp >= 7) mat = "Spring";
      snprintf(out_name, out_name_cap, "%s %s %s", prefix, mat, best_type);
      while (strstr(out_name, "  ")) {
        char *p = strstr(out_name, "  ");
        memmove(p, p + 1, strlen(p));
      }
      snprintf(d1, d1cap, "Something like a %s takes shape.", best_type);
      snprintf(d2, d2cap, "The materials agree — for now.");
      if (!strcmp(prefix, "Flimsy"))
        snprintf(d3, d3cap, "Light. Probably temporary.");
      else if (!strcmp(prefix, "Heavy Scrap"))
        snprintf(d3, d3cap, "Ugly weight. It might endure.");
      else if (!strcmp(prefix, "Reliable"))
        snprintf(d3, d3cap, "Steady enough to trust once.");
      else
        snprintf(d3, d3cap, "Improvised, but not nothing.");
    }
    if (ok_out) *ok_out = 1;
  }
}

static int line_equals_one_of(const char *line, const char *const *candidates) {
  int i;
  for (i = 0; candidates[i]; i++) {
    if (!strcmp(line, candidates[i])) return 1;
  }
  return 0;
}

static void run_material_forge(const char *pending_acc, int *did_fullscreen) {
  enum { FORGE_ROWS = 14 };
  static const char *const kForgeExit[] = {"done", "back", "exit", "resume",
                                           NULL};
  AetScreenFrame frame;
  char bench[6][MAX_ITEM_LEN];
  int bench_n = 0;
  char line[INPUT_LINE_MAX];
  screen_frame_init(&frame, "D Y N A M I C   M A T E R I A L   F O R G E",
                    pending_acc, did_fullscreen, 120);
  eq_prune_slots_not_in_inventory();
  for (;;) {
    int idx_map[MAX_INV];
    int listed = 0;
    int i;
    CraftAttr a = {0, 0, 0, 0, 0, 0, 0, 0};
    char out_name[64], d1[96], d2[96], d3[96];
    int ok = 0;
    char bd[12], bs[12], bh[12], bw[12];
    char left_row[512], mid_row[512], right_cell[512];
    char blabel[48];
    char wb[40], subln[160];
    const char *cred;
    int st_dur = 0, st_shp = 0, st_hnd = 0, st_wgt = 0, q_score = 0;
    char profbuf[28];

    cred = g_use_color ? "\x1b[91;1m" : "";

    snprintf(profbuf, sizeof profbuf, "[ PROFICIENCY: %2d ]",
             g_craft_proficiency);
    screen_frame_begin_status(&frame, profbuf);
    craft_predict(bench, bench_n, out_name, sizeof out_name, &a, d1, sizeof d1, d2,
                  sizeof d2, d3, sizeof d3, &ok);
    craft_compute_stats(&a, out_name, &st_dur, &st_shp, &st_hnd, &st_wgt, bench_n,
                        &q_score);
    format_bar10(st_dur, bd, sizeof bd);
    format_bar10(st_shp, bs, sizeof bs);
    format_bar10(st_hnd, bh, sizeof bh);
    format_bar10(st_wgt, bw, sizeof bw);

    for (i = 0; i < FORGE_ROWS && i < g_inv_n; i++) idx_map[listed++] = i;

    snprintf(wb, sizeof wb, "[ WORKBENCH : %d/6 ]", bench_n);
    {
      UiLineBuf sb;
      ui_line_init(&sb, subln, sizeof subln, 120);
      ui_line_append(&sb, " ");
      ui_line_append(&sb, "[ RAW INVENTORY ]");
      ui_line_append_width(&sb, "", 20);
      ui_line_append(&sb, wb);
      ui_line_append_width(&sb, "", 19);
      ui_line_append(&sb, "[ PREDICTED OUTCOME ]");
      ui_line_pad(&sb);
      printf("%s%s%s\n", C_HEADING, subln, C_RESET);
    }

    forge_print_sep();

    for (i = 0; i < FORGE_ROWS; i++) {
      CraftMatProfile ip;
      const AetItemCatalogEntry *cat;
      int inv_no;

      left_row[0] = '\0';
      mid_row[0] = '\0';
      right_cell[0] = '\0';

      if (i < g_inv_n) {
        craft_profile_for_item(g_inv[i], &ip);
        craft_short_display_name(g_inv[i], blabel, sizeof blabel);
        cat = aet_item_catalog_by_slug(g_inv[i]);
        inv_no = cat ? cat->id : (i + 1);
        snprintf(
            left_row, sizeof left_row,
            " %s[%2d]%s %s%-18.18s%s %sH:%d S:%d%s", C_ITEM, inv_no, C_RESET,
            ip.is_base_tool ? C_BOOT_OK : C_HEADING, blabel, C_RESET, C_MUTED,
            ip.hrd, ip.shp, C_RESET);
      }

      if (i < 6) {
        if (i < bench_n) {
          CraftMatProfile bpr;
          craft_profile_for_item(bench[i], &bpr);
          craft_short_display_name(bench[i], blabel, sizeof blabel);
          snprintf(mid_row, sizeof mid_row, "%s%d. %s%s%s", C_HEADING, i + 1,
                   bpr.is_base_tool ? C_BOOT_OK : C_HEADING, blabel, C_RESET);
        } else {
          snprintf(mid_row, sizeof mid_row, "%s %d. %s[ Empty ]%s", C_HEADING,
                   i + 1, C_MUTED, C_RESET);
        }
      } else if (bench_n > 0) {
        if (i == 7)
          snprintf(mid_row, sizeof mid_row, "%s COMBINED MATERIAL ATTRIBUTES:%s",
                   C_TITLE, C_RESET);
        else if (i == 8)
          snprintf(mid_row, sizeof mid_row,
                   "%s  Hrd:%02d Shp:%02d Flx:%02d Dur:%02d%s", C_HEADING, a.hrd,
                   a.shp, a.flx, a.dur, C_RESET);
        else if (i == 9)
          snprintf(mid_row, sizeof mid_row,
                   "%s  Wgt:%02d Grp:%02d Bnd:%02d Utl:%02d%s", C_HEADING, a.wgt,
                   a.grp, a.bnd, a.utl, C_RESET);
      }

      if (i == 0) {
        snprintf(right_cell, sizeof right_cell, "%s RESULT: %s%-25.25s%s", C_TITLE,
                 C_HEADING, out_name, C_RESET);
      } else if (i == 2) {
        snprintf(right_cell, sizeof right_cell, "%s BASE STATS:%s", C_HEADING,
                 C_RESET);
      } else if (i == 3) {
        snprintf(right_cell, sizeof right_cell,
                 "%s  Durability:[%s%s%s]%s%02d%s", C_MUTED, C_BOOT_OK, bd,
                 C_RESET, C_HEADING, st_dur, C_RESET);
      } else if (i == 4) {
        snprintf(right_cell, sizeof right_cell,
                 "%s  Sharpness:[%s%s%s]%s%02d%s", C_MUTED, cred, bs, C_RESET,
                 C_HEADING, st_shp, C_RESET);
      } else if (i == 5) {
        snprintf(right_cell, sizeof right_cell,
                 "%s  Handling:[%s%s%s]%s%02d%s", C_MUTED, C_ITEM, bh, C_RESET,
                 C_HEADING, st_hnd, C_RESET);
      } else if (i == 6) {
        snprintf(right_cell, sizeof right_cell,
                 "%s  Weight:[%s%s%s]%s%02d%s", C_MUTED, C_MUTED, bw, C_RESET,
                 C_HEADING, st_wgt, C_RESET);
      } else if (i == 8) {
        snprintf(right_cell, sizeof right_cell, "%s ANALYSIS:%s", C_HEADING,
                 C_RESET);
      } else if (i == 9) {
        snprintf(right_cell, sizeof right_cell, "%s  %.36s%s", C_MUTED, d1,
                 C_RESET);
      } else if (i == 10) {
        snprintf(right_cell, sizeof right_cell, "%s  %.36s%s", C_MUTED, d2,
                 C_RESET);
      } else if (i == 11) {
        snprintf(right_cell, sizeof right_cell, "%s  %.36s%s", C_MUTED, d3,
                 C_RESET);
      }

      forge_print_row(left_row, mid_row, right_cell);
    }

    forge_print_sep();
    printf(" %sADD  REM  CLEAR  CRAFT  DONE%s\n", C_MUTED, C_RESET);
    screen_frame_prompt(&frame);
    if (!screen_read_command(line, sizeof line, 1)) break;
    if (!line[0]) continue;
    if (line_equals_one_of(line, kForgeExit)) {
      break;
    } else if (!strcmp(line, "clear")) {
      bench_n = 0;
      continue;
    } else if (!strncmp(line, "add ", 4)) {
      long v = strtol(line + 4, NULL, 10);
      if (v >= 1 && v <= listed && bench_n < 6) {
        CraftMatProfile p;
        int has_base = 0;
        craft_profile_for_item(g_inv[idx_map[v - 1]], &p);
        for (i = 0; i < bench_n; i++) {
          CraftMatProfile bp;
          craft_profile_for_item(bench[i], &bp);
          if (bp.is_base_tool) {
            has_base = 1;
            break;
          }
        }
        if (p.is_base_tool && has_base) {
          ui_block_pause("FORGE", "Cannot combine two base weapons/tools.");
          continue;
        }
        snprintf(bench[bench_n], sizeof bench[bench_n], "%s", g_inv[idx_map[v - 1]]);
        bench_n++;
      }
      continue;
    } else if (!strncmp(line, "rem ", 4)) {
      long v = strtol(line + 4, NULL, 10);
      int p = (int)v - 1;
      if (p >= 0 && p < bench_n) {
        for (i = p; i < bench_n - 1; i++)
          snprintf(bench[i], sizeof bench[i], "%s", bench[i + 1]);
        bench_n--;
      }
      continue;
    } else if (!strcmp(line, "craft")) {
      char made[64];
      int can = 1;
      char msg[200];
      craft_predict(bench, bench_n, made, sizeof made, &a, d1, sizeof d1, d2,
                    sizeof d2, d3, sizeof d3, &ok);
      if (!ok || str_contains_ci(made, "worthless") ||
          str_contains_ci(made, "unknown /")) {
        ui_block_pause("FORGE", "The combination collapses into useless debris.");
        continue;
      }
      for (i = 0; i < bench_n; i++) {
        if (inv_find(bench[i]) < 0 && !inv_has(bench[i])) {
          can = 0;
          break;
        }
      }
      if (!can || g_inv_n >= MAX_INV) {
        ui_block_pause("FORGE", "Craft failed: missing materials or full inventory.");
        continue;
      }
      for (i = 0; i < bench_n; i++) {
        if (inv_remove_exact_one(bench[i])) eq_remove_item_from_slots(bench[i]);
      }
      craft_compute_stats(&a, made, &st_dur, &st_shp, &st_hnd, &st_wgt, bench_n,
                          &q_score);
      inv_add(made);
      craft_profile_save(made, &a, bench_n, q_score, st_dur, st_shp, st_hnd, st_wgt);
      g_score += 8 + g_craft_proficiency;
      if (g_craft_proficiency < 10 && (rand() % 3 == 0)) g_craft_proficiency++;
      snprintf(msg, sizeof msg, "You set aside: %s", made);
      causal_push("craft", made);
      ui_block_pause("FORGE COMPLETE", msg);
      bench_n = 0;
      continue;
    }
  }
  screen_frame_finish(&frame);
}

static void eq_ui_emit_left_h48(void) {
  int k;
  printf("%s+", C_BORDER);
  for (k = 0; k < 48; k++) putchar('-');
  printf("+%s", C_RESET);
}

static void eq_ui_emit_right_h64(void) {
  int k;
  printf("%s+", C_BORDER);
  for (k = 0; k < 64; k++) putchar('-');
  printf("+%s", C_RESET);
}

static void eq_ui_emit_slot_row(int slot_i) {
  char inner[256];
  char tag[16];
  size_t j;
  int sdef = 0, satk = 0;
  snprintf(tag, sizeof tag, "[%s]", EQ_SLOT_NAME[slot_i]);
  for (j = 0; tag[j]; j++) tag[j] = (char)toupper((unsigned char)tag[j]);
  inner[0] = '\0';
  snprintf(inner + strlen(inner), sizeof inner - strlen(inner), " %s%-12.12s%s",
           C_MUTED, tag, C_RESET);
  if (g_eq_slots[slot_i][0]) {
    const char *disp = g_eq_slots[slot_i];
    eq_compute_item_stats(g_eq_slots[slot_i], slot_i, &sdef, &satk, NULL, NULL, &disp);
    snprintf(inner + strlen(inner), sizeof inner - strlen(inner),
             "%s%-20.20s%s%s(D:%02d A:%02d)%s", C_BOOT_OK, disp, C_RESET, C_HEADING,
             sdef, satk, C_RESET);
  } else {
    snprintf(inner + strlen(inner), sizeof inner - strlen(inner), "%s[ Empty ]%s",
             C_MUTED, C_RESET);
  }
  aet_sgr_pad_to(inner, sizeof inner, 48);
  printf("%s|%s%s|%s%s", C_BORDER, C_RESET, inner, C_BORDER, C_RESET);
}

static void eq_ui_emit_left_spacer50(void) {
  int k;
  printf("%s|%s", C_BORDER, C_RESET);
  for (k = 0; k < 48; k++) putchar(' ');
  printf("%s|%s", C_BORDER, C_RESET);
}

static void eq_ui_emit_combat_banner(void) {
  UiLineBuf lb;
  char inner[96];
  ui_line_init(&lb, inner, sizeof inner, 48);
  ui_line_append(&lb, " [ COMBAT STATISTICS ]");
  ui_line_pad(&lb);
  printf("%s %s", C_BORDER, C_RESET);
  printf("%s%s%s", C_TITLE, inner, C_RESET);
  printf("%s %s", C_BORDER, C_RESET);
}

static void eq_ui_emit_stat_armor_wgt(int def, int wgt) {
  char inner[256];
  snprintf(inner, sizeof inner,
           "%s  Armor Rating : %02d      Total Weight : %02d%s", C_HEADING, def, wgt,
           C_RESET);
  aet_sgr_pad_to(inner, sizeof inner, 48);
  printf("%s|%s%s|%s%s", C_BORDER, C_RESET, inner, C_BORDER, C_RESET);
}

static void eq_ui_emit_stat_atk_mob(int atk, int twgt) {
  char inner[320];
  const char *mob = (twgt < 6) ? "High" : ((twgt < 12) ? "Norm" : "Heav");
  const char *mc = (twgt < 6) ? C_EXIT : ((twgt < 12) ? C_HEADING : C_MUTED);
  size_t L;
  snprintf(inner, sizeof inner, "%s  Attack Power : %02d      Mobility     : ", C_HEADING,
           atk);
  L = strlen(inner);
  snprintf(inner + L, sizeof inner - L, "%s%s%s", mc, mob, C_RESET);
  aet_sgr_pad_to(inner, sizeof inner, 48);
  printf("%s|%s%s|%s%s", C_BORDER, C_RESET, inner, C_BORDER, C_RESET);
}

static void eq_ui_emit_inv_header(void) {
  char inner[256];
  snprintf(inner, sizeof inner,
           "%s ID    ITEM NAME              TYPE          ATTRIBUTES%s", C_TITLE, C_RESET);
  aet_sgr_pad_to(inner, sizeof inner, 64);
  printf("%s|%s%s|%s%s", C_BORDER, C_RESET, inner, C_BORDER, C_RESET);
}

static void eq_ui_emit_inv_sep(void) {
  int k;
  printf("%s|%s", C_BORDER, C_RESET);
  for (k = 0; k < 64; k++) putchar('-');
  printf("%s|%s", C_BORDER, C_RESET);
}

static void eq_ui_emit_inv_line(int inv_ix) {
  char inner[384];
  int idef, iatk, iwgt, slot_ix;
  const char *slot_name = "misc";
  const char *disp_name = g_inv[inv_ix];
  char idbracket[16], typechunk[24];
  const AetItemCatalogEntry *cat;
  eq_compute_item_stats(g_inv[inv_ix], -1, &idef, &iatk, &iwgt, &slot_ix, &disp_name);
  if (slot_ix >= 0 && slot_ix < EQ_SLOT_COUNT) slot_name = EQ_SLOT_NAME[slot_ix];
  cat = aet_item_catalog_by_slug(g_inv[inv_ix]);
  snprintf(idbracket, sizeof idbracket, "[%d]", cat ? cat->id : (inv_ix + 1));
  snprintf(typechunk, sizeof typechunk, "(%s)", slot_name);
  snprintf(inner, sizeof inner,
           " %s%-5s%s %s%-22.22s%s %s%-12s%s %sDef:%d Atk:%d Wgt:%d%s", C_ITEM,
           idbracket, C_RESET, C_HEADING, disp_name, C_RESET, C_MUTED, typechunk, C_RESET,
           C_MUTED, idef, iatk, iwgt, C_RESET);
  aet_sgr_pad_to(inner, sizeof inner, 64);
  printf("%s|%s%s|%s%s", C_BORDER, C_RESET, inner, C_BORDER, C_RESET);
}

static void eq_ui_emit_inv_empty(void) {
  char inner[96];
  inner[0] = '\0';
  aet_sgr_pad_to(inner, sizeof inner, 64);
  printf("%s|%s%s|%s%s", C_BORDER, C_RESET, inner, C_BORDER, C_RESET);
}

static void run_equipment_inventory_ui(const char *pending_acc, int *did_fullscreen) {
  enum { EQ_INV_ROWS = 12, EQ_ROWS = 3 + EQ_INV_ROWS + 1 };
  static const char *const kEqUiExit[] = {"done", "back", "exit", "resume", NULL};
  AetScreenFrame frame;
  char line[INPUT_LINE_MAX];
  char msg[200];
  screen_frame_init(&frame, "***   E Q U I P M E N T   &   I N V E N T O R Y   ***",
                    pending_acc, did_fullscreen, 120);
  eq_prune_slots_not_in_inventory();
  for (;;) {
    int i, listed = 0;
    int idx_map[MAX_INV];
    int def = 0, atk = 0, wgt = 0;
    screen_frame_begin_status(&frame, AETER_MAIN_MENU_VER);
    for (i = 0; i < EQ_SLOT_COUNT; i++) {
      int sdef, satk, swgt;
      if (!g_eq_slots[i][0]) continue;
      eq_compute_item_stats(g_eq_slots[i], i, &sdef, &satk, &swgt, NULL, NULL);
      def += sdef;
      atk += satk;
      wgt += swgt;
    }
    for (i = 0; i < EQ_INV_ROWS && i < g_inv_n; i++) idx_map[listed++] = i;

    {
      UiLineBuf sb;
      char sline[160];
      ui_line_init(&sb, sline, sizeof sline, 120);
      ui_line_append(&sb, " ");
      ui_line_append(&sb, "[ EQUIPMENT SLOTS ]");
      ui_line_append_width(&sb, "", 34);
      ui_line_append(&sb, "[ INVENTORY DATA ]");
      ui_line_pad(&sb);
      printf("%s%s%s\n", C_TITLE, sline, C_RESET);
    }

    for (i = 0; i < EQ_ROWS; i++) {
      switch (i) {
      case 0:
        eq_ui_emit_left_h48();
        break;
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
        eq_ui_emit_slot_row(i - 1);
        break;
      case 9:
        eq_ui_emit_left_h48();
        break;
      case 10:
        eq_ui_emit_left_spacer50();
        break;
      case 11:
        eq_ui_emit_combat_banner();
        break;
      case 12:
        eq_ui_emit_left_h48();
        break;
      case 13:
        eq_ui_emit_stat_armor_wgt(def, wgt);
        break;
      case 14:
        eq_ui_emit_stat_atk_mob(atk, wgt);
        break;
      default:
        eq_ui_emit_left_h48();
        break;
      }
      printf("    ");
      switch (i) {
      case 0:
        eq_ui_emit_right_h64();
        break;
      case 1:
        eq_ui_emit_inv_header();
        break;
      case 2:
        eq_ui_emit_inv_sep();
        break;
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
        if (i - 3 < g_inv_n && i - 3 < EQ_INV_ROWS)
          eq_ui_emit_inv_line(i - 3);
        else
          eq_ui_emit_inv_empty();
        break;
      default:
        eq_ui_emit_right_h64();
        break;
      }
      printf("\n");
    }

    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(
        " %sEQUIP <1-%d>  |  EQUIP <id> TO <slot>  |  UNEQUIP <slot>  |  DONE | "
        "RESUME%s\n",
        C_MUTED, EQ_INV_ROWS, C_RESET);
    printf(
        " %s# = table row; [id] = catalog id (same commands). Slots: head chest hands "
        "legs feet weapon offhand accessory.%s\n",
        C_MUTED, C_RESET);
    if (g_inv_n > EQ_INV_ROWS)
      printf(
          " %sPack: %d items — only rows 1-%d shown; use catalog [id] or "
          "inventory for full list.%s\n",
          C_MUTED, g_inv_n, EQ_INV_ROWS, C_RESET);
    screen_frame_prompt(&frame);

    if (!screen_read_command(line, sizeof line, 1)) break;
    if (!line[0]) continue;
    if (line_equals_one_of(line, kEqUiExit)) break;
    if (!strncmp(line, "unequip ", 8)) {
      int slot = eq_slot_index(line + 8);
      if (slot < 0) ui_block_pause("EQUIPMENT", "Unknown slot.");
      else {
        eq_unequip_to_inventory(slot, msg, sizeof msg);
        ui_block_pause("EQUIPMENT", msg);
      }
      continue;
    }
    if (!strncmp(line, "equip ", 6)) {
      long n;
      int slot = -1;
      char *to = strstr(line + 6, " to ");
      const AetItemCatalogEntry *cat = NULL;
      int inv_ix = -1;
      int k;
      n = strtol(line + 6, NULL, 10);
      if (to) slot = eq_slot_index(to + 4);
      cat = aet_item_catalog_by_id((int)n);
      if (cat) {
        for (k = 0; k < g_inv_n; k++) {
          if (str_ieq(g_inv[k], cat->slug)) {
            inv_ix = k;
            break;
          }
        }
      }
      if (inv_ix < 0 && n >= 1 && n <= listed) inv_ix = idx_map[(int)n - 1];
      if (inv_ix < 0) {
        char err[200];
        if (cat)
          snprintf(err, sizeof err, "%s", "That catalog item is not in your inventory.");
        else if (listed > 0)
          snprintf(
              err, sizeof err,
              "Use a catalog ID (left column) or pack row (1-%d) from the table on this screen.",
              listed);
        else
          snprintf(err, sizeof err, "%s",
                   "No matching item or empty pack for that input.");
        ui_block_pause("EQUIPMENT", err);
        continue;
      }
      if (slot < 0) {
        if (cat)
          slot = cat->slot_index;
        else
          slot = eq_infer_slot(g_inv[inv_ix]);
      }
      eq_equip_from_inventory_index(inv_ix, slot, msg, sizeof msg);
      ui_block_pause("EQUIPMENT", msg);
      continue;
    }
    ui_block_pause(
        "EQUIPMENT",
        "Commands: equip <row>, equip <id> to <slot>, unequip <slot>, done, resume.");
  }
  screen_frame_finish(&frame);
}

static void run_game_menu(void) {
  char line[INPUT_LINE_MAX];
  char body[AETER_HELP_BODY_CAP];
  char msg[1024];

  for (;;) {
    int row, j;
    char hdr_pad[640];
    char loc_short[48];
    char left_loc[256], left_turn[160], left_hp[320], left_score[160],
        left_coin[160], left_id[320];
    char right_opt[256];
    char hdr_l[160], hdr_r[160];
    const char *hp_c;
    int hp_blocks, k;
    float hp_ratio;

    clear_frame();
    printf("%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    {
      const char ttl[] = "PAUSE MENU";
      int pad = (120 - (int)(sizeof ttl - 1)) / 2;
      if (pad < 0) pad = 0;
      snprintf(hdr_pad, sizeof hdr_pad, "%s%*s%s%s", C_TITLE, pad, "", ttl,
               C_RESET);
      aet_sgr_pad_to(hdr_pad, sizeof hdr_pad, 120);
      printf("  %s\n", hdr_pad);
    }
    printf("%s%s%s\n\n", C_BORDER, AETER_RULE_120, C_RESET);

    snprintf(loc_short, sizeof loc_short, "%.36s",
             resolve_world_title(g_room));
    snprintf(left_loc, sizeof left_loc, "%s  Location : %s%s%s", C_MUTED,
             C_BOOT_OK, loc_short, C_RESET);
    snprintf(left_turn, sizeof left_turn, "%s  Turn     : %s%d%s", C_MUTED,
             C_HEADING, g_turns, C_RESET);

    hp_ratio =
        g_max_health > 0 ? (float)g_health / (float)g_max_health : 0.f;
    hp_blocks = (int)(hp_ratio * 10.f + 0.5f);
    if (hp_blocks > 10) hp_blocks = 10;
    if (hp_blocks < 0) hp_blocks = 0;
    if (hp_ratio > 0.6f)
      hp_c = C_BOOT_OK;
    else if (hp_ratio > 0.3f)
      hp_c = C_ITEM;
    else
      hp_c = g_use_color ? "\x1b[91;1m" : "";

    {
      char hb[16];
      for (k = 0; k < 10; k++) hb[k] = (char)(k < hp_blocks ? '#' : '.');
      hb[10] = '\0';
      snprintf(left_hp, sizeof left_hp,
               "%s  Health   : %s[%s]%s %s%d/%d%s", C_MUTED, hp_c, hb,
               C_RESET, C_HEADING, g_health, g_max_health, C_RESET);
    }
    snprintf(left_score, sizeof left_score, "%s  Score    : %s%d%s", C_MUTED,
             C_HEADING, g_score, C_RESET);
    {
      char purseb[24];
      currency_format_compact(g_coins, purseb, sizeof purseb);
      snprintf(left_coin, sizeof left_coin, "%s  Purse    : %s%s%s", C_MUTED,
               C_ITEM, purseb, C_RESET);
    }
    {
      AetPcSave pcm;
      pc_capture(&pcm);
      snprintf(left_id, sizeof left_id, "%s  Identity : %s%s — %s %s%s",
               C_MUTED, C_HEADING, pc_display_name(),
               pcm.race[0] ? pcm.race : "Human",
               pcm.class_[0] ? pcm.class_ : "adventurer", C_RESET);
    }

    snprintf(hdr_l, sizeof hdr_l, "%s [ CURRENT STATUS ]%s", C_HEADING,
             C_RESET);
    snprintf(hdr_r, sizeof hdr_r, "%s [ AVAILABLE ACTIONS ]%s", C_HEADING,
             C_RESET);

    for (row = 0; row < 12; row++) {
      const char *lc = "";
      char div_left[256];

      if (row == 0) {
        pause_print_row(hdr_l, hdr_r);
      } else if (row == 1) {
        pause_print_divider_cell();
      } else if (row == 2) {
        lc = left_loc;
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sResume Game%s", C_ITEM, " 1", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      } else if (row == 3) {
        lc = left_turn;
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sSave Quicksave%s", C_ITEM, " 2", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      } else if (row == 4) {
        lc = left_hp;
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sLoad Quicksave%s", C_ITEM, " 3", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      } else if (row == 5) {
        lc = left_score;
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sSave to Slot%s", C_ITEM, " 4", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      } else if (row == 6) {
        lc = left_coin;
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sLoad from Slot%s", C_ITEM, " 5", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      } else if (row == 7) {
        snprintf(div_left, sizeof div_left, "%s%*s%s", C_BORDER, PAUSE_CELL_W,
                 "", C_RESET);
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sSave Manager (A:\\)%s", C_ITEM, " 6", C_HEADING,
                 C_RESET);
        pause_print_row(div_left, right_opt);
      } else if (row == 8) {
        lc = left_id;
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sSystem Help%s", C_ITEM, " 7", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      } else if (row == 9) {
        lc = "";
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sSettings%s", C_ITEM, " 8", C_HEADING, C_RESET);
        pause_print_row(lc, right_opt);
      } else if (row == 10) {
        lc = "";
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sReturn to Title%s", C_ITEM, " 9", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      } else {
        lc = "";
        snprintf(right_opt, sizeof right_opt,
                 "%s  [%s] %sQuit to System%s", C_ITEM, "10", C_HEADING,
                 C_RESET);
        pause_print_row(lc, right_opt);
      }
    }

    printf("\n%s%s%s\n", C_BORDER, AETER_RULE_120, C_RESET);
    printf(" %sEnter choice (1-10) or command (e.g., SAVE):%s\n", C_MUTED,
           C_RESET);
    printf(" %s>>%s ", C_TITLE, C_RESET);
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) {
      g_return_to_menu = 1;
      return;
    }
    chomp_line(line);
    strip_trailing_space(line);
    for (j = 0; line[j]; j++)
      line[j] = (char)tolower((unsigned char)line[j]);

    if (!strcmp(line, "10") || !strcmp(line, "quit") ||
        !strcmp(line, "exit") || !strcmp(line, "q")) {
      printf("Goodbye.\n");
      exit(0);
    }
    if (!line[0] || !strcmp(line, "1") || !strcmp(line, "resume") ||
        !strcmp(line, "back") || !strcmp(line, "return") ||
        !strcmp(line, "game")) {
      return_to_game_screen();
      return;
    }
    if (!strcmp(line, "2") || !strcmp(line, "quicksave") ||
        !strcmp(line, "qsave")) {
      save_game(msg, sizeof msg);
      ui_block_pause("SAVE", msg);
      return_to_game_screen();
      return;
    }
    if (!strcmp(line, "3") || !strcmp(line, "quickload") ||
        !strcmp(line, "qload")) {
      load_game(msg, sizeof msg);
      ui_block_pause("LOAD", msg);
      return_to_game_screen();
      return;
    }
    if (!strcmp(line, "4") || !strcmp(line, "save") ||
        !strcmp(line, "save slot")) {
      int slot;
      printf("\nSave slot (1-%d): ", SAVE_SLOT_COUNT);
      fflush(stdout);
      if (!fgets(line, sizeof line, stdin)) {
        return_to_game_screen();
        return;
      }
      chomp_line(line);
      strip_trailing_space(line);
      for (j = 0; line[j]; j++)
        line[j] = (char)tolower((unsigned char)line[j]);
      if (!parse_save_slot(line, &slot)) {
        snprintf(msg, sizeof msg, "Use a slot number from 1 to %d.",
                 SAVE_SLOT_COUNT);
        ui_block_pause("SAVE SLOT", msg);
        continue;
      }
      save_game_slot(slot, msg, sizeof msg);
      ui_block_pause("SAVE SLOT", msg);
      return_to_game_screen();
      return;
    }
    if (!strcmp(line, "5") || !strcmp(line, "load") ||
        !strcmp(line, "load slot")) {
      int slot;
      printf("\nLoad slot (1-%d): ", SAVE_SLOT_COUNT);
      fflush(stdout);
      if (!fgets(line, sizeof line, stdin)) {
        return_to_game_screen();
        return;
      }
      chomp_line(line);
      strip_trailing_space(line);
      for (j = 0; line[j]; j++)
        line[j] = (char)tolower((unsigned char)line[j]);
      if (!parse_save_slot(line, &slot)) {
        snprintf(msg, sizeof msg, "Use a slot number from 1 to %d.",
                 SAVE_SLOT_COUNT);
        ui_block_pause("LOAD SLOT", msg);
        continue;
      }
      load_game_slot(slot, msg, sizeof msg);
      ui_block_pause("LOAD SLOT", msg);
      return_to_game_screen();
      return;
    }
    if (!strcmp(line, "6") || !strcmp(line, "slots") ||
        !strcmp(line, "saves") || !strcmp(line, "manager")) {
      if (run_save_manager_ui(NULL, 1)) {
        return_to_game_screen();
        return;
      }
      continue;
    }
    if (!strcmp(line, "7") || !strcmp(line, "help") || !strcmp(line, "?")) {
      fill_help_text(body, sizeof body);
      ui_block_pause("HELP", body);
      continue;
    }
    if (!strcmp(line, "8") || !strcmp(line, "settings")) {
      run_settings_ui();
      continue;
    }
    if (!strcmp(line, "9") || !strcmp(line, "title") ||
        !strcmp(line, "main") || !strcmp(line, "main menu")) {
      g_return_to_menu = 1;
      return;
    }
    ui_block_pause(
        "MENU",
        "Unknown choice. Use 1-10 or resume | qsave | save | load | manager | "
        "help | settings | title | quit.\n");
  }
}

static void process_command(char *line, char *msg, size_t msgcap,
                            int *turn_advance, const char *pending_acc,
                            int *did_fullscreen) {
  char body[AETER_HELP_BODY_CAP];
  const char *slug;

  if (disambig_try_followup(line, msg, msgcap, turn_advance)) return;
  if (conv_try_followup(line, msg, msgcap, turn_advance)) return;
  static const char *const kQuit[] = {"quit", "q", "exit", NULL};
  static const char *const kMenu[] = {"menu", "mainmenu", "main menu", NULL};
  static const char *const kHelp[] = {"help", "?", NULL};
  static const char *const kHelpMod[] = {"help modding", "help mods", NULL};
  static const char *const kForge[] = {"forge", "crafting", NULL};
  static const char *const kCls[] = {"clear", "cls", NULL};
  static const char *const kVerbose[] = {"verbose", "long", NULL};
  static const char *const kBrief[] = {"brief", "short", NULL};
  static const char *const kRecap[] = {"recap", "transcript", NULL};
  static const char *const kRecapClr[] = {"recap clear", "clear recap", NULL};
  static const char *const kLights[] = {"lights", "lighting", NULL};
  static const char *const kAbout[] = {"about", "credits", NULL};
  static const char *const kVer[] = {"version", "ver", NULL};
  static const char *const kModsReload[] = {"mods reload", "reload mods", NULL};
  static const char *const kModsList[] = {"mods list", "list mods", NULL};
  static const char *const kModsDoctor[] = {"mods doctor", "mods verify",
                                             "mods repair", NULL};
  static const char *const kInv[] = {"inventory", "i", "inv", "pack", NULL};
  static const char *const kCoins[] = {"coins", "wallet", "money", "purse",
                                       NULL};
  static const char *const kTrailClr[] = {"trail clear", "clear trail", NULL};
  static const char *const kProgress[] = {"progress", "visited", "seen", NULL};
  static const char *const kWhyBlocked[] = {"why blocked", "why cant i move",
                                            "why can't i move", NULL};
  static const char *const kCausalityFacet[] = {
      "causality movement", "causality social", "causality save",
      "causality parser", NULL};
  static const char *const kErrDiagClr[] = {"errors clear",
                                            "diagnostics clear", NULL};
  static const char *const kCausalityRecent[] = {
      "causality recent", "causality summary", "why recent changes", NULL};
  static const char *const kRoomWords[] = {"describe", "blurb", "room", NULL};
  static const char *const kWpList[] = {"waypoints", "waypoint", "waystones",
                                        "waystone", "nexus", "fasttravel",
                                        NULL};
  static const char *const kCausalityBare[] = {"causality", "because", NULL};
  static const char *const kCausalityExplain[] = {"causality explain",
                                                   "because explain", NULL};
  static const char *const kWaitUntil[] = {"wait until ", "rest until ", NULL};
  char parse_before[INPUT_LINE_MAX];

  *did_fullscreen = 0;
  *turn_advance = 1;
  msg[0] = '\0';
  memset(&g_intent, 0, sizeof g_intent);

  strncpy(parse_before, line, sizeof parse_before - 1);
  parse_before[sizeof parse_before - 1] = '\0';
  strip_natural_prefixes(line);
  strip_motion_verb(line);
  extract_intent_modifiers(line, &g_intent);
  if (g_intent.present) {
    char ibuf[96];
    format_intent_suffix(ibuf, sizeof ibuf, &g_intent);
    if (ibuf[0]) causal_push("intent", ibuf + 1);
  }
  normalize_aliases(line);
  normalize_parser_intent(line);
  if (strcmp(parse_before, line) != 0) {
    char pbuf[420];
    snprintf(pbuf, sizeof pbuf, "%.*s -> %.*s", 180, parse_before, 180, line);
    causal_push("parser-normalize", pbuf);
  }

  if (line_equals_one_of(line, kQuit)) {
    printf("Thanks for playing.\n");
    exit(0);
  }

  if (line_equals_one_of(line, kMenu)) {
    *turn_advance = 0;
    run_game_menu();
    *did_fullscreen = 1;
    return;
  }

  if (line_equals_one_of(line, kHelp)) {
    *turn_advance = 0;
    fill_help_text(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_help_suffix());
    ui_fullscreen("HELP", body, pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kHelpMod)) {
    *turn_advance = 0;
    ui_modding_guide_pager(pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kForge)) {
    *turn_advance = 0;
    run_material_forge(pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kCls)) {
    *turn_advance = 0;
    paint_normal();
    return;
  }

  if (line_equals_one_of(line, kVerbose)) {
    *turn_advance = 0;
    g_verbose_room = 1;
    snprintf(msg, msgcap, "Full room descriptions on (main window).");
    return;
  }
  if (line_equals_one_of(line, kBrief)) {
    *turn_advance = 0;
    g_verbose_room = 0;
    snprintf(msg, msgcap,
             "Brief room blurbs on — first ~140 chars; use describe for full "
             "text.");
    return;
  }

  if (line_equals_one_of(line, kRecap)) {
    *turn_advance = 0;
    format_recap_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_recap_suffix());
    ui_fullscreen("RECAP", body, pending_acc, did_fullscreen);
    return;
  }
  if (line_equals_one_of(line, kRecapClr)) {
    *turn_advance = 0;
    g_recap_n = 0;
    snprintf(msg, msgcap, "Recap cleared.");
    return;
  }

  if (line_equals_one_of(line, kLights)) {
    *turn_advance = 0;
    format_lights_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_lights_suffix());
    ui_fullscreen("LIGHT SOURCES", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "lore") || !strcmp(line, "lore primer") ||
      !strcmp(line, "world lore")) {
    *turn_advance = 0;
    format_lore_body(body, sizeof body, NULL);
    ui_fullscreen("LORE", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "lore ", 5)) {
    *turn_advance = 0;
    format_lore_body(body, sizeof body, line + 5);
    ui_fullscreen("LORE", body, pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kAbout)) {
    *turn_advance = 0;
    format_about_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_about_suffix());
    ui_fullscreen("ABOUT", body, pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kVer)) {
    *turn_advance = 0;
    snprintf(msg, msgcap,
             "Aeternitas64 stdin port — %d locations in the compiled dataset. "
             "Regenerate data: py -3 tools/extract_world_tables_from_exe.py; py -3 "
             "tools/build_world_c_from_recovered_json.py\n"
             "Quick save / load file: %s",
             WORLD_ROOM_COUNT, g_save_path);
    return;
  }

  if (line_equals_one_of(line, kModsReload)) {
    *turn_advance = 0;
    mods_reload_same_rules();
    aet_mods_format_status(body, sizeof body);
    ui_fullscreen("MODS RELOADED", body, pending_acc, did_fullscreen);
    return;
  }

  {
    static const struct {
      const char *p;
      int n;
    } kModsPath[] = {{"mods directory ", 15}, {"mods path ", 10}, {NULL, 0}};
    int mi;
    for (mi = 0; kModsPath[mi].p; mi++) {
      if (!strncmp(line, kModsPath[mi].p, (size_t)kModsPath[mi].n)) {
        const char *p = line + kModsPath[mi].n;
        *turn_advance = 0;
        while (*p == ' ') p++;
        if (!p[0]) {
          snprintf(msg, msgcap,
                   "Usage: mods directory <folder>  (full path to mod packs "
                   "root)");
          return;
        }
        strncpy(g_mods_override, p, sizeof g_mods_override - 1);
        g_mods_override[sizeof g_mods_override - 1] = '\0';
        mods_reload_same_rules();
        snprintf(msg, msgcap, "Mods folder set to \"%s\" and reloaded.",
                 g_mods_override);
        return;
      }
    }
  }

  if (line_equals_one_of(line, kModsList)) {
    *turn_advance = 0;
    aet_mods_format_load_order(body, sizeof body);
    ui_fullscreen("MOD PACK ORDER", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "mods doctor verbose") || !strcmp(line, "mods verify verbose") ||
      !strcmp(line, "mods repair verbose")) {
    char mp[520];
    char st[AETER_HELP_BODY_CAP];
    char warn[3072];
    *turn_advance = 0;
    mods_resolve_root(mp, sizeof mp);
    aet_mod_bootstrap_prepare_runtime(g_save_path, mp, &g_bootstrap_status);
    aet_mods_reload(mp);
    format_bootstrap_status(body, sizeof body, mp);
    aet_mods_format_load_warnings(warn, sizeof warn);
    if (warn[0]) {
      size_t lo = strlen(body);
      snprintf(body + lo, sizeof body - lo, "\n%s", warn);
    }
    aet_mods_format_status(st, sizeof st);
    {
      body_append(body, sizeof body, "\n--- Mod status ---\n%s", st);
    }
    ui_fullscreen("MODS DOCTOR", body, pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kModsDoctor)) {
    char mp[520];
    char warn[3072];
    *turn_advance = 0;
    mods_resolve_root(mp, sizeof mp);
    aet_mod_bootstrap_prepare_runtime(g_save_path, mp, &g_bootstrap_status);
    aet_mods_reload(mp);
    format_bootstrap_status(body, sizeof body, mp);
    aet_mods_format_load_warnings(warn, sizeof warn);
    if (warn[0]) {
      size_t lo = strlen(body);
      snprintf(body + lo, sizeof body - lo, "\n%s", warn);
    }
    ui_fullscreen("MODS DOCTOR", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "inventory list") || !strcmp(line, "inv list") ||
      !strcmp(line, "pack list") || !strcmp(line, "inventory") ||
      !strcmp(line, "inv") || !strcmp(line, "pack")) {
    int sort_mode = 0;
    const char *filter = NULL;
    if (!strcmp(line, "inventory") || !strcmp(line, "inv") ||
        !strcmp(line, "pack"))
      run_equipment_inventory_ui(pending_acc, did_fullscreen);
    else {
      *turn_advance = 0;
      format_inventory_list_body(body, sizeof body, sort_mode, filter);
      ui_fullscreen("INVENTORY", body, pending_acc, did_fullscreen);
    }
    return;
  }
  if (!strcmp(line, "inventory sort") || !strcmp(line, "inv sort") ||
      !strcmp(line, "pack sort")) {
    *turn_advance = 0;
    format_inventory_list_body(body, sizeof body, 0, NULL);
    ui_fullscreen("INVENTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "inventory sort ", 15) ||
      !strncmp(line, "inv sort ", 9) || !strncmp(line, "pack sort ", 10)) {
    const char *mode =
        !strncmp(line, "inventory sort ", 15)
            ? line + 15
            : (!strncmp(line, "inv sort ", 9) ? line + 9 : line + 10);
    int sort_mode = 0;
    while (*mode == ' ') mode++;
    if (str_ieq(mode, "weight") || str_ieq(mode, "bulk") ||
        str_ieq(mode, "heavy"))
      sort_mode = 1;
    else if (str_ieq(mode, "type") || str_ieq(mode, "category") ||
             str_ieq(mode, "cat"))
      sort_mode = 2;
    *turn_advance = 0;
    format_inventory_list_body(body, sizeof body, sort_mode, NULL);
    ui_fullscreen("INVENTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "inventory find ", 15) ||
      !strncmp(line, "inv find ", 9) || !strncmp(line, "pack find ", 10) ||
      !strncmp(line, "inventory search ", 17) ||
      !strncmp(line, "inv search ", 11)) {
    const char *q;
    int sort_mode = 0;
    if (!strncmp(line, "inventory find ", 15))
      q = line + 15;
    else if (!strncmp(line, "inv find ", 9))
      q = line + 9;
    else if (!strncmp(line, "pack find ", 10))
      q = line + 10;
    else if (!strncmp(line, "inventory search ", 17))
      q = line + 17;
    else
      q = line + 11;
    while (*q == ' ') q++;
    if (!q[0]) {
      snprintf(msg, msgcap, "Usage: inventory find <text>");
      return;
    }
    *turn_advance = 0;
    format_inventory_list_body(body, sizeof body, sort_mode, q);
    ui_fullscreen("INVENTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "crafting guide") || !strcmp(line, "craft guide") ||
      !strcmp(line, "forge guide") || !strcmp(line, "forge help") ||
      !strcmp(line, "crafting help") || !strcmp(line, "recipe") ||
      !strcmp(line, "recipes") || !strcmp(line, "crafting recipes")) {
    *turn_advance = 0;
    snprintf(msg, msgcap,
             "No recipe book — only trial, scrap, and whatever the bench shows "
             "when you use  forge .");
    return;
  }
  if (line_equals_one_of(line, kInv)) {
    *turn_advance = 0;
    run_equipment_inventory_ui(pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "score")) {
    AetWorldClock wc;
    char t[32];
    char role[96];
    char pr[64];
    char purseb[48];
    AetPcSave ps;
    *turn_advance = 0;
    get_world_clock(&wc);
    format_clock_time(t, sizeof t, &wc);
    pc_capture(&ps);
    pc_fill_narrative_defaults(&ps);
    pc_format_role_phrase(role, sizeof role);
    pc_format_pronouns_short(ps.gender[0] ? ps.gender : "they", pr, sizeof pr);
    currency_format_long(g_coins, purseb, sizeof purseb);
    snprintf(body, sizeof body,
             "%s · %s\n%s\n\n"
             "Health %d / %d\nScore %d\nPurse %s\nTurns %d\nTime %s (%s), day "
             "%d, %s\n",
             pc_display_name(), role, pr, g_health, g_max_health, g_score,
             purseb, g_turns, t, wc.period, wc.day, wc.season);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_score_suffix());
    ui_fullscreen("SCORE", body, pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kCoins)) {
    char purseb[48];
    *turn_advance = 0;
    currency_format_long(g_coins, purseb, sizeof purseb);
    snprintf(msg, msgcap, "%s — purse: %s.", pc_display_name(), purseb);
    return;
  }

  if (line_equals_one_of(line, kTrailClr)) {
    *turn_advance = 0;
    g_hist_n = 0;
    snprintf(msg, msgcap, "%s — movement trail cleared.", pc_display_name());
    return;
  }

  if (!strcmp(line, "trail stats")) {
    *turn_advance = 0;
    snprintf(msg, msgcap,
             "%s — trail: %d step(s) you can retrace with \"back\".",
             pc_display_name(), g_hist_n);
    return;
  }

  if (!strncmp(line, "trail ", 6)) {
    long n;
    char *endp;
    const char *arg = line + 6;
    while (*arg == ' ') arg++;
    *turn_advance = 0;
    if (!*arg) {
      snprintf(msg, msgcap, "Use: trail <1-25>  or  trail.");
      return;
    }
    n = strtol(arg, &endp, 10);
    while (*endp == ' ') endp++;
    if (endp == arg || *endp != '\0' || n < 1 || n > 25) {
      snprintf(msg, msgcap, "Use: trail <1-25>  or  trail.");
      return;
    }
    cmd_trail(body, sizeof body, (int)n);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_trail_suffix());
    ui_fullscreen("TRAIL", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "trail")) {
    *turn_advance = 0;
    cmd_trail(body, sizeof body, 0);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_trail_suffix());
    ui_fullscreen("TRAIL", body, pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kProgress)) {
    *turn_advance = 0;
    format_progress_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_progress_suffix());
    ui_fullscreen("PROGRESS", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "character brief") || !strcmp(line, "char brief") ||
      !strcmp(line, "sheet brief")) {
    *turn_advance = 0;
    pc_format_summary(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_sheet_suffix());
    ui_fullscreen("CHARACTER (brief)", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "character") || !strcmp(line, "char") ||
      !strcmp(line, "sheet")) {
    static char portrait[AETER_CHARACTER_PORTRAIT_CAP];
    AetPcSave snap;
    const char *psfx;
    char xpsfx[4096];
    char *merged;
    size_t pl, sl;
    *turn_advance = 0;
    pc_capture(&snap);
    aet_describe_pc(&snap, portrait, sizeof portrait);
    psfx = aet_mods_character_portrait_suffix();
    if (!psfx || !psfx[0]) {
      ui_fullscreen("CHARACTER", portrait, pending_acc, did_fullscreen);
      return;
    }
    expand_mod_overlay_flat(psfx, xpsfx, sizeof xpsfx);
    pl = strlen(portrait);
    sl = strlen(xpsfx);
    merged = (char *)malloc(pl + sl + 80);
    if (!merged) {
      ui_fullscreen("CHARACTER", portrait, pending_acc, did_fullscreen);
      return;
    }
    (void)snprintf(merged, pl + sl + 80,
                   "%s\n\n--- DLC / mod notes ---\n%s", portrait, xpsfx);
    ui_fullscreen("CHARACTER", merged, pending_acc, did_fullscreen);
    free(merged);
    return;
  }

  if (!strcmp(line, "skills") || !strcmp(line, "skill") ||
      !strcmp(line, "aptitudes") || !strcmp(line, "aptitude")) {
    *turn_advance = 0;
    format_aptitudes_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_aptitudes_suffix());
    ui_fullscreen("APTITUDES", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "reputation") || !strcmp(line, "rep") ||
      !strcmp(line, "standing")) {
    *turn_advance = 0;
    format_reputation_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_reputation_suffix());
    ui_fullscreen("REPUTATION", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "loadout") || !strcmp(line, "gear") ||
      !strcmp(line, "equipment") || !strcmp(line, "outfit")) {
    *turn_advance = 0;
    run_equipment_inventory_ui(pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "traits") || !strcmp(line, "trait") ||
      !strcmp(line, "personality")) {
    *turn_advance = 0;
    format_traits_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_traits_suffix());
    ui_fullscreen("TRAITS", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "momentum") || !strcmp(line, "arc") ||
      !strcmp(line, "progression")) {
    *turn_advance = 0;
    format_momentum_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_momentum_suffix());
    ui_fullscreen("MOMENTUM", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "perks") || !strcmp(line, "perk")) {
    *turn_advance = 0;
    format_perks_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_perks_suffix());
    ui_fullscreen("PERKS", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "voice") || !strcmp(line, "speech") ||
      !strcmp(line, "vocals") || !strcmp(line, "pronouns")) {
    *turn_advance = 0;
    format_voice_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_voice_suffix());
    ui_fullscreen("VOICE & PRONOUNS", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "bio") || !strcmp(line, "backstory") ||
      !strcmp(line, "biography")) {
    *turn_advance = 0;
    format_bio_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_bio_suffix());
    ui_fullscreen("BIOGRAPHY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "tainting") || !strcmp(line, "corruption") ||
      !strcmp(line, "taint")) {
    *turn_advance = 0;
    format_tainting_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_tainting_suffix());
    ui_fullscreen("TAINTING", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "relationship history") ||
      !strcmp(line, "bond history") || !strcmp(line, "rapport history") ||
      !strcmp(line, "social history")) {
    *turn_advance = 0;
    format_relationship_history_body(body, sizeof body, NULL);
    ui_fullscreen("RELATIONSHIP HISTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "relationship history ", 21)) {
    *turn_advance = 0;
    format_relationship_history_body(body, sizeof body, line + 21);
    ui_fullscreen("RELATIONSHIP HISTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "bond history ", 13)) {
    *turn_advance = 0;
    format_relationship_history_body(body, sizeof body, line + 13);
    ui_fullscreen("RELATIONSHIP HISTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "rapport history ", 16)) {
    *turn_advance = 0;
    format_relationship_history_body(body, sizeof body, line + 16);
    ui_fullscreen("RELATIONSHIP HISTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "rapport") || !strcmp(line, "relationships") ||
      !strcmp(line, "bonds")) {
    *turn_advance = 0;
    format_rapport_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_rapport_suffix());
    ui_fullscreen("RAPPORT", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "vitals") || !strcmp(line, "wellness") ||
      !strcmp(line, "hp")) {
    *turn_advance = 0;
    format_vitals_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_vitals_suffix());
    ui_fullscreen("VITALS", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "status") || !strcmp(line, "stat")) {
    AetWorldClock wc;
    char t[32];
    char who[160];
    char pr[64];
    char purseb[48];
    AetPcSave pcs;
    const char *lit;
    int si, n_eq = 0;
    *turn_advance = 0;
    get_world_clock(&wc);
    format_clock_time(t, sizeof t, &wc);
    if (room_too_dark_to_see())
      lit = "DARK (no light in inventory)";
    else if (world_room_is_dark(g_room))
      lit = "dark zone — you have a light";
    else
      lit = "light OK";
    pc_capture(&pcs);
    pc_fill_narrative_defaults(&pcs);
    pc_format_pronouns_short(pcs.gender[0] ? pcs.gender : "they", pr, sizeof pr);
    currency_format_long(g_coins, purseb, sizeof purseb);
    snprintf(who, sizeof who, "%s (%s %s)",
             pcs.name[0] ? pcs.name : "Adventurer",
             pcs.race[0] ? pcs.race : "Human",
             pcs.class_[0] ? pcs.class_ : "adventurer");
    snprintf(
        body, sizeof body,
        "Character: %s\n"
        "Pronouns: %s\n"
        "Location: %s  [%s]\nRegion: %s\nTime: %s (%s), day %d, %s\n"
        "Weather: %s, %dC\nLighting: %s\nRoom text: %s\n"
        "Readied: %s\n"
        "It / last examined: %s\n"
        "Inventory slots: %d / %d\n"
        "Health: %d / %d\n"
        "Purse: %s\n"
        "Score: %d\nTurns: %d\n",
        who, pr, resolve_world_title(g_room), world_slug(g_room),
        world_region(g_room)[0] ? world_region(g_room) : "(unspecified)", t,
        wc.period, wc.day, wc.season, wc.weather, wc.temp_c, lit,
        g_verbose_room ? "full (verbose)" : "short (brief)",
        (g_ready_item[0] && inv_has(g_ready_item)) ? g_ready_item : "—",
        (g_last_focus[0] && inv_has(g_last_focus)) ? g_last_focus : "—",
        g_inv_n, MAX_INV, g_health, g_max_health, purseb, g_score, g_turns);
    body_append(body, sizeof body, "\nEquipped\n");
    for (si = 0; si < EQ_SLOT_COUNT; si++) {
      if (!g_eq_slots[si][0]) continue;
      n_eq++;
      body_append(body, sizeof body, "  %-10s  %s\n", EQ_SLOT_NAME[si],
                  g_eq_slots[si]);
    }
    if (!n_eq) body_append(body, sizeof body, "  (nothing worn)\n");
    body_append_people_here(body, sizeof body);
    body_append(body, sizeof body, "Pack bulk (est.): ~%u\n",
                inventory_total_bulk());
    if (g_barter_mode != BARTER_NONE && g_barter_item[0])
      body_append(body, sizeof body, "Barter: pending quote on %s\n",
                  g_barter_item);
    if (g_autosave_enabled)
      body_append(body, sizeof body, "Autosave shadow: on (after each turn)\n");
    append_causal_status_overlay(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_status_suffix());
    ui_fullscreen("STATUS", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "unstick") || !strcmp(line, "hints")) {
    *turn_advance = 0;
    format_contextual_hints_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_hints_panel_suffix());
    ui_fullscreen("HINTS", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "errors") || !strcmp(line, "healthcheck") ||
      !strcmp(line, "diagnostics") || !strcmp(line, "diag")) {
    *turn_advance = 0;
    format_diag_health_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_diagnostics_suffix());
    ui_fullscreen("DIAGNOSTICS", body, pending_acc, did_fullscreen);
    return;
  }
  if (line_equals_one_of(line, kCausalityBare)) {
    *turn_advance = 0;
    format_causality_body(body, sizeof body, NULL);
    ui_fullscreen("CAUSALITY", body, pending_acc, did_fullscreen);
    return;
  }
  if (line_equals_one_of(line, kCausalityExplain)) {
    *turn_advance = 0;
    format_causality_explain_body(body, sizeof body);
    ui_fullscreen("CAUSALITY EXPLAIN", body, pending_acc, did_fullscreen);
    return;
  }
  if (line_equals_one_of(line, kCausalityRecent)) {
    *turn_advance = 0;
    format_causality_recent_body(body, sizeof body);
    ui_fullscreen("CAUSALITY SUMMARY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "causality lastturn")) {
    int t = g_turns > 0 ? g_turns - 1 : 0;
    *turn_advance = 0;
    format_causality_turn_body(body, sizeof body, t);
    ui_fullscreen("CAUSALITY TURN", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "causality turn ", 15)) {
    const char *r = line + 15;
    char *end = NULL;
    long tv;
    while (*r == ' ') r++;
    tv = strtol(r, &end, 10);
    *turn_advance = 0;
    if (!r[0] || !end || end == r || tv < 0) {
      snprintf(msg, msgcap, "Use: causality turn <non-negative turn number>.");
      return;
    }
    format_causality_turn_body(body, sizeof body, (int)tv);
    ui_fullscreen("CAUSALITY TURN", body, pending_acc, did_fullscreen);
    return;
  }
  if (line_equals_one_of(line, kWhyBlocked)) {
    *turn_advance = 0;
    format_why_blocked_body(body, sizeof body);
    ui_fullscreen("WHY BLOCKED", body, pending_acc, did_fullscreen);
    return;
  }
  if (line_equals_one_of(line, kCausalityFacet)) {
    const char *q = line + 10;
    *turn_advance = 0;
    while (*q == ' ') q++;
    format_causality_body(body, sizeof body, q);
    ui_fullscreen("CAUSALITY", body, pending_acc, did_fullscreen);
    return;
  }
  {
    static const struct {
      const char *p;
      int n;
    } kCausalPhrase[] = {{"causality ", 10}, {"because ", 8}, {NULL, 0}};
    int ci;
    for (ci = 0; kCausalPhrase[ci].p; ci++) {
      if (!strncmp(line, kCausalPhrase[ci].p, (size_t)kCausalPhrase[ci].n)) {
        const char *q = line + kCausalPhrase[ci].n;
        *turn_advance = 0;
        while (*q == ' ') q++;
        format_causality_body(body, sizeof body, q);
        ui_fullscreen("CAUSALITY", body, pending_acc, did_fullscreen);
        return;
      }
    }
  }
  if (line_equals_one_of(line, kErrDiagClr)) {
    *turn_advance = 0;
    diag_clear();
    snprintf(msg, msgcap, "Session issue log cleared.");
    return;
  }
  if (!strcmp(line, "causality clear")) {
    *turn_advance = 0;
    causal_clear();
    snprintf(msg, msgcap, "Causality trace cleared.");
    return;
  }

  if (!strcmp(line, "history") || !strcmp(line, "command history") ||
      !strcmp(line, "cmd history")) {
    *turn_advance = 0;
    format_cmd_history_body(body, sizeof body);
    ui_fullscreen("HISTORY", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "utilities") || !strcmp(line, "utility") ||
      !strcmp(line, "room utilities") || !strcmp(line, "room objects") ||
      !strcmp(line, "objects")) {
    *turn_advance = 0;
    format_utilities_body(body, sizeof body);
    ui_fullscreen("UTILITIES", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "hint")) {
    *turn_advance = 0;
    format_contextual_hints_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_hints_panel_suffix());
    ui_fullscreen("HINT", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "z")) {
    snprintf(msg, msgcap, "Time passes.");
    return;
  }
  {
    int ui;
    for (ui = 0; kWaitUntil[ui]; ui++) {
      if (!strncmp(line, kWaitUntil[ui], 11)) {
        const char *period = line + 11;
        AetWorldClock wc;
        int target_hour;
        int hours;
        while (*period == ' ') period++;
        target_hour = period_start_hour(period);
        if (target_hour < 0) {
          *turn_advance = 0;
          snprintf(msg, msgcap,
                   "Use: wait until <morning|afternoon|evening|night>.");
          return;
        }
        get_world_clock(&wc);
        if (str_ieq(wc.period, period)) {
          *turn_advance = 0;
          snprintf(msg, msgcap, "It is already %s.", wc.period);
          return;
        }
        hours = hours_until_hour(wc.hour, target_hour);
        if (hours <= 0) hours = 24;
        *turn_advance = hours * 6;
        snprintf(msg, msgcap, "You wait %d hour%s, until %s.", hours,
                 hours == 1 ? "" : "s", period);
        return;
      }
    }
  }
  if (!strncmp(line, "wait ", 5)) {
    const char *r = line + 5;
    char *end = NULL;
    long n;
    while (*r == ' ') r++;
    if (!*r) {
      snprintf(msg, msgcap, "Wait how long? (e.g. wait 3)");
      return;
    }
    n = strtol(r, &end, 10);
    if (!end || end == r || n < 1 || n > 9999) {
      *turn_advance = 0;
      snprintf(msg, msgcap,
               "Use: wait, wait <number>, wait <number> hours, or wait until "
               "<period>.");
      return;
    }
    while (end && *end == ' ') end++;
    if (end && (!strcmp(end, "hour") || !strcmp(end, "hours"))) {
      if (n > 1666) {
        *turn_advance = 0;
        snprintf(msg, msgcap, "That is too long to wait at once.");
        return;
      }
      *turn_advance = (int)n * 6;
      snprintf(msg, msgcap, n == 1 ? "You wait 1 hour."
                                   : "You wait %d hours.",
               (int)n);
    } else if (end && *end) {
      *turn_advance = 0;
      snprintf(msg, msgcap,
               "Use: wait <number> for turns, or wait <number> hours.");
    } else {
      *turn_advance = (int)n;
      snprintf(msg, msgcap,
               n == 1 ? "You let a turn pass." : "You wait %d turns.",
               (int)n);
    }
    return;
  }
  if (!strcmp(line, "wait")) {
    snprintf(msg, msgcap, "Time passes.");
    return;
  }
  if (!strcmp(line, "rest") || !strcmp(line, "sleep")) {
    int was = g_health;
    if (!strcmp(line, "sleep"))
      player_heal(3);
    else
      player_heal(1);
    if (g_health > was) {
      snprintf(
          msg, msgcap,
          "%s You feel a little more whole. (%d / %d)",
          !strcmp(line, "sleep")
              ? "You sleep lightly; small hurts ease."
              : "You rest; your breath finds a slower gear.",
          g_health, g_max_health);
    } else {
      snprintf(
          msg, msgcap,
          !strcmp(line, "sleep")
              ? "You sleep lightly. Nothing shifts, but your thoughts quiet."
              : "You rest for a moment.");
    }
    return;
  }

  if (!strcmp(line, "retreat") || !strcmp(line, "back") ||
      !strncmp(line, "back ", 5)) {
    int steps = 1;
    int total_back;
    const char *r;
    if (!strncmp(line, "back ", 5)) {
      r = line + 5;
      while (*r == ' ') r++;
      if (*r) {
        char *end = NULL;
        long n = strtol(r, &end, 10);
        if (!end || end == r || n < 1) {
          *turn_advance = 0;
          snprintf(msg, msgcap,
                   "Use: back, retreat, or back <number> (steps to retrace).");
          return;
        }
        if (n > g_hist_n) {
          *turn_advance = 0;
          snprintf(msg, msgcap,
                   "You can only go back %d step(s) right now.", g_hist_n);
          return;
        }
        steps = (int)n;
      }
    }
    if (g_hist_n <= 0) {
      snprintf(msg, msgcap, "You cannot go back further.");
      return;
    }
    if (steps > g_hist_n) steps = g_hist_n;
    total_back = steps;
    while (steps-- > 0) {
      g_hist_n--;
      g_room = g_hist[g_hist_n];
    }
    if (total_back == 1)
      snprintf(msg, msgcap, "You retrace your steps.");
    else
      snprintf(msg, msgcap, "You retrace %d rooms.", total_back);
    return;
  }

  if (!strcmp(line, "exits") || !strncmp(line, "exits ", 6)) {
    const char *mode = NULL;
    *turn_advance = 0;
    if (!strncmp(line, "exits ", 6)) {
      mode = line + 6;
      while (*mode == ' ') mode++;
      if (!strcmp(mode, "locks")) mode = "locked";
      if (!strcmp(mode, "people")) mode = "npc";
    }
    format_exits_screen(body, sizeof body, mode);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_exits_suffix());
    ui_fullscreen("EXITS", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "lockcheck") || !strcmp(line, "locks") ||
      !strcmp(line, "where locks") || !strcmp(line, "where locked")) {
    *turn_advance = 0;
    format_lockcheck_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_lockcheck_suffix());
    ui_fullscreen("LOCKCHECK", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "noise") || !strcmp(line, "stealth") ||
      !strcmp(line, "suspicion")) {
    *turn_advance = 0;
    format_noise_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_noise_suffix());
    ui_fullscreen("NOISE", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "intent") || !strcmp(line, "tone")) {
    char now[96], last[96];
    *turn_advance = 0;
    format_intent_suffix(now, sizeof now, &g_intent);
    format_intent_suffix(last, sizeof last, &g_last_intent);
    snprintf(msg, msgcap,
             "Current parse intent: %s\nLast applied intent: %s\n"
             "Try phrasing like: talk politely / say quietly / yell aggressively.",
             now[0] ? now + 1 : "none", last[0] ? last + 1 : "none");
    return;
  }

  if (!strcmp(line, "whereami") || !strcmp(line, "where")) {
    *turn_advance = 0;
    snprintf(body, sizeof body, "You are at:\n\n  %s\n\n  [%s]\n",
             resolve_world_title(g_room), world_slug(g_room));
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_nav_suffix());
    ui_fullscreen("WHERE", body, pending_acc, did_fullscreen);
    return;
  }

  {
    static const struct {
      const char *pfx;
      int len;
    } kLocateVerb[] = {{"where ", 6}, {"locate ", 7}, {NULL, 0}};
    int li;
    for (li = 0; kLocateVerb[li].pfx; li++) {
      if (!strncmp(line, kLocateVerb[li].pfx, (size_t)kLocateVerb[li].len)) {
        const char *r = line + kLocateVerb[li].len;
        while (*r == ' ') r++;
        *turn_advance = 0;
        format_locate_body(body, sizeof body, r);
        append_dlc_mod_to_body(body, sizeof body,
                               aet_mods_character_nav_suffix());
        ui_fullscreen("LOCATE", body, pending_acc, did_fullscreen);
        return;
      }
    }
  }

  if (!strncmp(line, "find ", 5)) {
    const char *r = line + 5;
    while (*r == ' ') r++;
    *turn_advance = 0;
    format_find_item_body(body, sizeof body, r);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_nav_suffix());
    ui_fullscreen("FIND ITEM", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strncmp(line, "nearby ", 7) || !strcmp(line, "nearby") ||
      !strcmp(line, "map")) {
    const char *mode = NULL;
    *turn_advance = 0;
    if (!strncmp(line, "nearby ", 7)) {
      mode = line + 7;
      while (*mode == ' ') mode++;
    }
    format_nearby_body(body, sizeof body, mode);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_nearby_suffix());
    ui_fullscreen("NEARBY", body, pending_acc, did_fullscreen);
    return;
  }

  if (line_equals_one_of(line, kWpList)) {
    *turn_advance = 0;
    format_waypoints_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_waypoints_suffix());
    ui_fullscreen("WAYPOINTS", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "waypoint ", 9) || !strncmp(line, "waystone ", 9) ||
      !strncmp(line, "nexus ", 6) || !strncmp(line, "fasttravel ", 11)) {
    static const struct {
      const char *p;
      int n;
    } kWaypointTravel[] = {{"fasttravel ", 11}, {"waypoint ", 9},
                           {"waystone ", 9},     {"nexus ", 6},
                           {NULL, 0}};
    int wi;
    const char *r = NULL;
    for (wi = 0; kWaypointTravel[wi].p; wi++) {
      if (!strncmp(line, kWaypointTravel[wi].p, (size_t)kWaypointTravel[wi].n)) {
        r = line + kWaypointTravel[wi].n;
        break;
      }
    }
    if (r) {
      while (*r == ' ') r++;
      *turn_advance = cmd_waypoint_travel(r, msg, msgcap) ? 1 : 0;
    }
    return;
  }

  if (line_equals_one_of(line, kRoomWords)) {
    *turn_advance = 0;
    format_room_description_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_room_suffix());
    ui_fullscreen("DESCRIPTION", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strncmp(line, "route ", 6)) {
    const char *r = line + 6;
    char err[256];
    int dest;
    while (*r == ' ') r++;
    *turn_advance = 0;
    dest = resolve_room_index(r, err, sizeof err);
    if (dest < 0) {
      snprintf(msg, msgcap, "%s", err);
      return;
    }
    format_route_body(body, sizeof body, dest);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_route_suffix());
    ui_fullscreen("ROUTE", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strncmp(line, "goto ", 5) || !strncmp(line, "approach ", 9) ||
      !strncmp(line, "walk to ", 8) ||
      (!strncmp(line, "enter ", 6) && strlen(line) > 6)) {
    static const struct {
      const char *p;
      size_t n;
      int need_more_than_prefix;
    } kApproach[] = {{"goto ", 5, 0},
                      {"approach ", 9, 0},
                      {"walk to ", 8, 0},
                      {"enter ", 6, 1},
                      {NULL, 0, 0}};
    int ai;
    const char *r = NULL;
    char place[256];
    for (ai = 0; kApproach[ai].p; ai++) {
      if (kApproach[ai].need_more_than_prefix &&
          strlen(line) <= kApproach[ai].n)
        continue;
      if (!strncmp(line, kApproach[ai].p, kApproach[ai].n)) {
        r = line + kApproach[ai].n;
        break;
      }
    }
    if (!r) return;
    while (*r == ' ') r++;
    strncpy(place, r, sizeof place - 1);
    place[sizeof place - 1] = '\0';
    strip_leading_articles(place);
    strip_trailing_space(place);
    if (!place[0]) {
      int d, nx = -1;
      *turn_advance = 0;
      for (d = 0; d < DIR_COUNT; d++) {
        nx = world_exit(g_room, d);
        if (nx >= 0) break;
      }
      if (nx >= 0)
        snprintf(msg, msgcap,
                 "Name a place one exit away (e.g. approach %s), or see "
                 "nearby.",
                 world_slug(nx));
      else
        snprintf(msg, msgcap, "Name a neighboring place (see nearby).");
      return;
    }
    (void)try_approach_named_place(place, msg, msgcap);
    return;
  }

  if (!strcmp(line, "unequip") || !strcmp(line, "stow") ||
      !strcmp(line, "sheath") || !strcmp(line, "take off") ||
      !strcmp(line, "takeoff")) {
    *turn_advance = 0;
    g_ready_item[0] = '\0';
    g_eq_slots[EQ_WEAPON][0] = '\0';
    eq_sync_pc_sheet();
    snprintf(msg, msgcap, "You tuck your hands free.");
    return;
  }
  if (!strcmp(line, "equip") || !strcmp(line, "wield")) {
    *turn_advance = 0;
    snprintf(msg, msgcap, "Equip what? (e.g. equip torch — or unequip.)");
    return;
  }
  if (!strcmp(line, "hold")) {
    *turn_advance = 0;
    snprintf(msg, msgcap, "Hold what?");
    return;
  }
  if (!strncmp(line, "equip ", 6) || !strncmp(line, "hold ", 5) ||
      !strncmp(line, "wield ", 6)) {
    static const struct {
      const char *p;
      int n;
    } kEquipVerb[] = {{"equip ", 6}, {"wield ", 6}, {"hold ", 5}, {NULL, 0}};
    int ei;
    const char *r = NULL;
    for (ei = 0; kEquipVerb[ei].p; ei++) {
      if (!strncmp(line, kEquipVerb[ei].p, (size_t)kEquipVerb[ei].n)) {
        r = line + kEquipVerb[ei].n;
        break;
      }
    }
    if (!r) return;
    *turn_advance = 0;
    cmd_equip(r, msg, msgcap);
    return;
  }

  if (!strcmp(line, "objectives") || !strcmp(line, "goals")) {
    *turn_advance = 0;
    format_objectives_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_objectives_suffix());
    ui_fullscreen("OBJECTIVES", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "journal") || !strcmp(line, "quests")) {
    *turn_advance = 0;
    format_journal_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_journal_suffix());
    ui_fullscreen("JOURNAL", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "notes") || !strcmp(line, "notes show")) {
    *turn_advance = 0;
    format_notes_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_notes_panel_suffix());
    ui_fullscreen("NOTES", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "notes todo") || !strcmp(line, "notes pending") ||
      !strcmp(line, "notes checklist")) {
    *turn_advance = 0;
    format_notes_filtered_body(body, sizeof body, "Todo notes", 1, NULL);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_notes_panel_suffix());
    ui_fullscreen("NOTES - Todo", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "notes done") || !strcmp(line, "notes completed")) {
    *turn_advance = 0;
    format_notes_filtered_body(body, sizeof body, "Completed notes", 2, NULL);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_notes_panel_suffix());
    ui_fullscreen("NOTES DONE", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "notes stats") || !strcmp(line, "notes progress")) {
    *turn_advance = 0;
    format_notes_stats_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_notes_panel_suffix());
    ui_fullscreen("NOTES STATS", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "notes clear")) {
    *turn_advance = 0;
    g_note_n = 0;
    snprintf(msg, msgcap, "All notes cleared.");
    return;
  }
  if (!strcmp(line, "notes purge done") ||
      !strcmp(line, "notes purge completed") ||
      !strcmp(line, "notes prune done") ||
      !strcmp(line, "notes prune completed")) {
    int removed;
    *turn_advance = 0;
    removed = notes_purge_done();
    snprintf(msg, msgcap,
             removed ? "Removed %d completed note%s."
                     : "No completed notes to purge.",
             removed, removed == 1 ? "" : "s");
    return;
  }
  if (!strncmp(line, "notes find ", 11) ||
      !strncmp(line, "notes search ", 13)) {
    const char *q = !strncmp(line, "notes find ", 11) ? line + 11 : line + 13;
    char title[128];
    while (*q == ' ') q++;
    *turn_advance = 0;
    if (!q[0]) {
      snprintf(msg, msgcap, "Usage: notes find <text>");
      return;
    }
    snprintf(title, sizeof title, "Notes matching \"%.*s\"", 72, q);
    format_notes_filtered_body(body, sizeof body, title, 0, q);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_notes_panel_suffix());
    ui_fullscreen("NOTES FIND", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "notes done ", 11) ||
      !strncmp(line, "notes complete ", 15) ||
      !strncmp(line, "notes completed ", 16)) {
    const char *r = !strncmp(line, "notes done ", 11) ? line + 11
                    : !strncmp(line, "notes complete ", 15) ? line + 15
                                                            : line + 16;
    char *end = NULL;
    long n;
    while (*r == ' ') r++;
    *turn_advance = 0;
    if (str_ieq(r, "all")) {
      int changed = notes_set_all_done(1);
      snprintf(msg, msgcap, changed ? "Marked all notes done."
                                    : "No notes to mark done.");
      return;
    }
    n = strtol(r, &end, 10);
    if (!r[0] || !end || end == r || n < 1) {
      snprintf(msg, msgcap, "Use: notes done <line|all>.");
      return;
    }
    if (!notes_set_done_line((int)n, 1))
      snprintf(msg, msgcap, "No note #%d.", (int)n);
    else
      snprintf(msg, msgcap, "Marked note #%d done.", (int)n);
    return;
  }
  if (!strncmp(line, "notes undone ", 13) ||
      !strncmp(line, "notes todo ", 11) ||
      !strncmp(line, "notes incomplete ", 17)) {
    const char *r = !strncmp(line, "notes undone ", 13) ? line + 13
                    : !strncmp(line, "notes todo ", 11) ? line + 11
                                                        : line + 17;
    char *end = NULL;
    long n;
    while (*r == ' ') r++;
    *turn_advance = 0;
    if (str_ieq(r, "all")) {
      int changed = notes_set_all_done(0);
      snprintf(msg, msgcap, changed ? "Marked all notes todo."
                                    : "No notes to mark todo.");
      return;
    }
    n = strtol(r, &end, 10);
    if (!r[0] || !end || end == r || n < 1) {
      snprintf(msg, msgcap, "Use: notes undone <line|all>.");
      return;
    }
    if (!notes_set_done_line((int)n, 0))
      snprintf(msg, msgcap, "No note #%d.", (int)n);
    else
      snprintf(msg, msgcap, "Marked note #%d todo.", (int)n);
    return;
  }
  if (!strncmp(line, "notes delete ", 13)) {
    const char *r = line + 13;
    char *end = NULL;
    long n;
    while (*r == ' ') r++;
    *turn_advance = 0;
    n = strtol(r, &end, 10);
    if (!r[0] || !end || end == r || n < 1) {
      snprintf(msg, msgcap, "Use: notes delete <line> (1 = first note).");
      return;
    }
    if (!notes_delete_line((int)n))
      snprintf(msg, msgcap, "No note #%d.", (int)n);
    else
      snprintf(msg, msgcap, "Deleted note #%d.", (int)n);
    return;
  }
  if (!strncmp(line, "notes add ", 10)) {
    const char *t = line + 10;
    while (*t == ' ') t++;
    *turn_advance = 0;
    if (!t[0]) {
      snprintf(msg, msgcap, "Notes add what? (notes add <text>)");
      return;
    }
    if (!notes_add_line(t))
      snprintf(msg, msgcap, "Note book full (%d lines max).", MAX_NOTES);
    else
      snprintf(msg, msgcap, "Noted.");
    return;
  }
  if (!strncmp(line, "note ", 5)) {
    const char *t = line + 5;
    while (*t == ' ') t++;
    *turn_advance = 0;
    if (!t[0]) {
      snprintf(msg, msgcap, "Note what? (note <text>)");
      return;
    }
    if (!notes_add_line(t))
      snprintf(msg, msgcap, "Note book full (%d lines max).", MAX_NOTES);
    else
      snprintf(msg, msgcap, "Noted.");
    return;
  }

  if (!strcmp(line, "saves") || !strcmp(line, "save slots") ||
      !strcmp(line, "slots")) {
    *turn_advance = 0;
    (void)run_save_manager_ui(did_fullscreen, 0);
    return;
  }
  if (!strcmp(line, "save") || !strcmp(line, "quicksave") ||
      !strcmp(line, "qs")) {
    *turn_advance = 0;
    save_game(msg, msgcap);
    return;
  }
  if (!strncmp(line, "save ", 5)) {
    int slot;
    *turn_advance = 0;
    if (!parse_save_slot(line + 5, &slot)) {
      snprintf(msg, msgcap, "Use: save <1-%d>  or  saves.", SAVE_SLOT_COUNT);
      return;
    }
    save_game_slot(slot, msg, msgcap);
    return;
  }
  if (!strcmp(line, "load") || !strcmp(line, "reload") ||
      !strcmp(line, "quickload") || !strcmp(line, "ql") ||
      !strcmp(line, "restore")) {
    *turn_advance = 0;
    load_game(msg, msgcap);
    return;
  }
  if (!strcmp(line, "load autosave") || !strcmp(line, "load shadow") ||
      !strcmp(line, "restore autosave") || !strcmp(line, "restore shadow")) {
    char apath[520];
    *turn_advance = 0;
    make_autosave_path(apath, sizeof apath);
    if (!load_game_path(apath, msg, msgcap))
      snprintf(msg, msgcap,
               "No autosave shadow at %s — play a turn with autosave on first.",
               apath);
    return;
  }
  if (!strcmp(line, "autosave") || !strcmp(line, "autosave status")) {
    char apath[520];
    *turn_advance = 0;
    make_autosave_path(apath, sizeof apath);
    snprintf(msg, msgcap, "Autosave shadow is %s.\nFile: %s\nLoad with: load autosave",
             g_autosave_enabled ? "ON (written after each advancing turn)" : "OFF",
             apath);
    return;
  }
  if (!strcmp(line, "autosave on") || !strcmp(line, "autosave enable")) {
    *turn_advance = 0;
    g_autosave_enabled = 1;
    snprintf(msg, msgcap, "Autosave shadow enabled — a backup copy is written beside your quicksave after each turn.");
    return;
  }
  if (!strcmp(line, "autosave off") || !strcmp(line, "autosave disable")) {
    *turn_advance = 0;
    g_autosave_enabled = 0;
    snprintf(msg, msgcap, "Autosave shadow disabled — only the main quicksave updates each turn.");
    return;
  }
  if (!strncmp(line, "load ", 5) || !strncmp(line, "restore ", 8)) {
    const char *r = !strncmp(line, "load ", 5) ? line + 5 : line + 8;
    int slot;
    *turn_advance = 0;
    if (!parse_save_slot(r, &slot)) {
      snprintf(msg, msgcap, "Use: load <1-%d> | load autosave  or  saves.", SAVE_SLOT_COUNT);
      return;
    }
    load_game_slot(slot, msg, msgcap);
    return;
  }

  if (!strcmp(line, "who") || !strcmp(line, "npcs") ||
      !strcmp(line, "people") || !strcmp(line, "who all") ||
      !strcmp(line, "who global") || !strcmp(line, "npcs all") ||
      !strcmp(line, "people all")) {
    int global_people =
        (!strcmp(line, "who all") || !strcmp(line, "who global") ||
         !strcmp(line, "npcs all") || !strcmp(line, "people all"));
    *turn_advance = 0;
    cmd_who(body, sizeof body, global_people);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_people_suffix());
    ui_fullscreen(global_people ? "PEOPLE (GLOBAL)" : "PEOPLE HERE", body,
                  pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "npc schedule") || !strcmp(line, "routine")) {
    *turn_advance = 0;
    cmd_npc_schedule("", msg, msgcap);
    return;
  }
  if (!strncmp(line, "npc schedule ", 13)) {
    *turn_advance = 0;
    cmd_npc_schedule(line + 13, msg, msgcap);
    return;
  }
  if (!strncmp(line, "routine ", 8)) {
    *turn_advance = 0;
    cmd_npc_schedule(line + 8, msg, msgcap);
    return;
  }
  if (!strcmp(line, "topic") || !strcmp(line, "last topic") ||
      !strcmp(line, "topic mood") || !strcmp(line, "topic heat")) {
    AetPcSave ps;
    int hp_pct;
    int risk;
    const char *tm;
    *turn_advance = 0;
    pc_capture(&ps);
    hp_pct = g_max_health > 0 ? (g_health * 100) / g_max_health : 0;
    if (hp_pct < 0) hp_pct = 0;
    if (hp_pct > 100) hp_pct = 100;
    risk = ps.cor + (100 - hp_pct) / 10;
    tm = topic_mood_for(g_last_topic);
    snprintf(msg, msgcap,
             "Last conversation topic you steered toward: %s (mood: %s, heat: %s)",
             g_last_topic[0] ? g_last_topic : "—", tm, topic_heat_for(tm, risk));
    return;
  }
  if (!strcmp(line, "talk") || !strcmp(line, "greet") ||
      !strcmp(line, "hello") || !strcmp(line, "hi")) {
    cmd_talk("", NULL, msg, msgcap);
    apply_intent_to_social_feedback(msg, msgcap);
    g_last_intent = g_intent;
    causal_push("talk", msg);
    return;
  }
  if (!strncmp(line, "talk about ", 11)) {
    const char *t = line + 11;
    while (*t == ' ') t++;
    cmd_talk("", t[0] ? t : NULL, msg, msgcap);
    apply_intent_to_social_feedback(msg, msgcap);
    g_last_intent = g_intent;
    causal_push("talk-topic", t[0] ? t : "(none)");
    return;
  }
  if (!strncmp(line, "talk to ", 8)) {
    char work[INPUT_LINE_MAX];
    char *sep;
    const char *rest = line + 8;
    while (*rest == ' ') rest++;
    strncpy(work, rest, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    sep = strstr(work, " about ");
    if (sep) {
      *sep = '\0';
      strip_trailing_space(work);
      cmd_talk(work, sep + 7, msg, msgcap);
    } else
      cmd_talk(work, NULL, msg, msgcap);
    apply_intent_to_social_feedback(msg, msgcap);
    g_last_intent = g_intent;
    causal_push("talk-target", work[0] ? work : "(none)");
    return;
  }

  if (!strncmp(line, "protective grab ", 16)) {
    const char *r = line + 16;
    while (*r == ' ') r++;
    if (!cmd_protective_grab(r, msg, msgcap)) *turn_advance = 0;
    return;
  }

  if (!strcmp(line, "play piano") || !strncmp(line, "play piano ", 11)) {
    cmd_play_piano(msg, msgcap);
    return;
  }

  if (!strncmp(line, "say ", 4) || !strncmp(line, "shout ", 6) ||
      !strncmp(line, "yell ", 5)) {
    const char *t;
    int loud = 0;
    *turn_advance = 0;
    if (!strncmp(line, "shout ", 6)) {
      t = line + 6;
      loud = 1;
    } else if (!strncmp(line, "yell ", 5)) {
      t = line + 5;
      loud = 1;
    } else
      t = line + 4;
    while (*t == ' ') t++;
    if (!t[0]) {
      snprintf(msg, msgcap, "%s what?",
               loud ? "Shout" : "Say");
      return;
    }
    if (g_intent.quiet && !g_intent.loud && loud) loud = 0;
    if (g_intent.loud && !loud) loud = 1;
    snprintf(msg, msgcap,
             loud ? "You shout: \"%.*s\" (the world barely answers.)"
                  : "You murmur: \"%.*s\"",
             120, t);
    apply_intent_to_social_feedback(msg, msgcap);
    g_last_intent = g_intent;
    causal_push(loud ? "say-loud" : "say-quiet", t);
    return;
  }

  if (!strncmp(line, "read ", 5)) {
    *turn_advance = 0;
    read_target(line + 5, msg, msgcap);
    return;
  }

  if (!strncmp(line, "open mailbox", 12) ||
      !strncmp(line, "open the mailbox", 16)) {
    *turn_advance = 0;
    container_open_cmd(g_room, "mailbox", msg, msgcap);
    return;
  }

  if (!strncmp(line, "open ", 5)) {
    const char *rest = line + 5;
    char work[INPUT_LINE_MAX];
    while (*rest == ' ') rest++;
    if (rest[0] && strcmp(line, "open door") && strcmp(line, "open the door")) {
      int prep;
      *turn_advance = 0;
      prep = parser_prepare_object_query(rest, work, sizeof work, NULL, NULL,
                                         msg, msgcap);
      if (prep < 0) return;
      if (prep > 0 && container_open_cmd(g_room, work, msg, msgcap)) return;
    }
  }

  if (!strncmp(line, "close ", 6)) {
    const char *rest = line + 6;
    char work[INPUT_LINE_MAX];
    while (*rest == ' ') rest++;
    if (rest[0] && strcmp(line, "close door") && strcmp(line, "close the door") &&
        strcmp(line, "shut door")) {
      int prep = parser_prepare_object_query(rest, work, sizeof work, NULL, NULL,
                                             msg, msgcap);
      if (prep < 0) return;
      if (prep > 0 && container_close_cmd(g_room, work, msg, msgcap)) return;
    }
  }

  if (!strncmp(line, "unlock ", 7)) {
    char work[INPUT_LINE_MAX];
    char *keyp = NULL;
    const char *rest = line + 7;
    *turn_advance = 0;
    while (*rest == ' ') rest++;
    if (!strcmp(rest, "door")) {
      const char *room_slug = world_slug(g_room);
      if (!room_slug || strcmp(room_slug, "front_door") != 0) {
        snprintf(msg, msgcap, "Nothing to unlock here.");
      } else if (!inv_has("house_key")) {
        snprintf(msg, msgcap, "You have no key that fits.");
      } else {
        g_front_unlocked = 1;
        snprintf(msg, msgcap,
                 "The house key turns with a heavy click. The planks shift "
                 "slightly.");
      }
      return;
    }
    strncpy(work, rest, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    keyp = strstr(work, " with ");
    if (!keyp) keyp = strstr(work, " using ");
    if (keyp) {
      *keyp = '\0';
      strip_trailing_space(work);
      keyp += 6;
      while (*keyp == ' ') keyp++;
    }
    {
      int prep = parser_prepare_object_query(work, work, sizeof work, NULL, NULL,
                                             msg, msgcap);
      if (prep < 0) return;
      if (prep > 0 && container_unlock_cmd(g_room, work, keyp, msg, msgcap))
        return;
    }
    snprintf(msg, msgcap, "Nothing to unlock here.");
    return;
  }

  if (!strncmp(line, "touch ", 6)) {
    *turn_advance = 0;
    touch_target(line + 6, msg, msgcap);
    return;
  }
  if (!strncmp(line, "tap ", 4)) {
    *turn_advance = 0;
    touch_target(line + 4, msg, msgcap);
    return;
  }

  if (!strcmp(line, "search") || !strcmp(line, "rummage") ||
      !strcmp(line, "feel around")) {
    search_room(msg, msgcap);
    return;
  }

  if (!strcmp(line, "listen") || !strcmp(line, "hear")) {
    sensory_line("listen", msg, msgcap);
    return;
  }
  if (!strcmp(line, "smell") || !strcmp(line, "sniff")) {
    sensory_line("sniff the air", msg, msgcap);
    return;
  }

  if (!strncmp(line, "look under ", 11)) {
    search_room(msg, msgcap);
    return;
  }

  if (!strcmp(line, "scan")) {
    *turn_advance = 0;
    cmd_scan(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_scan_suffix());
    ui_fullscreen("SCAN", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "loot") || !strcmp(line, "loot value") ||
      !strcmp(line, "loot weight")) {
    const char *sort = NULL;
    if (!strcmp(line, "loot value")) sort = "value";
    else if (!strcmp(line, "loot weight")) sort = "weight";
    *turn_advance = 0;
    cmd_loot(body, sizeof body, sort);
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_loot_suffix());
    ui_fullscreen("LOOT", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strncmp(line, "compare ", 8)) {
    *turn_advance = 0;
    cmd_compare(line + 8, body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_compare_suffix());
    ui_fullscreen("COMPARE", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strncmp(line, "price compare ", 14) ||
      !strncmp(line, "compare prices ", 15) ||
      !strncmp(line, "market compare ", 15)) {
    const char *rest = line;
    if (!strncmp(line, "price compare ", 14)) rest = line + 14;
    else if (!strncmp(line, "compare prices ", 15)) rest = line + 15;
    else rest = line + 15;
    *turn_advance = 0;
    cmd_price_compare(rest, body, sizeof body);
    ui_fullscreen("PRICE COMPARISON", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strncmp(line, "look at ", 8)) {
    *turn_advance = 0;
    examine_target(line + 8, msg, msgcap);
    return;
  }
  if (!strncmp(line, "inspect ", 8)) {
    *turn_advance = 0;
    examine_target(line + 8, msg, msgcap);
    return;
  }
  if (!strncmp(line, "examine ", 8)) {
    *turn_advance = 0;
    examine_target(line + 8, msg, msgcap);
    return;
  }
  if (!strncmp(line, "x ", 2)) {
    *turn_advance = 0;
    examine_target(line + 2, msg, msgcap);
    return;
  }

  if (!strcmp(line, "look") || !strcmp(line, "l") || !strcmp(line, "x") ||
      !strcmp(line, "examine") || !strcmp(line, "look around")) {
    *turn_advance = 0;
    return;
  }
  if (!strncmp(line, "l ", 2)) {
    *turn_advance = 0;
    examine_target(line + 2, msg, msgcap);
    return;
  }
  if (!strncmp(line, "look ", 5)) {
    *turn_advance = 0;
    examine_target(line + 5, msg, msgcap);
    return;
  }

  if (!strncmp(line, "go ", 3)) {
    const char *tok = line + 3;
    char gwork[160];
    char *ga, *gb, *gc;
    long gn;
    char *ge;
    int gdir, gused;
    while (*tok == ' ') tok++;
    strncpy(gwork, tok, sizeof gwork - 1);
    gwork[sizeof gwork - 1] = '\0';
    ga = strtok(gwork, " ");
    gb = strtok(NULL, " ");
    gc = strtok(NULL, " ");
    if (ga && gb && !gc) {
      gn = strtol(ga, &ge, 10);
      if (ge > ga && *ge == '\0' && gn >= 1 && gn <= MAX_AUTO_STEPS &&
          parse_direction_token(gb, &gdir)) {
        (void)try_move_n_steps(gdir, (int)gn, msg, msgcap, &gused);
        *turn_advance = gused;
        return;
      }
      gn = strtol(gb, &ge, 10);
      if (ge > gb && *ge == '\0' && gn >= 1 && gn <= MAX_AUTO_STEPS &&
          parse_direction_token(ga, &gdir)) {
        (void)try_move_n_steps(gdir, (int)gn, msg, msgcap, &gused);
        *turn_advance = gused;
        return;
      }
    }
    if (!try_direction_parse(tok, msg, msgcap))
      snprintf(msg, msgcap,
               "Go where? (e.g. go north, go 3 east — max %d steps.)",
               MAX_AUTO_STEPS);
    return;
  }

  {
    char tw[INPUT_LINE_MAX];
    strncpy(tw, line, sizeof tw - 1);
    tw[sizeof tw - 1] = '\0';
    if (try_parse_two_token_move(tw, msg, msgcap, turn_advance)) return;
  }

  if (try_direction_parse(line, msg, msgcap)) return;

  slug = world_slug(g_room);

  if (!strcmp(line, "unlock door") || !strcmp(line, "use key")) {
    if (strcmp(slug, "front_door") != 0) {
      snprintf(msg, msgcap, "Nothing to unlock here.");
    } else if (!inv_has("house_key")) {
      snprintf(msg, msgcap, "You have no key that fits.");
    } else {
      g_front_unlocked = 1;
      snprintf(msg, msgcap,
               "The house key turns with a heavy click. The planks shift "
               "slightly.");
    }
    return;
  }

  if (!strcmp(line, "open door") || !strcmp(line, "open the door")) {
    if (strcmp(slug, "front_door") != 0) {
      snprintf(msg, msgcap, "Nothing obvious to open.");
    } else if (!inv_has("house_key")) {
      snprintf(msg, msgcap,
               "The planks and lock resist. You probably need the right key.");
    } else {
      g_front_unlocked = 1;
      snprintf(msg, msgcap,
               "You work the key; the barred entry groans open far enough to "
               "slip through.");
    }
    return;
  }

  if (!strcmp(line, "close door") || !strcmp(line, "shut door") ||
      !strcmp(line, "close the door")) {
    if (strcmp(slug, "front_door") != 0) {
      snprintf(msg, msgcap, "Nothing obvious to close.");
    } else {
      snprintf(msg, msgcap,
               "You tug the planks and fittings; they settle with a dull "
               "thud. (Travel rules unchanged in this port.)");
    }
    return;
  }

  if (!strcmp(line, "break bucket") || !strcmp(line, "smash bucket")) {
    if (!inv_has("bucket"))
      snprintf(msg, msgcap, "You are not carrying a bucket to assault.");
    else
      snprintf(
          msg, msgcap,
          "You commit violence against galvanized metal. The bucket declines "
          "to participate in this arc. (Still intact — this edition has no "
          "durability bar.)");
    return;
  }

  if (!strcmp(line, "fill bucket")) {
    if (!inv_has("bucket"))
      snprintf(msg, msgcap, "You have no bucket to fill.");
    else if (strstr(slug, "well") == NULL)
      snprintf(
          msg, msgcap,
          "You need a well or open water — the gesture wants an audience of "
          "ripples.");
    else
      snprintf(
          msg, msgcap,
          "You dip the bucket. Cold climbs the metal; the dark water argues "
          "with your reflection. This port does not track \"full\" state — the "
          "theatre is the point.");
    return;
  }

  if (!strncmp(line, "use ", 4)) {
    char *r = line + 4;
    char *target = NULL;
    char *on_sep;
    char *with_sep;
    while (*r == ' ') r++;
    on_sep = strstr(r, " on ");
    with_sep = strstr(r, " with ");
    if (on_sep) {
      *on_sep = '\0';
      target = on_sep + 4;
      strip_trailing_space(r);
      while (target && *target == ' ') target++;
    } else if (with_sep) {
      *with_sep = '\0';
      target = with_sep + 6;
      strip_trailing_space(r);
      while (target && *target == ' ') target++;
    }
    strip_leading_articles(r);
    if (target) strip_leading_articles(target);
    if (target && target[0]) {
      if (container_unlock_cmd(g_room, target, r, msg, msgcap)) return;
      if (container_lockpick_cmd(g_room, target, msg, msgcap)) return;
    }
    if (strstr(r, "bandage") != NULL && inv_has("bandage")) {
      char tmp[MAX_ITEM_LEN];
      int ix = inv_find("bandage");
      if (ix < 0) {
        snprintf(msg, msgcap, "You are not carrying a bandage.");
        return;
      }
      inv_take_out(ix, tmp, sizeof tmp);
      if (str_ieq(g_ready_item, tmp)) g_ready_item[0] = '\0';
      if (str_ieq(g_last_focus, tmp)) clear_focus();
      player_heal(35);
      snprintf(msg, msgcap,
               "You bind the cloth — pressure, tuck, tie. Noise in your nerves "
               "quiets. (%d / %d)",
               g_health, g_max_health);
      return;
    }
    if (inv_has("bucket") && target && strstr(target, "well") != NULL &&
        str_ieq(r, "bucket")) {
      snprintf(
          msg, msgcap,
          "You lower the bucket. The well answers with a hollow note, as if "
          "applauding a choice that was never on the main path.");
      return;
    }
    if (str_ieq(r, "bucket") && inv_has("bucket")) {
      static const char *const Ub[] = {
          "You hoist the bucket like evidence. The room withholds judgment.",
          "You invert the bucket. Nothing falls out except your certainty that "
          "this was optional.",
          "You and the bucket share a look. Neither of you admits to it.",
          "\"Use\" is a strong word. The bucket tolerates your interpretation."};
      int nb = (int)(sizeof Ub / sizeof Ub[0]);
      snprintf(msg, msgcap, "%s", Ub[(g_turns + 11) % nb]);
      return;
    }
    if ((strcmp(slug, "front_door") == 0 || strcmp(slug, "west_of_house") == 0 ||
         (target && (strstr(target, "door") || strstr(target, "keyhole")))) &&
        (strstr(r, "key") != NULL || str_ieq(r, "house_key"))) {
      if (!inv_has("house_key")) {
        snprintf(msg, msgcap, "You do not have that key.");
      } else {
        g_front_unlocked = 1;
        snprintf(msg, msgcap,
                 "The house key answers the glowing keyhole. The way east "
                 "eases open.");
      }
      return;
    }
    if ((strcmp(slug, "east_of_house") == 0 ||
         (target && (strstr(target, "shed") || strstr(target, "lock")))) &&
        (strstr(r, "lockpick") != NULL || strstr(r, "pick") != NULL)) {
      if (!has_lockpick_tool()) {
        snprintf(msg, msgcap, "You need a working lockpick.");
      } else {
#ifdef AETER_MINIGAMES
        if (try_minigame("lockpick", msg, msgcap)) {
          MgtPersistentState *st = mgt_host_state();
          game_mgt_apply_lockpick_stealth(st, msg, msgcap);
          if (st && st->last_success == 1) g_shed_unlocked = 1;
          return;
        }
#endif
        g_shed_unlocked = 1;
        snprintf(msg, msgcap,
                 "You coax the shed lock with your tools until it yields.");
      }
      return;
    }
    {
      int tu = 1;
      if (try_use_consume(r, msg, msgcap, &tu)) {
        *turn_advance = tu;
        return;
      }
    }
    snprintf(msg, msgcap, "You are not sure how to use that here.");
    return;
  }

  if (!strcmp(line, "pick lock") || !strcmp(line, "pick")) {
    if (strcmp(slug, "east_of_house") != 0) {
      snprintf(msg, msgcap, "Nothing to pick here.");
    } else if (!has_lockpick_tool()) {
      snprintf(msg, msgcap, "You need a lockpick.");
    } else {
#ifdef AETER_MINIGAMES
      if (try_minigame("lockpick", msg, msgcap)) {
        MgtPersistentState *st = mgt_host_state();
        game_mgt_apply_lockpick_stealth(st, msg, msgcap);
        if (st && st->last_success == 1) g_shed_unlocked = 1;
        return;
      }
#endif
      g_shed_unlocked = 1;
      snprintf(msg, msgcap,
               "The shed lock gives way with a quiet snick. The door can open.");
    }
    return;
  }

  if (!strcmp(line, "fish") || !strcmp(line, "go fishing")) {
    if (!room_can_fish()) {
      snprintf(msg, msgcap, "There is no good place to fish here.");
      return;
    }
#ifdef AETER_MINIGAMES
    if (try_minigame("fishing", msg, msgcap)) return;
#endif
    snprintf(msg, msgcap, "You cast a line but the moment passes without a bite.");
    return;
  }

  if (!strcmp(line, "farm") || !strcmp(line, "tend farm") ||
      !strcmp(line, "work farm")) {
    if (!room_can_farm()) {
      snprintf(msg, msgcap, "This is not farmland.");
      return;
    }
#ifdef AETER_MINIGAMES
    if (try_minigame("farming", msg, msgcap)) return;
#endif
    snprintf(msg, msgcap, "You pull a few weeds and call it enough for now.");
    return;
  }

  if (!strcmp(line, "cook") || !strcmp(line, "work kitchen") ||
      !strcmp(line, "work shift")) {
    if (!room_can_cook()) {
      snprintf(msg, msgcap, "There is no kitchen to work here.");
      return;
    }
#ifdef AETER_MINIGAMES
    if (try_minigame("cooking", msg, msgcap)) return;
#endif
    snprintf(msg, msgcap, "You stir a pot and leave the real service to memory.");
    return;
  }

  if (!strcmp(line, "gamble") || !strcmp(line, "play dice") ||
      !strcmp(line, "play cards")) {
    if (!room_can_gamble()) {
      snprintf(msg, msgcap, "No dice or cards are laid out here.");
      return;
    }
#ifdef AETER_MINIGAMES
    if (try_minigame("gambling", msg, msgcap)) return;
#endif
    snprintf(msg, msgcap, "You watch a hand of cards without joining in.");
    return;
  }

  if (!strcmp(line, "hunt") || !strcmp(line, "track game")) {
    const char *slug = world_slug(g_room);
    if (!slug || (strstr(slug, "forest") == NULL && strcmp(slug, "meadow") != 0 &&
                  strcmp(slug, "deep_forest") != 0)) {
      snprintf(msg, msgcap, "No game trails worth following here.");
      return;
    }
    if (!has_hunting_weapon()) {
      snprintf(msg, msgcap, "You need a bow to hunt here.");
      return;
    }
#ifdef AETER_MINIGAMES
    if (try_minigame("hunting", msg, msgcap)) return;
#endif
    snprintf(msg, msgcap, "You stalk the treeline but find only prints and silence.");
    return;
  }

  if (!strcmp(line, "write") || !strcmp(line, "scribe")) {
#ifdef AETER_MINIGAMES
    if (try_minigame("writing", msg, msgcap)) return;
#endif
    snprintf(msg, msgcap, "You have no desk handy for a proper writing session.");
    return;
  }


  if (!strncmp(line, "give ", 5)) {
    const char *r = line + 5;
    while (*r == ' ') r++;
    cmd_give(r, msg, msgcap);
    return;
  }

  if (!strcmp(line, "wares") || !strcmp(line, "shop") ||
      !strcmp(line, "stock") || !strcmp(line, "catalog") ||
      !strcmp(line, "prices")) {
    *turn_advance = 0;
    if (!format_merchant_wares(body, sizeof body)) {
      snprintf(msg, msgcap,
               "No merchant is here (try shops in towns and the mill).");
      return;
    }
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_wares_suffix());
    ui_fullscreen("WARES", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strcmp(line, "buy") || !strcmp(line, "purchase")) {
    *turn_advance = 0;
    if (!format_merchant_wares(body, sizeof body)) {
      snprintf(msg, msgcap, "No merchant here — use buy <item> when shopping.");
      return;
    }
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_wares_suffix());
    ui_fullscreen("WARES", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "sell")) {
    *turn_advance = 0;
    if (!format_merchant_wares(body, sizeof body)) {
      snprintf(msg, msgcap, "No buyer here — use sell <item> when trading.");
      return;
    }
    append_dlc_mod_to_body(body, sizeof body, aet_mods_character_wares_suffix());
    ui_fullscreen("WARES", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "haggle") || !strcmp(line, "barter") ||
      !strcmp(line, "negotiate")) {
    *turn_advance = 0;
    cmd_trade_haggle("", msg, msgcap);
    return;
  }
  if (!strncmp(line, "haggle ", 7) || !strncmp(line, "barter ", 7) ||
      !strncmp(line, "negotiate ", 10)) {
    const char *r = !strncmp(line, "haggle ", 7)
                        ? line + 7
                        : !strncmp(line, "barter ", 7) ? line + 7 : line + 10;
    while (*r == ' ') r++;
    cmd_trade_haggle(r, msg, msgcap);
    return;
  }
  if (!strncmp(line, "buy all except ", 15) ||
      !strncmp(line, "buy all but ", 12)) {
    const char *ex =
        !strncmp(line, "buy all except ", 15) ? line + 15 : line + 12;
    while (*ex == ' ') ex++;
    cmd_trade_buy_all_except(ex, msg, msgcap);
    return;
  }
  if (!strncmp(line, "purchase all except ", 20) ||
      !strncmp(line, "purchase all but ", 17)) {
    const char *ex =
        !strncmp(line, "purchase all except ", 20) ? line + 20 : line + 17;
    while (*ex == ' ') ex++;
    cmd_trade_buy_all_except(ex, msg, msgcap);
    return;
  }
  if (!strncmp(line, "sell all except ", 16) ||
      !strncmp(line, "sell all but ", 13)) {
    const char *ex =
        !strncmp(line, "sell all except ", 16) ? line + 16 : line + 13;
    while (*ex == ' ') ex++;
    cmd_trade_sell_all_except(ex, msg, msgcap);
    return;
  }
  if (!strcmp(line, "trade history") || !strcmp(line, "trade log") ||
      !strcmp(line, "transactions") || !strcmp(line, "transaction log") ||
      !strcmp(line, "transaction history")) {
    *turn_advance = 0;
    format_trade_history_body(body, sizeof body);
    ui_fullscreen("TRADE HISTORY", body, pending_acc, did_fullscreen);
    return;
  }

  if (!strncmp(line, "buy ", 4)) {
    const char *r = line + 4;
    while (*r == ' ') r++;
    if (str_ieq(r, "all"))
      cmd_trade_buy_all_except(NULL, msg, msgcap);
    else
      cmd_trade_buy(r, msg, msgcap);
    return;
  }
  if (!strncmp(line, "purchase ", 9)) {
    const char *r = line + 9;
    while (*r == ' ') r++;
    if (str_ieq(r, "all"))
      cmd_trade_buy_all_except(NULL, msg, msgcap);
    else
      cmd_trade_buy(r, msg, msgcap);
    return;
  }
  if (!strncmp(line, "sell ", 5)) {
    const char *r = line + 5;
    while (*r == ' ') r++;
    if (str_ieq(r, "all"))
      cmd_trade_sell_all_except(NULL, msg, msgcap);
    else
      cmd_trade_sell(r, msg, msgcap);
    return;
  }

  if (!strcmp(line, "eat") || !strcmp(line, "taste")) {
    *turn_advance = 0;
    snprintf(msg, msgcap, "Eat what? (food in your pack.)");
    return;
  }
  if (!strcmp(line, "drink") || !strcmp(line, "sip")) {
    *turn_advance = 0;
    snprintf(msg, msgcap, "Drink what?");
    return;
  }
  if (!strncmp(line, "eat ", 4)) {
    const char *r = line + 4;
    while (*r == ' ') r++;
    if (!cmd_consume(r, 0, msg, msgcap))
      *turn_advance = 0;
    return;
  }
  if (!strncmp(line, "drink ", 6)) {
    const char *r = line + 6;
    while (*r == ' ') r++;
    if (!cmd_consume(r, 1, msg, msgcap))
      *turn_advance = 0;
    return;
  }

  if (!strcmp(line, "push") || !strcmp(line, "pull")) {
    snprintf(msg, msgcap, "You push and pull; nothing special yields.");
    return;
  }
  if (!strcmp(line, "attack") || !strcmp(line, "fight") ||
      !strcmp(line, "hit") || !strcmp(line, "strike")) {
    snprintf(msg, msgcap,
             "No combat in this port — try talking, searching, or moving on.");
    return;
  }
  if (!strcmp(line, "time") || !strcmp(line, "clock")) {
    *turn_advance = 0;
    format_time_body(body, sizeof body);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_time_suffix());
    ui_fullscreen("TIME", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strncmp(line, "time until ", 11) || !strncmp(line, "time to ", 8) ||
      !strncmp(line, "clock until ", 12)) {
    const char *r = !strncmp(line, "time until ", 11) ? line + 11
                    : !strncmp(line, "clock until ", 12) ? line + 12
                                                         : line + 8;
    AetWorldClock wc;
    int target_hour;
    int hours;
    *turn_advance = 0;
    while (*r == ' ') r++;
    get_world_clock(&wc);
    target_hour = period_start_hour(r);
    if (target_hour >= 0) {
      if (str_ieq(wc.period, r)) {
        snprintf(msg, msgcap, "It is already %s.", wc.period);
        return;
      }
      hours = hours_until_hour(wc.hour, target_hour);
      snprintf(msg, msgcap, "%d hour%s until %s.", hours,
               hours == 1 ? "" : "s", r);
      return;
    }
    if (parse_clock_hour(r, &target_hour)) {
      hours = hours_until_hour(wc.hour, target_hour);
      if (hours == 0)
        snprintf(msg, msgcap, "It is already that hour.");
      else
        snprintf(msg, msgcap, "%d hour%s until %02d:00.", hours,
                 hours == 1 ? "" : "s", target_hour);
      return;
    }
    snprintf(msg, msgcap,
             "Use: time until <morning|afternoon|evening|night> or time until "
             "<HH[:MM]|Ham|Hpm>.");
    return;
  }
  if (!strcmp(line, "weather") || !strcmp(line, "weather forecast") ||
      !strcmp(line, "weather report") || !strcmp(line, "weather full") ||
      !strcmp(line, "weather impact") || !strcmp(line, "weather effects")) {
    const char *arg = "";
    *turn_advance = 0;
    if (!strncmp(line, "weather ", 8)) arg = line + 8;
    format_weather_body(body, sizeof body, arg);
    append_dlc_mod_to_body(body, sizeof body,
                           aet_mods_character_weather_suffix());
    ui_fullscreen("WEATHER", body, pending_acc, did_fullscreen);
    return;
  }
  if (!strcmp(line, "temperature") || !strcmp(line, "temp") ||
      !strcmp(line, "temperature c") || !strcmp(line, "temperature f") ||
      !strcmp(line, "temp c") || !strcmp(line, "temp f") ||
      !strcmp(line, "temperature celsius") ||
      !strcmp(line, "temperature fahrenheit")) {
    const char *arg = "";
    *turn_advance = 0;
    if (!strncmp(line, "temperature ", 12))
      arg = line + 12;
    else if (!strncmp(line, "temp ", 5))
      arg = line + 5;
    format_temperature_msg(msg, msgcap, arg);
    return;
  }

  if (!strncmp(line, "drop all except ", 16) ||
      !strncmp(line, "drop all but ", 13)) {
    const char *ex =
        !strncmp(line, "drop all except ", 16) ? line + 16 : line + 13;
    while (*ex == ' ') ex++;
    drop_all_except(ex, msg, msgcap);
    return;
  }

  if (!strncmp(line, "put ", 4)) {
    char work[INPUT_LINE_MAX];
    char *sep = NULL;
    char *dest = NULL;
    strncpy(work, line + 4, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    sep = strstr(work, " into ");
    if (sep)
      dest = sep + 6;
    else if ((sep = strstr(work, " in ")))
      dest = sep + 4;
    else if ((sep = strstr(work, " on ")))
      dest = sep + 4;
    if (sep && dest) {
      *sep = '\0';
      strip_trailing_space(work);
      while (*dest == ' ') dest++;
      container_put_in(g_room, work, dest, msg, msgcap);
      return;
    }
  }

  if (!strncmp(line, "drop ", 5)) {
    const char *rest = line + 5;
    while (*rest == ' ') rest++;
    if (str_ieq(rest, "all"))
      drop_all(msg, msgcap);
    else
      drop_item(rest, msg, msgcap);
    return;
  }

  if (!strncmp(line, "take all except ", 16) ||
      !strncmp(line, "take all but ", 13)) {
    const char *ex =
        !strncmp(line, "take all except ", 16) ? line + 16 : line + 13;
    while (*ex == ' ') ex++;
    take_all_except(ex, msg, msgcap);
    return;
  }

  if (!strncmp(line, "take ", 5)) {
    char work[INPUT_LINE_MAX];
    char *from;
    const char *rest = line + 5;
    while (*rest == ' ') rest++;
    if (str_ieq(rest, "all")) {
      take_all(msg, msgcap);
      return;
    }
    strncpy(work, rest, sizeof work - 1);
    work[sizeof work - 1] = '\0';
    from = strstr(work, " from ");
    if (from && !parser_rhs_is_surface(from + 6)) {
      *from = '\0';
      strip_trailing_space(work);
      if (str_ieq(work, "all")) {
        container_take_all_from(g_room, from + 6, msg, msgcap);
        return;
      }
      container_take_from(g_room, work, from + 6, msg, msgcap);
      return;
    }
    take_item(rest, msg, msgcap);
    return;
  }

  if (!strncmp(line, "get all except ", 15) ||
      !strncmp(line, "get all but ", 12)) {
    const char *ex =
        !strncmp(line, "get all except ", 15) ? line + 15 : line + 12;
    while (*ex == ' ') ex++;
    take_all_except(ex, msg, msgcap);
    return;
  }

  if (!strncmp(line, "grab all except ", 16) ||
      !strncmp(line, "grab all but ", 13)) {
    const char *ex =
        !strncmp(line, "grab all except ", 16) ? line + 16 : line + 13;
    while (*ex == ' ') ex++;
    take_all_except(ex, msg, msgcap);
    return;
  }

  if (!strncmp(line, "get ", 4) || !strncmp(line, "grab ", 5)) {
    const char *rest = line + (line[0] == 'g' && line[1] == 'e' ? 4 : 5);
    while (*rest == ' ') rest++;
    if (str_ieq(rest, "all"))
      take_all(msg, msgcap);
    else
      take_item(rest, msg, msgcap);
    return;
  }

  {
    char hint[96];
    char fw[64];
    const char *sp = strchr(line, ' ');
    size_t wl = sp ? (size_t)(sp - line) : strlen(line);
    if (wl >= sizeof fw) wl = sizeof fw - 1;
    memcpy(fw, line, wl);
    fw[wl] = '\0';
    if (wl >= 3) {
      int ties;
      const char *best;
      int bestd = parser_topverb_min_dist(fw, 2, &ties, &best);
      if (ties == 1 && best && bestd > 0 && bestd <= 2 &&
          !(bestd == 2 && wl < 6) && strcmp(fw, best) != 0) {
        diag_push("unknown", line);
        causal_push("unknown-command", line);
        if (sp) {
          const char *rest = sp;
          while (*rest == ' ') rest++;
          if (rest[0])
            snprintf(msg, msgcap, "Unknown command. Did you mean \"%s %s\"?",
                     best, rest);
          else
            snprintf(msg, msgcap, "Unknown command. Did you mean \"%s\"?", best);
        } else {
          snprintf(msg, msgcap, "Unknown command. Did you mean \"%s\"?", best);
        }
        return;
      }
    }
    suggest_typo(fw, hint, sizeof hint);
    diag_push("unknown", line);
    causal_push("unknown-command", line);
    snprintf(msg, msgcap, "You do not recognize that command.%s Try 'help'.",
             hint);
  }
}

static void init_new_game(int start_room) {
  if (start_room < 0 || start_room >= WORLD_ROOM_COUNT) return;
  disambig_reset_all();
  init_items_from_world();
  place_persistent_story_items();
  containers_init_new_game();
  apply_reference_new_game_bootstrap();
  g_room = start_room;
  g_turns = 0;
  g_score = 0;
  g_max_health = AETER_START_HP;
  g_health = AETER_START_HP;
  g_inv_n = 0;
  g_front_unlocked = 0;
  g_shed_unlocked = 0;
  g_hist_n = 0;
  g_note_n = 0;
  g_verbose_room = 1;
  g_recap_n = 0;
  g_last_npc[0] = '\0';
  g_last_topic[0] = '\0';
  g_ready_item[0] = '\0';
  eq_clear_all();
  merchant_rep_clear();
  memset(g_visited, 0, sizeof g_visited);
  g_visited[start_room] = 1;
  g_transcript[0] = '\0';
  clear_focus();
  g_last_dropped[0] = '\0';
  g_last_cmd[0] = '\0';
  cmd_hist_clear();
  g_craft_proficiency = 1;
  g_craft_prof_n = 0;
  g_lockpick_last_noise = 0;
  g_lockpick_suspicion = 0;
  diag_clear();
  causal_clear();
  trade_history_clear();
  rel_hist_clear();
  barter_clear();
  g_npc_validation_checked = 0;
  g_npc_validation_warnings = 0;
  g_autosave_enabled = 1;
  validate_npc_world_refs();
#ifdef AETER_MINIGAMES
  g_mgt_quicksave_valid = 0;
  g_mgt_ready = 0;
#endif
}

static void main_menu_box_row(char d, const char *it, const char *al) {
  char mid1[48], mid2[24];
  int itlen = (int)strlen(it);
  int pad1 = 26 - itlen;
  int pad2;
  if (pad1 < 0) pad1 = 0;
  snprintf(mid1, sizeof mid1, "%s%*s  ", it, pad1, "");
  snprintf(mid2, sizeof mid2, "( %-4s )", al);
  pad2 = 58 - 3 - (int)strlen(mid1) - (int)strlen(mid2);
  if (pad2 < 0) pad2 = 0;
  printf("%s               |[%s%c%s]%s%s%s%s%s%s%s%*s%s|%s\n", C_BORDER, C_ITEM, d,
         C_BORDER, C_RESET, C_HEADING, mid1, C_RESET, C_MUTED, mid2, C_RESET,
         pad2, "", C_BORDER, C_RESET);
}

static void print_main_menu_screen(void) {
  static const char *const logo[] = {
      "        @@@@@@  @@@@@@@@ @@@@@@@ @@@@@@@@ @@@@@@@  @@@  @@@ @@@ @@@@@@@ "
      "@@@@@@   @@@@@@ ",
      "       @@!  @@@ @@!        @!!   @@!      @@!  @@@ @@!@!@@@ @@!   @!!  "
      "@@!  @@@ !@@     ",
      "       @!@!@!@! @!!!:!     @!!   @!!!:!   @!@!!@!  @!@@!!@! !!@   @!!  "
      "@!@!@!@!  !@@!!  ",
      "       !!:  !!! !!:        !!:   !!:      !!: :!!  !!:  !!! !!:   !!:  "
      "!!:  !!!     !:! ",
      "        :   : : : :: ::     :    : :: ::   :   : : ::    :  :      :    "
      ":   : : ::.: :  "};
  int i;
  const char *ttl = "A E T E R N I T A S   S Y S T E M";
  int pad = (int)(sizeof AETER_MENU_RULE - 1) - 3 - (int)strlen(ttl) -
            (int)strlen(AETER_MAIN_MENU_VER);

  if (pad < 1) pad = 1;

  for (i = 0; i < 5; i++)
    printf("%s%s%s\n", C_TITLE, logo[i], C_RESET);
  printf("\n");
  printf("%s%s%s\n", C_BORDER, AETER_MENU_RULE, C_RESET);
  printf("   %s%s%s%*s%s%s%s\n", C_HEADING, ttl, C_RESET, pad, "", C_EXIT,
         AETER_MAIN_MENU_VER, C_RESET);
  printf("%s%s%s\n\n", C_BORDER, AETER_MENU_RULE, C_RESET);

  printf("%s               +----------------------------------------------------------+%s\n",
         C_BORDER, C_RESET);
  printf("%s               |                                                          |%s\n",
         C_BORDER, C_RESET);
  main_menu_box_row('1', "Begin a New Journey", "new");
  printf("%s               |                                                          |%s\n",
         C_BORDER, C_RESET);
  main_menu_box_row('2', "Restore a Saved Game", "load");
  printf("%s               |                                                          |%s\n",
         C_BORDER, C_RESET);
  main_menu_box_row('3', "System Documentation", "help");
  printf("%s               |                                                          |%s\n",
         C_BORDER, C_RESET);
  main_menu_box_row('4', "Manage Game Mods", "mods");
  printf("%s               |                                                          |%s\n",
         C_BORDER, C_RESET);
  main_menu_box_row('5', "Exit to Computer", "quit");
  printf("%s               |                                                          |%s\n",
         C_BORDER, C_RESET);
  printf("%s               +----------------------------------------------------------+%s\n\n",
         C_BORDER, C_RESET);

  printf("  %sSelect an option from the menu above:%s\n\n", C_MUTED, C_RESET);
  printf("  %s>>%s ", C_TITLE, C_RESET);
  fflush(stdout);
}

static int run_main_menu(int start_room) {
  char line[INPUT_LINE_MAX];
  char err[512];
  char h[AETER_HELP_BODY_CAP];
  int i;

  for (;;) {
    clear_frame();
    print_main_menu_screen();
    if (!fgets(line, sizeof line, stdin)) return 0;
    strip_trailing_space(line);
    chomp_line(line);
    for (i = 0; line[i]; i++) line[i] = (char)tolower((unsigned char)line[i]);
    if (line[0] == '1' || !strcmp(line, "new")) {
      init_new_game(start_room);
      run_character_creation(aet_autotest());
      grant_starting_loadout();
      eq_bootstrap_from_character();
      (void)write_save_file();
      return 1;
    }
    if (line[0] == '2' || !strcmp(line, "load")) {
      if (load_game(err, sizeof err)) {
        snprintf(g_transcript, sizeof g_transcript, "%s", err);
        return 1;
      }
      ui_block_pause("LOAD FAILED", err);
      continue;
    }
    if (line[0] == '3' || !strcmp(line, "help") || !strcmp(line, "?")) {
      fill_help_text(h, sizeof h);
      ui_block_pause("HELP", h);
      continue;
    }
    if (line[0] == '4' || !strcmp(line, "mods") || !strcmp(line, "mod")) {
      char mbuf[5120];
      aet_mods_format_status(mbuf, sizeof mbuf);
      ui_block_pause("MOD MANAGER", mbuf);
      continue;
    }
    if (line[0] == '5' || !strcmp(line, "quit") || !strcmp(line, "q") ||
        !strcmp(line, "exit"))
      return 0;
    ui_block_pause(
        "MENU",
        "Unknown choice. Use 1–5 or new / load / help / mods / quit.\n");
  }
}

static void acc_append(char *acc, size_t acccap, const char *msg) {
  size_t L;
  if (!msg || !msg[0] || acccap < 2) return;
  L = strnlen(acc, acccap - 1);
  if (L > 0)
    (void)snprintf(acc + L, acccap - L, "\n%s", msg);
  else
    (void)snprintf(acc, acccap, "%s", msg);
}

static void handle_line(char *line) {
  char work[INPUT_LINE_MAX];
  char acc[TRANSCRIPT_CAP];
  char segments[16][INPUT_LINE_MAX];
  int seg_n = 0;
  int si;
  int any = 0;
  char *p;
  int any_adv = 0;
  int is_repeat = 0;

  g_return_to_menu = 0;
  for (p = line; *p; p++) *p = (char)tolower((unsigned char)*p);
  strip_trailing_space(line);
  if (!line[0]) return;

  strncpy(work, line, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  commas_then_to_semicolon(work);
  {
    char chk[INPUT_LINE_MAX];
    strncpy(chk, work, sizeof chk - 1);
    chk[sizeof chk - 1] = '\0';
    if (!strcmp(chk, "g") || !strcmp(chk, "again") ||
        !strcmp(chk, "repeat")) {
      if (!g_last_cmd[0]) {
        snprintf(g_transcript, sizeof g_transcript, "%s — nothing to repeat yet.",
                 pc_display_name());
        paint_normal();
        return;
      }
      is_repeat = 1;
      strncpy(work, g_last_cmd, sizeof work - 1);
      work[sizeof work - 1] = '\0';
    } else if (chk[0] == '!' && chk[1] >= '1' && chk[1] <= '9') {
      char *end = NULL;
      long hn = strtol(chk + 1, &end, 10);
      char recalled[INPUT_LINE_MAX];
      if (end && *end == '\0' && hn >= 1 &&
          cmd_hist_get_from_latest((int)hn, recalled, sizeof recalled)) {
        strncpy(work, recalled, sizeof work - 1);
        work[sizeof work - 1] = '\0';
      } else {
        snprintf(g_transcript, sizeof g_transcript,
                 "%s — no command history entry #%ld.", pc_display_name(), hn);
        paint_normal();
        return;
      }
    } else if (!strncmp(chk, "history ", 8)) {
      const char *arg = chk + 8;
      char *end = NULL;
      long hn;
      while (*arg == ' ') arg++;
      if (str_ieq(arg, "list")) {
        format_cmd_history_body(g_transcript, sizeof g_transcript);
        paint_normal();
        return;
      }
      hn = strtol(arg, &end, 10);
      if (end > arg && *end == '\0' && hn >= 1) {
        char recalled[INPUT_LINE_MAX];
        if (cmd_hist_get_from_latest((int)hn, recalled, sizeof recalled)) {
          strncpy(work, recalled, sizeof work - 1);
          work[sizeof work - 1] = '\0';
        } else {
          snprintf(g_transcript, sizeof g_transcript,
                   "%s — no command history entry #%ld.", pc_display_name(), hn);
          paint_normal();
          return;
        }
      }
    }
  }

  {
    char chain_for_repeat[INPUT_LINE_MAX];
    size_t wlen = strnlen(work, sizeof chain_for_repeat - 1);
    memcpy(chain_for_repeat, work, wlen);
    chain_for_repeat[wlen] = '\0';

    acc[0] = '\0';
    for (p = work; p && *p && seg_n < 16;) {
      char *end = strchr(p, ';');
      size_t n;
      if (end) *end = '\0';
      while (*p == ' ') p++;
      strip_trailing_space(p);
      if (*p) {
        n = strnlen(p, INPUT_LINE_MAX - 1);
        memcpy(segments[seg_n], p, n);
        segments[seg_n][n] = '\0';
        seg_n++;
      }
      if (!end) break;
      p = end + 1;
    }
    for (si = 0; si < seg_n; si++) {
      char *seg = segments[si];
      while (*seg == ' ') seg++;
      strip_trailing_space(seg);
      if (!seg[0]) continue;
      any = 1;
      {
        char msg[PROCESS_MSG_CAP];
        int adv = 0;
        int did_fs = 0;
        process_command(seg, msg, sizeof msg, &adv, acc, &did_fs);
        
        if (g_return_to_menu) {
          if (any_adv) {
            relationship_decay_tick();
            (void)write_save_file();
            autosave_write_shadow();
          }
          return;
        }
        if (adv > 0) {
          int prev_turn = g_turns;
          g_turns += adv;
          note_npc_routine_changes(prev_turn, g_turns);
          any_adv = 1;
        }
        append_auto_causal_hint(seg, msg, sizeof msg);
        if (msg[0]) acc_append(acc, sizeof acc, msg);
      }
    }
    if (!any) return;
    snprintf(g_transcript, sizeof g_transcript, "%s", acc);
    if (acc[0]) recap_push(acc);
    paint_normal();
    if (any_adv) {
      relationship_decay_tick();
      (void)write_save_file();
      autosave_write_shadow();
    }
    if (!is_repeat) {
      strncpy(g_last_cmd, chain_for_repeat, sizeof g_last_cmd - 1);
      g_last_cmd[sizeof g_last_cmd - 1] = '\0';
      cmd_hist_push(chain_for_repeat);
    }
  }
}

static void usage_stdout(void) {
  fputs(
      "Aeternitas64 — text adventure (stdin).\n"
      "\n"
      "  aeternitas64 [options]\n"
      "\n"
      "Options:\n"
      "  --save <path>     Quick save / load file (default: beside executable)\n"
      "                    Dev starter: aeternitas64_dev_save.txt (repo root; then "
      "load at title)\n"
      "  --mods <dir>      Mod pack root (overrides AETER_MODS and default)\n"
#if defined(_WIN32) && defined(AETER_WIN_PICKERS)
      "  --pick-save       (Windows) Pick quicksave file in a file dialog\n"
      "  --pick-mods       (Windows) Pick mod pack root folder in a dialog\n"
#endif
      "  -h, --help        This text\n"
      "\n"
      "Environment:\n"
      "  AETER_AUTOTEST=1  Skip splash and blocking Enter waits (CI; see tools/autotest)\n"
      "  AETER_MODS=<dir>  Folder of mod packs (if no --mods / in-game mods directory)\n"
      "\n"
      "Ship a small .exe; put saves and mods on a writable drive. First run may create\n"
      "sample pack under the active mods path. In-game:  help modding  (arrow-key guide).\n"
      "\n"
      "Regenerate world:  py -3 tools/extract_world_tables_from_exe.py; py -3 "
      "tools/build_world_c_from_recovered_json.py\n",
      stdout);
}

static void parse_args(int argc, char **argv) {
  int i;
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
      usage_stdout();
      exit(0);
    }
    if (!strcmp(argv[i], "--save")) {
      if (i + 1 >= argc || !argv[i + 1][0]) {
        fputs("aeternitas64: --save requires a non-empty path (try --help)\n",
              stderr);
        exit(2);
      }
      i++;
      strncpy(g_save_path, argv[i], sizeof g_save_path - 1);
      g_save_path[sizeof g_save_path - 1] = '\0';
      continue;
    }
#if defined(_WIN32) && defined(AETER_WIN_PICKERS)
    if (!strcmp(argv[i], "--pick-save")) {
      if (!win32_browse_save_file(g_save_path, sizeof g_save_path)) {
        fputs("aeternitas64: save file selection cancelled\n", stderr);
        exit(1);
      }
      continue;
    }
#endif
    if (!strcmp(argv[i], "--mods")) {
      if (i + 1 >= argc || !argv[i + 1][0]) {
        fputs("aeternitas64: --mods requires a non-empty directory (try --help)\n",
              stderr);
        exit(2);
      }
      i++;
      strncpy(g_mods_override, argv[i], sizeof g_mods_override - 1);
      g_mods_override[sizeof g_mods_override - 1] = '\0';
      continue;
    }
#if defined(_WIN32) && defined(AETER_WIN_PICKERS)
    if (!strcmp(argv[i], "--pick-mods")) {
      if (!win32_browse_mods_directory(g_mods_override,
                                       sizeof g_mods_override)) {
        fputs("aeternitas64: mods folder selection cancelled\n", stderr);
        exit(1);
      }
      continue;
    }
#endif
    fprintf(stderr, "Unknown option: %s (try --help)\n", argv[i]);
    exit(2);
  }
}

int main(int argc, char **argv) {
  char line[INPUT_LINE_MAX];
  int start;

#if defined(_WIN32)
  win32_console_utf8_begin();
#endif
  init_save_path(argv[0]);
  parse_args(argc, argv);
  mods_init_from_env();
  ui_init_color();

  if (WORLD_ROOM_COUNT > MAX_WORLD_ROOMS) {
    fprintf(stderr, "WORLD_ROOM_COUNT %d > MAX_WORLD_ROOMS %d — raise cap.\n",
            WORLD_ROOM_COUNT, MAX_WORLD_ROOMS);
    return 1;
  }

  start = world_room_index("west_of_house");
  if (start < 0) {
    fprintf(stderr, "west_of_house missing from generated world.\n");
    return 1;
  }

  splash_wait();
  age_disclaimer_wait();
  for (;;) {
    if (!run_main_menu(start)) {
      printf("Goodbye.\n");
      return 0;
    }
    paint_normal();
    for (;;) {
      printf("\n%s>> %s", C_PROMPT, C_RESET);
      fflush(stdout);
      if (!fgets(line, sizeof line, stdin)) {
        printf("Goodbye.\n");
        return 0;
      }
      line[strcspn(line, "\r\n")] = '\0';
      handle_line(line);
      if (g_return_to_menu) break;
    }
  }
}
