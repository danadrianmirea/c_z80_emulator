#pragma once

#include <stdint.h>

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

// Zilog Z80 Register State
typedef struct {
    // Main registers
    union { struct { uint8_t f, a; }; uint16_t af; };
    union { struct { uint8_t c, b; }; uint16_t bc; };
    union { struct { uint8_t e, d; }; uint16_t de; };
    union { struct { uint8_t l, h; }; uint16_t hl; };

    // Special registers
    uint16_t sp, pc;
    uint16_t ix, iy;

    // Alternate register set
    union { struct { uint8_t f_, a_; }; uint16_t af_; };
    union { struct { uint8_t c_, b_; }; uint16_t bc_; };
    union { struct { uint8_t e_, d_; }; uint16_t de_; };
    union { struct { uint8_t l_, h_; }; uint16_t hl_; };

    // R register
    uint8_t r;

   // I register (Interrupt Vector Register)
   uint8_t i;    

    // Control flags
    uint8_t iff1, iff2;
    uint8_t imode;
} Z80_State;