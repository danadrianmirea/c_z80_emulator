#include "memory.h"

uint8_t memory[MEM_SIZE] = { 0 };


uint8_t mem_read(uint32_t addr) {
    return memory[addr];
  }
  
  uint16_t mem_read16(uint32_t addr) {
    return (memory[addr + 1] << 8) | memory[addr];
  }
  
  void mem_write(uint32_t addr, uint8_t value) {
    if (addr >= MEM_SIZE) return;
    memory[addr] = value;
  }
  
  void mem_write16(uint32_t addr, uint8_t value) {
    if (addr >= MEM_SIZE) return;
    memory[addr + 1] = value >> 8;
    memory[addr] = value & 0xFF;
  }
  
  uint8_t input_port(Z80_State* state, uint8_t port) {
    return mem_read(port);
  }
  
  void output_port(Z80_State* state, uint8_t port, uint8_t val) {
    // Set the border color
    memory[0x5800 + (port & 0x1F)] = val;
    //printf("Port %02X: %02X\n", port, val);
  }
  