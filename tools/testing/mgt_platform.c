#include "mgt_platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <conio.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

static unsigned g_rng = 1;
static int g_host_tty = 0;
static int g_term_inited = 0;

#if defined(_WIN32)
static HANDLE g_stdout = INVALID_HANDLE_VALUE;
static int g_vt_mode = 0;
static int g_win_console_saved = 0;
static CONSOLE_SCREEN_BUFFER_INFO g_win_saved_csbi;
static int g_win_last_rows = 0;
enum { MGT_VIEW_COLS = 96, MGT_VIEW_ROWS = 40 };

static void mgt_win_write_line(int row, const char *text, int len, int width) {
  COORD pos;
  DWORD written;
  int pad, i;
  if (g_stdout == INVALID_HANDLE_VALUE || row < 0 || row >= MGT_VIEW_ROWS) return;
  if (len < 0) len = 0;
  if (len > width) len = width;
  pos.X = 0;
  pos.Y = (SHORT)row;
  SetConsoleCursorPosition(g_stdout, pos);
  if (len > 0) WriteConsoleA(g_stdout, text, (DWORD)len, &written, NULL);
  pad = width - len;
  for (i = 0; i < pad; i++) WriteConsoleA(g_stdout, " ", 1, &written, NULL);
}

static void mgt_win_flush_plain_frame(const char *buf, size_t len) {
  size_t off = 0;
  int row = 0;
  if (!buf || g_stdout == INVALID_HANDLE_VALUE) return;
  while (off < len && row < MGT_VIEW_ROWS) {
    size_t e = off;
    while (e < len && buf[e] != '\n') e++;
    mgt_win_write_line(row, buf + off, (int)(e - off), MGT_VIEW_COLS);
    off = e + (e < len ? 1u : 0u);
    row++;
  }
  for (; row < g_win_last_rows; row++) mgt_win_write_line(row, "", 0, MGT_VIEW_COLS);
  g_win_last_rows = row;
  SetConsoleCursorPosition(g_stdout, (COORD){0, 0});
}

static void mgt_win_viewport_enter(void) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD bufsize;
  SMALL_RECT win;
  DWORD mode = 0;
  DWORD n = 0;
  COORD origin = {0, 0};

  g_stdout = h;
  if (h == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(h, &csbi)) return;

  g_win_saved_csbi = csbi;
  g_win_console_saved = 1;
  g_win_last_rows = 0;

  if (GetConsoleMode(h, &mode))
    SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  g_vt_mode = 1;

  win.Left = 0;
  win.Top = 0;
  win.Right = (SHORT)(MGT_VIEW_COLS - 1);
  win.Bottom = (SHORT)(MGT_VIEW_ROWS - 1);
  SetConsoleWindowInfo(h, TRUE, &win);

  bufsize.X = (SHORT)MGT_VIEW_COLS;
  bufsize.Y = (SHORT)MGT_VIEW_ROWS;
  SetConsoleScreenBufferSize(h, bufsize);

  FillConsoleOutputCharacterA(h, ' ', (DWORD)(MGT_VIEW_COLS * MGT_VIEW_ROWS), origin, &n);
  FillConsoleOutputAttribute(h, csbi.wAttributes, (DWORD)(MGT_VIEW_COLS * MGT_VIEW_ROWS),
                           origin, &n);
  SetConsoleCursorPosition(h, origin);

  {
    CONSOLE_CURSOR_INFO cci = {1, FALSE};
    SetConsoleCursorInfo(h, &cci);
  }
}

static void mgt_win_viewport_leave(void) {
  HANDLE h = g_stdout;
  CONSOLE_CURSOR_INFO cci = {1, TRUE};
  if (!g_win_console_saved || h == INVALID_HANDLE_VALUE) return;
  SetConsoleScreenBufferSize(h, g_win_saved_csbi.dwSize);
  SetConsoleWindowInfo(h, TRUE, &g_win_saved_csbi.srWindow);
  SetConsoleCursorPosition(h, g_win_saved_csbi.dwCursorPosition);
  SetConsoleCursorInfo(h, &cci);
  g_win_console_saved = 0;
  g_win_last_rows = 0;
}
#endif

void mgt_seed(unsigned s) {
  g_rng = s ? s : 1;
}

static unsigned mgt_rng_next(void) {
  g_rng = g_rng * 1664525u + 1013904223u;
  return g_rng;
}

double mgt_rand01(void) {
  return (double)(mgt_rng_next() >> 8) / (double)(1u << 24);
}

int mgt_rand_range(int lo, int hi) {
  int span;
  if (hi <= lo) return lo;
  span = hi - lo + 1;
  return lo + (int)(mgt_rand01() * (double)span);
}

void mgt_terminal_init(void) {
  if (g_term_inited) return;
#if defined(_WIN32)
  mgt_win_viewport_enter();
#else
  fputs("\033[?25l\033[?1049h\033[2J\033[H", stdout);
  fflush(stdout);
#endif
  g_term_inited = 1;
}

void mgt_terminal_shutdown(void) {
  if (!g_term_inited) return;
#if defined(_WIN32)
  mgt_win_viewport_leave();
#else
  fputs("\033[?1049l\033[?25h\033[0m", stdout);
  fflush(stdout);
#endif
  g_term_inited = 0;
}

void mgt_clear_screen(void) {
  fputs("\033[2J\033[H", stdout);
}

void mgt_canvas_init(MgtCanvas *c, int w, int h) {
  if (!c) return;
  if (w > MGT_CANVAS_W) w = MGT_CANVAS_W;
  if (h > MGT_CANVAS_H) h = MGT_CANVAS_H;
  c->w = w;
  c->h = h;
  mgt_canvas_clear(c);
}

void mgt_canvas_clear(MgtCanvas *c) {
  size_t n;
  if (!c) return;
  n = (size_t)c->w * (size_t)c->h;
  memset(c->cells, ' ', n);
}

void mgt_canvas_write(MgtCanvas *c, int x, int y, const char *text) {
  int i;
  if (!c || !text) return;
  for (i = 0; text[i]; i++) {
    int xx = x + i;
    if (xx < 0 || xx >= c->w || y < 0 || y >= c->h) continue;
    c->cells[y * c->w + xx] = text[i];
  }
}

void mgt_canvas_box(MgtCanvas *c, int x, int y, int w, int h, const char *title) {
  int i;
  if (!c || w < 2 || h < 2) return;
  mgt_canvas_write(c, x, y, "+");
  for (i = 1; i < w - 1; i++) {
    if (x + i < c->w) c->cells[y * c->w + (x + i)] = '-';
    if (x + i < c->w && y + h - 1 < c->h)
      c->cells[(y + h - 1) * c->w + (x + i)] = '-';
  }
  mgt_canvas_write(c, x + w - 1, y, "+");
  mgt_canvas_write(c, x, y + h - 1, "+");
  mgt_canvas_write(c, x + w - 1, y + h - 1, "+");
  for (i = 1; i < h - 1; i++) {
    if (y + i < c->h) c->cells[(y + i) * c->w + x] = '|';
    if (y + i < c->h && x + w - 1 < c->w)
      c->cells[(y + i) * c->w + (x + w - 1)] = '|';
  }
  if (title && title[0]) {
    char buf[96];
    snprintf(buf, sizeof buf, "[ %s ]", title);
    mgt_canvas_write(c, x + 2, y, buf);
  }
}

void mgt_canvas_meter(char *out, size_t cap, int len, double fill01) {
  int n, i;
  if (!out || cap < 4 || len < 2) return;
  if (fill01 < 0.0) fill01 = 0.0;
  if (fill01 > 1.0) fill01 = 1.0;
  n = (int)(fill01 * (double)(len - 2) + 0.5);
  if (n > len - 2) n = len - 2;
  out[0] = '[';
  for (i = 0; i < len - 2; i++) out[1 + i] = (i < n) ? '#' : '-';
  out[len - 1] = ']';
  out[len] = '\0';
}

enum { MGT_TTY_FRAME_CAP = 16384 };

static size_t mgt_tty_append(char *buf, size_t cap, size_t off, const char *s) {
  size_t n;
  if (!buf || !s || off >= cap) return off;
  n = strlen(s);
  if (n > cap - off - 1) n = cap - off - 1;
  memcpy(buf + off, s, n);
  return off + n;
}

static size_t mgt_tty_append_line(char *buf, size_t cap, size_t off, const char *text,
                                  int width) {
  int i, len = text ? (int)strlen(text) : 0;
  if (width < 0) width = 0;
  if (len > width) len = width;
  if ((size_t)(off + (size_t)width + 4) >= cap) return off;
  if (len > 0) {
    memcpy(buf + off, text, (size_t)len);
    off += (size_t)len;
  }
  for (i = len; i < width; i++) buf[off++] = ' ';
#if defined(_WIN32)
  if (off + 1 < cap) buf[off++] = '\n';
#else
  off = mgt_tty_append(buf, cap, off, "\033[K\n");
#endif
  return off;
}

static size_t mgt_tty_append_canvas(char *buf, size_t cap, size_t off,
                                    const MgtCanvas *c) {
  int y;
  if (!c) return off;
  for (y = 0; y < c->h; y++) {
    if ((size_t)(off + (size_t)c->w + 4) >= cap) break;
    memcpy(buf + off, c->cells + y * c->w, (size_t)c->w);
    off += (size_t)c->w;
#if defined(_WIN32)
    if (off + 1 < cap) buf[off++] = '\n';
#else
    off = mgt_tty_append(buf, cap, off, "\033[K\n");
#endif
  }
  return off;
}

void mgt_tty_flush_bytes(const char *buf, size_t len) {
  if (!buf || len == 0) return;
#if defined(_WIN32)
  if (g_stdout != INVALID_HANDLE_VALUE) {
    mgt_win_flush_plain_frame(buf, len);
    return;
  }
#endif
  fwrite(buf, 1, len, stdout);
  fflush(stdout);
}

void mgt_present(const MgtCanvas *c) {
  char frame[MGT_TTY_FRAME_CAP];
  size_t off = 0;
  if (!c) return;
#if !defined(_WIN32)
  off = mgt_tty_append(frame, sizeof frame, off, "\033[H");
#endif
  off = mgt_tty_append_canvas(frame, sizeof frame, off, c);
#if !defined(_WIN32)
  off = mgt_tty_append(frame, sizeof frame, off, "\033[J");
#endif
  mgt_tty_flush_bytes(frame, off);
}

size_t mgt_host_tty_encode_frame(char *buf, size_t cap, const MgtCanvas *canvas,
                                 const char *panel_title, const char *status_line,
                                 const char *controls) {
  char title[128];
  char status[128];
  char ctrl[128];
  size_t off = 0;
  if (!buf || cap < 32) return 0;
  snprintf(title, sizeof title, "  %s", panel_title && panel_title[0] ? panel_title : "");
  snprintf(status, sizeof status, "  %s",
           status_line && status_line[0] ? status_line : "");
  snprintf(ctrl, sizeof ctrl, "  %s", controls && controls[0] ? controls : "");
#if !defined(_WIN32)
  off = mgt_tty_append(buf, cap, off, "\033[H");
#endif
  off = mgt_tty_append_line(buf, cap, off,
                            "================================================================",
                            MGT_CANVAS_W);
  off = mgt_tty_append_line(buf, cap, off, title, MGT_CANVAS_W);
  off = mgt_tty_append_line(buf, cap, off,
                            "----------------------------------------------------------------",
                            MGT_CANVAS_W);
  if (canvas) off = mgt_tty_append_canvas(buf, cap, off, canvas);
  off = mgt_tty_append_line(buf, cap, off,
                            "----------------------------------------------------------------",
                            MGT_CANVAS_W);
  off = mgt_tty_append_line(buf, cap, off, status, MGT_CANVAS_W);
  off = mgt_tty_append_line(buf, cap, off, ctrl, MGT_CANVAS_W);
  off = mgt_tty_append_line(buf, cap, off,
                            "================================================================",
                            MGT_CANVAS_W);
#if !defined(_WIN32)
  off = mgt_tty_append(buf, cap, off, "\033[J");
#endif
  return off;
}

unsigned long mgt_now_ms(void) {
#if defined(_WIN32)
  return (unsigned long)GetTickCount();
#else
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) return 0;
  return (unsigned long)(ts.tv_sec * 1000ul + ts.tv_nsec / 1000000ul);
#endif
}

void mgt_sleep_ms(unsigned ms) {
#if defined(_WIN32)
  Sleep(ms);
#else
  usleep((useconds_t)ms * 1000u);
#endif
}

#if !defined(_WIN32)
static struct termios g_save;
static int g_raw;

static int tty_raw_on(void) {
  struct termios t;
  if (!isatty(STDIN_FILENO)) return 0;
  if (tcgetattr(STDIN_FILENO, &g_save) != 0) return 0;
  t = g_save;
  t.c_lflag &= (tcflag_t)~(ICANON | ECHO);
  t.c_cc[VMIN] = 0;
  t.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &t) != 0) return 0;
  g_raw = 1;
  return 1;
}

static void tty_raw_off(void) {
  if (g_raw) {
    (void)tcsetattr(STDIN_FILENO, TCSANOW, &g_save);
    g_raw = 0;
  }
}

static int tty_read_byte(void) {
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) != 1) return -1;
  return (int)c;
}

static MgtKey decode_unix(int c, int *ch) {
  if (c < 0) return MGT_KEY_QUIT;
  if (ch) *ch = c;
  if (c == 3 || c == 4) return MGT_KEY_QUIT;
  if (c == '\r' || c == '\n') return MGT_KEY_ENTER;
  if (c == ' ') return MGT_KEY_SPACE;
  if (c == 9) return MGT_KEY_TAB;
  if (c == 'q' || c == 'Q') return MGT_KEY_QUIT;
  if (c >= 32 && c < 127) return MGT_KEY_CHAR;
  if (c == 27) {
    fd_set rfds;
    struct timeval tv = {0, 40000};
    int b1, b2;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    if (select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) <= 0)
      return MGT_KEY_ESC;
    b1 = tty_read_byte();
    if (b1 != '[') return MGT_KEY_NONE;
    b2 = tty_read_byte();
    if (b2 == 'A') return MGT_KEY_UP;
    if (b2 == 'B') return MGT_KEY_DOWN;
    if (b2 == 'C') return MGT_KEY_RIGHT;
    if (b2 == 'D') return MGT_KEY_LEFT;
    if (b2 == 'H') return MGT_KEY_HOME;
    if (b2 == 'F') return MGT_KEY_END;
    if (b2 == '5') {
      (void)tty_read_byte();
      return MGT_KEY_PGUP;
    }
    if (b2 == '6') {
      (void)tty_read_byte();
      return MGT_KEY_PGDN;
    }
  }
  return MGT_KEY_NONE;
}
#endif

#if defined(_WIN32)
static MgtKey decode_win(int *ch) {
  int c = _getch();
  if (c == 0 || c == 224) {
    int c2 = _getch();
    switch (c2) {
      case 72:
        return MGT_KEY_UP;
      case 80:
        return MGT_KEY_DOWN;
      case 75:
        return MGT_KEY_LEFT;
      case 77:
        return MGT_KEY_RIGHT;
      case 73:
        return MGT_KEY_PGUP;
      case 81:
        return MGT_KEY_PGDN;
      case 71:
        return MGT_KEY_HOME;
      case 79:
        return MGT_KEY_END;
      default:
        return MGT_KEY_NONE;
    }
  }
  if (ch) *ch = c;
  if (c == 3) return MGT_KEY_QUIT;
  if (c == '\r') return MGT_KEY_ENTER;
  if (c == 27) return MGT_KEY_ESC;
  if (c == ' ') return MGT_KEY_SPACE;
  if (c == 9) return MGT_KEY_TAB;
  if (c == 'q' || c == 'Q') return MGT_KEY_QUIT;
  if (c >= 32 && c < 127) return MGT_KEY_CHAR;
  return MGT_KEY_NONE;
}
#endif

void mgt_input_host_begin(void) {
#if !defined(_WIN32)
  (void)tty_raw_on();
#endif
  g_host_tty = 1;
}

void mgt_input_host_end(void) {
#if !defined(_WIN32)
  if (!g_host_tty) tty_raw_off();
#endif
  g_host_tty = 0;
}

int mgt_input_host_active(void) { return g_host_tty; }

int mgt_autotest_active(void) {
  const char *e = getenv("MGT_AUTOTEST");
  if (e && e[0] && e[0] != '0') return 1;
  e = getenv("AETER_AUTOTEST");
  return e && e[0] && e[0] != '0';
}

MgtKey mgt_poll_key(int timeout_ms, int *ch) {
#if defined(_WIN32)
  unsigned long deadline = mgt_now_ms() + (unsigned long)(timeout_ms > 0 ? timeout_ms : 0);
  for (;;) {
    if (_kbhit()) return decode_win(ch);
    if ((int)(deadline - mgt_now_ms()) <= 0) return MGT_KEY_NONE;
    mgt_sleep_ms(16);
  }
#else
  fd_set rfds;
  struct timeval tv;
  int c;
  if (!g_raw && !g_host_tty && !tty_raw_on()) {
    mgt_sleep_ms((unsigned)timeout_ms);
    return MGT_KEY_NONE;
  }
  FD_ZERO(&rfds);
  FD_SET(STDIN_FILENO, &rfds);
  tv.tv_sec = timeout_ms / 1000;
  tv.tv_usec = (timeout_ms % 1000) * 1000;
  if (select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) <= 0)
    return MGT_KEY_NONE;
  c = tty_read_byte();
  return decode_unix(c, ch);
#endif
}

MgtKey mgt_read_key(int *ch) {
  for (;;) {
    MgtKey k = mgt_poll_key(50, ch);
    if (k != MGT_KEY_NONE) return k;
  }
}

int mgt_run_loop(void *ctx, void (*update)(void *ctx, double dt),
                 void (*draw)(void *ctx, MgtCanvas *c),
                 int (*on_key)(void *ctx, MgtKey key, int ch)) {
  MgtCanvas canvas;
  char last_cells[sizeof canvas.cells];
  unsigned long last = mgt_now_ms();
  unsigned long last_present = 0;
  int running = 1;
  int result = 0;
  int dirty = 1;
  int have_last_cells = 0;
  int autotest_esc = mgt_autotest_active();
  const int animated = (update != NULL);
  const unsigned present_ms = animated ? 50u : 0u;
  const size_t cell_bytes = (size_t)MGT_CANVAS_W * (size_t)MGT_CANVAS_H;

#if !defined(_WIN32)
  if (!g_host_tty) (void)tty_raw_on();
#endif
  mgt_canvas_init(&canvas, MGT_CANVAS_W, MGT_CANVAS_H);

  while (running) {
    unsigned long now = mgt_now_ms();
    double dt = (now - last) / 1000.0;
    int ch = 0;
    MgtKey k;
    int poll_ms;
    if (dt < 0.0) dt = 0.0;
    if (dt > 0.1) dt = 0.1;
    last = now;

    if (update) update(ctx, dt);

    if (animated) {
      if (last_present == 0 || now - last_present >= present_ms) {
        if (draw) draw(ctx, &canvas);
        if (!have_last_cells || memcmp(canvas.cells, last_cells, cell_bytes) != 0) {
          mgt_frame_present(&canvas);
          memcpy(last_cells, canvas.cells, cell_bytes);
          have_last_cells = 1;
          last_present = now;
        }
      }
      poll_ms = 16;
    } else if (dirty) {
      if (draw) draw(ctx, &canvas);
      mgt_frame_present(&canvas);
      last_present = now;
      dirty = 0;
      poll_ms = 50;
    } else {
      poll_ms = 50;
    }

    if (autotest_esc && on_key) {
      autotest_esc = 0;
      int r = on_key(ctx, MGT_KEY_ESC, 27);
      if (r != 0) {
        result = r;
        running = 0;
      }
      continue;
    }

    k = mgt_poll_key(poll_ms, &ch);
    if (animated) {
      unsigned long after = mgt_now_ms();
      unsigned long frame_ms = after - now;
      if (frame_ms < 16) mgt_sleep_ms(16 - frame_ms);
    }
    if (k != MGT_KEY_NONE && on_key) {
      int r = on_key(ctx, k, ch);
      if (r != 0) {
        result = r;
        running = 0;
      }
      dirty = 1;
    }
  }

#if !defined(_WIN32)
  if (!g_host_tty) tty_raw_off();
#endif
  return result;
}

void mgt_pause_panel(const char *title, const char *body) {
  if (g_host_tty) {
    MgtCanvas c;
    int ch = 0;
    mgt_canvas_init(&c, MGT_CANVAS_W, MGT_CANVAS_H);
    for (;;) {
      MgtKey k;
      mgt_canvas_clear(&c);
      mgt_canvas_box(&c, 4, 4, 88, 20, title ? title : "DONE");
      mgt_canvas_write(&c, 6, 8, body ? body : "");
      mgt_canvas_write(&c, 6, 22, "Enter — continue");
      mgt_present(&c);
      k = mgt_poll_key(100, &ch);
      if (k == MGT_KEY_ENTER || k == MGT_KEY_ESC || k == MGT_KEY_QUIT) break;
    }
    return;
  }
  {
    char line[256];
    mgt_clear_screen();
    printf("=== %s ===\n\n%s\n\n[Press Enter]", title ? title : "Done",
           body ? body : "");
    fflush(stdout);
    if (!fgets(line, sizeof line, stdin)) line[0] = '\0';
  }
}
