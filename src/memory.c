/* memory.c */
#include "zx_spectrum.h"
#include <stdio.h>
#include <string.h>

uint8_t memory[MEM_SIZE];

void load_rom(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (f) {
    fread(memory, 1, 16384, f); /* Load first 16KB into ROM space */
    fclose(f);
  }
}
