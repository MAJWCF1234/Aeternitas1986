#ifndef AETER_MOD_BOOTSTRAP_H
#define AETER_MOD_BOOTSTRAP_H

#include <stddef.h>

void aet_mod_bootstrap_sample_pack(const char *mods_root);

typedef struct {
  int save_parent_ready;
  int mods_root_ready;
  int sample_pack_present;
  int marker_present;
  int repaired_files;
} AetRuntimeBootstrapStatus;

void aet_mod_bootstrap_prepare_runtime(const char *save_file_path,
                                       const char *mods_root,
                                       AetRuntimeBootstrapStatus *out_status);

#endif
