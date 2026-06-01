#include "mgt.h"
#include "mgt_host.h"
#include "mgt_platform.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  MgtPersistentState *st;
  char target_name[64];
  char target_dir[8];
  int difficulty;
  int skill;
  int pin_count;
  struct {
    double center;
    double width;
    int set;
  } pins[4];
  int pin_index;
  double cursor;
  int dir;
  double speed;
  double tension;
  double pick_integrity;
  int focus_charges;
  double focus_timer_ms;
  int attempts;
  int misses;
  int rescued_hits;
  double lock_noise;
  int rusty_wear;
  char tool_label[32];
  char tool_id[24];
  int has_tension_wrench;
  int finished;
  int success;
  char message[160];
  char last_check[96];
  char noise_band[8];
} LpCtx;

static const char *noise_band_label(double n) {
  if (n >= 70.0) return "HIGH";
  if (n >= 35.0) return "MED";
  return "LOW";
}

static void lp_store_exit(LpCtx *ctx) {
  MgtPersistentState *st = ctx ? ctx->st : NULL;
  if (!st) return;
  st->lock_exit_noise = (int)(ctx->lock_noise + 0.5);
  if (st->lock_exit_noise < 0) st->lock_exit_noise = 0;
  if (st->lock_exit_noise > 100) st->lock_exit_noise = 100;
  st->lock_exit_misses = ctx->misses;
  st->lock_pick_broken = (ctx->pick_integrity <= 0.0 && ctx->finished && !ctx->success) ? 1 : 0;
  snprintf(st->lock_noise_band, sizeof st->lock_noise_band, "%s",
           noise_band_label(ctx->lock_noise));
  snprintf(st->lock_tool_id, sizeof st->lock_tool_id, "%s",
           ctx->tool_id[0] ? ctx->tool_id : "lockpick");
}

static void lane_string(char *out, size_t cap, const LpCtx *ctx, int pin_i,
                        int active) {
  const int len = 56;
  int i;
  double pin_c, half;
  int center, hstart, hend, cpos;
  if (!out || cap < (size_t)len + 4) return;
  for (i = 0; i < len; i++) out[i] = '-';
  out[len] = '\0';
  pin_c = ctx->pins[pin_i].center;
  half = ctx->pins[pin_i].width * (double)len;
  if (half < 1.0) half = 1.0;
  center = (int)(pin_c * (double)(len - 1) + 0.5);
  if (center < 0) center = 0;
  if (center >= len) center = len - 1;
  hstart = center - (int)half;
  hend = center + (int)half;
  if (hstart < 0) hstart = 0;
  if (hend >= len) hend = len - 1;
  for (i = hstart; i <= hend; i++) out[i] = '=';
  if (ctx->pins[pin_i].set) {
    for (i = 0; i < len; i++)
      if (out[i] == '=') out[i] = '#';
  }
  if (active && !ctx->pins[pin_i].set) {
    cpos = (int)(ctx->cursor * (double)(len - 1) + 0.5);
    if (cpos < 0) cpos = 0;
    if (cpos >= len) cpos = len - 1;
    out[cpos] = '|';
  }
}

static int skill_rescue(LpCtx *ctx, double dist, int pin_i) {
  double roll, dc;
  int pass;
  double near = ctx->pins[pin_i].width + 0.05;
  if (dist > near) return 0;
  roll = mgt_rand01() * 100.0 + (double)ctx->skill * 0.5;
  dc = 70.0 + (double)ctx->difficulty * 10.0 + dist * 120.0;
  pass = roll >= dc;
  snprintf(ctx->last_check, sizeof ctx->last_check,
           "Skill check roll %.1f vs DC %.1f (%s)", roll, dc,
           pass ? "PASS" : "FAIL");
  if (pass) ctx->rescued_hits++;
  return pass;
}

static void lp_draw(void *vctx, MgtCanvas *c) {
  LpCtx *ctx = (LpCtx *)vctx;
  char meter[32];
  char line[96];
  char lane[80];
  int i;
  mgt_canvas_clear(c);
  mgt_canvas_box(c, 1, 1, c->w - 2, c->h - 2, "LOCKPICKING");
  snprintf(line, sizeof line, "Target: %s (%s)", ctx->target_name,
           ctx->target_dir);
  mgt_canvas_write(c, 4, 3, line);
  snprintf(line, sizeof line, "Pins %d/%d  Skill:%d/100",
           ctx->pin_index + 1 > ctx->pin_count ? ctx->pin_count
                                               : ctx->pin_index + 1,
           ctx->pin_count, ctx->skill);
  mgt_canvas_write(c, 4, 4, line);
  mgt_canvas_meter(meter, sizeof meter, 18, ctx->tension / 100.0);
  mgt_canvas_write(c, 4, 5, "Tension ");
  mgt_canvas_write(c, 13, 5, meter);
  mgt_canvas_meter(meter, sizeof meter, 18, ctx->pick_integrity / 100.0);
  mgt_canvas_write(c, 33, 5, " Pick ");
  mgt_canvas_write(c, 40, 5, meter);
  snprintf(meter, sizeof meter, "Focus:%d", ctx->focus_charges);
  mgt_canvas_write(c, 58, 5, meter);
  snprintf(line, sizeof line, "Tool: %s%s  Noise: %s (%.0f)", ctx->tool_label,
           ctx->has_tension_wrench ? " +Wrench" : "",
           noise_band_label(ctx->lock_noise), ctx->lock_noise);
  mgt_canvas_write(c, 4, 6, line);
  mgt_canvas_box(c, 4, 7, c->w - 8, 16, "PINS");
  for (i = 0; i < ctx->pin_count; i++) {
    char row[96];
    int active = (i == ctx->pin_index && !ctx->finished);
    const char *st = ctx->pins[i].set ? "[SET ]"
                      : active ? "[ACT ]"
                               : "[WAIT]";
    lane_string(lane, sizeof lane, ctx, i, active);
    snprintf(row, sizeof row, "Pin %d %s [%s]", i + 1, st, lane);
    mgt_canvas_write(c, 6, 9 + i * 3, row);
  }
  snprintf(line, sizeof line, "Checks: %s", ctx->last_check);
  mgt_canvas_write(c, 4, 24, line);
  snprintf(line, sizeof line, "Attempts:%d Misses:%d Rescues:%d", ctx->attempts,
           ctx->misses, ctx->rescued_hits);
  mgt_canvas_write(c, 4, 25, line);
  mgt_canvas_write(c, 4, 26, "Controls: SPACE set pin, F focus, ESC exit");
  {
    char status[96];
    snprintf(status, sizeof status, "Status: %s", ctx->message);
    mgt_canvas_write(c, 4, 27, status);
  }
}

static void lp_update(void *vctx, double dt) {
  LpCtx *ctx = (LpCtx *)vctx;
  double mult;
  double drift;
  if (ctx->finished) return;
  mult = ctx->focus_timer_ms > 0.0 ? 0.45 : 1.0;
  drift = ctx->speed * mult * dt;
  ctx->cursor += drift * (double)ctx->dir;
  if (ctx->cursor >= 1.0) {
    ctx->cursor = 1.0;
    ctx->dir = -1;
  } else if (ctx->cursor <= 0.0) {
    ctx->cursor = 0.0;
    ctx->dir = 1;
  }
  if (ctx->focus_timer_ms > 0.0)
    ctx->focus_timer_ms -= dt * 1000.0;
  if (ctx->focus_timer_ms < 0.0) ctx->focus_timer_ms = 0.0;
}

static void lp_finish(LpCtx *ctx, int success, const char *msg) {
  ctx->finished = 1;
  ctx->success = success;
  snprintf(ctx->message, sizeof ctx->message, "%s", msg ? msg : "");
  lp_store_exit(ctx);
}

static void lp_miss_message(LpCtx *ctx) {
  static const char *const lines[] = {
      "Your pick slipped! A loud scrape — someone might have noticed.",
      "The pick skates off the pin; metal clicks echo in the quiet.",
      "Slip! The pick chatters against brass — you freeze and listen.",
      "A clumsy slip sends a sharp ping down the hall. Footsteps?",
  };
  int n = (int)(sizeof lines / sizeof lines[0]);
  int pick = n > 0 ? mgt_rand_range(0, n - 1) : 0;
  snprintf(ctx->message, sizeof ctx->message, "%s", lines[pick]);
  snprintf(ctx->last_check, sizeof ctx->last_check, "Miss %d: timing slip",
           ctx->misses);
}

static double lp_miss_noise_delta(const LpCtx *ctx) {
  double d = 18.0 + (double)ctx->difficulty * 4.0;
  if (ctx->tool_id[0] && !strcmp(ctx->tool_id, "fine_lockpick")) d *= 0.55;
  else if (ctx->tool_id[0] && !strcmp(ctx->tool_id, "rusty_pick")) d *= 1.35;
  return d;
}

static double lp_hit_noise_delta(const LpCtx *ctx) {
  double d = 4.0;
  if (ctx->tool_id[0] && !strcmp(ctx->tool_id, "fine_lockpick")) d *= 0.5;
  else if (ctx->tool_id[0] && !strcmp(ctx->tool_id, "rusty_pick")) d *= 1.25;
  return d;
}

static int lp_key(void *vctx, MgtKey key, int ch) {
  LpCtx *ctx = (LpCtx *)vctx;
  int i;
  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) return -1;
  if (ctx->finished) {
    if (key == MGT_KEY_ENTER) return 1;
    return 0;
  }
  if (key == MGT_KEY_CHAR && (ch == 'f' || ch == 'F')) {
    if (ctx->focus_charges <= 0)
      snprintf(ctx->message, sizeof ctx->message,
               "No focus charges left.");
    else {
      ctx->focus_charges--;
      ctx->focus_timer_ms = 1500.0;
      snprintf(ctx->message, sizeof ctx->message,
               "Focus active. Cursor slowed briefly.");
    }
    return 0;
  }
  if (key != MGT_KEY_SPACE) return 0;
  ctx->attempts++;
  i = ctx->pin_index;
  if (i < 0 || i >= ctx->pin_count) return 0;
  {
    double dist = fabs(ctx->cursor - ctx->pins[i].center);
    int direct = dist <= ctx->pins[i].width;
    int rescued = !direct && skill_rescue(ctx, dist, i);
    if (direct || rescued) {
      ctx->pins[i].set = 1;
      ctx->pin_index++;
      ctx->speed += 0.055;
      ctx->lock_noise += lp_hit_noise_delta(ctx);
      if (ctx->lock_noise > 100.0) ctx->lock_noise = 100.0;
      if (direct)
        snprintf(ctx->last_check, sizeof ctx->last_check, "Direct timing hit.");
      snprintf(ctx->message, sizeof ctx->message, "Pin %d/%d set.",
               ctx->pin_index, ctx->pin_count);
      if (ctx->pin_index >= ctx->pin_count) {
        lp_finish(ctx, 1, "Lock opened.");
        return 0;
      }
      return 0;
    }
    ctx->misses++;
    ctx->tension += 14.0 + (double)ctx->difficulty * 4.0;
    ctx->pick_integrity -=
        7.0 + (double)ctx->difficulty * 3.0 + (double)ctx->rusty_wear;
    ctx->lock_noise += lp_miss_noise_delta(ctx);
    if (ctx->tension > 100.0) ctx->tension = 100.0;
    if (ctx->lock_noise > 100.0) ctx->lock_noise = 100.0;
    lp_miss_message(ctx);
    if (ctx->tension >= 100.0) {
      lp_finish(ctx, 0, "Lock tension maxed out. The mechanism jams.");
      return 0;
    }
    if (ctx->pick_integrity <= 0.0) {
      lp_finish(ctx, 0, "Your lockpick snaps.");
      return 0;
    }
  }
  return 0;
}

static void lp_setup_tools(LpCtx *ctx, MgtPersistentState *st) {
  ctx->tool_id[0] = '\0';
  if (st && st->has_fine_pick) {
    snprintf(ctx->tool_label, sizeof ctx->tool_label, "Fine Pick");
    snprintf(ctx->tool_id, sizeof ctx->tool_id, "fine_lockpick");
  } else if (st && st->has_basic_lockpick) {
    snprintf(ctx->tool_label, sizeof ctx->tool_label, "Lockpick");
    snprintf(ctx->tool_id, sizeof ctx->tool_id, "lockpick");
  } else {
    snprintf(ctx->tool_label, sizeof ctx->tool_label, "Rusty Pick");
    snprintf(ctx->tool_id, sizeof ctx->tool_id, "rusty_pick");
  }
  ctx->has_tension_wrench = st ? st->has_tension_wrench : 1;
  ctx->rusty_wear =
      (st && st->has_rusty_pick && !st->has_basic_lockpick && !st->has_fine_pick)
          ? 4
          : 0;
  ctx->pick_integrity = (ctx->rusty_wear > 0) ? 72.0 : 100.0;
  ctx->focus_charges = 2 + ((st && st->has_fine_pick) ? 1 : 0);
}

static void lp_setup_target(LpCtx *ctx, MgtPersistentState *st) {
  if (st && st->lock_target_name[0])
    snprintf(ctx->target_name, sizeof ctx->target_name, "%s",
             st->lock_target_name);
  else
    snprintf(ctx->target_name, sizeof ctx->target_name, "Shed Door");
  if (st && st->lock_target_dir[0])
    snprintf(ctx->target_dir, sizeof ctx->target_dir, "%s", st->lock_target_dir);
  else
    snprintf(ctx->target_dir, sizeof ctx->target_dir, "east");
}

int mg_run_lockpick(MgtSession *session) {
  LpCtx ctx;
  int i;
  double base_speed;
  MgtPersistentState *st = session ? session->state : NULL;
  memset(&ctx, 0, sizeof ctx);
  ctx.st = st;
  ctx.skill = st ? st->skill_engineering : 20;
  if (ctx.skill < 0) ctx.skill = 0;
  if (ctx.skill > 100) ctx.skill = 100;
  ctx.difficulty = st ? st->test_lock_difficulty : 1;
  if (ctx.difficulty < 1) ctx.difficulty = 1;
  if (ctx.difficulty > 2) ctx.difficulty = 2;
  lp_setup_target(&ctx, st);
  lp_setup_tools(&ctx, st);
  ctx.pin_count = ctx.difficulty >= 2 ? 4 : 3;
  base_speed =
      (ctx.difficulty >= 2 ? 0.8 : 0.6) +
      ((st && st->has_fine_pick) ? 0.03 : 0.0);
  for (i = 0; i < ctx.pin_count; i++) {
    double base_w =
        0.065 - (double)ctx.difficulty * 0.01 + (double)ctx.skill * 0.00025;
    ctx.pins[i].center = 0.1 + mgt_rand01() * 0.8;
    ctx.pins[i].width =
        base_w + (ctx.has_tension_wrench ? 0.012 : 0.0);
    if (st && st->has_fine_pick) ctx.pins[i].width += 0.008;
    if (ctx.pins[i].width < 0.025) ctx.pins[i].width = 0.025;
    if (ctx.pins[i].width > 0.09) ctx.pins[i].width = 0.09;
    ctx.pins[i].set = 0;
  }
  ctx.speed = base_speed;
  snprintf(ctx.message, sizeof ctx.message,
           "Line up the active pin and press SPACE.");
  snprintf(ctx.last_check, sizeof ctx.last_check, "No checks yet.");
  if (st && st->has_skeleton_key && ctx.difficulty <= 1) {
    snprintf(ctx.message, sizeof ctx.message,
             "Skeleton key bypass (weak lock) — no minigame needed in web.");
    mgt_host_message("Lockpicking", ctx.message);
    return 0;
  }
  if (mgt_autotest_script("lock_noise")) {
    ctx.misses = 2;
    ctx.lock_noise = 78.0;
    ctx.finished = 1;
    ctx.success = 0;
    snprintf(ctx.message, sizeof ctx.message,
             "Your pick slipped! A loud scrape — someone might have noticed.");
    lp_store_exit(&ctx);
    if (st) {
      st->last_success = 0;
      snprintf(st->last_banner, sizeof st->last_banner, "%s", ctx.message);
    }
    return 0;
  }
  if (mgt_autotest_script("lock_win")) {
    ctx.finished = 1;
    ctx.success = 1;
    ctx.lock_noise = 12.0;
    snprintf(ctx.message, sizeof ctx.message, "Lock opened.");
    lp_store_exit(&ctx);
    if (st) {
      st->last_success = 1;
      snprintf(st->last_banner, sizeof st->last_banner, "%s", ctx.message);
    }
    return 0;
  }
  mgt_run_loop(&ctx, lp_update, lp_draw, lp_key);
  if (st) {
    if (ctx.finished) {
      if (ctx.success) st->skill_engineering++;
      st->last_success = ctx.success ? 1 : 0;
      snprintf(st->last_banner, sizeof st->last_banner, "%s", ctx.message);
      lp_store_exit(&ctx);
    } else {
      st->last_success = -1;
      st->lock_exit_noise = (int)(ctx.lock_noise + 0.5);
      st->lock_exit_misses = ctx.misses;
      st->lock_pick_broken = 0;
      snprintf(st->lock_noise_band, sizeof st->lock_noise_band, "%s",
               noise_band_label(ctx.lock_noise));
      snprintf(st->last_banner, sizeof st->last_banner,
               "You abandon the lock.");
    }
  }
  return 0;
}
