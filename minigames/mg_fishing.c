#include "mgt.h"
#include "mgt_platform.h"

#include <stdio.h>
#include <string.h>

enum {
  FISH_SCR_IDLE = 0,
  FISH_SCR_CASTING,
  FISH_SCR_WAITING,
  FISH_SCR_BITE,
  FISH_SCR_REELING,
  FISH_SCR_SHOP,
  FISH_SCR_INV
};

enum {
  FISH_WATER_Y = 12,
  FISH_ROD_TIP_X = 7,
  FISH_ROD_TIP_Y = 8,
  FISH_CAST_BASE_X = 10,
  FISH_DOCK_X = 0,
  FISH_LOG_ROW = 15,
  FISH_HELP_Y = 22
};

static void fish_put(MgtCanvas *c, int x, int y, char ch) {
  if (!c || x < 0 || x >= c->w || y < 0 || y >= c->h) return;
  c->cells[y * c->w + x] = ch;
}

typedef struct {
  const char *id;
  const char *name;
  int value;
  int difficulty;
  const char *ascii;
} FishDef;

static const FishDef k_fish[] = {
    {"old_boot", "Old Boot", 0, 10, "L__"},
    {"rusty_can", "Rusty Can", 0, 10, "( )"},
    {"guppy", "Guppy", 5, 20, "><>"},
    {"bass", "Bass", 15, 40, "><==>"},
    {"trout", "Trout", 30, 55, "><(({*>"},
    {"pike", "Pike", 60, 70, "><=======>"},
    {"salmon", "Salmon", 80, 75, "><(((((*>"},
    {"catfish", "Catfish", 100, 80, "><=====}>"},
    {"shark", "Shark", 250, 90, "/|\\___)>"},
    {"kraken", "Kraken", 1000, 98, "<{:}:{:}>"}};

enum { FISH_N = (int)(sizeof k_fish / sizeof k_fish[0]) };

typedef struct {
  const char *name;
  int price;
  double stat;
} UpgradeDef;

static const UpgradeDef k_rods[] = {
    {"Bamboo Pole", 0, 1.0},
    {"Fiberglass", 100, 1.5},
    {"Carbon Pro", 500, 2.2},
    {"Neptune's Spear", 2500, 4.0}};

static const UpgradeDef k_baits[] = {
    {"Bread", 0, 1.0},
    {"Worms", 50, 1.5},
    {"Crickets", 200, 2.5},
    {"Magic Lure", 1000, 5.0}};

typedef struct {
  MgtPersistentState *st;
  int screen;
  int money;
  int xp;
  int level;
  int rod;
  int bait;
  double cast_distance;
  double cast_target;
  double fish_progress;
  double tension;
  int hooked;
  int inv_idx[MGT_FISH_INV_MAX];
  int inv_count;
  char log[5][92];
  int log_n;
  unsigned long wait_until;
  unsigned long bite_fail;
  unsigned long cast_step_ms;
  unsigned long reel_step_ms;
  unsigned long reel_grace_until;
  unsigned long last_reel_ms;
  int tick;
} FishCtx;

static void flog(FishCtx *f, const char *msg) {
  int i;
  for (i = 4; i > 0; i--) strcpy(f->log[i], f->log[i - 1]);
  snprintf(f->log[0], sizeof f->log[0], "%s", msg ? msg : "");
  if (f->log_n < 5) f->log_n++;
}

static void fish_reset_line(FishCtx *f) {
  f->cast_distance = 0;
  f->fish_progress = 0;
  f->tension = 0;
  f->hooked = -1;
  f->bite_fail = 0;
  f->cast_target = 0;
  f->reel_step_ms = 0;
  f->reel_grace_until = 0;
  f->last_reel_ms = 0;
}

enum { FISH_REEL_COOLDOWN_MS = 100, FISH_PHYSICS_MS = 50 };

static int survival_bonus(const FishCtx *f) {
  if (!f->st) return 0;
  return f->st->skill_survival / 10;
}

static double bite_chance(const FishCtx *f) {
  double c = 0.3 + k_baits[f->bait].stat * 0.1;
  c += (double)survival_bonus(f) / 200.0;
  return c;
}

static int pick_fish(const FishCtx *f) {
  double roll = mgt_rand01() * 100.0 * k_baits[f->bait].stat +
                (double)survival_bonus(f) * 0.5;
  int i, best = 0, any = 0;
  for (i = 0; i < FISH_N; i++) {
    if (k_fish[i].difficulty <= (int)roll + 20) {
      any = 1;
      if (mgt_rand01() < 0.35) return i;
      best = i;
    }
  }
  if (!any) return 0;
  return best;
}

static void schedule_wait(FishCtx *f) {
  f->wait_until = mgt_now_ms() + (unsigned long)(2000 + mgt_rand01() * 3000.0);
}

static void catch_fish(FishCtx *f) {
  const FishDef *fish;
  if (f->hooked < 0 || f->hooked >= FISH_N) return;
  fish = &k_fish[f->hooked];
  if (f->inv_count < MGT_FISH_INV_MAX) f->inv_idx[f->inv_count++] = f->hooked;
  f->xp += fish->value;
  if (f->xp > f->level * 100) {
    f->level++;
    flog(f, "LEVEL UP!");
  }
  {
    char msg[92];
    snprintf(msg, sizeof msg, "CAUGHT: %s ($%d)", fish->name, fish->value);
    flog(f, msg);
  }
  f->screen = FISH_SCR_IDLE;
  fish_reset_line(f);
}

static void draw_hline(MgtCanvas *c, int y) {
  int x;
  if (y < 0 || y >= c->h) return;
  fish_put(c, 0, y, '+');
  for (x = 1; x < c->w - 1; x++) fish_put(c, x, y, '-');
  if (c->w > 1) fish_put(c, c->w - 1, y, '+');
}

static void draw_water(MgtCanvas *c, const FishCtx *f) {
  int x;
  char ch = (f->tick / 4) % 2 == 0 ? '~' : '-';
  for (x = 0; x < c->w; x++) fish_put(c, x, FISH_WATER_Y, ch);
}

static void draw_angler_figure(MgtCanvas *c, int reeling) {
  const int head_y = FISH_WATER_Y - 4;
  const int body_y = FISH_WATER_Y - 3;
  const int legs_y = FISH_WATER_Y - 2;
  const int dock_y = FISH_WATER_Y - 1;
  int x;

  fish_put(c, 5, head_y, 'O');
  fish_put(c, 4, body_y, '/');
  fish_put(c, 5, body_y, '|');
  fish_put(c, 6, body_y, '\\');
  fish_put(c, 7, body_y, reeling ? '/' : '_');

  for (x = FISH_ROD_TIP_X + 1; x <= 22; x++) fish_put(c, x, FISH_ROD_TIP_Y, '=');

  fish_put(c, 4, legs_y, '/');
  fish_put(c, 6, legs_y, '\\');

  for (x = 0; x < 8; x++) {
    if (x == 0 || x == 7)
      fish_put(c, FISH_DOCK_X + x, dock_y, '|');
    else
      fish_put(c, FISH_DOCK_X + x, dock_y, '=');
  }
}

static void draw_cast_line(MgtCanvas *c, int end_x, int end_y) {
  int steps, i;
  if (end_x <= FISH_ROD_TIP_X) return;
  steps = end_x - FISH_ROD_TIP_X;
  if (steps < 1) steps = 1;
  for (i = 0; i < steps; i++) {
    int x = FISH_ROD_TIP_X + i;
    int y = FISH_ROD_TIP_Y + (i * (end_y - FISH_ROD_TIP_Y)) / steps;
    if (x < end_x) fish_put(c, x, y, '.');
  }
}

static void draw_tension_bar(MgtCanvas *c, double tension) {
  char bar[48];
  char line[64];
  mgt_canvas_meter(bar, sizeof bar, 38, tension / 100.0);
  snprintf(line, sizeof line, "TENSION %s", bar);
  mgt_canvas_write(c, 18, 18, line);
  if (tension > 80.0) mgt_canvas_write(c, 62, 18, "WARNING!");
}

static void draw_log_panel(MgtCanvas *c, const FishCtx *f) {
  int y;
  draw_hline(c, FISH_LOG_ROW);
  for (y = 0; y < f->log_n && y < 5; y++)
    mgt_canvas_write(c, 2, FISH_LOG_ROW + 1 + y, f->log[y]);
}

static void draw_stats(MgtCanvas *c, const FishCtx *f) {
  char line[96];
  snprintf(line, sizeof line, "MONEY: $%-6d", f->money);
  mgt_canvas_write(c, 50, 2, line);
  snprintf(line, sizeof line, "LEVEL: %-3d", f->level);
  mgt_canvas_write(c, 50, 3, line);
  snprintf(line, sizeof line, "ROD:   %-18s", k_rods[f->rod].name);
  mgt_canvas_write(c, 50, 4, line);
  snprintf(line, sizeof line, "BAIT:  %-18s", k_baits[f->bait].name);
  mgt_canvas_write(c, 50, 5, line);
}

static void draw_help_bar(MgtCanvas *c, const char *help) {
  mgt_canvas_box(c, 0, FISH_HELP_Y, c->w, 3, NULL);
  if (help) mgt_canvas_write(c, 2, FISH_HELP_Y + 1, help);
}

static void fish_draw_shop(FishCtx *f, MgtCanvas *c) {
  char line[96];
  mgt_canvas_box(c, 8, 4, 64, 16, "THE ANGLER'S SHOP");
  mgt_canvas_write(c, 12, 7, "RODS [1] buy next tier");
  if (f->rod + 1 < 4)
    snprintf(line, sizeof line, "Next: %s ($%d) power %.1f",
             k_rods[f->rod + 1].name, k_rods[f->rod + 1].price,
             k_rods[f->rod + 1].stat);
  else
    snprintf(line, sizeof line, "MAX ROD");
  mgt_canvas_write(c, 12, 8, line);
  mgt_canvas_write(c, 12, 10, "BAIT [2] buy next tier");
  if (f->bait + 1 < 4)
    snprintf(line, sizeof line, "Next: %s ($%d) qual %.1f",
             k_baits[f->bait + 1].name, k_baits[f->bait + 1].price,
             k_baits[f->bait + 1].stat);
  else
    snprintf(line, sizeof line, "MAX BAIT");
  mgt_canvas_write(c, 12, 11, line);
  snprintf(line, sizeof line, "YOUR MONEY: $%d   [X] exit shop", f->money);
  mgt_canvas_write(c, 12, 14, line);
}

static void fish_draw_inv(FishCtx *f, MgtCanvas *c) {
  char line[96];
  int i, start;
  mgt_canvas_box(c, 8, 4, 64, 16, "CATCH LOG");
  if (f->inv_count == 0) {
    mgt_canvas_write(c, 26, 9, "Your net is empty.");
  } else {
    start = f->inv_count > 8 ? f->inv_count - 8 : 0;
    for (i = start; i < f->inv_count; i++) {
      const FishDef *fish = &k_fish[f->inv_idx[i]];
      snprintf(line, sizeof line, "%d. %-18s $%d", i + 1, fish->name, fish->value);
      mgt_canvas_write(c, 12, 7 + (i - start), line);
    }
  }
  snprintf(line, sizeof line, "Total: %d   [S] sell all   [X] close", f->inv_count);
  mgt_canvas_write(c, 12, 14, line);
}

static void fish_draw_lake(FishCtx *f, MgtCanvas *c) {
  int end_x, end_y;
  const char *help = "";

  draw_stats(c, f);
  draw_water(c, f);

  draw_angler_figure(c, f->screen == FISH_SCR_REELING);

  if (f->screen != FISH_SCR_IDLE) {
    if (f->screen == FISH_SCR_REELING)
      end_x = FISH_CAST_BASE_X + (int)f->fish_progress;
    else
      end_x = FISH_CAST_BASE_X + (int)f->cast_distance;
    end_y = FISH_WATER_Y;
    if (end_x < FISH_CAST_BASE_X) end_x = FISH_CAST_BASE_X;
    if (end_x >= c->w - 2) end_x = c->w - 2;

    draw_cast_line(c, end_x, end_y);

    if (f->screen == FISH_SCR_CASTING) {
      fish_put(c, end_x, end_y, '*');
    } else if (f->screen == FISH_SCR_WAITING) {
      char bob = (f->tick / 10) % 2 ? 'O' : 'o';
      fish_put(c, end_x, end_y, bob);
    } else if (f->screen == FISH_SCR_BITE) {
      mgt_canvas_write(c, end_x - 1, end_y, "!!!");
    } else if (f->screen == FISH_SCR_REELING && f->hooked >= 0) {
      int fx = FISH_CAST_BASE_X + (int)f->fish_progress;
      int fy = FISH_WATER_Y + 1;
      const char *ascii = k_fish[f->hooked].ascii;
      int aw = (int)strlen(ascii);
      if (fx < 0) fx = 0;
      if (fx + aw >= c->w) fx = c->w - aw - 1;
      if (fy >= 0 && fy < c->h) mgt_canvas_write(c, fx, fy, ascii);
      draw_tension_bar(c, f->tension);
    }
  }

  draw_log_panel(c, f);

  if (f->screen == FISH_SCR_IDLE)
    help = "[SPACE] Cast  [S] Shop  [I] Inv  [ESC] Exit";
  else if (f->screen == FISH_SCR_CASTING)
    help = "Casting...";
  else if (f->screen == FISH_SCR_WAITING)
    help = "Wait for the bite... (SPACE too early cancels)";
  else if (f->screen == FISH_SCR_BITE)
    help = "[SPACE] HOOK IT NOW!";
  else if (f->screen == FISH_SCR_REELING)
    help = "[SPACE] Tap to reel (not hold!)  watch TENSION";

  draw_help_bar(c, help);
}

static void fish_draw(void *vctx, MgtCanvas *c) {
  FishCtx *f = (FishCtx *)vctx;

  mgt_canvas_clear(c);

  if (f->screen == FISH_SCR_SHOP) {
    fish_draw_shop(f, c);
    return;
  }
  if (f->screen == FISH_SCR_INV) {
    fish_draw_inv(f, c);
    return;
  }
  fish_draw_lake(f, c);
}

static void fish_update(void *vctx, double dt) {
  FishCtx *f = (FishCtx *)vctx;
  unsigned long now = mgt_now_ms();
  (void)dt;

  f->tick++;

  if (f->screen == FISH_SCR_CASTING) {
    if (now >= f->cast_step_ms) {
      f->cast_distance += 5.0;
      f->cast_step_ms = now + 50;
      if (f->cast_target <= 0.0) f->cast_target = 40.0 + mgt_rand01() * 20.0;
      if (f->cast_distance >= f->cast_target) {
        f->screen = FISH_SCR_WAITING;
        flog(f, "Line settled. Waiting for bite...");
        schedule_wait(f);
      }
    }
    return;
  }

  if (f->screen == FISH_SCR_WAITING && now >= f->wait_until) {
    if (mgt_rand01() < bite_chance(f)) {
      f->screen = FISH_SCR_BITE;
      f->hooked = pick_fish(f);
      f->fish_progress = f->cast_distance;
      f->bite_fail =
          now + (unsigned long)(2500.0 + k_baits[f->bait].stat * 300.0);
      flog(f, "!!! BITE !!! PRESS SPACE!");
    } else {
      flog(f, "Nibble... but nothing.");
      schedule_wait(f);
    }
    return;
  }

  if (f->screen == FISH_SCR_BITE && f->bite_fail && now >= f->bite_fail) {
    f->screen = FISH_SCR_IDLE;
    fish_reset_line(f);
    flog(f, "The fish got away...");
    return;
  }

  if (f->screen == FISH_SCR_REELING && f->hooked >= 0) {
    if (f->reel_step_ms == 0) f->reel_step_ms = now;
    if (now >= f->reel_step_ms) {
      double fish_str = k_fish[f->hooked].difficulty / 10.0;
      f->reel_step_ms = now + (unsigned long)FISH_PHYSICS_MS;
      f->tension += (mgt_rand01() * 2.0 * fish_str - 0.8) * 0.55;
      f->fish_progress += 0.06 * fish_str;
      if (f->tension < 0.0) f->tension = 0.0;
      if (now >= f->reel_grace_until && f->tension >= 100.0) {
        f->screen = FISH_SCR_IDLE;
        fish_reset_line(f);
        flog(f, "LINE SNAPPED!");
        return;
      }
      if (f->fish_progress <= 1.0) {
        catch_fish(f);
        return;
      }
    }
  }
}

static void buy_rod(FishCtx *f) {
  int next = f->rod + 1;
  if (next >= 4) {
    flog(f, "Max rod reached!");
    return;
  }
  if (f->money < k_rods[next].price) {
    flog(f, "Not enough cash!");
    return;
  }
  f->money -= k_rods[next].price;
  f->rod = next;
  flog(f, "Bought rod upgrade.");
}

static void buy_bait(FishCtx *f) {
  int next = f->bait + 1;
  if (next >= 4) {
    flog(f, "Max bait reached!");
    return;
  }
  if (f->money < k_baits[next].price) {
    flog(f, "Not enough cash!");
    return;
  }
  f->money -= k_baits[next].price;
  f->bait = next;
  flog(f, "Bought bait upgrade.");
}

static int fish_key(void *vctx, MgtKey key, int ch) {
  FishCtx *f = (FishCtx *)vctx;

  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) {
    if (f->screen == FISH_SCR_SHOP || f->screen == FISH_SCR_INV) {
      f->screen = FISH_SCR_IDLE;
      return 0;
    }
    return -1;
  }

  if (f->screen == FISH_SCR_SHOP) {
    if (key == MGT_KEY_CHAR && (ch == 'x' || ch == 'X')) f->screen = FISH_SCR_IDLE;
    if (key == MGT_KEY_CHAR && ch == '1') buy_rod(f);
    if (key == MGT_KEY_CHAR && ch == '2') buy_bait(f);
    return 0;
  }

  if (f->screen == FISH_SCR_INV) {
    if (key == MGT_KEY_CHAR && (ch == 'x' || ch == 'X')) f->screen = FISH_SCR_IDLE;
    if (key == MGT_KEY_CHAR && (ch == 's' || ch == 'S')) {
      int i, tot = 0;
      for (i = 0; i < f->inv_count; i++) tot += k_fish[f->inv_idx[i]].value;
      f->money += tot;
      f->inv_count = 0;
      flog(f, "Sold all fish.");
    }
    return 0;
  }

  if (key == MGT_KEY_CHAR && (ch == 's' || ch == 'S') && f->screen == FISH_SCR_IDLE) {
    f->screen = FISH_SCR_SHOP;
    return 0;
  }
  if (key == MGT_KEY_CHAR && (ch == 'i' || ch == 'I') && f->screen == FISH_SCR_IDLE) {
    f->screen = FISH_SCR_INV;
    return 0;
  }

  if (f->screen == FISH_SCR_WAITING && key == MGT_KEY_SPACE) {
    f->screen = FISH_SCR_IDLE;
    fish_reset_line(f);
    flog(f, "Pulled too early!");
    return 0;
  }

  if (key != MGT_KEY_SPACE) return 0;

  if (f->screen == FISH_SCR_IDLE) {
    f->screen = FISH_SCR_CASTING;
    f->cast_distance = 0;
    f->cast_target = 0;
    f->cast_step_ms = mgt_now_ms();
    flog(f, "Casting line...");
    return 0;
  }
  if (f->screen == FISH_SCR_BITE) {
    if (f->hooked < 0) return 0;
    f->screen = FISH_SCR_REELING;
    f->fish_progress = f->cast_distance;
    f->tension = 15.0;
    f->bite_fail = 0;
    f->reel_step_ms = mgt_now_ms();
    f->last_reel_ms = 0;
    f->reel_grace_until = mgt_now_ms() + 1200;
    {
      char msg[92];
      snprintf(msg, sizeof msg, "HOOKED %s! REEL IT IN!", k_fish[f->hooked].name);
      flog(f, msg);
    }
    return 0;
  }
  if (f->screen == FISH_SCR_REELING && f->hooked >= 0) {
    unsigned long now = mgt_now_ms();
    if (f->last_reel_ms && now - f->last_reel_ms < (unsigned long)FISH_REEL_COOLDOWN_MS)
      return 0;
    f->last_reel_ms = now;
    f->fish_progress -= k_rods[f->rod].stat * 2.2;
    if (now >= f->reel_grace_until)
      f->tension += 3.0;
    if (f->tension > 100.0) f->tension = 100.0;
    return 0;
  }
  return 0;
}

static void fish_sync_to_state(FishCtx *f) {
  MgtPersistentState *st = f->st;
  int i;
  if (!st) return;
  st->money = f->money;
  st->rod_index = f->rod;
  st->bait_index = f->bait;
  st->fishing_xp = f->xp;
  st->fishing_level = f->level;
  st->fish_inv_n = f->inv_count;
  for (i = 0; i < f->inv_count && i < MGT_FISH_INV_MAX; i++) {
    const FishDef *fish = &k_fish[f->inv_idx[i]];
    snprintf(st->fish_inv[i].id, sizeof st->fish_inv[i].id, "%s", fish->id);
    snprintf(st->fish_inv[i].name, sizeof st->fish_inv[i].name, "%s", fish->name);
    snprintf(st->fish_inv[i].ascii, sizeof st->fish_inv[i].ascii, "%s",
             fish->ascii);
    st->fish_inv[i].value = fish->value;
    st->fish_inv[i].difficulty = fish->difficulty;
    st->fish_inv[i].weight_x10 = 10;
  }
}

static void fish_load_from_state(FishCtx *f) {
  MgtPersistentState *st = f->st;
  int i;
  if (!st) {
    f->money = 50;
    f->level = 1;
    f->xp = 0;
    return;
  }
  f->money = st->money > 0 ? st->money : 50;
  f->rod = st->rod_index;
  f->bait = st->bait_index;
  if (f->rod < 0) f->rod = 0;
  if (f->rod > 3) f->rod = 3;
  if (f->bait < 0) f->bait = 0;
  if (f->bait > 3) f->bait = 3;
  f->level = st->fishing_level > 0 ? st->fishing_level : 1;
  f->xp = st->fishing_xp;
  f->inv_count = st->fish_inv_n;
  if (f->inv_count > MGT_FISH_INV_MAX) f->inv_count = MGT_FISH_INV_MAX;
  for (i = 0; i < f->inv_count; i++) {
    int j;
    f->inv_idx[i] = 0;
    for (j = 0; j < FISH_N; j++) {
      if (!strcmp(st->fish_inv[i].id, k_fish[j].id) ||
          !strcmp(st->fish_inv[i].name, k_fish[j].name)) {
        f->inv_idx[i] = j;
        break;
      }
    }
  }
}

int mg_run_fishing(MgtSession *session) {
  FishCtx ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.st = session ? session->state : NULL;
  ctx.screen = FISH_SCR_IDLE;
  ctx.hooked = -1;
  fish_load_from_state(&ctx);
  flog(&ctx, "Terminal Fisher - SPACE to cast");

  mgt_run_loop(&ctx, fish_update, fish_draw, fish_key);

  fish_sync_to_state(&ctx);
  if (session && session->state) {
    session->state->last_success = 1;
    snprintf(session->state->last_banner, sizeof session->state->last_banner,
             "Fishing L%d $%d", ctx.level, ctx.money);
  }
  return 0;
}
