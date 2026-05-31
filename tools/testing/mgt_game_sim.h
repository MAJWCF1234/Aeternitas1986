#ifndef MGT_GAME_SIM_H
#define MGT_GAME_SIM_H

#define MGT_SIM_INV_MAX 48
#define MGT_SIM_ITEM_LEN 32
#define MGT_SIM_ROOM_LEN 48

typedef struct MgtGameSim {
  int coins;
  int craft_proficiency;
  int cha;
  int wis;
  int intl;
  int inv_n;
  char inv[MGT_SIM_INV_MAX][MGT_SIM_ITEM_LEN];
  char room_slug[MGT_SIM_ROOM_LEN];
  char weather[16];
  int adventure_turn;
} MgtGameSim;

void mgt_game_sim_defaults(MgtGameSim *sim);

#endif
