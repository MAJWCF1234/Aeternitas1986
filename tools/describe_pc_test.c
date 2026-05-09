/*
 * Standalone harness for aet_describe_pc(): synthetic AetPcSave profiles,
 * prints each description for manual / diff review.
 *
 * Build (from repo root):
 *   gcc -std=c11 -Wall -Wextra -finput-charset=UTF-8 -fexec-charset=UTF-8 \
 *       -o describe_pc_test.exe tools/describe_pc_test.c aeternitas_char_description.c
 */

#include "../aeternitas_char_description.h"

#include <stdio.h>
#include <string.h>

static void copy_field(char *dst, size_t cap, const char *src) {
  if (!dst || cap < 1) return;
  if (!src) src = "";
  strncpy(dst, src, cap - 1);
  dst[cap - 1] = '\0';
}

static void baseline(AetPcSave *pc) {
  memset(pc, 0, sizeof(*pc));
  copy_field(pc->name, sizeof pc->name, "Mj");
  copy_field(pc->race, sizeof pc->race, "Human");
  copy_field(pc->skin, sizeof pc->skin, "Pale");
  copy_field(pc->gender, sizeof pc->gender, "male");
  copy_field(pc->genitalia, sizeof pc->genitalia, "Penis");
  copy_field(pc->class_, sizeof pc->class_, "warrior");
  copy_field(pc->height, sizeof pc->height, "Tiny (3' - 4')");
  copy_field(pc->build, sizeof pc->build, "Slender");
  copy_field(pc->hair_color, sizeof pc->hair_color, "Pink");
  copy_field(pc->hair_style, sizeof pc->hair_style, "Short");
  copy_field(pc->eye_color, sizeof pc->eye_color, "Brown");
  copy_field(pc->armor, sizeof pc->armor, "None");
  copy_field(pc->weapon, sizeof pc->weapon, "None");
  pc->str = pc->tou = pc->spe = pc->intl = pc->wis = pc->will = pc->agi = pc->cha =
      pc->per = 10;
  pc->cor = 0;
  copy_field(pc->muscle_tone, sizeof pc->muscle_tone, "Slender");
  copy_field(pc->body_fat, sizeof pc->body_fat, "Slender");
  copy_field(pc->hips, sizeof pc->hips, "Average");
  copy_field(pc->butt, sizeof pc->butt, "Average");
  copy_field(pc->age, sizeof pc->age, "Adult");
  copy_field(pc->voice_pitch, sizeof pc->voice_pitch, "Tenor");
  copy_field(pc->voice_quality, sizeof pc->voice_quality, "Smooth");
  pc->scars = 0;
  pc->tattoos = 0;
  copy_field(pc->breasts, sizeof pc->breasts, "automatic");
  copy_field(pc->cock, sizeof pc->cock, "automatic");
  copy_field(pc->balls, sizeof pc->balls, "automatic");
  copy_field(pc->pussy, sizeof pc->pussy, "None");
  copy_field(pc->nipple_type, sizeof pc->nipple_type, "Normal");
  pc->milk = 0;
  pc->ball_fullness = 0;
}

static void print_case(const char *title, const AetPcSave *pc) {
  static char buf[AETER_CHARACTER_PORTRAIT_CAP];
  printf(
      "\n"
      "###############################################################################\n");
  printf("# %s\n", title);
  printf(
      "###############################################################################\n");
  aet_describe_pc(pc, buf, sizeof buf);
  fputs(buf, stdout);
  fputc('\n', stdout);
}

int main(void) {
  AetPcSave pc;
  setvbuf(stdout, NULL, _IONBF, 0);


  baseline(&pc);
  print_case("Baseline (tiny male warrior, automatic anatomy)", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.class_, sizeof pc.class_, "white mage");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.breasts, sizeof pc.breasts, "automatic");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  print_case("Female vagina, mage, average height", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  copy_field(pc.race, sizeof pc.race, "Android");
  print_case("Android female vagina-only default stride (thermal seams)", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  copy_field(pc.race, sizeof pc.race, "Slime");
  print_case("Slime female vagina-only default stride (meniscus film)", &pc);

  baseline(&pc);
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  copy_field(pc.pussy, sizeof pc.pussy, "soft folds");
  print_case("Herm / both sets, large breasts, explicit pussy string", &pc);

  baseline(&pc);
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  copy_field(pc.pussy, sizeof pc.pussy, "soft folds");
  copy_field(pc.race, sizeof pc.race, "Android");
  print_case("Herm Android + explicit pussy (subroutine pairing)", &pc);

  baseline(&pc);
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  copy_field(pc.pussy, sizeof pc.pussy, "soft folds");
  copy_field(pc.race, sizeof pc.race, "Slime");
  print_case("Herm Slime + explicit pussy (tide pairing)", &pc);

  baseline(&pc);
  copy_field(pc.genitalia, sizeof pc.genitalia, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "None");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  print_case("Neuter genitalia", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.genitalia, sizeof pc.genitalia, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "None");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  print_case("Android neuter (interface groin)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.genitalia, sizeof pc.genitalia, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "None");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  print_case("Slime neuter (viscosity groin)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.ball_fullness = 20;
  print_case("Android no balls + fullness tier 1 (stack/charge)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.ball_fullness = 80;
  print_case("Android no balls + fullness tier 3 (reservoir blame)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.ball_fullness = 55;
  print_case("Android no balls + fullness tier 2 (torque tally)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.ball_fullness = 55;
  print_case("Slime no balls + fullness tier 2 (tide tally)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Harpy");
  copy_field(pc.height, sizeof pc.height, "Short");
  print_case("Harpy (winged body + talons in race clause)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Elf");
  pc.str = 14;
  pc.cha = 13;
  pc.cor = 4;
  print_case("Elf, high STR/CHA, corruption > 0", &pc);

  baseline(&pc);
  pc.scars = 2;
  pc.tattoos = 3;
  pc.milk = 40;
  print_case("Scarred, inked, lactation", &pc);

  baseline(&pc);
  copy_field(pc.cock, sizeof pc.cock, "Large");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.ball_fullness = 75;
  print_case("Explicit cock size, ball fullness high", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "they");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Penis");
  copy_field(pc.height, sizeof pc.height, "Gigantic");
  copy_field(pc.build, sizeof pc.build, "Muscular");
  print_case("Androgynous giant muscular", &pc);

  /* --- Gear closer branches (append_gear_closer) --- */
  baseline(&pc);
  copy_field(pc.armor, sizeof pc.armor, "Iron breastplate");
  copy_field(pc.weapon, sizeof pc.weapon, "None");
  print_case("Gear: armor only (weapon None)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.armor, sizeof pc.armor, "Iron breastplate");
  copy_field(pc.weapon, sizeof pc.weapon, "None");
  print_case("Gear: Android armor only (routing straps)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.armor, sizeof pc.armor, "Iron breastplate");
  copy_field(pc.weapon, sizeof pc.weapon, "None");
  print_case("Gear: Slime armor only (borrowed shells)", &pc);

  baseline(&pc);
  copy_field(pc.armor, sizeof pc.armor, "None");
  copy_field(pc.weapon, sizeof pc.weapon, "Silver rapier");
  print_case("Gear: weapon only (armor None)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.armor, sizeof pc.armor, "None");
  copy_field(pc.weapon, sizeof pc.weapon, "Silver rapier");
  print_case("Gear: Android weapon only (chassis light)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.armor, sizeof pc.armor, "None");
  copy_field(pc.weapon, sizeof pc.weapon, "Silver rapier");
  print_case("Gear: Slime weapon only (mantle light)", &pc);

  baseline(&pc);
  copy_field(pc.armor, sizeof pc.armor, "Chain hauberk");
  copy_field(pc.weapon, sizeof pc.weapon, "Warhammer");
  print_case("Gear: armor + weapon both set", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.armor, sizeof pc.armor, "Chain hauberk");
  copy_field(pc.weapon, sizeof pc.weapon, "Warhammer");
  print_case("Gear: Android armor + weapon (alloy trappings)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.armor, sizeof pc.armor, "Chain hauberk");
  copy_field(pc.weapon, sizeof pc.weapon, "Warhammer");
  print_case("Gear: Slime armor + weapon (resin trappings)", &pc);

  baseline(&pc);
  copy_field(pc.armor, sizeof pc.armor, "none");
  copy_field(pc.weapon, sizeof pc.weapon, "none");
  print_case("Gear: lowercase none none (label normalization)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.armor, sizeof pc.armor, "None");
  copy_field(pc.weapon, sizeof pc.weapon, "None");
  print_case("Gear: Android unarmed unarmored (spec uptime)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.armor, sizeof pc.armor, "None");
  copy_field(pc.weapon, sizeof pc.weapon, "None");
  print_case("Gear: Slime unarmed unarmored (tide viscosity)", &pc);

  /* --- Stature / height parsing --- */
  baseline(&pc);
  copy_field(pc.height, sizeof pc.height, "");
  print_case("Edge: empty height string (fallback stature)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.height, sizeof pc.height, "");
  print_case("Android empty height (calibration promised)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.height, sizeof pc.height, "");
  print_case("Slime empty height (tide levels gossip)", &pc);

  baseline(&pc);
  copy_field(pc.height, sizeof pc.height, "Very Tall (6' - 7')");
  copy_field(pc.build, sizeof pc.build, "Lean");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Lean");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Lean");
  print_case("Very Tall CC-style height + lean build/muscle/fat match", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.height, sizeof pc.height, "Very Tall (6' - 7')");
  copy_field(pc.build, sizeof pc.build, "Lean");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Lean");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Lean");
  print_case("Android Very Tall CC parentheses (lintels suggestions)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.height, sizeof pc.height, "Very Tall (6' - 7')");
  copy_field(pc.build, sizeof pc.build, "Lean");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Lean");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Lean");
  print_case("Slime Very Tall CC parentheses (doorways patience)", &pc);

  baseline(&pc);
  copy_field(pc.height, sizeof pc.height, "Tall");
  print_case("Bare height token Tall (no parentheses)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Average");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Average");
  print_case("Android Average height token (nominal chart)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Average");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Average");
  print_case("Slime Average height token (tide quiet)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.height, sizeof pc.height, "6'2\"");
  print_case("Android raw height token (reach rated)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.height, sizeof pc.height, "6'2\"");
  print_case("Slime raw height token (span borrowed)", &pc);

  /* --- Face / marks --- */
  baseline(&pc);
  pc.scars = 1;
  print_case("Exactly one scar (singular face paragraph)", &pc);

  /* --- Female lactation tiers (non-male breast CC path) --- */
  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  pc.milk = 80;
  copy_field(pc.breasts, sizeof pc.breasts, "Huge");
  print_case("Female vagina rank-3 lactation + Huge breasts", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.milk = 12;
  copy_field(pc.breasts, sizeof pc.breasts, "automatic");
  print_case("Female vagina rank-1 milk (trace fullness prose)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  pc.milk = 15;
  copy_field(pc.breasts, sizeof pc.breasts, "automatic");
  print_case("Android female rank-1 lactation (coolant gather)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  pc.milk = 15;
  copy_field(pc.breasts, sizeof pc.breasts, "automatic");
  print_case("Slime female rank-1 lactation (tide memory)", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Slender");
  copy_field(pc.breasts, sizeof pc.breasts, "None");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  pc.milk = 70;
  print_case("Female flat breasts None + high milk (modest-curve lactation)", &pc);

  /* --- Male flat chest + milk (skip_breast_cc lactation branch) --- */
  baseline(&pc);
  copy_field(pc.breasts, sizeof pc.breasts, "None");
  pc.milk = 8;
  print_case("Male penis breasts None + trace milk (male lactation branch)", &pc);

  /* --- Penis config but cock label none (generic shaft paragraph) --- */
  baseline(&pc);
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  print_case("Penis genitalia but cock/balls None strings", &pc);

  /* --- Ball fullness tiers with sack present --- */
  baseline(&pc);
  pc.ball_fullness = 20;
  print_case("Ball fullness low tier (fr=1), automatic cock/balls", &pc);

  baseline(&pc);
  copy_field(pc.cock, sizeof pc.cock, "Huge");
  copy_field(pc.balls, sizeof pc.balls, "Large");
  pc.ball_fullness = 55;
  print_case("Explicit cock/balls + fullness mid tier (fr=2)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.cock, sizeof pc.cock, "Huge");
  copy_field(pc.balls, sizeof pc.balls, "Large");
  pc.ball_fullness = 20;
  print_case("Android explicit cock/balls + fullness low (reservoir pressure)",
             &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.cock, sizeof pc.cock, "Huge");
  copy_field(pc.balls, sizeof pc.balls, "Large");
  pc.ball_fullness = 20;
  print_case("Slime explicit cock/balls + fullness low (gel tide)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.cock, sizeof pc.cock, "Huge");
  copy_field(pc.balls, sizeof pc.balls, "Large");
  pc.ball_fullness = 55;
  print_case("Android explicit cock/balls + fullness mid (torque ache)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.cock, sizeof pc.cock, "Huge");
  copy_field(pc.balls, sizeof pc.balls, "Large");
  pc.ball_fullness = 55;
  print_case("Slime explicit cock/balls + fullness mid (meniscus ache)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.cock, sizeof pc.cock, "Massive");
  print_case("Android explicit cock label (routing crowd)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.cock, sizeof pc.cock, "Massive");
  print_case("Slime explicit cock label (shear crowd)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.cock, sizeof pc.cock, "Large");
  copy_field(pc.balls, sizeof pc.balls, "Huge");
  print_case("Android explicit balls label (torque datum)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.cock, sizeof pc.cock, "Large");
  copy_field(pc.balls, sizeof pc.balls, "Huge");
  print_case("Slime explicit balls label (swell reading)", &pc);

  /* --- Appearance tags: hips/butt/bald --- */
  baseline(&pc);
  copy_field(pc.hair_style, sizeof pc.hair_style, "Bald");
  copy_field(pc.hips, sizeof pc.hips, "Very wide");
  copy_field(pc.butt, sizeof pc.butt, "Large");
  print_case("Bald + very wide hips + large butt tags", &pc);

  /* --- Race / modification inference --- */
  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  print_case("Android (constructed / seams prose)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.cock, sizeof pc.cock, "None");
  print_case("Android cock CC None (hardware / hydraulic shaft)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.height, sizeof pc.height, "Short");
  print_case("Android + Short height (manufacture band, not bloodline)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.height, sizeof pc.height, "Gigantic");
  copy_field(pc.build, sizeof pc.build, "Stocky");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Stocky");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Stocky");
  print_case("Android Gigantic Stocky (manufacture intent stature)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.height, sizeof pc.height, "Gigantic");
  copy_field(pc.build, sizeof pc.build, "Stocky");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Stocky");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Stocky");
  print_case("Slime Gigantic Stocky (mass pooled stature)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.height, sizeof pc.height, "Tall");
  print_case("Android bare Tall token (engineered reach)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.height, sizeof pc.height, "Tall");
  print_case("Slime bare Tall token (tide-drawn)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.gender, sizeof pc.gender, "female");
  print_case("Android female (mask reads feminine)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.gender, sizeof pc.gender, "they");
  print_case("Android they (facial calibration hedges)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.hair_style, sizeof pc.hair_style, "Bald");
  print_case("Android bald (dome / laminate)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.build, sizeof pc.build, "Muscular");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Slender");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Slender");
  print_case("Android Muscular lean (torque carved into housing)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Average");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Average");
  print_case("Android equal tone labels (calibration vs tolerance dial)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.build, sizeof pc.build, "Muscular");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Thick");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Slender");
  print_case("Android Muscular mixed labels (torque under substrate)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  print_case("Slime (moist gleam prose)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Cave slime");
  print_case("Lowercase slime token (surface/stance race clause)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Goomurali");
  print_case("Goomurali (threshold coda + tide hips)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime Gorgon");
  print_case("Slime Gorgon (scale infer + veil profile)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.height, sizeof pc.height, "Short");
  print_case("Slime + Short height (strain vs bloodline)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.gender, sizeof pc.gender, "female");
  print_case("Slime female (surface hints feminine)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.gender, sizeof pc.gender, "they");
  print_case("Slime they (taxonomy unstable mask)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.hair_style, sizeof pc.hair_style, "Bald");
  print_case("Slime bald (crown meniscus)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.build, sizeof pc.build, "Muscular");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Slender");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Slender");
  print_case("Slime Muscular lean (strain carved into gel)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.cor = 6;
  print_case("Android + corruption (specification leak)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.cor = 6;
  print_case("Slime + corruption (viscosity tint)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.scars = 1;
  print_case("Android scars=1 (seam face + hairline ledger)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.scars = 4;
  print_case("Android heavy scars (relay face + ledger)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.scars = 2;
  print_case("Slime scars=2 (healed seams ledger)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.scars = 5;
  print_case("Slime heavy scars (turbulent face + ledger)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.tattoos = 1;
  print_case("Android tattoos=1 (routing stain)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.tattoos = 4;
  print_case("Android heavy tattoos (etched housing)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.tattoos = 1;
  print_case("Slime tattoos=1 (cling stain)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.tattoos = 6;
  print_case("Slime heavy tattoos (second tide)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Dwarf");
  print_case("Dwarf stock (compact durable prose)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Wolf morph");
  print_case("Wolf morph (beast-tell prose)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime wolf morph");
  print_case("Slime wolf morph (beast strain infer)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Cyborg wolf morph");
  print_case("Cyborg wolf morph (beast routing infer)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Cyborg tiefling morph");
  print_case("Cyborg tiefling morph (routing bend morph gloss)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Drider");
  copy_field(pc.height, sizeof pc.height, "Tall (5'8\" - 6')");
  print_case("Drider tall CC height (spider body + race hips)", &pc);

  /* --- Corruption + extreme stats --- */
  baseline(&pc);
  pc.cor = 12;
  pc.str = 18;
  pc.cha = 5;
  pc.per = 6;
  print_case("High corruption + lopsided stats (STR hero, low CHA)", &pc);

  baseline(&pc);
  pc.str = pc.cha = pc.agi = pc.per = 18;
  pc.tou = pc.intl = pc.wis = pc.will = 8;
  print_case("Multiple max stats (dual calling-cards line)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.str = pc.cha = pc.agi = pc.per = 18;
  pc.tou = pc.intl = pc.wis = pc.will = 8;
  print_case("Android multiple max stats (torque dual line)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.str = pc.cha = pc.agi = pc.per = 18;
  pc.tou = pc.intl = pc.wis = pc.will = 8;
  print_case("Slime multiple max stats (tide dual line)", &pc);

  /* --- Name defaulting --- */
  baseline(&pc);
  copy_field(pc.name, sizeof pc.name, "");
  print_case("Empty name (should fall back in opener)", &pc);

  /* --- Ball fullness: chargen-scale 1 / 2 (must map to narrative tiers) --- */
  baseline(&pc);
  pc.ball_fullness = 1;
  print_case("Chargen ball fullness Slight (stored value 1)", &pc);

  baseline(&pc);
  pc.ball_fullness = 2;
  print_case("Chargen ball fullness Heavy (stored value 2)", &pc);

  baseline(&pc);
  pc.ball_fullness = 3;
  print_case("Chargen ball fullness maps tier-2 (stored value 3)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.ball_fullness = 3;
  print_case("Chargen bf=3 Android no balls (tier-2 negotiation)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.ball_fullness = 3;
  print_case("Chargen bf=3 Slime no balls (tier-2 negotiation)", &pc);

  /* --- Herm + female presentation + milk --- */
  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "automatic");
  copy_field(pc.balls, sizeof pc.balls, "automatic");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  pc.milk = 35;
  print_case("Female-presenting herm, Both, Large breasts, milk rank 2", &pc);

  /* --- More races + builds --- */
  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Orc");
  copy_field(pc.build, sizeof pc.build, "Thick");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Thick");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Thick");
  print_case("Orc + Thick build with matching muscle/fat labels", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Centaur");
  copy_field(pc.height, sizeof pc.height, "Very Tall (6' - 7')");
  print_case("Centaur morph + very tall CC height", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Halfling");
  copy_field(pc.height, sizeof pc.height, "Short (4' - 5')");
  print_case("Halfling + short CC height (parentheses, no double tall)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Gnome");
  print_case("Gnome stock (compact durable)", &pc);

  /* --- Gender + neuter --- */
  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "they");
  copy_field(pc.genitalia, sizeof pc.genitalia, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "None");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  print_case("Neuter + they (androgynous face hint)", &pc);

  /* --- Marks: tattoo without scars --- */
  baseline(&pc);
  pc.tattoos = 2;
  pc.scars = 0;
  print_case("Inked but no scars (face stays scarless clause)", &pc);

  /* --- Nipples + female explicit breasts --- */
  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Stocky");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  copy_field(pc.nipple_type, sizeof pc.nipple_type, "Pierced");
  print_case("Female stocky + Large breasts + pierced nipples", &pc);

  /* --- Gear: blank fields (not the word None) --- */
  baseline(&pc);
  copy_field(pc.armor, sizeof pc.armor, "");
  copy_field(pc.weapon, sizeof pc.weapon, "");
  print_case("Gear: empty armor and weapon strings", &pc);

  /* --- Stat floor (no heroic calling-cards line) --- */
  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Elf");
  pc.str = pc.cha = pc.agi = 9;
  pc.cor = 0;
  print_case("Elf all stats below 12, no corruption (capable not legendary)", &pc);

  /* --- Single max stat --- */
  baseline(&pc);
  pc.per = 17;
  pc.str = pc.cha = 11;
  print_case("Single standout stat (perception highest)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.per = 17;
  pc.str = pc.cha = 11;
  print_case("Android single standout PER (torque measure)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.per = 17;
  pc.str = pc.cha = 11;
  print_case("Slime single standout PER (tide measure)", &pc);

  /* --- Herm + full gear + corruption --- */
  baseline(&pc);
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.breasts, sizeof pc.breasts, "Average");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  copy_field(pc.armor, sizeof pc.armor, "Leather jack");
  copy_field(pc.weapon, sizeof pc.weapon, "Hand axe");
  pc.cor = 7;
  pc.milk = 5;
  print_case("Herm both sets + gear + corruption + trace milk", &pc);

  /* --- Vagina-only fullness-style edge: milk 0 --- */
  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.milk = 0;
  copy_field(pc.voice_pitch, sizeof pc.voice_pitch, "Alto");
  copy_field(pc.voice_quality, sizeof pc.voice_quality, "Raspy");
  print_case("Female mage-ish voice Alto/Raspy, no lactation", &pc);

  /* --- More morph races + presentation --- */
  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Lamia");
  copy_field(pc.height, sizeof pc.height, "Average (5' - 5'8\")");
  print_case("Lamia + CC average height (serpent hips/seat)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Kitsune");
  print_case("Kitsune (beast-kin tell)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Dragon morph");
  print_case("Dragon morph (scale-sheen)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime dragon morph");
  print_case("Slime dragon morph (heat under veil)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Golem dragon morph");
  print_case("Golem dragon morph (heat under housing)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Golem");
  print_case("Golem (engineered seams)", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "male");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  print_case("Male-presenting herm Both + Large breasts", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.race, sizeof pc.race, "Minotaur");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Tall (5'8\" - 6')");
  copy_field(pc.build, sizeof pc.build, "Muscular");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  print_case("Female Minotaur Vagina + tall CC height + muscular", &pc);

  baseline(&pc);
  pc.str = pc.tou = pc.spe = pc.intl = pc.wis = pc.will = pc.agi = pc.cha = pc.per =
      11;
  print_case("All stats 11 (no trait >= 12)", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  /* pussy[] is 24 bytes in AetPcSave — keep within buffer or text truncates. */
  copy_field(pc.pussy, sizeof pc.pussy, "neat thatch, pink slit");
  print_case("Explicit pussy phrase (fits 23-char field)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Seraph");
  copy_field(pc.armor, sizeof pc.armor, "Studded leather");
  copy_field(pc.weapon, sizeof pc.weapon, "None");
  print_case("Seraph (wings) + armor only gear branch", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Naga");
  copy_field(pc.height, sizeof pc.height, "Short (4' - 5')");
  print_case("Naga + short CC height (morph clause + short tag)", &pc);

  /* --- Batch: 30 more edge combos (classes, ages, races, gear, stats) --- */
  baseline(&pc);
  copy_field(pc.class_, sizeof pc.class_, "paladin");
  print_case("Class slug paladin", &pc);

  baseline(&pc);
  copy_field(pc.class_, sizeof pc.class_, "rogue");
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Lean");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  print_case("Female rogue vagina lean average height", &pc);

  baseline(&pc);
  copy_field(pc.class_, sizeof pc.class_, "warlock");
  pc.cor = 9;
  print_case("Warlock + corruption 9", &pc);

  baseline(&pc);
  pc.scars = 5;
  pc.tattoos = 0;
  print_case("Heavy scars, zero tattoos", &pc);

  baseline(&pc);
  pc.scars = 0;
  pc.tattoos = 6;
  print_case("Heavy ink, zero scars", &pc);

  baseline(&pc);
  copy_field(pc.build, sizeof pc.build, "Stocky");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Stocky");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Stocky");
  print_case("Stocky build with matching muscle/fat labels", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "automatic");
  copy_field(pc.balls, sizeof pc.balls, "automatic");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  pc.milk = 45;
  print_case("Female-presenting herm rank-2 milk", &pc);

  baseline(&pc);
  pc.milk = 28;
  print_case("Male penis automatic breasts milk rank-2", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Cow morph");
  pc.milk = 55;
  print_case("Cow morph + milk (beast + lactation tag)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Rabbit morph");
  print_case("Rabbit morph (beast tell)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Gorgon");
  print_case("Gorgon (scale-sheen)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Cyborg");
  print_case("Cyborg (engineered seams)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Lizard folk");
  print_case("Lizard folk (predator stillness)", &pc);

  baseline(&pc);
  copy_field(pc.age, sizeof pc.age, "Young");
  print_case("Age field Young", &pc);

  baseline(&pc);
  copy_field(pc.age, sizeof pc.age, "Elder");
  print_case("Age field Elder", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "they");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  print_case("They + vagina (non-binary + vag prose)", &pc);

  baseline(&pc);
  copy_field(pc.height, sizeof pc.height, "Gigantic");
  copy_field(pc.build, sizeof pc.build, "Stocky");
  copy_field(pc.muscle_tone, sizeof pc.muscle_tone, "Thick");
  copy_field(pc.body_fat, sizeof pc.body_fat, "Thick");
  print_case("Gigantic + Stocky + thick muscle/fat", &pc);

  baseline(&pc);
  copy_field(pc.hair_style, sizeof pc.hair_style, "Very Long");
  print_case("Very Long hair style", &pc);

  baseline(&pc);
  copy_field(pc.armor, sizeof pc.armor, "None");
  copy_field(pc.weapon, sizeof pc.weapon, "Longbow");
  print_case("Gear: weapon Longbow only", &pc);

  baseline(&pc);
  copy_field(pc.armor, sizeof pc.armor, "Scholar robes");
  copy_field(pc.weapon, sizeof pc.weapon, "Oak staff");
  print_case("Gear: robes + staff both set", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "they");
  copy_field(pc.genitalia, sizeof pc.genitalia, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "None");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  pc.tattoos = 4;
  print_case("Neuter they + heavy tattoos", &pc);

  baseline(&pc);
  copy_field(pc.gender, sizeof pc.gender, "female");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Vagina");
  copy_field(pc.height, sizeof pc.height, "Average");
  copy_field(pc.build, sizeof pc.build, "Average");
  copy_field(pc.cock, sizeof pc.cock, "None");
  copy_field(pc.balls, sizeof pc.balls, "None");
  copy_field(pc.breasts, sizeof pc.breasts, "Small");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  print_case("Female Small breasts explicit label", &pc);

  baseline(&pc);
  pc.ball_fullness = 95;
  print_case("Legacy ball fullness 95 (tier 3 urgency)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.ball_fullness = 95;
  print_case("Android ball fullness 95 tier-3 with sack", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.ball_fullness = 95;
  print_case("Slime ball fullness 95 tier-3 with sack", &pc);

  baseline(&pc);
  pc.ball_fullness = 45;
  print_case("Legacy ball fullness 45 (tier 2 ache)", &pc);

  baseline(&pc);
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.breasts, sizeof pc.breasts, "Average");
  copy_field(pc.pussy, sizeof pc.pussy, "bare seam");
  copy_field(pc.armor, sizeof pc.armor, "Brigandine");
  copy_field(pc.weapon, sizeof pc.weapon, "Short sword");
  print_case("Herm + explicit short pussy + brigandine gear", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Moon Elf");
  copy_field(pc.height, sizeof pc.height, "Very Tall (6' - 7')");
  pc.wis = 16;
  pc.str = pc.cha = pc.agi = 10;
  pc.cor = 0;
  print_case("Moon Elf very tall + wisdom standout + no corruption", &pc);

  baseline(&pc);
  pc.str = pc.cha = pc.agi = 17;
  pc.tou = pc.spe = pc.intl = pc.wis = pc.will = pc.per = 11;
  print_case("Three-way tie 17 STR CHA AGI (dual calling-cards)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Android");
  pc.str = pc.cha = pc.agi = 17;
  pc.tou = pc.spe = pc.intl = pc.wis = pc.will = pc.per = 11;
  print_case("Android three-way tie 17 STR CHA AGI (torque dual)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Slime");
  pc.str = pc.cha = pc.agi = 17;
  pc.tou = pc.spe = pc.intl = pc.wis = pc.will = pc.per = 11;
  print_case("Slime three-way tie 17 STR CHA AGI (tide dual)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Goblin");
  copy_field(pc.height, sizeof pc.height, "Tiny (3' - 4')");
  print_case("Goblin + tiny CC height", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Centaur");
  copy_field(pc.genitalia, sizeof pc.genitalia, "Both");
  copy_field(pc.breasts, sizeof pc.breasts, "Large");
  copy_field(pc.pussy, sizeof pc.pussy, "None");
  print_case("Centaur herm Large breasts default pussy", &pc);

  baseline(&pc);
  copy_field(pc.skin, sizeof pc.skin, "Emerald Green");
  copy_field(pc.hair_color, sizeof pc.hair_color, "Silver");
  print_case("Non-default skin + hair colors", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Angel");
  copy_field(pc.height, sizeof pc.height, "Average (5' - 5'8\")");
  print_case("Angel CC height (wing-load hips/seat)", &pc);

  baseline(&pc);
  copy_field(pc.race, sizeof pc.race, "Horse morph");
  print_case("Horse morph (infer equine tell)", &pc);

  printf("\nDone.\n");
  return 0;
}
