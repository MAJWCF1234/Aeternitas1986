#ifndef AETERNITAS_CHAR_CREATION_H
#define AETERNITAS_CHAR_CREATION_H

#include <stddef.h>
#include <stdio.h>

typedef struct AetPcSave {
  char name[64];
  char race[32];
  char skin[32];
  char gender[24];
  char genitalia[24];
  char class_[32];
  char height[48];
  char build[32];
  char hair_color[32];
  char hair_style[32];
  char eye_color[32];
  char armor[32];
  char weapon[32];
  int str, tou, spe, intl, wis, will, agi, cha, per, cor;
  char muscle_tone[32];
  char body_fat[32];
  char hips[32];
  char butt[32];
  char age[24];
  char voice_pitch[24];
  char voice_quality[24];
  int scars;
  int tattoos;
  char breasts[24];
  char cock[24];
  char balls[24];
  char pussy[24];
  char nipple_type[24];
  int milk;
  int ball_fullness;
} AetPcSave;

void pc_capture(AetPcSave *out);
void pc_restore(const AetPcSave *in);
void pc_reset_empty(void);
void pc_fill_narrative_defaults(AetPcSave *io);
const char *pc_display_name(void);
void pc_format_pronouns_short(const char *gender, char *out, size_t outcap);
void pc_format_identity_banner(char *out, size_t outcap);
void pc_format_hud_tag(char *out, size_t outcap);
void pc_format_role_phrase(char *out, size_t outcap);
void pc_expand_world_placeholders(const char *in, char *out, size_t outcap);
void pc_set_default_adventurer(void);
int pc_write_save(FILE *fp);
int pc_read_save(FILE *fp, char *scratch, size_t scratch_cap);
void pc_format_summary(char *out, size_t outcap);
void run_character_creation(int autotest_mode);

#endif
