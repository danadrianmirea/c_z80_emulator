#pragma once 

#include "zx_spectrum.h"

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

// Core functions
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