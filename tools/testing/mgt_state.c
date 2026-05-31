#include "mgt_state.h"

#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static char g_save_path[512];

const char *mgt_state_default_path(void) {
  if (g_save_path[0]) return g_save_path;
#if defined(_WIN32)
  {
    char mod[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, mod, sizeof mod);
    if (n > 0 && n < sizeof mod) {
      char *slash = strrchr(mod, '\\');
      if (slash) {
        snprintf(g_save_path, sizeof g_save_path, "%.*s\\harness_save.mgt",
                 (int)(slash - mod + 1), mod);
        return g_save_path;
      }
    }
  }
#endif
  snprintf(g_save_path, sizeof g_save_path, "harness_save.mgt");
  return g_save_path;
}

void mgt_state_defaults(MgtPersistentState *st) {
  if (!st) return;
  memset(st, 0, sizeof *st);
  st->magic = MGT_SAVE_MAGIC;
  st->version = MGT_SAVE_VERSION;
  st->rng_seed = 1;
  st->money = 100;
  st->skill_engineering = 20;
  st->skill_survival = 20;
  st->skill_cooking = 10;
  st->has_basic_lockpick = 1;
  st->has_tension_wrench = 1;
  st->test_lock_difficulty = 1;
  st->piano_owned = 7;
  snprintf(st->piano_selected_id, sizeof st->piano_selected_id,
           "barkeep_lesson");
  st->rod_index = 0;
  st->bait_index = 0;
  st->fishing_level = 1;
  st->farm_seeds[0] = 5;
  st->farm_seeds[1] = 2;
  st->farm_seeds[2] = 0;
  st->farm_fertilizer = 0;
  snprintf(st->game_weather, sizeof st->game_weather, "clear");
  st->paper = 5;
  st->ink_uses = 50;
  st->envelopes = 2;
  snprintf(st->hunt_target, sizeof st->hunt_target, "deer");
  snprintf(st->hunt_area, sizeof st->hunt_area, "Woodland Trail");
  st->has_hunting_bow = 1;
  st->adventure_turn = 0;
  st->hunt_cooldown_until_turn = 0;
  snprintf(st->last_banner, sizeof st->last_banner, "New harness profile.");
}

void mgt_harness_reset(MgtGameSim *game, MgtPersistentState *profile) {
  mgt_game_sim_defaults(game);
  mgt_state_defaults(profile);
  if (profile) profile->rng_seed = 1;
}

int mgt_harness_load(MgtGameSim *game, MgtPersistentState *profile,
                     const char *path) {
  FILE *f;
  MgtHarnessSave save;
  MgtPersistentState legacy;
  size_t n;
  if (!game || !profile) return 0;
  if (!path || !path[0]) path = mgt_state_default_path();
  strncpy(g_save_path, path, sizeof g_save_path - 1);
  g_save_path[sizeof g_save_path - 1] = '\0';

  f = fopen(path, "rb");
  if (!f) {
    mgt_harness_reset(game, profile);
    return 0;
  }

  n = fread(&save, 1, sizeof save, f);
  if (n == sizeof save && save.magic == MGT_SAVE_MAGIC &&
      save.version == MGT_SAVE_VERSION) {
    fclose(f);
    *game = save.game;
    *profile = save.profile;
    if (profile->rng_seed == 0) profile->rng_seed = 1;
    return 1;
  }

  rewind(f);
  n = fread(&legacy, 1, sizeof legacy, f);
  fclose(f);
  if (n == sizeof legacy && legacy.magic == MGT_SAVE_MAGIC &&
      legacy.version == MGT_SAVE_VERSION_LEGACY) {
    mgt_game_sim_defaults(game);
    game->coins = legacy.money > 0 ? legacy.money : 50;
    *profile = legacy;
    profile->version = MGT_SAVE_VERSION;
    return 1;
  }

  mgt_harness_reset(game, profile);
  return 0;
}

int mgt_harness_save(const MgtGameSim *game, const MgtPersistentState *profile,
                     const char *path) {
  FILE *f;
  MgtHarnessSave save;
  if (!game || !profile) return 0;
  if (!path || !path[0]) path = mgt_state_default_path();
  memset(&save, 0, sizeof save);
  save.magic = MGT_SAVE_MAGIC;
  save.version = MGT_SAVE_VERSION;
  save.game = *game;
  save.profile = *profile;
  save.profile.magic = MGT_SAVE_MAGIC;
  save.profile.version = MGT_SAVE_VERSION;
  f = fopen(path, "wb");
  if (!f) return 0;
  if (fwrite(&save, 1, sizeof save, f) != sizeof save) {
    fclose(f);
    return 0;
  }
  fclose(f);
  return 1;
}

int mgt_state_load(MgtPersistentState *st, const char *path) {
  MgtGameSim game;
  if (!st) return 0;
  return mgt_harness_load(&game, st, path);
}

int mgt_state_save(const MgtPersistentState *st, const char *path) {
  MgtGameSim game;
  if (!st) return 0;
  mgt_game_sim_defaults(&game);
  game.coins = st->money;
  return mgt_harness_save(&game, st, path);
}

static void chomp_line_buf(char *s) {
  size_t n;
  if (!s) return;
  n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

void mgt_profile_fresh_adventure(MgtPersistentState *st, unsigned rng_seed) {
  if (!st) return;
  memset(st, 0, sizeof *st);
  st->magic = MGT_SAVE_MAGIC;
  st->version = MGT_SAVE_VERSION;
  st->rng_seed = rng_seed ? rng_seed : 1u;
  snprintf(st->hunt_target, sizeof st->hunt_target, "rabbit");
  snprintf(st->hunt_area, sizeof st->hunt_area, "Open Country");
  snprintf(st->game_weather, sizeof st->game_weather, "clear");
}

int mgt_profile_validate(const MgtPersistentState *st) {
  if (!st) return 0;
  if (st->magic != MGT_SAVE_MAGIC) return 0;
  if (st->version != MGT_SAVE_VERSION) return 0;
  if (st->rng_seed == 0) return 0;
  return 1;
}

static int b64_encode(const unsigned char *in, size_t in_len, char *out,
                      size_t out_cap) {
  static const char tbl[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t i = 0, o = 0;
  if (!in || !out || out_cap < 4) return 0;
  for (i = 0; i < in_len; i += 3) {
    int rem = (int)(in_len - i);
    unsigned a = in[i];
    unsigned b = rem > 1 ? in[i + 1] : 0;
    unsigned c = rem > 2 ? in[i + 2] : 0;
    unsigned triple = (a << 16) | (b << 8) | c;
    if (o + 4 >= out_cap) return 0;
    out[o++] = tbl[(triple >> 18) & 63];
    out[o++] = tbl[(triple >> 12) & 63];
    out[o++] = rem > 1 ? tbl[(triple >> 6) & 63] : '=';
    out[o++] = rem > 2 ? tbl[triple & 63] : '=';
  }
  out[o] = '\0';
  return 1;
}

static int b64_val(int c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

static int b64_decode(const char *in, unsigned char *out, size_t out_cap,
                      size_t *out_len) {
  size_t i = 0, o = 0, len;
  int v[4];
  if (!in || !out || !out_len) return 0;
  len = strlen(in);
  while (i < len) {
    int j, pad = 0;
    for (j = 0; j < 4; j++) {
      while (i < len && (in[i] == '\r' || in[i] == '\n' || in[i] == ' ')) i++;
      if (i >= len) break;
      if (in[i] == '=') {
        v[j] = 0;
        pad++;
        i++;
      } else {
        v[j] = b64_val((unsigned char)in[i]);
        if (v[j] < 0) return 0;
        i++;
      }
    }
    if (j < 4) break;
    if (o + 3 > out_cap) return 0;
    out[o++] = (unsigned char)((v[0] << 2) | (v[1] >> 4));
    if (pad < 2) out[o++] = (unsigned char)(((v[1] & 15) << 4) | (v[2] >> 2));
    if (pad < 1) out[o++] = (unsigned char)(((v[2] & 3) << 6) | v[3]);
  }
  *out_len = o;
  return 1;
}

int mgt_profile_write_embedded(FILE *fp, const MgtPersistentState *st) {
  char b64[16384];
  MgtPersistentState copy;
  if (!fp || !st) return 0;
  copy = *st;
  copy.magic = MGT_SAVE_MAGIC;
  copy.version = MGT_SAVE_VERSION;
  if (!b64_encode((const unsigned char *)&copy, sizeof copy, b64, sizeof b64))
    return 0;
  if (fprintf(fp, "MGT\n%d\n%s\n", MGT_SAVE_VERSION, b64) < 0) return 0;
  return 1;
}

int mgt_profile_read_embedded(FILE *fp, MgtPersistentState *st) {
  char buf[16384];
  char b64[16384];
  unsigned char raw[sizeof(MgtPersistentState) + 8];
  size_t raw_len = 0;
  int ver;
  if (!fp || !st) return 0;
  if (!fgets(buf, sizeof buf, fp)) return 0;
  chomp_line_buf(buf);
  ver = atoi(buf);
  if (ver != MGT_SAVE_VERSION) return 0;
  if (!fgets(b64, sizeof b64, fp)) return 0;
  chomp_line_buf(b64);
  if (!b64_decode(b64, raw, sizeof raw, &raw_len)) return 0;
  if (raw_len != sizeof(MgtPersistentState)) return 0;
  memcpy(st, raw, sizeof *st);
  return mgt_profile_validate(st);
}
