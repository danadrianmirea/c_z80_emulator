#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// Register State
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
    uint8_t a_, f_, b_, c_, d_, e_, h_, l_;

    // Control flags
    uint8_t iff1, iff2;
    uint8_t imode;
} Z80_State;

// Flag bit positions
#define FLAG_C  0x01
#define FLAG_N  0x02
#define FLAG_PV 0x04
#define FLAG_H  0x10
#define FLAG_Z  0x40
#define FLAG_S  0x80

// Flag manipulation macros
#define SET_FLAG(state, flag)  ((state)->f |= (flag))
#define CLR_FLAG(state, flag)  ((state)->f &= ~(flag))
#define TST_FLAG(state, flag)  ((state)->f & (flag))

// Helper macros
#define UPDATE_SZ(state, val) do { \
    CLR_FLAG(state, FLAG_Z | FLAG_S); \
    if(!(val)) SET_FLAG(state, FLAG_Z); \
    if((val) & 0x80) SET_FLAG(state, FLAG_S); \
} while(0)

#define UPDATE_SZP(state, val) do { \
    UPDATE_SZ(state, val); \
    CLR_FLAG(state, FLAG_PV); \
    if(parity_table[(val)]) SET_FLAG(state, FLAG_PV); \
} while(0)

// Memory interface
uint8_t mem_read(uint32_t addr);
void mem_write(uint32_t addr, uint8_t val);

// Core functions
void z80_init(Z80_State* state);
int decode_cb(Z80_State* state);
int z80_step(Z80_State* state);


// Stack operations
void push16(Z80_State* state, uint16_t val);
uint16_t pop16(Z80_State* state);

#endif