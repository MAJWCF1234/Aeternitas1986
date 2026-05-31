#include "mgt_host.h"

#include "mgt_bus.h"
#include "mgt_game_bridge.h"
#include "mgt_platform.h"
#include "mgt_sync.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static MgtGameSim g_game;
static MgtPersistentState g_state;
static MgtSession g_session;
static int g_host_open;
static int g_harness_sim_bridge;
static int g_plugged;
static MgtBusProbe g_probe;
static int g_seat_phase;
static int g_show_ejected;
static char g_ejected_label[16];
static int g_run_lock;
static int g_menu_dirty;

static MgtSession *g_frame_session;
static char g_frame_title[64];
static char g_frame_controls[96];

static void harness_sync_from(MgtPersistentState *st, void *ctx) {
  (void)ctx;
  mgt_sync_from_world(st, &g_game);
}

static void harness_sync_to(const MgtPersistentState *st, void *ctx) {
  (void)ctx;
  mgt_sync_to_world((MgtPersistentState *)st, &g_game);
}

static void harness_give_item(const char *id, void *ctx) {
  (void)ctx;
  mgt_sim_give_item(&g_game, id);
}

static void status_line(char *out, size_t cap) {
  snprintf(out, cap,
           "ADV $%d inv:%d  |  MG $%d eng:%d surv:%d cook:%d fish:L%d",
           g_game.coins, g_game.inv_n, g_state.money, g_state.skill_engineering,
           g_state.skill_survival, g_state.skill_cooking, g_state.fishing_level);
}

void mgt_frame_present(const MgtCanvas *c) {
  if (g_frame_session && g_frame_session->state) {
    char status[96];
    status_line(status, sizeof status);
    mgt_host_present(c, g_frame_title, status, g_frame_controls);
  } else
    mgt_present(c);
}

void mgt_host_begin_frame(MgtSession *session, const char *title,
                          const char *controls) {
  g_frame_session = session;
  snprintf(g_frame_title, sizeof g_frame_title, "%s",
           title ? title : "MINIGAME");
  snprintf(g_frame_controls, sizeof g_frame_controls, "%s",
           controls ? controls : "ESC - return to menu");
}

void mgt_host_end_frame(void) {
  g_frame_session = NULL;
  g_frame_title[0] = '\0';
  g_frame_controls[0] = '\0';
}

void mgt_host_init(unsigned rng_seed_if_new) {
  int loaded = mgt_harness_load(&g_game, &g_state, NULL);
  if (!loaded && rng_seed_if_new) g_state.rng_seed = rng_seed_if_new;
  mgt_seed(g_state.rng_seed);
  g_session.state = &g_state;
  g_session.from_game = 0;
  g_session.adventure_embedded = 0;
  g_session.game_resume = NULL;
  g_session.game_resume_ctx = NULL;
  if (!g_harness_sim_bridge) {
    aet_minigames_register_sync(harness_sync_from, harness_sync_to, NULL, NULL);
    aet_minigames_register_give(harness_give_item, NULL);
    g_harness_sim_bridge = 1;
  }
}

void mgt_host_shutdown(void) {
  if (g_host_open) {
    mgt_input_host_end();
    mgt_terminal_shutdown();
    g_host_open = 0;
  }
  mgt_harness_save(&g_game, &g_state, NULL);
}

MgtPersistentState *mgt_host_state(void) { return &g_state; }

MgtGameSim *mgt_host_game_sim(void) { return &g_game; }

MgtSession *mgt_host_session(void) { return &g_session; }

void mgt_host_present(const MgtCanvas *canvas, const char *panel_title,
                      const char *status_line, const char *controls) {
  char frame[16384];
  size_t len = mgt_host_tty_encode_frame(frame, sizeof frame, canvas, panel_title,
                                           status_line, controls);
  if (len > 0) mgt_tty_flush_bytes(frame, len);
}

void mgt_host_message(const char *title, const char *body) {
  mgt_pause_panel(title, body);
}


static const MgtModuleSpec *bench_module_for(int reg_i) {
  const MgtMinigame *list = mgt_registry();
  if (!list || reg_i < 0 || reg_i >= mgt_registry_count()) return NULL;
  return mgt_bus_module_spec(list[reg_i].id);
}

static void bench_auto_prepare(int reg_i) {
  const MgtModuleSpec *mod = bench_module_for(reg_i);
  if (mod) mgt_bus_auto_wire(&g_game, mod);
}

static void bench_refresh_probe(int reg_i) {
  mgt_bus_probe(&g_game, &g_state, bench_module_for(reg_i), &g_probe);
}

static void draw_harness_menu(int reg_selected) {
  MgtCanvas c;
  char status[96];
  MgtBenchVisual vis;
  const MgtModuleSpec *mod = bench_module_for(reg_selected);

  status_line(status, sizeof status);
  mgt_canvas_init(&c, MGT_CANVAS_W, MGT_CANVAS_H);
  mgt_canvas_clear(&c);
  bench_auto_prepare(reg_selected);
  bench_refresh_probe(reg_selected);
  vis.seat_phase = g_seat_phase;
  vis.show_ejected = g_show_ejected;
  vis.ejected_label = g_ejected_label;
  vis.linked_selected = reg_selected;
  mgt_bus_draw_bench(&c, &g_game, &g_state, mod, g_plugged, &g_probe, &vis);
  mgt_host_present(&c, "MODULE BENCH", status,
                   "Up/Down select   Enter run   Q quit");
  g_menu_dirty = 0;
}

static void bench_animate_seat(int reg_selected) {
  g_run_lock = 1;
  g_seat_phase = 2;
  g_plugged = 1;
  g_menu_dirty = 1;
  draw_harness_menu(reg_selected);
  mgt_sleep_ms(60);
}

static int harness_run_selected(int reg_selected) {
  const MgtMinigame *list = mgt_registry();
  const MgtMinigame *mg;
  const MgtModuleSpec *mod;
  if (!list || reg_selected < 0 || reg_selected >= mgt_registry_count())
    return 0;

  g_show_ejected = 0;
  bench_auto_prepare(reg_selected);
  bench_refresh_probe(reg_selected);
  mod = bench_module_for(reg_selected);
  mg = &list[reg_selected];
  if (!mg->run) return 0;

  bench_animate_seat(reg_selected);
  mgt_host_run_minigame(&g_session, mg->id);
  g_run_lock = 0;
  g_plugged = 0;
  g_seat_phase = 0;
  g_show_ejected = 1;
  if (mod)
    snprintf(g_ejected_label, sizeof g_ejected_label, "%s", mod->cube_label);
  snprintf(g_state.last_banner, sizeof g_state.last_banner,
           "Finished %s — saved.", mg->title);
  return 1;
}

MgtHostResult mgt_host_run_minigame(MgtSession *session, const char *id) {
  const MgtMinigame *mg;
  int i;
  int sim_enter = 0;
  if (!session || !id) return MGT_HOST_ABORT;
  mg = mgt_registry_find(id);
  if (!mg || !mg->run) return MGT_HOST_ABORT;
  for (i = 0; i < mgt_registry_count(); i++) {
    const MgtMinigame *e = &mgt_registry()[i];
    if (e && e->id && !strcmp(e->id, id)) {
      if (i < MGT_PLAY_COUNT) g_state.play_count[i]++;
      break;
    }
  }
  g_state.last_success = -1;
  g_state.last_banner[0] = '\0';

  if (!session->from_game) {
    mgt_sync_from_world(session->state, &g_game);
    session->from_game = 1;
    sim_enter = 1;
  }

  mgt_host_begin_frame(session, mg->title,
                       "ESC - exit minigame (returns to harness / game)");
  mg->run(session);
  if (!g_state.last_banner[0]) {
    if (g_state.last_success < 0)
      snprintf(g_state.last_banner, sizeof g_state.last_banner,
               "You leave %s.", mg->title);
    else
      snprintf(g_state.last_banner, sizeof g_state.last_banner,
               "Finished %s.", mg->title);
  }
  mgt_host_end_frame();

  if (sim_enter) {
    mgt_sync_to_world(session->state, &g_game);
    session->from_game = 0;
    mgt_harness_save(&g_game, &g_state, NULL);
  } else if (!session->from_game)
    mgt_state_save(&g_state, NULL);

  if (session->game_resume) session->game_resume(session->game_resume_ctx);
  return MGT_HOST_OK;
}

MgtHostResult mgt_host_run_index(MgtSession *session, int index) {
  const MgtMinigame *list = mgt_registry();
  if (index < 0 || index >= mgt_registry_count()) return MGT_HOST_ABORT;
  return mgt_host_run_minigame(session, list[index].id);
}

int mgt_host_run_harness(void) {
  int selected = 0;
  int count = mgt_registry_count();
  if (!g_host_open) {
    mgt_terminal_init();
    mgt_input_host_begin();
    g_host_open = 1;
  }
  g_plugged = 0;
  g_seat_phase = 0;
  g_run_lock = 0;
  g_show_ejected = 0;
  g_menu_dirty = 1;
  if (count > 0) bench_auto_prepare(0);
  for (;;) {
    MgtKey k;
    int ch = 0;

    if (g_menu_dirty) draw_harness_menu(selected);
    k = mgt_poll_key(g_menu_dirty ? 200 : 60000, &ch);
    if (k == MGT_KEY_NONE) continue;
    if (k == MGT_KEY_QUIT || (k == MGT_KEY_CHAR && (ch == 'q' || ch == 'Q')))
      break;
    if (k == MGT_KEY_ESC && g_session.from_game) break;
    if (count > 0) {
      if (k == MGT_KEY_UP) {
        selected = (selected - 1 + count) % count;
        g_seat_phase = 0;
        g_plugged = 0;
        g_menu_dirty = 1;
      } else if (k == MGT_KEY_DOWN) {
        selected = (selected + 1) % count;
        g_seat_phase = 0;
        g_plugged = 0;
        g_menu_dirty = 1;
      } else if (k == MGT_KEY_ENTER) {
        harness_run_selected(selected);
        g_menu_dirty = 1;
      }
    }
  }
  mgt_harness_save(&g_game, &g_state, NULL);
  if (!g_session.from_game) {
    mgt_input_host_end();
    mgt_terminal_shutdown();
    g_host_open = 0;
  }
  return 0;
}

MgtHostResult mgt_host_run_from_game(const char *minigame_id,
                                     MgtGameBridge *bridge) {
  if (bridge && bridge->sync_from_game)
    bridge->sync_from_game(&g_state, bridge->game_ctx);
  g_session.from_game = 1;
  g_session.adventure_embedded = 1;
  mgt_terminal_init();
  if (!g_host_open) {
    mgt_input_host_begin();
    g_host_open = 1;
  }
  mgt_host_run_minigame(&g_session, minigame_id);
  if (bridge && bridge->sync_to_game)
    bridge->sync_to_game(&g_state, bridge->game_ctx);
  mgt_input_host_end();
  g_host_open = 0;
  mgt_terminal_shutdown();
  if (bridge && bridge->redraw_game) bridge->redraw_game(bridge->game_ctx);
  g_session.from_game = 0;
  g_session.adventure_embedded = 0;
  return MGT_HOST_OK;
}
