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

int decode_cb(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;

  switch (opcode) {
  case 0x40: // BIT 0, B
  case 0x41: // BIT 0, C
  case 0x42: // BIT 0, D
  case 0x43: // BIT 0, E
  case 0x44: // BIT 0, H
  case 0x45: // BIT 0, L
  case 0x46: // BIT 0, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0x48: // BIT 1, B
  case 0x49: // BIT 1, C
  case 0x4A: // BIT 1, D
  case 0x4B: // BIT 1, E
  case 0x4C: // BIT 1, H
  case 0x4D: // BIT 1, L
  case 0x4E: // BIT 1, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0x50: // BIT 2, B
  case 0x51: // BIT 2, C
  case 0x52: // BIT 2, D
  case 0x53: // BIT 2, E
  case 0x54: // BIT 2, H
  case 0x55: // BIT 2, L
  case 0x56: // BIT 2, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0x58: // BIT 3, B
  case 0x59: // BIT 3, C
  case 0x5A: // BIT 3, D
  case 0x5B: // BIT 3, E
  case 0x5C: // BIT 3, H
  case 0x5D: // BIT 3, L
  case 0x5E: // BIT 3, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0x60: // BIT 4, B
  case 0x61: // BIT 4, C
  case 0x62: // BIT 4, D
  case 0x63: // BIT 4, E
  case 0x64: // BIT 4, H
  case 0x65: // BIT 4, L
  case 0x66: // BIT 4, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0x68: // BIT 5, B
  case 0x69: // BIT 5, C
  case 0x6A: // BIT 5, D
  case 0x6B: // BIT 5, E
  case 0x6C: // BIT 5, H
  case 0x6D: // BIT 5, L
  case 0x6E: // BIT 5, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0x70: // BIT 6, B
  case 0x71: // BIT 6, C
  case 0x72: // BIT 6, D
  case 0x73: // BIT 6, E
  case 0x74: // BIT 6, H
  case 0x75: // BIT 6, L
  case 0x76: // BIT 6, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0x78: // BIT 7, B
  case 0x79: // BIT 7, C
  case 0x7A: // BIT 7, D
  case 0x7B: // BIT 7, E
  case 0x7C: // BIT 7, H
  case 0x7D: // BIT 7, L
  case 0x7E: // BIT 7, (HL)
    temp = (state->b >> (opcode & 0x07)) & 1;
    state->f &= ~(FLAG_Z | FLAG_N | FLAG_H | FLAG_PV);
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_H;
    break;

  case 0xC0: // SET 0, B
  case 0xC1: // SET 0, C
  case 0xC2: // SET 0, D
  case 0xC3: // SET 0, E
  case 0xC4: // SET 0, H
  case 0xC5: // SET 0, L
  case 0xC6: // SET 0, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xC6) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xC7: // SET 1, B
  case 0xC8: // SET 1, C
  case 0xC9: // SET 1, D
  case 0xCA: // SET 1, E
  case 0xCB: // SET 1, H
  case 0xCC: // SET 1, L
  case 0xCD: // SET 1, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xCD) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xCE: // SET 2, B
  case 0xCF: // SET 2, C
  case 0xD0: // SET 2, D
  case 0xD1: // SET 2, E
  case 0xD2: // SET 2, H
  case 0xD3: // SET 2, L
  case 0xD4: // SET 2, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xD4) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xD5: // SET 3, B
  case 0xD6: // SET 3, C
  case 0xD7: // SET 3, D
  case 0xD8: // SET 3, E
  case 0xD9: // SET 3, H
  case 0xDA: // SET 3, L
  case 0xDB: // SET 3, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xDB) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xDC: // SET 4, B
  case 0xDD: // SET 4, C
  case 0xDE: // SET 4, D
  case 0xDF: // SET 4, E
  case 0xE0: // SET 4, H
  case 0xE1: // SET 4, L
  case 0xE2: // SET 4, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xE2) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xE3: // SET 5, B
  case 0xE4: // SET 5, C
  case 0xE5: // SET 5, D
  case 0xE6: // SET 5, E
  case 0xE7: // SET 5, H
  case 0xE8: // SET 5, L
  case 0xE9: // SET 5, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xE9) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xEA: // SET 6, B
  case 0xEB: // SET 6, C
  case 0xEC: // SET 6, D
  case 0xED: // SET 6, E
  case 0xEE: // SET 6, H
  case 0xEF: // SET 6, L
  case 0xF0: // SET 6, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xF0) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xF1: // SET 7, B
  case 0xF2: // SET 7, C
  case 0xF3: // SET 7, D
  case 0xF4: // SET 7, E
  case 0xF5: // SET 7, H
  case 0xF6: // SET 7, L
  case 0xF7: // SET 7, (HL)
    temp = (state->b | (1 << (opcode & 0x07)));
    if (opcode == 0xF7) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xB0: // RES 0, B
  case 0xB1: // RES 0, C
  case 0xB2: // RES 0, D
  case 0xB3: // RES 0, E
  case 0xB4: // RES 0, H
  case 0xB5: // RES 0, L
  case 0xB6: // RES 0, (HL)
    temp = (state->b & ~(1 << (opcode & 0x07)));
    if (opcode == 0xB6) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xB7: // RES 1, B
  case 0xB8: // RES 1, C
  case 0xB9: // RES 1, D
  case 0xBA: // RES 1, E
  case 0xBB: // RES 1, H
  case 0xBC: // RES 1, L
  case 0xBD: // RES 1, (HL)
    temp = (state->b & ~(1 << (opcode & 0x07)));
    if (opcode == 0xBD) {
      mem_write(state->hl, temp);
    }
    else {
      switch (opcode & 0xC0) {
      case 0x00: state->b = temp; break;
      case 0x40: state->c = temp; break;
      case 0x80: state->d = temp; break;
      case 0xC0: state->e = temp; break;
      }
    }
    break;

  case 0xBE: // RES 2, B
  case 0xBF: // RES 2, C
  default:
    printf("Unknown CB opcode: %02X\n", opcode);
    return 0;
  }
}

int z80_step(Z80_State* state) {
  uint8_t opcode = mem_read(state->pc++);
  uint8_t temp;
  uint16_t temp16;
  uint8_t n;
  uint8_t carry;
  uint8_t res;
  uint8_t port;

  switch (opcode) {
    // 8-bit Load Group
  case 0x06: // LD B,n
    state->b = mem_read(state->pc++);
    break;

  case 0x0A: // LD A,(BC)
    state->a = mem_read(state->bc);
    break;

  case 0x1A: // LD A,(DE)
    state->a = mem_read(state->de);
    break;

  case 0x2A: // LD HL,(nn)
    temp16 = mem_read(state->pc++);
    temp16 |= mem_read(state->pc++) << 8;
    state->l = mem_read(temp16);
    state->h = mem_read(temp16 + 1);
    break;

  case 0x3A: // LD A,(nn)
    temp16 = mem_read(state->pc++);
    temp16 |= mem_read(state->pc++) << 8;
    state->a = mem_read(temp16);
    break;

  case 0x4A: // LD C,D
    state->c = state->d;
    break;

  case 0x5A: // LD E,D
    state->e = state->d;
    break;

  case 0x6A: // LD L,D
    state->l = state->d;
    break;

  case 0x7A: // LD A,D
    state->a = state->d;
    break;

  case 0x0E: // LD C,n
    state->c = mem_read(state->pc++);
    break;

  case 0x16: // LD D,n
    state->d = mem_read(state->pc++);
    break;

  case 0x1E: // LD E,n
    state->e = mem_read(state->pc++);
    break;

  case 0x26: // LD H,n
    state->h = mem_read(state->pc++);
    break;

  case 0x2E: // LD L,n
    state->l = mem_read(state->pc++);
    break;

  case 0x36: // LD (HL),n
    mem_write(state->hl, mem_read(state->pc++));
    break;

  case 0x3E: // LD A,n
    state->a = mem_read(state->pc++);
    break;

  case 0x40: // LD B,B
    state->b = state->b;
    break;

  case 0x41: // LD B,C
    state->b = state->c;
    break;

  case 0x42: // LD B,D
    state->b = state->d;
    break;

  case 0x43: // LD B,E
    state->b = state->e;
    break;

  case 0x44: // LD B,H
    state->b = state->h;
    break;

  case 0x45: // LD B,L
    state->b = state->l;
    break;

  case 0x46: // LD B,(HL)
    state->b = mem_read(state->hl);
    break;

  case 0x47: // LD B,A
    state->b = state->a;
    break;

  case 0x48: // LD C,B
    state->c = state->b;
    break;

  case 0x49: // LD C,C
    state->c = state->c;
    break;

  case 0x4B: // LD C,E
    state->c = state->e;
    break;

  case 0x4C: // LD C,H
    state->c = state->h;
    break;

  case 0x4D: // LD C,L
    state->c = state->l;
    break;

  case 0x4E: // LD C,(HL)
    state->c = mem_read(state->hl);
    break;

  case 0x4F: // LD C,A
    state->c = state->a;
    break;

  case 0x50: // LD D,B
    state->d = state->b;
    break;

  case 0x51: // LD D,C
    state->d = state->c;
    break;

  case 0x52: // LD D,D
    state->d = state->d;
    break;

  case 0x53: // LD D,E
    state->d = state->e;
    break;

  case 0x54: // LD D,H
    state->d = state->h;
    break;

  case 0x55: // LD D,L
    state->d = state->l;
    break;

  case 0x56: // LD D,(HL)
    state->d = mem_read(state->hl);
    break;

  case 0x57: // LD D,A
    state->d = state->a;
    break;

  case 0x58: // LD E,B
    state->e = state->b;
    break;

  case 0x59: // LD E,C
    state->e = state->c;
    break;

  case 0x5B: // LD E,E
    state->e = state->e;
    break;

  case 0x5C: // LD E,H
    state->e = state->h;
    break;

  case 0x5D: // LD E,L
    state->e = state->l;
    break;

  case 0x5E: // LD E,(HL)
    state->e = mem_read(state->hl);
    break;

  case 0x5F: // LD E,A
    state->e = state->a;
    break;

  case 0x60: // LD H,B
    state->h = state->b;
    break;

  case 0x61: // LD H,C
    state->h = state->c;
    break;

  case 0x62: // LD H,D
    state->h = state->d;
    break;

  case 0x63: // LD H,E
    state->h = state->e;
    break;

  case 0x64: // LD H,H
    state->h = state->h;
    break;

  case 0x65: // LD H,L
    state->h = state->l;
    break;

  case 0x66: // LD H,(HL)
    state->h = mem_read(state->hl);
    break;

  case 0x67: // LD H,A
    state->h = state->a;
    break;

  case 0x68: // LD L,B
    state->l = state->b;
    break;

  case 0x69: // LD L,C
    state->l = state->c;
    break;

  case 0x6B: // LD L,E
    state->l = state->e;
    break;

  case 0x6C: // LD L,H
    state->l = state->h;
    break;

  case 0x6D: // LD L,L
    state->l = state->l;
    break;

  case 0x6E: // LD L,(HL)
    state->l = mem_read(state->hl);
    break;

  case 0x6F: // LD L,A
    state->l = state->a;
    break;

  case 0x70: // LD (HL),B
    mem_write(state->hl, state->b);
    break;

  case 0x71: // LD (HL),C
    mem_write(state->hl, state->c);
    break;

  case 0x72: // LD (HL),D
    mem_write(state->hl, state->d);
    break;

  case 0x73: // LD (HL),E
    mem_write(state->hl, state->e);
    break;

  case 0x74: // LD (HL),H
    mem_write(state->hl, state->h);
    break;

  case 0x75: // LD (HL),L
    mem_write(state->hl, state->l);
    break;

  case 0x76: // HALT
    // TODO: Implement HALT instruction
    break;

  case 0x77: // LD (HL),A
    mem_write(state->hl, state->a);
    break;

  case 0x78: // LD A,B
    state->a = state->b;
    break;

  case 0x79: // LD A,C
    state->a = state->c;
    break;

  case 0x7B: // LD A,E
    state->a = state->e;
    break;

  case 0x7C: // LD A,H
    state->a = state->h;
    break;

  case 0x7D: // LD A,L
    state->a = state->l;
    break;

  case 0x7E: // LD A,(HL)
    state->a = mem_read(state->hl);
    break;

  case 0x7F: // LD A,A
    state->a = state->a;
    break;

    // 8-bit Logical Group
  case 0x80: // ADD A, B
    uint8_t temp = state->a + state->b;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->b & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->b ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x81: // ADD A, C
    temp = state->a + state->c;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->c & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->c ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x82: // ADD A, D
    temp = state->a + state->d;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->d & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->d ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x83: // ADD A, E
    temp = state->a + state->e;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->e & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->e ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x84: // ADD A, H
    temp = state->a + state->h;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->h & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->h ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x85: // ADD A, L
    temp = state->a + state->l;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->l & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->l ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x86: // ADD A, (HL)
    temp = state->a + mem_read(state->hl);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (mem_read(state->hl) & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ mem_read(state->hl) ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x87: // ADD A, A
    temp = state->a + state->a;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->a & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->a ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x88: // ADC A, B
    temp = state->a + state->b + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->b & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->b ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x89: // ADC A, C
    temp = state->a + state->c + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->c & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->c ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8A: // ADC A, D
    temp = state->a + state->d + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->d & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->d ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8B: // ADC A, E
    temp = state->a + state->e + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->e & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->e ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8C: // ADC A, H
    temp = state->a + state->h + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->h & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->h ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8D: // ADC A, L
    temp = state->a + state->l + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->l & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->l ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8E: // ADC A, (HL)
    temp = state->a + mem_read(state->hl) + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (mem_read(state->hl) & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ mem_read(state->hl) ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x8F: // ADC A, A
    temp = state->a + state->a + (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (state->a & 0x0F) + (state->f & FLAG_C) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->f |= (state->a ^ state->a ^ temp) & FLAG_C;
    state->a = temp;
    break;

  case 0x90: // SUB B
    temp = state->a - state->b;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->b & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->b) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x91: // SUB C
    temp = state->a - state->c;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->c & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->c) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x92: // SUB D
    temp = state->a - state->d;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->d & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->d) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x93: // SUB E
    temp = state->a - state->e;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->e & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->e) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x94: // SUB H
    temp = state->a - state->h;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->h & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->h) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x95: // SUB L
    temp = state->a - state->l;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->l & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->l) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x96: // SUB (HL)
    temp = state->a - mem_read(state->hl);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (mem_read(state->hl) & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < mem_read(state->hl)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x97: // SUB A
    temp = state->a - state->a;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->a & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->a) ? FLAG_C : 0;
    state->a = temp;
    break;
  case 0x98: // SBC A, B
    temp = state->a - state->b - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->b & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->b + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x99: // SBC A, C
    temp = state->a - state->c - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->c & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->c + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9A: // SBC A, D
    temp = state->a - state->d - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->d & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->d + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9B: // SBC A, E
    temp = state->a - state->e - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->e & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->e + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9C: // SBC A, H
    temp = state->a - state->h - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->h & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->h + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9D: // SBC A, L
    temp = state->a - state->l - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->l & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->l + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9E: // SBC A, (HL)
    temp = state->a - mem_read(state->hl) - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (mem_read(state->hl) & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < mem_read(state->hl) + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0x9F: // SBC A, A
    temp = state->a - state->a - (state->f & FLAG_C);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->a & 0x0F) + (state->f & FLAG_C) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->f |= (state->a < state->a + (state->f & FLAG_C)) ? FLAG_C : 0;
    state->a = temp;
    break;

  case 0xA0: // AND B
    state->a = state->a & state->b;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA1: // AND C
    state->a = state->a & state->c;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA2: // AND D
    state->a = state->a & state->d;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA3: // AND E
    state->a = state->a & state->e;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA4: // AND H
    state->a = state->a & state->h;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA5: // AND L
    state->a = state->a & state->l;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA6: // AND (HL)
    state->a = state->a & mem_read(state->hl);
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA7: // AND A
    state->a = state->a & state->a;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA8: // XOR B
    state->a = state->a ^ state->b;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xA9: // XOR C
    state->a = state->a ^ state->c;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAA: // XOR D
    state->a = state->a ^ state->d;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAB: // XOR E
    state->a = state->a ^ state->e;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAC: // XOR H
    state->a = state->a ^ state->h;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAD: // XOR L
    state->a = state->a ^ state->l;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAE: // XOR (HL)
    state->a = state->a ^ mem_read(state->hl);
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xAF: // XOR A
    state->a = state->a ^ state->a;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB0: // OR B
    state->a = state->a | state->b;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;
  case 0xB1: // OR C
    state->a = state->a | state->c;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB2: // OR D
    state->a = state->a | state->d;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB3: // OR E
    state->a = state->a | state->e;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB4: // OR H
    state->a = state->a | state->h;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB5: // OR L
    state->a = state->a | state->l;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB6: // OR (HL)
    state->a = state->a | mem_read(state->hl);
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB7: // OR A
    state->a = state->a | state->a;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    break;

  case 0xB8: // CP B
    temp = state->a - state->b;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->b & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->b) ? FLAG_N : 0;
    break;

  case 0xB9: // CP C
    temp = state->a - state->c;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->c & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->c) ? FLAG_N : 0;
    break;

  case 0xBA: // CP D
    temp = state->a - state->d;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->d & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->d) ? FLAG_N : 0;
    break;

  case 0xBB: // CP E
    temp = state->a - state->e;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->e & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->e) ? FLAG_N : 0;
    break;

  case 0xBC: // CP H
    temp = state->a - state->h;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->h & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->h) ? FLAG_N : 0;
    break;

  case 0xBD: // CP L
    temp = state->a - state->l;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->l & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->l) ? FLAG_N : 0;
    break;

  case 0xBE: // CP (HL)
    temp = state->a - mem_read(state->hl);
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (mem_read(state->hl) & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < mem_read(state->hl)) ? FLAG_N : 0;
    break;

  case 0xBF: // CP A
    temp = state->a - state->a;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (state->a & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < state->a) ? FLAG_N : 0;
    break;

  case 0xC6: // ADD A, n
    uint8_t n = mem_read(state->pc + 1);
    temp = state->a + n;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (n & 0x0F) > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->a = temp;
    break;

  case 0xCE: // ADC A, n
    n = mem_read(state->pc + 1);
    carry = (state->f & FLAG_C) != 0 ? 1 : 0;
    temp = state->a + n + carry;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) + (n & 0x0F) + carry > 0x0F ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    state->a = temp;
    break;

  case 0xD6: // SUB n
    n = mem_read(state->pc + 1);
    temp = state->a - n;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (n & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->a = temp;
    break;

  case 0xDE: // SBC A, n
    n = mem_read(state->pc + 1);
    carry = (state->f & FLAG_C) != 0 ? 1 : 0;
    temp = state->a - n - carry;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (n & 0x0F) + carry ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= FLAG_N;
    state->a = temp;
    break;

  case 0xE6: // AND n
    n = mem_read(state->pc + 1);
    state->a &= n;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f &= ~FLAG_PV;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    break;

  case 0xEE: // XOR n
    n = mem_read(state->pc + 1);
    state->a ^= n;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f &= ~FLAG_PV;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    break;

  case 0xEF: // RST 28H
    temp16 = state->pc;
    state->sp -= 2;
    mem_write(state->sp, (temp >> 8) & 0xFF);
    mem_write(state->sp + 1, temp & 0xFF);
    state->pc = 0x28;
    break;

  case 0xF6: // OR n
    n = mem_read(state->pc + 1);
    state->a |= n;
    state->f = (state->a & FLAG_S) != 0 ? FLAG_S : 0;
    state->f &= ~FLAG_PV;
    state->f |= (state->a & 0x08) != 0 ? FLAG_PV : 0;
    state->f &= ~FLAG_H;
    state->f |= (state->a == 0) ? FLAG_Z : 0;
    state->f &= ~FLAG_N;
    break;

  case 0xFE: // CP n
    n = mem_read(state->pc + 1);
    temp = state->a - n;
    state->f = (temp & FLAG_S) != 0 ? FLAG_S : 0;
    state->f |= (temp & 0x08) != 0 ? FLAG_PV : 0;
    state->f |= (state->a & 0x0F) < (n & 0x0F) ? FLAG_H : 0;
    state->f |= (temp == 0) ? FLAG_Z : 0;
    state->f |= (state->a < n) ? FLAG_N : 0;
    break;

    // Increment/Decrement Group
  case 0x04: { // INC B
    res = state->b + 1;
    CLR_FLAG(state, FLAG_N);
    if ((state->b & 0x0F) == 0x0F) SET_FLAG(state, FLAG_H);
    state->b = res;
    UPDATE_SZP(state, res);
    break;
  }

  case 0x05: { // DEC B
    res = state->b - 1;
    SET_FLAG(state, FLAG_N);
    if ((state->b & 0x0F) == 0) SET_FLAG(state, FLAG_H);
    state->b = res;
    UPDATE_SZP(state, res);
    break;
  }
           // Control flow instructions 
  case 0x10: // DJNZ n
    temp = mem_read(state->pc + 1);
    state->b--;
    if (state->b != 0) {
      state->pc += temp;
    }
    break;

  case 0x18: // JR n
    temp = mem_read(state->pc + 1);
    state->pc += temp;
    break;

  case 0x20: // JR NZ, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_Z) == 0) {
      state->pc += temp;
    }
    break;

  case 0x28: // JR Z, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_Z) != 0) {
      state->pc += temp;
    }
    break;

  case 0x30: // JR NC, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_C) == 0) {
      state->pc += temp;
    }
    break;

  case 0x38: // JR C, n
    temp = mem_read(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      state->pc += temp;
    }
    break;

  case 0xC2: // JP NZ, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) == 0) {
      state->pc = temp16;
    }
    break;

  case 0xC4: // CALL NZ, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) == 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xCA: // JP Z, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) != 0) {
      state->pc = temp16;
    }
    break;

  case 0xCC: // CALL Z, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_Z) != 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xD2: // JP NC, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) == 0) {
      state->pc = temp16;
    }
    break;

  case 0xD4: // CALL NC, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) == 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xDA: // JP C, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      state->pc = temp16;
    }
    break;

  case 0xDC: // CALL C, nn
    temp16 = mem_read16(state->pc + 1);
    if ((state->f & FLAG_C) != 0) {
      push16(state, state->pc + 3);
      state->pc = temp16;
    }
    break;

  case 0xF3: // DI
    state->iff1 = state->iff2 = 0;
    break;

  case 0xFB: // EI
    state->iff1 = state->iff2 = 1;
    break;

    // Miscellaneous instructions
  case 0x08: // EX AF, AF'
    uint8_t tempA = state->a;
    uint8_t tempF = state->f;
    state->a = state->a_;
    state->f = state->f_;
    state->a_ = tempA;
    state->f_ = tempF;
    break;

  case 0xD3: // OUT (n), A
    port = mem_read(state->pc++);
    // Implement I/O write to 'port' with 'state->a' here
    // Example: io_write(port, state->a);
    break;

  case 0xDB: // IN A, (n)
    port = mem_read(state->pc++);
    // Implement I/O read from 'port' to 'state->a' here
    // Example: state->a = io_read(port);
    break;

  case 0xE3: // EX (SP), HL
    uint16_t tempHL = state->hl;
    state->hl = pop16(state);
    push16(state, tempHL);
    break;

  case 0xE9: // JP (HL)
    state->pc = state->hl;
    break;

  case 0xF9: // LD SP, HL
    state->sp = state->hl;
    break;

  case 0xCB:
    return decode_cb(state); // CB prefixed instructions           

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