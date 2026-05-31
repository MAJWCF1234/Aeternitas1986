#include "mgt.h"
#include "mgt_platform.h"

#include <stdio.h>
#include <string.h>

enum { GAM_PHASE_BET = 0, GAM_PHASE_PLAY = 1, GAM_PHASE_RESULT = 2 };

typedef struct {
  int screen;
  int phase;
  int money;
  int bet;
  int dice_p[2];
  int dice_h[2];
  int dealer_hidden;
  char deck[52][4];
  int deck_n;
  char player[12][4];
  int player_n;
  char dealer[12][4];
  int dealer_n;
  int can_double;
  char message[96];
} GamCtx;

static int card_val(const char *r) {
  if (r[0] == 'A') return 11;
  if (r[0] == 'K' || r[0] == 'Q' || r[0] == 'J') return 10;
  if (r[0] == '1') return 10;
  return r[0] - '0';
}

static int hand_val(GamCtx *g, int dealer_hand) {
  int i, sum = 0, aces = 0;
  int n = dealer_hand ? g->dealer_n : g->player_n;
  for (i = 0; i < n; i++) {
    const char *c = dealer_hand ? g->dealer[i] : g->player[i];
    sum += card_val(c);
    if (c[0] == 'A') aces++;
  }
  while (sum > 21 && aces > 0) {
    sum -= 10;
    aces--;
  }
  return sum;
}

static void shuffle(GamCtx *g) {
  static const char *ranks[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10",
                                "J", "Q", "K", "A"};
  static const char suits[] = {'S', 'H', 'D', 'C'};
  int i;
  for (i = 0; i < 52; i++)
    snprintf(g->deck[i], sizeof g->deck[i], "%s%c", ranks[i / 4], suits[i % 4]);
  g->deck_n = 52;
  for (i = g->deck_n - 1; i > 0; i--) {
    int k = mgt_rand_range(0, i);
    char tmp[4];
    strcpy(tmp, g->deck[i]);
    strcpy(g->deck[i], g->deck[k]);
    strcpy(g->deck[k], tmp);
  }
}

static void draw_card(GamCtx *g, int to_dealer) {
  if (g->deck_n <= 0) return;
  g->deck_n--;
  if (to_dealer)
    strcpy(g->dealer[g->dealer_n++], g->deck[g->deck_n]);
  else
    strcpy(g->player[g->player_n++], g->deck[g->deck_n]);
}

static void render_dice(MgtCanvas *c, int val, int x, int y) {
  mgt_canvas_box(c, x, y, 7, 5, NULL);
  if (val % 2 == 1) mgt_canvas_write(c, x + 3, y + 2, "o");
  if (val > 1) {
    mgt_canvas_write(c, x + 1, y + 1, "o");
    mgt_canvas_write(c, x + 5, y + 3, "o");
  }
  if (val > 3) {
    mgt_canvas_write(c, x + 5, y + 1, "o");
    mgt_canvas_write(c, x + 1, y + 3, "o");
  }
  if (val == 6) {
    mgt_canvas_write(c, x + 1, y + 2, "o");
    mgt_canvas_write(c, x + 5, y + 2, "o");
  }
}

static void resolve_cards(GamCtx *g);

static void render_card(MgtCanvas *c, const char *card, int x, int y, int hidden) {
  char line[8];
  mgt_canvas_box(c, x, y, 6, 5, NULL);
  if (hidden) {
    mgt_canvas_write(c, x + 1, y + 2, " ?? ");
    return;
  }
  snprintf(line, sizeof line, "%c", card[0]);
  mgt_canvas_write(c, x + 1, y + 1, line);
  snprintf(line, sizeof line, "%c", card[1]);
  mgt_canvas_write(c, x + 3, y + 2, line);
}

static void adjust_bet(GamCtx *g, int delta) {
  g->bet += delta;
  if (g->bet < 5) g->bet = 5;
  if (g->bet > g->money) g->bet = g->money;
  if (g->money < 5) g->bet = g->money;
}

static void gam_draw(void *vctx, MgtCanvas *c) {
  GamCtx *g = (GamCtx *)vctx;
  char line[96];
  int i;

  mgt_canvas_clear(c);
  mgt_canvas_write(c, 25, 1, "THE RUSTY ANCHOR TAVERN");
  snprintf(line, sizeof line, "CASH: $%d   BET: $%d", g->money, g->bet);
  mgt_canvas_write(c, 30, 2, line);

  if (g->screen == 0) {
    mgt_canvas_box(c, 20, 6, 40, 10, "WHAT'LL IT BE?");
    mgt_canvas_write(c, 25, 9, "[1] DICE DUEL");
    mgt_canvas_write(c, 25, 11, "[2] BLACKJACK");
    mgt_canvas_write(c, 25, 13, "[ESC] LEAVE");
    mgt_canvas_write(c, 22, 15, g->message);
    mgt_canvas_write(c, 18, 17, "UP/DN adjust bet before picking game");
    return;
  }

  if (g->phase == GAM_PHASE_BET) {
    mgt_canvas_box(c, 25, 8, 30, 8, "PLACE BET");
    snprintf(line, sizeof line, "BET: $%d", g->bet);
    mgt_canvas_write(c, 30, 11, line);
    mgt_canvas_write(c, 27, 13, "[UP/DN] Change  [SPACE] Start");
    return;
  }

  if (g->screen == 1) {
    mgt_canvas_write(c, 10, 6, "PLAYER ROLL:");
    render_dice(c, g->dice_p[0], 10, 8);
    render_dice(c, g->dice_p[1], 18, 8);
    mgt_canvas_write(c, 50, 6, "HOUSE ROLL:");
    render_dice(c, g->dice_h[0], 50, 8);
    render_dice(c, g->dice_h[1], 58, 8);
    mgt_canvas_write(c, 28, 15, g->message);
    if (g->phase == GAM_PHASE_RESULT)
      mgt_canvas_write(c, 26, 17, "[SPACE] AGAIN  [ESC] MENU");
    return;
  }

  mgt_canvas_write(c, 10, 6, "DEALER:");
  for (i = 0; i < g->dealer_n; i++)
    render_card(c, g->dealer[i], 10 + i * 7, 8, g->dealer_hidden && i == 1);
  snprintf(line, sizeof line, g->dealer_hidden ? "Dealer: ??" : "Dealer: %d",
           hand_val(g, 1));
  mgt_canvas_write(c, 10, 14, line);
  mgt_canvas_write(c, 10, 16, "PLAYER:");
  for (i = 0; i < g->player_n; i++)
    render_card(c, g->player[i], 10 + i * 7, 18, 0);
  snprintf(line, sizeof line, "You: %d", hand_val(g, 0));
  mgt_canvas_write(c, 10, 24, line);
  mgt_canvas_write(c, 10, 26, g->message);
  if (g->phase == GAM_PHASE_PLAY)
    mgt_canvas_write(c, 10, 27, "H hit  S stand  D double");
  if (g->phase == GAM_PHASE_RESULT)
    mgt_canvas_write(c, 10, 27, "[SPACE] AGAIN  [ESC] MENU");
}

static void start_cards(GamCtx *g) {
  if (g->bet > g->money) g->bet = g->money;
  g->money -= g->bet;
  g->player_n = g->dealer_n = 0;
  g->dealer_hidden = 1;
  shuffle(g);
  draw_card(g, 0);
  draw_card(g, 0);
  draw_card(g, 1);
  draw_card(g, 1);
  g->phase = GAM_PHASE_PLAY;
  g->can_double = 1;
  snprintf(g->message, sizeof g->message, "Hit, Stand or Double?");
  if (hand_val(g, 0) == 21) {
    g->dealer_hidden = 0;
    resolve_cards(g);
  }
}

static void resolve_cards(GamCtx *g) {
  int ps, ds, win;
  g->dealer_hidden = 0;
  g->phase = GAM_PHASE_RESULT;
  g->can_double = 0;
  ps = hand_val(g, 0);
  ds = hand_val(g, 1);
  while (ds < 17 && ps <= 21) {
    draw_card(g, 1);
    ds = hand_val(g, 1);
  }
  ps = hand_val(g, 0);
  ds = hand_val(g, 1);
  win = g->bet;
  if (ps > 21) {
    snprintf(g->message, sizeof g->message, "Bust! You lost $%d.", g->bet);
    return;
  }
  if (ds > 21) {
    g->money += g->bet * 2;
    snprintf(g->message, sizeof g->message, "Dealer bust! Won $%d!", g->bet);
    return;
  }
  if (ps > ds) {
    if (ps == 21 && g->player_n == 2) {
      int w = (g->bet * 3) / 2;
      g->money += g->bet + w;
      snprintf(g->message, sizeof g->message, "BLACKJACK! +$%d", w);
    } else {
      g->money += g->bet * 2;
      snprintf(g->message, sizeof g->message, "You win $%d!", g->bet);
    }
    return;
  }
  if (ps < ds) {
    snprintf(g->message, sizeof g->message, "House wins. Lost $%d.", g->bet);
    return;
  }
  g->money += g->bet;
  snprintf(g->message, sizeof g->message, "Push (tie). Bet returned.");
  (void)win;
}

static void play_dice(GamCtx *g) {
  int ps, hs;
  if (g->bet > g->money) g->bet = g->money;
  g->money -= g->bet;
  g->dice_p[0] = mgt_rand_range(1, 6);
  g->dice_p[1] = mgt_rand_range(1, 6);
  g->dice_h[0] = mgt_rand_range(1, 6);
  g->dice_h[1] = mgt_rand_range(1, 6);
  ps = g->dice_p[0] + g->dice_p[1];
  hs = g->dice_h[0] + g->dice_h[1];
  if (ps > hs) {
    g->money += g->bet * 2;
    snprintf(g->message, sizeof g->message, "You won $%d!", g->bet);
  } else if (ps < hs) {
    snprintf(g->message, sizeof g->message, "You lost $%d.", g->bet);
  } else {
    g->money += g->bet;
    snprintf(g->message, sizeof g->message, "Draw. Money returned.");
  }
  g->phase = GAM_PHASE_RESULT;
}

static int gam_key(void *vctx, MgtKey key, int ch) {
  GamCtx *g = (GamCtx *)vctx;

  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) {
    if (g->screen != 0) {
      g->screen = 0;
      g->phase = GAM_PHASE_BET;
      snprintf(g->message, sizeof g->message, "Back at the bar.");
      return 0;
    }
    return -1;
  }

  if (key == MGT_KEY_UP) adjust_bet(g, 5);
  if (key == MGT_KEY_DOWN) adjust_bet(g, -5);

  if (g->screen == 0) {
    if (key == MGT_KEY_CHAR && ch == '1') {
      g->screen = 1;
      g->phase = GAM_PHASE_BET;
      snprintf(g->message, sizeof g->message, "Dice duel — place your bet.");
    }
    if (key == MGT_KEY_CHAR && ch == '2') {
      g->screen = 2;
      g->phase = GAM_PHASE_BET;
      snprintf(g->message, sizeof g->message, "Blackjack — place your bet.");
    }
    return 0;
  }

  if (g->phase == GAM_PHASE_RESULT && key == MGT_KEY_SPACE) {
    g->phase = GAM_PHASE_BET;
    return 0;
  }
  if (g->phase == GAM_PHASE_RESULT) return 0;

  if (g->phase == GAM_PHASE_BET) {
    if (g->money < 5) {
      snprintf(g->message, sizeof g->message, "You are broke.");
      return 0;
    }
    if (g->bet > g->money) g->bet = g->money;
    if (key == MGT_KEY_SPACE && g->bet > 0 && g->money >= g->bet) {
      if (g->screen == 1)
        play_dice(g);
      else
        start_cards(g);
    }
    return 0;
  }

  if (g->screen == 1) return 0;

  if (key == MGT_KEY_CHAR && (ch == 'h' || ch == 'H')) {
    draw_card(g, 0);
    g->can_double = 0;
    if (hand_val(g, 0) > 21) resolve_cards(g);
    return 0;
  }
  if (key == MGT_KEY_CHAR && (ch == 's' || ch == 'S')) {
    resolve_cards(g);
    return 0;
  }
  if (key == MGT_KEY_CHAR && (ch == 'd' || ch == 'D') && g->can_double) {
    if (g->money >= g->bet) {
      g->money -= g->bet;
      g->bet *= 2;
    }
    draw_card(g, 0);
    g->can_double = 0;
    resolve_cards(g);
  }
  return 0;
}

int mg_run_gambling(MgtSession *session) {
  GamCtx ctx;
  MgtPersistentState *st = session ? session->state : NULL;
  memset(&ctx, 0, sizeof ctx);
  ctx.money = st ? st->money : 100;
  ctx.bet = 10;
  if (ctx.bet > ctx.money) ctx.bet = ctx.money;
  mgt_run_loop(&ctx, NULL, gam_draw, gam_key);
  if (st) {
    st->money = ctx.money;
    snprintf(st->last_banner, sizeof st->last_banner, "Left tavern with $%d", ctx.money);
  }
  return 0;
}
