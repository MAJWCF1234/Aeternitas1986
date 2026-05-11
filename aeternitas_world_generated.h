#ifndef AETERNITAS_WORLD_GENERATED_H
#define AETERNITAS_WORLD_GENERATED_H

enum { DIR_COUNT = 19 };
#define WORLD_ROOM_COUNT 161

typedef struct AetNpcTopic {
  const char *keywords;
  const char *response;
} AetNpcTopic;

typedef struct AetNpcLineSet {
  const char *slug;
  const char *greeting;
  const char *const *chatter;
  const AetNpcTopic *topics;
} AetNpcLineSet;

typedef struct AetMerchantOffer {
  const char *item;
  int price;
} AetMerchantOffer;

typedef struct AetMerchantTable {
  const char *slug;
  const AetMerchantOffer *stock;
  const AetMerchantOffer *buys;
} AetMerchantTable;

const char *const *world_consume_food_ids(void);
const char *const *world_consume_drink_ids(void);
const char *const *world_quest_hints(void);

const char *world_slug(int room);
const char *world_title(int room);
const char *world_blurb(int room);
const char *world_region(int room);
int world_room_is_dark(int room);
const char *world_room_entity(int room);
int world_exit(int room, int dir);
const char *const *world_item_list(int room);
const char *const *world_hidden_item_list(int room);
int world_room_index(const char *slug);

const AetNpcLineSet *aet_npc_lines(const char *entity_slug);
int aet_npc_line_count(void);
const char *aet_npc_line_slug_at(int idx);
int aet_merchant_count(void);
int aet_merchant_index(const char *slug);
const char *aet_merchant_slug_at(int idx);
const AetMerchantTable *aet_merchant_trades(const char *slug);

#endif
