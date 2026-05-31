#include "mgt_bus.h"
#include "mgt.h"

#include "mgt_sync.h"

#include <stdio.h>
#include <string.h>

static int slug_in_list(const char *slug, const char *const *list) {
  int i;
  if (!list) return 1;
  if (!slug || !slug[0]) return 0;
  for (i = 0; list[i]; i++)
    if (!strcmp(slug, list[i])) return 1;
  return 0;
}

static int sim_has_any(const MgtGameSim *sim, const char *const *items) {
  int i;
  if (!items) return 1;
  for (i = 0; items[i]; i++)
    if (mgt_sim_has_item(sim, items[i])) return 1;
  return 0;
}

static const char *const room_tavern[] = {"tavern_common_room", "tavern_kitchen",
                                          NULL};
static const char *const room_kitchen[] = {"tavern_kitchen", NULL};
static const char *const room_farm[] = {"farm", NULL};
static const char *const room_water[] = {"pond", "river_shore", "fishing_piers",
                                           "stream", NULL};
static const char *const room_wild[] = {"meadow", "deep_forest", "forest_path",
                                        NULL};
static const char *const room_locked[] = {"east_of_house", "hidden_cellar",
                                          "castle", NULL};

static const char *const item_lock[] = {"lockpick", "fine_lockpick", "rusty_pick",
                                        "skeleton_key", NULL};
static const char *const item_bow[] = {"bow", "shortbow", "hunting_bow", NULL};
static const char *const item_paper[] = {"parchment", "paper", NULL};

static const MgtModuleSpec g_modules[] = {
    {"test_cube", "TESTCUBE",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_COIN) |
         MGT_PIN_BIT(MGT_PIN_PACK) | MGT_PIN_BIT(MGT_PIN_MGST),
     NULL, NULL},
    {"lockpick", "LOCKPICK",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_COIN) |
         MGT_PIN_BIT(MGT_PIN_PACK) | MGT_PIN_BIT(MGT_PIN_ROOM) |
         MGT_PIN_BIT(MGT_PIN_SKILL) | MGT_PIN_BIT(MGT_PIN_MGST),
     room_locked, item_lock},
    {"piano", "PIANO",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_ROOM) |
         MGT_PIN_BIT(MGT_PIN_MGST),
     room_tavern, NULL},
    {"fishing", "FISHING",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_COIN) |
         MGT_PIN_BIT(MGT_PIN_PACK) | MGT_PIN_BIT(MGT_PIN_ROOM) |
         MGT_PIN_BIT(MGT_PIN_WEATH) | MGT_PIN_BIT(MGT_PIN_MGST),
     room_water, NULL},
    {"farming", "FARMING",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_ROOM) |
         MGT_PIN_BIT(MGT_PIN_WEATH) | MGT_PIN_BIT(MGT_PIN_MGST),
     room_farm, NULL},
    {"cooking", "COOKING",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_COIN) |
         MGT_PIN_BIT(MGT_PIN_ROOM) | MGT_PIN_BIT(MGT_PIN_SKILL) |
         MGT_PIN_BIT(MGT_PIN_MGST),
     room_kitchen, NULL},
    {"writing", "WRITING",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_PACK) |
         MGT_PIN_BIT(MGT_PIN_MGST),
     NULL, item_paper},
    {"reading", "READING",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_PACK) |
         MGT_PIN_BIT(MGT_PIN_MGST),
     NULL, item_paper},
    {"gambling", "GAMBLING",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_COIN) |
         MGT_PIN_BIT(MGT_PIN_ROOM) | MGT_PIN_BIT(MGT_PIN_MGST),
     room_tavern, NULL},
    {"hunting", "HUNTING",
     MGT_PIN_BIT(MGT_PIN_PWR) | MGT_PIN_BIT(MGT_PIN_PACK) |
         MGT_PIN_BIT(MGT_PIN_ROOM) | MGT_PIN_BIT(MGT_PIN_TURN) |
         MGT_PIN_BIT(MGT_PIN_MGST),
     room_wild, item_bow},
};

typedef struct {
  const char *name;
  const char *desc;
  void (*apply)(MgtGameSim *sim);
} MgtScenario;

static void sim_clear_inv(MgtGameSim *sim) { sim->inv_n = 0; }

static void sim_push(MgtGameSim *sim, const char *id) {
  mgt_sim_give_item(sim, id);
}

static void scen_tavern(MgtGameSim *sim) {
  mgt_game_sim_defaults(sim);
  sim->coins = 80;
  snprintf(sim->room_slug, sizeof sim->room_slug, "tavern_common_room");
}

static void scen_kitchen(MgtGameSim *sim) {
  mgt_game_sim_defaults(sim);
  sim->coins = 40;
  sim->craft_proficiency = 15;
  sim->cha = 14;
  snprintf(sim->room_slug, sizeof sim->room_slug, "tavern_kitchen");
}

static void scen_farm(MgtGameSim *sim) {
  mgt_game_sim_defaults(sim);
  snprintf(sim->room_slug, sizeof sim->room_slug, "farm");
  snprintf(sim->weather, sizeof sim->weather, "clear");
}

static void scen_pond(MgtGameSim *sim) {
  mgt_game_sim_defaults(sim);
  sim->coins = 25;
  snprintf(sim->room_slug, sizeof sim->room_slug, "pond");
  snprintf(sim->weather, sizeof sim->weather, "overcast");
}

static void scen_hunt(MgtGameSim *sim) {
  mgt_game_sim_defaults(sim);
  sim->wis = 14;
  sim->adventure_turn = 12;
  snprintf(sim->room_slug, sizeof sim->room_slug, "deep_forest");
  sim_clear_inv(sim);
  sim_push(sim, "bow");
}

static void scen_shed(MgtGameSim *sim) {
  mgt_game_sim_defaults(sim);
  sim->intl = 13;
  snprintf(sim->room_slug, sizeof sim->room_slug, "east_of_house");
  sim_clear_inv(sim);
  sim_push(sim, "lockpick");
  sim_push(sim, "tension_wrench");
}

static const MgtScenario g_scenarios[] = {
    {"Tavern", "Piano / gambling", scen_tavern},
    {"Kitchen", "Cooking shift", scen_kitchen},
    {"Farm", "Plant / harvest", scen_farm},
    {"Pond", "Fishing", scen_pond},
    {"Forest", "Hunting + bow", scen_hunt},
    {"Shed", "Lockpick at east_of_house", scen_shed},
};

const MgtModuleSpec *mgt_bus_module_spec(const char *id) {
  int i;
  if (!id) return NULL;
  for (i = 0; i < (int)(sizeof g_modules / sizeof g_modules[0]); i++)
    if (!strcmp(g_modules[i].id, id)) return &g_modules[i];
  return NULL;
}

int mgt_bus_module_count(void) {
  return (int)(sizeof g_modules / sizeof g_modules[0]);
}

const MgtModuleSpec *mgt_bus_module_at(int index) {
  if (index < 0 || index >= mgt_bus_module_count()) return NULL;
  return &g_modules[index];
}

int mgt_bus_module_index(const char *id) {
  int i;
  if (!id) return 0;
  for (i = 0; i < mgt_bus_module_count(); i++)
    if (!strcmp(g_modules[i].id, id)) return i;
  return 0;
}

void mgt_bus_auto_wire(MgtGameSim *sim, const MgtModuleSpec *mod) {
  if (!sim || !mod) return;
  if (!strcmp(mod->id, "test_cube")) {
    mgt_game_sim_defaults(sim);
    return;
  }
  if (!strcmp(mod->id, "lockpick")) {
    scen_shed(sim);
    return;
  }
  if (!strcmp(mod->id, "piano") || !strcmp(mod->id, "gambling")) {
    scen_tavern(sim);
    return;
  }
  if (!strcmp(mod->id, "fishing")) {
    scen_pond(sim);
    return;
  }
  if (!strcmp(mod->id, "farming")) {
    scen_farm(sim);
    return;
  }
  if (!strcmp(mod->id, "cooking")) {
    scen_kitchen(sim);
    return;
  }
  if (!strcmp(mod->id, "hunting")) {
    scen_hunt(sim);
    return;
  }
  if (!strcmp(mod->id, "writing") || !strcmp(mod->id, "reading")) {
    mgt_game_sim_defaults(sim);
    return;
  }
  mgt_game_sim_defaults(sim);
}

int mgt_bus_scenario_count(void) {
  return (int)(sizeof g_scenarios / sizeof g_scenarios[0]);
}

const char *mgt_bus_scenario_name(int index) {
  if (index < 0 || index >= mgt_bus_scenario_count()) return "?";
  return g_scenarios[index].name;
}

const char *mgt_bus_scenario_desc(int index) {
  if (index < 0 || index >= mgt_bus_scenario_count()) return "";
  return g_scenarios[index].desc;
}

void mgt_bus_apply_scenario(MgtGameSim *sim, int index) {
  if (!sim) return;
  if (index < 0) index = 0;
  if (index >= mgt_bus_scenario_count()) index = 0;
  g_scenarios[index].apply(sim);
}

void mgt_bus_probe(const MgtGameSim *sim, const MgtPersistentState *st,
                   const MgtModuleSpec *mod, MgtBusProbe *out) {
  unsigned mask;
  int p, all = 1;
  if (!out) return;
  memset(out, 0, sizeof *out);
  if (!mod) {
    snprintf(out->summary, sizeof out->summary, "No module selected.");
    return;
  }
  out->pin_mated[MGT_PIN_PWR] = 1;
  out->pin_mated[MGT_PIN_COIN] = sim && sim->coins >= 0;
  out->pin_mated[MGT_PIN_PACK] = sim && sim->inv_n > 0;
  out->pin_mated[MGT_PIN_ROOM] =
      sim && slug_in_list(sim->room_slug, mod->room_any);
  out->pin_mated[MGT_PIN_SKILL] =
      sim && (sim->craft_proficiency >= 1 || sim->intl >= 5);
  out->pin_mated[MGT_PIN_WEATH] = sim && sim->weather[0];
  out->pin_mated[MGT_PIN_TURN] = sim && sim->adventure_turn >= 0;
  out->pin_mated[MGT_PIN_MGST] = st && st->magic == MGT_SAVE_MAGIC;

  out->room_ok = out->pin_mated[MGT_PIN_ROOM];
  out->items_ok = sim_has_any(sim, mod->need_any_item);
  if (mod->need_any_item && !out->items_ok) all = 0;
  if (mod->room_any && !out->room_ok) all = 0;

  mask = mod->required_pins;
  for (p = 0; p < MGT_PIN_COUNT; p++) {
    if (!(mask & MGT_PIN_BIT((MgtBusPin)p))) continue;
    if (!out->pin_mated[p]) all = 0;
  }
  out->ready = all;

  if (out->ready)
    snprintf(out->summary, sizeof out->summary,
             "CONTACT OK — all pins mated for %s.", mod->cube_label);
  else if (mod->room_any && !out->room_ok)
    snprintf(out->summary, sizeof out->summary,
             "ROOM pin open — move bus sim (G / 1-6).");
  else if (mod->need_any_item && !out->items_ok)
    snprintf(out->summary, sizeof out->summary,
             "PACK pin open — scenario lacks required item.");
  else
    snprintf(out->summary, sizeof out->summary,
             "Pins misaligned — probe with T before INSERT.");

  snprintf(out->fix_hint, sizeof out->fix_hint,
           "Native path: copy tested cube to minigames/ + build_aeternitas64.bat");
}

static void draw_module_viewport(MgtCanvas *c, int x, int y, const char *id) {
  if (!id) return;
  if (!strcmp(id, "test_cube")) {
    mgt_canvas_write(c, x, y,     "|~BUS SYNC~|");
    mgt_canvas_write(c, x, y + 1, "| *  *  *  |");
    mgt_canvas_write(c, x, y + 2, "|  PULSE   |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "lockpick")) {
    mgt_canvas_write(c, x, y,     "|  .--.    |");
    mgt_canvas_write(c, x, y + 1, "| /DOOR\\   |");
    mgt_canvas_write(c, x, y + 2, "|  pins    |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "piano")) {
    mgt_canvas_write(c, x, y,     "| ~~~keys~~|");
    mgt_canvas_write(c, x, y + 1, "||#|#|#|#|#|");
    mgt_canvas_write(c, x, y + 2, "|  tavern  |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "fishing")) {
    mgt_canvas_write(c, x, y,     "|  ~pond~  |");
    mgt_canvas_write(c, x, y + 1, "|   ))     |");
    mgt_canvas_write(c, x, y + 2, "|  /fish   |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "farming")) {
    mgt_canvas_write(c, x, y,     "| #=#=#=#  |");
    mgt_canvas_write(c, x, y + 1, "| rows     |");
    mgt_canvas_write(c, x, y + 2, "|  ^crop^  |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "cooking")) {
    mgt_canvas_write(c, x, y,     "|  skillet |");
    mgt_canvas_write(c, x, y + 1, "|  ~steam~ |");
    mgt_canvas_write(c, x, y + 2, "|  diner   |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "hunting")) {
    mgt_canvas_write(c, x, y,     "|  /\\_/\\   |");
    mgt_canvas_write(c, x, y + 1, "| forest   |");
    mgt_canvas_write(c, x, y + 2, "|  bow->   |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "gambling")) {
    mgt_canvas_write(c, x, y,     "| .---.    |");
    mgt_canvas_write(c, x, y + 1, "| |:::|dice|");
    mgt_canvas_write(c, x, y + 2, "| '---'    |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  if (!strcmp(id, "writing") || !strcmp(id, "reading")) {
    mgt_canvas_write(c, x, y,     "|  scroll  |");
    mgt_canvas_write(c, x, y + 1, "| ~~~~~~~~ |");
    mgt_canvas_write(c, x, y + 2, "|  ink     |");
    mgt_canvas_write(c, x, y + 3, "|__________|");
    return;
  }
  mgt_canvas_write(c, x, y + 1, "|  module  |");
  mgt_canvas_write(c, x, y + 3, "|__________|");
}

static void draw_linked_menu(MgtCanvas *c, int x, int y, int selected) {
  int i, n = mgt_registry_count();
  const MgtMinigame *list = mgt_registry();
  char line[96];
  mgt_canvas_box(c, x, y, 28, 10, "LINKED");
  if (n <= 0 || !list) {
    mgt_canvas_write(c, x + 2, y + 2, "(none in build)");
    return;
  }
  for (i = 0; i < n && i < 7; i++) {
    snprintf(line, sizeof line, "%c %-20s", (i == selected) ? '>' : ' ',
             list[i].title);
    mgt_canvas_write(c, x + 2, y + 2 + i, line);
  }
}

static void draw_socket(MgtCanvas *c, int x, int y, int plugged) {
  mgt_canvas_box(c, x, y, 28, 8, "SOCKET");
  mgt_canvas_write(c, x + 2, y + 2, "~~~~~~~~~~~~~~~~~~");
  mgt_canvas_write(c, x + 2, y + 3, "| || || || || || |");
  mgt_canvas_write(c, x + 2, y + 4, "~~~~~~~~~~~~~~~~~~");
  mgt_canvas_write(c, x + 2, y + 5, "      ====^====");
  if (plugged)
    mgt_canvas_write(c, x + 6, y + 6, "[ seated ]");
}

static void draw_cube_preview(MgtCanvas *c, int x, int y, const MgtModuleSpec *mod) {
  char line[96];
  if (!mod) return;
  mgt_canvas_box(c, x, y, 28, 10, mod->cube_label);
  draw_module_viewport(c, x + 2, y + 2, mod->id);
  mgt_canvas_write(c, x + 2, y + 7, "+----------+");
  snprintf(line, sizeof line, "| o o o o o|");
  mgt_canvas_write(c, x + 2, y + 8, line);
}

void mgt_bus_draw_bench(MgtCanvas *c, const MgtGameSim *sim,
                        const MgtPersistentState *st, const MgtModuleSpec *mod,
                        int plugged, const MgtBusProbe *probe,
                        const MgtBenchVisual *vis) {
  char line[96];
  int sel;

  (void)sim;
  (void)probe;
  if (!c) return;
  sel = vis ? vis->linked_selected : 0;

  mgt_canvas_box(c, 1, 0, 94, 20, " MODULE BENCH ");

  draw_linked_menu(c, 2, 2, sel);
  if (mod) {
    draw_cube_preview(c, 34, 2, mod);
    draw_socket(c, 64, 2, plugged);
  } else {
    mgt_canvas_write(c, 34, 4, "Select a linked module.");
  }

  if (vis && vis->show_ejected && vis->ejected_label) {
    snprintf(line, sizeof line, "Ejected %s — ready for minigames/",
             vis->ejected_label);
    mgt_canvas_write(c, 2, 14, line);
  }

  if (st && st->last_banner[0]) {
    snprintf(line, sizeof line, "%.78s", st->last_banner);
    mgt_canvas_write(c, 2, 16, line);
  }

  mgt_canvas_write(c, 2, 18, "Up/Down select   Enter run   Q quit");
}
