#ifndef AETERNITAS_ITEM_CATALOG_H
#define AETERNITAS_ITEM_CATALOG_H

/* Recovered from Aeternitas Web Edition equipment prototype (ITEM_DB). */

typedef struct AetItemCatalogEntry {
  int id;
  const char *slug;
  /** ITEM_DB ``name`` — shown in equipment terminal (web prototype). */
  const char *label;
  int def;
  int atk;
  int wgt;
  /** 0=head .. 7=accessory; matches EQ_* in aeternitas64_ascii.c */
  int slot_index;
} AetItemCatalogEntry;

const AetItemCatalogEntry *aet_item_catalog_by_slug(const char *slug);
const AetItemCatalogEntry *aet_item_catalog_by_id(int id);
/** Visible row title for slug, or NULL if not in catalog. */
const char *aet_item_catalog_label_for_slug(const char *slug);
int aet_item_catalog_count(void);

#endif /* AETERNITAS_ITEM_CATALOG_H */
