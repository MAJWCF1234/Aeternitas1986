#ifndef AETERNITAS_ITEM_CATALOG_H
#define AETERNITAS_ITEM_CATALOG_H

typedef struct AetItemCatalogEntry {
  int id;
  const char *slug;
  
  const char *label;
  
  const char *description;
  int def;
  int atk;
  int wgt;
  
  int slot_index;
} AetItemCatalogEntry;

const AetItemCatalogEntry *aet_item_catalog_by_slug(const char *slug);
const AetItemCatalogEntry *aet_item_catalog_by_id(int id);

const char *aet_item_catalog_label_for_slug(const char *slug);

const char *aet_item_catalog_description_for_slug(const char *slug);
int aet_item_catalog_count(void);

#endif 
