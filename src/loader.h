#pragma once

#include <stdbool.h>
#include "zx_spectrum.h"

bool load_rom(const char* filename);
bool load_z80_snapshot(const char* filename, Z80_State* state);
bool load_sna(const char* filename, Z80_State* state);