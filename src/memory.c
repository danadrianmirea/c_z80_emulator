// memory.h
#include "zx_spectrum.h"

uint8_t memory[MEM_SIZE];

bool load_rom(const char* path) {
  FILE* rom = fopen(path, "rb");
  if (!rom) {
    perror("ROM load failed");
    return 0;
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