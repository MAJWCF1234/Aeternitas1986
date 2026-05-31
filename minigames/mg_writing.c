#include "mgt.h"
#include "mgt_platform.h"

#include <stdio.h>
#include <string.h>

enum {
  W_SCR_MENU = 0,
  W_SCR_SHOP = 1,
  W_SCR_PORT = 2,
  W_SCR_WRITE = 3
};

typedef struct {
  char title[48];
  char type[12];
  char body[256];
  int value;
} PortfolioWork;

typedef struct {
  MgtPersistentState *st;
  int screen;
  int money;
  int paper;
  int ink;
  int envelopes;
  int leather;
  int books;
  char wtype[12];
  char title[48];
  char body[256];
  int body_len;
  char message[96];
  PortfolioWork port[16];
  int port_n;
  int total_earnings;
} WriteCtx;

static void sync_from_st(WriteCtx *w) {
  MgtPersistentState *st = w->st;
  if (!st) return;
  w->money = st->money;
  w->paper = st->paper;
  w->ink = st->ink_uses;
  w->envelopes = st->envelopes;
  w->leather = st->leather;
  w->books = st->empty_books;
}

static void sync_to_st(WriteCtx *w) {
  MgtPersistentState *st = w->st;
  if (!st) return;
  st->money = w->money;
  st->paper = w->paper;
  st->ink_uses = w->ink;
  st->envelopes = w->envelopes;
  st->leather = w->leather;
  st->empty_books = w->books;
}

static void write_draw(void *vctx, MgtCanvas *c) {
  WriteCtx *w = (WriteCtx *)vctx;
  char line[96];
  int i;

  mgt_canvas_clear(c);
  mgt_canvas_write(c, 28, 1, "THE SCRIBE'S DESK");

  if (w->screen == W_SCR_MENU) {
    mgt_canvas_box(c, 8, 4, 64, 17, "WRITER'S DESK");
    snprintf(line, sizeof line, "Funds $%d  Paper %d  Ink %d  Env %d  Leather %d  Books %d",
             w->money, w->paper, w->ink, w->envelopes, w->leather, w->books);
    mgt_canvas_write(c, 12, 7, line);
    mgt_canvas_write(c, 12, 9, "1) Write note   2) Write letter   3) Write book");
    mgt_canvas_write(c, 12, 11, "4) Craft empty book (5 paper + leather)");
    mgt_canvas_write(c, 12, 13, "S) Shop   P) Portfolio   ESC exit");
    mgt_canvas_write(c, 12, 16, w->message);
    return;
  }

  if (w->screen == W_SCR_SHOP) {
    mgt_canvas_box(c, 12, 5, 56, 14, "SUPPLY SHOP");
    mgt_canvas_write(c, 16, 7, "[1] Paper $2   [2] Ink vial $8 (+50 uses)");
    mgt_canvas_write(c, 16, 9, "[3] Envelope $3   [4] Leather $15");
    mgt_canvas_write(c, 16, 12, "[X] Back");
    snprintf(line, sizeof line, "Funds $%d", w->money);
    mgt_canvas_write(c, 16, 14, line);
    mgt_canvas_write(c, 12, 20, w->message);
    return;
  }

  if (w->screen == W_SCR_PORT) {
    mgt_canvas_box(c, 10, 5, 60, 16, "PORTFOLIO");
    if (w->port_n == 0)
      mgt_canvas_write(c, 24, 10, "No works yet.");
    else
      for (i = 0; i < w->port_n && i < 8; i++) {
        snprintf(line, sizeof line, "%d) %s (%s) $%d", i + 1, w->port[i].title,
                 w->port[i].type, w->port[i].value);
        mgt_canvas_write(c, 14, 7 + i, line);
      }
    mgt_canvas_write(c, 14, 16, "1-8 sell work   X back");
    mgt_canvas_write(c, 10, 22, w->message);
    return;
  }

  mgt_canvas_box(c, 4, 4, 72, 20, "WRITING");
  snprintf(line, sizeof line, "Type: %s  Title: %s", w->wtype, w->title);
  mgt_canvas_write(c, 6, 6, line);
  mgt_canvas_write(c, 6, 8, "Letters to type; BACKSPACE; ENTER finish");
  snprintf(line, sizeof line, "%.*s", 68, w->body);
  mgt_canvas_write(c, 6, 10, line);
  mgt_canvas_write(c, 6, 14, w->message);
}

static int can_start(WriteCtx *w, const char *type) {
  if (w->ink < 5) {
    snprintf(w->message, sizeof w->message, "Out of ink!");
    return 0;
  }
  if (!strcmp(type, "NOTE") && w->paper < 1) {
    snprintf(w->message, sizeof w->message, "Need paper.");
    return 0;
  }
  if (!strcmp(type, "LETTER") && (w->paper < 1 || w->envelopes < 1)) {
    snprintf(w->message, sizeof w->message, "Need paper and envelope.");
    return 0;
  }
  if (!strcmp(type, "BOOK") && w->books < 1) {
    snprintf(w->message, sizeof w->message, "Need an empty book.");
    return 0;
  }
  return 1;
}

static void finish_work(WriteCtx *w) {
  int base = w->body_len / 2;
  PortfolioWork *pw;
  if (base < 1) base = 1;
  if (!strcmp(w->wtype, "LETTER")) base = (base * 3) / 2;
  if (!strcmp(w->wtype, "BOOK")) base *= 3;
  if (!strcmp(w->wtype, "NOTE")) w->paper--;
  if (!strcmp(w->wtype, "LETTER")) {
    w->paper--;
    w->envelopes--;
  }
  if (!strcmp(w->wtype, "BOOK")) w->books--;
  w->ink -= 5;
  if (w->port_n < 16) {
    pw = &w->port[w->port_n++];
    snprintf(pw->title, sizeof pw->title, "%s",
             w->title[0] ? w->title : "Untitled");
    snprintf(pw->type, sizeof pw->type, "%s", w->wtype);
    snprintf(pw->body, sizeof pw->body, "%s", w->body);
    pw->value = base;
  }
  snprintf(w->message, sizeof w->message, "Finished \"%s\" (est. $%d).",
           w->title[0] ? w->title : "Untitled", base);
  w->screen = W_SCR_MENU;
}

static int write_key(void *vctx, MgtKey key, int ch) {
  WriteCtx *w = (WriteCtx *)vctx;
  int i;

  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) {
    if (w->screen == W_SCR_WRITE) {
      w->screen = W_SCR_MENU;
      return 0;
    }
    if (w->screen != W_SCR_MENU) {
      w->screen = W_SCR_MENU;
      return 0;
    }
    return -1;
  }

  if (w->screen == W_SCR_MENU) {
    if (key == MGT_KEY_CHAR && ch == '1' && can_start(w, "NOTE")) {
      snprintf(w->wtype, sizeof w->wtype, "NOTE");
      w->title[0] = w->body[0] = '\0';
      w->body_len = 0;
      w->screen = W_SCR_WRITE;
    }
    if (key == MGT_KEY_CHAR && ch == '2' && can_start(w, "LETTER")) {
      snprintf(w->wtype, sizeof w->wtype, "LETTER");
      w->title[0] = w->body[0] = '\0';
      w->body_len = 0;
      w->screen = W_SCR_WRITE;
    }
    if (key == MGT_KEY_CHAR && ch == '3' && can_start(w, "BOOK")) {
      snprintf(w->wtype, sizeof w->wtype, "BOOK");
      w->title[0] = w->body[0] = '\0';
      w->body_len = 0;
      w->screen = W_SCR_WRITE;
    }
    if (key == MGT_KEY_CHAR && ch == '4') {
      if (w->paper >= 5 && w->leather >= 1) {
        w->paper -= 5;
        w->leather--;
        w->books++;
        snprintf(w->message, sizeof w->message, "Crafted empty book.");
      } else
        snprintf(w->message, sizeof w->message, "Need 5 paper + 1 leather.");
    }
    if (key == MGT_KEY_CHAR && (ch == 's' || ch == 'S')) w->screen = W_SCR_SHOP;
    if (key == MGT_KEY_CHAR && (ch == 'p' || ch == 'P')) w->screen = W_SCR_PORT;
    return 0;
  }

  if (w->screen == W_SCR_SHOP) {
    if (key == MGT_KEY_CHAR && ch == '1' && w->money >= 2) {
      w->money -= 2;
      w->paper++;
      snprintf(w->message, sizeof w->message, "Bought paper.");
    }
    if (key == MGT_KEY_CHAR && ch == '2' && w->money >= 8) {
      w->money -= 8;
      w->ink += 50;
      snprintf(w->message, sizeof w->message, "Bought ink.");
    }
    if (key == MGT_KEY_CHAR && ch == '3' && w->money >= 3) {
      w->money -= 3;
      w->envelopes++;
      snprintf(w->message, sizeof w->message, "Bought envelope.");
    }
    if (key == MGT_KEY_CHAR && ch == '4' && w->money >= 15) {
      w->money -= 15;
      w->leather++;
      snprintf(w->message, sizeof w->message, "Bought leather.");
    }
    if (key == MGT_KEY_CHAR && (ch == 'x' || ch == 'X')) w->screen = W_SCR_MENU;
    return 0;
  }

  if (w->screen == W_SCR_PORT) {
    if (key == MGT_KEY_CHAR && ch >= '1' && ch <= '8') {
      i = ch - '1';
      if (i < w->port_n) {
        w->money += w->port[i].value;
        w->total_earnings += w->port[i].value;
        memmove(&w->port[i], &w->port[i + 1], (size_t)(w->port_n - i - 1) * sizeof w->port[0]);
        w->port_n--;
        snprintf(w->message, sizeof w->message, "Sold work.");
      }
    }
    if (key == MGT_KEY_CHAR && (ch == 'x' || ch == 'X')) w->screen = W_SCR_MENU;
    return 0;
  }

  if (w->screen == W_SCR_WRITE) {
    if (key == MGT_KEY_ENTER) {
      finish_work(w);
      return 0;
    }
    if (key == MGT_KEY_CHAR && ch == 8 && w->body_len > 0) w->body[--w->body_len] = '\0';
    if (key == MGT_KEY_CHAR && ch >= 32 && ch < 127 && w->body_len < 250) {
      if (w->body_len == 0 && !w->title[0]) {
        w->title[w->body_len] = (char)ch;
        w->title[w->body_len + 1] = '\0';
      } else {
        w->body[w->body_len++] = (char)ch;
        w->body[w->body_len] = '\0';
      }
    }
  }
  return 0;
}

int mg_run_writing(MgtSession *session) {
  WriteCtx ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.st = session ? session->state : NULL;
  ctx.ink = 50;
  ctx.paper = 3;
  sync_from_st(&ctx);
  if (!ctx.st) {
    ctx.money = 50;
    ctx.envelopes = 1;
  }
  mgt_run_loop(&ctx, NULL, write_draw, write_key);
  sync_to_st(&ctx);
  if (ctx.st) {
    snprintf(ctx.st->last_banner, sizeof ctx.st->last_banner, "Writing +$%d",
             ctx.total_earnings);
  }
  return 0;
}
