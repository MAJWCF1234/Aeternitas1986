#include "aeternitas_char_creation.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static AetPcSave g_pc;

static void copy_s(char *dst, size_t cap, const char *src) {
  if (!dst || cap < 1) return;
  if (!src) src = "";
  strncpy(dst, src, cap - 1);
  dst[cap - 1] = '\0';
}

static int ieq(const char *a, const char *b) {
  unsigned char ca, cb;
  if (!a || !b) return 0;
  while (*a && *b) {
    ca = (unsigned char)*a++;
    cb = (unsigned char)*b++;
    if (tolower(ca) != tolower(cb)) return 0;
  }
  return *a == '\0' && *b == '\0';
}

void pc_set_default_adventurer(void) {
  memset(&g_pc, 0, sizeof g_pc);
  copy_s(g_pc.name, sizeof g_pc.name, "Wanderer");
  copy_s(g_pc.race, sizeof g_pc.race, "Human");
  copy_s(g_pc.skin, sizeof g_pc.skin, "Fair");
  copy_s(g_pc.gender, sizeof g_pc.gender, "they");
  copy_s(g_pc.genitalia, sizeof g_pc.genitalia, "none");
  copy_s(g_pc.class_, sizeof g_pc.class_, "adventurer");
  copy_s(g_pc.height, sizeof g_pc.height, "Average");
  copy_s(g_pc.build, sizeof g_pc.build, "Average");
  copy_s(g_pc.hair_color, sizeof g_pc.hair_color, "Brown");
  copy_s(g_pc.hair_style, sizeof g_pc.hair_style, "Short");
  copy_s(g_pc.eye_color, sizeof g_pc.eye_color, "Brown");
  copy_s(g_pc.armor, sizeof g_pc.armor, "None");
  copy_s(g_pc.weapon, sizeof g_pc.weapon, "None");
  g_pc.str = 10;
  g_pc.tou = 10;
  g_pc.spe = 10;
  g_pc.intl = 10;
  g_pc.wis = 10;
  g_pc.will = 10;
  g_pc.agi = 10;
  g_pc.cha = 10;
  g_pc.per = 10;
  g_pc.cor = 0;
  copy_s(g_pc.muscle_tone, sizeof g_pc.muscle_tone, "Average");
  copy_s(g_pc.body_fat, sizeof g_pc.body_fat, "Average");
  copy_s(g_pc.hips, sizeof g_pc.hips, "Average");
  copy_s(g_pc.butt, sizeof g_pc.butt, "Average");
  copy_s(g_pc.age, sizeof g_pc.age, "Adult");
  copy_s(g_pc.voice_pitch, sizeof g_pc.voice_pitch, "Mid");
  copy_s(g_pc.voice_quality, sizeof g_pc.voice_quality, "Clear");
  g_pc.scars = 0;
  g_pc.tattoos = 0;
  copy_s(g_pc.breasts, sizeof g_pc.breasts, "None");
  copy_s(g_pc.cock, sizeof g_pc.cock, "None");
  copy_s(g_pc.balls, sizeof g_pc.balls, "None");
  copy_s(g_pc.pussy, sizeof g_pc.pussy, "None");
  copy_s(g_pc.nipple_type, sizeof g_pc.nipple_type, "Normal");
  g_pc.milk = 0;
  g_pc.ball_fullness = 0;
}

void pc_reset_empty(void) { memset(&g_pc, 0, sizeof g_pc); }

void pc_fill_narrative_defaults(AetPcSave *io) {
  if (!io) return;
  if (!io->name[0]) copy_s(io->name, sizeof io->name, "Wanderer");
  if (!io->race[0]) copy_s(io->race, sizeof io->race, "Human");
  if (!io->gender[0]) copy_s(io->gender, sizeof io->gender, "they");
  if (!io->class_[0]) copy_s(io->class_, sizeof io->class_, "adventurer");
  if (!io->age[0]) copy_s(io->age, sizeof io->age, "Adult");
  if (!io->voice_pitch[0]) copy_s(io->voice_pitch, sizeof io->voice_pitch, "Mid");
  if (!io->voice_quality[0]) copy_s(io->voice_quality, sizeof io->voice_quality, "Clear");
}

void pc_capture(AetPcSave *out) {
  if (!out) return;
  *out = g_pc;
}

void pc_restore(const AetPcSave *in) {
  if (!in) return;
  g_pc = *in;
  pc_fill_narrative_defaults(&g_pc);
}

const char *pc_display_name(void) { return g_pc.name[0] ? g_pc.name : "Wanderer"; }

void pc_format_pronouns_short(const char *gender, char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  if (!gender) gender = "they";
  if (ieq(gender, "she") || ieq(gender, "female"))
    copy_s(out, outcap, "she/her");
  else if (ieq(gender, "he") || ieq(gender, "male"))
    copy_s(out, outcap, "he/him");
  else
    copy_s(out, outcap, "they/them");
}

void pc_format_identity_banner(char *out, size_t outcap) {
  char pr[32];
  if (!out || outcap < 2) return;
  pc_format_pronouns_short(g_pc.gender, pr, sizeof pr);
  snprintf(out, outcap, "%s | %s | %s", pc_display_name(),
           g_pc.class_[0] ? g_pc.class_ : "adventurer", pr);
}

void pc_format_hud_tag(char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  snprintf(out, outcap, "[%s | %s | COR %d]",
           g_pc.class_[0] ? g_pc.class_ : "adventurer",
           g_pc.race[0] ? g_pc.race : "Human", g_pc.cor);
}

void pc_format_role_phrase(char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  snprintf(out, outcap, "%s %s", g_pc.race[0] ? g_pc.race : "Human",
           g_pc.class_[0] ? g_pc.class_ : "adventurer");
}

void pc_expand_world_placeholders(const char *in, char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  if (!in) {
    out[0] = '\0';
    return;
  }
  copy_s(out, outcap, in);
}

static int write_kv_s(FILE *fp, const char *k, const char *v) {
  return fprintf(fp, "%s %s\n", k, v ? v : "") > 0;
}
static int write_kv_i(FILE *fp, const char *k, int v) { return fprintf(fp, "%s %d\n", k, v) > 0; }

int pc_write_save(FILE *fp) {
  if (!fp) return 0;
  if (!write_kv_s(fp, "CHARACTER", "")) return 0;
  write_kv_s(fp, "name", g_pc.name);
  write_kv_s(fp, "race", g_pc.race);
  write_kv_s(fp, "skin", g_pc.skin);
  write_kv_s(fp, "gender", g_pc.gender);
  write_kv_s(fp, "genitalia", g_pc.genitalia);
  write_kv_s(fp, "class", g_pc.class_);
  write_kv_s(fp, "height", g_pc.height);
  write_kv_s(fp, "build", g_pc.build);
  write_kv_s(fp, "hair_color", g_pc.hair_color);
  write_kv_s(fp, "hair_style", g_pc.hair_style);
  write_kv_s(fp, "eye_color", g_pc.eye_color);
  write_kv_s(fp, "armor", g_pc.armor);
  write_kv_s(fp, "weapon", g_pc.weapon);
  write_kv_i(fp, "str", g_pc.str);
  write_kv_i(fp, "tou", g_pc.tou);
  write_kv_i(fp, "spe", g_pc.spe);
  write_kv_i(fp, "intl", g_pc.intl);
  write_kv_i(fp, "wis", g_pc.wis);
  write_kv_i(fp, "will", g_pc.will);
  write_kv_i(fp, "agi", g_pc.agi);
  write_kv_i(fp, "cha", g_pc.cha);
  write_kv_i(fp, "per", g_pc.per);
  write_kv_i(fp, "cor", g_pc.cor);
  write_kv_s(fp, "muscle_tone", g_pc.muscle_tone);
  write_kv_s(fp, "body_fat", g_pc.body_fat);
  write_kv_s(fp, "hips", g_pc.hips);
  write_kv_s(fp, "butt", g_pc.butt);
  write_kv_s(fp, "age", g_pc.age);
  write_kv_s(fp, "voice_pitch", g_pc.voice_pitch);
  write_kv_s(fp, "voice_quality", g_pc.voice_quality);
  write_kv_i(fp, "scars", g_pc.scars);
  write_kv_i(fp, "tattoos", g_pc.tattoos);
  write_kv_s(fp, "breasts", g_pc.breasts);
  write_kv_s(fp, "cock", g_pc.cock);
  write_kv_s(fp, "balls", g_pc.balls);
  write_kv_s(fp, "pussy", g_pc.pussy);
  write_kv_s(fp, "nipple_type", g_pc.nipple_type);
  write_kv_i(fp, "milk", g_pc.milk);
  write_kv_i(fp, "ball_fullness", g_pc.ball_fullness);
  return fprintf(fp, "ENDCHAR\n") > 0;
}

static void parse_line(char *line) {
  char *sp = strchr(line, ' ');
  char *key = line;
  char *val = "";
  if (sp) {
    *sp = '\0';
    val = sp + 1;
    while (*val == ' ') val++;
  }
  if (ieq(key, "name")) copy_s(g_pc.name, sizeof g_pc.name, val);
  else if (ieq(key, "race")) copy_s(g_pc.race, sizeof g_pc.race, val);
  else if (ieq(key, "skin")) copy_s(g_pc.skin, sizeof g_pc.skin, val);
  else if (ieq(key, "gender")) copy_s(g_pc.gender, sizeof g_pc.gender, val);
  else if (ieq(key, "genitalia")) copy_s(g_pc.genitalia, sizeof g_pc.genitalia, val);
  else if (ieq(key, "class")) copy_s(g_pc.class_, sizeof g_pc.class_, val);
  else if (ieq(key, "height")) copy_s(g_pc.height, sizeof g_pc.height, val);
  else if (ieq(key, "build")) copy_s(g_pc.build, sizeof g_pc.build, val);
  else if (ieq(key, "hair_color")) copy_s(g_pc.hair_color, sizeof g_pc.hair_color, val);
  else if (ieq(key, "hair_style")) copy_s(g_pc.hair_style, sizeof g_pc.hair_style, val);
  else if (ieq(key, "eye_color")) copy_s(g_pc.eye_color, sizeof g_pc.eye_color, val);
  else if (ieq(key, "armor")) copy_s(g_pc.armor, sizeof g_pc.armor, val);
  else if (ieq(key, "weapon")) copy_s(g_pc.weapon, sizeof g_pc.weapon, val);
  else if (ieq(key, "str")) g_pc.str = atoi(val);
  else if (ieq(key, "tou")) g_pc.tou = atoi(val);
  else if (ieq(key, "spe")) g_pc.spe = atoi(val);
  else if (ieq(key, "intl")) g_pc.intl = atoi(val);
  else if (ieq(key, "wis")) g_pc.wis = atoi(val);
  else if (ieq(key, "will")) g_pc.will = atoi(val);
  else if (ieq(key, "agi")) g_pc.agi = atoi(val);
  else if (ieq(key, "cha")) g_pc.cha = atoi(val);
  else if (ieq(key, "per")) g_pc.per = atoi(val);
  else if (ieq(key, "cor")) g_pc.cor = atoi(val);
  else if (ieq(key, "muscle_tone")) copy_s(g_pc.muscle_tone, sizeof g_pc.muscle_tone, val);
  else if (ieq(key, "body_fat")) copy_s(g_pc.body_fat, sizeof g_pc.body_fat, val);
  else if (ieq(key, "hips")) copy_s(g_pc.hips, sizeof g_pc.hips, val);
  else if (ieq(key, "butt")) copy_s(g_pc.butt, sizeof g_pc.butt, val);
  else if (ieq(key, "age")) copy_s(g_pc.age, sizeof g_pc.age, val);
  else if (ieq(key, "voice_pitch")) copy_s(g_pc.voice_pitch, sizeof g_pc.voice_pitch, val);
  else if (ieq(key, "voice_quality")) copy_s(g_pc.voice_quality, sizeof g_pc.voice_quality, val);
  else if (ieq(key, "scars")) g_pc.scars = atoi(val);
  else if (ieq(key, "tattoos")) g_pc.tattoos = atoi(val);
  else if (ieq(key, "breasts")) copy_s(g_pc.breasts, sizeof g_pc.breasts, val);
  else if (ieq(key, "cock")) copy_s(g_pc.cock, sizeof g_pc.cock, val);
  else if (ieq(key, "balls")) copy_s(g_pc.balls, sizeof g_pc.balls, val);
  else if (ieq(key, "pussy")) copy_s(g_pc.pussy, sizeof g_pc.pussy, val);
  else if (ieq(key, "nipple_type")) copy_s(g_pc.nipple_type, sizeof g_pc.nipple_type, val);
  else if (ieq(key, "milk")) g_pc.milk = atoi(val);
  else if (ieq(key, "ball_fullness")) g_pc.ball_fullness = atoi(val);
}

int pc_read_save(FILE *fp, char *scratch, size_t scratch_cap) {
  if (!fp || !scratch || scratch_cap < 8) return 0;
  while (fgets(scratch, (int)scratch_cap, fp)) {
    size_t n = strlen(scratch);
    while (n > 0 && (scratch[n - 1] == '\n' || scratch[n - 1] == '\r')) scratch[--n] = '\0';
    if (!scratch[0]) continue;
    if (ieq(scratch, "ENDCHAR")) {
      pc_fill_narrative_defaults(&g_pc);
      return 1;
    }
    parse_line(scratch);
  }
  pc_fill_narrative_defaults(&g_pc);
  return 1;
}

void pc_format_summary(char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  snprintf(out, outcap,
           "%s | %s | %s\nSTR %d TOU %d SPE %d AGI %d INT %d WIS %d WILL %d CHA %d PER %d COR %d",
           pc_display_name(), g_pc.race[0] ? g_pc.race : "Human",
           g_pc.class_[0] ? g_pc.class_ : "adventurer", g_pc.str, g_pc.tou, g_pc.spe,
           g_pc.agi, g_pc.intl, g_pc.wis, g_pc.will, g_pc.cha, g_pc.per, g_pc.cor);
}

void run_character_creation(int autotest_mode) {
  (void)autotest_mode;
  if (!g_pc.name[0]) pc_set_default_adventurer();
  pc_fill_narrative_defaults(&g_pc);
}
