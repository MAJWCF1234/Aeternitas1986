#ifndef AETERNITAS_CHAR_DESCRIPTION_H
#define AETERNITAS_CHAR_DESCRIPTION_H

#include "aeternitas_char_creation.h"
#include <stddef.h>

#define AETER_CHARACTER_PORTRAIT_CAP 8192

void aet_describe_pc(const AetPcSave *pc, char *out, size_t outcap);

#endif
