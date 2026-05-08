#include "aeternitas_item_catalog.h"

#include <ctype.h>
#include <string.h>

/* clang-format off — compact item catalog (slug, display label, equip hints). */
static const AetItemCatalogEntry CATALOG[] = {
    {1,  "leather_cowl",       "Leather Cowl",                  2, 0, 1, 0},
    {2,  "rusted_iron_mail",   "Rusted Iron Mail",              6, 0, 8, 1},
    {3,  "woven_grass_shirt",  "Woven Grass Shirt",             1, 0, 1, 1},
    {4,  "steel_shortsword",   "Steel Shortsword",              0, 6, 3, 5},
    {5,  "wooden_buckler",     "Wooden Buckler",                3, 0, 2, 6},
    {6,  "bone_ring",          "Bone Ring",                     1, 1, 0, 7},
    {7,  "heavy_scrap_axe",    "Heavy Scrap Axe",               0, 8, 6, 5},
    {8,  "leather_boots",      "Leather Boots",                 2, 0, 1, 4},
    {9,  "torn_trousers",      "Torn Trousers",                 1, 0, 1, 3},
    {10, "terrible_long_item_name", "A Terribly Long Item Name That Breaks GUIs",
     0, 0, 1, 7},
};
/* clang-format on */

static int slug_ieq(const char *a, const char *b) {
  unsigned char ca, cb;
  if (!a || !b) return 0;
  while (*a && *b) {
    ca = (unsigned char)*a++;
    cb = (unsigned char)*b++;
    if (tolower(ca) != tolower(cb)) return 0;
  }
  return *a == '\0' && *b == '\0';
}

const AetItemCatalogEntry *aet_item_catalog_by_slug(const char *slug) {
  size_t i;
  if (!slug || !slug[0]) return NULL;
  for (i = 0; i < sizeof CATALOG / sizeof CATALOG[0]; i++) {
    if (slug_ieq(slug, CATALOG[i].slug)) return &CATALOG[i];
  }
  return NULL;
}

const AetItemCatalogEntry *aet_item_catalog_by_id(int id) {
  size_t i;
  if (id < 1) return NULL;
  for (i = 0; i < sizeof CATALOG / sizeof CATALOG[0]; i++) {
    if (CATALOG[i].id == id) return &CATALOG[i];
  }
  return NULL;
}

int aet_item_catalog_count(void) {
  return (int)(sizeof CATALOG / sizeof CATALOG[0]);
}

const char *aet_item_catalog_label_for_slug(const char *slug) {
  const AetItemCatalogEntry *e = aet_item_catalog_by_slug(slug);
  return (e && e->label && e->label[0]) ? e->label : NULL;
}
