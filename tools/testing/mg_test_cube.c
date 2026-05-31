#include "mgt.h"
#include "mgt_host.h"
#include "mgt_platform.h"

#include <stdio.h>
#include <string.h>

typedef struct {
  MgtPersistentState *st;
  int pulse;
} TestCubeCtx;

static void test_cube_draw(void *vctx, MgtCanvas *c) {
  TestCubeCtx *ctx = (TestCubeCtx *)vctx;
  MgtPersistentState *st = ctx->st;
  char line[96];
  int y = 3;

  mgt_canvas_init(c, MGT_CANVAS_W, MGT_CANVAS_H);
  mgt_canvas_clear(c);
  mgt_canvas_box(c, 2, 1, 92, 28, "TEST CUBE - MODULE VIEWPORT LIVE");

  mgt_canvas_write(c, 4, y++, "Seated in socket. Bus contacts streaming below.");
  mgt_canvas_box(c, 4, y, 40, 10, "CONTACT STREAM");
  y++;
  if (!st) {
    mgt_canvas_write(c, 6, y + 1, "ERROR: session->state is NULL");
    return;
  }

  snprintf(line, sizeof line, "COIN  $%-6d  (purse wire)", st->money);
  mgt_canvas_write(c, 6, y + 1, line);
  snprintf(line, sizeof line, "SKIL  eng:%-3d surv:%-3d cook:%-3d", st->skill_engineering,
           st->skill_survival, st->skill_cooking);
  mgt_canvas_write(c, 6, y + 2, line);
  snprintf(line, sizeof line, "WTHR  %-12s  ROOM  %s", st->game_weather, st->hunt_area);
  mgt_canvas_write(c, 6, y + 3, line);
  snprintf(line, sizeof line, "TURN  %-6d  MGST  profile v%d OK", st->adventure_turn,
           st->version);
  mgt_canvas_write(c, 6, y + 4, line);
  snprintf(line, sizeof line, "PACK  lock:%d bow:%d key:%d", st->has_basic_lockpick,
           st->has_hunting_bow, st->has_skeleton_key);
  mgt_canvas_write(c, 6, y + 5, line);

  mgt_canvas_box(c, 48, 8, 44, 12, "PULSE CHAMBER");
  snprintf(line, sizeof line, "Count: %d / 3", ctx->pulse);
  mgt_canvas_write(c, 50, 10, line);
  mgt_canvas_write(c, 50, 11, "[");
  {
    int i;
    char bar[8];
    for (i = 0; i < 3; i++) bar[i] = (i < ctx->pulse) ? '#' : '-';
    bar[3] = '\0';
    snprintf(line, sizeof line, "%s] verify", bar);
    mgt_canvas_write(c, 51, 11, line);
  }
  mgt_canvas_write(c, 50, 13, "SPACE  send pulse");
  mgt_canvas_write(c, 50, 14, "ESC    eject to workbench");
  if (ctx->pulse >= 3)
    mgt_canvas_write(c, 50, 16, ">>> PASS — integrate to minigames/");

  mgt_canvas_box(c, 4, 20, 88, 6, "SCHEMATIC");
  mgt_canvas_write(c, 6, 21, "  +------+     tubes     +--------+");
  mgt_canvas_write(c, 6, 22, "  | CORE |~~~~~~~~~~~~~~| SOCKET |");
  mgt_canvas_write(c, 6, 23, "  +------+               +---+----+");
  mgt_canvas_write(c, 6, 24, "                             |");
  mgt_canvas_write(c, 6, 25, "                         [ TESTCUBE ]");
}

static int test_cube_key(void *vctx, MgtKey k, int ch) {
  TestCubeCtx *ctx = (TestCubeCtx *)vctx;
  (void)ch;
  if (k == MGT_KEY_ESC) return 0;
  if (k == MGT_KEY_SPACE) ctx->pulse++;
  return 1;
}

int mg_run_test_cube(MgtSession *session) {
  TestCubeCtx ctx;

  memset(&ctx, 0, sizeof ctx);
  ctx.st = session ? session->state : NULL;
  mgt_run_loop(&ctx, NULL, test_cube_draw, test_cube_key);
  if (ctx.st) {
    ctx.st->last_success = ctx.pulse >= 1 ? 1 : 0;
    snprintf(ctx.st->last_banner, sizeof ctx.st->last_banner,
             "TEST CUBE ejected — %d pulse(s), bus sync OK.", ctx.pulse);
  }
  return 0;
}
