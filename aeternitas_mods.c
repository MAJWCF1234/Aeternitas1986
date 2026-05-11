#include "aeternitas_mods.h"
#include "aeternitas_world_generated.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AETER_MODS_MAX_PACKS
#define AETER_MODS_MAX_PACKS 96
#endif

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

static char **g_blurb_ov;
static char **g_title_ov;
static int g_world_n;

typedef struct {
  char *id;
  char *text;
} IdText;

static IdText *g_items;
static int g_items_n, g_items_cap;

typedef struct {
  char *id;
  char *mat_class;
  int is_base;
  int hrd, shp, flx, dur, wgt, grp, bnd, utl;
} CraftProfileMod;
static CraftProfileMod *g_craft_profiles;
static int g_craft_profiles_n, g_craft_profiles_cap;

typedef struct {
  char *name;
  int req_hrd, req_shp, req_dur, req_bnd, req_grp, req_flx;
} CraftArchetypeMod;
static CraftArchetypeMod *g_craft_arch;
static int g_craft_arch_n, g_craft_arch_cap;

typedef struct {
  char *entity;
  char *text;
} EntText;

static EntText *g_greetings;
static int g_greetings_n, g_greetings_cap;

typedef struct {
  char *entity;
  char *keyword;
  char *text;
} NpcTopicMod;

static NpcTopicMod *g_topics;
static int g_topics_n, g_topics_cap;

static char *g_status_summary;
static char g_mods_root[520];

static char *g_char_sheet_append;
static char *g_char_portrait_append;
static char *g_char_aptitudes_append;
static char *g_char_reputation_append;
static char *g_char_loadout_append;
static char *g_char_traits_append;
static char *g_char_momentum_append;
static char *g_char_perks_append;
static char *g_char_voice_append;
static char *g_char_bio_append;
static char *g_char_tainting_append;
static char *g_char_rapport_append;
static char *g_char_objectives_append;
static char *g_char_vitals_append;
static char *g_char_examine_append;
static char *g_char_notes_append;
static char *g_char_hints_append;
static char *g_char_journal_append;
static char *g_char_progress_append;
static char *g_char_inventory_append;
static char *g_char_waypoints_append;
static char *g_char_status_append;
static char *g_char_score_append;
static char *g_char_time_append;
static char *g_char_weather_append;
static char *g_char_room_append;
static char *g_char_recap_append;
static char *g_char_help_append;
static char *g_char_about_append;
static char *g_char_lights_append;
static char *g_char_exits_append;
static char *g_char_scan_append;
static char *g_char_trail_append;
static char *g_char_nearby_append;
static char *g_char_lockcheck_append;
static char *g_char_noise_append;
static char *g_char_nav_append;
static char *g_char_route_append;
static char *g_char_loot_append;
static char *g_char_compare_append;
static char *g_char_people_append;
static char *g_char_diagnostics_append;
static char *g_char_wares_append;
static char *g_char_saves_append;
static char *g_char_hints_panel_append;
static char *g_char_notes_panel_append;

static char g_loaded_packs[AETER_MODS_MAX_PACKS][256];
static int g_loaded_pack_pri[AETER_MODS_MAX_PACKS];
static char g_loaded_pack_id[AETER_MODS_MAX_PACKS][128];
static char g_loaded_pack_title[AETER_MODS_MAX_PACKS][192];
static int g_loaded_pack_n;

#ifndef AETER_MOD_WARN_CAP
#define AETER_MOD_WARN_CAP 36
#endif
static char g_warn_unknown_room[AETER_MOD_WARN_CAP][144];
static int g_warn_unknown_room_n;
static char g_warn_room_read[AETER_MOD_WARN_CAP][144];
static int g_warn_room_read_n;
static int g_warn_path_join;

static void lc_inplace(char *s) {
  char *p;
  for (p = s; *p; p++) *p = (char)tolower((unsigned char)*p);
}

static int str_ieq_local(const char *a, const char *b) {
  if (!a || !b) return 0;
  for (; *a && *b; a++, b++) {
    if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
  }
  return *a == *b;
}

static void trim_tail(char *s) {
  size_t n;
  if (!s) return;
  n = strlen(s);
  while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r' || s[n - 1] == '\t' ||
                   s[n - 1] == ' '))
    s[--n] = '\0';
}

static void strip_utf8_bom(char *s) {
  unsigned char *u;
  if (!s || !s[0]) return;
  u = (unsigned char *)s;
  if (u[0] == 0xefu && u[1] == 0xbbu && u[2] == 0xbfu)
    memmove(s, s + 3, strlen(s + 3) + 1u);
}

static void strip_cr_inplace(char *s) {
  char *r, *w;
  if (!s) return;
  for (r = w = s; *r; r++) {
    if (*r != '\r') *w++ = *r;
  }
  *w = '\0';
}

static int ends_with(const char *s, const char *suf) {
  size_t ls, lf;
  if (!s || !suf) return 0;
  ls = strlen(s);
  lf = strlen(suf);
  return lf <= ls && strcmp(s + ls - lf, suf) == 0;
}

static int dir_exists(const char *path);

static char *read_text_file(const char *path, size_t max_bytes) {
  FILE *fp;
  char *buf;
  long sz;

  fp = fopen(path, "rb");
  if (!fp) return NULL;
  if (fseek(fp, 0, SEEK_END) != 0) {
    fclose(fp);
    return NULL;
  }
  sz = ftell(fp);
  if (sz < 0 || (size_t)sz > max_bytes) {
    fclose(fp);
    return NULL;
  }
  rewind(fp);
  buf = (char *)malloc((size_t)sz + 1u);
  if (!buf) {
    fclose(fp);
    return NULL;
  }
  if (sz > 0 && fread(buf, 1, (size_t)sz, fp) != (size_t)sz) {
    free(buf);
    fclose(fp);
    return NULL;
  }
  buf[sz] = '\0';
  fclose(fp);
  strip_utf8_bom(buf);
  strip_cr_inplace(buf);
  trim_tail(buf);
  return buf;
}

static int path_join(char *out, size_t cap, const char *a, const char *b) {
  size_t la = a ? strlen(a) : 0, lb = b ? strlen(b) : 0;
  int need_sep = (la > 0 && a[la - 1] != '/' && a[la - 1] != '\\');
  if (la + lb + (need_sep ? 2 : 1) > cap) return 0;
  memcpy(out, a, la);
  if (need_sep) {
#if defined(_WIN32)
    out[la++] = '\\';
#else
    out[la++] = '/';
#endif
  }
  memcpy(out + la, b, lb + 1);
  return 1;
}

void aet_mods_build_default_path(const char *save_file_path, char *out,
                                 size_t outcap) {
  const char *slash;
  size_t dirlen;
  if (!out || outcap < 8) return;
  out[0] = '\0';
  if (!save_file_path || !save_file_path[0]) {
    snprintf(out, outcap, "mods");
    return;
  }
  slash = strrchr(save_file_path, '\\');
  if (!slash) slash = strrchr(save_file_path, '/');
  if (!slash) {
    snprintf(out, outcap, "mods");
    return;
  }
  dirlen = (size_t)(slash - save_file_path);
  if (dirlen + 2 >= outcap) {
    snprintf(out, outcap, "mods");
    return;
  }
  memcpy(out, save_file_path, dirlen);
  out[dirlen] = '\0';
#if defined(_WIN32)
  path_join(out, outcap, out, "mods");
#else
  path_join(out, outcap, out, "mods");
#endif
}

static void free_all(void) {
  int i;
  if (g_blurb_ov) {
    for (i = 0; i < g_world_n; i++) free(g_blurb_ov[i]);
    free(g_blurb_ov);
    g_blurb_ov = NULL;
  }
  if (g_title_ov) {
    for (i = 0; i < g_world_n; i++) free(g_title_ov[i]);
    free(g_title_ov);
    g_title_ov = NULL;
  }
  g_world_n = 0;
  for (i = 0; i < g_items_n; i++) {
    free(g_items[i].id);
    free(g_items[i].text);
  }
  free(g_items);
  g_items = NULL;
  g_items_n = g_items_cap = 0;
  for (i = 0; i < g_craft_profiles_n; i++) {
    free(g_craft_profiles[i].id);
    free(g_craft_profiles[i].mat_class);
  }
  free(g_craft_profiles);
  g_craft_profiles = NULL;
  g_craft_profiles_n = g_craft_profiles_cap = 0;
  for (i = 0; i < g_craft_arch_n; i++) free(g_craft_arch[i].name);
  free(g_craft_arch);
  g_craft_arch = NULL;
  g_craft_arch_n = g_craft_arch_cap = 0;
  for (i = 0; i < g_greetings_n; i++) {
    free(g_greetings[i].entity);
    free(g_greetings[i].text);
  }
  free(g_greetings);
  g_greetings = NULL;
  g_greetings_n = g_greetings_cap = 0;
  for (i = 0; i < g_topics_n; i++) {
    free(g_topics[i].entity);
    free(g_topics[i].keyword);
    free(g_topics[i].text);
  }
  free(g_topics);
  g_topics = NULL;
  g_topics_n = g_topics_cap = 0;
  free(g_status_summary);
  g_status_summary = NULL;
  free(g_char_sheet_append);
  g_char_sheet_append = NULL;
  free(g_char_portrait_append);
  g_char_portrait_append = NULL;
  free(g_char_aptitudes_append);
  g_char_aptitudes_append = NULL;
  free(g_char_reputation_append);
  g_char_reputation_append = NULL;
  free(g_char_loadout_append);
  g_char_loadout_append = NULL;
  free(g_char_traits_append);
  g_char_traits_append = NULL;
  free(g_char_momentum_append);
  g_char_momentum_append = NULL;
  free(g_char_perks_append);
  g_char_perks_append = NULL;
  free(g_char_voice_append);
  g_char_voice_append = NULL;
  free(g_char_bio_append);
  g_char_bio_append = NULL;
  free(g_char_tainting_append);
  g_char_tainting_append = NULL;
  free(g_char_rapport_append);
  g_char_rapport_append = NULL;
  free(g_char_objectives_append);
  g_char_objectives_append = NULL;
  free(g_char_vitals_append);
  g_char_vitals_append = NULL;
  free(g_char_examine_append);
  g_char_examine_append = NULL;
  free(g_char_notes_append);
  g_char_notes_append = NULL;
  free(g_char_hints_append);
  g_char_hints_append = NULL;
  free(g_char_journal_append);
  g_char_journal_append = NULL;
  free(g_char_progress_append);
  g_char_progress_append = NULL;
  free(g_char_inventory_append);
  g_char_inventory_append = NULL;
  free(g_char_waypoints_append);
  g_char_waypoints_append = NULL;
  free(g_char_status_append);
  g_char_status_append = NULL;
  free(g_char_score_append);
  g_char_score_append = NULL;
  free(g_char_time_append);
  g_char_time_append = NULL;
  free(g_char_weather_append);
  g_char_weather_append = NULL;
  free(g_char_room_append);
  g_char_room_append = NULL;
  free(g_char_recap_append);
  g_char_recap_append = NULL;
  free(g_char_help_append);
  g_char_help_append = NULL;
  free(g_char_about_append);
  g_char_about_append = NULL;
  free(g_char_lights_append);
  g_char_lights_append = NULL;
  free(g_char_exits_append);
  g_char_exits_append = NULL;
  free(g_char_scan_append);
  g_char_scan_append = NULL;
  free(g_char_trail_append);
  g_char_trail_append = NULL;
  free(g_char_nearby_append);
  g_char_nearby_append = NULL;
  free(g_char_lockcheck_append);
  g_char_lockcheck_append = NULL;
  free(g_char_noise_append);
  g_char_noise_append = NULL;
  free(g_char_nav_append);
  g_char_nav_append = NULL;
  free(g_char_route_append);
  g_char_route_append = NULL;
  free(g_char_loot_append);
  g_char_loot_append = NULL;
  free(g_char_compare_append);
  g_char_compare_append = NULL;
  free(g_char_people_append);
  g_char_people_append = NULL;
  free(g_char_diagnostics_append);
  g_char_diagnostics_append = NULL;
  free(g_char_wares_append);
  g_char_wares_append = NULL;
  free(g_char_saves_append);
  g_char_saves_append = NULL;
  free(g_char_hints_panel_append);
  g_char_hints_panel_append = NULL;
  free(g_char_notes_panel_append);
  g_char_notes_panel_append = NULL;
  g_loaded_pack_n = 0;
}

static int parse_int_strict(const char *s, int *out) {
  char *end = NULL;
  long v;
  if (!s || !*s || !out) return 0;
  v = strtol(s, &end, 10);
  if (end == s || *end != '\0') return 0;
  *out = (int)v;
  return 1;
}

static int split_pipe_fields(char *line, char **fields, int max_fields) {
  int n = 0;
  char *p = line;
  if (!line || !fields || max_fields <= 0) return 0;
  while (p && n < max_fields) {
    char *sep = strchr(p, '|');
    if (sep) {
      *sep = '\0';
      fields[n++] = p;
      p = sep + 1;
    } else {
      fields[n++] = p;
      break;
    }
  }
  return n;
}

static void add_craft_profile(char *id, char *mat_class, int is_base, int hrd,
                              int shp, int flx, int dur, int wgt, int grp,
                              int bnd, int utl) {
  int i;
  if (!id || !mat_class) return;
  for (i = 0; i < g_craft_profiles_n; i++) {
    if (!str_ieq_local(g_craft_profiles[i].id, id)) continue;
    free(g_craft_profiles[i].mat_class);
    g_craft_profiles[i].mat_class = mat_class;
    g_craft_profiles[i].is_base = is_base;
    g_craft_profiles[i].hrd = hrd;
    g_craft_profiles[i].shp = shp;
    g_craft_profiles[i].flx = flx;
    g_craft_profiles[i].dur = dur;
    g_craft_profiles[i].wgt = wgt;
    g_craft_profiles[i].grp = grp;
    g_craft_profiles[i].bnd = bnd;
    g_craft_profiles[i].utl = utl;
    free(id);
    return;
  }
  if (g_craft_profiles_n >= g_craft_profiles_cap) {
    int nc = g_craft_profiles_cap ? g_craft_profiles_cap * 2 : 16;
    CraftProfileMod *p =
        (CraftProfileMod *)realloc(g_craft_profiles, (size_t)nc * sizeof *p);
    if (!p) {
      free(id);
      free(mat_class);
      return;
    }
    g_craft_profiles = p;
    g_craft_profiles_cap = nc;
  }
  g_craft_profiles[g_craft_profiles_n].id = id;
  g_craft_profiles[g_craft_profiles_n].mat_class = mat_class;
  g_craft_profiles[g_craft_profiles_n].is_base = is_base;
  g_craft_profiles[g_craft_profiles_n].hrd = hrd;
  g_craft_profiles[g_craft_profiles_n].shp = shp;
  g_craft_profiles[g_craft_profiles_n].flx = flx;
  g_craft_profiles[g_craft_profiles_n].dur = dur;
  g_craft_profiles[g_craft_profiles_n].wgt = wgt;
  g_craft_profiles[g_craft_profiles_n].grp = grp;
  g_craft_profiles[g_craft_profiles_n].bnd = bnd;
  g_craft_profiles[g_craft_profiles_n].utl = utl;
  g_craft_profiles_n++;
}

static void add_craft_archetype(char *name, int req_hrd, int req_shp,
                                int req_dur, int req_bnd, int req_grp,
                                int req_flx) {
  int i;
  if (!name) return;
  for (i = 0; i < g_craft_arch_n; i++) {
    if (!str_ieq_local(g_craft_arch[i].name, name)) continue;
    g_craft_arch[i].req_hrd = req_hrd;
    g_craft_arch[i].req_shp = req_shp;
    g_craft_arch[i].req_dur = req_dur;
    g_craft_arch[i].req_bnd = req_bnd;
    g_craft_arch[i].req_grp = req_grp;
    g_craft_arch[i].req_flx = req_flx;
    free(name);
    return;
  }
  if (g_craft_arch_n >= g_craft_arch_cap) {
    int nc = g_craft_arch_cap ? g_craft_arch_cap * 2 : 12;
    CraftArchetypeMod *p =
        (CraftArchetypeMod *)realloc(g_craft_arch, (size_t)nc * sizeof *p);
    if (!p) {
      free(name);
      return;
    }
    g_craft_arch = p;
    g_craft_arch_cap = nc;
  }
  g_craft_arch[g_craft_arch_n].name = name;
  g_craft_arch[g_craft_arch_n].req_hrd = req_hrd;
  g_craft_arch[g_craft_arch_n].req_shp = req_shp;
  g_craft_arch[g_craft_arch_n].req_dur = req_dur;
  g_craft_arch[g_craft_arch_n].req_bnd = req_bnd;
  g_craft_arch[g_craft_arch_n].req_grp = req_grp;
  g_craft_arch[g_craft_arch_n].req_flx = req_flx;
  g_craft_arch_n++;
}

static void load_crafting_tables(const char *pack_path) {
  char dir[640], fp[700];
  char *body, *line, *save = NULL;
  if (!path_join(dir, sizeof dir, pack_path, "crafting") || !dir_exists(dir))
    return;
  if (path_join(fp, sizeof fp, dir, "profiles.txt")) {
    body = read_text_file(fp, 128000);
    if (body) {
      for (line = strtok_r(body, "\n", &save); line;
           line = strtok_r(NULL, "\n", &save)) {
        char *f[11];
        int n;
        trim_tail(line);
        if (!line[0] || line[0] == '#') continue;
        n = split_pipe_fields(line, f, 11);
        if (n == 11) {
          int isb, hrd, shp, flx, dur, wgt, grp, bnd, utl;
          if (parse_int_strict(f[2], &isb) && parse_int_strict(f[3], &hrd) &&
              parse_int_strict(f[4], &shp) && parse_int_strict(f[5], &flx) &&
              parse_int_strict(f[6], &dur) && parse_int_strict(f[7], &wgt) &&
              parse_int_strict(f[8], &grp) && parse_int_strict(f[9], &bnd) &&
              parse_int_strict(f[10], &utl)) {
            add_craft_profile(strdup(f[0]), strdup(f[1]), isb, hrd, shp, flx,
                              dur, wgt, grp, bnd, utl);
          }
        }
      }
      free(body);
    }
  }
  if (path_join(fp, sizeof fp, dir, "archetypes.txt")) {
    body = read_text_file(fp, 64000);
    if (body) {
      save = NULL;
      for (line = strtok_r(body, "\n", &save); line;
           line = strtok_r(NULL, "\n", &save)) {
        char *f[7];
        int n;
        trim_tail(line);
        if (!line[0] || line[0] == '#') continue;
        n = split_pipe_fields(line, f, 7);
        if (n == 7) {
          int hrd, shp, dur, bnd, grp, flx;
          if (parse_int_strict(f[1], &hrd) && parse_int_strict(f[2], &shp) &&
              parse_int_strict(f[3], &dur) && parse_int_strict(f[4], &bnd) &&
              parse_int_strict(f[5], &grp) && parse_int_strict(f[6], &flx)) {
            add_craft_archetype(strdup(f[0]), hrd, shp, dur, bnd, grp, flx);
          }
        }
      }
      free(body);
    }
  }
}

static void set_blurb(int room, char *text) {
  if (room < 0 || room >= g_world_n || !text) {
    free(text);
    return;
  }
  free(g_blurb_ov[room]);
  g_blurb_ov[room] = text;
}

static void set_title(int room, char *text) {
  if (room < 0 || room >= g_world_n || !text) {
    free(text);
    return;
  }
  free(g_title_ov[room]);
  g_title_ov[room] = text;
}

static char *blurb_base_dup(int room) {
  const char *b;
  char *out;
  size_t L;
  if (room < 0 || room >= g_world_n) return NULL;
  if (g_blurb_ov[room] && g_blurb_ov[room][0]) b = g_blurb_ov[room];
  else {
    b = world_blurb(room);
    if (!b) b = "";
  }
  L = strlen(b);
  out = (char *)malloc(L + 1u);
  if (!out) return NULL;
  memcpy(out, b, L + 1u);
  return out;
}

static void apply_room_prepend(int room, char *prefix) {
  char *base;
  size_t lb, lp;
  char *out;
  if (room < 0 || room >= g_world_n || !prefix) {
    free(prefix);
    return;
  }
  base = blurb_base_dup(room);
  if (!base) {
    free(prefix);
    return;
  }
  lb = strlen(base);
  lp = strlen(prefix);
  out = (char *)malloc(lb + lp + 4u);
  if (!out) {
    free(base);
    free(prefix);
    return;
  }
  memcpy(out, prefix, lp);
  out[lp] = '\n';
  out[lp + 1] = '\n';
  memcpy(out + lp + 2, base, lb + 1u);
  free(base);
  free(prefix);
  free(g_blurb_ov[room]);
  g_blurb_ov[room] = out;
}

static void apply_room_append(int room, char *suffix) {
  char *base;
  size_t lb, ls;
  char *out;
  if (room < 0 || room >= g_world_n || !suffix) {
    free(suffix);
    return;
  }
  base = blurb_base_dup(room);
  if (!base) {
    free(suffix);
    return;
  }
  lb = strlen(base);
  ls = strlen(suffix);
  out = (char *)malloc(lb + ls + 4u);
  if (!out) {
    free(base);
    free(suffix);
    return;
  }
  memcpy(out, base, lb);
  out[lb] = '\n';
  out[lb + 1] = '\n';
  memcpy(out + lb + 2, suffix, ls + 1u);
  free(base);
  free(suffix);
  free(g_blurb_ov[room]);
  g_blurb_ov[room] = out;
}

static void add_item_text(char *id, char *text) {
  int i;
  if (!id || !text) {
    free(id);
    free(text);
    return;
  }
  for (i = 0; i < g_items_n; i++) {
    if (str_ieq_local(g_items[i].id, id)) {
      free(g_items[i].text);
      g_items[i].text = text;
      free(id);
      return;
    }
  }
  if (g_items_n >= g_items_cap) {
    int nc = g_items_cap ? g_items_cap * 2 : 16;
    IdText *p = (IdText *)realloc(g_items, (size_t)nc * sizeof *g_items);
    if (!p) {
      free(id);
      free(text);
      return;
    }
    g_items = p;
    g_items_cap = nc;
  }
  g_items[g_items_n].id = id;
  g_items[g_items_n].text = text;
  g_items_n++;
}

static void add_greeting(char *entity, char *text) {
  int i;
  if (!entity || !text) {
    free(entity);
    free(text);
    return;
  }
  for (i = 0; i < g_greetings_n; i++) {
    if (str_ieq_local(g_greetings[i].entity, entity)) {
      free(g_greetings[i].text);
      g_greetings[i].text = text;
      free(entity);
      return;
    }
  }
  if (g_greetings_n >= g_greetings_cap) {
    int nc = g_greetings_cap ? g_greetings_cap * 2 : 8;
    EntText *p = (EntText *)realloc(g_greetings, (size_t)nc * sizeof *g_greetings);
    if (!p) {
      free(entity);
      free(text);
      return;
    }
    g_greetings = p;
    g_greetings_cap = nc;
  }
  g_greetings[g_greetings_n].entity = entity;
  g_greetings[g_greetings_n].text = text;
  g_greetings_n++;
}

static void add_topic(char *entity, char *keyword, char *text) {
  int i;
  if (!entity || !keyword || !text) {
    free(entity);
    free(keyword);
    free(text);
    return;
  }
  for (i = 0; i < g_topics_n; i++) {
    if (!str_ieq_local(g_topics[i].entity, entity)) continue;
    if (!g_topics[i].keyword || strcmp(g_topics[i].keyword, keyword) != 0)
      continue;
    free(g_topics[i].text);
    g_topics[i].text = text;
    free(entity);
    free(keyword);
    return;
  }
  if (g_topics_n >= g_topics_cap) {
    int nc = g_topics_cap ? g_topics_cap * 2 : 8;
    NpcTopicMod *p =
        (NpcTopicMod *)realloc(g_topics, (size_t)nc * sizeof *g_topics);
    if (!p) {
      free(entity);
      free(keyword);
      free(text);
      return;
    }
    g_topics = p;
    g_topics_cap = nc;
  }
  g_topics[g_topics_n].entity = entity;
  g_topics[g_topics_n].keyword = keyword;
  g_topics[g_topics_n].text = text;
  g_topics_n++;
}

#if defined(_WIN32)
typedef int (*mod_file_cb)(const char *fullpath, const char *relpart, void *ctx);

static int foreach_file_in_dir(const char *dir, const char *ext,
                               mod_file_cb cb, void *ctx) {
  char pat[600];
  WIN32_FIND_DATAA fd;
  HANDLE h;
  int n = 0;
  if (!dir || !ext || !cb) return 0;
  snprintf(pat, sizeof pat, "%s\\*", dir);
  h = FindFirstFileA(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) return 0;
  do {
    char full[600];
    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
    if (!ends_with(fd.cFileName, ext)) continue;
    if (!path_join(full, sizeof full, dir, fd.cFileName)) continue;
    if (cb(full, fd.cFileName, ctx)) n++;
  } while (FindNextFileA(h, &fd));
  FindClose(h);
  return n;
}

static int list_subdirs(const char *parent, char names[][256], int maxn) {
  char pat[600];
  WIN32_FIND_DATAA fd;
  HANDLE h;
  int n = 0;
  snprintf(pat, sizeof pat, "%s\\*", parent);
  h = FindFirstFileA(pat, &fd);
  if (h == INVALID_HANDLE_VALUE) return 0;
  do {
    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
    if (!strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..")) continue;
    if (n < maxn) {
      size_t L = strlen(fd.cFileName);
      if (L >= 256) L = 255;
      memcpy(names[n], fd.cFileName, L);
      names[n][L] = '\0';
      n++;
    }
  } while (FindNextFileA(h, &fd) && n < maxn);
  FindClose(h);
  return n;
}

static int dir_exists(const char *path) {
  DWORD a = GetFileAttributesA(path);
  return (a != INVALID_FILE_ATTRIBUTES &&
          (a & FILE_ATTRIBUTE_DIRECTORY));
}

#else

static int dir_exists(const char *path) {
  struct stat st;
  return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int list_subdirs(const char *parent, char names[][256], int maxn) {
  DIR *d = opendir(parent);
  struct dirent *e;
  int n = 0;
  if (!d) return 0;
  while ((e = readdir(d)) != NULL && n < maxn) {
    char sub[600];
    struct stat st;
    if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
    if (!path_join(sub, sizeof sub, parent, e->d_name)) continue;
    if (stat(sub, &st) != 0 || !S_ISDIR(st.st_mode)) continue;
    {
      size_t L = strlen(e->d_name);
      if (L >= 256) L = 255;
      memcpy(names[n], e->d_name, L);
      names[n][L] = '\0';
    }
    n++;
  }
  closedir(d);
  return n;
}

typedef int (*mod_file_cb)(const char *fullpath, const char *relpart, void *ctx);

static int foreach_file_in_dir(const char *dir, const char *ext,
                               mod_file_cb cb, void *ctx) {
  DIR *d;
  struct dirent *e;
  int n = 0;
  d = opendir(dir);
  if (!d) return 0;
  while ((e = readdir(d)) != NULL) {
    char full[600];
    if (!ends_with(e->d_name, ext)) continue;
    if (!path_join(full, sizeof full, dir, e->d_name)) continue;
    if (cb(full, e->d_name, ctx)) n++;
  }
  closedir(d);
  return n;
}
#endif

typedef struct {
  char name[256];
  int priority;
  char id[128];
  char title[192];
} PackSlot;

static int cmp_pack_load_order(const void *a, const void *b) {
  const PackSlot *x = (const PackSlot *)a;
  const PackSlot *y = (const PackSlot *)b;
  if (x->priority != y->priority) return x->priority - y->priority;
  return strcmp(x->name, y->name);
}

typedef struct {
  int priority;
  int enabled;
  char id[128];
  char title[192];
} ModManifest;

static void copy_manifest_value(const char *raw, char *dest, size_t destcap) {
  size_t L;
  if (!dest || destcap < 1) return;
  dest[0] = '\0';
  if (!raw) return;
  while (*raw == ' ' || *raw == '\t') raw++;
  L = strlen(raw);
  while (L > 0 && (raw[L - 1] == '\n' || raw[L - 1] == '\r' || raw[L - 1] == ' ' ||
                   raw[L - 1] == '\t'))
    L--;
  if (L >= destcap) L = destcap - 1;
  memcpy(dest, raw, L);
  dest[L] = '\0';
}

static void read_manifest(const char *pack_path, ModManifest *m) {
  char mf[650];
  FILE *fp;
  char line[320];
  char *p;
  m->priority = 0;
  m->enabled = 1;
  m->id[0] = '\0';
  m->title[0] = '\0';
  if (!path_join(mf, sizeof mf, pack_path, "manifest.txt")) return;
  fp = fopen(mf, "r");
  if (!fp) return;
  while (fgets(line, sizeof line, fp)) {
    trim_tail(line);
    p = line;
    while (*p == ' ' || *p == '\t') p++;
    if (*p == '#' || *p == '\r' || *p == '\n' || *p == '\0') continue;
    if (strncmp(p, "priority=", 9) == 0) m->priority = atoi(p + 9);
    else if (strncmp(p, "enabled=", 8) == 0) m->enabled = atoi(p + 8) != 0;
    else if (strncmp(p, "disabled=", 9) == 0) {
      if (atoi(p + 9) != 0) m->enabled = 0;
    } else if (strncmp(p, "id=", 3) == 0)
      copy_manifest_value(p + 3, m->id, sizeof m->id);
    else if (strncmp(p, "title=", 6) == 0)
      copy_manifest_value(p + 6, m->title, sizeof m->title);
  }
  fclose(fp);
}

typedef struct {
  const char *pack_name;
} ModFileCtx;

static void warn_unknown_slug(const char *pack, const char *subdir,
                              const char *fname, const char *slug) {
  if (g_warn_unknown_room_n >= AETER_MOD_WARN_CAP) return;
  snprintf(g_warn_unknown_room[g_warn_unknown_room_n],
           sizeof g_warn_unknown_room[0],
           "\"%s\"/%s/%s  —  unknown slug \"%s\" (skipped)", pack ? pack : "?",
           subdir ? subdir : "?", fname, slug);
  g_warn_unknown_room_n++;
}

static void warn_overlay_read(const char *pack, const char *subdir,
                              const char *fname) {
  if (g_warn_room_read_n >= AETER_MOD_WARN_CAP) return;
  snprintf(g_warn_room_read[g_warn_room_read_n], sizeof g_warn_room_read[0],
           "\"%s\"/%s/%s  —  could not read file", pack ? pack : "?",
           subdir ? subdir : "?", fname);
  g_warn_room_read_n++;
}

static int cb_room_blurb(const char *full, const char *fname, void *v) {
  char slug[192];
  int room;
  const ModFileCtx *ctx = (const ModFileCtx *)v;
  const char *pk = ctx && ctx->pack_name ? ctx->pack_name : "?";
  if (!ends_with(fname, ".txt")) return 0;
  if (ends_with(fname, ".append.txt") || ends_with(fname, ".prepend.txt"))
    return 0;
  strncpy(slug, fname, sizeof slug - 1);
  slug[sizeof slug - 1] = '\0';
  slug[strlen(slug) - 4] = '\0';
  room = world_room_index(slug);
  if (room < 0) {
    warn_unknown_slug(pk, "rooms", fname, slug);
    return 0;
  }
  {
    char *body = read_text_file(full, 256000);
    if (!body) {
      warn_overlay_read(pk, "rooms", fname);
      return 0;
    }
    if (!body[0]) {
      free(body);
      return 0;
    }
    set_blurb(room, body);
  }
  return 1;
}

static int cb_room_prepend(const char *full, const char *fname, void *v) {
  char slug[192];
  int room;
  char *body;
  size_t fl;
  const ModFileCtx *ctx = (const ModFileCtx *)v;
  const char *pk = ctx && ctx->pack_name ? ctx->pack_name : "?";
  if (!ends_with(fname, ".prepend.txt")) return 0;
  fl = strlen(fname);
  if (fl < 13u) return 0;
  strncpy(slug, fname, sizeof slug - 1);
  slug[sizeof slug - 1] = '\0';
  slug[fl - 12u] = '\0';
  room = world_room_index(slug);
  if (room < 0) {
    warn_unknown_slug(pk, "rooms", fname, slug);
    return 0;
  }
  body = read_text_file(full, 256000);
  if (!body) {
    warn_overlay_read(pk, "rooms", fname);
    return 0;
  }
  if (!body[0]) {
    free(body);
    return 0;
  }
  apply_room_prepend(room, body);
  return 1;
}

static int cb_room_append(const char *full, const char *fname, void *v) {
  char slug[192];
  int room;
  char *body;
  size_t fl;
  const ModFileCtx *ctx = (const ModFileCtx *)v;
  const char *pk = ctx && ctx->pack_name ? ctx->pack_name : "?";
  if (!ends_with(fname, ".append.txt")) return 0;
  fl = strlen(fname);
  if (fl < 12u) return 0;
  strncpy(slug, fname, sizeof slug - 1);
  slug[sizeof slug - 1] = '\0';
  slug[fl - 11u] = '\0';
  room = world_room_index(slug);
  if (room < 0) {
    warn_unknown_slug(pk, "rooms", fname, slug);
    return 0;
  }
  body = read_text_file(full, 256000);
  if (!body) {
    warn_overlay_read(pk, "rooms", fname);
    return 0;
  }
  if (!body[0]) {
    free(body);
    return 0;
  }
  apply_room_append(room, body);
  return 1;
}

static int cb_room_title(const char *full, const char *fname, void *v) {
  char slug[192];
  int room;
  const ModFileCtx *ctx = (const ModFileCtx *)v;
  const char *pk = ctx && ctx->pack_name ? ctx->pack_name : "?";
  if (!ends_with(fname, ".txt")) return 0;
  if (ends_with(fname, ".append.txt") || ends_with(fname, ".prepend.txt"))
    return 0;
  strncpy(slug, fname, sizeof slug - 1);
  slug[sizeof slug - 1] = '\0';
  slug[strlen(slug) - 4] = '\0';
  room = world_room_index(slug);
  if (room < 0) {
    warn_unknown_slug(pk, "titles", fname, slug);
    return 0;
  }
  {
    char *body = read_text_file(full, 8000);
    if (!body) {
      warn_overlay_read(pk, "titles", fname);
      return 0;
    }
    if (!body[0]) {
      free(body);
      return 0;
    }
    set_title(room, body);
  }
  return 1;
}

static int cb_item(const char *full, const char *fname, void *v) {
  char id[192];
  char *buf;
  char *idheap;
  (void)v;
  if (!ends_with(fname, ".txt")) return 0;
  strncpy(id, fname, sizeof id - 1);
  id[sizeof id - 1] = '\0';
  id[strlen(id) - 4] = '\0';
  buf = read_text_file(full, 128000);
  if (!buf) return 0;
  if (!buf[0]) {
    free(buf);
    return 0;
  }
  idheap = strdup(id);
  if (!idheap) {
    free(buf);
    return 0;
  }
  add_item_text(idheap, buf);
  return 1;
}

static int cb_npc_greeting(const char *full, const char *fname, void *v) {
  char stem[256];
  char *entity;
  char *text;
  (void)v;
  if (!ends_with(fname, ".greeting.txt")) return 0;
  strncpy(stem, fname, sizeof stem - 1);
  stem[sizeof stem - 1] = '\0';
  stem[strlen(stem) - 13] = '\0';
  text = read_text_file(full, 16000);
  if (!text) return 0;
  if (!text[0]) {
    free(text);
    return 0;
  }
  entity = strdup(stem);
  if (!entity) {
    free(text);
    return 0;
  }
  add_greeting(entity, text);
  return 1;
}

static int cb_npc_topic(const char *full, const char *fname, void *v) {
  char work[256];
  char *sep;
  char *entity;
  char *keyword;
  char *text;
  (void)v;
  if (ends_with(fname, ".greeting.txt")) return 0;
  if (!ends_with(fname, ".txt")) return 0;
  strncpy(work, fname, sizeof work - 1);
  work[sizeof work - 1] = '\0';
  work[strlen(work) - 4] = '\0';
  sep = strstr(work, "__");
  if (!sep) return 0;
  *sep = '\0';
  entity = strdup(work);
  keyword = strdup(sep + 2);
  if (!entity || !keyword) {
    free(entity);
    free(keyword);
    return 0;
  }
  text = read_text_file(full, 32000);
  if (!text) {
    free(entity);
    free(keyword);
    return 0;
  }
  if (!text[0]) {
    free(entity);
    free(keyword);
    free(text);
    return 0;
  }
  lc_inplace(keyword);
  add_topic(entity, keyword, text);
  return 1;
}

static void load_character_overlays(const char *pack_path) {
  char base[600], fp[650];
  char *t;
  if (!path_join(base, sizeof base, pack_path, "character")) return;
  if (!dir_exists(base)) return;
  if (path_join(fp, sizeof fp, base, "sheet_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_sheet_append);
      g_char_sheet_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "portrait_append.txt")) {
    t = read_text_file(fp, 24000);
    if (t) {
      free(g_char_portrait_append);
      g_char_portrait_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "aptitudes_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_aptitudes_append);
      g_char_aptitudes_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "reputation_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_reputation_append);
      g_char_reputation_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "loadout_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_loadout_append);
      g_char_loadout_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "traits_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_traits_append);
      g_char_traits_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "momentum_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_momentum_append);
      g_char_momentum_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "perks_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_perks_append);
      g_char_perks_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "voice_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_voice_append);
      g_char_voice_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "bio_append.txt")) {
    t = read_text_file(fp, 24000);
    if (t) {
      free(g_char_bio_append);
      g_char_bio_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "tainting_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_tainting_append);
      g_char_tainting_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "rapport_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_rapport_append);
      g_char_rapport_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "objectives_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_objectives_append);
      g_char_objectives_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "vitals_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_vitals_append);
      g_char_vitals_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "examine_append.txt")) {
    t = read_text_file(fp, 4096);
    if (t) {
      free(g_char_examine_append);
      g_char_examine_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "notes_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_notes_append);
      g_char_notes_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "hints_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_hints_append);
      g_char_hints_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "journal_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_journal_append);
      g_char_journal_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "progress_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_progress_append);
      g_char_progress_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "inventory_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_inventory_append);
      g_char_inventory_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "waypoints_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_waypoints_append);
      g_char_waypoints_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "status_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_status_append);
      g_char_status_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "score_append.txt")) {
    t = read_text_file(fp, 4096);
    if (t) {
      free(g_char_score_append);
      g_char_score_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "time_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_time_append);
      g_char_time_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "weather_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_weather_append);
      g_char_weather_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "room_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_room_append);
      g_char_room_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "recap_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_recap_append);
      g_char_recap_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "help_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_help_append);
      g_char_help_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "about_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_about_append);
      g_char_about_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "lights_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_lights_append);
      g_char_lights_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "exits_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_exits_append);
      g_char_exits_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "scan_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_scan_append);
      g_char_scan_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "trail_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_trail_append);
      g_char_trail_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "nearby_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_nearby_append);
      g_char_nearby_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "lockcheck_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_lockcheck_append);
      g_char_lockcheck_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "noise_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_noise_append);
      g_char_noise_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "nav_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_nav_append);
      g_char_nav_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "route_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_route_append);
      g_char_route_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "loot_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_loot_append);
      g_char_loot_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "compare_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_compare_append);
      g_char_compare_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "people_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_people_append);
      g_char_people_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "diagnostics_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_diagnostics_append);
      g_char_diagnostics_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "wares_append.txt")) {
    t = read_text_file(fp, 12000);
    if (t) {
      free(g_char_wares_append);
      g_char_wares_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "saves_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_saves_append);
      g_char_saves_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "hints_panel_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_hints_panel_append);
      g_char_hints_panel_append = t;
    }
  }
  if (path_join(fp, sizeof fp, base, "notes_panel_append.txt")) {
    t = read_text_file(fp, 8192);
    if (t) {
      free(g_char_notes_panel_append);
      g_char_notes_panel_append = t;
    }
  }
}

static void load_one_pack(const char *pack_path, const char *pack_name) {
  char sub[600];
  ModFileCtx ctx;
  ctx.pack_name = pack_name;
  load_character_overlays(pack_path);
  if (!path_join(sub, sizeof sub, pack_path, "rooms"))
    g_warn_path_join++;
  else if (dir_exists(sub)) {
    (void)foreach_file_in_dir(sub, ".txt", cb_room_blurb, &ctx);
    (void)foreach_file_in_dir(sub, ".prepend.txt", cb_room_prepend, &ctx);
    (void)foreach_file_in_dir(sub, ".append.txt", cb_room_append, &ctx);
  }
  if (!path_join(sub, sizeof sub, pack_path, "titles"))
    g_warn_path_join++;
  else if (dir_exists(sub))
    (void)foreach_file_in_dir(sub, ".txt", cb_room_title, &ctx);
  if (!path_join(sub, sizeof sub, pack_path, "items"))
    g_warn_path_join++;
  else if (dir_exists(sub))
    (void)foreach_file_in_dir(sub, ".txt", cb_item, NULL);
  if (!path_join(sub, sizeof sub, pack_path, "npcs"))
    g_warn_path_join++;
  else if (dir_exists(sub)) {
    (void)foreach_file_in_dir(sub, ".greeting.txt", cb_npc_greeting, NULL);
    (void)foreach_file_in_dir(sub, ".txt", cb_npc_topic, NULL);
  }
  load_crafting_tables(pack_path);
}

void aet_mods_init(const char *mods_directory) {
  char raw_names[AETER_MODS_MAX_PACKS][256];
  PackSlot slots[AETER_MODS_MAX_PACKS];
  int np, ns, i;
  char root[520];

  aet_mods_shutdown();
  g_warn_unknown_room_n = 0;
  g_warn_room_read_n = 0;
  g_warn_path_join = 0;

  if (!mods_directory || !mods_directory[0]) return;
  strncpy(root, mods_directory, sizeof root - 1);
  root[sizeof root - 1] = '\0';
  strncpy(g_mods_root, root, sizeof g_mods_root - 1);
  g_mods_root[sizeof g_mods_root - 1] = '\0';

  if (!dir_exists(root)) {
    g_status_summary = strdup("(mods folder not found — create it beside the "
                              "save, or set AETER_MODS to a directory path.)");
    return;
  }

  g_world_n = WORLD_ROOM_COUNT;
  g_blurb_ov = (char **)calloc((size_t)g_world_n, sizeof *g_blurb_ov);
  g_title_ov = (char **)calloc((size_t)g_world_n, sizeof *g_title_ov);
  if (!g_blurb_ov || !g_title_ov) {
    free_all();
    return;
  }

  np = list_subdirs(root, raw_names, AETER_MODS_MAX_PACKS);
  ns = 0;
  g_loaded_pack_n = 0;
  for (i = 0; i < np; i++) {
    char ppath[600];
    ModManifest mf;
    if (raw_names[i][0] == '_' || raw_names[i][0] == '.') continue;
    if (!path_join(ppath, sizeof ppath, root, raw_names[i])) continue;
    if (!dir_exists(ppath)) continue;
    read_manifest(ppath, &mf);
    if (!mf.enabled) continue;
    {
      size_t L = strlen(raw_names[i]);
      if (L >= sizeof slots[ns].name) L = sizeof slots[ns].name - 1u;
      memcpy(slots[ns].name, raw_names[i], L);
      slots[ns].name[L] = '\0';
    }
    slots[ns].priority = mf.priority;
    snprintf(slots[ns].id, sizeof slots[ns].id, "%s", mf.id);
    snprintf(slots[ns].title, sizeof slots[ns].title, "%s", mf.title);
    ns++;
  }
  qsort(slots, (size_t)ns, sizeof slots[0], cmp_pack_load_order);
  for (i = 0; i < ns; i++) {
    char ppath[600];
    if (!path_join(ppath, sizeof ppath, root, slots[i].name)) continue;
    load_one_pack(ppath, slots[i].name);
    if (g_loaded_pack_n < AETER_MODS_MAX_PACKS) {
      {
        size_t Ln = strlen(slots[i].name);
        if (Ln >= sizeof g_loaded_packs[g_loaded_pack_n])
          Ln = sizeof g_loaded_packs[g_loaded_pack_n] - 1u;
        memcpy(g_loaded_packs[g_loaded_pack_n], slots[i].name, Ln);
        g_loaded_packs[g_loaded_pack_n][Ln] = '\0';
      }
      g_loaded_pack_pri[g_loaded_pack_n] = slots[i].priority;
      snprintf(g_loaded_pack_id[g_loaded_pack_n],
               sizeof g_loaded_pack_id[g_loaded_pack_n], "%s", slots[i].id);
      snprintf(g_loaded_pack_title[g_loaded_pack_n],
               sizeof g_loaded_pack_title[g_loaded_pack_n], "%s", slots[i].title);
      g_loaded_pack_n++;
    }
  }

  {
    size_t cap = 3680 + (size_t)ns * 142;
    char *s = (char *)malloc(cap);
    int c = 0, t = 0, j;
    if (s) {
      for (j = 0; j < g_world_n; j++) {
        if (g_blurb_ov[j]) c++;
        if (g_title_ov[j]) t++;
      }
      snprintf(s, cap,
               "Mods root: %s\n"
               "Packs loaded: %d (sort: manifest priority= ascending, then "
               "folder name; last pack wins overlaps)\n"
               "Room blurbs patched: %d   Titles: %d\n"
               "Item examine overrides: %d\n"
               "Craft profiles: %d   Craft archetypes: %d\n"
               "NPC greetings: %d   NPC topic files: %d\n"
               "Character text: sheet %s | portrait %s | aptitudes %s |\n"
               "               reputation %s | loadout %s | traits %s |\n"
               "               momentum %s | perks %s |\n"
               "               voice %s | bio %s |\n"
               "               tainting %s | rapport %s |\n"
               "               objectives %s | journal %s | progress %s |\n"
               "               vitals %s | inventory %s | waypoints %s |\n"
               "               score %s | status %s | time %s | weather %s |\n"
               "               rpanel %s | recap %s | exits %s | scan %s |\n"
               "               help %s | about %s | lights %s |\n"
               "               trail %s | nearby %s | lockcheck %s | noise %s |\n"
               "               nav %s | route %s | loot %s | compare %s | "
               "people %s |\n"
               "               diag %s | wares %s | saves %s |\n"
               "               hints+ %s | notes+ %s\n\n"
               "In-game:  help modding  |  mods list  (load order)\n",
               root, ns, c, t, g_items_n, g_craft_profiles_n, g_craft_arch_n,
               g_greetings_n, g_topics_n,
               (g_char_sheet_append && g_char_sheet_append[0]) ? "yes" : "no",
               (g_char_portrait_append && g_char_portrait_append[0]) ? "yes"
                                                                     : "no",
               (g_char_aptitudes_append && g_char_aptitudes_append[0]) ? "yes"
                                                                         : "no",
               (g_char_reputation_append && g_char_reputation_append[0])
                   ? "yes"
                   : "no",
               (g_char_loadout_append && g_char_loadout_append[0]) ? "yes"
                                                                   : "no",
               (g_char_traits_append && g_char_traits_append[0]) ? "yes"
                                                                 : "no",
               (g_char_momentum_append && g_char_momentum_append[0]) ? "yes"
                                                                     : "no",
               (g_char_perks_append && g_char_perks_append[0]) ? "yes"
                                                               : "no",
               (g_char_voice_append && g_char_voice_append[0]) ? "yes"
                                                                 : "no",
               (g_char_bio_append && g_char_bio_append[0]) ? "yes" : "no",
               (g_char_tainting_append && g_char_tainting_append[0]) ? "yes"
                                                                      : "no",
               (g_char_rapport_append && g_char_rapport_append[0]) ? "yes"
                                                                     : "no",
               (g_char_objectives_append && g_char_objectives_append[0]) ? "yes"
                                                                         : "no",
               (g_char_journal_append && g_char_journal_append[0]) ? "yes"
                                                                     : "no",
               (g_char_progress_append && g_char_progress_append[0]) ? "yes"
                                                                       : "no",
               (g_char_vitals_append && g_char_vitals_append[0]) ? "yes"
                                                                   : "no",
               (g_char_inventory_append && g_char_inventory_append[0]) ? "yes"
                                                                         : "no",
               (g_char_waypoints_append && g_char_waypoints_append[0]) ? "yes"
                                                                       : "no",
               (g_char_score_append && g_char_score_append[0]) ? "yes" : "no",
               (g_char_status_append && g_char_status_append[0]) ? "yes"
                                                                 : "no",
               (g_char_time_append && g_char_time_append[0]) ? "yes" : "no",
               (g_char_weather_append && g_char_weather_append[0]) ? "yes"
                                                                    : "no",
               (g_char_room_append && g_char_room_append[0]) ? "yes" : "no",
               (g_char_recap_append && g_char_recap_append[0]) ? "yes" : "no",
               (g_char_exits_append && g_char_exits_append[0]) ? "yes" : "no",
               (g_char_scan_append && g_char_scan_append[0]) ? "yes" : "no",
               (g_char_help_append && g_char_help_append[0]) ? "yes" : "no",
               (g_char_about_append && g_char_about_append[0]) ? "yes" : "no",
               (g_char_lights_append && g_char_lights_append[0]) ? "yes"
                                                                    : "no",
               (g_char_trail_append && g_char_trail_append[0]) ? "yes" : "no",
               (g_char_nearby_append && g_char_nearby_append[0]) ? "yes"
                                                                  : "no",
               (g_char_lockcheck_append && g_char_lockcheck_append[0]) ? "yes"
                                                                       : "no",
               (g_char_noise_append && g_char_noise_append[0]) ? "yes" : "no",
               (g_char_nav_append && g_char_nav_append[0]) ? "yes" : "no",
               (g_char_route_append && g_char_route_append[0]) ? "yes" : "no",
               (g_char_loot_append && g_char_loot_append[0]) ? "yes" : "no",
               (g_char_compare_append && g_char_compare_append[0]) ? "yes"
                                                                     : "no",
               (g_char_people_append && g_char_people_append[0]) ? "yes"
                                                                  : "no",
               (g_char_diagnostics_append && g_char_diagnostics_append[0]) ? "yes"
                                                                           : "no",
               (g_char_wares_append && g_char_wares_append[0]) ? "yes" : "no",
               (g_char_saves_append && g_char_saves_append[0]) ? "yes" : "no",
               (g_char_hints_panel_append && g_char_hints_panel_append[0]) ? "yes"
                                                                            : "no",
               (g_char_notes_panel_append && g_char_notes_panel_append[0]) ? "yes"
                                                                           : "no");
      g_status_summary = s;
    }
  }
}

void aet_mods_shutdown(void) { free_all(); }

void aet_mods_reload(const char *mods_directory) {
  aet_mods_init(mods_directory);
}

const char *aet_mods_room_blurb(int room_index) {
  if (!g_blurb_ov || room_index < 0 || room_index >= g_world_n) return NULL;
  return g_blurb_ov[room_index];
}

const char *aet_mods_room_title(int room_index) {
  if (!g_title_ov || room_index < 0 || room_index >= g_world_n) return NULL;
  return g_title_ov[room_index];
}

int aet_mods_item_description(const char *item_id, char *out, size_t outcap) {
  int i;
  if (!item_id || !out || outcap < 2) return 0;
  for (i = 0; i < g_items_n; i++) {
    if (str_ieq_local(g_items[i].id, item_id)) {
      snprintf(out, outcap, "%s", g_items[i].text);
      return 1;
    }
  }
  return 0;
}

const char *aet_mods_npc_greeting(const char *entity_slug) {
  int i;
  if (!entity_slug) return NULL;
  for (i = 0; i < g_greetings_n; i++) {
    if (str_ieq_local(g_greetings[i].entity, entity_slug))
      return g_greetings[i].text;
  }
  return NULL;
}

const char *aet_mods_npc_topic_response(const char *entity_slug,
                                      const char *topic_phrase_lc) {
  int i;
  const char *best = NULL;
  size_t best_len = 0;
  if (!entity_slug || !topic_phrase_lc) return NULL;
  for (i = 0; i < g_topics_n; i++) {
    size_t klen;
    if (!str_ieq_local(g_topics[i].entity, entity_slug)) continue;
    if (!g_topics[i].keyword || strstr(topic_phrase_lc, g_topics[i].keyword) == NULL)
      continue;
    klen = strlen(g_topics[i].keyword);
    if (klen >= best_len) {
      best_len = klen;
      best = g_topics[i].text;
    }
  }
  return best;
}

int aet_mods_crafting_profile(const char *item_id, char *mat_class,
                              size_t mat_class_cap, int *is_base, int *hrd,
                              int *shp, int *flx, int *dur, int *wgt, int *grp,
                              int *bnd, int *utl) {
  int i;
  if (!item_id) return 0;
  for (i = 0; i < g_craft_profiles_n; i++) {
    const CraftProfileMod *p = &g_craft_profiles[i];
    if (!str_ieq_local(p->id, item_id)) continue;
    if (mat_class && mat_class_cap > 0)
      snprintf(mat_class, mat_class_cap, "%s", p->mat_class ? p->mat_class : "Mixed");
    if (is_base) *is_base = p->is_base;
    if (hrd) *hrd = p->hrd;
    if (shp) *shp = p->shp;
    if (flx) *flx = p->flx;
    if (dur) *dur = p->dur;
    if (wgt) *wgt = p->wgt;
    if (grp) *grp = p->grp;
    if (bnd) *bnd = p->bnd;
    if (utl) *utl = p->utl;
    return 1;
  }
  return 0;
}

int aet_mods_crafting_archetype_count(void) { return g_craft_arch_n; }

int aet_mods_crafting_archetype_get(int idx, AetCraftArchetype *out) {
  if (!out || idx < 0 || idx >= g_craft_arch_n) return 0;
  snprintf(out->name, sizeof out->name, "%s", g_craft_arch[idx].name ? g_craft_arch[idx].name : "Tool");
  out->req_hrd = g_craft_arch[idx].req_hrd;
  out->req_shp = g_craft_arch[idx].req_shp;
  out->req_dur = g_craft_arch[idx].req_dur;
  out->req_bnd = g_craft_arch[idx].req_bnd;
  out->req_grp = g_craft_arch[idx].req_grp;
  out->req_flx = g_craft_arch[idx].req_flx;
  return 1;
}

void aet_mods_format_status(char *buf, size_t cap) {
  if (!buf || cap < 4) return;
  if (g_status_summary)
    snprintf(buf, cap, "%s", g_status_summary);
  else
    snprintf(buf, cap, "Mods not initialized.\n");
}

void aet_mods_format_load_warnings(char *buf, size_t cap) {
  size_t used;
  int i;
  if (!buf || cap < 8) return;
  buf[0] = '\0';
  if (!g_warn_unknown_room_n && !g_warn_room_read_n && !g_warn_path_join)
    return;
  used = 0;
  if (g_warn_path_join > 0)
    used += (size_t)snprintf(
        buf + used, cap > used ? cap - used : 0,
        "Mod paths truncated (%d); shorten pack or mods root path.\n\n",
        g_warn_path_join);
  if (g_warn_unknown_room_n > 0) {
    used += (size_t)snprintf(buf + used, cap > used ? cap - used : 0,
                             "Room/title overlays skipped (slug not in world):\n");
    for (i = 0; i < g_warn_unknown_room_n && used + 2 < cap; i++)
      used += (size_t)snprintf(buf + used, cap > used ? cap - used : 0,
                               "  %s\n", g_warn_unknown_room[i]);
    used += (size_t)snprintf(buf + used, cap > used ? cap - used : 0, "\n");
  }
  if (g_warn_room_read_n > 0) {
    used += (size_t)snprintf(buf + used, cap > used ? cap - used : 0,
                             "Unreadable overlay files:\n");
    for (i = 0; i < g_warn_room_read_n && used + 2 < cap; i++)
      used += (size_t)snprintf(buf + used, cap > used ? cap - used : 0,
                               "  %s\n", g_warn_room_read[i]);
  }
}

const char *aet_mods_character_sheet_suffix(void) {
  if (g_char_sheet_append && g_char_sheet_append[0]) return g_char_sheet_append;
  return NULL;
}

const char *aet_mods_character_portrait_suffix(void) {
  if (g_char_portrait_append && g_char_portrait_append[0])
    return g_char_portrait_append;
  return NULL;
}

const char *aet_mods_character_aptitudes_suffix(void) {
  if (g_char_aptitudes_append && g_char_aptitudes_append[0])
    return g_char_aptitudes_append;
  return NULL;
}

const char *aet_mods_character_reputation_suffix(void) {
  if (g_char_reputation_append && g_char_reputation_append[0])
    return g_char_reputation_append;
  return NULL;
}

const char *aet_mods_character_loadout_suffix(void) {
  if (g_char_loadout_append && g_char_loadout_append[0])
    return g_char_loadout_append;
  return NULL;
}

const char *aet_mods_character_traits_suffix(void) {
  if (g_char_traits_append && g_char_traits_append[0])
    return g_char_traits_append;
  return NULL;
}

const char *aet_mods_character_momentum_suffix(void) {
  if (g_char_momentum_append && g_char_momentum_append[0])
    return g_char_momentum_append;
  return NULL;
}

const char *aet_mods_character_perks_suffix(void) {
  if (g_char_perks_append && g_char_perks_append[0])
    return g_char_perks_append;
  return NULL;
}

const char *aet_mods_character_voice_suffix(void) {
  if (g_char_voice_append && g_char_voice_append[0])
    return g_char_voice_append;
  return NULL;
}

const char *aet_mods_character_bio_suffix(void) {
  if (g_char_bio_append && g_char_bio_append[0])
    return g_char_bio_append;
  return NULL;
}

const char *aet_mods_character_tainting_suffix(void) {
  if (g_char_tainting_append && g_char_tainting_append[0])
    return g_char_tainting_append;
  return NULL;
}

const char *aet_mods_character_rapport_suffix(void) {
  if (g_char_rapport_append && g_char_rapport_append[0])
    return g_char_rapport_append;
  return NULL;
}

const char *aet_mods_character_objectives_suffix(void) {
  if (g_char_objectives_append && g_char_objectives_append[0])
    return g_char_objectives_append;
  return NULL;
}

const char *aet_mods_character_vitals_suffix(void) {
  if (g_char_vitals_append && g_char_vitals_append[0])
    return g_char_vitals_append;
  return NULL;
}

const char *aet_mods_character_examine_suffix(void) {
  if (g_char_examine_append && g_char_examine_append[0])
    return g_char_examine_append;
  return NULL;
}

const char *aet_mods_character_notes_suffix(void) {
  if (g_char_notes_append && g_char_notes_append[0])
    return g_char_notes_append;
  return NULL;
}

const char *aet_mods_character_hints_suffix(void) {
  if (g_char_hints_append && g_char_hints_append[0])
    return g_char_hints_append;
  return NULL;
}

const char *aet_mods_character_hints_panel_suffix(void) {
  if (g_char_hints_panel_append && g_char_hints_panel_append[0])
    return g_char_hints_panel_append;
  return NULL;
}

const char *aet_mods_character_notes_panel_suffix(void) {
  if (g_char_notes_panel_append && g_char_notes_panel_append[0])
    return g_char_notes_panel_append;
  return NULL;
}

const char *aet_mods_character_journal_suffix(void) {
  if (g_char_journal_append && g_char_journal_append[0])
    return g_char_journal_append;
  return NULL;
}

const char *aet_mods_character_progress_suffix(void) {
  if (g_char_progress_append && g_char_progress_append[0])
    return g_char_progress_append;
  return NULL;
}

const char *aet_mods_character_inventory_suffix(void) {
  if (g_char_inventory_append && g_char_inventory_append[0])
    return g_char_inventory_append;
  return NULL;
}

const char *aet_mods_character_waypoints_suffix(void) {
  if (g_char_waypoints_append && g_char_waypoints_append[0])
    return g_char_waypoints_append;
  return NULL;
}

const char *aet_mods_character_status_suffix(void) {
  if (g_char_status_append && g_char_status_append[0])
    return g_char_status_append;
  return NULL;
}

const char *aet_mods_character_score_suffix(void) {
  if (g_char_score_append && g_char_score_append[0])
    return g_char_score_append;
  return NULL;
}

const char *aet_mods_character_time_suffix(void) {
  if (g_char_time_append && g_char_time_append[0]) return g_char_time_append;
  return NULL;
}

const char *aet_mods_character_weather_suffix(void) {
  if (g_char_weather_append && g_char_weather_append[0])
    return g_char_weather_append;
  return NULL;
}

const char *aet_mods_character_room_suffix(void) {
  if (g_char_room_append && g_char_room_append[0]) return g_char_room_append;
  return NULL;
}

const char *aet_mods_character_recap_suffix(void) {
  if (g_char_recap_append && g_char_recap_append[0]) return g_char_recap_append;
  return NULL;
}

const char *aet_mods_character_help_suffix(void) {
  if (g_char_help_append && g_char_help_append[0]) return g_char_help_append;
  return NULL;
}

const char *aet_mods_character_about_suffix(void) {
  if (g_char_about_append && g_char_about_append[0]) return g_char_about_append;
  return NULL;
}

const char *aet_mods_character_lights_suffix(void) {
  if (g_char_lights_append && g_char_lights_append[0]) return g_char_lights_append;
  return NULL;
}

const char *aet_mods_character_exits_suffix(void) {
  if (g_char_exits_append && g_char_exits_append[0]) return g_char_exits_append;
  return NULL;
}

const char *aet_mods_character_scan_suffix(void) {
  if (g_char_scan_append && g_char_scan_append[0]) return g_char_scan_append;
  return NULL;
}

const char *aet_mods_character_trail_suffix(void) {
  if (g_char_trail_append && g_char_trail_append[0]) return g_char_trail_append;
  return NULL;
}

const char *aet_mods_character_nearby_suffix(void) {
  if (g_char_nearby_append && g_char_nearby_append[0]) return g_char_nearby_append;
  return NULL;
}

const char *aet_mods_character_lockcheck_suffix(void) {
  if (g_char_lockcheck_append && g_char_lockcheck_append[0])
    return g_char_lockcheck_append;
  return NULL;
}

const char *aet_mods_character_noise_suffix(void) {
  if (g_char_noise_append && g_char_noise_append[0]) return g_char_noise_append;
  return NULL;
}

const char *aet_mods_character_nav_suffix(void) {
  if (g_char_nav_append && g_char_nav_append[0]) return g_char_nav_append;
  return NULL;
}

const char *aet_mods_character_route_suffix(void) {
  if (g_char_route_append && g_char_route_append[0]) return g_char_route_append;
  return NULL;
}

const char *aet_mods_character_loot_suffix(void) {
  if (g_char_loot_append && g_char_loot_append[0]) return g_char_loot_append;
  return NULL;
}

const char *aet_mods_character_compare_suffix(void) {
  if (g_char_compare_append && g_char_compare_append[0]) return g_char_compare_append;
  return NULL;
}

const char *aet_mods_character_people_suffix(void) {
  if (g_char_people_append && g_char_people_append[0]) return g_char_people_append;
  return NULL;
}

const char *aet_mods_character_diagnostics_suffix(void) {
  if (g_char_diagnostics_append && g_char_diagnostics_append[0])
    return g_char_diagnostics_append;
  return NULL;
}

const char *aet_mods_character_wares_suffix(void) {
  if (g_char_wares_append && g_char_wares_append[0]) return g_char_wares_append;
  return NULL;
}

const char *aet_mods_character_saves_suffix(void) {
  if (g_char_saves_append && g_char_saves_append[0]) return g_char_saves_append;
  return NULL;
}

void aet_mods_format_load_order(char *buf, size_t cap) {
  int i;
  size_t used;
  if (!buf || cap < 64) return;
  if (g_loaded_pack_n <= 0) {
    snprintf(buf, cap,
             "(No packs loaded — missing mods folder, all packs disabled in "
             "manifest.txt, or mods not initialized yet.)\n");
    return;
  }
  used = (size_t)snprintf(
      buf, cap,
      "Pack load order (first -> last; later wins on conflicts):\n");
  if (used >= cap) return;
  for (i = 0; i < g_loaded_pack_n; i++) {
    int w = snprintf(buf + used, cap > used ? cap - used : 0, "  [%4d]  %s",
                     g_loaded_pack_pri[i], g_loaded_packs[i]);
    if (w < 0 || (size_t)w >= cap - used) break;
    used += (size_t)w;
    if (g_loaded_pack_title[i][0]) {
      w = snprintf(buf + used, cap > used ? cap - used : 0, " — %s",
                   g_loaded_pack_title[i]);
      if (w < 0 || (size_t)w >= cap - used) break;
      used += (size_t)w;
    }
    if (g_loaded_pack_id[i][0]) {
      w = snprintf(buf + used, cap > used ? cap - used : 0, "  (%s)",
                   g_loaded_pack_id[i]);
      if (w < 0 || (size_t)w >= cap - used) break;
      used += (size_t)w;
    }
    w = snprintf(buf + used, cap > used ? cap - used : 0, "\n");
    if (w < 0 || (size_t)w >= cap - used) break;
    used += (size_t)w;
  }
}
