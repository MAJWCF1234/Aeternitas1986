#include "mgt.h"
#include "mgt_host.h"
#include "mgt_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const k_ids[] = {
    "lockpick", "piano", "fishing", "farming", "cooking",
    "writing", "reading", "gambling", "hunting",
};

static int fail(const char *m) {
  fprintf(stderr, "FAIL: %s\n", m);
  return 1;
}

static int test_smoke(void) {
  int i, n = mgt_registry_count();
  if (n != 9) return fail("registry count");
  for (i = 0; i < n; i++) {
    const MgtMinigame *mg = mgt_registry_find(k_ids[i]);
    if (!mg || !mg->run || strcmp(mg->id, k_ids[i]) || !mg->title[0])
      return fail("registry entry");
  }
  if (mgt_registry_find("not_a_game")) return fail("spurious find");
  printf("PASS: %d minigames registered\n", n);
  return 0;
}

static int test_embed(void) {
  MgtPersistentState in, out;
  char line[256], *path = "embed_test_tmp.sav";
  FILE *fp;

  mgt_profile_fresh_adventure(&in, 42u);
  in.fishing_level = 7;
  in.fishing_xp = 1200;
  in.hunt_cooldown_until_turn = 55;
  in.farm_seeds[0] = 3;
  in.lock_exit_noise = 72;
  in.lock_exit_misses = 2;
  snprintf(in.lock_noise_band, sizeof in.lock_noise_band, "HIGH");
  snprintf(in.last_banner, sizeof in.last_banner, "roundtrip probe");
  fp = fopen(path, "w");
  if (!fp || fprintf(fp, "AET64SAVE1\n") < 0 || !mgt_profile_write_embedded(fp, &in))
    return fail("write");
  fclose(fp);
  fp = fopen(path, "r");
  if (!fp || !fgets(line, sizeof line, fp) || strncmp(line, "AET64SAVE1", 10) ||
      !fgets(line, sizeof line, fp) || strncmp(line, "MGT", 3) ||
      !mgt_profile_read_embedded(fp, &out))
    return fail("read");
  fclose(fp);
  remove(path);
  if (out.fishing_level != 7 || out.fishing_xp != 1200 ||
      out.hunt_cooldown_until_turn != 55 || out.farm_seeds[0] != 3 ||
      out.lock_exit_noise != 72 || out.lock_exit_misses != 2 ||
      strcmp(out.lock_noise_band, "HIGH") || strcmp(out.last_banner, "roundtrip probe"))
    return fail("roundtrip");
  printf("PASS: MGT quicksave embed roundtrip\n");
  return 0;
}

static int run_scripted_minigame(const char *id, const char *script) {
  MgtSession *s;
  MgtPersistentState *st;
  char envbuf[64];
#if defined(_WIN32)
  _putenv("MGT_AUTOTEST=1");
  snprintf(envbuf, sizeof envbuf, "MGT_AUTOTEST_SCRIPT=%s", script);
  _putenv(envbuf);
#else
  setenv("MGT_AUTOTEST", "1", 1);
  setenv("MGT_AUTOTEST_SCRIPT", script, 1);
#endif
  mgt_host_init(42u);
  s = mgt_host_session();
  if (!s) return fail("session");
  if (mgt_host_run_minigame(s, id) == (int)MGT_HOST_ABORT) {
    mgt_host_shutdown();
    return fail("script launch");
  }
  st = mgt_host_state();
  if (!st) {
    mgt_host_shutdown();
    return fail("state");
  }
  if (!strcmp(script, "hunt_win")) {
    if (st->last_success != 1) {
      mgt_host_shutdown();
      return fail("hunt win outcome");
    }
  } else if (!strcmp(script, "lock_noise")) {
    if (st->lock_exit_noise < 70 || st->lock_exit_misses < 2) {
      mgt_host_shutdown();
      return fail("lock noise outcome");
    }
  } else if (!strcmp(script, "lock_win")) {
    if (st->last_success != 1 || st->lock_exit_noise > 30) {
      mgt_host_shutdown();
      return fail("lock win outcome");
    }
  }
  mgt_host_shutdown();
#if defined(_WIN32)
  _putenv("MGT_AUTOTEST_SCRIPT=");
#else
  unsetenv("MGT_AUTOTEST_SCRIPT");
#endif
  return 0;
}

static int test_outcomes(void) {
  if (run_scripted_minigame("hunting", "hunt_win")) return 1;
  printf("PASS: hunting scripted win\n");
  if (run_scripted_minigame("lockpick", "lock_noise")) return 1;
  printf("PASS: lockpick noise band\n");
  if (run_scripted_minigame("lockpick", "lock_win")) return 1;
  printf("PASS: lockpick quiet win\n");
  return 0;
}

static int test_launch(void) {
  MgtSession *s;
  int i, n, bad = 0;
#if defined(_WIN32)
  _putenv("MGT_AUTOTEST=1");
  _putenv("MGT_AUTOTEST_SCRIPT=");
#else
  setenv("MGT_AUTOTEST", "1", 1);
  unsetenv("MGT_AUTOTEST_SCRIPT");
#endif
  mgt_host_init(42u);
  s = mgt_host_session();
  if (!s) return fail("session");
  n = mgt_registry_count();
  for (i = 0; i < n; i++) {
    const MgtMinigame *mg = &mgt_registry()[i];
    if (mgt_host_run_minigame(s, mg->id) == (int)MGT_HOST_ABORT) {
      fprintf(stderr, "FAIL launch: %s\n", mg->id);
      bad++;
    } else
      printf("OK  launch: %s\n", mg->id);
  }
  mgt_host_shutdown();
  if (bad) return fail("launch");
  printf("PASS: %d minigame launch\n", n);
  return 0;
}

int main(int argc, char **argv) {
  const char *m = (argc > 1 && argv[1][0]) ? argv[1] : "native";
  if (!strcmp(m, "smoke")) return test_smoke() ? 1 : 0;
  if (!strcmp(m, "embed")) return test_embed() ? 1 : 0;
  if (!strcmp(m, "outcomes")) return test_outcomes() ? 1 : 0;
  if (!strcmp(m, "launch")) return test_launch() ? 1 : 0;
  if (!strcmp(m, "native") || !strcmp(m, "all")) {
    if (test_smoke() || test_embed() || test_outcomes() || test_launch()) return 1;
    return 0;
  }
  fprintf(stderr, "Usage: tester.exe [smoke|embed|outcomes|launch|native|all]\n");
  return 1;
}
