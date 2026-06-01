#include "mgt.h"
#include "mgt_game_bridge.h"
#include "mgt_host.h"
#include "mgt_platform.h"
#include "mgt_sync.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

enum {
  HUNT_TRACK = 0,
  HUNT_APPROACH = 1,
  HUNT_SHOT = 2,
  HUNT_RESULT = 3
};

typedef struct {
  const char *id;
  const char *name;
  const char *clue_primary;
  const char *clue_secondary;
  const char *listen_hint;
  double shot_width;
  int approach_distance;
  int max_approach_turns;
  int meat_qty;
  int pelt_qty;
} PreySpec;

typedef struct {
  MgtSession *session;
  MgtPersistentState *st;
  int phase;
  const PreySpec *prey;
  char area[32];
  char clue_a[96];
  char clue_b[96];
  char choice_id[3][16];
  char choice_label[3][24];
  char answer_id[16];
  int track_mistakes;
  int approach_distance;
  int approach_turns;
  int max_approach_turns;
  int noise;
  int stamina;
  int focus;
  int heard_prey;
  double shot_meter;
  int shot_dir;
  double shot_center;
  double shot_width;
  double shot_speed;
  int outcome;
  int loot_meat;
  int loot_pelt;
  char message[120];
  char result[160];
} HuntCtx;

static const PreySpec k_prey[] = {
    {"rabbit", "Rabbit",
     "Tiny paired prints — four toes, no claw drag — stitch through clover.",
     "Fresh droppings like dark beads; nibbled stems at ankle height.",
     "A dry rustle in the brush, close and nervous.",
     0.15, 3, 6, 1, 0},
    {"deer", "Deer",
     "Split hoofprints in soft loam, spaced for a loping stride.",
     "Bark rubbed smooth at chest height; musk sweet and green.",
     "A branch snaps once, then deliberate silence.",
     0.18, 4, 7, 2, 1},
    {"wolf", "Wolf",
     "Heavy paw pads with claw scores; the trail circles back on itself.",
     "Rank musk and a tuft of grey fur snagged on thorn.",
     "Low breathing downwind — something large and patient.",
     0.11, 5, 8, 2, 1}};

static const PreySpec *prey_for_id(const char *id) {
  size_t i;
  if (!id || !id[0]) return &k_prey[1];
  for (i = 0; i < sizeof k_prey / sizeof k_prey[0]; i++)
    if (!strcmp(k_prey[i].id, id)) return &k_prey[i];
  return &k_prey[1];
}

static void shuffle_choices(HuntCtx *ctx) {
  const char *pool[3] = {"rabbit", "deer", "wolf"};
  const char *labels[3] = {"Rabbit", "Deer", "Wolf"};
  int order[3] = {0, 1, 2};
  int i, j, tmp;
  for (i = 2; i > 0; i--) {
    j = mgt_rand_range(0, i);
    tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
  }
  for (i = 0; i < 3; i++) {
    snprintf(ctx->choice_id[i], sizeof ctx->choice_id[i], "%s", pool[order[i]]);
    snprintf(ctx->choice_label[i], sizeof ctx->choice_label[i], "%s",
             labels[order[i]]);
  }
}

static void bar(char *out, size_t cap, int val, int max, int len) {
  int n, i;
  (void)cap;
  if (max < 1) max = 1;
  n = (int)((double)val / (double)max * (double)len + 0.5);
  if (n > len) n = len;
  if (n < 0) n = 0;
  out[0] = '[';
  for (i = 0; i < len; i++) out[1 + i] = (i < n) ? '#' : '-';
  out[1 + len] = ']';
  out[2 + len] = '\0';
}

static void hunt_give_item(MgtSession *session, const char *id, int n) {
  MgtGameSim *sim;
  int i;
  if (!id || n <= 0) return;
  if (session && session->from_game) {
    for (i = 0; i < n; i++) aet_minigames_give_item(id);
    return;
  }
  sim = mgt_host_game_sim();
  if (sim)
    for (i = 0; i < n; i++) mgt_sim_give_item(sim, id);
}

static void hunt_finish(HuntCtx *ctx, int success) {
  MgtPersistentState *st = ctx->st;
  ctx->phase = HUNT_RESULT;
  if (success && ctx->loot_meat + ctx->loot_pelt > 0) {
    hunt_give_item(ctx->session, "meat", ctx->loot_meat);
    hunt_give_item(ctx->session, "animal_pelt", ctx->loot_pelt);
  }
  if (st) {
    st->last_success = success ? 1 : 0;
    if (success) {
      if (ctx->outcome >= 3)
        snprintf(st->last_banner, sizeof st->last_banner,
                 "Perfect kill: %d meat, %d pelt.", ctx->loot_meat, ctx->loot_pelt);
      else if (ctx->outcome >= 2)
        snprintf(st->last_banner, sizeof st->last_banner,
                 "Clean shot: %d meat, %d pelt.", ctx->loot_meat, ctx->loot_pelt);
      else
        snprintf(st->last_banner, sizeof st->last_banner,
                 "Grazing shot: %d meat.", ctx->loot_meat);
      if (st->skill_survival < 100) st->skill_survival++;
      if (ctx->session && ctx->session->adventure_embedded)
        st->hunt_cooldown_until_turn = st->adventure_turn + 6;
    } else if (ctx->result[0]) {
      snprintf(st->last_banner, sizeof st->last_banner, "%s", ctx->result);
    }
  }
}

static void hunt_draw(void *vctx, MgtCanvas *c) {
  HuntCtx *ctx = (HuntCtx *)vctx;
  char line[96];
  int len, i, mpos, cpos, half;
  char meter[68];

  mgt_canvas_clear(c);
  mgt_canvas_box(c, 1, 1, 94, 28, "HUNTING");
  snprintf(line, sizeof line, "Quarry: %s   Area: %s", ctx->prey ? ctx->prey->name : "?",
           ctx->area[0] ? ctx->area : "Wilds");
  mgt_canvas_write(c, 4, 3, line);
  mgt_canvas_write(c, 4, 4, "Track -> Approach -> Shot   [1-3] [1-4] [SPACE]   ESC quit");
  bar(line, sizeof line, ctx->stamina, 100, 16);
  mgt_canvas_write(c, 4, 5, "Stamina ");
  mgt_canvas_write(c, 14, 5, line);
  bar(line, sizeof line, ctx->noise, 100, 16);
  mgt_canvas_write(c, 34, 5, " Noise ");
  mgt_canvas_write(c, 42, 5, line);
  if (ctx->focus > 0) {
    snprintf(line, sizeof line, "Focus +%d", ctx->focus);
    mgt_canvas_write(c, 62, 5, line);
  }

  if (ctx->phase == HUNT_TRACK) {
    mgt_canvas_box(c, 4, 7, 88, 10, "TRACK READ");
    mgt_canvas_write(c, 6, 9, ctx->clue_a);
    mgt_canvas_write(c, 6, 10, ctx->clue_b);
    for (i = 0; i < 3; i++) {
      snprintf(line, sizeof line, "%d) %s", i + 1, ctx->choice_label[i]);
      mgt_canvas_write(c, 8, 12 + i, line);
    }
    snprintf(line, sizeof line, "Misreads: %d/2", ctx->track_mistakes);
    mgt_canvas_write(c, 6, 16, line);
  } else if (ctx->phase == HUNT_APPROACH) {
    mgt_canvas_box(c, 4, 7, 88, 10, "APPROACH");
    snprintf(line, sizeof line, "Distance: %d   Turns left: %d", ctx->approach_distance,
             ctx->max_approach_turns - ctx->approach_turns);
    mgt_canvas_write(c, 6, 9, line);
    if (ctx->heard_prey)
      mgt_canvas_write(c, 6, 10, ctx->prey ? ctx->prey->listen_hint : "");
    mgt_canvas_write(c, 8, 13, "1) Slow   2) Steady   3) Rush   4) Wait & listen");
  } else if (ctx->phase == HUNT_SHOT) {
    len = 60;
    mpos = (int)(ctx->shot_meter * (double)(len - 1) + 0.5);
    cpos = (int)(ctx->shot_center * (double)(len - 1) + 0.5);
    half = (int)(ctx->shot_width * (double)len / 2.0 + 0.5);
    if (half < 1) half = 1;
    mgt_canvas_box(c, 4, 7, 88, 10, "SHOT WINDOW");
    for (i = 0; i < len; i++) meter[i] = '-';
    for (i = cpos - half; i <= cpos + half; i++)
      if (i >= 0 && i < len) meter[i] = '=';
    if (mpos >= 0 && mpos < len) meter[mpos] = '|';
    meter[len] = '\0';
    snprintf(line, sizeof line, "[%s]", meter);
    mgt_canvas_write(c, 6, 9, line);
    mgt_canvas_write(c, 6, 11, "SPACE when the marker sits in the window.");
  } else {
    mgt_canvas_box(c, 4, 7, 88, 10, "RESULT");
    mgt_canvas_write(c, 6, 9, ctx->result);
    mgt_canvas_write(c, 6, 11, "SPACE or ESC to return.");
  }

  snprintf(line, sizeof line, "Status: %.90s", ctx->message);
  mgt_canvas_write(c, 4, 26, line);
}

static void hunt_update(void *vctx, double dt) {
  HuntCtx *ctx = (HuntCtx *)vctx;
  if (ctx->phase != HUNT_SHOT) return;
  ctx->shot_meter += ctx->shot_speed * (double)ctx->shot_dir * dt * 60.0;
  if (ctx->shot_meter >= 1.0) {
    ctx->shot_meter = 1.0;
    ctx->shot_dir = -1;
  } else if (ctx->shot_meter <= 0.0) {
    ctx->shot_meter = 0.0;
    ctx->shot_dir = 1;
  }
}

static void hunt_fail(HuntCtx *ctx, const char *result, const char *msg) {
  snprintf(ctx->result, sizeof ctx->result, "%s", result ? result : "Hunt failed.");
  snprintf(ctx->message, sizeof ctx->message, "%s", msg ? msg : "Failed.");
  hunt_finish(ctx, 0);
}

static int hunt_key(void *vctx, MgtKey key, int ch) {
  HuntCtx *ctx = (HuntCtx *)vctx;

  if (ctx->phase == HUNT_RESULT) {
    if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT || key == MGT_KEY_SPACE) return -1;
    return 0;
  }
  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) return -1;

  if (ctx->phase == HUNT_TRACK && key == MGT_KEY_CHAR && ch >= '1' && ch <= '3') {
    int idx = ch - '1';
    if (!strcmp(ctx->choice_id[idx], ctx->answer_id)) {
      ctx->focus += 12;
      ctx->phase = HUNT_APPROACH;
      snprintf(ctx->message, sizeof ctx->message,
               "Sign confirmed. Begin the approach.");
    } else {
      ctx->track_mistakes++;
      ctx->noise += 12;
      if (ctx->track_mistakes >= 2) {
        hunt_fail(ctx, "You misread the trail and lose the spoor.",
                  "Trail lost.");
      } else {
        shuffle_choices(ctx);
        snprintf(ctx->message, sizeof ctx->message,
                 "Wrong quarry sign. Study the spoor again.");
      }
    }
    return 0;
  }

  if (ctx->phase == HUNT_APPROACH && key == MGT_KEY_CHAR && ch >= '1' && ch <= '4') {
    ctx->approach_turns++;
    if (ch == '1') {
      ctx->approach_distance -= 1;
      ctx->noise -= 4;
      ctx->stamina -= 5;
    } else if (ch == '2') {
      ctx->approach_distance -= 1;
      ctx->noise += 5;
      ctx->stamina -= 8;
    } else if (ch == '3') {
      ctx->approach_distance -= 2;
      ctx->noise += 16;
      ctx->stamina -= 14;
    } else {
      ctx->noise -= 10;
      ctx->stamina += 6;
      if (mgt_rand01() < 0.45) {
        ctx->heard_prey = 1;
        ctx->focus += 6;
      } else if (mgt_rand01() < 0.25)
        ctx->approach_distance += 1;
    }
    if (ctx->noise < 0) ctx->noise = 0;
    if (ctx->stamina > 100) ctx->stamina = 100;
    if (ctx->noise >= 85) {
      hunt_fail(ctx, "The quarry spooks and vanishes.", "Too much noise.");
      return 0;
    }
    if (ctx->stamina <= 0) {
      hunt_fail(ctx, "You are too exhausted to keep stalking.", "Out of stamina.");
      return 0;
    }
    if (ctx->approach_turns >= ctx->max_approach_turns && ctx->approach_distance > 0) {
      hunt_fail(ctx, "The window closes before you get a shot.", "Approach timed out.");
      return 0;
    }
    if (ctx->approach_distance <= 0) {
      ctx->phase = HUNT_SHOT;
      ctx->shot_meter = mgt_rand01();
      ctx->shot_dir = mgt_rand01() < 0.5 ? 1 : -1;
      snprintf(ctx->message, sizeof ctx->message, "Line of sight. Take the shot.");
    } else {
      snprintf(ctx->message, sizeof ctx->message, "You slip closer...");
    }
    return 0;
  }

  if (ctx->phase == HUNT_SHOT && key == MGT_KEY_SPACE) {
    double delta = fabs(ctx->shot_meter - ctx->shot_center);
    double perfect = ctx->shot_width * 0.35;
    double graze = ctx->shot_width * 1.55;
    if (delta <= perfect) {
      ctx->outcome = 3;
      ctx->loot_meat = ctx->prey ? ctx->prey->meat_qty : 1;
      ctx->loot_pelt = ctx->prey ? ctx->prey->pelt_qty : 0;
      if (ctx->prey && !strcmp(ctx->prey->id, "rabbit")) ctx->loot_pelt = 0;
      snprintf(ctx->result, sizeof ctx->result, "Perfect shot on the %s.",
               ctx->prey ? ctx->prey->name : "quarry");
      snprintf(ctx->message, sizeof ctx->message, "Clean kill.");
      hunt_finish(ctx, 1);
    } else if (delta <= ctx->shot_width) {
      ctx->outcome = 2;
      ctx->loot_meat = ctx->prey ? ctx->prey->meat_qty : 1;
      ctx->loot_pelt = ctx->prey && ctx->prey->pelt_qty > 0 ? 1 : 0;
      snprintf(ctx->result, sizeof ctx->result, "Solid hit on the %s.",
               ctx->prey ? ctx->prey->name : "quarry");
      snprintf(ctx->message, sizeof ctx->message, "Good shot.");
      hunt_finish(ctx, 1);
    } else if (delta <= graze) {
      ctx->outcome = 1;
      ctx->loot_meat = 1;
      ctx->loot_pelt = 0;
      snprintf(ctx->result, sizeof ctx->result,
               "The shot grazes; you recover partial meat.");
      snprintf(ctx->message, sizeof ctx->message, "Grazing hit.");
      hunt_finish(ctx, 1);
    } else {
      hunt_fail(ctx, "The shot goes wide; the quarry escapes.", "Missed.");
    }
    return 0;
  }
  return 0;
}

static void hunt_apply_weather(HuntCtx *ctx, const char *weather) {
  if (!weather || !weather[0]) return;
  if (strstr(weather, "rain") != NULL || strstr(weather, "storm") != NULL)
    ctx->shot_speed += 0.0035;
  if (strstr(weather, "fog") != NULL || strstr(weather, "mist") != NULL)
    ctx->shot_width *= 1.12;
  if (strstr(weather, "clear") != NULL) ctx->shot_speed -= 0.0010;
  if (ctx->shot_speed < 0.0065) ctx->shot_speed = 0.0065;
  if (ctx->shot_speed > 0.0160) ctx->shot_speed = 0.0160;
}

static int hunt_gate(MgtSession *session, MgtPersistentState *st) {
  int wait;
  if (!st) return 1;
  if (!st->has_hunting_bow) {
    st->last_success = 0;
    snprintf(st->last_banner, sizeof st->last_banner,
             "You need a bow to hunt here.");
    mgt_host_message("Hunting", st->last_banner);
    return 0;
  }
  if (session && session->adventure_embedded && !mgt_autotest_active() &&
      st->hunt_cooldown_until_turn > st->adventure_turn) {
    wait = st->hunt_cooldown_until_turn - st->adventure_turn;
    st->last_success = 0;
    snprintf(st->last_banner, sizeof st->last_banner,
             "Trails are cold — wait %d more turn%s.", wait, wait == 1 ? "" : "s");
    mgt_host_message("Hunting", st->last_banner);
    return 0;
  }
  return 1;
}

static void hunt_setup(HuntCtx *ctx, MgtSession *session, MgtPersistentState *st,
                       const char *target_id) {
  const PreySpec *prey;
  double skill_bonus;

  memset(ctx, 0, sizeof *ctx);
  ctx->session = session;
  ctx->st = st;
  prey = prey_for_id(target_id);
  ctx->prey = prey;
  snprintf(ctx->answer_id, sizeof ctx->answer_id, "%s", prey->id);
  snprintf(ctx->clue_a, sizeof ctx->clue_a, "%s", prey->clue_primary);
  snprintf(ctx->clue_b, sizeof ctx->clue_b, "%s", prey->clue_secondary);
  if (st && st->hunt_area[0])
    snprintf(ctx->area, sizeof ctx->area, "%s", st->hunt_area);
  else
    snprintf(ctx->area, sizeof ctx->area, "Wild Margin");
  shuffle_choices(ctx);
  ctx->phase = HUNT_TRACK;
  ctx->approach_distance = prey->approach_distance;
  ctx->max_approach_turns = prey->max_approach_turns;
  ctx->stamina = 100;
  ctx->shot_center = 0.28 + mgt_rand01() * 0.44;
  ctx->shot_width = prey->shot_width;
  ctx->shot_speed = 0.0095;
  skill_bonus = st ? (double)(st->skill_survival - 50) * 0.0015 : 0.0;
  ctx->shot_width *= 1.0 + skill_bonus;
  if (ctx->focus > 0) ctx->shot_width *= 1.05;
  if (ctx->shot_width < 0.07) ctx->shot_width = 0.07;
  if (ctx->shot_width > 0.24) ctx->shot_width = 0.24;
  if (st) hunt_apply_weather(ctx, st->game_weather);
  snprintf(ctx->message, sizeof ctx->message,
           "Read both sign lines, then pick the quarry.");
}

int mg_run_hunting(MgtSession *session) {
  HuntCtx ctx;
  MgtPersistentState *st = session ? session->state : NULL;
  const char *target = "deer";

  if (!hunt_gate(session, st)) return 0;
  if (st && st->hunt_target[0]) target = st->hunt_target;
  hunt_setup(&ctx, session, st, target);
  if (mgt_autotest_script("hunt_win")) {
    ctx.outcome = 2;
    ctx.loot_meat = ctx.prey ? ctx.prey->meat_qty : 1;
    ctx.loot_pelt = ctx.prey ? ctx.prey->pelt_qty : 0;
    if (ctx.prey && !strcmp(ctx.prey->id, "rabbit")) ctx.loot_pelt = 0;
    snprintf(ctx.result, sizeof ctx.result, "Clean shot on the %s.",
             ctx.prey ? ctx.prey->name : "quarry");
    snprintf(ctx.message, sizeof ctx.message, "Good shot.");
    hunt_finish(&ctx, 1);
    return 0;
  }
  mgt_run_loop(&ctx, hunt_update, hunt_draw, hunt_key);
  if (st && st->last_success < 0) st->last_success = 0;
  return 0;
}
