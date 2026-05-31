#include "mgt.h"
#include "mgt_platform.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
  COOK_SCR_PREP = 0,
  COOK_SCR_GRILL_WAIT = 1,
  COOK_SCR_GRILL = 2,
  COOK_SCR_RESULT = 3,
  COOK_SCR_GAMEOVER = 4
};

enum {
  COOK_TICKET_X = 2,
  COOK_TICKET_Y = 2,
  COOK_BOARD_X = 30,
  COOK_BOARD_Y = 8,
  COOK_PANTRY_X = 52,
  COOK_PANTRY_Y = 2,
  COOK_GRILL_X = 20,
  COOK_GRILL_Y = 5,
  COOK_BAR_X = 21,
  COOK_BAR_Y = 15,
  COOK_BAR_W = 50,
  COOK_RESULT_MS = 2000,
  COOK_GRILL_DELAY_MS = 500,
  COOK_INPUT_MS = 100
};

typedef struct {
  const char *name;
  char id;
  const char *icon;
} Ingredient;

typedef struct {
  const char *name;
  const char *ids;
  int diff;
  int score;
} Recipe;

static const Ingredient k_ing[] = {
    {"Burger Bun", '1', "( )"},   {"Beef Patty", '2', "[=]"},
    {"Cheese", '3', "< >"},       {"Tomato", '4', "(@)"},
    {"Lettuce", '5', "{~}"},      {"Potato", '6', "00"}};

static const Recipe k_rec[] = {
    {"Classic Burger", "125", 1, 100},
    {"Cheeseburger", "123", 2, 150},
    {"Garden Salad", "45", 1, 120},
    {"Steak & Chips", "26", 3, 300},
    {"Pizza Slice", "134", 2, 200},
    {"Loaded Fries", "632", 3, 250},
    {"BLT Sandwich", "1254", 4, 350}};

typedef struct {
  MgtPersistentState *st;
  int screen;
  int recipe_i;
  char picked[8];
  int pick_n;
  int lives;
  int score;
  int streak;
  int total_earnings;
  double cook_progress;
  int cook_dir;
  int target_start;
  int target_width;
  double cook_speed;
  char feedback[96];
  int tick;
  unsigned long result_until_ms;
  unsigned long grill_until_ms;
  unsigned long last_input_ms;
} CookCtx;

static void cook_put(MgtCanvas *c, int x, int y, char ch) {
  if (!c || x < 0 || x >= c->w || y < 0 || y >= c->h) return;
  c->cells[y * c->w + x] = ch;
}

static int cmp_char(const void *a, const void *b) {
  return (*(const char *)a - *(const char *)b);
}

static int cook_skill(const CookCtx *c) {
  int skill = c->st ? c->st->skill_cooking : 20;
  if (skill < 5) skill = 5;
  if (skill > 100) skill = 100;
  return skill;
}

static double cook_speed_for_skill(int skill) {
  return 1.0 - (skill - 20) * 0.0035;
}

static int cook_zone_bonus(int skill) {
  if (skill >= 80) return 3;
  if (skill >= 50) return 2;
  if (skill >= 30) return 1;
  return 0;
}

static int recipe_ok(const CookCtx *c) {
  const Recipe *r = &k_rec[c->recipe_i];
  char need[8], have[8];
  int i;
  if (c->pick_n != (int)strlen(r->ids)) return 0;
  for (i = 0; r->ids[i]; i++) need[i] = r->ids[i];
  need[i] = '\0';
  for (i = 0; i < c->pick_n; i++) have[i] = c->picked[i];
  have[c->pick_n] = '\0';
  qsort(need, strlen(need), 1, cmp_char);
  qsort(have, strlen(have), 1, cmp_char);
  return strcmp(need, have) == 0;
}

static void new_order(CookCtx *c) {
  int skill = cook_skill(c);
  c->recipe_i = mgt_rand_range(0, (int)(sizeof k_rec / sizeof k_rec[0]) - 1);
  c->pick_n = 0;
  c->screen = COOK_SCR_PREP;
  c->cook_progress = 0;
  c->cook_dir = 1;
  c->cook_speed = cook_speed_for_skill(skill);
  c->target_width = 20 - k_rec[c->recipe_i].diff * 2 + cook_zone_bonus(skill);
  if (c->target_width < 6) c->target_width = 6;
  if (c->target_width > 24) c->target_width = 24;
  c->target_start = 30 + mgt_rand_range(0, 40);
  c->result_until_ms = 0;
  c->grill_until_ms = 0;
  snprintf(c->feedback, sizeof c->feedback, "NEW ORDER UP!");
}

static void fail_order(CookCtx *c) {
  c->lives--;
  c->streak = 0;
  if (c->lives <= 0) {
    c->screen = COOK_SCR_GAMEOVER;
    c->result_until_ms = 0;
  } else {
    c->screen = COOK_SCR_RESULT;
    c->result_until_ms = mgt_now_ms() + COOK_RESULT_MS;
    if (!c->feedback[0]) snprintf(c->feedback, sizeof c->feedback, "Order failed.");
  }
}

static void success_order(CookCtx *c, double mult) {
  int pts;
  c->streak++;
  pts = (int)(k_rec[c->recipe_i].score * mult * (1.0 + c->streak * 0.1));
  c->score += pts;
  c->total_earnings += pts / 10;
  if (mult >= 1.5)
    snprintf(c->feedback, sizeof c->feedback, "PERFECT GRILL! CUSTOMER LOVED IT!");
  else if (mult >= 1.0)
    snprintf(c->feedback, sizeof c->feedback, "Order up!");
  else
    snprintf(c->feedback, sizeof c->feedback, "A bit raw, but edible.");
  c->screen = COOK_SCR_RESULT;
  c->result_until_ms = mgt_now_ms() + COOK_RESULT_MS;
}

static void draw_lives(MgtCanvas *c, int x, int y, int lives) {
  char buf[8];
  int i;
  buf[0] = '\0';
  for (i = 0; i < 3 - lives; i++) strcat(buf, "X");
  for (i = 0; i < lives; i++) strcat(buf, "H");
  mgt_canvas_write(c, x, y, buf);
}

static void draw_header_rule(MgtCanvas *c) {
  int x;
  for (x = 0; x < c->w; x++) cook_put(c, x, 1, '-');
}

static void render_prep(CookCtx *x, MgtCanvas *c) {
  char line[96];
  char upper[48];
  int i;

  mgt_canvas_box(c, COOK_TICKET_X, COOK_TICKET_Y, 25, 10, "ORDER TICKET");
  for (i = 0; k_rec[x->recipe_i].name[i] && i < (int)sizeof upper - 1; i++)
    upper[i] = (char)toupper((unsigned char)k_rec[x->recipe_i].name[i]);
  upper[i] = '\0';
  mgt_canvas_write(c, 4, 4, upper);
  mgt_canvas_write(c, 4, 6, "NEEDS:");
  for (i = 0; k_rec[x->recipe_i].ids[i]; i++) {
    int j;
    for (j = 0; j < 6; j++)
      if (k_ing[j].id == k_rec[x->recipe_i].ids[i]) {
        snprintf(line, sizeof line, "- %s", k_ing[j].name);
        mgt_canvas_write(c, 6, 7 + i, line);
        break;
      }
  }

  mgt_canvas_box(c, COOK_BOARD_X, COOK_BOARD_Y, 20, 12, "CUTTING BOARD");
  for (i = 0; i < x->pick_n; i++) {
    int j;
    for (j = 0; j < 6; j++)
      if (k_ing[j].id == x->picked[i]) {
        snprintf(line, sizeof line, "%s %s", k_ing[j].icon, k_ing[j].name);
        mgt_canvas_write(c, 32, 10 + i * 2, line);
      }
  }
  snprintf(line, sizeof line, "Items: %d", x->pick_n);
  mgt_canvas_write(c, 32, 18, line);

  mgt_canvas_box(c, COOK_PANTRY_X, COOK_PANTRY_Y, 26, 18, "PANTRY");
  for (i = 0; i < 6; i++) {
    int sel = 0, j;
    for (j = 0; j < x->pick_n; j++)
      if (x->picked[j] == k_ing[i].id) sel = 1;
    snprintf(line, sizeof line, "[%c] %s %s", k_ing[i].id, k_ing[i].icon,
             k_ing[i].name);
    mgt_canvas_write(c, 54, 4 + i * 2, line);
    if (sel) mgt_canvas_write(c, 75, 4 + i * 2, "<");
  }
  mgt_canvas_write(c, 2, 23, "CONTROLS: Press 1-6 to add ingredient. BACKSPACE to undo.");
}

static void render_grill(CookCtx *x, MgtCanvas *c) {
  char bar[64], line[96];
  int ts, tw, pos, i;

  mgt_canvas_write(c, COOK_GRILL_X, COOK_GRILL_Y, "      ______________________________");
  mgt_canvas_write(c, COOK_GRILL_X, COOK_GRILL_Y + 1,
                   "     /                              \\");
  mgt_canvas_write(c, COOK_GRILL_X, COOK_GRILL_Y + 2,
                   "    |   [||||]     [||||]     [||||] |");
  mgt_canvas_write(c, COOK_GRILL_X, COOK_GRILL_Y + 3,
                   "    |                                |");
  mgt_canvas_write(c, COOK_GRILL_X, COOK_GRILL_Y + 4,
                   "    |________________________________|");
  mgt_canvas_write(c, COOK_GRILL_X + 10, COOK_GRILL_Y + 5,
                   (x->tick / 10) % 2 ? " ( ) ( ) ( ) " : "  (   )   (  ");
  snprintf(line, sizeof line, "GRILLING: %s", k_rec[x->recipe_i].name);
  mgt_canvas_write(c, 30, 12, line);

  ts = (x->target_start * COOK_BAR_W + 50) / 100;
  tw = (x->target_width * COOK_BAR_W + 99) / 100;
  if (tw < 1) tw = 1;
  if (ts + tw > COOK_BAR_W) ts = COOK_BAR_W - tw;
  memset(bar, '-', (size_t)COOK_BAR_W);
  bar[COOK_BAR_W] = '\0';
  for (i = 0; i < tw; i++) bar[ts + i] = '=';
  pos = (int)((x->cook_progress / 100.0) * (double)COOK_BAR_W);
  if (pos < 0) pos = 0;
  if (pos >= COOK_BAR_W) pos = COOK_BAR_W - 1;
  bar[pos] = '|';
  snprintf(line, sizeof line, "COLD [%s] HOT", bar);
  mgt_canvas_write(c, 15, COOK_BAR_Y, line);
  mgt_canvas_write(c, COOK_BAR_X + ts, COOK_BAR_Y + 1, "^^ ZONE ^^");
  cook_put(c, COOK_BAR_X + pos, COOK_BAR_Y - 1, 'V');
  mgt_canvas_write(c, 25, 20, "PRESS [SPACE] WHEN MARKER IS IN ZONE!");
}

static void cook_draw(void *vctx, MgtCanvas *c) {
  CookCtx *x = (CookCtx *)vctx;
  char line[96];

  mgt_canvas_clear(c);
  if (x->screen != COOK_SCR_GAMEOVER) {
    snprintf(line, sizeof line, "SCORE: %d  STREAK: x%d  EARNINGS: $%d",
             x->score, x->streak, x->total_earnings);
    mgt_canvas_write(c, 2, 0, line);
    snprintf(line, sizeof line, "LIVES: ");
    mgt_canvas_write(c, 60, 0, line);
    draw_lives(c, 67, 0, x->lives);
    draw_header_rule(c);
  }

  if (x->screen == COOK_SCR_GAMEOVER) {
    mgt_canvas_box(c, 25, 8, 30, 9, "GAME OVER");
    mgt_canvas_write(c, 30, 11, "KITCHEN CLOSED!");
    snprintf(line, sizeof line, "FINAL SCORE: %d", x->score);
    mgt_canvas_write(c, 28, 13, line);
    snprintf(line, sizeof line, "TOTAL EARNINGS: $%d", x->total_earnings);
    mgt_canvas_write(c, 27, 15, line);
    mgt_canvas_write(c, 27, 16, "PRESS [SPACE] TO RESTART");
    mgt_canvas_write(c, 27, 17, "PRESS [ESC] TO EXIT");
    return;
  }

  if (x->screen == COOK_SCR_PREP || x->screen == COOK_SCR_RESULT ||
      x->screen == COOK_SCR_GRILL_WAIT)
    render_prep(x, c);
  if (x->screen == COOK_SCR_GRILL) render_grill(x, c);

  mgt_canvas_write(c, 20, 22, x->feedback);
  if (x->screen == COOK_SCR_PREP)
    mgt_canvas_write(c, 2, 24, "1-6 add  BACKSPACE undo  ENTER check  ESC quit");
  else if (x->screen == COOK_SCR_RESULT)
    mgt_canvas_write(c, 2, 24, "ENTER next order (auto in 2s)");
  else if (x->screen == COOK_SCR_GRILL_WAIT)
    mgt_canvas_write(c, 2, 24, "Heading to the grill...");
  else if (x->screen == COOK_SCR_GRILL)
    mgt_canvas_write(c, 2, 24, "SPACE to stop the bar in the zone  ESC quit");
}

static void cook_update(void *vctx, double dt) {
  CookCtx *x = (CookCtx *)vctx;
  unsigned long now = mgt_now_ms();

  x->tick++;
  if (x->screen == COOK_SCR_GRILL_WAIT && x->grill_until_ms &&
      now >= x->grill_until_ms) {
    x->screen = COOK_SCR_GRILL;
    snprintf(x->feedback, sizeof x->feedback, "STOP THE BAR IN THE GREEN ZONE!");
    x->grill_until_ms = 0;
  }

  if (x->screen == COOK_SCR_RESULT && x->result_until_ms && now >= x->result_until_ms) {
    new_order(x);
    return;
  }

  if (x->screen != COOK_SCR_GRILL) return;

  x->cook_progress += x->cook_dir * x->cook_speed * dt * 60.0;
  if (x->cook_progress >= 100.0) {
    x->cook_progress = 100.0;
    x->cook_dir = -1;
  } else if (x->cook_progress <= 0.0) {
    x->cook_progress = 0.0;
    x->cook_dir = 1;
  }
}

static void check_prep(CookCtx *x) {
  const Recipe *r = &k_rec[x->recipe_i];
  if (x->pick_n > (int)strlen(r->ids)) {
    snprintf(x->feedback, sizeof x->feedback, "Too much junk! DISCARDED!");
    fail_order(x);
    return;
  }
  if (x->pick_n == (int)strlen(r->ids)) {
    if (recipe_ok(x)) {
      snprintf(x->feedback, sizeof x->feedback, "Prep complete! TO THE GRILL!");
      x->screen = COOK_SCR_GRILL_WAIT;
      x->grill_until_ms = mgt_now_ms() + COOK_GRILL_DELAY_MS;
    } else {
      snprintf(x->feedback, sizeof x->feedback, "Wrong ingredients! DISCARDED!");
      fail_order(x);
    }
  }
}

static int input_ok(CookCtx *x) {
  unsigned long now = mgt_now_ms();
  if (x->last_input_ms && now - x->last_input_ms < COOK_INPUT_MS) return 0;
  x->last_input_ms = now;
  return 1;
}

static int cook_key(void *vctx, MgtKey key, int ch) {
  CookCtx *x = (CookCtx *)vctx;
  int i;

  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) return -1;

  if (x->screen == COOK_SCR_GAMEOVER) {
    if (key == MGT_KEY_SPACE) {
      x->score = 0;
      x->lives = 3;
      x->streak = 0;
      x->total_earnings = 0;
      new_order(x);
    }
    return 0;
  }

  if (x->screen == COOK_SCR_RESULT && key == MGT_KEY_ENTER) {
    new_order(x);
    return 0;
  }

  if (x->screen == COOK_SCR_PREP) {
    if (key == MGT_KEY_CHAR && ch >= '1' && ch <= '6') {
      int found = 0;
      if (!input_ok(x)) return 0;
      for (i = 0; i < x->pick_n; i++)
        if (x->picked[i] == (char)ch) found = 1;
      if (!found && x->pick_n < 7) {
        x->picked[x->pick_n++] = (char)ch;
        check_prep(x);
      }
    }
    if (key == MGT_KEY_CHAR && ch == 8 && x->pick_n > 0) {
      if (!input_ok(x)) return 0;
      x->pick_n--;
    }
    if (key == MGT_KEY_ENTER) check_prep(x);
    return 0;
  }

  if (x->screen == COOK_SCR_GRILL && key == MGT_KEY_SPACE) {
    double p = x->cook_progress;
    double start = (double)x->target_start;
    double end = start + (double)x->target_width;
    double center = start + (double)x->target_width / 2.0;
    double dist;
    if (p >= start && p <= end) {
      dist = center - p;
      if (dist < 0) dist = -dist;
      if (dist < x->target_width / 4.0)
        success_order(x, 1.5);
      else
        success_order(x, 1.0);
    } else {
      snprintf(x->feedback, sizeof x->feedback, "BURNT TO A CRISP!");
      fail_order(x);
    }
    return 0;
  }
  return 0;
}

int mg_run_cooking(MgtSession *session) {
  CookCtx ctx;
  MgtPersistentState *st = session ? session->state : NULL;
  memset(&ctx, 0, sizeof ctx);
  ctx.st = st;
  ctx.lives = 3;
  new_order(&ctx);
  if (st) st->cooking_shift_earnings = 0;
  mgt_run_loop(&ctx, cook_update, cook_draw, cook_key);
  if (st) {
    st->money += ctx.total_earnings;
    st->cooking_shift_earnings = ctx.total_earnings;
    st->skill_cooking += ctx.score / 100;
    if (st->skill_cooking > 100) st->skill_cooking = 100;
    snprintf(st->last_banner, sizeof st->last_banner, "Cook shift +$%d",
             ctx.total_earnings);
    st->last_success = (ctx.lives > 0 && ctx.total_earnings > 0) ? 1 : 0;
  }
  return 0;
}
