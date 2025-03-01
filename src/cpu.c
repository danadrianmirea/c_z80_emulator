// cpu.c
#include "zx_spectrum.h"
#include <stdint.h>

uint16_t pc, sp;
uint8_t a, f, b, c, d, e, h, l;

// Flag bit positions
#define FLAG_C 0x01  // Carry
#define FLAG_N 0x02  // Add/Subtract
#define FLAG_PV 0x04 // Parity/Overflow
#define FLAG_H 0x10  // Half Carry
#define FLAG_Z 0x40  // Zero
#define FLAG_S 0x80  // Sign

// Helper macros for flag operations
#define SET_FLAG(bit) (f |= (bit))
#define CLR_FLAG(bit) (f &= ~(bit))
#define TST_FLAG(bit) (f & (bit))

// Memory access shorthand
#define MEM(addr) memory[addr]
#define MEM_WORD(addr) (memory[addr] | (memory[addr+1] << 8))

// Register pairs
#define BC ( (b << 8) | c )
#define DE ( (d << 8) | e )
#define HL ( (h << 8) | l )

void z80_init() {}

void z80_step(uint8_t* memory) {
  uint8_t opcode = MEM(pc++);

  switch (opcode) {
    // 8-bit Loads
  case 0x06: // LD B,n
    b = MEM(pc++);
    break;

  case 0x0E: // LD C,n
    c = MEM(pc++);
    break;

  case 0x32: { // LD (nn),A
    uint16_t addr = MEM_WORD(pc);
    pc += 2;
    MEM(addr) = a;
    break;
  }

           // 8 bit ALU
  case 0x80: // ADD A,B
    a += b;
    CLR_FLAG(FLAG_N);
    // Set Z, S, H, C flags
    UPDATE_FLAGS_ADD(a, b);
    break;

  case 0x90: // SUB B
    a -= b;
    SET_FLAG(FLAG_N);
    UPDATE_FLAGS_SUB(a, b);
    break;

    // Jumps
  case 0xC3: { // JP nn
    pc = MEM_WORD(pc);
    break;
  }

  case 0xDA: { // JP C,nn
    if (TST_FLAG(FLAG_C)) {
      pc = MEM_WORD(pc);
    }
    else {
      pc += 2;
    }
    break;
  }

           // Stack Ops
  case 0xF5: // PUSH AF
    MEM(--sp) = a;
    MEM(--sp) = f;
    break;

  default:
    // Handle unknown opcode
    printf("Unknown opcode: 0x%02X at 0x%04X\n", opcode, pc - 1);
    break;
  }
}