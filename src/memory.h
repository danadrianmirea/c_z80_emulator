#pragma once

#include "zx_spectrum.h"
#include <stdint.h>

extern uint8_t memory[MEM_SIZE];

// Memory interface
uint8_t mem_read(uint32_t addr);
uint16_t mem_read16(uint32_t addr);
void mem_write(uint32_t addr, uint8_t val);
void mem_write16(uint32_t addr, uint8_t val);
uint8_t input_port(Z80_State* state, uint8_t port);
void output_port(Z80_State* state, uint8_t port, uint8_t val);
void z80_int_reti(Z80_State* state);
