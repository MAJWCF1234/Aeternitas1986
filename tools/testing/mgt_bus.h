#ifndef MGT_BUS_H
#define MGT_BUS_H

#include "mgt_game_sim.h"
#include "mgt_platform.h"
#include "mgt_state.h"

typedef enum MgtBusPin {
  MGT_PIN_PWR = 0,
  MGT_PIN_COIN,
  MGT_PIN_PACK,
  MGT_PIN_ROOM,
  MGT_PIN_SKILL,
  MGT_PIN_WEATH,
  MGT_PIN_TURN,
  MGT_PIN_MGST,
  MGT_PIN_COUNT
} MgtBusPin;

#define MGT_PIN_BIT(p) (1u << (unsigned)(p))

typedef struct MgtModuleSpec {
  const char *id;
  const char *cube_label;
  unsigned required_pins;
  const char *const *room_any;
  const char *const *need_any_item;
} MgtModuleSpec;

typedef struct MgtBusProbe {
  int pin_mated[MGT_PIN_COUNT];
  int room_ok;
  int items_ok;
  int ready;
  char summary[96];
  char fix_hint[96];
} MgtBusProbe;

const MgtModuleSpec *mgt_bus_module_spec(const char *id);
int mgt_bus_module_count(void);
const MgtModuleSpec *mgt_bus_module_at(int index);

void mgt_bus_probe(const MgtGameSim *sim, const MgtPersistentState *st,
                   const MgtModuleSpec *mod, MgtBusProbe *out);

int mgt_bus_scenario_count(void);
const char *mgt_bus_scenario_name(int index);
const char *mgt_bus_scenario_desc(int index);
void mgt_bus_apply_scenario(MgtGameSim *sim, int index);

int mgt_bus_module_index(const char *id);
void mgt_bus_auto_wire(MgtGameSim *sim, const MgtModuleSpec *mod);

typedef struct MgtBenchVisual {
  int seat_phase;
  int show_ejected;
  const char *ejected_label;
  int linked_selected;
} MgtBenchVisual;

void mgt_bus_draw_bench(MgtCanvas *c, const MgtGameSim *sim,
                        const MgtPersistentState *st,
                        const MgtModuleSpec *mod, int plugged,
                        const MgtBusProbe *probe, const MgtBenchVisual *vis);

#endif
