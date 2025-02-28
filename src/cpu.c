
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