// memory.c
#include "memory.h"
#include "zx_spectrum.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

uint8_t memory[MEM_SIZE] = { 0 };

bool load_rom(const char* path) {
  // Existing ROM loading code
  FILE* rom = fopen(path, "rb");
  if (!rom) {
    perror("ROM load failed");
    return false;
  }

  fseek(rom, 0, SEEK_END);
  long size = ftell(rom);
  rewind(rom);

  if (size != ROM_SIZE) {
    fprintf(stderr, "Invalid Spectrum ROM: %ld bytes (expected 16KB)\n", size);
    fclose(rom);
    return false;
  }

  size_t read = fread(&memory[ROM_START], 1, ROM_SIZE, rom);
  fclose(rom);

  if (read != ROM_SIZE) {
    fprintf(stderr, "Partial ROM read: %zu/16384 bytes\n", read);
    return false;
  }

  printf("Loaded Spectrum ROM successfully\n");
  return true;
}

bool load_z80_snapshot(const char* filename) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    perror("Failed to open Z80 file");
    return false;
  }

  // Read main header (at least 30 bytes)
  uint8_t header[58];
  if (fread(header, 1, 30, file) != 30) {
    fclose(file);
    return false;
  }

  // Detect version (byte 29/0x1D)
  int version = 1;
  if (header[29] == 23 || header[29] == 54) version = 2;

  // Read extended header for version 2/3
  if (version >= 2) {
    if (fread(&header[30], 1, 28, file) != 28) {
      fclose(file);
      return false;
    }
  }

  // Handle different version formats
  uint16_t pc = (header[7] << 8) | header[6];
  bool compressed = false;
  size_t data_offset = 30;

  if (version >= 2) {
    // Version 2+ specific handling
    compressed = (header[34] & 0x20); // Compression flag in extended header
    data_offset = 32 + header[30];    // Skip additional headers
    pc = (header[33] << 8) | header[32];
  }
  else {
    // Original version 1 handling
    compressed = (header[12] & 0x20);
  }

  // Get RAM data
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, data_offset, SEEK_SET);
  long data_size = file_size - data_offset;

  uint8_t* data = malloc(data_size);
  if (!data || fread(data, 1, data_size, file) != data_size) {
    free(data);
    fclose(file);
    return false;
  }
  fclose(file);

  // Handle different memory models
  size_t dest_idx = 0x4000;  // Standard 48K Spectrum RAM start
  size_t ram_size = 0xC000;   // 48K RAM size

  if (!compressed) {
    // Simple memory copy for uncompressed data
    if (data_size != ram_size) {
      fprintf(stderr, "Unexpected RAM size: %ld (expected %zu)\n",
        data_size, ram_size);
      free(data);
      return false;
    }
    memcpy(&memory[dest_idx], data, ram_size);
  }
  else {
    // Enhanced decompression handling
    size_t src_idx = 0;
    while (src_idx < data_size && dest_idx < (0x4000 + ram_size)) {
      if (data[src_idx] == 0xED && (src_idx + 1 < data_size) && data[src_idx + 1] == 0xED) {
        // Compressed block
        src_idx += 2;
        if (src_idx + 1 >= data_size) break;

        uint16_t count = data[src_idx++];
        uint16_t value = data[src_idx++];

        if (count == 0) count = 256;
        memset(&memory[dest_idx], value, count);
        dest_idx += count;
      }
      else {
        // Uncompressed byte
        memory[dest_idx++] = data[src_idx++];
      }
    }
  }

  free(data);
  printf("Successfully loaded Z80 snapshot (version %d)\n", version);
  return true;
}

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