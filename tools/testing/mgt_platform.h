#ifndef MGT_PLATFORM_H
#define MGT_PLATFORM_H

#include <stddef.h>

enum {
  MGT_CANVAS_W = 96,
  MGT_CANVAS_H = 30,
  MGT_CANVAS_W_SM = 80,
  MGT_CANVAS_H_SM = 25
};

typedef struct MgtCanvas {
  int w;
  int h;
  char cells[96 * 30];
} MgtCanvas;

typedef enum MgtKey {
  MGT_KEY_NONE = 0,
  MGT_KEY_UP,
  MGT_KEY_DOWN,
  MGT_KEY_LEFT,
  MGT_KEY_RIGHT,
  MGT_KEY_PGUP,
  MGT_KEY_PGDN,
  MGT_KEY_HOME,
  MGT_KEY_END,
  MGT_KEY_TAB,
  MGT_KEY_ENTER,
  MGT_KEY_ESC,
  MGT_KEY_SPACE,
  MGT_KEY_QUIT,
  MGT_KEY_CHAR
} MgtKey;

void mgt_seed(unsigned s);
double mgt_rand01(void);
int mgt_rand_range(int lo, int hi);

void mgt_terminal_init(void);
void mgt_terminal_shutdown(void);
void mgt_clear_screen(void);
void mgt_canvas_init(MgtCanvas *c, int w, int h);
void mgt_canvas_clear(MgtCanvas *c);
void mgt_canvas_write(MgtCanvas *c, int x, int y, const char *text);
void mgt_canvas_box(MgtCanvas *c, int x, int y, int w, int h, const char *title);
void mgt_canvas_meter(char *out, size_t cap, int len, double fill01);
void mgt_present(const MgtCanvas *c);
void mgt_tty_flush_bytes(const char *buf, size_t len);
size_t mgt_host_tty_encode_frame(char *buf, size_t cap, const MgtCanvas *canvas,
                                 const char *panel_title, const char *status_line,
                                 const char *controls);
void mgt_frame_present(const MgtCanvas *c);

void mgt_input_host_begin(void);
void mgt_input_host_end(void);
int mgt_input_host_active(void);

unsigned long mgt_now_ms(void);
void mgt_sleep_ms(unsigned ms);

MgtKey mgt_read_key(int *ch);

MgtKey mgt_poll_key(int timeout_ms, int *ch);

int mgt_run_loop(void *ctx, void (*update)(void *ctx, double dt),
                 void (*draw)(void *ctx, MgtCanvas *c),
                 int (*on_key)(void *ctx, MgtKey key, int ch));

void mgt_pause_panel(const char *title, const char *body);

int mgt_autotest_active(void);

#endif
