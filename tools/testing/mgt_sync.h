#ifndef MGT_SYNC_H
#define MGT_SYNC_H

#include "mgt_game_sim.h"
#include "mgt_state.h"

void mgt_sync_from_world(MgtPersistentState *st, const MgtGameSim *sim);
void mgt_sync_to_world(MgtPersistentState *st, MgtGameSim *sim);

void mgt_sim_give_item(MgtGameSim *sim, const char *id);
int mgt_sim_has_item(const MgtGameSim *sim, const char *id);

#endif
