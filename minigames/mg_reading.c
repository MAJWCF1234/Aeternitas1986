#include "mgt.h"
#include "mgt_read.h"
#include "mgt_platform.h"
#include "mgt_host.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

enum { READ_MAX_LINES = 400, READ_LINE_W = 88 };

typedef struct {
  MgtReadDocument doc;
  int seamless;
  int scroll;  char lines[READ_MAX_LINES][READ_LINE_W];
  int line_count;
} ReadCtx;

typedef struct {
  int margin_x;
  int box_x;
  int box_y;
  int box_w;
  int box_h;
  int text_x;
  int text_y0;
  int visible;
  int spine_x;
  int show_spine;
  const char *frame_tag;
} ReadLayout;

static void read_layout_for(MgtReadKind kind, ReadLayout *lo) {
  memset(lo, 0, sizeof *lo);
  lo->box_y = 3;
  lo->box_h = 20;
  lo->text_y0 = 7;
  lo->visible = 13;
  lo->frame_tag = mgt_read_kind_label(kind);

  switch (kind) {
  case MGT_READ_BOOK:
    lo->margin_x = 15;
    lo->box_w = 54;
    lo->text_x = 17;
    lo->spine_x = lo->margin_x + 25;
    lo->show_spine = 1;
    lo->frame_tag = "Reading a Book";
    break;
  case MGT_READ_SIGN:
    lo->margin_x = 4;
    lo->box_w = 88;
    lo->text_x = 6;
    lo->visible = 14;
    lo->text_y0 = 6;
    break;
  case MGT_READ_SCROLL:
    lo->margin_x = 10;
    lo->box_w = 66;
    lo->text_x = 14;
    lo->visible = 12;
    break;
  case MGT_READ_LEDGER:
    lo->margin_x = 8;
    lo->box_w = 72;
    lo->text_x = 10;
    lo->visible = 14;
    lo->text_y0 = 6;
    break;
  case MGT_READ_LETTER:
    lo->margin_x = 12;
    lo->box_w = 58;
    lo->text_x = 16;
    lo->visible = 12;
  break;
  case MGT_READ_MAP:
    lo->margin_x = 8;
    lo->box_w = 76;
    lo->text_x = 10;
    lo->visible = 15;
    lo->text_y0 = 6;
    break;
  case MGT_READ_MUSIC:
    lo->margin_x = 8;
    lo->box_w = 72;
    lo->text_x = 10;
    lo->visible = 14;
    lo->text_y0 = 6;
    lo->frame_tag = "Sheet Music";
    break;
  case MGT_READ_NOTE:
  default:
    lo->margin_x = 5;
    lo->box_w = 70;
    lo->text_x = 7;
    break;
  }
  lo->box_x = lo->margin_x > 2 ? lo->margin_x - 2 : 2;
}

static void wrap_document(ReadCtx *r) {
  const char *words = r->doc.body;
  char word[64];
  int max_w;
  ReadLayout lo;
  int line = 0;
  int col = 0;
  int wi;

  read_layout_for(r->doc.kind, &lo);
  max_w = lo.box_w - 4;
  if (max_w < 24) max_w = 24;

  r->line_count = 0;
  r->lines[0][0] = '\0';

  while (*words && r->line_count < READ_MAX_LINES) {
    while (*words == ' ' || *words == '\n' || *words == '\r' || *words == '\t')
      words++;
    if (!*words) break;
    wi = 0;
    while (*words && *words != ' ' && *words != '\n' && *words != '\r' &&
           *words != '\t' && wi < 60)
      word[wi++] = *words++;
    word[wi] = '\0';
    if (!word[0]) continue;

    if (col > 0 && col + wi + 1 > max_w && r->line_count + 1 < READ_MAX_LINES) {
      line++;
      col = 0;
      r->lines[line][0] = '\0';
    }
    if (col > 0) {
      size_t L = strlen(r->lines[line]);
      if (L + 1 < READ_LINE_W) {
        r->lines[line][L] = ' ';
        r->lines[line][L + 1] = '\0';
      }
      col++;
    }
    if (strlen(r->lines[line]) + wi < READ_LINE_W)
      strcat(r->lines[line], word);
    col += wi;
    if (line + 1 > r->line_count) r->line_count = line + 1;
  }
  if (r->line_count == 0) {
    snprintf(r->lines[0], READ_LINE_W, "(blank page)");
    r->line_count = 1;
  }
}

static void read_put(MgtCanvas *c, int x, int y, char ch) {
  if (!c || x < 0 || x >= c->w || y < 0 || y >= c->h) return;
  c->cells[y * c->w + x] = ch;
}

static void read_draw(void *vctx, MgtCanvas *c) {
  ReadCtx *r = (ReadCtx *)vctx;
  ReadLayout lo;
  char line[96];
  int y, i;

  read_layout_for(r->doc.kind, &lo);
  mgt_canvas_clear(c);

  if (r->doc.kind == MGT_READ_BOOK)
    mgt_canvas_box(c, lo.box_x, lo.box_y, lo.box_w, lo.box_h, "Reading a Book");
  else if (r->seamless)
    mgt_canvas_box(c, lo.box_x, lo.box_y, lo.box_w, lo.box_h, NULL);
  else
    mgt_canvas_box(c, lo.box_x, lo.box_y, lo.box_w, lo.box_h, lo.frame_tag);
  if (lo.show_spine) {
    for (y = lo.box_y + 1; y < lo.box_y + lo.box_h - 1; y++)
      read_put(c, lo.spine_x, y, '|');
  }

  if (r->doc.kind == MGT_READ_SIGN) {
    snprintf(line, sizeof line, "[ %s ]", r->doc.title);
    mgt_canvas_write(c, lo.text_x, lo.box_y + 1, line);
  } else if (r->doc.kind == MGT_READ_MUSIC) {
    snprintf(line, sizeof line, "~ %s ~", r->doc.title);
    mgt_canvas_write(c, lo.text_x + 4, lo.box_y + 1, line);
  } else {
    snprintf(line, sizeof line, "~ %s ~", r->doc.title);
    mgt_canvas_write(c, lo.text_x + 2, lo.box_y + 1, line);
  }

  for (i = 0; i < lo.visible && r->scroll + i < r->line_count; i++) {
    int ly = lo.text_y0 + i;
    mgt_canvas_write(c, lo.text_x, ly, r->lines[r->scroll + i]);
  }

  if (r->scroll + lo.visible < r->line_count)
    mgt_canvas_write(c, lo.text_x, lo.box_y + lo.box_h - 2, "...");

  if (!r->seamless)
    mgt_canvas_write(c, 2, 1, "HARNESS READER (pack item or sample)");

  if (!r->seamless) {
    snprintf(line, sizeof line, "%s", mgt_read_kind_label(r->doc.kind));
    mgt_canvas_write(c, c->w - (int)strlen(line) - 3, 1, line);
  }

  if (!r->seamless) {
    snprintf(line, sizeof line, "Line %d/%d", r->scroll + 1,
             r->line_count > 0 ? r->line_count : 1);
    mgt_canvas_write(c, lo.text_x, lo.box_y + lo.box_h, line);
  }
  mgt_canvas_write(c, lo.text_x, lo.box_y + lo.box_h + (r->seamless ? 0 : 1),
                   r->seamless ? "ESC" : "Up/Down  PgUp/PgDn  ESC close");}

static int read_key(void *vctx, MgtKey key, int ch) {
  ReadCtx *r = (ReadCtx *)vctx;
  ReadLayout lo;
  int page;
  (void)ch;

  read_layout_for(r->doc.kind, &lo);
  page = lo.visible;
  if (page < 4) page = 4;

  if (key == MGT_KEY_ESC || key == MGT_KEY_QUIT) return -1;
  if (key == MGT_KEY_UP && r->scroll > 0) r->scroll--;
  if (key == MGT_KEY_DOWN && r->scroll + page < r->line_count) r->scroll++;
  if (key == MGT_KEY_PGUP) {
    r->scroll -= page;
    if (r->scroll < 0) r->scroll = 0;
  }
  if (key == MGT_KEY_PGDN) {
    r->scroll += page;
    if (r->scroll + page >= r->line_count && r->line_count > page)
      r->scroll = r->line_count - page;
    if (r->scroll < 0) r->scroll = 0;
  }
  return 0;
}

static void read_default_harness_doc(MgtReadDocument *doc) {
  memset(doc, 0, sizeof *doc);
  doc->valid = 1;
  doc->kind = MGT_READ_BOOK;
  snprintf(doc->title, sizeof doc->title, "Hollow Ridge Primer");
  snprintf(doc->source_id, sizeof doc->source_id, "harness_primer");
  snprintf(doc->body, sizeof doc->body,
           "Veritasfurtum frays where divine attention thins. Hollow Ridge is a "
           "lived-in chart: manor, village, temple, ridge. Waystones mark safe "
           "crossings; rifts are arguments between layers that no longer agree. "
           "In-game, read <item> opens any literature with the correct layout for "
           "books, signs, scrolls, ledgers, letters, maps, and music sheets.");
}

int mg_run_reading(MgtSession *session) {
  ReadCtx ctx;
  memset(&ctx, 0, sizeof ctx);
  ctx.seamless = session && session->adventure_embedded;

  if (!mgt_read_take_document(&ctx.doc)) {
    if (ctx.seamless) {
      if (session && session->state)
        snprintf(session->state->last_banner, sizeof session->state->last_banner,
                 "Nothing to read.");
      return 0;
    }
    if (!mgt_read_harness_fill_from_sim(mgt_host_game_sim()) ||
        !mgt_read_take_document(&ctx.doc))
      read_default_harness_doc(&ctx.doc);
  }
  wrap_document(&ctx);
  mgt_run_loop(&ctx, NULL, read_draw, read_key);
  mgt_read_clear_document();

  if (session && session->state && ctx.doc.title[0])
    snprintf(session->state->last_banner, sizeof session->state->last_banner,
             "Finished reading: %s", ctx.doc.title);
  return 0;
}
