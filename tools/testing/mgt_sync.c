#include "mgt_sync.h"

#include <stdio.h>
#include <string.h>

static int str_ieq(const char *a, const char *b) {
  if (!a || !b) return 0;
  while (*a && *b) {
    char ca = *a;
    char cb = *b;
    if (ca >= 'A' && ca <= 'Z') ca = (char)(ca + ('a' - 'A'));
    if (cb >= 'A' && cb <= 'Z') cb = (char)(cb + ('a' - 'A'));
    if (ca != cb) return 0;
    a++;
    b++;
  }
  return *a == *b;
}

int mgt_sim_has_item(const MgtGameSim *sim, const char *id) {
  int i;
  if (!sim || !id || !id[0]) return 0;
  for (i = 0; i < sim->inv_n; i++)
    if (str_ieq(sim->inv[i], id)) return 1;
  return 0;
}

void mgt_sim_give_item(MgtGameSim *sim, const char *id) {
  size_t n;
  if (!sim || !id || !id[0] || sim->inv_n >= MGT_SIM_INV_MAX) return;
  if (mgt_sim_has_item(sim, id)) return;
  n = strlen(id);
  if (n >= MGT_SIM_ITEM_LEN) n = MGT_SIM_ITEM_LEN - 1;
  memcpy(sim->inv[sim->inv_n], id, n);
  sim->inv[sim->inv_n][n] = '\0';
  sim->inv_n++;
}

static const char *farm_crop_id(char seed) {
  if (seed == 't') return "turnip";
  if (seed == 'c') return "carrots";
  if (seed == 'p') return "pumpkin";
  return NULL;
}

static const char *hunt_area_label(const char *slug) {
  if (!slug || !slug[0]) return "Open Country";
  if (!strcmp(slug, "deep_forest")) return "Deep Forest";
  if (!strcmp(slug, "meadow")) return "High Meadow";
  if (strstr(slug, "forest") != NULL) return "Woodland Trail";
  return "Wild Margin";
}

static int sim_has_bow(const MgtGameSim *sim) {
  if (!sim) return 0;
  return mgt_sim_has_item(sim, "bow") || mgt_sim_has_item(sim, "shortbow") ||
         mgt_sim_has_item(sim, "hunting_bow");
}

void mgt_sync_from_world(MgtPersistentState *st, const MgtGameSim *sim) {
  const char *slug;
  int i;
  if (!st || !sim) return;

  st->money = sim->coins;
  st->skill_engineering = sim->intl;
  if (st->skill_engineering < 5) st->skill_engineering = 5;
  if (st->skill_engineering > 100) st->skill_engineering = 100;
  st->skill_survival = sim->wis;
  if (st->skill_survival < 5) st->skill_survival = 5;
  if (st->skill_survival > 100) st->skill_survival = 100;
  st->skill_cooking = sim->cha;
  if (st->skill_cooking < 5) st->skill_cooking = 5;
  if (st->skill_cooking > 100) st->skill_cooking = 100;

  st->has_rusty_pick = mgt_sim_has_item(sim, "rusty_pick");
  st->has_basic_lockpick = mgt_sim_has_item(sim, "lockpick");
  st->has_fine_pick = mgt_sim_has_item(sim, "fine_lockpick");
  st->has_tension_wrench =
      mgt_sim_has_item(sim, "tension_wrench") || mgt_sim_has_item(sim, "fine_lockpick");
  st->has_skeleton_key = mgt_sim_has_item(sim, "skeleton_key");

  st->piano_owned = 0;
  for (i = 0; i < sim->inv_n; i++) {
    if (strstr(sim->inv[i], "sheet_barkeep") != NULL ||
        strstr(sim->inv[i], "barkeep_lesson") != NULL)
      st->piano_owned |= 1;
    if (strstr(sim->inv[i], "sheet_lantern") != NULL ||
        strstr(sim->inv[i], "lantern") != NULL)
      st->piano_owned |= 2;
    if (strstr(sim->inv[i], "sheet_last_call") != NULL ||
        strstr(sim->inv[i], "last_call") != NULL)
      st->piano_owned |= 4;
    if (strstr(sim->inv[i], "music_sheet") != NULL) st->piano_owned |= 7;
  }
  if (st->piano_owned == 0) st->piano_owned = 1;

  st->paper = 0;
  for (i = 0; i < sim->inv_n; i++) {
    if (str_ieq(sim->inv[i], "parchment") || str_ieq(sim->inv[i], "paper")) st->paper++;
  }
  if (st->paper < 1 && sim->inv_n > 0) st->paper = 1;
  st->envelopes = mgt_sim_has_item(sim, "envelope") ? 1 : 0;
  st->leather = mgt_sim_has_item(sim, "leather_binding") ? 1 : 0;
  st->empty_books = mgt_sim_has_item(sim, "empty_book") ? 1 : 0;

  slug = sim->room_slug[0] ? sim->room_slug : "village_square";
  st->test_lock_difficulty = 1;
  if (!strcmp(slug, "hidden_cellar") || !strcmp(slug, "castle"))
    st->test_lock_difficulty = 2;
  if (!strcmp(slug, "east_of_house")) {
    snprintf(st->lock_target_name, sizeof st->lock_target_name, "Shed Door");
    snprintf(st->lock_target_dir, sizeof st->lock_target_dir, "east");
  } else if (!strcmp(slug, "hidden_cellar")) {
    snprintf(st->lock_target_name, sizeof st->lock_target_name, "Cellar Door");
    snprintf(st->lock_target_dir, sizeof st->lock_target_dir, "down");
  } else {
    st->lock_target_name[0] = '\0';
    st->lock_target_dir[0] = '\0';
  }
  if (!strcmp(slug, "deep_forest"))
    snprintf(st->hunt_target, sizeof st->hunt_target, "wolf");
  else if (strstr(slug, "forest") != NULL)
    snprintf(st->hunt_target, sizeof st->hunt_target, "deer");
  else
    snprintf(st->hunt_target, sizeof st->hunt_target, "rabbit");
  snprintf(st->hunt_area, sizeof st->hunt_area, "%s", hunt_area_label(slug));
  st->has_hunting_bow = sim_has_bow(sim);
  st->adventure_turn = sim->adventure_turn;

  snprintf(st->game_weather, sizeof st->game_weather, "%s",
           sim->weather[0] ? sim->weather : "clear");
  st->has_scarecrow = mgt_sim_has_item(sim, "scarecrow") ? 1 : 0;
}

void mgt_sync_to_world(MgtPersistentState *st, MgtGameSim *sim) {
  int i, fish_n = 0;
  if (!st || !sim) return;

  sim->coins = st->money;
  if (st->skill_engineering > sim->craft_proficiency)
    sim->craft_proficiency = st->skill_engineering;
  if (sim->craft_proficiency > 100) sim->craft_proficiency = 100;

  for (i = 0; i < st->farm_barn_n && i < 32; i++) {
    const char *crop = farm_crop_id((char)st->farm_barn[i]);
    if (crop) mgt_sim_give_item(sim, crop);
  }
  st->farm_barn_n = 0;

  for (i = 0; i < st->fish_inv_n && i < MGT_FISH_INV_MAX; i++) {
    const char *fid = st->fish_inv[i].id;
    if (!fid[0]) continue;
    if (!strcmp(fid, "old_boot") || !strcmp(fid, "rusty_can")) continue;
    mgt_sim_give_item(sim, "fish");
    fish_n++;
  }
  st->fish_inv_n = 0;

  if (st->cooking_shift_earnings > 0)
    snprintf(st->last_banner, sizeof st->last_banner,
             "Shift pay $%d in your purse.", st->cooking_shift_earnings);
  else if (fish_n > 0)
    snprintf(st->last_banner, sizeof st->last_banner,
             "%d fish added to your pack.", fish_n);
  st->cooking_shift_earnings = 0;
}
