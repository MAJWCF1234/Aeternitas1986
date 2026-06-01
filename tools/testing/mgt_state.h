#ifndef MGT_STATE_H
#define MGT_STATE_H

#include "mgt_game_sim.h"

#include <stddef.h>
#include <stdio.h>

#define MGT_SAVE_MAGIC 0x3147544Du
#define MGT_SAVE_VERSION 6
#define MGT_SAVE_VERSION_LEGACY 3
#define MGT_FISH_INV_MAX 32
#define MGT_FARM_W 10
#define MGT_FARM_H 8
#define MGT_PLAY_COUNT 9

typedef struct MgtFishCatch {
  char id[24];
  char name[32];
  char ascii[16];
  int value;
  int weight_x10;
  int difficulty;
} MgtFishCatch;

typedef struct MgtFarmPlot {
  unsigned char plowed;
  unsigned char watered;
  unsigned char growth;
  char seed;
} MgtFarmPlot;

typedef struct MgtPersistentState {
  unsigned magic;
  unsigned version;
  unsigned rng_seed;

  int money;
  int skill_engineering;
  int skill_survival;
  int skill_cooking;

  int has_rusty_pick;
  int has_basic_lockpick;
  int has_fine_pick;
  int has_tension_wrench;
  int has_skeleton_key;
  int test_lock_difficulty;

  unsigned char piano_owned;
  char piano_selected_id[32];

  int rod_index;
  int bait_index;
  int fishing_xp;
  int fishing_level;
  int fish_inv_n;
  MgtFishCatch fish_inv[MGT_FISH_INV_MAX];

  MgtFarmPlot farm[MGT_FARM_H][MGT_FARM_W];
  int farm_cx;
  int farm_cy;
  int farm_seed_i;
  int farm_seeds[3];
  int farm_fertilizer;
  char game_weather[16];
  int has_scarecrow;
  int farm_barn_n;
  unsigned char farm_barn[32];

  int cooking_shift_earnings;

  int paper;
  int ink_uses;
  int envelopes;
  int leather;
  int empty_books;

  char hunt_target[16];
  char hunt_area[32];
  int has_hunting_bow;
  int adventure_turn;
  int hunt_cooldown_until_turn;

  unsigned play_count[MGT_PLAY_COUNT];
  char last_banner[128];
  int last_success;

  int lock_exit_noise;
  int lock_exit_misses;
  int lock_pick_broken;
  char lock_noise_band[8];
  char lock_tool_id[24];
  char lock_target_name[48];
  char lock_target_dir[8];
} MgtPersistentState;

typedef struct MgtHarnessSave {
  unsigned magic;
  unsigned version;
  MgtGameSim game;
  MgtPersistentState profile;
} MgtHarnessSave;

void mgt_state_defaults(MgtPersistentState *st);
int mgt_state_load(MgtPersistentState *st, const char *path);
int mgt_state_save(const MgtPersistentState *st, const char *path);
const char *mgt_state_default_path(void);

int mgt_harness_load(MgtGameSim *game, MgtPersistentState *profile, const char *path);
int mgt_harness_save(const MgtGameSim *game, const MgtPersistentState *profile,
                     const char *path);
void mgt_harness_reset(MgtGameSim *game, MgtPersistentState *profile);

void mgt_profile_fresh_adventure(MgtPersistentState *st, unsigned rng_seed);
int mgt_profile_validate(const MgtPersistentState *st);

int mgt_profile_write_embedded(FILE *fp, const MgtPersistentState *st);
int mgt_profile_read_embedded(FILE *fp, MgtPersistentState *st);

#endif
