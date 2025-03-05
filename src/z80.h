#ifndef Z80_H
#define Z80_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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
#define UPDATE_FLAGS_ADD(state, result, operand) \
    CLR_FLAG(state, FLAG_Z | FLAG_S | FLAG_H | FLAG_C); \
    if((result) == 0) SET_FLAG(state, FLAG_Z); \
    if((result) & 0x80) SET_FLAG(state, FLAG_S); \
    if(((operand) & 0x0F) + ((result) & 0x0F) > 0x0F) SET_FLAG(state, FLAG_H); \
    if((uint16_t)(operand) + (uint16_t)(result) > 0xFF) SET_FLAG(state, FLAG_C);

#define UPDATE_FLAGS_SUB(state, result, operand) \
    CLR_FLAG(state, FLAG_Z | FLAG_S | FLAG_H | FLAG_C); \
    if((result) == 0) SET_FLAG(state, FLAG_Z); \
    if((result) & 0x80) SET_FLAG(state, FLAG_S); \
    if(((operand) & 0x0F) > (state->a & 0x0F)) SET_FLAG(state, FLAG_H); \
    if((operand) > state->a) SET_FLAG(state, FLAG_C);

#define UPDATE_FLAGS_LOGIC(result) \
    CLR_FLAG(state, FLAG_Z | FLAG_S | FLAG_H | FLAG_C); \
    if((result) == 0) SET_FLAG(state, FLAG_Z); \
    if((result) & 0x80) SET_FLAG(state, FLAG_S);

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
bool load_rom(const char* filename);
bool load_z80_snapshot(const char* filename, Z80_State* state);
uint8_t input_port(Z80_State* state, uint8_t port);
void output_port(Z80_State* state, uint8_t port, uint8_t val);
void z80_int_reti(Z80_State* state);
uint8_t mem_read(uint32_t addr);
uint16_t mem_read16(uint32_t addr);
void mem_write(uint32_t addr, uint8_t val);
void mem_write16(uint32_t addr, uint8_t val);

// Core functions
void display_update();
void input_handle();
void z80_init(Z80_State* state);
int decode_cb(Z80_State* state);
int decode_dd(Z80_State* state);
int decode_ddcb(Z80_State* state);
int decode_ed(Z80_State* state);
int decode_fd(Z80_State* state);
int decode_fdcb(Z80_State* state);
int z80_step(Z80_State* state);

// Stack operations
void push16(Z80_State* state, uint16_t val);
uint16_t pop16(Z80_State* state);

#endif