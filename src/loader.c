#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "loader.h"
#include "zx_spectrum.h"
#include "memory.h"

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
  
bool load_z80_snapshot(const char* filename, Z80_State* state) {
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
  
    // Read register state
    state->pc = pc;
    state->af = (header[0] << 8) | header[1];
    state->bc = (header[2] << 8) | header[3];
    state->de = (header[4] << 8) | header[5];
    state->hl = (header[8] << 8) | header[9];
    state->ix = (header[10] << 8) | header[11];
    state->iy = (header[12] << 8) | header[13];
    state->sp = (header[14] << 8) | header[15];
    state->f = header[16];
    state->imode = header[17];
    state->r = header[18];
  
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

  bool load_sna(const char* filename, Z80_State* state) {
    FILE* sna_file = fopen(filename, "rb");
    if (!sna_file) {
        perror("SNA load failed");
        return false;
    }

    fread(&state->i, sizeof(uint8_t), 1, sna_file);
    fread(&state->hl_, sizeof(uint16_t), 1, sna_file);
    fread(&state->de_, sizeof(uint16_t), 1, sna_file);
    fread(&state->bc_, sizeof(uint16_t), 1, sna_file);
    fread(&state->af_, sizeof(uint16_t), 1, sna_file);
    fread(&state->hl, sizeof(uint16_t), 1, sna_file);
    fread(&state->de, sizeof(uint16_t), 1, sna_file);
    fread(&state->bc, sizeof(uint16_t), 1, sna_file);
    fread(&state->iy, sizeof(uint16_t), 1, sna_file);
    fread(&state->ix, sizeof(uint16_t), 1, sna_file);
    fread(&state->iff1, sizeof(uint8_t), 1, sna_file);
    fread(&state->iff2, sizeof(uint8_t), 1, sna_file);

    uint8_t imode;
    fread(&imode, sizeof(uint8_t), 1, sna_file);
    state->imode = imode & 0x03; // IM is only 2 bits

    fread(&state->sp, sizeof(uint16_t), 1, sna_file);
    fread(&state->af, sizeof(uint16_t), 1, sna_file);
    fread(&state->r, sizeof(uint8_t), 1, sna_file);

    // Read RAM (48 KB from 0x4000 to 0xFFFF)
    size_t bytes_read = fread(&memory[0x4000], 1, 0xC000, sna_file);
    if (bytes_read != 0xC000) {
        fprintf(stderr, "Partial SNA read: only %zu bytes read\n", bytes_read);
        fclose(sna_file);
        return false;
    }

    fclose(sna_file);

    // Fix PC: it's stored at the top of the stack
    state->pc = memory[state->sp] | (memory[state->sp + 1] << 8);
    state->sp += 2;  // Adjust SP to pop the stored PC

    printf("Loaded SNA snapshot successfully\n");
    return true;
}