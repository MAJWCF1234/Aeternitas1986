#include "mgt.h"

#include <string.h>

int mg_run_lockpick(MgtSession *session);
int mg_run_piano(MgtSession *session);
int mg_run_fishing(MgtSession *session);
int mg_run_farming(MgtSession *session);
int mg_run_cooking(MgtSession *session);
int mg_run_writing(MgtSession *session);
int mg_run_reading(MgtSession *session);
int mg_run_gambling(MgtSession *session);
int mg_run_hunting(MgtSession *session);

static const MgtMinigame g_registry[] = {
    {"lockpick", "Lockpicking", "startLockpickingMinigame",
     "Pins + skill rescue + focus", mg_run_lockpick},
    {"piano", "Piano Performance", "startPianoMinigame",
     "96x30 rhythm lanes + GM MIDI", mg_run_piano},
    {"fishing", "Fishing", "startFishingMinigame", "Terminal Fisher",
     mg_run_fishing},
    {"farming", "Farming", "startFarmingMinigame", "Terminal Farm",
     mg_run_farming},
    {"cooking", "Cooking", "startCookingMinigame", "Terminal Diner",
     mg_run_cooking},
    {"writing", "Writing", "startWritingMinigame", "Writer's Desk",
     mg_run_writing},
    {"reading", "Reading", "startReadingMinigame", "Literature viewer (ESC only)",
     mg_run_reading},
    {"gambling", "Gambling", "startGamblingMinigame", "Dice + Blackjack",
     mg_run_gambling},
    {"hunting", "Hunting", "startHuntingMinigame",
     "Track read + stalk + timed shot", mg_run_hunting},
};

const MgtMinigame *mgt_registry(void) { return g_registry; }

int mgt_registry_count(void) {
  return (int)(sizeof g_registry / sizeof g_registry[0]);
}

const MgtMinigame *mgt_registry_find(const char *id) {
  int i;
  if (!id) return NULL;
  for (i = 0; i < mgt_registry_count(); i++)
    if (strcmp(g_registry[i].id, id) == 0) return &g_registry[i];
  return NULL;
}
