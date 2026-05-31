#include "mgt_host.h"

#include <stdio.h>
#include <time.h>

int main(void) {
  unsigned seed = (unsigned)time(NULL);
  mgt_host_init(seed);
  mgt_host_run_harness();
  mgt_host_shutdown();
  printf("Module bench closed. Bus sim + profile saved to harness_save.mgt\n");
  return 0;
}
