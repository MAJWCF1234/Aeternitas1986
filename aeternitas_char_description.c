#include "aeternitas_char_description.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static int ieq(const char *a, const char *b) {
  unsigned char ca, cb;
  if (!a || !b) return 0;
  while (*a && *b) {
    ca = (unsigned char)tolower((unsigned char)*a++);
    cb = (unsigned char)tolower((unsigned char)*b++);
    if (ca != cb) return 0;
  }
  return *a == '\0' && *b == '\0';
}

static const char *S(const char *s, const char *fallback) {
  return (s && s[0]) ? s : fallback;
}

static int label_is_none(const char *s) {
  if (!s || !s[0]) return 1;
  return ieq(s, "none");
}

static const char *indef_article_noun(const char *noun) {
  char c;
  if (!noun || !noun[0]) return "a";
  c = (char)tolower((unsigned char)noun[0]);
  if (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') return "an";
  return "a";
}

static int race_is_construct(const AetPcSave *pc);
static int race_is_slime_like(const AetPcSave *pc);
static int race_reads_construct_adjacent(const AetPcSave *pc);

/* Avoid "Slender muscle softened by Slender flesh"; merge when labels match. */
static void muscle_and_softness_clause(const AetPcSave *pc, char *buf,
                                       size_t cap) {
  const char *m = S(pc->muscle_tone, "steady");
  const char *f = S(pc->body_fat, "ordinary");
  if (ieq(pc->build, "Muscular")) {
    if (ieq(m, f)) {
      if (ieq(m, "Slender") || ieq(m, "Lean")) {
        if (race_reads_construct_adjacent(pc))
          snprintf(buf, cap,
                   "Training carved torque into your silhouette from throat to "
                   "ankle — power written large, housing folded tight where "
                   "joints demand");
        else if (race_is_slime_like(pc))
          snprintf(buf, cap,
                   "Training carved strain into your silhouette from throat to "
                   "ankle — power written large, gel folded tight where joints "
                   "demand");
        else
          snprintf(buf, cap,
                   "Training carved muscle onto your silhouette from throat to "
                   "ankle — power written large, flesh folded tight where joints "
                   "demand");
      } else {
        if (race_reads_construct_adjacent(pc))
          snprintf(buf, cap,
                   "%s actuator load owns your silhouette from throat to ankle "
                   "— power written large, housing folded tight where joints "
                   "demand",
                   m);
        else if (race_is_slime_like(pc))
          snprintf(buf, cap,
                   "%s strain owns your silhouette from throat to ankle — power "
                   "written large, bulk folded tight where joints demand",
                   m);
        else
          snprintf(buf, cap,
                   "%s muscle owns your silhouette from throat to ankle — power "
                   "written large, flesh folded tight where joints demand",
                   m);
      }
    } else {
      if (race_reads_construct_adjacent(pc))
        snprintf(buf, cap,
                 "%s torque stacks under %s substrate — lift and labor carved "
                 "your outline; compliance lingers only where mercy allows",
                 m, f);
      else if (race_is_slime_like(pc))
        snprintf(buf, cap,
                 "%s strain stacks under %s bulk — lift and labor carved your "
                 "outline; yield lingers only where mercy allows",
                 m, f);
      else
        snprintf(buf, cap,
                 "%s muscle stacks under %s flesh — lift and labor carved your "
                 "outline; softness lingers only where mercy allows",
                 m, f);
    }
    return;
  }
  if (ieq(m, f)) {
    if (race_reads_construct_adjacent(pc))
      snprintf(buf, cap,
               "%s calibration holds from scalp to heel — torque and tolerance "
               "kept on the same dial, neither starving the other",
               m);
    else if (race_is_slime_like(pc))
      snprintf(buf, cap,
               "%s tone holds from scalp to heel — strain and yield kept on the "
               "same dial, neither starving the other",
               m);
    else
      snprintf(buf, cap,
               "%s tone holds from scalp to heel — muscle and softness kept on "
               "the same dial, neither starving the other",
               m);
    return;
  }
  if (race_reads_construct_adjacent(pc))
    snprintf(buf, cap,
             "%s torque rides under %s substrate — definition shows where labor "
             "landed, compliance where idle cycles still claim their share",
             m, f);
  else if (race_is_slime_like(pc))
    snprintf(buf, cap,
             "%s strain rides under %s bulk — definition shows where labor "
             "landed, yield where rest and appetite still claim their share",
             m, f);
  else
    snprintf(buf, cap,
             "%s muscle rides under %s flesh — definition shows where labor "
             "landed, softness where rest and appetite still claim their share",
             m, f);
}

static int label_is_automatic(const char *s) {
  return s && ieq(s, "automatic");
}

static int has_penis_config(const AetPcSave *pc) {
  return ieq(pc->genitalia, "Penis") || ieq(pc->genitalia, "Both");
}

static int has_vagina_config(const AetPcSave *pc) {
  return ieq(pc->genitalia, "Vagina") || ieq(pc->genitalia, "Both");
}

static int creation_skipped_breast_prompt(const AetPcSave *pc) {
  return has_penis_config(pc) && ieq(pc->gender, "male");
}

static void append(char *out, size_t cap, size_t *pos, const char *fmt, ...) {
  va_list ap;
  size_t p = *pos;
  int n;
  if (!out || cap < 2 || p >= cap - 1) return;
  va_start(ap, fmt);
  n = vsnprintf(out + p, cap - p, fmt, ap);
  va_end(ap);
  if (n < 0) return;
  if ((size_t)n >= cap - p) {
    *pos = cap - 1;
    out[cap - 1] = '\0';
    return;
  }
  *pos = p + (size_t)n;
}

static void append_gear_closer(char *out, size_t cap, size_t *n,
                               const AetPcSave *pc) {
  const char *ar = pc->armor[0] ? pc->armor : "";
  const char *wp = pc->weapon[0] ? pc->weapon : "";
  int anone = label_is_none(ar);
  int wnone = label_is_none(wp);

  if (anone && wnone) {
    if (race_reads_construct_adjacent(pc))
      append(out, cap, n,
             "Dress the scene however merchants invent it — from twenty paces "
             "the story starts under cloth: no polished pauldron catches "
             "lantern-light across your chassis, no sworn blade sings against "
             "your hip with its own name yet. You layer weave, strap, and "
             "buckle the way travelers do — practical, anonymous, replaceable — "
             "and your hands stay light until anger or hunger puts something "
             "solid in them: tankard, reins, haft, throat. Alloy and leather are "
             "ornaments you rent when fortune allows; the specification and "
             "stubborn uptime beneath stay yours either way.");
    else if (race_is_slime_like(pc))
      append(out, cap, n,
             "Dress the scene however merchants invent it — from twenty paces "
             "the story starts under cloth: no lacquered cuirass catches "
             "sunlight across your mantle, no sworn blade sings against your "
             "hip with its own name yet. You layer weave, strap, and buckle the "
             "way travelers do — practical, anonymous, replaceable — and your "
             "hands stay light until anger or hunger puts something solid in "
             "them: tankard, reins, haft, throat. Silk and waxed strap are "
             "ornaments you rent when fortune allows; the tide and measured "
             "viscosity beneath stay yours either way.");
    else
      append(out, cap, n,
             "Dress the scene however merchants invent it — from twenty paces "
             "the story starts under cloth: no lacquered breastplate catches "
             "sunlight across your chest, no sworn blade sings against your hip "
             "with its own name yet. You layer weave, strap, and buckle the way "
             "travelers do — practical, anonymous, replaceable — and your hands "
             "stay light until anger or hunger puts something solid in them: "
             "tankard, reins, haft, throat. Steel and leather are ornaments you "
             "rent when fortune allows; the bone and temper beneath stay yours "
             "either way.");
    return;
  }
  if (!anone && !wnone) {
    if (race_reads_construct_adjacent(pc))
      append(out, cap, n,
             "War-trappings finish what naked probability began: %s rides your "
             "frame — alloy and leather shifting how doorways measure you, how "
             "crowds give way, how lantern-light catches edge instead of only "
             "seam. %s waits ready — worn grip, honest balance — the period at "
             "the end of every threat you choose to spell out loud.",
             ar, wp);
    else if (race_is_slime_like(pc))
      append(out, cap, n,
             "War-trappings finish what naked probability began: %s rides your "
             "frame — leather and slick resin shifting how doorways measure you, "
             "how crowds give way, how lantern-light catches ripple instead of "
             "only curve. %s waits ready — worn grip, honest balance — the "
             "period at the end of every threat you choose to spell out loud.",
             ar, wp);
    else
      append(out, cap, n,
             "War-trappings finish what naked probability began: %s rides your "
             "frame — leather and metal shifting how doorways measure you, how "
             "crowds give way, how lantern-light catches edge instead of only "
             "curve. %s waits ready — worn grip, honest balance — the period at "
             "the end of every threat you choose to spell out loud.",
             ar, wp);
    return;
  }
  if (!anone && wnone) {
    if (race_reads_construct_adjacent(pc))
      append(out, cap, n,
             "%s names your silhouette before your face does — plating, "
             "routing straps, and the honest squeak of harness when you torque "
             "a breath. Your hands stay bare more often than courtly tales "
             "admit, curling around coin, reins, or whatever the hour demands; "
             "no single blade owns your palm yet, so violence stays improvised, "
             "intimate.",
             ar);
    else if (race_is_slime_like(pc))
      append(out, cap, n,
             "%s names your silhouette before your face does — borrowed shells, "
             "straps, and the honest creep of harness when you breathe deep. "
             "Your hands stay bare more often than courtly tales admit, curling "
             "around coin, reins, or whatever the hour demands; no single blade "
             "owns your palm yet, so violence stays improvised, intimate.",
             ar);
    else
      append(out, cap, n,
             "%s names your silhouette before your face does — plates, straps, "
             "and the honest squeak of harness when you breathe deep. Your "
             "hands stay bare more often than courtly tales admit, curling "
             "around coin, reins, or whatever the hour demands; no single blade "
             "owns your palm yet, so violence stays improvised, intimate.",
             ar);
    return;
  }
  if (race_reads_construct_adjacent(pc))
    append(out, cap, n,
           "Your chassis reads light — linen, vent drift, and the honest tick "
           "of plating under strain — while %s insists on its own grammar: "
           "reach, mass, the clean promise that trouble has already chosen its "
           "instrument.",
           wp);
  else if (race_is_slime_like(pc))
    append(out, cap, n,
           "Your mantle reads light — cloth, meniscus edge, and the honest "
           "slide of gel under strain — while %s insists on its own grammar: "
           "reach, drag, the clean promise that trouble has already chosen its "
           "instrument.",
           wp);
  else
    append(out, cap, n,
           "Your torso goes light — cloth, sweat, the mapwork of veins under "
           "strain — while %s insists on its own grammar: reach, weight, the "
           "clean promise that trouble has already chosen its instrument.",
           wp);
}

static int lactation_rank(int milk) {
  if (milk <= 0) return 0;
  if (milk < 25) return 1;
  if (milk < 60) return 2;
  return 3;
}

static int fullness_rank(int bf) {
  if (bf <= 0) return 0;
  /* Character creation stores 1=Slight, 2=Heavy (see ball_fullness = r - 1).
     Legacy tests / saves may use a rough 0–100 scale instead. */
  if (bf <= 3) {
    if (bf == 1)
      return 1;
    if (bf == 2)
      return 3;
    if (bf == 3)
      return 2;
    return 0;
  }
  if (bf < 33)
    return 1;
  if (bf < 66)
    return 2;
  return 3;
}

static const char *scar_phrase(int n) {
  if (n <= 0)
    return "free of obvious scar tissue — whatever broke you left its lesson "
           "elsewhere";
  if (n == 1)
    return "marked by a scar or two that catch light when you turn — small "
           "advertisements of risk survived";
  if (n == 2)
    return "cross-hatched with several old scars that disagree about angle "
           "and age — a ledger of blades, falls, and stubborn healing";
  return "mapped by heavy scarring — skin remembers fights your mouth never "
         "retells the same way twice";
}

static const char *tattoo_phrase(int n) {
  if (n <= 0)
    return "un-inked — whatever symbols you carry stay under skin or in "
           "silence";
  if (n == 1)
    return "carrying a little ink placed with intent — pattern legible only "
           "to someone close enough to deserve it";
  return "worked over with ink until pigment becomes second hide — sigil, "
           "story, and boast layered until nakedness feels unfamiliar";
}

static const char *gender_face_hint(const AetPcSave *pc) {
  if (!pc)
    return "Your features balance in an androgynous band: cheek and lip refuse "
           "the easy verdict, and strangers hesitate before they assign you a "
           "story.";
  if (race_reads_construct_adjacent(pc)) {
    if (ieq(pc->gender, "male"))
      return "Your faceplate reads masculine — heavier brow shelf, jaw filed "
             "toward leverage, mouth corners set for refusal before surprise.";
    if (ieq(pc->gender, "female"))
      return "Your mask reads feminine — softer planes around eyes and mouth, "
             "lashes shading warmth someone threaded into pigment on purpose.";
    return "Your facial calibration hedges — cheek and lip withhold verdict "
           "until strangers blame tuning instead of nerve.";
  }
  if (race_is_slime_like(pc)) {
    if (ieq(pc->gender, "male"))
      return "Your surface hints masculine — brow stacks shadow where softer "
             "curves might pool, jaw tapering into blunt appetite.";
    if (ieq(pc->gender, "female"))
      return "Your surface hints feminine — softer wells around eyes and mouth, "
             "lashes dragging rumor where gravity tries to settle.";
    return "Your mask keeps taxonomy unstable — cheek and lip shimmer between "
           "verdicts until strangers invent patience.";
  }
  if (ieq(pc->gender, "male"))
    return "Your bone structure reads masculine — heavier brow, leaner "
           "jawline, mouth corners that hold discipline more readily than "
           "surprise.";
  if (ieq(pc->gender, "female"))
    return "Your features trend feminine — softer planes around the eyes and "
           "mouth, lashes casting a shade that flatters without begging.";
  return "Your features balance in an androgynous band: cheek and lip refuse "
         "the easy verdict, and strangers hesitate before they assign you a "
         "story.";
}

static const char *hips_clause(const char *hips) {
  if (!hips || !hips[0]) return "Your pelvis keeps an ordinary span";
  if (ieq(hips, "Narrow")) return "Your pelvis runs narrow";
  if (ieq(hips, "Very wide"))
    return "Your pelvis flares wide, a dramatic hip line above the thigh";
  return "Your pelvis holds an unremarkable, steady span";
}

static const char *butt_clause(const char *b) {
  if (!b || !b[0]) return "From behind, your seat is unremarkable";
  if (ieq(b, "Flat")) return "From behind, your seat stays tight and flat";
  if (ieq(b, "Large")) return "From behind, your seat is full and prominent";
  return "From behind, your seat is proportionate, neither sharp nor "
         "exaggerated";
}

static void append_tag(char *buf, size_t cap, size_t *pos, const char *tag) {
  if (!buf || !cap || !pos || !tag || !tag[0]) return;
  if (*pos > 0) {
    size_t rem = cap - *pos;
    int n = snprintf(buf + *pos, rem, ", ");
    if (n < 0 || (size_t)n >= rem) {
      *pos = cap - 1;
      buf[cap - 1] = '\0';
      return;
    }
    *pos += (size_t)n;
  }
  {
    size_t rem = cap - *pos;
    int n = snprintf(buf + *pos, rem, "%s", tag);
    if (n < 0 || (size_t)n >= rem) {
      *pos = cap - 1;
      buf[cap - 1] = '\0';
      return;
    }
    *pos += (size_t)n;
  }
}

static void build_appearance_tags(const AetPcSave *pc, char *buf, size_t cap) {
  size_t p = 0;
  if (!buf || cap < 2 || !pc) return;
  buf[0] = '\0';

  if (ieq(pc->gender, "male"))
    append_tag(buf, cap, &p, "masculine");
  else if (ieq(pc->gender, "female"))
    append_tag(buf, cap, &p, "feminine");
  else
    append_tag(buf, cap, &p, "androgynous");

  if (ieq(pc->genitalia, "Penis"))
    append_tag(buf, cap, &p, "phallic");
  else if (ieq(pc->genitalia, "Vagina"))
    append_tag(buf, cap, &p, "vaginal");
  else if (ieq(pc->genitalia, "Both"))
    append_tag(buf, cap, &p, "dual-sex");
  else
    append_tag(buf, cap, &p, "neuter");

  if (strstr(pc->height, "Tiny"))
    append_tag(buf, cap, &p, "small-framed");
  else if (strstr(pc->height, "Gigantic"))
    append_tag(buf, cap, &p, "giant-framed");
  else if (strstr(pc->height, "Very Tall") || strstr(pc->height, "Tall"))
    append_tag(buf, cap, &p, "tall");
  else if (strstr(pc->height, "Short"))
    append_tag(buf, cap, &p, "short");

  if (ieq(pc->build, "Slender") || ieq(pc->build, "Lean"))
    append_tag(buf, cap, &p, "slim");
  else if (ieq(pc->build, "Muscular"))
    append_tag(buf, cap, &p, "muscular");
  else if (ieq(pc->build, "Stocky") || ieq(pc->build, "Thick"))
    append_tag(buf, cap, &p, "thickset");

  if (ieq(pc->hips, "Very wide")) append_tag(buf, cap, &p, "wide-hipped");
  if (ieq(pc->butt, "Large")) append_tag(buf, cap, &p, "full-rear");
  if (ieq(pc->hair_style, "Bald")) append_tag(buf, cap, &p, "bald");
  if (pc->scars >= 2) append_tag(buf, cap, &p, "scarred");
  if (pc->tattoos >= 1) append_tag(buf, cap, &p, "inked");
  if (pc->milk > 0) append_tag(buf, cap, &p, "lactating");
}

static int has_token_ci(const char *s, const char *needle) {
  size_t i, nlen;
  if (!s || !needle || !needle[0]) return 0;
  nlen = strlen(needle);
  for (i = 0; s[i]; i++) {
    size_t j = 0;
    while (s[i + j] && j < nlen &&
           tolower((unsigned char)s[i + j]) ==
               tolower((unsigned char)needle[j])) {
      j++;
    }
    if (j == nlen) return 1;
  }
  return 0;
}

static const char *race_origin_tag(const AetPcSave *pc) {
  if (race_reads_construct_adjacent(pc))
    return "by artifice";
  if (race_is_slime_like(pc))
    return "by strange chemistry";
  return "by blood";
}

static int race_is_construct(const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  return ieq(r, "Android") || ieq(r, "Cyborg") || ieq(r, "Golem");
}

static int race_is_slime_like(const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  return has_token_ci(r, "slime") || has_token_ci(r, "goo") || ieq(r, "Goomurali");
}

/* Stock construct races or composite strings carrying construct tokens (e.g.
 * Cyborg wolf morph). Slime-like races always lose to slime prose elsewhere. */
static int race_reads_construct_adjacent(const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  if (race_is_slime_like(pc))
    return 0;
  return ieq(r, "Android") || ieq(r, "Cyborg") || ieq(r, "Golem") ||
         has_token_ci(r, "android") || has_token_ci(r, "cyborg") ||
         has_token_ci(r, "golem");
}

/* Full-body scar summary in the marks paragraph (not only the face). */
static const char *scar_ledger_for_pc(const AetPcSave *pc, int n) {
  if (!pc)
    return scar_phrase(n);
  if (race_reads_construct_adjacent(pc)) {
    if (n <= 0)
      return "free of obvious scoring — whatever struck you logged its lesson "
             "elsewhere";
    if (n == 1)
      return "marked by hairline faults that catch hazard-light — small "
             "advertisements of blunt luck survived";
    if (n == 2)
      return "cross-hatched with relay bruises that disagree about angle and "
             "age — a ledger of impacts, drops, and stubborn polish passes";
    return "mapped by heavy relay scoring — plating remembers fights your "
           "mouth never retells the same way twice";
  }
  if (race_is_slime_like(pc)) {
    if (n <= 0)
      return "free of obvious tear scars — insults mend fast enough that risk "
             "stays rumor";
    if (n == 1)
      return "marked by tears that sealed before witnesses agreed — thin "
             "advertisements of violence survived";
    if (n == 2)
      return "cross-hatched with healed seams that disagree about angle — a "
             "ledger of blades, falls, and stubborn surface tension";
    return "mapped by turbulent scarring — gel remembers fights your mouth "
           "never retells the same way twice";
  }
  return scar_phrase(n);
}

/* Tattoo / dye summary beside scars in the marks paragraph. */
static const char *tattoo_ledger_for_pc(const AetPcSave *pc, int n) {
  if (!pc)
    return tattoo_phrase(n);
  if (race_reads_construct_adjacent(pc)) {
    if (n <= 0)
      return "un-etched — whatever sigils you carry live in ROM or in silence";
    if (n == 1)
      return "carrying a little routing inked into laminate — pattern legible "
             "only to someone close enough to deserve the read";
    return "worked over with etch and dye until routing becomes second housing "
           "— sigil, version, and boast layered until bare metal feels "
           "undressed";
  }
  if (race_is_slime_like(pc)) {
    if (n <= 0)
      return "un-inked — whatever symbols you carry stay in gel memory or in "
             "silence";
    if (n == 1)
      return "carrying a little stain that clings with intent — pattern "
             "legible only when meniscus holds still long enough to mean it";
    return "worked over with pigment until color becomes second tide — story, "
           "sigil, and brag layered until surface tension forgets who owns the "
           "room";
  }
  return tattoo_phrase(n);
}

static const char *hips_clause_for_pc(const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  if (has_token_ci(r, "harpy"))
    return "Your hips stay lighter than a grounded human's — built for updraft "
           "and the sudden pivot of talons on stone";
  if (has_token_ci(r, "seraph") || has_token_ci(r, "angel"))
    return "Your upper back carries cantilever muscle — wing roots widen "
           "stance until shoulders answer to loft as much as to soil";
  if (has_token_ci(r, "lamia") || has_token_ci(r, "naga"))
    return "Where a human pelvis would frame stride, serpent girth and belly "
           "scales take over — each coil flexes from ribs to ground";
  if (has_token_ci(r, "centaur"))
    return "Your human waist meets equine barrel at a seam everyone steals "
           "glances at — hips read one way above the join, another below";
  if (has_token_ci(r, "drider"))
    return "Your torso anchors to chitin at the waist — hip-line vocabulary "
           "fails where mammal thighs yield to spider geometry";
  if (has_token_ci(r, "gorgon"))
    return "Your pelvis keeps mortal sway — above it, serpent crowns braid "
           "motion until strangers forget whether to watch waist or myth first";
  if (has_token_ci(r, "lizard"))
    return "Your pelvis hinges with reptile patience — tail memory tugs at the "
           "haunch even when cloth pretends human symmetry";
  if (strstr(r, "Orc"))
    return "Your pelvis shoulders its share of blunt work — hips trade poetry "
           "for clearance when crowds forget to move";
  if (ieq(r, "Goblin") || ieq(r, "Kobold"))
    return "Your pelvis stays compact enough to steal inches — hips pivot "
           "where taller folk never think to guard";
  if (has_token_ci(r, "elf"))
    return "Your pelvis lengthens the stride your blood promised — hips arc "
           "with leisure shorter-lived lines rehearse as myth";
  if (strstr(r, "Dwarf") || ieq(r, "Gnome") || ieq(r, "Halfling"))
    return "Your pelvis sits low and definite — hips anchor you where tavern "
           "floors cup stone honest beneath nail and hoof";
  if (race_is_slime_like(pc))
    return "Your pelvis yields until intent firms it — hips steer tide before "
           "linen earns its excuse";
  if (race_reads_construct_adjacent(pc))
    return "Your pelvis traces arcs torque already settled — hips swing where "
           "calibration allows and substrate pretends it voted";
  return hips_clause(pc ? pc->hips : NULL);
}

static const char *butt_clause_for_pc(const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  if (has_token_ci(r, "harpy"))
    return "From behind, wings dominate outline before cloth lies — below "
           "them, stance favors perch and strike, not chairs meant for softer "
           "creatures";
  if (has_token_ci(r, "seraph") || has_token_ci(r, "angel"))
    return "From behind, shoulders spread for pinions — your seat still reads "
           "human enough for benches while flight tilts posture upward";
  if (has_token_ci(r, "centaur"))
    return "From behind, haunch and tail own the silhouette — flank shifts "
           "like war-bred stock, not gossip-circle softness";
  if (has_token_ci(r, "lamia") || has_token_ci(r, "naga"))
    return "From behind, tail breadth and scale ridge carry the eye — no "
           "human cheek of seat, only serpent length";
  if (has_token_ci(r, "drider"))
    return "From behind, abdomen and spinneret read silhouette — nothing "
           "about your rear answers to cushioned-seat gossip";
  if (has_token_ci(r, "gorgon"))
    return "From behind, living coils sketch threat aloft before your seat "
           "finishes the shape gossip tries to name";
  if (has_token_ci(r, "lizard"))
    return "From behind, tail and scale ridge own the read — no polite cushion "
           "vocabulary fits what upholstery forgot";
  if (strstr(r, "Orc"))
    return "From behind, breadth reads labor before vanity gets a vote — "
           "nothing about you apologizes toward cushions";
  if (ieq(r, "Goblin") || ieq(r, "Kobold"))
    return "From behind, compact muscle favors tuck and bolt — theater spends "
           "what you save for leverage";
  if (has_token_ci(r, "elf"))
    return "From behind, line lengthens like a sentence refusing hurry — "
           "silhouette finishes thoughts slower folk spread across gossip";
  if (strstr(r, "Dwarf") || ieq(r, "Gnome") || ieq(r, "Halfling"))
    return "From behind, solidity owns the shadow — bench-stone and tunnel "
           "echo answer before flirt might";
  if (race_reads_construct_adjacent(pc))
    return "From behind, plating and seam-lines sketch posture metal remembers "
           "— cushion gossip never reads the manual";
  return butt_clause(pc ? pc->butt : NULL);
}

static const char *silhouette_motion_phrase(const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  if (has_token_ci(r, "centaur"))
    return "the shift of human ribs above a barrel already choosing its next "
           "stride";
  if (has_token_ci(r, "lamia") || has_token_ci(r, "naga"))
    return "the muscle of coils rolling under weave that pretends at hips";
  if (has_token_ci(r, "drider"))
    return "the arithmetic of too many joints negotiating one doorway";
  if (has_token_ci(r, "harpy"))
    return "the cant of folded wings still arguing with gravity";
  if (has_token_ci(r, "seraph") || has_token_ci(r, "angel"))
    return "primaries still arguing with thresholds meant for feet alone";
  if (race_is_slime_like(pc))
    return "outline yielding until purpose firms it";
  if (has_token_ci(r, "horse"))
    return "the ghost of a gallop hiding in calves that walk upright";
  if (has_token_ci(r, "wolf"))
    return "the restless angle of ears sketching attention before your mouth "
           "moves";
  if (has_token_ci(r, "rabbit"))
    return "compact weight primed to bolt before thought catches up";
  if (has_token_ci(r, "cow"))
    return "the patient sway of mass that remembers stalls and shoulder checks";
  if (has_token_ci(r, "dragon"))
    return "scale-light shifting while tail completes the arc your spine began";
  if (has_token_ci(r, "kitsune") || has_token_ci(r, "neko"))
    return "the restless flourish of tails rewriting space behind you";
  if (has_token_ci(r, "gorgon"))
    return "serpent crowns weaving menace before your stride commits to stay "
           "or strike";
  if (has_token_ci(r, "lizard"))
    return "tail counterweight finishing arcs your shoulders only started";
  if (strstr(r, "Orc"))
    return "mass borrowing the lane before courtesy remembers its manners";
  if (ieq(r, "Goblin") || ieq(r, "Kobold"))
    return "low lines trading reach for leverage where taller witnesses blink "
           "first";
  if (has_token_ci(r, "elf"))
    return "elongated calm that makes thresholds feel briefly young again";
  if (strstr(r, "Dwarf") || ieq(r, "Gnome") || ieq(r, "Halfling"))
    return "patient density claiming floorboards before flattery finds pitch";
  if (race_reads_construct_adjacent(pc))
    return "tolerance stacking where biology expected slack and courtesy "
           "pretended noise";
  return "the swing of hip against gravity";
}

/* Between collarbone cant and motion phrase in the silhouette/tags paragraph. */
static const char *silhouette_scan_clause(const AetPcSave *pc) {
  if (race_reads_construct_adjacent(pc))
    return "the cant of casing where linen lies about torque paths";
  if (race_is_slime_like(pc))
    return "the ripple where gel rehearses ribs under veil";
  return "the tilt of ribs under linen";
}

/* Second clause of the voice paragraph — vary spatial metaphor off biped rooms. */
static const char *voice_space_opening(const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  if (has_token_ci(r, "centaur"))
    return "It fills a barn aisle and a tavern crowd differently depending "
           "on temper";
  if (has_token_ci(r, "lamia") || has_token_ci(r, "naga"))
    return "It travels ribs and coil before strangers finish parsing diction "
           "differently depending on temper";
  if (has_token_ci(r, "drider"))
    return "It threads silk-minded silence through rooms sized for fewer legs "
           "differently depending on temper";
  if (has_token_ci(r, "harpy"))
    return "It rings off perch, stone, and sudden wind-gap differently "
           "depending on temper";
  if (has_token_ci(r, "seraph") || has_token_ci(r, "angel"))
    return "It lifts toward rafter-breath and open sky differently depending "
           "on temper";
  if (race_is_slime_like(pc))
    return "It carries through gel and rumor differently depending on temper";
  if (has_token_ci(r, "minotaur"))
    return "It shoulders down corridors before excuses arrive differently "
           "depending on temper";
  if (has_token_ci(r, "gorgon"))
    return "It teaches listeners how small their breath can go differently "
           "depending on temper";
  if (has_token_ci(r, "lizard"))
    return "It puts hiss and heat ahead of pretty apologies differently "
           "depending on temper";
  if (has_token_ci(r, "dragon"))
    return "It banks under scales before courtesy cools differently depending "
           "on temper";
  if (strstr(r, "Orc"))
    return "It carries tunnel-width and war-camp bluntness differently "
           "depending on temper";
  if (ieq(r, "Goblin") || ieq(r, "Kobold"))
    return "It slips through gaps polite rooms pretend were never there "
           "differently depending on temper";
  if (has_token_ci(r, "elf"))
    return "It braids cool vowels through chambers built for shorter memories "
           "differently depending on temper";
  if (strstr(r, "Dwarf") || ieq(r, "Gnome") || ieq(r, "Halfling"))
    return "It rings hearth and stone closer than tallfolk bother to hear "
           "differently depending on temper";
  if (race_reads_construct_adjacent(pc))
    return "It couples through seams and borrowed lung-space differently "
           "depending on temper";
  return "It fills a doorway differently depending on temper";
}

/* Voice §: what braids into sound before "that carries farther…". */
static const char *voice_braid_phrase(const AetPcSave *pc) {
  if (race_reads_construct_adjacent(pc))
    return "coil, exciter mesh, and mapped habit braided into sound";
  if (race_is_slime_like(pc))
    return "bubble-rise, meniscus pull, and stubborn habit braided into sound";
  return "breath, cord, and habit braided into sound";
}

static void append_space_carriage_coda(char *out, size_t cap, size_t *n,
                                       const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  const char *race = S(pc->race, "Human");
  if (race_reads_construct_adjacent(pc))
    append(out, cap, n,
           "Each inch of you negotiates space the way torque budgets and seam "
           "allowance shaped by %s specification learn to: ",
           race);
  else if (race_is_slime_like(pc))
    append(out, cap, n,
           "Each inch of you negotiates space the way yielding mass and "
           "surface tension shaped by %s strain learn to: ",
           race);
  else
    append(out, cap, n,
           "Each inch of you negotiates space the way flesh shaped by %s blood "
           "learns to: ",
           race);
  if (has_token_ci(r, "centaur"))
    append(out, cap, n,
           "shoulders claim lintels drafted for two-legged pride while hooves "
           "measure cobble and straw — your outline reads herd before flattery "
           "catches up.\n\n");
  else if (has_token_ci(r, "lamia") || has_token_ci(r, "naga"))
    append(out, cap, n,
           "coils budget doorways before courtesy does — cloth lies until you "
           "declare how much length the room still owes your tail.\n\n");
  else if (has_token_ci(r, "drider"))
    append(out, cap, n,
           "extra limbs invoice geometry meant for pairs — furniture and "
           "patience revise together.\n\n");
  else if (has_token_ci(r, "harpy"))
    append(out, cap, n,
           "updraft argues with anyone who expects floor-bound etiquette — "
           "gravity stays negotiable.\n\n");
  else if (has_token_ci(r, "seraph") || has_token_ci(r, "angel"))
    append(out, cap, n,
           "pinions shame lintels drafted for modest saints — light catches "
           "feathers before it excuses your passage.\n\n");
  else if (ieq(r, "Goomurali"))
    append(out, cap, n,
           "outline trades honesty with every threshold — mass shifts before "
           "strangers finish pretending rooms stay dry.\n\n");
  else if (has_token_ci(r, "slime") || has_token_ci(r, "goo"))
    append(out, cap, n,
           "outline yields until purpose firms it — mass follows mood as "
           "often as appetite.\n\n");
  else if (has_token_ci(r, "minotaur"))
    append(out, cap, n,
           "horns audition lintels before excuses do — mass arrives "
           "shoulder-first when crowds forget to yield.\n\n");
  else if (has_token_ci(r, "horse"))
    append(out, cap, n,
           "flank memory lengthens pace — stairs forgive you less than pasture "
           "ever did.\n\n");
  else if (has_token_ci(r, "wolf"))
    append(out, cap, n,
           "ears crown silhouette before face arrives — habit listens sideways "
           "while strangers pretend otherwise.\n\n");
  else if (has_token_ci(r, "rabbit"))
    append(out, cap, n,
           "stillness and bolt trade places fast — compact weight hides until "
           "flight commits.\n\n");
  else if (has_token_ci(r, "cow"))
    append(out, cap, n,
           "haunch gravity remembers herd manners — mass shoulders through "
           "crowds like stalls taught it to.\n\n");
  else if (has_token_ci(r, "dragon"))
    append(out, cap, n,
           "scale edges audition shadow — tail finishes the sentence your "
           "spine begins before modest maps catch up.\n\n");
  else if (has_token_ci(r, "kitsune") || has_token_ci(r, "neko"))
    append(out, cap, n,
           "tails and ears negotiate gossip before your mouth bothers — rumor "
           "travels along fur and flourish alike.\n\n");
  else if (has_token_ci(r, "gorgon"))
    append(out, cap, n,
           "stone myths borrow your reflection — strangers learn glance "
           "discipline before they learn your given name.\n\n");
  else if (has_token_ci(r, "lizard"))
    append(out, cap, n,
           "tail and dewlap audit shadows humans treat as empty — heat meets "
           "tile quietly until temper proves otherwise.\n\n");
  else if (strstr(r, "Orc"))
    append(out, cap, n,
           "shoulders treat thresholds as debt — you collect clearance before "
           "you collect apologies.\n\n");
  else if (ieq(r, "Goblin") || ieq(r, "Kobold"))
    append(out, cap, n,
           "ceilings feel generous while floorboards remember leverage — you "
           "take corners before quarrels earn their map.\n\n");
  else if (has_token_ci(r, "elf"))
    append(out, cap, n,
           "light finds longer bones first — lintels measure you like borrowed "
           "clothes until posture returns them.\n\n");
  else if (strstr(r, "Dwarf") || ieq(r, "Gnome") || ieq(r, "Halfling"))
    append(out, cap, n,
           "stone learns your weight in passing — low centers make narrow "
           "rooms honest about who owns the floor.\n\n");
  else if (race_reads_construct_adjacent(pc))
    append(out, cap, n,
           "joints and plating admit torque courtesy pretends not to need — "
           "fabric hides mechanics more honest than bare skin.\n\n");
  else
    append(out, cap, n,
           "shoulders width the doorway first, hips decide how cloth falls, "
           "and the eye follows line before it hunts for coin or "
           "insignia.\n\n");
}

static void append_default_vag_anatomy(char *out, size_t cap, size_t *n,
                                       const AetPcSave *pc) {
  const char *r = pc && pc->race[0] ? pc->race : "";
  if (has_token_ci(r, "lamia") || has_token_ci(r, "naga"))
    append(out, cap, n,
           "Where belly scales shelter what modest maps mislabel, heat banks "
           "close — slick gathers quiet along the seam until welcome earns the "
           "bend.\n\n");
  else if (race_reads_construct_adjacent(pc))
    append(out, cap, n,
           "Your cunt sits forward in your stride — thermal bloom banks "
           "behind the same hips that sway when you walk, seams warming "
           "wherever linen pretends it keeps secrets dry.\n\n");
  else if (race_is_slime_like(pc))
    append(out, cap, n,
           "Your cunt sits forward in your stride — warmth pools behind the "
           "same hips that sway when you walk, meniscus film climbing wherever "
           "cloth lies about how dry you stay.\n\n");
  else
    append(out, cap, n,
           "Your cunt sits forward in your stride — heat kept behind the "
           "same hips that sway when you walk, thighs brushing with a secret "
           "humidity cloth tries and fails to hide entirely.\n\n");
}

/* Non-human / extra morphology; empty buffer means nothing to add (human norm). */
static void infer_modification_prose(const AetPcSave *pc, char *buf, size_t cap) {
  size_t p = 0;
  const char *r = pc->race[0] ? pc->race : "Human";
  if (!buf || cap < 2) return;
  buf[0] = '\0';

  /* Harpy + Seraph/Angel: wing and flight muscle live in race_extra_clause so
   * we do not repeat wing beats here. */
  /* Cow morph gets creature body copy in race_extra_clause; omit duplicate
   * herd-flavor here. */
  if (has_token_ci(r, "wolf") || has_token_ci(r, "kitsune") ||
      has_token_ci(r, "neko")) {
    if (race_is_slime_like(pc))
      append(buf, cap, &p,
             "%sBeast strain shows in ear-set, stance, and the small tells of "
             "movement no tailor can fully hide.",
             p ? " " : "");
    else if (race_reads_construct_adjacent(pc))
      append(buf, cap, &p,
             "%sBeast routing shows in ear-set, stance, and the small tells of "
             "movement no tailor can fully hide.",
             p ? " " : "");
    else
      append(buf, cap, &p,
             "%sBeast blood shows in ear-set, stance, and the small tells of "
             "movement no tailor can fully hide.",
             p ? " " : "");
  }
  /* Dragon morph gets scales in race_extra; plain dragon/gorgon/lizard keep
   * this gloss without duplicating morph paragraphs. */
  if (has_token_ci(r, "gorgon") || has_token_ci(r, "lizard") ||
      (has_token_ci(r, "dragon") && !strstr(r, "morph"))) {
    if (race_reads_construct_adjacent(pc))
      append(buf, cap, &p,
             "%sScale-sheen and a predator stillness cling to plating and "
             "profile.",
             p ? " " : "");
    else if (race_is_slime_like(pc))
      append(buf, cap, &p,
             "%sScale-sheen and a predator stillness cling to veil and profile.",
             p ? " " : "");
    else
      append(buf, cap, &p,
             "%sScale-sheen and a predator stillness cling to skin and profile.",
             p ? " " : "");
  }
  if (has_token_ci(r, "slime") || has_token_ci(r, "goo")) {
    /* race_extra_clause already glosses pure slime races; avoid doubling moist
     * gleam when the race line carries it. */
    if (!(strstr(r, "Slime") || ieq(r, "Goomurali"))) {
      if (race_reads_construct_adjacent(pc))
        append(buf, cap, &p,
               "%sMoist gleam and soft, uncertain edges replace crisp anatomy "
               "where substrate alone draws the limit.",
               p ? " " : "");
      else if (race_is_slime_like(pc))
        append(buf, cap, &p,
               "%sMoist gleam and soft, uncertain edges replace crisp anatomy "
               "where tide alone draws the limit.",
               p ? " " : "");
      else
        append(buf, cap, &p,
               "%sMoist gleam and soft, uncertain edges replace crisp anatomy "
               "where flesh should stop.",
               p ? " " : "");
    }
  }
  if (has_token_ci(r, "android") || has_token_ci(r, "cyborg") ||
      has_token_ci(r, "golem")) {
    /* Construct stock races get seams gloss in race_extra_clause; keep this for
     * partial tokens (e.g. mixed morphs) without repeating polish cadence. */
    if (!race_is_construct(pc))
      append(buf, cap, &p,
             "%sSeams, polish, and engineered weight interrupt what biology alone "
             "would settle.",
             p ? " " : "");
  }
}

static void stat_carriage_line(const AetPcSave *pc, char *buf, size_t bufcap) {
  struct {
    const char *name;
    int v;
  } t[] = {{"strength", pc->str},  {"toughness", pc->tou},
            {"speed", pc->spe},   {"agility", pc->agi},
            {"intellect", pc->intl}, {"wisdom", pc->wis},
            {"will", pc->will},   {"charisma", pc->cha},
            {"perception", pc->per}};
  int i;
  int vmax = t[0].v;
  for (i = 1; i < 9; i++) {
    if (t[i].v > vmax) vmax = t[i].v;
  }
  if (vmax < 12) {
    if (race_reads_construct_adjacent(pc))
      snprintf(buf, bufcap,
               "Your chassis does not yet advertise any single heroic extreme — "
               "you read as capable, not legendary.");
    else if (race_is_slime_like(pc))
      snprintf(buf, bufcap,
               "Your mantle does not yet advertise any single heroic extreme — "
               "you read as capable, not legendary.");
    else
      snprintf(buf, bufcap,
               "Your body does not yet broadcast any single heroic extreme — "
               "you read as capable, not legendary.");
    return;
  }

  int atmax[9];
  int nat = 0;
  for (i = 0; i < 9; i++) {
    if (t[i].v == vmax) atmax[nat++] = i;
  }
  {
    int swapped;
    do {
      swapped = 0;
      for (i = 0; i < nat - 1; i++) {
        if (strcmp(t[atmax[i]].name, t[atmax[i + 1]].name) > 0) {
          int tmp = atmax[i];
          atmax[i] = atmax[i + 1];
          atmax[i + 1] = tmp;
          swapped = 1;
        }
      }
    } while (swapped);
  }

  if (nat >= 2) {
    if (race_reads_construct_adjacent(pc))
    snprintf(buf, bufcap,
               "Wear and torque have marked you: especially %s and %s "
               "telegraph first in motion and in a fight.",
               t[atmax[0]].name, t[atmax[1]].name);
    else if (race_is_slime_like(pc))
      snprintf(buf, bufcap,
               "Wear and tide have marked you: especially %s and %s "
               "surface first in motion and in a fight.",
               t[atmax[0]].name, t[atmax[1]].name);
    else
      snprintf(buf, bufcap,
               "Wear and habit have marked you: especially %s and %s are your "
               "calling cards in motion and in a fight.",
               t[atmax[0]].name, t[atmax[1]].name);
    return;
  }

  {
    int imax = atmax[0];
    int second = -1;
    for (i = 0; i < 9; i++) {
      if (i == imax) continue;
      if (second < 0 || t[i].v > t[second].v) second = i;
  }
  if (second >= 0 && t[second].v >= 12) {
      if (race_reads_construct_adjacent(pc))
    snprintf(buf, bufcap,
                 "Wear and torque have marked you: especially %s and %s "
                 "telegraph first in motion and in a fight.",
                 t[imax].name, t[second].name);
      else if (race_is_slime_like(pc))
        snprintf(buf, bufcap,
                 "Wear and tide have marked you: especially %s and %s "
                 "surface first in motion and in a fight.",
                 t[imax].name, t[second].name);
      else
        snprintf(buf, bufcap,
                 "Wear and habit have marked you: especially %s and %s are your "
                 "calling cards in motion and in a fight.",
             t[imax].name, t[second].name);
  } else {
      if (race_reads_construct_adjacent(pc))
    snprintf(buf, bufcap,
                 "Wear and torque have marked you: %s is the trait strangers "
                 "parse through plating when they take your measure.",
                 t[imax].name);
      else if (race_is_slime_like(pc))
        snprintf(buf, bufcap,
                 "Wear and tide have marked you: %s is the trait strangers "
                 "read through sheen when they take your measure.",
                 t[imax].name);
      else
        snprintf(buf, bufcap,
                 "Wear and habit have marked you: %s is the trait strangers feel "
                 "first when they take your measure.",
             t[imax].name);
    }
  }
}

static void race_extra_clause(const AetPcSave *pc, char *buf, size_t bufcap) {
  const char *r = pc->race[0] ? pc->race : "Human";
  buf[0] = '\0';
  if (ieq(r, "Human")) {
    snprintf(buf, bufcap,
             "Human symmetry grounds your look — adaptable, familiar to every "
             "market square.");
    return;
  }
  if (strstr(r, "Elf") || strstr(r, "elf")) {
    snprintf(buf, bufcap,
             "%s heritage leaves long bones and an edge of otherworldly calm "
             "around the eyes.",
             r);
    return;
  }
  if (strstr(r, "Dwarf") || ieq(r, "Gnome") || ieq(r, "Halfling")) {
    snprintf(buf, bufcap,
             "%s stock runs compact and durable — built closer to the stone "
             "than to the sky.",
             r);
    return;
  }
  if (ieq(r, "Minotaur")) {
    snprintf(buf, bufcap,
             "Minotaur blood writes heavy horns and a bull-forward sweep of "
             "skull — neck and shoulders carry mass meant for charge and "
             "butting room as much as blade-work.");
    return;
  }
  if (strstr(r, "Orc") || ieq(r, "Goblin") || ieq(r, "Kobold")) {
    snprintf(buf, bufcap,
             "%s blood shows in thicker hide and a silhouette meant for "
             "crowding doorways.",
             r);
    return;
  }
  if (ieq(r, "Neko") || ieq(r, "Kitsune")) {
    snprintf(buf, bufcap,
             "Beast-kin detail threads through your look — ears and restless "
             "grace where humans stay blunt.");
    return;
  }

  /* What this body *is* — centaur, drider, serpent, harpy, morphs, etc. */
  if (has_token_ci(r, "centaur")) {
    snprintf(buf, bufcap,
             "You are centaur-built — human torso and arms mount above an "
             "equine barrel, haunches, and tail; four hooves wear the road "
             "while your stride shifts between walk and the thunder humans "
             "only pretend at.");
    return;
  }
  if (has_token_ci(r, "drider")) {
    snprintf(buf, bufcap,
             "You are drider-shaped — a spider's cephalothorax and eight legs "
             "carry what used to be a waist; silk-minded joints flex where "
             "mammal thighs once ended.");
    return;
  }
  if (has_token_ci(r, "lamia") || has_token_ci(r, "naga")) {
    snprintf(buf, bufcap,
             "You are serpent from the waist down — belly scales flex in "
             "muscle rings, tail coils and drags its weight behind you, and "
             "contact with earth travels along scales before strangers read "
             "your face.");
    return;
  }
  if (has_token_ci(r, "harpy")) {
    snprintf(buf, bufcap,
             "You are harpy-formed — flight feathers arc from shoulders, and "
             "your gait lands on talons built for perch and prey, not polite "
             "flooring.");
    return;
  }
  if (has_token_ci(r, "seraph") || has_token_ci(r, "angel")) {
    snprintf(buf, bufcap,
             "Winged bearing stacks muscle along wing roots — shoulder blade "
             "and spine bulk where pinions anchor, and your outline remembers "
             "loft even when boots stay earth-bound.");
    return;
  }

  if (strstr(r, "morph")) {
    if (has_token_ci(r, "cow")) {
      snprintf(buf, bufcap,
               "Cow kin shapes your frame — flank and belly carry bovine "
               "breadth; hips and chest settle under weight and sway honest "
               "dairy tells without asking permission.");
      return;
    }
    if (has_token_ci(r, "rabbit")) {
      snprintf(buf, bufcap,
               "Rabbit morph leaves you long in foot and light through the "
               "haunch — ears flag danger before your mouth commits, and "
               "your legs remember sprint.");
      return;
    }
    if (has_token_ci(r, "wolf")) {
      snprintf(buf, bufcap,
               "Wolf morph pulls your shoulders forward — breath hunts scent "
               "before sight finishes its verdict, and your jaw remembers "
               "shear even when courtesy hides it.");
      return;
    }
    if (has_token_ci(r, "horse")) {
      snprintf(buf, bufcap,
               "Horse morph lengthens flank and neck — hips remember gallop "
               "even when boots pretend civility, and your lungs trade tavern "
               "smoke for remembered pasture wind.");
      return;
    }
    if (has_token_ci(r, "dragon")) {
      if (race_is_slime_like(pc))
        snprintf(buf, bufcap,
                 "Dragon morph armors hide along sternum and flank — scale "
                 "edges catch lantern-light, and heat banks quiet under veil "
                 "as if something winged slept there.");
      else if (race_reads_construct_adjacent(pc))
        snprintf(buf, bufcap,
                 "Dragon morph armors hide along sternum and flank — scale "
                 "edges catch lantern-light, and heat banks quiet under "
                 "housing as if something winged slept there.");
      else
        snprintf(buf, bufcap,
                 "Dragon morph armors hide along sternum and flank — scale "
                 "edges catch lantern-light, and heat banks quiet under skin "
                 "as if something winged slept there.");
      return;
    }
    if (race_reads_construct_adjacent(pc)) {
      snprintf(buf, bufcap,
               "%s routing bends your outline away from the human mean — joints "
               "and proportions answer to a stranger blueprint.",
             r);
    return;
  }
    snprintf(buf, bufcap,
             "%s blood bends your outline away from the human mean — joints "
             "and proportions answer to a stranger blueprint.",
             r);
    return;
  }

  if (strstr(r, "Slime") || ieq(r, "Goomurali")) {
    snprintf(buf, bufcap,
             "Your surface catches light oddly — moist gleam, subtle translucence "
             "— where sober anatomy charts drew meat alone.");
    return;
  }
  if (race_reads_construct_adjacent(pc)) {
    snprintf(buf, bufcap,
             "Constructed lines interrupt biology: seams, sheen, or weight where "
             "soft tissue ought to dominate.");
    return;
  }
  if (race_is_slime_like(pc))
    snprintf(buf, bufcap,
             "%s traits lace through surface and stance until strain reads before "
             "profession.",
             r);
  else
  snprintf(buf, bufcap,
           "%s traits lace through skin and stance until race reads before "
           "profession.",
           r);
}

void aet_describe_pc(const AetPcSave *pc, char *out, size_t outcap) {
  size_t n = 0;
  char statbuf[320];
  char racebuf[512];
  char tagsbuf[320];
  char modbuf[768];
  char musfat[384];
  const char *breasts;
  const char *nip;
  const char *cock;
  const char *balls_d;
  const char *pussy;
  int lr, fr;
  int pen = 0, vag = 0, skip_breast_cc = 0;
  int describe_breasts = 0;
  int lr_used = 0;

  if (!out || outcap < 2) return;
  if (!pc) {
    snprintf(out, outcap, "No character profile loaded.");
    return;
  }

  pen = has_penis_config(pc);
  vag = has_vagina_config(pc);
  skip_breast_cc = creation_skipped_breast_prompt(pc);

  breasts = pc->breasts[0] ? pc->breasts : "";
  nip = pc->nipple_type[0] ? pc->nipple_type : "";
  cock = pc->cock[0] ? pc->cock : "";
  balls_d = pc->balls[0] ? pc->balls : "";
  pussy = pc->pussy[0] ? pc->pussy : "";
  lr = lactation_rank(pc->milk);
  fr = fullness_rank(pc->ball_fullness);

  describe_breasts = !skip_breast_cc || !label_is_none(breasts);

  stat_carriage_line(pc, statbuf, sizeof statbuf);
  race_extra_clause(pc, racebuf, sizeof racebuf);
  build_appearance_tags(pc, tagsbuf, sizeof tagsbuf);
  infer_modification_prose(pc, modbuf, sizeof modbuf);
  muscle_and_softness_clause(pc, musfat, sizeof musfat);

  /* --- Paragraph 1: identity, stature, race gloss --- */
  append(out, outcap, &n,
         "You are %s — %s, %s %s, presenting as %s, with the habits and "
         "bearing of %s. ",
         S(pc->name, "Wanderer"), S(pc->age, "an adult"),
         S(pc->race, "Human"), race_origin_tag(pc), S(pc->gender, "they"),
         S(pc->class_, "an adventurer"));
  {
    const char *h = pc->height[0] ? pc->height : "";
    const char *b = S(pc->build, "balanced");
    const char *aa = indef_article_noun(b);
    if (strchr(h, '(') != NULL) {
      if (race_reads_construct_adjacent(pc))
  append(out, outcap, &n,
               "You stand %s on %s %s frame — %s, lintels read as suggestions. ",
               h, aa, b, musfat);
      else if (race_is_slime_like(pc))
  append(out, outcap, &n,
               "You stand %s on %s %s frame — %s, doorways learn patience. ", h,
               aa, b, musfat);
      else
        append(out, outcap, &n, "You stand %s on %s %s frame — %s. ", h, aa, b,
               musfat);
    } else if (ieq(h, "Average")) {
      if (race_reads_construct_adjacent(pc))
        append(out, outcap, &n,
               "You stand at average height on %s %s frame — %s, nominal where "
               "the chart pretends neutrality. ",
               aa, b, musfat);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "You stand at average height on %s %s frame — %s, quiet until "
               "tide argues. ",
               aa, b, musfat);
      else
        append(out, outcap, &n,
               "You stand at average height on %s %s frame — %s. ", aa, b,
               musfat);
    } else if (ieq(h, "Short")) {
      if (race_reads_construct_adjacent(pc))
        append(out, outcap, &n,
               "You stand short for your manufacture band on %s %s frame — %s. ",
               aa, b, musfat);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "You stand short where your strain prefers density over span on "
               "%s %s frame — %s. ",
               aa, b, musfat);
      else
        append(out, outcap, &n,
               "You stand short for your bloodline on %s %s frame — %s. ", aa, b,
               musfat);
    } else if (ieq(h, "Gigantic")) {
      if (race_reads_construct_adjacent(pc))
        append(out, outcap, &n,
               "You stand gigantic by manufacture intent on %s %s frame — %s. ",
               aa, b, musfat);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "You stand gigantic where mass pooled upward on %s %s frame — "
               "%s. ",
               aa, b, musfat);
      else
        append(out, outcap, &n,
               "You stand gigantic in stature on %s %s frame — %s. ", aa, b,
               musfat);
    } else if (ieq(h, "Tall")) {
      if (race_reads_construct_adjacent(pc))
        append(out, outcap, &n,
               "You stand tall by engineered reach on %s %s frame — %s. ", aa, b,
               musfat);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "You stand tall where tide drew you up on %s %s frame — %s. ", aa,
               b, musfat);
      else
        append(out, outcap, &n,
               "You stand taller than many peers on %s %s frame — %s. ", aa, b,
               musfat);
    } else if (!h[0]) {
      if (race_reads_construct_adjacent(pc))
        append(out, outcap, &n,
               "You stand at average height on %s %s frame — %s, charts assume "
               "what calibration never promised. ",
               aa, b, musfat);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "You stand at average height on %s %s frame — %s, tide levels "
               "where gossip hoped for drama. ",
               aa, b, musfat);
      else
        append(out, outcap, &n,
               "You stand at average height on %s %s frame — %s. ", aa, b,
               musfat);
    } else {
      if (race_reads_construct_adjacent(pc))
        append(out, outcap, &n,
               "You stand %s tall on %s %s frame — %s, reach rated without "
               "apology. ",
               h, aa, b, musfat);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "You stand %s tall on %s %s frame — %s, span borrowed from "
               "appetite. ",
               h, aa, b, musfat);
      else
        append(out, outcap, &n, "You stand %s tall on %s %s frame — %s. ", h, aa,
               b, musfat);
    }
  }
  append_space_carriage_coda(out, outcap, &n, pc);

  append(out, outcap, &n, "%s\n\n", racebuf);

  if (tagsbuf[0]) {
    if (race_reads_construct_adjacent(pc))
  append(out, outcap, &n,
             "Before strangers settle on your face, they swallow the whole "
             "silhouette — %s. That first read rides junction cant, %s, and "
             "%s; detail "
             "arrives later, when curiosity wins over caution.\n\n",
             tagsbuf, silhouette_scan_clause(pc), silhouette_motion_phrase(pc));
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Before strangers settle on your face, they swallow the whole "
             "silhouette — %s. That first read rides surface cant, %s, and "
             "%s; detail "
             "arrives later, when curiosity wins over caution.\n\n",
             tagsbuf, silhouette_scan_clause(pc), silhouette_motion_phrase(pc));
    else
      append(out, outcap, &n,
             "Before strangers settle on your face, they swallow the whole "
             "silhouette — %s. That first read rides collarbone cant, %s, and "
             "%s; detail "
             "arrives later, when curiosity wins over caution.\n\n",
             tagsbuf, silhouette_scan_clause(pc), silhouette_motion_phrase(pc));
  }

  /* --- Paragraph 2: face, eyes, hair, skin --- */
  append(out, outcap, &n, "%s ", gender_face_hint(pc));
  append(out, outcap, &n,
         "Light finds %s eyes and %s hair worn %s — strand, shadow, and sheen "
         "arguing together before a single word lands; ",
         S(pc->eye_color, "steady"), S(pc->hair_color, "natural"),
         S(pc->hair_style, "plainly"));
  if (ieq(pc->hair_style, "Bald")) {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "your dome stays cleared — sensor rings and bare laminate honest "
             "to hazard-light and witness alike. ");
    else if (race_is_slime_like(pc))
  append(out, outcap, &n,
             "your crown stays slick — meniscus offers combs nothing to "
             "purchase, honest to drift and witness alike. ");
    else
  append(out, outcap, &n,
             "your scalp stays bare and blunt, skull honest to weather and "
             "witness alike. ");
  }
  if (race_reads_construct_adjacent(pc)) {
  append(out, outcap, &n,
           "%s finish rides chassis lines your %s specification threaded "
           "through mimic bone — seam, sheen, and stubborn uptime tell their "
           "own weather. Up close the palette sharpens: pores, ",
           S(pc->skin, "fair"), S(pc->race, "Human"));
  } else if (race_is_slime_like(pc)) {
    append(out, outcap, &n,
           "%s gleam coats surfaces your %s strain cannot decide whether to "
           "hold — meniscus, ripple, and blunt appetite tell their own weather. "
           "Up close the palette sharpens: pores, ",
           S(pc->skin, "fair"), S(pc->race, "Human"));
  } else {
    append(out, outcap, &n,
           "%s skin pulls taut over the meat your %s lineage stamped into bone "
           "— freckle, flush, and undertone tell their own weather. Up close "
           "the palette sharpens: pores, ",
           S(pc->skin, "fair"), S(pc->race, "Human"));
  }
  if (pc->scars <= 0) {
    if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
             "laminate stays honest — unscored facets or polish laps only "
             "torque remembers; the minute tolerances people memorize without "
             "meaning to. ");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "surface stays rumor-smooth — insults mend fast enough that "
             "violence becomes hearsay; the minute geography people memorize "
             "without meaning to. ");
    else
      append(out, outcap, &n,
             "fine scarless planes or old cuts, the minute geography people "
             "memorize without meaning to. ");
  } else if (pc->scars == 1) {
    if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
             "one seam reopened by blunt luck — polish and sheen disagree "
             "about whether mercy was invited. ");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "one tear sealed before gossip settled — ripple and gloss disagree "
             "about what struck first. ");
    else
      append(out, outcap, &n,
             "a face mapped once by healed violence — line and gloss disagree "
             "about what happened first. ");
  } else {
    if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
             "relay tracks stacked across plating — old torque tells silver on "
             "silver, the minute tolerances people memorize without meaning "
             "to. ");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "turbulent planes where insults stacked without sticking — old "
             "ripples silver on silver, the minute geography people memorize "
             "without meaning to. ");
    else
      append(out, outcap, &n,
             "scar-knotted planes where old stories stacked silver on silver, "
             "the minute geography people memorize without meaning to. ");
  }
  append(out, outcap, &n,
         "At distance the silhouette still arrives first — color becomes rumor, "
         "shape becomes fact.\n\n");

  /* --- Paragraph 3: voice --- */
  append(out, outcap, &n,
         "Voice sits %s in register and %s in texture — %s that carries farther "
         "than your footsteps "
         "and lingers after quarrels end. %s: soft when you choose mercy, "
         "edged when you choose boundary, flat when you decide silence is "
         "the sharper blade.\n\n",
         S(pc->voice_pitch[0] ? pc->voice_pitch : NULL, "steady"),
         S(pc->voice_quality[0] ? pc->voice_quality : NULL, "clear"),
         voice_braid_phrase(pc),
         voice_space_opening(pc));

  /* --- Paragraph 4: carriage, hips/seat, marks, stats, extras --- */
  append(out, outcap, &n,
         "%s.\n%s.\n%s\n\n",
         hips_clause_for_pc(pc), butt_clause_for_pc(pc), statbuf);

  if (race_reads_construct_adjacent(pc))
  append(out, outcap, &n,
           "Strip the costumes away and the ledger reads plain: you are %s, "
           "and you are %s — coolant traces and stress bloom belong to labor; "
           "ink belongs to vows, whims, or brands someone else thought "
           "permanent.\n\n",
           scar_ledger_for_pc(pc, pc->scars), tattoo_ledger_for_pc(pc,
                                                                   pc->tattoos));
  else if (race_is_slime_like(pc))
    append(out, outcap, &n,
           "Strip the costumes away and the ledger reads plain: you are %s, "
           "and you are %s — drip-edge sheen and idle shimmer belong to labor; "
           "ink belongs to vows, whims, or brands someone else thought "
           "permanent.\n\n",
           scar_ledger_for_pc(pc, pc->scars), tattoo_ledger_for_pc(pc,
                                                                   pc->tattoos));
  else
    append(out, outcap, &n,
           "Strip the costumes away and the ledger reads plain: you are %s, "
           "and you are %s — vein-tracery and sweat-glisten belong to labor; "
           "ink belongs to vows, whims, or brands someone else thought "
           "permanent.\n\n",
           scar_ledger_for_pc(pc, pc->scars), tattoo_ledger_for_pc(pc,
                                                                   pc->tattoos));

  if (modbuf[0])
    append(out, outcap, &n, "%s\n\n", modbuf);

  if (pc->cor > 0) {
    if (race_reads_construct_adjacent(pc))
  append(out, outcap, &n,
             "Something not quite wholesome leaks through specification — not "
             "always visible as crack or sigil, more like thermal creep across "
             "housing, or an angle in your calibration map that arrives half a "
             "heartbeat before mercy should. Others feel it in pauses firmware "
             "cannot justify yet.\n\n");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Something not quite wholesome has begun to tint you — not always "
             "visible as wound or sigil, more like viscosity gone rude on the "
             "surface, or a ripple at the corners that arrives before mercy "
             "earns its say. Others feel it in pauses meniscus cannot steady "
             "yet.\n\n");
    else
      append(out, outcap, &n,
             "Something not quite wholesome has begun to stain you — not always "
             "visible as wound or sigil, more like temperature gone wrong on "
             "skin, or an angle in your smile that arrives half a heartbeat "
             "before mercy should. Others feel it in pauses they cannot justify "
             "yet.\n\n");
  }

  /* --- Paragraph 5: intimate anatomy (continuous prose) --- */
  if (ieq(pc->genitalia, "None")) {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "Beneath cloth your interface stays smooth and deliberate — no "
             "coupling advertises intent to every glance; thermal budget "
             "concentrates elsewhere, and pleasure, when it finds you, borrows "
             "clever hands and patience rather than obvious routing shouting "
             "its name.\n\n");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Beneath cloth your groin stays smooth and deliberate — no shaft "
             "or slit holds the eye for long; viscosity hoards interest "
             "elsewhere, and pleasure, when it finds you, borrows clever hands "
             "and patience rather than obvious plumbing shouting its name.\n\n");
    else
      append(out, outcap, &n,
             "Beneath cloth your groin stays smooth and deliberate — no shaft or "
             "slit advertises its hunger to every glance; warmth concentrates "
             "elsewhere, and pleasure, when it finds you, borrows clever hands "
             "and patience rather than obvious plumbing shouting its name.\n\n");
  } else {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "Under travel-stained linen your sex registers as %s — thermal "
             "mass banked close, weight held private until intent or daring "
             "hauls fabric aside and lets breath meet truth.\n\n",
             S(pc->genitalia, "your own secret"));
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Under travel-stained linen your sex settles as %s — warmth pools "
             "close behind veil, mass held private until intent or daring "
             "hauls fabric aside and lets breath meet truth.\n\n",
             S(pc->genitalia, "your own secret"));
    else
      append(out, outcap, &n,
             "Under travel-stained linen your sex settles as %s — heat banked "
             "close, weight held private until intent or daring hauls fabric "
             "aside and lets breath meet truth.\n\n",
             S(pc->genitalia, "your own secret"));
  }

  if (skip_breast_cc && label_is_none(breasts)) {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "Your chassis stays flat and martial — %s torque where softer "
             "curves might swell, panel line drawing clean under strain; linen "
             "lies flat unless wind or labor pulls it honest.\n\n",
             S(pc->muscle_tone, "trained"));
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Your chest stays flat and martial — %s muscle where softer curves "
             "might swell, surface drawing honest under strain; linen lies flat "
             "unless drift or labor pulls it honest.\n\n",
             S(pc->muscle_tone, "trained"));
    else
      append(out, outcap, &n,
             "Your chest stays flat and martial — %s muscle where softer curves "
             "might swell, pectoral line drawing clean under strain; linen lies "
             "flat unless wind or labor pulls it honest.\n\n",
           S(pc->muscle_tone, "trained"));
  } else if (!describe_breasts) {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "Your torso follows a narrow male line — shoulders and servos do "
             "most of the talking, plating visible only when breath runs deep; "
             "whatever curves other templates promised stay traded for leverage "
             "and reach.\n\n");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Your torso follows a narrow male line — shoulders and lats do "
             "most of the talking, ribs suggested only when breath runs deep; "
             "whatever curves other templates promised stay traded for leverage "
             "and reach.\n\n");
    else
      append(out, outcap, &n,
             "Your torso follows a narrow male line — shoulders and lats do "
             "most of the talking, ribs visible only when breath runs deep; "
             "whatever curves other templates promised stay traded for leverage "
             "and reach.\n\n");
  } else if (label_is_automatic(breasts)) {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "Breasts match your frame rather than argue with it — mass sized "
             "to height and habit, sway tuned to stride, neither trophy nor "
             "afterthought unless you bent the mirror until it lied for you. "
             "They shift thermal load, ache with cold, gather condensate where "
             "leather rides honest.\n\n");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Breasts match your frame rather than argue with it — mass sized "
             "to height and habit, sway tuned to stride, neither trophy nor "
             "afterthought unless you bent the mirror until it lied for you. "
             "They shift temperature, ache with chill, gather meniscus where "
             "leather rides honest.\n\n");
    else
      append(out, outcap, &n,
             "Breasts match your frame rather than argue with it — mass sized "
             "to height and habit, sway tuned to stride, neither trophy nor "
             "afterthought unless you bent the mirror until it lied for you. "
             "They shift temperature, ache with cold, gather sweat where "
             "leather rides honest.\n\n");
  } else if (label_is_none(breasts)) {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "Your chest stays deliberately minimal — flat ambition held under "
             "cloth, ports tuned to chill and friction more than display.\n\n");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "Your chest stays deliberately minimal — flat ambition held under "
             "cloth, peaks tuned to ripple and friction more than display.\n\n");
    else
      append(out, outcap, &n,
             "Your chest stays deliberately minimal — flat ambition held under "
             "cloth, nipples tuned to chill and friction more than display.\n\n");
  } else {
    append(out, outcap, &n,
           "Your breasts settle as %s", breasts);
    if (nip[0])
      append(out, outcap, &n, ", nipples %s", nip);
    if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
             " — compliant mass that shifts how linen lies, how harness bites "
             "or forgives, and how strangers negotiate where their eyes are "
             "allowed to rest.\n\n");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             " — yielding mass that shifts how linen lies, how harness bites "
             "or forgives, and how strangers negotiate where their eyes are "
             "allowed to rest.\n\n");
    else
      append(out, outcap, &n,
             " — soft mass that shifts how linen lies, how harness bites or "
             "forgives, and how strangers negotiate where their eyes are "
             "allowed to rest.\n\n");
  }

  if (pen) {
    if (label_is_none(cock)) {
      if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
               "Between your thighs hangs hardware the world refuses to "
               "mythologize yet — ordinary reach for your scale, hydraulic rise "
               "when demand insists, crown flushed when thought wanders where "
               "hands have not earned passage.\n\n");
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "Between your thighs hangs a cock the world refuses to "
               "mythologize yet — ordinary length and heft for your scale, "
               "flush rising when appetite insists, crown slick when thought "
               "wanders where hands have not earned passage.\n\n");
      else
        append(out, outcap, &n,
               "Between your thighs hangs a cock the world refuses to mythologize "
               "yet — ordinary length and heft for your scale, veins rising "
               "when blood insists, crown flushed when thought wanders where "
               "hands have not earned passage.\n\n");
    } else if (label_is_automatic(cock)) {
      if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
               "Your shaft tracks specification and bearing — thickness and "
               "reach that look mapped rather than staged behind perfume; "
               "arousal writes honesty along routing where thumb and mouth "
               "eventually argue jurisdiction.\n\n");
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "Your shaft matches strain and bearing — thickness and reach "
               "that look earned by appetite and habit rather than staged "
               "behind perfume; arousal writes honesty along the underside "
               "where thumb and mouth eventually argue jurisdiction.\n\n");
      else
        append(out, outcap, &n,
               "Your shaft matches height and bearing — thickness and reach "
               "that look earned by bone and habit rather than staged behind "
               "perfume; arousal writes honesty along the underside where thumb "
               "and mouth eventually argue jurisdiction.\n\n");
    } else {
      if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
               "Between your thighs hangs a %s cock — mass enough to crowd "
               "routing through underwear into betrayal, outline stubborn "
               "beneath cloth when humor or heat climbs your throat.\n\n",
               cock);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "Between your thighs hangs a %s cock — heft enough to crowd shear "
               "through linen into betrayal, outline stubborn beneath cloth when "
               "humor or heat climbs your throat.\n\n",
               cock);
      else
        append(out, outcap, &n,
               "Between your thighs hangs a %s cock — weight enough to crowd "
               "underwear into betrayal, outline stubborn beneath cloth when "
               "humor or heat climbs your throat.\n\n",
             cock);
    }

    if (label_is_none(balls_d)) {
      if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
               "Below your shaft the housing stays spare — no paired reservoir "
               "to sway with your stride; torque and thermal stack ride the "
               "shaft and root alone when want crowds thought aside.\n\n");
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "Below your shaft the gel stays spare — no paired ballast to sway "
               "with your stride; pressure and appetite ride the shaft and root "
               "alone when want crowds thought aside.\n\n");
      else
        append(out, outcap, &n,
               "Below your shaft the terrain stays spare — no paired weight to "
               "sway with your stride; heat and tension ride the shaft and root "
               "alone when want crowds thought aside.\n\n");
    } else if (label_is_automatic(balls_d)) {
      if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
               "Your sack carries compliance that fits the rest of you — swing "
               "and heft tuned to thigh and belly, tight enough to remind you "
               "when you sit wrong, quiet enough to forget until someone's "
               "mouth makes memory sharp.\n\n");
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "Your sack carries ballast that fits the rest of you — swing "
               "and heft tuned to thigh and belly, slack enough to remind you "
               "when you shift wrong, quiet enough to forget until someone's "
               "mouth makes memory sharp.\n\n");
      else
        append(out, outcap, &n,
               "Your sack carries weight that fits the rest of you — swing and "
               "heft tuned to thigh and belly, tight enough to remind you when "
               "you sit wrong, quiet enough to forget until someone's mouth "
               "makes memory sharp.\n\n");
    } else {
      if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
               "Your balls settle as %s — another torque datum that changes how "
               "harness straps bite, how coolant rinses feel merciless, how "
               "standing naked reads as boast or exhaustion depending on who "
               "watches.\n\n",
               balls_d);
      else if (race_is_slime_like(pc))
        append(out, outcap, &n,
               "Your balls settle as %s — another swell reading that changes "
               "how saddle leathers bite, how rinse baths peel heat wrong, how "
               "standing naked reads as boast or exhaustion depending on who "
               "watches.\n\n",
               balls_d);
      else
        append(out, outcap, &n,
               "Your balls settle as %s — another datum that changes how saddle "
               "leathers bite, how baths feel too hot, how standing naked "
               "reads as boast or exhaustion depending on who watches.\n\n",
             balls_d);
    }

    if (fr > 0 && pen && !label_is_none(cock)) {
      if (label_is_none(balls_d)) {
        if (fr == 1) {
          if (race_reads_construct_adjacent(pc))
        append(out, outcap, &n,
                   "A restless stack gathers behind your root — not the swing "
                   "of reservoirs but an inward charge, enough to blunt focus "
                   "when the room goes quiet.\n\n");
          else if (race_is_slime_like(pc))
        append(out, outcap, &n,
                   "A restless knot gathers behind your root — not the swing of "
                   "ballast but an inward swell, enough to blunt focus when the "
                   "room goes quiet.\n\n");
      else
        append(out, outcap, &n,
                   "A restless knot gathers behind your root — not the swing of "
                   "weight but an inward insistence, enough to blunt focus when "
                   "the room goes quiet.\n\n");
        } else if (fr == 2) {
          if (race_reads_construct_adjacent(pc))
            append(out, outcap, &n,
                   "Pressure stacks deep behind your shaft until standing feels "
                   "like negotiation — spine keeps torque tally even when you "
                   "pretend stillness.\n\n");
          else if (race_is_slime_like(pc))
            append(out, outcap, &n,
                   "Pressure stacks deep behind your shaft until standing feels "
                   "like negotiation — gel keeps tide tally even when you "
                   "pretend stillness.\n\n");
          else
            append(out, outcap, &n,
                   "Pressure stacks deep behind your shaft until standing feels "
                   "like negotiation — spine keeps tally even when you pretend "
                   "stillness.\n\n");
        } else if (race_reads_construct_adjacent(pc)) {
          append(out, outcap, &n,
                 "Urgency locks along your core until breath comes shallow — "
                 "fit to burst without reservoirs to share the blame, mercy "
                 "measured in inches and stubborn silence.\n\n");
        } else if (race_is_slime_like(pc)) {
          append(out, outcap, &n,
                 "Urgency locks along your core until breath comes shallow — "
                 "fit to burst without ballast to share the blame, mercy "
                 "measured in inches and stubborn silence.\n\n");
        } else {
          append(out, outcap, &n,
                 "Urgency locks along your core until breath comes shallow — "
                 "fit to burst without weight to share the blame, mercy "
                 "measured in inches and stubborn silence.\n\n");
        }
      } else if (fr == 1) {
        if (race_reads_construct_adjacent(pc))
          append(out, outcap, &n,
                 "A faint tightness gathers low — reservoir pressure simmering "
                 "but still manageable, enough to blunt calibration when silence "
                 "stretches too long.\n\n");
        else if (race_is_slime_like(pc))
          append(out, outcap, &n,
                 "A faint tightness gathers low — gel tide simmering but still "
                 "manageable, enough to thicken thought when silence stretches "
                 "too long.\n\n");
        else
          append(out, outcap, &n,
                 "A faint tightness gathers low — seed simmering but still "
                 "manageable, enough to thicken thought when silence stretches "
                 "too long.\n\n");
      } else if (fr == 2) {
        if (race_reads_construct_adjacent(pc))
          append(out, outcap, &n,
                 "Pressure builds to a distracting ache — spine remembers "
                 "torque even when duty insists on stillness; calibration "
                 "arrives late and blunt.\n\n");
        else if (race_is_slime_like(pc))
          append(out, outcap, &n,
                 "Pressure builds to a distracting ache — meniscus remembers "
                 "appetite even when duty insists on stillness; thought arrives "
                 "late and blunt.\n\n");
        else
          append(out, outcap, &n,
                 "Pressure builds to a distracting ache — spine remembers purpose "
                 "even when duty insists on stillness; thought arrives late and "
                 "blunt.\n\n");
      } else {
        if (race_reads_construct_adjacent(pc))
          append(out, outcap, &n,
                 "Urgency crowds your groin until standing still feels like "
                 "calibration — you are fit to burst, mercy measured in inches "
                 "and stubborn silence.\n\n");
        else if (race_is_slime_like(pc))
          append(out, outcap, &n,
                 "Urgency crowds your groin until standing still feels like "
                 "kindness — you are fit to burst, mercy measured in inches and "
                 "swollen tide.\n\n");
        else
          append(out, outcap, &n,
                 "Urgency crowds your groin until standing still feels like "
                 "kindness — you are fit to burst, mercy measured in inches and "
                 "breath.\n\n");
      }
    }
  }

  if (vag) {
    if (!label_is_none(pussy) && !ieq(pussy, "None")) {
      if (race_reads_construct_adjacent(pc))
      append(out, outcap, &n,
               "Between your thighs, %s — folds, flush, and slick yours alone; "
               "thermal bloom banks behind curls until welcome writes its own "
               "permission.\n\n",
             pussy);
      else if (race_is_slime_like(pc))
      append(out, outcap, &n,
               "Between your thighs, %s — folds, flush, and slick yours alone; "
               "warmth pools behind curls until welcome writes its own "
               "permission.\n\n",
               pussy);
      else
        append(out, outcap, &n,
               "Between your thighs, %s — folds, flush, and slick yours alone; "
               "warmth banks behind curls until welcome writes its own "
               "permission.\n\n",
               pussy);
    } else if (!(pen && vag)) {
      /* Herm with default pussy: skip — the dual-sex paragraph below covers
       * integration without repeating the generic stride/cunt line. */
      append_default_vag_anatomy(out, outcap, &n, pc);
    }
  }

  if (pen && vag) {
    if (race_reads_construct_adjacent(pc))
    append(out, outcap, &n,
             "You carry cock and cunt together — neither subroutine cancels the "
             "other; the pairing reads as deliberate integration, appetite "
             "doubled rather than confused, and every lover learns two "
             "vocabularies before either earns fluency.\n\n");
    else if (race_is_slime_like(pc))
      append(out, outcap, &n,
             "You carry cock and cunt together — neither tide cancels the other; "
             "the pairing reads as deliberate craft, appetite doubled rather "
             "than confused, and every lover learns two vocabularies before "
             "either earns fluency.\n\n");
    else
      append(out, outcap, &n,
             "You carry cock and cunt together — neither piece cancels the other; "
             "the pairing reads as deliberate craft, appetite doubled rather "
             "than confused, and every lover learns two vocabularies before "
             "either earns fluency.\n\n");
  }

  if (describe_breasts && !skip_breast_cc && lr > 0) {
    lr_used = 1;
    if (race_reads_construct_adjacent(pc)) {
    if (lr == 1)
      append(out, outcap, &n,
               "White coolant gathers with a restless, faint fullness — feed "
               "circuits stay awake even when no one watches; nipples load "
               "sensors at chill or unwelcome signal.\n\n");
    else if (lr == 2)
      append(out, outcap, &n,
               "Your feed lines strain with steady insistence — let-down lurks "
               "behind every rough cheer, every accidental squeeze; shirts "
               "become negotiations.\n\n");
    else
      append(out, outcap, &n,
               "Your reservoirs pack tight — one rough breath could loose more "
               "than calibration; ache spills toward throat and belly until "
               "kindness or cruelty drains you.\n\n");
    } else if (race_is_slime_like(pc)) {
      if (lr == 1)
        append(out, outcap, &n,
               "Milk gathers with a restless, faint fullness — tide remembers "
               "appetite even when no one watches; nipples bead at chill or "
               "unwelcome ripple.\n\n");
      else if (lr == 2)
        append(out, outcap, &n,
               "Your channels strain with steady insistence — let-down lurks "
               "behind every rough cheer, every accidental squeeze; shirts "
               "become negotiations.\n\n");
      else
        append(out, outcap, &n,
               "Your chest feels packed tight — one rough breath could loose "
               "more than pride; ache spills toward throat and belly until "
               "kindness or cruelty drains you.\n\n");
    } else {
      if (lr == 1)
        append(out, outcap, &n,
               "Milk gathers with a restless, faint fullness — glands awake "
               "even when no one watches; nipples tighten at chill or unwelcome "
               "thought.\n\n");
      else if (lr == 2)
        append(out, outcap, &n,
               "Your ducts strain with steady insistence — let-down lurks behind "
               "every rough cheer, every accidental squeeze; shirts become "
               "negotiations.\n\n");
      else
        append(out, outcap, &n,
               "Your chest feels packed tight — one rough breath could loose "
               "more than pride; ache spills toward throat and belly until "
               "kindness or cruelty drains you.\n\n");
    }
  } else if (!lr_used && lr > 0 && vag) {
    lr_used = 1;
    if (race_reads_construct_adjacent(pc)) {
    if (lr == 1)
        append(out, outcap, &n,
               "A trace fullness lingers in your chest even when your breasts "
               "stay modest — feed circuits remember duty whether curves "
               "advertise or not.\n\n");
    else if (lr == 2)
        append(out, outcap, &n,
               "Lactation demands attention without loud curves on display — "
               "condensate rings become their own confession.\n\n");
    else
      append(out, outcap, &n,
               "Your body wants to weep milk no matter how you bind or hide "
               "it — routing and reservoir insist where silhouette stays shy.\n\n");
    } else if (race_is_slime_like(pc)) {
      if (lr == 1)
        append(out, outcap, &n,
               "A trace fullness lingers in your chest even when your breasts "
               "stay modest — glands remember duty whether curves advertise or "
               "not.\n\n");
      else if (lr == 2)
        append(out, outcap, &n,
               "Milk demands attention without loud curves on display — wet "
               "sheen becomes its own confession.\n\n");
      else
        append(out, outcap, &n,
               "Your body wants to weep milk no matter how you bind or hide "
               "it — channel and meniscus insist where silhouette stays shy.\n\n");
    } else {
      if (lr == 1)
        append(out, outcap, &n,
               "A trace fullness lingers in your chest even when your breasts "
               "stay modest — glands remember duty whether curves advertise or "
               "not.\n\n");
      else if (lr == 2)
        append(out, outcap, &n,
               "Milk demands attention without loud curves on display — wet spots "
               "become their own confession.\n\n");
      else
        append(out, outcap, &n,
               "Your body wants to weep milk no matter how you bind or hide "
               "it — duct and gland insist where silhouette stays shy.\n\n");
    }
  } else if (!lr_used && lr > 0 && pen && skip_breast_cc) {
    lr_used = 1;
    if (race_reads_construct_adjacent(pc)) {
      if (lr == 1)
  append(out, outcap, &n,
               "Lactation gathers anyway — feed circuits rude enough to wake "
               "behind whatever story your chest tells the room; nipples betray "
               "chill and input alike.\n\n");
      else if (lr == 2)
  append(out, outcap, &n,
               "Your feed lines insist despite the harness logic — condensate "
               "streaks become their own rumor across plate or modesty "
               "alike.\n\n");
      else
        append(out, outcap, &n,
               "Your chest packs tight with let-down waiting — pride and cloth "
               "both lie; relief or shame arrives on schedule whether anyone "
               "asked.\n\n");
    } else if (race_is_slime_like(pc)) {
      if (lr == 1)
        append(out, outcap, &n,
               "Milk gathers anyway — tide rude enough to wake behind whatever "
               "story your chest tells the room; peaks betray chill and "
               "thought alike.\n\n");
      else if (lr == 2)
        append(out, outcap, &n,
               "Your channels insist despite the harness logic — wet linen "
               "becomes its own rumor across muscle or modesty alike.\n\n");
      else
        append(out, outcap, &n,
               "Your chest packs tight with let-down waiting — pride and cloth "
               "both lie; relief or shame arrives on schedule whether anyone "
               "asked.\n\n");
    } else {
      if (lr == 1)
        append(out, outcap, &n,
               "Milk gathers anyway — glands rude enough to wake behind whatever "
               "story your chest tells the room; nipples betray chill and "
               "thought alike.\n\n");
      else if (lr == 2)
        append(out, outcap, &n,
               "Your ducts insist despite the harness logic — wet linen becomes "
               "its own rumor across muscle or modesty alike.\n\n");
      else
        append(out, outcap, &n,
               "Your chest packs tight with let-down waiting — pride and cloth "
               "both lie; relief or shame arrives on schedule whether anyone "
               "asked.\n\n");
    }
  }

  append_gear_closer(out, outcap, &n, pc);

  if (n >= outcap - 1)
    out[outcap - 1] = '\0';
}
