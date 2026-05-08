#include "aeternitas_mod_guide.h"

const char *aet_mod_guide_full_text(void) {
  return "Aeternitas Modding Guide\n\n"
         "1) Create mod packs under mods/<pack_name>/\n"
         "2) Add optional manifest.txt with priority and enabled flags\n"
         "3) Override room text via rooms/<slug>.txt or append/prepend variants\n"
         "4) Override item examine text via items/<item_id>.txt\n"
         "5) Reload in game with: mods reload\n\n"
         "This recovered guide is minimal but functional.";
}
