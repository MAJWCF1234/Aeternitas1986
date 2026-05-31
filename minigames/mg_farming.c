#include "mgt.h"
#include "mgt_game_bridge.h"
#include "mgt_platform.h"

#include <stdio.h>
#include <string.h>

enum {
  FARM_W = MGT_FARM_W,
  FARM_H = MGT_FARM_H,
  FARM_OFF_X = 5,
  FARM_OFF_Y = 6,
  FARM_BOX_W = FARM_W * 6 + 4,
  FARM_BOX_H = FARM_H * 2 + 2
};

typedef struct {
  char id;
  const char *name;
  int cost;
  int sell;
  const char *stages[4];
} SeedDef;

static const SeedDef k_seeds[] = {
    {'t', "Turnip", 5, 15, {".", "t", "T", "(@)"}},
    {'c', "Carrot", 12, 35, {".", "i", "Y", "V"}},
    {'p', "Pumpkin", 30, 100, {".", "o", "O", "(_)"}}};

enum {
  FARM_SCR_FIELD = 0,
  FARM_SCR_SHOP = 1,
  FARM_SCR_INV = 2
};

typedef struct {
  int plowed;
  int watered;
  char seed;
  int growth;
  int has_crow;
} FarmCell;

typedef struct {
  MgtPersistentState *st;
  MgtSession *session;
  int screen;
  int cx, cy;
  int seed_sel;
  int weather;
  int has_scarecrow;
  int from_game;
  char msg[96];
  FarmCell grid[FARM_H][FARM_W];
  int tick;
  int crop_inv[32];
  int crop_n;
} FarmCtx;

static void farm_put(MgtCanvas *c, int x, int y, char ch) {
  if (!c || x < 0 || x >= c->w || y < 0 || y >= c->h) return;
  c->cells[y * c->w + x] = ch;
}

static int seed_idx(char id) {
  int i;
  for (i = 0; i < 3; i++)
    if (k_seeds[i].id == id) return i;
  return -1;
}

static int farm_is_rain(const FarmCtx *f) { return f->weather != 0; }

static void farm_weather_from_state(FarmCtx *f) {
  if (!f->st || !f->st->game_weather[0]) return;
  f->weather =
      (!strcmp(f->st->game_weather, "rain") || !strcmp(f->st->game_weather, "storm"));
}

static void farm_weather_msg(FarmCtx *f, int was_rain) {
  if (farm_is_rain(f) && !was_rain)
    snprintf(f->msg, sizeof f->msg, "Rain started.");
  else if (!farm_is_rain(f) && was_rain)
    snprintf(f->msg, sizeof f->msg, "Rain stopped.");
}

static void farm_cell_visual(const FarmCell *c, char out[5]) {
  int si, stage;
  if (c->has_crow) {
    memcpy(out, "^v^", 4);
    out[3] = '\0';
    return;
  }
  if (!c->plowed) {
    memcpy(out, ".  ", 4);
    out[3] = '\0';
    return;
  }
  if (!c->seed) {
    if (c->watered)
      memcpy(out, "~~~~", 5);
    else
      memcpy(out, "____", 5);
    out[4] = '\0';
    return;
  }
  si = seed_idx(c->seed);
  if (si < 0) {
    memcpy(out, " ?? ", 5);
    return;
  }
  stage = c->growth / 34;
  if (stage > 3) stage = 3;
  if (strlen(k_seeds[si].stages[stage]) > 1) {
    snprintf(out, 5, "%-4s", k_seeds[si].stages[stage]);
    return;
  }
  if (c->watered) {
    out[0] = '~';
    out[1] = k_seeds[si].stages[stage][0];
    out[2] = k_seeds[si].stages[stage][0];
    out[3] = '~';
    out[4] = '\0';
  } else {
    out[0] = ' ';
    out[1] = k_seeds[si].stages[stage][0];
    out[2] = k_seeds[si].stages[stage][0];
    out[3] = ' ';
    out[4] = '\0';
  }
}

static int farm_manhattan(int x0, int y0, int x1, int y1) {
  int dx = x0 - x1;
  int dy = y0 - y1;
  if (dx < 0) dx = -dx;
  if (dy < 0) dy = -dy;
  return dx + dy;
}

static void sync_from_state(FarmCtx *f) {
  MgtPersistentState *st = f->st;
  int y, x, i;
  if (!st) return;
  for (y = 0; y < FARM_H; y++)
    for (x = 0; x < FARM_W; x++) {
      MgtFarmPlot *p = &st->farm[y][x];
      f->grid[y][x].plowed = p->plowed;
      f->grid[y][x].watered = p->watered;
      f->grid[y][x].seed = p->seed;
      f->grid[y][x].growth = (int)p->growth;
      if (f->grid[y][x].growth > 100) f->grid[y][x].growth = 100;
      f->grid[y][x].has_crow = 0;
    }
  f->cx = st->farm_cx;
  f->cy = st->farm_cy;
  if (f->cx < 0 || f->cx >= FARM_W) f->cx = 0;
  if (f->cy < 0 || f->cy >= FARM_H) f->cy = 0;
  f->seed_sel = st->farm_seed_i;
  if (f->seed_sel < 0 || f->seed_sel > 2) f->seed_sel = 0;
  f->has_scarecrow = st->has_scarecrow;
  f->crop_n = st->farm_barn_n;
  if (f->crop_n > 32) f->crop_n = 32;
  for (i = 0; i < f->crop_n; i++) f->crop_inv[i] = (int)st->farm_barn[i];
  farm_weather_from_state(f);
}

static void sync_to_state(FarmCtx *f) {
  MgtPersistentState *st = f->st;
  int y, x, i;
  if (!st) return;
  for (y = 0; y < FARM_H; y++)
    for (x = 0; x < FARM_W; x++) {
      MgtFarmPlot *p = &st->farm[y][x];
      p->plowed = (unsigned char)f->grid[y][x].plowed;
      p->watered = (unsigned char)f->grid[y][x].watered;
      p->seed = f->grid[y][x].seed;
      p->growth = (unsigned char)f->grid[y][x].growth;
    }
  st->farm_cx = f->cx;
  st->farm_cy = f->cy;
  st->farm_seed_i = f->seed_sel;
  st->has_scarecrow = f->has_scarecrow;
  st->farm_barn_n = f->crop_n;
  for (i = 0; i < f->crop_n && i < 32; i++) st->farm_barn[i] = (unsigned char)f->crop_inv[i];
}

static const char *farm_crop_id(char seed) {
  if (seed == 't') return "turnip";
  if (seed == 'c') return "carrots";
  if (seed == 'p') return "pumpkin";
  return NULL;
}

static void farm_barn_add(FarmCtx *f, char seed_id) {
  if (f->crop_n < 32) f->crop_inv[f->crop_n++] = (int)seed_id;
}

static void farm_act(FarmCtx *f) {
  FarmCell *c = &f->grid[f->cy][f->cx];
  MgtPersistentState *st = f->st;
  char sid = k_seeds[f->seed_sel].id;

  if (!c->plowed) {
    c->plowed = 1;
    snprintf(f->msg, sizeof f->msg, "Plowed.");
    return;
  }
  if (!c->seed) {
    int have = 0;
    if (st) {
      if (sid == 't') have = st->farm_seeds[0];
      else if (sid == 'c') have = st->farm_seeds[1];
      else if (sid == 'p') have = st->farm_seeds[2];
    } else
      have = 99;
    if (have <= 0) {
      snprintf(f->msg, sizeof f->msg, "No seeds! Visit shop (S).");
      return;
    }
    c->seed = sid;
    c->growth = 0;
    c->watered = 0;
    if (st) {
      if (sid == 't') st->farm_seeds[0]--;
      else if (sid == 'c') st->farm_seeds[1]--;
      else if (sid == 'p') st->farm_seeds[2]--;
    }
    snprintf(f->msg, sizeof f->msg, "Planted %s.", k_seeds[f->seed_sel].name);
    return;
  }
  if (c->growth >= 100) {
    int si = seed_idx(c->seed);
    if (si >= 0) {
      if (f->session && f->session->from_game) {
        const char *crop = farm_crop_id(c->seed);
        if (crop) {
          aet_minigames_give_item(crop);
          snprintf(f->msg, sizeof f->msg, "Harvested %s -> pack (+$%d).",
                   k_seeds[si].name, k_seeds[si].sell);
        } else
          snprintf(f->msg, sizeof f->msg, "Harvested! (+$%d)", k_seeds[si].sell);
      } else {
        farm_barn_add(f, c->seed);
        snprintf(f->msg, sizeof f->msg, "Harvested! (+$%d)", k_seeds[si].sell);
      }
      if (st) st->money += k_seeds[si].sell;
    }
    c->seed = 0;
    c->growth = 0;
    c->watered = 0;
    c->has_crow = 0;
    return;
  }
  if (!c->watered) {
    c->watered = 1;
    snprintf(f->msg, sizeof f->msg, "Watered.");
    return;
  }
  snprintf(f->msg, sizeof f->msg, "Growing... fertilize (F) or wait.");
}

static void farm_draw_rain(MgtCanvas *c, const FarmCtx *f) {
  int i;
  if (!farm_is_rain(f)) return;
  for (i = 0; i < 24; i++) {
    int x = mgt_rand_range(0, c->w - 1);
    int y = mgt_rand_range(3, c->h - 4);
    farm_put(c, x, y, '/');
  }
}

static void farm_draw_field(FarmCtx *f, MgtCanvas *c) {
  int y, x;
  char vis[8];
  char line[96];
  MgtPersistentState *st = f->st;
  int seed_have = 0;

  mgt_canvas_box(c, FARM_OFF_X - 2, FARM_OFF_Y - 1, FARM_BOX_W, FARM_BOX_H, NULL);

  if (f->has_scarecrow)
    mgt_canvas_write(c, FARM_OFF_X + FARM_W * 6 + 2, FARM_OFF_Y + 2, " (+) ");

  for (y = 0; y < FARM_H; y++) {
    for (x = 0; x < FARM_W; x++) {
      int sx = FARM_OFF_X + x * 6;
      int sy = FARM_OFF_Y + y * 2;
      farm_cell_visual(&f->grid[y][x], vis);
      mgt_canvas_write(c, sx, sy, vis);
      if (f->cx == x && f->cy == y) {
        farm_put(c, sx - 1, sy, '>');
        farm_put(c, sx + 4, sy, '<');
      }
    }
  }

  if (st) {
    if (k_seeds[f->seed_sel].id == 't') seed_have = st->farm_seeds[0];
    else if (k_seeds[f->seed_sel].id == 'c') seed_have = st->farm_seeds[1];
    else seed_have = st->farm_seeds[2];
  } else
    seed_have = 99;

  snprintf(line, sizeof line, "$%-4d SEED:%s(%d) %s",
           st ? st->money : 0, k_seeds[f->seed_sel].name, seed_have,
           farm_is_rain(f) ? "RAIN" : "SUN");
  mgt_canvas_write(c, 2, 1, line);
  snprintf(line, sizeof line,
           "[ARROWS] Move [SPACE] Act [TAB] Seed [F] Fert [S] Shop [I] Inv");
  mgt_canvas_write(c, 2, 2, line);
  if (f->msg[0])
    mgt_canvas_write(c, 52, 2, f->msg);
  if (st)
    snprintf(line, sizeof line, "Seeds t:%d c:%d p:%d  Fert:%d", st->farm_seeds[0],
             st->farm_seeds[1], st->farm_seeds[2], st->farm_fertilizer);
  else
    snprintf(line, sizeof line, "Seeds (harness)");
  mgt_canvas_write(c, 2, 3, line);
  mgt_canvas_write(c, 2, FARM_OFF_Y + FARM_BOX_H + 1, f->msg);
  farm_draw_rain(c, f);
}

static void farm_draw(void *vctx, MgtCanvas *c) {
  FarmCtx *f = (FarmCtx *)vctx;
  char line[96];
  int i;

  mgt_canvas_clear(c);

  if (f->screen == FARM_SCR_SHOP) {
    mgt_canvas_box(c, 10, 5, 60, 15, "SHOP");
    for (i = 0; i < 3; i++) {
      snprintf(line, sizeof line, "[%d] %s $%d", i + 1, k_seeds[i].name, k_seeds[i].cost);
      mgt_canvas_write(c, 15, 7 + i, line);
    }
    mgt_canvas_write(c, 15, 11, "[F] Fertilizer $20 (+5)");
    if (!f->has_scarecrow)
      mgt_canvas_write(c, 15, 12, "[K] Scarecrow $500");
    else
      mgt_canvas_write(c, 15, 12, "Scarecrow installed.");
    mgt_canvas_write(c, 15, 14, "[X] Exit Shop");
    mgt_canvas_write(c, 2, 22, f->msg);
    return;
  }

  if (f->screen == FARM_SCR_INV) {
    mgt_canvas_box(c, 10, 5, 60, 15, "BARN");
    if (f->crop_n == 0)
      mgt_canvas_write(c, 25, 10, "No crops in inventory.");
    else {
      for (i = 0; i < f->crop_n && i < 10; i++) {
        int si = seed_idx((char)f->crop_inv[i]);
        if (si >= 0) {
          snprintf(line, sizeof line, "%d. %s $%d", i + 1, k_seeds[si].name,
                   k_seeds[si].sell);
          mgt_canvas_write(c, 15, 7 + i, line);
        }
      }
    }
    snprintf(line, sizeof line, "Total Crops: %d", f->crop_n);
    mgt_canvas_write(c, 15, 18, line);
    mgt_canvas_write(c, 40, 18, "[X] Close");
    mgt_canvas_write(c, 2, 22, f->msg);
    return;
  }

  farm_draw_field(f, c);
}

static void farm_update(void *vctx, double dt) {
  FarmCtx *f = (FarmCtx *)vctx;
  int y, x;
  int was_rain;
  (void)dt;

  f->tick++;
  if (f->tick % 30 != 0) return;

  was_rain = farm_is_rain(f);
  if (f->from_game)
    farm_weather_from_state(f);
  else if (mgt_rand_range(0, 999) < 15)
    f->weather = 1 - f->weather;
  if (was_rain != farm_is_rain(f)) farm_weather_msg(f, was_rain);

  if (farm_is_rain(f)) {
    for (y = 0; y < FARM_H; y++)
      for (x = 0; x < FARM_W; x++)
        if (f->grid[y][x].plowed) f->grid[y][x].watered = 1;
  } else {
    for (y = 0; y < FARM_H; y++)
      for (x = 0; x < FARM_W; x++)
        if (f->grid[y][x].plowed && f->grid[y][x].seed && mgt_rand_range(0, 999) < 10)
          f->grid[y][x].watered = 0;
  }

  if (!f->has_scarecrow && !farm_is_rain(f) && mgt_rand_range(0, 999) < 10) {
    int cx = mgt_rand_range(0, FARM_W - 1);
    int cy = mgt_rand_range(0, FARM_H - 1);
    FarmCell *c = &f->grid[cy][cx];
    if (c->seed && c->growth < 100) c->has_crow = 1;
  }

  for (y = 0; y < FARM_H; y++) {
    for (x = 0; x < FARM_W; x++) {
      FarmCell *c = &f->grid[y][x];
      if (c->has_crow) {
        if (farm_manhattan(x, y, f->cx, f->cy) <= 1) {
          c->has_crow = 0;
          snprintf(f->msg, sizeof f->msg, "Crow scared!");
        } else if (mgt_rand_range(0, 999) < 10) {
          c->seed = 0;
          c->growth = 0;
          c->has_crow = 0;
          snprintf(f->msg, sizeof f->msg, "Crow ate crop!");
        }
      }
      if (c->seed && c->watered && c->growth < 100) {
        c->growth += 1;
        if (c->growth > 100) c->growth = 100;
      }
    }
  }
}

static int farm_key(void *vctx, MgtKey key, int ch) {
  FarmCtx *f = (FarmCtx *)vctx;
  MgtPersistentState *st = f->st;

  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) {
    if (f->screen != FARM_SCR_FIELD) {
      f->screen = FARM_SCR_FIELD;
      return 0;
    }
    return -1;
  }

  if (f->screen == FARM_SCR_SHOP) {
    if (key == MGT_KEY_CHAR && ch >= '1' && ch <= '3' && st) {
      int i = ch - '1';
      if (st->money >= k_seeds[i].cost) {
        st->money -= k_seeds[i].cost;
        if (k_seeds[i].id == 't') st->farm_seeds[0]++;
        else if (k_seeds[i].id == 'c') st->farm_seeds[1]++;
        else st->farm_seeds[2]++;
        snprintf(f->msg, sizeof f->msg, "Bought %s.", k_seeds[i].name);
      } else
        snprintf(f->msg, sizeof f->msg, "Not enough money.");
    }
    if (key == MGT_KEY_CHAR && (ch == 'f' || ch == 'F') && st) {
      if (st->money >= 20) {
        st->money -= 20;
        st->farm_fertilizer += 5;
        snprintf(f->msg, sizeof f->msg, "Bought fertilizer.");
      } else
        snprintf(f->msg, sizeof f->msg, "Not enough money.");
    }
    if (key == MGT_KEY_CHAR && (ch == 'k' || ch == 'K') && st && !f->has_scarecrow) {
      if (st->money >= 500) {
        st->money -= 500;
        f->has_scarecrow = 1;
        st->has_scarecrow = 1;
        snprintf(f->msg, sizeof f->msg, "Scarecrow installed!");
      } else
        snprintf(f->msg, sizeof f->msg, "Not enough money.");
    }
    if (key == MGT_KEY_CHAR && (ch == 'x' || ch == 'X')) f->screen = FARM_SCR_FIELD;
    return 0;
  }

  if (f->screen == FARM_SCR_INV) {
    if (key == MGT_KEY_CHAR && (ch == 'x' || ch == 'X')) f->screen = FARM_SCR_FIELD;
    return 0;
  }

  if (key == MGT_KEY_LEFT) f->cx = (f->cx + FARM_W - 1) % FARM_W;
  if (key == MGT_KEY_RIGHT) f->cx = (f->cx + 1) % FARM_W;
  if (key == MGT_KEY_UP) f->cy = (f->cy + FARM_H - 1) % FARM_H;
  if (key == MGT_KEY_DOWN) f->cy = (f->cy + 1) % FARM_H;
  if (key == MGT_KEY_SPACE) farm_act(f);
  if (key == MGT_KEY_TAB) f->seed_sel = (f->seed_sel + 1) % 3;
  if (key == MGT_KEY_CHAR && (ch == 's' || ch == 'S')) f->screen = FARM_SCR_SHOP;
  if (key == MGT_KEY_CHAR && (ch == 'i' || ch == 'I')) f->screen = FARM_SCR_INV;
  if (key == MGT_KEY_CHAR && (ch == 'f' || ch == 'F')) {
    FarmCell *c = &f->grid[f->cy][f->cx];
    if (st && st->farm_fertilizer > 0 && c->seed) {
      st->farm_fertilizer--;
      c->growth = 100;
      snprintf(f->msg, sizeof f->msg, "Fertilized!");
    } else
      snprintf(f->msg, sizeof f->msg, "No fertilizer or no crop.");
  }
  return 0;
}

int mg_run_farming(MgtSession *session) {
  FarmCtx ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.st = session ? session->state : NULL;
  ctx.session = session;
  ctx.from_game = session && session->from_game;
  ctx.screen = FARM_SCR_FIELD;
  snprintf(ctx.msg, sizeof ctx.msg, "Farm ready. Arrows + SPACE to work soil.");
  sync_from_state(&ctx);
  if (ctx.st && ctx.st->farm_seeds[0] == 0 && ctx.st->farm_seeds[1] == 0 &&
      ctx.st->farm_seeds[2] == 0) {
    ctx.st->farm_seeds[0] = 5;
    ctx.st->farm_seeds[1] = 2;
  }
  mgt_run_loop(&ctx, farm_update, farm_draw, farm_key);
  sync_to_state(&ctx);
  if (ctx.st) {
    ctx.st->skill_survival += 1;
    if (ctx.st->skill_survival > 100) ctx.st->skill_survival = 100;
    snprintf(ctx.st->last_banner, sizeof ctx.st->last_banner, "Farming session saved");
  }
  return 0;
}
