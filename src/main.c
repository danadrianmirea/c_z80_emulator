/* main.c */
#include "zx_spectrum.h"
#include <SDL2/SDL.h>
#include <stdio.h>

extern uint8_t memory[MEM_SIZE];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

// Spectrum color palette (RGB888)
const uint32_t palette[16] = {
    0xFF000000, 0xFF0000D7, 0xFFD70000, 0xFFD700D7, // Black, Blue, Red, Magenta
    0xFF00D700, 0xFF00D7D7, 0xFFD7D700, 0xFFD7D7D7, // Green, Cyan, Yellow, White
    0xFF000000, 0xFF0000FF, 0xFFFF0000, 0xFFFF00FF, // Bright variants
    0xFF00FF00, 0xFF00FFFF, 0xFFFFFF00, 0xFFFFFFFF
};

void display_init() {
  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow(
    "ZX Spectrum Emulator",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    SCREEN_WIDTH * SCALE_FACTOR,
    SCREEN_HEIGHT * SCALE_FACTOR,
    SDL_WINDOW_SHOWN
  );

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    SCREEN_WIDTH,
    SCREEN_HEIGHT
  );
}

void display_update(uint8_t* memory) {
  static uint32_t flash_counter = 0;
  flash_counter++;

  // Convert Spectrum screen memory to pixels
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      // Calculate screen memory address
      uint16_t addr = 0x4000 |
        ((y & 0b11000000) << 5) |
        ((y & 0b00111000) << 2) |
        ((y & 0b00000111) << 8) |
        (x >> 3);

      // Get pixel value from bitmap
      uint8_t byte = memory[addr];
      uint8_t mask = 0x80 >> (x & 7);
      bool pixel = byte & mask;

      // Get color attributes
      uint16_t attr_addr = 0x5800 + ((y >> 3) << 5) + (x >> 3);
      uint8_t attr = memory[attr_addr];

      // Decode colors
      uint8_t ink = attr & 0x07;
      uint8_t paper = (attr >> 3) & 0x07;
      bool bright = (attr & 0x40) >> 6;
      bool flash = (attr & 0x80) && (flash_counter & 0x10);

      // Apply brightness and flash
      if (bright) { ink += 8; paper += 8; }
      if (flash) { uint8_t temp = ink; ink = paper; paper = temp; }

      // Set pixel color
      pixels[y * SCREEN_WIDTH + x] = palette[(pixel ? ink : paper) + (bright ? 8 : 0)];
    }
  }

  // Update SDL texture
  SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void input_handle(Z80_State* state) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) exit(0);
    // Add keyboard input handling here
  }
}

void display_cleanup() {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

int WinMain(int argc, char* argv[]) {
  display_init();

  Z80_State z80_state;
  z80_init(&z80_state);

  const char* romName = "48.rom";
  const char* z80Name = "mm2000.z80";

  if (!load_rom(romName)) {
    display_cleanup();
    printf("Error: Unable to load ROM\n");
    return RETCODE_ROM_LOADING_FAILED;
  }

  if (!load_z80_snapshot(z80Name)) {
    display_cleanup();
    printf("Error: Unable to load Z80 Snapshot\n");
    return RETCODE_Z80_SNAPSHOT_LOADING_FAILED;
  }

  while (1) {
    z80_step(&z80_state);
    display_update(memory);
    input_handle(&z80_state);
  }

  display_cleanup();
  return RETCODE_NO_ERROR;
}