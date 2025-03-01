/* zx_spectrum.h - Common Definitions */
#ifndef ZX_SPECTRUM_H
#define ZX_SPECTRUM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"

#define SPECTRUM_ROM_SIZE 16384  // 16KB
#define ROM_START 0x0000U
#define ROM_END 0x3FFFU        // 16KB ROM area
#define MEM_SIZE 65536
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192

// Flag update macros
#define UPDATE_FLAGS_ADD(result, operand) \
    CLR_FLAG(FLAG_Z | FLAG_S | FLAG_H | FLAG_C); \
    if((result) == 0) SET_FLAG(FLAG_Z); \
    if((result) & 0x80) SET_FLAG(FLAG_S); \
    if(((operand) & 0x0F) + ((a) & 0x0F) > 0x0F) SET_FLAG(FLAG_H); \
    if((uint16_t)(operand) + (uint16_t)(a) > 0xFF) SET_FLAG(FLAG_C);

#define UPDATE_FLAGS_SUB(result, operand) \
    CLR_FLAG(FLAG_Z | FLAG_S | FLAG_H | FLAG_C); \
    if((result) == 0) SET_FLAG(FLAG_Z); \
    if((result) & 0x80) SET_FLAG(FLAG_S); \
    if(((operand) & 0x0F) > (a & 0x0F)) SET_FLAG(FLAG_H); \
    if((operand) > a) SET_FLAG(FLAG_C);

bool load_rom(const char* filename);
bool load_z80_snapshot(const char* filename);
void display_update();
void input_handle();
#endif /* ZX_SPECTRUM_H */