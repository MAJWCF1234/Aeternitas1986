#ifndef MGT_READ_H
#define MGT_READ_H

#include "mgt_game_sim.h"

typedef enum MgtReadKind {
  MGT_READ_NOTE = 0,
  MGT_READ_BOOK,
  MGT_READ_SIGN,
  MGT_READ_SCROLL,
  MGT_READ_LEDGER,
  MGT_READ_LETTER,
  MGT_READ_MAP,
  MGT_READ_MUSIC
} MgtReadKind;

#define MGT_READ_TITLE_MAX 96
#define MGT_READ_SOURCE_MAX 48
#define MGT_READ_BODY_MAX 12000

typedef struct MgtReadDocument {
  int valid;
  MgtReadKind kind;
  char title[MGT_READ_TITLE_MAX];
  char source_id[MGT_READ_SOURCE_MAX];
  char body[MGT_READ_BODY_MAX];
} MgtReadDocument;

const char *mgt_read_kind_label(MgtReadKind kind);

void mgt_read_set_document(const MgtReadDocument *doc);
const MgtReadDocument *mgt_read_current_document(void);
void mgt_read_clear_document(void);

int mgt_read_take_document(MgtReadDocument *out);

MgtReadKind mgt_read_classify_source_id(const char *source_id);

typedef int (*MgtReadResolveFn)(const char *source_id, MgtReadDocument *doc);
void mgt_read_register_resolver(MgtReadResolveFn fn);

int mgt_read_resolve_item(const char *source_id, MgtReadDocument *doc);
int mgt_read_harness_fill_from_sim(const MgtGameSim *sim);

#endif
