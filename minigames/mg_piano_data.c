#include "mg_piano_data.h"
#include <stddef.h>

static const char *const k_notation[] = {
  "[3e] - 0 - [5w] t y r\n"
    "[6e] - 9 - [2w] r y w\n"
    "[3e] - [5tu] p u [0r] s [up]\n"
    "[5oa] - f [6u] [os] - [4ey] -\n"
    "[80] [5t] o - t w 8 0\n"
    "[3e] - 0 - [5w] t [8y] r\n"
    "[6e] - [30r] e 8 [%y] i u\n"
    "3 a [9w] - [2e] [wt] r e\n"
    "[4w] 0 - [5w] t y [2r] e -\n"
    "[39] - w [5t] y r [2e] -\n"
    "u p [wt] [0p] w [2ru] -\n"
    "[0e] - [30] w 1 y [qt] [wr]\n"
    "[2e] - 0 - [3w] t y r\n"
    "[2e] - 9 - [1w] r y w\n"
    "[2e] - [eu] - [3eu] - e u\n"
    "[5p]-b-d - [ys] 3 [ei] y j\n"
    "[2g] [rup] - [yoa] 3 p u t\n"
    "2 e 8 0 3 w t y\n"
    "[5r] e - [9ey] [3o] s p -\n"
    "[2t] [eu] p o [3e] y o [ta]\n"
    "[1e] - 9 w [up]-",
  "[woB]-E-t - [EP] - [ts] -\n"
    "[Dtc]-Y-o - t d [Es] -\n"
    "[EPwz]-E-t - [epZ] - [woz] -\n"
    "[qpl]-e-t, [wP] [es] E\n"
    "[woB]-E-t - [EP] - [ts] -\n"
    "[Dtc]-Y-o - t d [Es] -\n"
    "[EPwz]-E-t - [epZ] w [qoc] 9\n"
    "[8epc]-t-i - ( [qP] [ws] [eg]\n"
    "[wthB]-Y-o - Y [yg] [ED] w\n"
    "[(Edc]-y-i - [ys] t [EP] w\n"
    "[9esz]-t-i - [yZ] [tP] [Esz] w\n"
    "[E8dl]-y-i - t E [eg] q\n"
    "[wthB]-Y-o - Y [yg] [ED] w\n"
    "[(Edc]-y-i - [ys] t [EP] w\n"
    "[9esz]-t-i - [yZ] [tP] [Esc] w\n"
    "[8wdc]-E-Y - t E [ep] q\n"
    "[woz]-E-t - [EP] - [ts] E\n"
    "[Dtc]-Y-o - E d [es] q\n"
    "[Pwz]-E-t - [pEZ] - [toc] E\n"
    "[qtpl]-e-t - E P [es] -\n"
    "[woz]-E-t - [EP] - [ts] E\n"
    "[Dtc]-Y-o - E d [Es] w\n"
    "[Pwyc]-E-t - [tpZ] - [Eoz] -\n"
    "[epl]-t-i - t P [es] p",
  "[26p] y [15i]-u-i [^9qo] y [60] [^59y]\n"
    "[468t] e e q [135w],\n"
    "[26p] y [15i]-u-i [^9qo] y [690] y\n"
    "[468t] e e 0 [269],\n"
    "[26ryip] y [15i]-u-i [^9qo] y [60] [^59y]\n"
    "[468t] e e q [135w],\n"
    "[26tyup] y [15i]-u-i [^9qo] y [690] y\n"
    "[468t] e e 0 [269],\n"
    "[$*(126ryip]-$-[$y]-[1$]-[$(125i]-[$u]-[$i]-$-[$^(9qo]-[1$]-[$y]-[1$]-[$(260]-$-[$^259y]-$-\n"
    "[$(1468t]-$-[$e]-[1$]-[$(2e]-$-[$1q]-$-[$(135w]-[1$]-$-[1$]-[2$(]-$-$-[2$]-\n"
    "[$(126tyup]-$-[$y]-[1$]-[$(125i]-[$u]-[$i]-$-[$^(9qo]-[1$]-[$y]-[1$]-[$(2690]-$-[$2y]-$-\n"
    "[$(1468t]-$-[$e]-[1$]-[$(2e]-$-[$10]-$-[269$(]-[1$]-$-[1$]-[2$(]-$-$-[2$]-",
};

static const char *const k_titles[] = {
  "The Barkeep's First Lesson",
  "Candlelight Reel of Hollow Ridge",
  "Last Call at the Copper Cup",
};

static const char *const k_ids[] = {
  "barkeep_lesson",
  "lantern_waltz",
  "last_call_etude",
};

static const char *const k_diff[] = {
  "easy",
  "medium",
  "hard",
};

int mg_piano_sheet_count(void) { return 3; }
const char *mg_piano_sheet_id(int i) { return k_ids[i]; }
const char *mg_piano_sheet_title(int i) { return k_titles[i]; }
const char *mg_piano_sheet_diff(int i) { return k_diff[i]; }
const char *mg_piano_sheet_notation(int i) { return k_notation[i]; }
