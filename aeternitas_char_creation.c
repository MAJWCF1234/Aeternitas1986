#include "aeternitas_char_creation.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
  copy_s(g_pc.voice_pitch, sizeof g_pc.voice_pitch, "Tenor");
  copy_s(g_pc.voice_quality, sizeof g_pc.voice_quality, "Smooth");
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
  if (!io->voice_pitch[0]) copy_s(io->voice_pitch, sizeof io->voice_pitch, "Tenor");
  if (!io->voice_quality[0]) copy_s(io->voice_quality, sizeof io->voice_quality, "Smooth");
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
    copy_s(out, outcap, "she / her / her");
  else if (ieq(gender, "he") || ieq(gender, "male"))
    copy_s(out, outcap, "he / him / his");
  else
    copy_s(out, outcap, "they / them / their");
}

void pc_format_identity_banner(char *out, size_t outcap) {
  char pr[32];
  if (!out || outcap < 2) return;
  pc_format_pronouns_short(g_pc.gender, pr, sizeof pr);
  {
    char role[96];
    pc_format_role_phrase(role, sizeof role);
    snprintf(out, outcap, "%s — %s · %s", pc_display_name(), role, pr);
  }
}

void pc_format_hud_tag(char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  snprintf(out, outcap, "Playing as: %s — %s %s", pc_display_name(),
           g_pc.race[0] ? g_pc.race : "Human",
           g_pc.class_[0] ? g_pc.class_ : "adventurer");
}

void pc_format_role_phrase(char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  snprintf(out, outcap, "a %s %s", g_pc.race[0] ? g_pc.race : "Human",
           g_pc.class_[0] ? g_pc.class_ : "adventurer");
}

void pc_expand_world_placeholders(const char *in, char *out, size_t outcap) {
  const char *s;
  size_t o = 0;
  char pro[32];
  char role[96];
  const char *name = pc_display_name();
  const char *race = g_pc.race[0] ? g_pc.race : "Human";
  const char *klass = g_pc.class_[0] ? g_pc.class_ : "adventurer";
  if (!out || outcap < 2) return;
  if (!in) {
    out[0] = '\0';
    return;
  }
  pc_format_pronouns_short(g_pc.gender[0] ? g_pc.gender : "they", pro, sizeof pro);
  pc_format_role_phrase(role, sizeof role);
  s = in;
  while (*s && o + 1 < outcap) {
    if (*s == '%' && strncmp(s, "%NAME%", 6) == 0) {
      size_t n = strlen(name), i;
      for (i = 0; i < n && o + 1 < outcap; i++) out[o++] = name[i];
      s += 6;
      continue;
    }
    if (*s == '%' && strncmp(s, "%RACE%", 6) == 0) {
      size_t n = strlen(race), i;
      for (i = 0; i < n && o + 1 < outcap; i++) out[o++] = race[i];
      s += 6;
      continue;
    }
    if (*s == '%' && strncmp(s, "%CLASS%", 7) == 0) {
      size_t n = strlen(klass), i;
      for (i = 0; i < n && o + 1 < outcap; i++) out[o++] = klass[i];
      s += 7;
      continue;
    }
    if (*s == '%' && strncmp(s, "%PRONOUNS%", 10) == 0) {
      size_t n = strlen(pro), i;
      for (i = 0; i < n && o + 1 < outcap; i++) out[o++] = pro[i];
      s += 10;
      continue;
    }
    if (*s == '%' && strncmp(s, "%ROLE%", 6) == 0) {
      size_t n = strlen(role), i;
      for (i = 0; i < n && o + 1 < outcap; i++) out[o++] = role[i];
      s += 6;
      continue;
    }
    out[o++] = *s++;
  }
  out[o] = '\0';
}

static int write_kv_s(FILE *fp, const char *k, const char *v) {
  return fprintf(fp, "%s %s\n", k, v ? v : "") > 0;
}
static int write_kv_i(FILE *fp, const char *k, int v) { return fprintf(fp, "%s %d\n", k, v) > 0; }

int pc_write_save(FILE *fp) {
  if (!fp) return 0;
  /* Golden binary: fwrite("CHARACTER\n", 1, 10, fp) — not "CHARACTER \n".
   * Loader uses strcmp(buf, "CHARACTER") before pc_read_save; a stray space breaks load. */
  if (fwrite("CHARACTER\n", 1, 10, fp) != 10)
    return 0;
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

/* Keys matched case-sensitively like shipped binary (strcmp). */
static void parse_kv(const char *key, const char *val) {
  if (!key || !val) return;
  if (strcmp(key, "name") == 0) copy_s(g_pc.name, sizeof g_pc.name, val);
  else if (strcmp(key, "race") == 0) copy_s(g_pc.race, sizeof g_pc.race, val);
  else if (strcmp(key, "skin") == 0) copy_s(g_pc.skin, sizeof g_pc.skin, val);
  else if (strcmp(key, "gender") == 0) copy_s(g_pc.gender, sizeof g_pc.gender, val);
  else if (strcmp(key, "genitalia") == 0) copy_s(g_pc.genitalia, sizeof g_pc.genitalia, val);
  else if (strcmp(key, "class") == 0) copy_s(g_pc.class_, sizeof g_pc.class_, val);
  else if (strcmp(key, "height") == 0) copy_s(g_pc.height, sizeof g_pc.height, val);
  else if (strcmp(key, "build") == 0) copy_s(g_pc.build, sizeof g_pc.build, val);
  else if (strcmp(key, "hair_color") == 0) copy_s(g_pc.hair_color, sizeof g_pc.hair_color, val);
  else if (strcmp(key, "hair_style") == 0) copy_s(g_pc.hair_style, sizeof g_pc.hair_style, val);
  else if (strcmp(key, "eye_color") == 0) copy_s(g_pc.eye_color, sizeof g_pc.eye_color, val);
  else if (strcmp(key, "armor") == 0) copy_s(g_pc.armor, sizeof g_pc.armor, val);
  else if (strcmp(key, "weapon") == 0) copy_s(g_pc.weapon, sizeof g_pc.weapon, val);
  else if (strcmp(key, "str") == 0) g_pc.str = atoi(val);
  else if (strcmp(key, "tou") == 0) g_pc.tou = atoi(val);
  else if (strcmp(key, "spe") == 0) g_pc.spe = atoi(val);
  else if (strcmp(key, "intl") == 0) g_pc.intl = atoi(val);
  else if (strcmp(key, "wis") == 0) g_pc.wis = atoi(val);
  else if (strcmp(key, "will") == 0) g_pc.will = atoi(val);
  else if (strcmp(key, "agi") == 0) g_pc.agi = atoi(val);
  else if (strcmp(key, "cha") == 0) g_pc.cha = atoi(val);
  else if (strcmp(key, "per") == 0) g_pc.per = atoi(val);
  else if (strcmp(key, "cor") == 0) g_pc.cor = atoi(val);
  else if (strcmp(key, "muscle_tone") == 0)
    copy_s(g_pc.muscle_tone, sizeof g_pc.muscle_tone, val);
  else if (strcmp(key, "body_fat") == 0) copy_s(g_pc.body_fat, sizeof g_pc.body_fat, val);
  else if (strcmp(key, "hips") == 0) copy_s(g_pc.hips, sizeof g_pc.hips, val);
  else if (strcmp(key, "butt") == 0) copy_s(g_pc.butt, sizeof g_pc.butt, val);
  else if (strcmp(key, "age") == 0) copy_s(g_pc.age, sizeof g_pc.age, val);
  else if (strcmp(key, "voice_pitch") == 0)
    copy_s(g_pc.voice_pitch, sizeof g_pc.voice_pitch, val);
  else if (strcmp(key, "voice_quality") == 0)
    copy_s(g_pc.voice_quality, sizeof g_pc.voice_quality, val);
  else if (strcmp(key, "scars") == 0) g_pc.scars = atoi(val);
  else if (strcmp(key, "tattoos") == 0) g_pc.tattoos = atoi(val);
  else if (strcmp(key, "breasts") == 0) copy_s(g_pc.breasts, sizeof g_pc.breasts, val);
  else if (strcmp(key, "cock") == 0) copy_s(g_pc.cock, sizeof g_pc.cock, val);
  else if (strcmp(key, "balls") == 0) copy_s(g_pc.balls, sizeof g_pc.balls, val);
  else if (strcmp(key, "pussy") == 0) copy_s(g_pc.pussy, sizeof g_pc.pussy, val);
  else if (strcmp(key, "nipple_type") == 0)
    copy_s(g_pc.nipple_type, sizeof g_pc.nipple_type, val);
  else if (strcmp(key, "milk") == 0) g_pc.milk = atoi(val);
  else if (strcmp(key, "ball_fullness") == 0) g_pc.ball_fullness = atoi(val);
}

int pc_read_save(FILE *fp, char *scratch, size_t scratch_cap) {
  char *sp;
  const char *val;

  if (!fp || !scratch || scratch_cap < 8) return 0;
  memset(&g_pc, 0, sizeof g_pc);

  while (fgets(scratch, (int)scratch_cap, fp)) {
    scratch[strcspn(scratch, "\r\n")] = '\0';
    if (!scratch[0]) continue;
    if (strcmp(scratch, "ENDCHAR") == 0) {
      pc_fill_narrative_defaults(&g_pc);
      return 1;
    }
    sp = strchr(scratch, ' ');
    if (!sp) return 0;
    *sp = '\0';
    val = sp + 1;
    while (*val == ' ') val++;
    parse_kv(scratch, val);
  }
  return 0;
}

void pc_format_summary(char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  snprintf(out, outcap,
           "%s | %s | %s\nSTR %d TOU %d SPE %d AGI %d INT %d WIS %d WILL %d CHA %d PER %d COR %d",
           pc_display_name(), g_pc.race[0] ? g_pc.race : "Human",
           g_pc.class_[0] ? g_pc.class_ : "adventurer", g_pc.str, g_pc.tou, g_pc.spe,
           g_pc.agi, g_pc.intl, g_pc.wis, g_pc.will, g_pc.cha, g_pc.per, g_pc.cor);
}

/* --- Interactive character creation (matches shipped exe prompts; see
 * recovery_artifacts/raw_original_char_defaults.txt). --- */

static void cc_cls(void) {
#ifdef _WIN32
  (void)system("cls");
#else
  (void)system("clear");
#endif
}

static const char *const CC_RACES[53] = {
    "Human",       "Elf",          "Dark Elf",    "Wood Elf",    "High Elf",
    "Drow",        "Dwarf",        "Orc",         "Half-Orc",    "Minotaur",
    "Demon",       "Succubus",     "Incubus",     "Tiefling",    "Neko",
    "Kitsune",     "Wolfmorph",    "Rabbitmorph", "Rathshar",    "Pig Orc",
    "Angel",       "Seraph",       "Aasimar",     "Shark-morph", "Anemone",
    "Harpy",       "Sylph",        "Dryad",       "Nymph",       "Faerie",
    "Lizardfolk",  "Naga",        "Gorgon",      "Goblin",      "Kobold",
    "Gnome",       "Halfling",    "Holluschick", "Spider-morph", "Drider",
    "Goomurali",   "Lucerni",     "Horse-morph", "Cow-morph",   "Dragonmorph",
    "Draugandr",   "Slime-morph", "Lamia",       "Centaur",     "Satyr",
    "Android",     "Cyborg",      "Golem"};

static void cc_print_race_grid(void) {
  int i;
  for (i = 0; i < 53; i++) {
    printf("%2d. %-12s", i + 1, CC_RACES[i]);
    if (i == 52 || (i % 4) == 3)
      putchar('\n');
    else
      putchar(' ');
  }
}

static int cc_pick_index(int max_choice) {
  char buf[320];
  char *endp;
  long v;

  for (;;) {
    printf("\nEnter the number: ");
    fflush(stdout);
    if (!fgets(buf, sizeof buf, stdin)) return 1;
    buf[strcspn(buf, "\r\n")] = '\0';
    if (!buf[0]) continue;
    v = strtol(buf, &endp, 10);
    if (endp != buf && *endp == '\0' && v >= 1 && v <= max_choice)
      return (int)v;
    printf("Enter a number from 1 to %d: ", max_choice);
    fflush(stdout);
    if (!fgets(buf, sizeof buf, stdin)) return 1;
    buf[strcspn(buf, "\r\n")] = '\0';
    if (!buf[0]) continue;
    v = strtol(buf, &endp, 10);
    if (endp != buf && *endp == '\0' && v >= 1 && v <= max_choice)
      return (int)v;
  }
}

static void cc_read_name(void) {
  char buf[256];

  for (;;) {
    cc_cls();
    puts("=== CHARACTER CREATION ===\n");
    printf("%s", "Enter your character's name:\n");
    fflush(stdout);
    if (!fgets(buf, sizeof buf, stdin)) {
      buf[0] = '\0';
      break;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
    if (buf[0]) {
      size_t i = 0;
      while (buf[i] && isspace((unsigned char)buf[i])) i++;
      if (buf[i]) {
        copy_s(g_pc.name, sizeof g_pc.name, buf + i);
        return;
      }
    }
    puts("Name cannot be empty.");
    fflush(stdout);
  }
  copy_s(g_pc.name, sizeof g_pc.name, "Traveler");
}

static const char *cc_class_slug(int n) {
  static const char *const cl[15] = {
      "warrior",   "rogue",    "ranger",   "paladin", "barbarian",
      "priest",    "wizard",   "bard",     "druid",   "warlock",
      "sorcerer",  "assassin", "knight",   "mercenary", "mage"};
  if (n < 1 || n > 15) return "adventurer";
  return cl[n - 1];
}

static const char *cc_class_role_word(int n) {
  static const char *const rw[15] = {
      "Champion", "Shadow",   "Warden", "Paladin", "Berserker",
      "Oracle",   "Archmage", "Virtuoso", "Oracle", "Warlock",
      "Prodigy",  "Assassin", "Knight", "Veteran", "Sage"};
  if (n < 1 || n > 15) return "Hero";
  return rw[n - 1];
}

static void cc_apply_stat_defaults(void) {
  g_pc.str = g_pc.tou = g_pc.spe = g_pc.intl = g_pc.wis = g_pc.will = g_pc.agi =
      g_pc.cha = g_pc.per = 10;
  g_pc.cor = 0;
}

static const char *cc_subject_pronoun(void) {
  if (ieq(g_pc.gender, "male")) return "He";
  if (ieq(g_pc.gender, "female")) return "She";
  return "They";
}

/** Penis / testicles / fullness prompts only when anatomy includes a penis. */
static int cc_should_prompt_penis_and_balls(void) {
  return ieq(g_pc.genitalia, "Penis") || ieq(g_pc.genitalia, "Both");
}

/**
 * Breast sizing is omitted for typical cis-male (male + penis-only): later 18+
 * questions should not appear when that combination cannot apply.
 */
static int cc_should_prompt_breasts(void) {
  if (ieq(g_pc.genitalia, "Penis") && ieq(g_pc.gender, "male")) return 0;
  return 1;
}

static void cc_print_created_summary(void) {
  const char *subj = cc_subject_pronoun();
  const char *vposs =
      ieq(g_pc.gender, "male") ? "His"
                               : (ieq(g_pc.gender, "female") ? "Her" : "Their");
  const char *verb = ieq(g_pc.gender, "they") ? "are" : "is";
  char headline[128];
  char identity[256];
  char hud[160];
  int ci = 1;
  /* Recover class index from slug for role title (best-effort). */
  {
    int i;
    for (i = 0; i < 15; i++) {
      if (ieq(g_pc.class_, cc_class_slug(i + 1))) {
        ci = i + 1;
        break;
      }
    }
  }
  snprintf(headline, sizeof headline, "%s, a %s and Adventurer.\n",
           pc_display_name(), cc_class_role_word(ci));

  cc_cls();
  puts("=== CHARACTER CREATED ===\n");
  printf("%s", headline);
  printf("\n%s %s a %s %s %s, standing fit for the tale ahead.\n",
         subj, verb, g_pc.race[0] ? g_pc.race : "Human",
         g_pc.gender[0] ? g_pc.gender : "they",
         g_pc.class_[0] ? g_pc.class_ : "adventurer");
  printf("\n%s voice reads as %s / %s. Marks: scars intensity %d, tattoos "
         "%d.\n",
         vposs,
         g_pc.voice_pitch[0] ? g_pc.voice_pitch : "balanced",
         g_pc.voice_quality[0] ? g_pc.voice_quality : "smooth", g_pc.scars,
         g_pc.tattoos);

  {
    char pron[64];
    pc_format_hud_tag(hud, sizeof hud);
    pc_format_identity_banner(identity, sizeof identity);
    pc_format_pronouns_short(g_pc.gender[0] ? g_pc.gender : "they", pron,
                             sizeof pron);
    printf("\nPlaying as: %s\n\n%s\n\n", hud, identity);
    printf("Portrait and examine commands use these pronouns in prose: %s\n",
           pron);
  }
  printf("%s\n\n",
         "Mod NPC lines can substitute tokens: %NAME% %RACE% %CLASS% %ROLE% "
         "%PRONOUNS%.");
  printf("%s\n\n%s\n",
         "Many world commands (scan, route, loot triage, compare, notes…) "
         "print this same identity line at the top.",
         "Saves store your CHARACTER block — load quicksave or a slot to resume "
         "this sheet.");
  printf("%s", "Press Enter to begin.\n");
  fflush(stdout);
  {
    char discard[256];
    if (!fgets(discard, sizeof discard, stdin)) {
    }
  }
}

static void cc_run_interactive(void) {
  int r;
  int skin_n = 6;
  static const char *const skins[] = {"Pale", "Fair", "Olive", "Bronzed",
                                      "Dark", "Ebony"};
  static const char *const genders[] = {"male", "female", "they"};
  static const char *const genitals[] = {"Penis", "Vagina", "Both", "None"};
  static const char *const ages[] = {"Adult",      "Youthful", "Middle-Aged",
                                     "Weathered", "Ageless"};
  static const char *const vpitches[] = {
      "Default (from class / build)", "Tenor", "Alto",
      "Soprano", "Baritone", "Bass"};
  static const char *const vquals[] = {"Default (smooth baseline)", "Raspy",
                                       "Melodious", "Breathy", "Gruff"};

  cc_read_name();

  cc_cls();
  printf("=== CHARACTER CREATION ===\n\nCreating: %s\n\nSelect your race "
         "(1-53):\n\n",
         pc_display_name());
  cc_print_race_grid();
  r = cc_pick_index(53);
  copy_s(g_pc.race, sizeof g_pc.race, CC_RACES[r - 1]);

  cc_cls();
  printf("=== CHARACTER CREATION ===\n\nSelect skin / coloration for %s:\n\n",
         g_pc.race);
  {
    int i;
    for (i = 0; i < skin_n; i++)
      printf("  %d. %s\n", i + 1, skins[i]);
  }
  r = cc_pick_index(skin_n);
  copy_s(g_pc.skin, sizeof g_pc.skin, skins[r - 1]);

  cc_cls();
  printf("=== CHARACTER CREATION ===\n\nCreating: %s\n\nSelect gender:\n",
         pc_display_name());
  puts(" 1. Male\n 2. Female\n 3. Other\n");
  r = cc_pick_index(3);
  copy_s(g_pc.gender, sizeof g_pc.gender, genders[r - 1]);

  cc_cls();
  printf("=== CHARACTER CREATION ===\n\nCreating: %s\n\nSelect genitalia:\n",
         pc_display_name());
  puts(" 1. Penis\n 2. Vagina\n 3. Both\n 4. None\n");
  r = cc_pick_index(4);
  copy_s(g_pc.genitalia, sizeof g_pc.genitalia, genitals[r - 1]);

  cc_cls();
  printf("=== CHARACTER CREATION ===\n\nCreating: %s\n\nSelect class:\n\n",
         pc_display_name());
  puts(" 1. Warrior   2. Rogue\n"
       " 3. Ranger   4. Paladin\n"
       " 5. Barbarian   6. Priest\n"
       " 7. Wizard   8. Bard\n"
       " 9. Druid  10. Warlock\n"
       "11. Sorcerer  12. Assassin\n"
       "13. Knight  14. Mercenary\n"
       "15. Mage\n");
  r = cc_pick_index(15);
  copy_s(g_pc.class_, sizeof g_pc.class_, cc_class_slug(r));

  cc_cls();
  puts("=== CHARACTER CREATION ===\n\nSelect height:\n"
       " 1. Tiny (3' - 4')\n"
       " 2. Short (4' - 5')\n"
       " 3. Average (5' - 5'8\")\n"
       " 4. Tall (5'8\" - 6')\n"
       " 5. Very Tall (6' - 7')\n"
       " 6. Gigantic (7'+)\n");
  {
    static const char *const H[] = {
        "Tiny (3' - 4')",    "Short (4' - 5')", "Average (5' - 5'8\")",
        "Tall (5'8\" - 6')", "Very Tall (6' - 7')", "Gigantic (7'+)"};
    r = cc_pick_index(6);
    copy_s(g_pc.height, sizeof g_pc.height, H[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION ===\n\nSelect build:\n"
       " 1. Slender   2. Lean\n"
       " 3. Average   4. Stocky\n"
       " 5. Muscular   6. Thick\n");
  {
    static const char *const B[] = {"Slender", "Lean", "Average",
                                     "Stocky", "Muscular", "Thick"};
    r = cc_pick_index(6);
    copy_s(g_pc.build, sizeof g_pc.build, B[r - 1]);
    copy_s(g_pc.muscle_tone, sizeof g_pc.muscle_tone, B[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION ===\n\nSelect hair color:\n"
       " 1. Black     2. Brown     3. Blonde\n"
       " 4. Light Brown     5. Platinum Blonde     6. Strawberry Blonde\n"
       " 7. Red     8. Auburn     9. Silver\n"
       "10. White    11. Pink    12. Violet\n"
       "13. Gray    14. Green    15. Blue\n"
       "16. Purple    17. Orange    18. Crimson\n");
  {
    static const char *const HC[] = {
        "Black", "Brown", "Blonde", "Light Brown", "Platinum Blonde",
        "Strawberry Blonde", "Red", "Auburn", "Silver", "White", "Pink",
        "Violet", "Gray", "Green", "Blue", "Purple", "Orange", "Crimson"};
    r = cc_pick_index(18);
    copy_s(g_pc.hair_color, sizeof g_pc.hair_color, HC[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION ===\n\nSelect hair style:\n"
       " 1. Short\n"
       " 2. Shoulder-length\n"
       " 3. Long\n"
       " 4. Very Long\n"
       " 5. Braided\n"
       " 6. Ponytail\n"
       " 7. Mohawk\n"
       " 8. Bald\n");
  {
    static const char *const HS[] = {
        "Short", "Shoulder-length", "Long", "Very Long",
        "Braided", "Ponytail", "Mohawk", "Bald"};
    r = cc_pick_index(8);
    copy_s(g_pc.hair_style, sizeof g_pc.hair_style, HS[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION ===\n\nSelect eye color:\n"
       " 1. Brown   2. Blue\n"
       " 3. Green   4. Hazel\n"
       " 5. Gray   6. Amber\n"
       " 7. Violet   8. Red\n");
  {
    static const char *const EC[] = {"Brown", "Blue", "Green", "Hazel",
                                      "Gray", "Amber", "Violet", "Red"};
    r = cc_pick_index(8);
    copy_s(g_pc.eye_color, sizeof g_pc.eye_color, EC[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION ===\n\nSelect armor:\n"
       " 1. None   2. Leather\n"
       " 3. Chainmail   4. Plate\n"
       " 5. Scale   6. Robe\n");
  {
    static const char *const AR[] = {"None", "Leather", "Chainmail",
                                      "Plate", "Scale", "Robe"};
    r = cc_pick_index(6);
    copy_s(g_pc.armor, sizeof g_pc.armor, AR[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION ===\n\nSelect weapon:\n"
       " 1. None   2. Sword\n"
       " 3. Axe   4. Mace\n"
       " 5. Spear   6. Staff\n"
       " 7. Dagger   8. Bow\n"
       " 9. Club\n");
  {
    static const char *const WP[] = {"None", "Sword", "Axe", "Mace", "Spear",
                                     "Staff", "Dagger", "Bow", "Club"};
    r = cc_pick_index(9);
    copy_s(g_pc.weapon, sizeof g_pc.weapon, WP[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION — AGE & SILHOUETTE ===\n\n"
       "Age category (portrait phrasing):\n"
       " 1 Default (adult)\n"
       " 2 Youthful\n"
       " 3 Middle-Aged\n"
       " 4 Weathered\n"
       " 5 Ageless\n");
  r = cc_pick_index(5);
  copy_s(g_pc.age, sizeof g_pc.age, ages[r - 1]);

  cc_cls();
  puts("=== CHARACTER CREATION — VOICE ===\n\nVoice pitch:\n"
       " 1 Default (from class / build)\n"
       " 2 Tenor\n"
       " 3 Alto\n"
       " 4 Soprano\n"
       " 5 Baritone\n"
       " 6 Bass\n");
  r = cc_pick_index(6);
  copy_s(g_pc.voice_pitch, sizeof g_pc.voice_pitch, vpitches[r - 1]);

  cc_cls();
  puts("=== CHARACTER CREATION — VOICE (TIMBRE) ===\n\nVoice quality:\n"
       " 1 Default (smooth baseline)\n"
       " 2 Raspy\n"
       " 3 Melodious\n"
       " 4 Breathy\n"
       " 5 Gruff\n");
  r = cc_pick_index(5);
  copy_s(g_pc.voice_quality, sizeof g_pc.voice_quality, vquals[r - 1]);

  cc_cls();
  puts("=== CHARACTER CREATION — MARKS ===\n\n"
       "Visible scars (narrative intensity):\n"
       " 1 None\n"
       " 2 A few\n"
       " 3 Several\n"
       " 4 Many\n");
  r = cc_pick_index(4);
  g_pc.scars = r - 1;

  cc_cls();
  puts("=== CHARACTER CREATION — TATTOOS ===\n\nTattoos:\n"
       " 1 None\n"
       " 2 Some\n"
       " 3 Extensive\n");
  r = cc_pick_index(3);
  g_pc.tattoos = r - 1;

  cc_cls();
  puts("=== CHARACTER CREATION — HIPS & SEAT ===\n\nHips:\n"
       " 1 Average (default)\n"
       " 2 Narrow\n"
       " 3 Very wide\n");
  {
    static const char *const HP[] = {"Average", "Narrow", "Very wide"};
    r = cc_pick_index(3);
    copy_s(g_pc.hips, sizeof g_pc.hips, HP[r - 1]);
  }

  cc_cls();
  puts("=== CHARACTER CREATION — SEAT ===\n\nRear / seat:\n"
       " 1 Average (default)\n"
       " 2 Flat\n"
       " 3 Large\n");
  {
    static const char *const BT[] = {"Average", "Flat", "Large"};
    r = cc_pick_index(3);
    copy_s(g_pc.butt, sizeof g_pc.butt, BT[r - 1]);
  }

  if (cc_should_prompt_breasts()) {
    cc_cls();
    puts("=== CHARACTER CREATION — PHYSICAL DETAIL (18+) ===\n\n"
         "Fine-tunes the full-text portrait (character / sheet).\n"
         "Choose 1 on any screen to keep the automatic default from your build\n"
         "and genitalia.\n\n"
         "Breast size / chest:\n"
         " 1 Keep automatic\n"
         " 2 None (flat chest)\n"
         " 3 Tiny\n"
         " 4 Small\n"
         " 5 Average\n"
         " 6 Large\n"
         " 7 Huge\n"
         " 8 Massive\n");
    {
      static const char *const BR[] = {
          "Automatic", "None", "Tiny", "Small", "Average",
          "Large", "Huge", "Massive"};
      r = cc_pick_index(8);
      copy_s(g_pc.breasts, sizeof g_pc.breasts, BR[r - 1]);
    }
  } else {
    copy_s(g_pc.breasts, sizeof g_pc.breasts, "None");
  }

  if (cc_should_prompt_penis_and_balls()) {
    cc_cls();
    puts("=== PHYSICAL DETAIL (18+) ===\n\nPenis size:\n"
         " 1 Keep automatic\n"
         " 2 Tiny\n"
         " 3 Small\n"
         " 4 Average\n"
         " 5 Large\n"
         " 6 Huge\n"
         " 7 Massive\n");
    {
      static const char *const CK[] = {
          "Automatic", "Tiny", "Small", "Average", "Large", "Huge", "Massive"};
      r = cc_pick_index(7);
      copy_s(g_pc.cock, sizeof g_pc.cock, CK[r - 1]);
    }

    cc_cls();
    puts("=== PHYSICAL DETAIL (18+) ===\n\nTesticles:\n"
         " 1 Keep automatic\n"
         " 2 Tiny\n"
         " 3 Small\n"
         " 4 Average\n"
         " 5 Large\n"
         " 6 Huge\n"
         " 7 Massive\n");
    {
      static const char *const BL[] = {
          "Automatic", "Tiny", "Small", "Average", "Large", "Huge", "Massive"};
      r = cc_pick_index(7);
      copy_s(g_pc.balls, sizeof g_pc.balls, BL[r - 1]);
    }

    cc_cls();
    puts("=== PHYSICAL DETAIL (18+) ===\n\nBall fullness (portrait):\n"
         " 1 Normal\n"
         " 2 Slight\n"
         " 3 Heavy\n");
    r = cc_pick_index(3);
    g_pc.ball_fullness = r - 1;
  } else {
    copy_s(g_pc.cock, sizeof g_pc.cock, "None");
    copy_s(g_pc.balls, sizeof g_pc.balls, "None");
    g_pc.ball_fullness = 0;
  }

  cc_apply_stat_defaults();
  pc_fill_narrative_defaults(&g_pc);
  cc_print_created_summary();
}

void run_character_creation(int autotest_mode) {
  if (autotest_mode) {
    /* CI / golden playthrough: match original EXE output (see dual_exe_compare). */
    pc_set_default_adventurer();
    copy_s(g_pc.name, sizeof g_pc.name, "Autotest Hero");
    copy_s(g_pc.race, sizeof g_pc.race, "Human");
    copy_s(g_pc.gender, sizeof g_pc.gender, "they");
    copy_s(g_pc.class_, sizeof g_pc.class_, "adventurer");
    pc_fill_narrative_defaults(&g_pc);
    return;
  }
  pc_reset_empty();
  cc_run_interactive();
}
