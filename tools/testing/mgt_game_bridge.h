#ifndef MGT_GAME_BRIDGE_H
#define MGT_GAME_BRIDGE_H

#include "mgt_host.h"

#ifdef __cplusplus
extern "C" {
#endif

void aet_minigames_register_sync(
    void (*sync_from)(MgtPersistentState *st, void *game),
    void (*sync_to)(const MgtPersistentState *st, void *game),
    void (*redraw)(void *game), void *game_ctx);

void aet_minigames_register_give(void (*give_item)(const char *id, void *game),
                                 void *game_ctx);
void aet_minigames_give_item(const char *id);

int aet_minigame_takeover(const char *minigame_id);

#ifdef __cplusplus
}
#endif

#endif
