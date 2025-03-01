// memory.c
#include "zx_spectrum.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// 64KB memory array
uint8_t memory[MEM_SIZE] = { 0 };

bool load_rom(const char* path) {
  FILE* rom = fopen(path, "rb");
  if (!rom) {
    perror("ROM load failed");
    return false;
  }

  fseek(rom, 0, SEEK_END);
  long size = ftell(rom);
  rewind(rom);

  if (size != SPECTRUM_ROM_SIZE) {
    fprintf(stderr, "Invalid Spectrum ROM: %ld bytes (expected 16KB)\n", size);
    fclose(rom);
    return false;
  }

  size_t read = fread(&memory[ROM_START], 1, SPECTRUM_ROM_SIZE, rom);
  fclose(rom);

  if (read != SPECTRUM_ROM_SIZE) {
    fprintf(stderr, "Partial ROM read: %zu/16384 bytes\n", read);
    return false;
  }

  printf("Loaded Spectrum ROM successfully\n");
  return true;
}

// Memory access functions
uint8_t mem_read(uint16_t addr) {
  // Basic memory access without contention
  return memory[addr];
}

void mem_write(uint16_t addr, uint8_t value) {
  // Handle ROM protection (Spectrum ROM is read-only)
  if (addr >= ROM_START && addr <= ROM_END) {
    return; // Ignore writes to ROM area
  }

  // Regular RAM write
  memory[addr] = value;
}