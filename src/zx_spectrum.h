/* zx_spectrum.h - Common Definitions */
#ifndef ZX_SPECTRUM_H
#define ZX_SPECTRUM_H

#include <stdint.h>

#define MEM_SIZE 65536
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

extern uint8_t memory[MEM_SIZE];
void load_rom(const char *filename);
void cpu_init();
void cpu_step();
void display_update();
void input_handle();
#endif /* ZX_SPECTRUM_H */