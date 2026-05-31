#include "mgt.h"

#include <string.h>

int mg_run_test_cube(MgtSession *session);

static const MgtMinigame g_registry[] = {
    {"test_cube", "Test Cube", "mg_run_test_cube",
     "Bench contact verifier — start here", mg_run_test_cube},
};

const MgtMinigame *mgt_registry(void) { return g_registry; }

int mgt_registry_count(void) {
  return (int)(sizeof g_registry / sizeof g_registry[0]);
}

const MgtMinigame *mgt_registry_find(const char *id) {
  int i;
  if (!id) return NULL;
  for (i = 0; i < mgt_registry_count(); i++)
    if (g_registry[i].id && strcmp(g_registry[i].id, id) == 0)
      return &g_registry[i];
  return NULL;
}
