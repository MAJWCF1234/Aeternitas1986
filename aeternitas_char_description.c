#include "aeternitas_char_description.h"

#include <stdio.h>

void aet_describe_pc(const AetPcSave *pc, char *out, size_t outcap) {
  if (!out || outcap < 2) return;
  if (!pc) {
    snprintf(out, outcap, "No character profile loaded.");
    return;
  }
  snprintf(out, outcap,
           "%s, a %s %s.\n"
           "Build: %s, Eyes: %s, Hair: %s %s.\n"
           "Voice: %s / %s. Marks: scars=%d tattoos=%d.\n"
           "Loadout preference: armor=%s, weapon=%s.",
           pc->name[0] ? pc->name : "Wanderer",
           pc->race[0] ? pc->race : "Human",
           pc->class_[0] ? pc->class_ : "adventurer",
           pc->build[0] ? pc->build : "average",
           pc->eye_color[0] ? pc->eye_color : "unknown",
           pc->hair_color[0] ? pc->hair_color : "unknown",
           pc->hair_style[0] ? pc->hair_style : "unstyled",
           pc->voice_pitch[0] ? pc->voice_pitch : "mid",
           pc->voice_quality[0] ? pc->voice_quality : "clear", pc->scars,
           pc->tattoos, pc->armor[0] ? pc->armor : "none",
           pc->weapon[0] ? pc->weapon : "none");
}
