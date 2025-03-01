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

  // Read and verify header (30 bytes for version 1)
  uint8_t header[30];
  if (fread(header, 1, 30, file) != 30) {
    fprintf(stderr, "Invalid Z80 header\n");
    fclose(file);
    return false;
  }

  // Check for version 1 format (PC != 0)
  uint16_t pc = (header[7] << 8) | header[6];
  if (pc == 0) {
    fprintf(stderr, "Only version 1 Z80 files are supported\n");
    fclose(file);
    return false;
  }

  // Check compression flag (bit 5 of byte 12)
  bool compressed = (header[12] & 0x20) != 0;

  // Read compressed data
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 30, SEEK_SET);
  long data_size = file_size - 30;

  if (data_size <= 0) {
    fprintf(stderr, "Invalid Z80 data block\n");
    fclose(file);
    return false;
  }

  uint8_t* data = malloc(data_size);
  if (!data || fread(data, 1, data_size, file) != data_size) {
    fprintf(stderr, "Failed to read snapshot data\n");
    free(data);
    fclose(file);
    return false;
  }
  fclose(file);

  // Prepare memory writing
  size_t dest_idx = 0x4000;  // Start of Spectrum RAM
  const size_t ram_size = 0xC000;  // 48KB

  if (!compressed) {
    // Direct copy for uncompressed data
    if (data_size != ram_size) {
      fprintf(stderr, "Invalid RAM size: %ld (expected %zu)\n", data_size, ram_size);
      free(data);
      return false;
    }
    memcpy(&memory[dest_idx], data, ram_size);
  }
  else {
    // Decompress RLE data
    size_t src_idx = 0;
    while (src_idx < data_size && dest_idx < (0x4000 + ram_size)) {
      uint8_t b1 = data[src_idx++];

      if (b1 == 0xED && src_idx < data_size) {
        uint8_t b2 = data[src_idx++];
        if (b2 == 0xED && src_idx + 1 < data_size) {
          // RLE compressed block
          uint8_t count = data[src_idx++];
          uint8_t value = data[src_idx++];
          if (count == 0) count = 256;

          if (dest_idx + count > (0x4000 + ram_size)) {
            fprintf(stderr, "RLE overflow at 0x%zX\n", dest_idx);
            free(data);
            return false;
          }
          memset(&memory[dest_idx], value, count);
          dest_idx += count;
        }
        else {
          // Literal ED sequence
          if (dest_idx + 1 > (0x4000 + ram_size)) break;
          memory[dest_idx++] = 0xED;
          memory[dest_idx++] = b2;
        }
      }
      else {
        // Single byte
        if (dest_idx >= (0x4000 + ram_size)) break;
        memory[dest_idx++] = b1;
      }
    }

    if (dest_idx != (0x4000 + ram_size)) {
      fprintf(stderr, "Decompression incomplete: %zu/%zu bytes\n",
        dest_idx - 0x4000, ram_size);
      free(data);
      return false;
    }
  }

  free(data);
  printf("Z80 snapshot loaded successfully\n");
  return true;
}

uint8_t mem_read(uint16_t addr) {
  return memory[addr];
}

void mem_write(uint16_t addr, uint8_t value) {
  if (addr >= ROM_START && addr <= ROM_END) return;
  memory[addr] = value;
}