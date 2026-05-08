#ifndef AETER_MOD_BOOTSTRAP_H
#define AETER_MOD_BOOTSTRAP_H

#include <stddef.h>

/**
 * If <mods_root>/000_aeternitas_sample is missing, create the mods folder tree
 * and install a tutorial pack (safe to delete; recreate by removing the folder).
 */
void aet_mod_bootstrap_sample_pack(const char *mods_root);

typedef struct {
  int save_parent_ready;
  int mods_root_ready;
  int sample_pack_present;
  int marker_present;
  int repaired_files;
} AetRuntimeBootstrapStatus;

/**
 * Runtime hardening for floppy/single-exe deployments:
 * - creates parent folders for the save file path
 * - creates mods root when missing
 * - installs/repairs the tutorial pack when files are missing
 * - writes a bootstrap marker under mods root
 *
 * This is idempotent and never overwrites existing sample-pack files.
 */
void aet_mod_bootstrap_prepare_runtime(const char *save_file_path,
                                       const char *mods_root,
                                       AetRuntimeBootstrapStatus *out_status);

#endif
