#ifndef MGT_H
#define MGT_H

#include "mgt_state.h"

typedef struct MgtCanvas MgtCanvas;

typedef struct MgtSession {
  MgtPersistentState *state;
  int from_game;
  int adventure_embedded;
  void (*game_resume)(void *ctx);
  void *game_resume_ctx;
} MgtSession;

typedef struct MgtMinigame {
  const char *id;
  const char *title;
  const char *web_ref;
  const char *status_note;
  int (*run)(MgtSession *session);
} MgtMinigame;

const MgtMinigame *mgt_registry(void);
int mgt_registry_count(void);
const MgtMinigame *mgt_registry_find(const char *id);

#endif
