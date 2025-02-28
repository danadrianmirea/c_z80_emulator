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

/* cpu.c */
#include "zx_spectrum.h"

uint16_t pc;                    /* Program Counter */
uint8_t a, f, b, c, d, e, h, l; /* Registers */

void cpu_init() { pc = 0x0000; /* Start at ROM */ }

void cpu_step() {
  uint8_t opcode = memory[pc++];
  switch (opcode) {
  case 0x00: /* NOP */
    break;
  default:
    break;
  }
}
