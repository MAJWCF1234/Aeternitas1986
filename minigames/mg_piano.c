#include "mgt.h"
#include "mg_piano_audio.h"
#include "mg_piano_data.h"
#include "mgt_platform.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  PIANO_W = MGT_CANVAS_W,
  PIANO_H = MGT_CANVAS_H,
  PIANO_MAX_STEPS = 512,
  LANE_COUNT = 12,
  LANE_W = 7,
  LANE_X = 6,
  TOP_Y = 6,
  HIT_Y = 18,
  PIANO_SCR_PICK = 0,
  PIANO_SCR_STAGE = 1
};

typedef struct {
  unsigned fall_ms;
} PianoSpeedCfg;

static const PianoSpeedCfg k_speeds[] = {{4200}, {3000}, {2200}};

static const char *const k_speed_names[] = {"slow", "normal", "fast"};

static const char k_chromatic[] =
    "12!@345$%^89(0qQwWeErRtTyYuUiIoOpPaAsSdDfFgGhHjJkKlLzZxXcCvVnNm";

static const char *const k_lane_labels[LANE_COUNT] = {
    "1-2", "3-5", "6-7", "8-0", "q-w", "e-t",
    "y-i", "o-p", "a-d", "f-j", "k-z", "x-m"};

typedef struct {
  int screen;
  int song_index;
  int sheet_map[8];
  int sheet_count;
  int speed_index;
  char steps[PIANO_MAX_STEPS][16];
  int step_total;
  int step_index;
  char remaining[16];
  int rem_n;
  int running;
  int paused;
  int wait_hit;
  unsigned long step_started_at;
  int command_mode;
  char command[92];
  char history[92];
  unsigned long started_at;
  unsigned long pause_accum;
  unsigned long paused_at;
  int score;
  int combo;
  int hits;
  int misses;
  int bot;
  unsigned long bot_timer_ms;
  char flash[24];
  unsigned long flash_until_ms;
  char pressed[12];
  unsigned long pressed_until[12];
  int pressed_n;
} PianoCtx;

static int piano_key_index(char k) {
  const char *p = strchr(k_chromatic, k);
  if (!p) return -1;
  return (int)(p - k_chromatic);
}

static int piano_key_playable(char k) {
  return piano_key_index(k) >= 0;
}

static int parse_notation(const char *notation, char steps[][16], int max_steps) {
  const char *p = notation;
  int n = 0;
  if (!notation) return 0;
  while (*p && n < max_steps) {
    if (*p == '[') {
      const char *end = strchr(p + 1, ']');
      int j = 0;
      if (!end) break;
      for (p++; p < end && j < 15; p++) {
        if (piano_key_playable(*p)) steps[n][j++] = *p;
      }
      steps[n][j] = '\0';
      if (j > 0) n++;
      p = end + 1;
    } else if (piano_key_playable(*p)) {
      steps[n][0] = *p;
      steps[n][1] = '\0';
      n++;
      p++;
    } else {
      p++;
    }
  }
  return n;
}

static int lane_for_key(char key) {
  int idx = piano_key_index(key);
  if (idx < 0) return 0;
  return (idx * (LANE_COUNT - 1)) / ((int)strlen(k_chromatic) - 1);
}

static unsigned long game_now(const PianoCtx *ctx) {
  unsigned long now = mgt_now_ms();
  unsigned long pause_tail = 0;
  if (!ctx->started_at) return 0;
  if (ctx->paused && ctx->paused_at) pause_tail = now - ctx->paused_at;
  return now - ctx->started_at - ctx->pause_accum - pause_tail;
}

static void rem_set(PianoCtx *ctx, int step_i) {
  int i;
  ctx->rem_n = 0;
  if (step_i < 0 || step_i >= ctx->step_total) return;
  for (i = 0; ctx->steps[step_i][i] && ctx->rem_n < 15; i++) {
    int j, found = 0;
    for (j = 0; j < ctx->rem_n; j++)
      if (ctx->remaining[j] == ctx->steps[step_i][i]) found = 1;
    if (!found) ctx->remaining[ctx->rem_n++] = ctx->steps[step_i][i];
  }
}

static void flash_msg(PianoCtx *ctx, const char *msg) {
  snprintf(ctx->flash, sizeof ctx->flash, "%s", msg ? msg : "");
  ctx->flash_until_ms = mgt_now_ms() + 560;
}

static void note_pressed(PianoCtx *ctx, char key) {
  int i, oldest = 0;
  unsigned long until = mgt_now_ms() + 130;
  for (i = 0; i < ctx->pressed_n; i++) {
    if (ctx->pressed[i] == key) {
      ctx->pressed_until[i] = until;
      return;
    }
  }
  if (ctx->pressed_n < (int)(sizeof ctx->pressed / sizeof ctx->pressed[0])) {
    i = ctx->pressed_n++;
  } else {
    for (i = 1; i < ctx->pressed_n; i++)
      if (ctx->pressed_until[i] < ctx->pressed_until[oldest]) oldest = i;
    i = oldest;
  }
  ctx->pressed[i] = key;
  ctx->pressed_until[i] = until;
}

static int piano_data_index(const PianoCtx *ctx, int pick_i) {
  if (!ctx || pick_i < 0 || pick_i >= ctx->sheet_count) return 0;
  return ctx->sheet_map[pick_i];
}

static void setup_song(PianoCtx *ctx) {
  const char *notation =
      mg_piano_sheet_notation(piano_data_index(ctx, ctx->song_index));
  ctx->step_total = parse_notation(notation, ctx->steps, PIANO_MAX_STEPS);
  ctx->step_index = 0;
  ctx->wait_hit = 0;
  rem_set(ctx, 0);
}

static void begin_step_fall(PianoCtx *ctx) {
  if (ctx->step_index >= ctx->step_total) return;
  rem_set(ctx, ctx->step_index);
  ctx->wait_hit = 0;
  ctx->step_started_at = game_now(ctx);
}

static void advance_step(PianoCtx *ctx) {
  ctx->step_index++;
  if (ctx->step_index < ctx->step_total)
    begin_step_fall(ctx);
}

static void miss_step(PianoCtx *ctx, const char *msg) {
  if (ctx->step_index >= ctx->step_total) return;
  ctx->misses++;
  ctx->combo = 0;
  advance_step(ctx);
  flash_msg(ctx, msg);
  snprintf(ctx->history, sizeof ctx->history, "%s", msg);
}

static unsigned long step_fall_elapsed(const PianoCtx *ctx) {
  const PianoSpeedCfg *cfg = &k_speeds[ctx->speed_index];
  unsigned long e;
  if (!ctx->running || ctx->step_index >= ctx->step_total) return 0;
  if (ctx->wait_hit) return cfg->fall_ms;
  e = game_now(ctx) - ctx->step_started_at;
  if (e > cfg->fall_ms) return cfg->fall_ms;
  return e;
}

static void step_check_arrival(PianoCtx *ctx) {
  const PianoSpeedCfg *cfg = &k_speeds[ctx->speed_index];
  if (ctx->wait_hit || ctx->step_index >= ctx->step_total) return;
  if (game_now(ctx) - ctx->step_started_at >= cfg->fall_ms) {
    ctx->wait_hit = 1;
    flash_msg(ctx, "HIT!");
  }
}

static void play_step_keys(PianoCtx *ctx, const char *step) {
  int i;
  for (i = 0; step[i]; i++) {
    mg_piano_play_key(step[i]);
    note_pressed(ctx, step[i]);
  }
}

static void start_game(PianoCtx *ctx, int reset) {
  if (reset) {
    setup_song(ctx);
    ctx->score = 0;
    ctx->combo = 0;
    ctx->hits = 0;
    ctx->misses = 0;
    ctx->started_at = mgt_now_ms();
    ctx->pause_accum = 0;
    ctx->bot_timer_ms = 0;
    ctx->step_index = 0;
    begin_step_fall(ctx);
  } else if (ctx->paused) {
    ctx->pause_accum += mgt_now_ms() - ctx->paused_at;
  }
  ctx->running = 1;
  ctx->paused = 0;
  ctx->command_mode = 0;
  flash_msg(ctx, "GO!");
  snprintf(ctx->history, sizeof ctx->history, "playing");
}

static void pause_game(PianoCtx *ctx) {
  if (!ctx->running || ctx->paused) return;
  ctx->paused = 1;
  ctx->paused_at = mgt_now_ms();
  snprintf(ctx->flash, sizeof ctx->flash, "PAUSED");
  ctx->flash_until_ms = mgt_now_ms() + 9999999ul;
}

static void resume_game(PianoCtx *ctx) { start_game(ctx, 0); }

static void stop_performance(PianoCtx *ctx) {
  ctx->running = 0;
  ctx->paused = 0;
  ctx->started_at = 0;
  setup_song(ctx);
  snprintf(ctx->history, sizeof ctx->history, "ENTER to play again");
}

static void enter_stage(PianoCtx *ctx) {
  ctx->screen = PIANO_SCR_STAGE;
  ctx->running = 0;
  ctx->paused = 0;
  setup_song(ctx);
  snprintf(ctx->history, sizeof ctx->history, "ENTER to start  ESC back to songs");
}

static void handle_piano_input(PianoCtx *ctx, char key) {
  int in_rem = 0, ri;

  if (key == 'b' || key == 'B') {
    ctx->bot = !ctx->bot;
    flash_msg(ctx, ctx->bot ? "BOT ON" : "BOT OFF");
    return;
  }

  if (!piano_key_playable(key)) return;

  mg_piano_play_key(key);
  note_pressed(ctx, key);

  if (!ctx->running || ctx->paused || ctx->step_index >= ctx->step_total) return;

  if (!ctx->wait_hit) {
    flash_msg(ctx, "WAIT...");
    return;
  }

  for (ri = 0; ri < ctx->rem_n; ri++)
    if (ctx->remaining[ri] == key) in_rem = 1;

  if (in_rem) {
    for (ri = 0; ri < ctx->rem_n; ri++) {
      if (ctx->remaining[ri] == key) {
        for (; ri < ctx->rem_n - 1; ri++)
          ctx->remaining[ri] = ctx->remaining[ri + 1];
        ctx->rem_n--;
        break;
      }
    }
    if (ctx->rem_n == 0) {
      ctx->hits++;
      ctx->combo++;
      ctx->score += 100 + ctx->combo;
      advance_step(ctx);
      if (ctx->step_index >= ctx->step_total) {
        ctx->running = 0;
        flash_msg(ctx, "COMPLETE");
        snprintf(ctx->history, sizeof ctx->history, "done - ENTER retry");
      } else {
        flash_msg(ctx, "GOOD");
      }
    } else {
      flash_msg(ctx, "CHORD");
    }
  } else {
    miss_step(ctx, "BAD");
  }
}

static void execute_command(PianoCtx *ctx, const char *raw) {
  char lower[92];
  size_t i, n;

  if (!raw || !raw[0]) return;

  n = strlen(raw);
  if (n >= sizeof lower) n = sizeof lower - 1;
  for (i = 0; i < n; i++) lower[i] = (char)tolower((unsigned char)raw[i]);
  lower[n] = '\0';

  if (!strcmp(lower, "help")) {
    snprintf(ctx->history, sizeof ctx->history, "ENTER start  ESC songs  SPEED keys");
  } else if (!strcmp(lower, "start") || !strcmp(lower, "play")) {
    if (ctx->screen == PIANO_SCR_STAGE) start_game(ctx, 1);
  } else if (!strcmp(lower, "pause")) {
    pause_game(ctx);
  } else if (!strcmp(lower, "resume")) {
    resume_game(ctx);
  } else if (!strcmp(lower, "reset") || !strcmp(lower, "restart")) {
    stop_performance(ctx);
    flash_msg(ctx, "RESET");
  } else if (!strcmp(lower, "bot")) {
    ctx->bot = !ctx->bot;
    flash_msg(ctx, ctx->bot ? "BOT ON" : "BOT OFF");
  } else if (!strncmp(lower, "speed ", 6)) {
    const char *arg = lower + 6;
    int si;
    for (si = 0; si < 3; si++) {
      if (!strcmp(arg, k_speed_names[si])) {
        ctx->speed_index = si;
        if (ctx->screen == PIANO_SCR_STAGE) setup_song(ctx);
        snprintf(ctx->history, sizeof ctx->history, "speed %s", k_speed_names[si]);
        return;
      }
    }
  }
}

static void piano_tick(PianoCtx *ctx) {
  unsigned long now;
  int i;

  now = mgt_now_ms();
  for (i = 0; i < ctx->pressed_n; i++) {
    if (now > ctx->pressed_until[i]) {
      ctx->pressed[i] = ctx->pressed[ctx->pressed_n - 1];
      ctx->pressed_until[i] = ctx->pressed_until[ctx->pressed_n - 1];
      ctx->pressed_n--;
      i--;
    }
  }

  if (ctx->screen != PIANO_SCR_STAGE || ctx->paused || !ctx->running) return;

  step_check_arrival(ctx);

  if (ctx->bot && ctx->wait_hit && ctx->step_index < ctx->step_total &&
      now - ctx->bot_timer_ms > 400) {
    ctx->bot_timer_ms = now;
    play_step_keys(ctx, ctx->steps[ctx->step_index]);
    ctx->hits++;
    ctx->combo++;
    ctx->score += 100 + ctx->combo;
    advance_step(ctx);
    flash_msg(ctx, "BOT");
    if (ctx->step_index >= ctx->step_total) {
      ctx->running = 0;
      flash_msg(ctx, "COMPLETE");
      snprintf(ctx->history, sizeof ctx->history, "done - ENTER retry");
    }
  }
}

static void draw_center(MgtCanvas *c, int y, const char *text) {
  int x = (c->w - (int)strlen(text)) / 2;
  if (x < 0) x = 0;
  mgt_canvas_write(c, x, y, text);
}

static void draw_hline(MgtCanvas *c, int y, int x1, int x2, char ch) {
  int x;
  char buf[2] = {ch, '\0'};
  if (y < 0 || y >= c->h) return;
  if (x1 < 0) x1 = 0;
  if (x2 >= c->w) x2 = c->w - 1;
  for (x = x1; x <= x2; x++) mgt_canvas_write(c, x, y, buf);
}

static void draw_pick_screen(PianoCtx *ctx, MgtCanvas *c) {
  char line[96];
  int i;

  mgt_canvas_clear(c);
  mgt_canvas_box(c, 0, 0, PIANO_W, 6, " TAVERN PIANO - SONG SELECT ");
  mgt_canvas_write(c, 2, 2, "Up/Down pick song   Left/Right speed   Enter confirm");
  mgt_canvas_write(c, 2, 3, "Esc exit minigame");

  snprintf(line, sizeof line, "SPEED: %s", k_speed_names[ctx->speed_index]);
  mgt_canvas_write(c, 2, 4, line);

  mgt_canvas_box(c, 4, 7, 88, 14, " SHEETS ");
  for (i = 0; i < ctx->sheet_count; i++) {
    int di = piano_data_index(ctx, i);
    snprintf(line, sizeof line, "%c %d  %-40s  (%s)",
             (i == ctx->song_index) ? '>' : ' ', i, mg_piano_sheet_title(di),
             mg_piano_sheet_diff(di));
    mgt_canvas_write(c, 6, 9 + i, line);
  }

  snprintf(line, sizeof line, "Steps in song: %d", ctx->step_total);
  mgt_canvas_write(c, 4, 22, line);
  draw_center(c, 24, "Press ENTER to open piano stage");
}

static int step_note_draw_y(const PianoCtx *ctx, int fall_h) {
  const PianoSpeedCfg *cfg = &k_speeds[ctx->speed_index];
  unsigned long e = step_fall_elapsed(ctx);
  double progress;
  if (cfg->fall_ms == 0) return TOP_Y + fall_h;
  progress = (double)e / (double)cfg->fall_ms;
  if (progress >= 1.0) return TOP_Y + fall_h;
  return TOP_Y + 1 + (int)(progress * (double)fall_h + 0.5);
}

static void draw_stage_screen(PianoCtx *ctx, MgtCanvas *c) {
  char line[96];
  int acc, li, i;
  const char *state;
  const int fall_h = HIT_Y - TOP_Y - 1;

  mgt_canvas_clear(c);

  mgt_canvas_box(c, 0, 0, PIANO_W, 6, " ASCII TAVERN PIANO ");
  acc = (ctx->hits + ctx->misses) ? (ctx->hits * 100) / (ctx->hits + ctx->misses) : 100;
  if (ctx->paused) state = "PAUSED";
  else if (ctx->running) state = "PLAYING";
  else state = "READY";

  snprintf(line, sizeof line,
           "STATE %-7s SONG %-24s SPEED %-6s SCORE %06d",
           state, mg_piano_sheet_title(piano_data_index(ctx, ctx->song_index)),
           k_speed_names[ctx->speed_index],
           ctx->score);
  mgt_canvas_write(c, 2, 1, line);

  snprintf(line, sizeof line,
           "COMBO %3d HITS %3d MISSES %3d ACC %3d%% STEP %3d/%3d  BOT %s",
           ctx->combo, ctx->hits, ctx->misses, acc,
           ctx->step_index < ctx->step_total ? ctx->step_index + 1 : ctx->step_total,
           ctx->step_total, ctx->bot ? "ON" : "OFF");
  mgt_canvas_write(c, 2, 2, line);

  line[0] = '\0';
  snprintf(line, sizeof line, "NOW ");
  if (ctx->running && ctx->step_index < ctx->step_total && ctx->steps[ctx->step_index][0]) {
    char tmp[20];
    snprintf(tmp, sizeof tmp, "[%s]", ctx->steps[ctx->step_index]);
    strncat(line, tmp, sizeof line - strlen(line) - 1);
  } else {
    strncat(line, "---", sizeof line - strlen(line) - 1);
  }
  strncat(line, "  HIT ", sizeof line - strlen(line) - 1);
  if (ctx->rem_n) {
    for (i = 0; i < ctx->rem_n; i++) {
      size_t L = strlen(line);
      if (L + 2 >= sizeof line) break;
      line[L] = ctx->remaining[i];
      line[L + 1] = '\0';
    }
  } else {
    strncat(line, "---", sizeof line - strlen(line) - 1);
  }
  mgt_canvas_write(c, 2, 3, line);

  if (ctx->running && ctx->wait_hit)
    snprintf(line, sizeof line, "At HIT line — play keys shown, then next note falls");
  else if (ctx->running)
    snprintf(line, sizeof line, "Note falling... waits at === HIT === until you play it");
  else if (ctx->paused)
    snprintf(line, sizeof line, "PAUSED - Esc to resume");
  else
    snprintf(line, sizeof line, "ENTER start   Esc songs   B bot   : commands");
  mgt_canvas_write(c, 2, 4, line);

  mgt_canvas_box(c, 0, 6, PIANO_W, 14, " FALLING NOTES ");
  for (li = 0; li < LANE_COUNT; li++) {
    int x = LANE_X + li * LANE_W;
    snprintf(line, sizeof line, "%-5s", k_lane_labels[li]);
    mgt_canvas_write(c, x, TOP_Y, line);
    for (i = TOP_Y + 1; i < HIT_Y; i++) {
      mgt_canvas_write(c, x, i, "|");
      if (x + LANE_W - 1 < c->w) mgt_canvas_write(c, x + LANE_W - 1, i, "|");
    }
  }
  draw_hline(c, HIT_Y, 2, PIANO_W - 3, '=');
  draw_center(c, HIT_Y, " HIT ");

  if (ctx->running && !ctx->paused && ctx->step_index < ctx->step_total) {
    const char *step = ctx->steps[ctx->step_index];
    int y = step_note_draw_y(ctx, fall_h);
    int chord = (int)strlen(step) > 1;
    if (y > HIT_Y) y = HIT_Y;
    for (i = 0; step[i]; i++) {
      int x = LANE_X + lane_for_key(step[i]) * LANE_W + 2;
      char note[8];
      if (chord)
        snprintf(note, sizeof note, "[%c]", step[i]);
      else
        snprintf(note, sizeof note, " %c ", step[i]);
      mgt_canvas_write(c, x, y, note);
    }
    if (ctx->wait_hit) draw_center(c, TOP_Y + 2, ">> PLAY NOW <<");
  } else if (!ctx->running) {
    draw_center(c, 12, "Press ENTER to begin");
  }

  if (mgt_now_ms() < ctx->flash_until_ms) draw_center(c, 13, ctx->flash);

  mgt_canvas_box(c, 0, 20, PIANO_W, 6, " VP MAP ");
  mgt_canvas_write(c, 2, 21,
                   "BLACK: ! @   $ % ^   * (    Q W E   T Y   I O P    A S D   G H J");
  mgt_canvas_write(c, 2, 22,
                   "WHITE: 1 2 3 4 5 6 7 8 9 0  q w e r t y u i o p    a s d f g h j k l");
  mgt_canvas_write(c, 2, 23, "z x c v n m  |  b/B=bot  |  VP note B is n");
  line[0] = '\0';
  snprintf(line, sizeof line, "DOWN:");
  for (i = 0; i < ctx->pressed_n; i++) {
    size_t L = strlen(line);
    if (L + 2 >= sizeof line) break;
    line[L] = ' ';
    line[L + 1] = ctx->pressed[i];
    line[L + 2] = '\0';
  }
  mgt_canvas_write(c, 2, 24, line);

  mgt_canvas_box(c, 0, 26, PIANO_W, 4, " PROMPT ");
  if (ctx->command_mode)
    snprintf(line, sizeof line, ":%s_", ctx->command);
  else
    snprintf(line, sizeof line, ">%s", ctx->history);
  mgt_canvas_write(c, 2, 27, line);
  mgt_canvas_write(c, 2, 28, ctx->command_mode ? "ENTER run cmd  ESC cancel" : "");
}

static void piano_draw(void *vctx, MgtCanvas *c) {
  PianoCtx *ctx = (PianoCtx *)vctx;
  if (ctx->screen == PIANO_SCR_PICK)
    draw_pick_screen(ctx, c);
  else
    draw_stage_screen(ctx, c);
}

static void piano_update(void *vctx, double dt) {
  (void)dt;
  piano_tick((PianoCtx *)vctx);
}

static int piano_key(void *vctx, MgtKey key, int ch) {
  PianoCtx *ctx = (PianoCtx *)vctx;

  if (ctx->command_mode && ctx->screen == PIANO_SCR_STAGE) {
    if (key == MGT_KEY_ESC) {
      ctx->command_mode = 0;
      ctx->command[0] = '\0';
      return 0;
    }
    if (key == MGT_KEY_ENTER) {
      char cmd[92];
      snprintf(cmd, sizeof cmd, "%s", ctx->command);
      ctx->command_mode = 0;
      ctx->command[0] = '\0';
      execute_command(ctx, cmd);
      return 0;
    }
    if (key == MGT_KEY_CHAR && ch == 8) {
      size_t L = strlen(ctx->command);
      if (L > 0) ctx->command[L - 1] = '\0';
      return 0;
    }
    if (key == MGT_KEY_CHAR && ch >= 32 && ch < 127 &&
        strlen(ctx->command) < sizeof ctx->command - 1) {
      size_t L = strlen(ctx->command);
      ctx->command[L] = (char)ch;
      ctx->command[L + 1] = '\0';
      return 0;
    }
    return 0;
  }

  if (ctx->screen == PIANO_SCR_PICK) {
    if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) return -1;
    if (key == MGT_KEY_UP)
      ctx->song_index = (ctx->song_index - 1 + ctx->sheet_count) % ctx->sheet_count;
    else if (key == MGT_KEY_DOWN)
      ctx->song_index = (ctx->song_index + 1) % ctx->sheet_count;
    else if (key == MGT_KEY_LEFT)
      ctx->speed_index = (ctx->speed_index + 2) % 3;
    else if (key == MGT_KEY_RIGHT)
      ctx->speed_index = (ctx->speed_index + 1) % 3;
    else if (key == MGT_KEY_ENTER) {
      setup_song(ctx);
      enter_stage(ctx);
      return 0;
    }
    if (key == MGT_KEY_UP || key == MGT_KEY_DOWN || key == MGT_KEY_LEFT ||
        key == MGT_KEY_RIGHT)
      setup_song(ctx);
    return 0;
  }

  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) {
    if (ctx->paused) {
      resume_game(ctx);
      return 0;
    }
    if (ctx->running) {
      pause_game(ctx);
      return 0;
    }
    ctx->screen = PIANO_SCR_PICK;
    ctx->running = 0;
    setup_song(ctx);
    return 0;
  }

  if (key == MGT_KEY_CHAR && ch == ':' && !ctx->running) {
    ctx->command_mode = 1;
    ctx->command[0] = '\0';
    return 0;
  }

  if (!ctx->running) {
    if (key == MGT_KEY_ENTER) {
      start_game(ctx, 1);
      return 0;
    }
    if (key == MGT_KEY_CHAR && ch >= 32 && ch < 127) {
      mg_piano_play_key((char)ch);
      note_pressed(ctx, (char)ch);
      return 0;
    }
    return 0;
  }

  if (key == MGT_KEY_CHAR && ch >= 32 && ch < 127) {
    handle_piano_input(ctx, (char)ch);
    return 0;
  }

  return 0;
}

int mg_run_piano(MgtSession *session) {
  PianoCtx ctx;
  MgtPersistentState *st = session ? session->state : NULL;

  mg_piano_audio_init();
  memset(&ctx, 0, sizeof ctx);
  {
    int i, n = mg_piano_sheet_count();
    unsigned owned = st ? (unsigned)st->piano_owned : 7u;
    if (owned == 0) owned = 1u;
    for (i = 0; i < n && ctx.sheet_count < 8; i++) {
      if (owned & (1u << i)) ctx.sheet_map[ctx.sheet_count++] = i;
    }
    if (ctx.sheet_count == 0) {
      ctx.sheet_map[0] = 0;
      ctx.sheet_count = 1;
    }
  }
  ctx.speed_index = 1;
  ctx.screen = PIANO_SCR_PICK;
  setup_song(&ctx);

  mgt_run_loop(&ctx, piano_update, piano_draw, piano_key);

  mg_piano_audio_shutdown();
  if (st) {
    st->last_success = ctx.score > 0 ? 1 : 0;
    snprintf(st->last_banner, sizeof st->last_banner,
             "Performance done. Score %d (combo x%d).", ctx.score, ctx.combo);
  }
  return 0;
}
