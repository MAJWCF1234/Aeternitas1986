#include "mgt_game_sim.h"

#include <stdio.h>
#include <string.h>

static void sim_inv_push(MgtGameSim *sim, const char *id) {
  size_t n;
  if (!sim || !id || !id[0] || sim->inv_n >= MGT_SIM_INV_MAX) return;
  n = strlen(id);
  if (n >= MGT_SIM_ITEM_LEN) n = MGT_SIM_ITEM_LEN - 1;
  memcpy(sim->inv[sim->inv_n], id, n);
  sim->inv[sim->inv_n][n] = '\0';
  sim->inv_n++;
}

void mgt_game_sim_defaults(MgtGameSim *sim) {
  if (!sim) return;
  memset(sim, 0, sizeof *sim);
  sim->coins = 50;
  sim->craft_proficiency = 1;
  sim->cha = 12;
  sim->wis = 11;
  sim->intl = 10;
  snprintf(sim->room_slug, sizeof sim->room_slug, "tavern_kitchen");
  snprintf(sim->weather, sizeof sim->weather, "clear");
  sim_inv_push(sim, "lockpick");
  sim_inv_push(sim, "tension_wrench");
  sim_inv_push(sim, "parchment");
  sim_inv_push(sim, "bow");
}
