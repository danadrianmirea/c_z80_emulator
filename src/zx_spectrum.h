/* zx_spectrum.h - Common Definitions */
#ifndef ZX_SPECTRUM_H
#define ZX_SPECTRUM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define SPECTRUM_ROM_SIZE 16384  // 16KB
#define ROM_START 0x0000

#define MEM_SIZE 65536
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

extern uint8_t memory[MEM_SIZE];
bool load_rom(const char* filename);
void cpu_init();
void cpu_step();
void display_update();
void input_handle();
#endif /* ZX_SPECTRUM_H */