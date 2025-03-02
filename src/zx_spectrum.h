/* zx_spectrum.h - Common Definitions */
#ifndef ZX_SPECTRUM_H
#define ZX_SPECTRUM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"

#define MEM_SIZE 65536
#define ROM_START 0x0000
#define ROM_END 0x3FFF   // 16KB ROM area
#define RAM_START 0x4000
#define RAM_END 0xFFFF
#define ROM_SIZE 0x4000  // 16KB
#define RAM_SIZE 0xC000  // 48KB
#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 192
#define SCALE_FACTOR 2

enum RETURN_CODES
{
    RETCODE_NO_ERROR = 0,
    RETCODE_INVALID_ARGUMENTS,
    RETCODE_ROM_LOADING_FAILED,
    RETCODE_Z80_SNAPSHOT_LOADING_FAILED
};

enum Z80_VERSION
{
    Z80_VERSION_1 = 1,
    Z80_VERSION_2,
    Z80_VERSION_3
};

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