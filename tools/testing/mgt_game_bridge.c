#include "mgt_game_bridge.h"

#include "mgt_platform.h"
#include "mgt_state.h"

#include <string.h>
#include <time.h>

static void stub_sync_from(MgtPersistentState *st, void *game) {
  (void)game;
  (void)st;
}

static void stub_sync_to(const MgtPersistentState *st, void *game) {
  (void)st;
  (void)game;
}

static void stub_give_item(const char *id, void *game) {
  (void)id;
  (void)game;
}

static void (*s_sync_from)(MgtPersistentState *st, void *game) = stub_sync_from;
static void (*s_sync_to)(const MgtPersistentState *st, void *game) = stub_sync_to;
static void (*s_redraw)(void *game) = NULL;
static void (*s_give_item)(const char *id, void *game) = stub_give_item;
static void *s_game_ctx = NULL;

void aet_minigames_register_sync(
    void (*sync_from)(MgtPersistentState *st, void *game),
    void (*sync_to)(const MgtPersistentState *st, void *game),
    void (*redraw)(void *game), void *game_ctx) {
  s_sync_from = sync_from ? sync_from : stub_sync_from;
  s_sync_to = sync_to ? sync_to : stub_sync_to;
  s_redraw = redraw;
  s_game_ctx = game_ctx;
}

void aet_minigames_register_give(void (*give_item)(const char *id, void *game),
                                 void *game_ctx) {
  s_give_item = give_item ? give_item : stub_give_item;
  if (game_ctx) s_game_ctx = game_ctx;
}

void aet_minigames_give_item(const char *id) {
  if (id && id[0]) s_give_item(id, s_game_ctx);
}

int aet_minigame_takeover(const char *minigame_id) {
  MgtGameBridge bridge;
  static int inited;
  if (!inited) {
    if (s_sync_from != stub_sync_from) {
      MgtSession *sess = mgt_host_session();
      if (sess) sess->state = mgt_host_state();
    } else
      mgt_host_init(0);
    inited = 1;
  }
  memset(&bridge, 0, sizeof bridge);
  bridge.sync_from_game = s_sync_from;
  bridge.sync_to_game = s_sync_to;
  bridge.redraw_game = s_redraw;
  bridge.game_ctx = s_game_ctx;
  return (int)mgt_host_run_from_game(minigame_id, &bridge);
}
