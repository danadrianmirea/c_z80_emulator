/* memory.c */
#include "zx_spectrum.h"

uint8_t memory[MEM_SIZE];

bool load_rom(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f) {
    fread(memory, 1, 16384, f); /* Load first 16KB into ROM space */
    fclose(f);
    return true;
  }
  return false;
}
