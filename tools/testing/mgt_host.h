#ifndef MGT_HOST_H
#define MGT_HOST_H

#include "mgt.h"
#include "mgt_game_sim.h"

typedef enum MgtHostResult {
  MGT_HOST_OK = 0,
  MGT_HOST_QUIT = 1,
  MGT_HOST_ABORT = -1
} MgtHostResult;

void mgt_host_init(unsigned rng_seed_if_new);
void mgt_host_shutdown(void);
int mgt_host_run_harness(void);
MgtHostResult mgt_host_run_minigame(MgtSession *session, const char *id);
MgtHostResult mgt_host_run_index(MgtSession *session, int index);
void mgt_host_present(const MgtCanvas *canvas, const char *panel_title,
                      const char *status_line, const char *controls);
void mgt_host_message(const char *title, const char *body);
void mgt_host_begin_frame(MgtSession *session, const char *title,
                          const char *controls);
void mgt_host_end_frame(void);
MgtPersistentState *mgt_host_state(void);
MgtGameSim *mgt_host_game_sim(void);
MgtSession *mgt_host_session(void);

typedef struct MgtGameBridge {
  void (*sync_from_game)(MgtPersistentState *st, void *game);
  void (*sync_to_game)(const MgtPersistentState *st, void *game);
  void (*redraw_game)(void *game);
  void *game_ctx;
} MgtGameBridge;

MgtHostResult mgt_host_run_from_game(const char *minigame_id,
                                     MgtGameBridge *bridge);

#endif
