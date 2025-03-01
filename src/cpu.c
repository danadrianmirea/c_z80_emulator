#include "zx_spectrum.h"
#include "cpu.h"
#include <stdio.h>

// Precomputed parity table (even parity)
static const uint8_t parity_table[256] = {
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1,0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,
    0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0,1,0,0,1,0,1,1,0,0,1,1,0,1,0,0,1
};

void z80_init(Z80_State* state) {
  state->pc = 0x0000;
  state->sp = 0xFFFF;
  state->iff1 = state->iff2 = 0;
  state->imode = 0;
}

static void add_a(Z80_State* state, uint8_t val) {
  uint16_t res = state->a + val;
  uint8_t carry = (res > 0xFF) ? FLAG_C : 0;

  CLR_FLAG(state, FLAG_N | FLAG_C | FLAG_H);
  SET_FLAG(state, carry);
  if (((state->a & 0x0F) + (val & 0x0F)) > 0x0F) SET_FLAG(state, FLAG_H);

  state->a = res & 0xFF;
  UPDATE_SZP(state, state->a);
}

static void sub_a(Z80_State* state, uint8_t val) {
  uint16_t res = state->a - val;
  uint8_t carry = (res > 0xFF) ? FLAG_C : 0;

  SET_FLAG(state, FLAG_N);
  CLR_FLAG(state, FLAG_C | FLAG_H);
  SET_FLAG(state, carry);
  if (((state->a & 0x0F) - (val & 0x0F)) & 0x10) SET_FLAG(state, FLAG_H);

  state->a = res & 0xFF;
  UPDATE_SZP(state, state->a);
}

int z80_step(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);

  switch (opcode) {
    // 8-bit Load Group
  case 0x06: // LD B,n
    state->b = mem_read(state->pc++);
    break;

  case 0x0E: // LD C,n
    state->c = mem_read(state->pc++);
    break;

    // 8-bit Arithmetic Group
  case 0x80: // ADD A,B
    add_a(state, state->b);
    break;

  case 0x90: // SUB B
    sub_a(state, state->b);
    break;

    // 8-bit Logical Group
  case 0xA0: // AND B
    state->a &= state->b;
    CLR_FLAG(state, FLAG_N | FLAG_C);
    SET_FLAG(state, FLAG_H);
    UPDATE_SZP(state, state->a);
    break;

  case 0xB0: // OR B
    state->a |= state->b;
    CLR_FLAG(state, FLAG_N | FLAG_H | FLAG_C);
    UPDATE_SZP(state, state->a);
    break;

    // Increment/Decrement Group
  case 0x04: { // INC B
    uint8_t res = state->b + 1;
    CLR_FLAG(state, FLAG_N);
    if ((state->b & 0x0F) == 0x0F) SET_FLAG(state, FLAG_H);
    state->b = res;
    UPDATE_SZP(state, res);
    break;
  }

  case 0x05: { // DEC B
    uint8_t res = state->b - 1;
    SET_FLAG(state, FLAG_N);
    if ((state->b & 0x0F) == 0) SET_FLAG(state, FLAG_H);
    state->b = res;
    UPDATE_SZP(state, res);
    break;
  }

  default:
    fprintf(stderr, "Unimplemented opcode: 0x%02X at 0x%04X\n",
      opcode, state->pc - 1);
    return -1;
  }

  return 0;
}

void push16(Z80_State* state, uint16_t val) {
  mem_write(--state->sp, (val >> 8) & 0xFF);
  mem_write(--state->sp, val & 0xFF);
}

uint16_t pop16(Z80_State* state) {
  uint16_t lo = mem_read(state->sp++);
  uint16_t hi = mem_read(state->sp++);
  return (hi << 8) | lo;
}