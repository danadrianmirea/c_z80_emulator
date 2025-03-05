/* main.c */
#include "z80.h"
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <time.h>

#include "zx_spectrum.h"
#include "z80.h"
#include "loader.h"
#include "memory.h"

//#define DEBUG
#define DEBUG_TICK_SPEED
#define LOGGING_INTERVAL_FAST  100
#define LOGGING_INTERVAL_MID   500
#define LOGGING_INTERVAL_SLOW 1000


extern uint8_t memory[MEM_SIZE];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* texture = NULL;
uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

// Spectrum color palette (RGB888)
const uint32_t palette[16] = { 0xFF000000, 0xFF0000D7, 0xFFD70000,
                              0xFFD700D7, // Black, Blue, Red, Magenta
                              0xFF00D700, 0xFF00D7D7, 0xFFD7D700,
                              0xFFD7D7D7, // Green, Cyan, Yellow, White
                              0xFF000000, 0xFF0000FF, 0xFFFF0000,
                              0xFFFF00FF, // Bright variants
                              0xFF00FF00, 0xFF00FFFF, 0xFFFFFF00, 0xFFFFFFFF };

void display_init() {
  SDL_Init(SDL_INIT_VIDEO);
  window =
    SDL_CreateWindow("ZX Spectrum Emulator", SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH * SCALE_FACTOR,
      SCREEN_HEIGHT * SCALE_FACTOR, SDL_WINDOW_SHOWN);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH,
    SCREEN_HEIGHT);
}

void display_update(uint8_t* memory) {
  static uint32_t flash_counter = 0;
  flash_counter++;

  // Convert Spectrum screen memory to pixels
  for (int y = 0; y < SCREEN_HEIGHT; y++) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
      // Calculate screen memory address
      uint16_t addr = 0x4000 | ((y & 0b11000000) << 5) |
        ((y & 0b00111000) << 2) | ((y & 0b00000111) << 8) |
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
      if (bright) {
        ink += 8;
        paper += 8;
      }
      if (flash) {
        uint8_t temp = ink;
        ink = paper;
        paper = temp;
      }

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

// Helper function to convert SDL scancode to ZX Spectrum key value
uint8_t sdl_scancode_to_zx_key(uint8_t scancode) {
  // Implement the conversion logic here
  static const uint8_t keymap[] = {
      [SDL_SCANCODE_A] = 0x01,[SDL_SCANCODE_B] = 0x02,
      [SDL_SCANCODE_C] = 0x03,[SDL_SCANCODE_D] = 0x04,
      [SDL_SCANCODE_E] = 0x05,[SDL_SCANCODE_F] = 0x06,
      [SDL_SCANCODE_G] = 0x07,[SDL_SCANCODE_H] = 0x08,
      [SDL_SCANCODE_I] = 0x09,[SDL_SCANCODE_J] = 0x0A,
      [SDL_SCANCODE_K] = 0x0B,[SDL_SCANCODE_L] = 0x0C,
      [SDL_SCANCODE_M] = 0x0D,[SDL_SCANCODE_N] = 0x0E,
      [SDL_SCANCODE_O] = 0x0F,[SDL_SCANCODE_P] = 0x10,
      [SDL_SCANCODE_Q] = 0x11,[SDL_SCANCODE_R] = 0x12,
      [SDL_SCANCODE_S] = 0x13,[SDL_SCANCODE_T] = 0x14,
      [SDL_SCANCODE_U] = 0x15,[SDL_SCANCODE_V] = 0x16,
      [SDL_SCANCODE_W] = 0x17,[SDL_SCANCODE_X] = 0x18,
      [SDL_SCANCODE_Y] = 0x19,[SDL_SCANCODE_Z] = 0x1A,
      [SDL_SCANCODE_1] = 0x1B,[SDL_SCANCODE_2] = 0x1C,
      [SDL_SCANCODE_3] = 0x1D,[SDL_SCANCODE_4] = 0x1E,
      [SDL_SCANCODE_5] = 0x1F,[SDL_SCANCODE_6] = 0x20,
      [SDL_SCANCODE_7] = 0x21,[SDL_SCANCODE_8] = 0x22,
      [SDL_SCANCODE_9] = 0x23,[SDL_SCANCODE_0] = 0x24,
      [SDL_SCANCODE_RETURN] = 0x25,[SDL_SCANCODE_SPACE] = 0x26,
      [SDL_SCANCODE_LSHIFT] = 0x27,[SDL_SCANCODE_RSHIFT] = 0x28,
      [SDL_SCANCODE_CAPSLOCK] = 0x29,[SDL_SCANCODE_TAB] = 0x2A };

  return keymap[scancode];
}

// Helper function to convert ZX Spectrum key value to row and column
uint8_t zx_key_to_row(uint8_t key_value) {
  static const uint8_t rowmap[] = {
      [0x01] = 0x00,[0x02] = 0x01,[0x03] = 0x02,[0x04] = 0x03,[0x05] = 0x04,
      [0x06] = 0x05,[0x07] = 0x06,[0x08] = 0x07,[0x09] = 0x08,[0x0A] = 0x09,
      [0x0B] = 0x0A,[0x0C] = 0x0B,[0x0D] = 0x0C,[0x0E] = 0x0D,[0x0F] = 0x0E,
      [0x10] = 0x0F,[0x11] = 0x10,[0x12] = 0x11,[0x13] = 0x12,[0x14] = 0x13,
      [0x15] = 0x14,[0x16] = 0x15,[0x17] = 0x16,[0x18] = 0x17,[0x19] = 0x18,
      [0x1A] = 0x19,[0x1B] = 0x1A,[0x1C] = 0x1B,[0x1D] = 0x1C,[0x1E] = 0x1D,
      [0x1F] = 0x1E,[0x20] = 0x1F,[0x21] = 0x20,[0x22] = 0x21,[0x23] = 0x22,
      [0x24] = 0x23,[0x25] = 0x24,[0x26] = 0x25,[0x27] = 0x26,[0x28] = 0x27,
      [0x29] = 0x28,[0x2A] = 0x29 };
  return rowmap[key_value];
}

uint8_t zx_key_to_col(uint8_t key_value) {
  switch (key_value) {
  case 0x01:
    return 0x00;
  case 0x02:
    return 0x01;
  case 0x03:
    return 0x02;
  case 0x04:
    return 0x03;
  case 0x05:
    return 0x04;
  case 0x06:
    return 0x05;
  case 0x07:
    return 0x06;
  case 0x08:
    return 0x07;
  case 0x09:
    return 0x08;
  case 0x0A:
    return 0x09;
  case 0x0B:
    return 0x0A;
  case 0x0C:
    return 0x0B;
  case 0x0D:
    return 0x0C;
  case 0x0E:
    return 0x0D;
  case 0x0F:
    return 0x0E;
  case 0x10:
    return 0x0F;
  case 0x11:
    return 0x10;
  case 0x12:
    return 0x11;
  case 0x13:
    return 0x12;
  case 0x14:
    return 0x13;
  case 0x15:
    return 0x14;
  case 0x16:
    return 0x15;
  case 0x17:
    return 0x16;
  case 0x18:
    return 0x17;
  case 0x19:
    return 0x18;
  case 0x1A:
    return 0x19;
  case 0x1B:
    return 0x1A;
  case 0x1C:
    return 0x1B;
  case 0x1D:
    return 0x1C;
  case 0x1E:
    return 0x1D;
  case 0x1F:
    return 0x1E;
  case 0x20:
    return 0x1F;
  case 0x21:
    return 0x20;
  case 0x22:
    return 0x21;
  case 0x23:
    return 0x22;
  case 0x24:
    return 0x23;
  case 0x25:
    return 0x24;
  case 0x26:
    return 0x25;
  case 0x27:
    return 0x26;
  case 0x28:
    return 0x27;
  case 0x29:
    return 0x28;
  case 0x2A:
    return 0x29;
  default:
    return 0;
  }
}

void input_handle(Z80_State* state) {
  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT)
      exit(0);
    // Add keyboard input handling here
    if (e.type == SDL_KEYDOWN) {
      uint8_t scancode = e.key.keysym.scancode;
      uint8_t key_value = sdl_scancode_to_zx_key(scancode);
      if (key_value != 0) {
        uint8_t row = zx_key_to_row(key_value);
        uint8_t col = zx_key_to_col(key_value);
        memory[0x5c00 + row] |= (1 << col);
      }
    }
  }
}

void display_cleanup() {
  SDL_DestroyTexture(texture);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void perform_sleep() {
  SDL_Delay(2); // TODO: rework this to a more accurate sleep
}

void print_usage(const char* program_name) {
  printf("ZX Spectrum Emulator\n");
  printf("Usage: %s <rom_file> <z80_snapshot>\n\n", program_name);
  printf("Example: %s 48.rom game.z80\n", program_name);
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    print_usage(argv[0]);
    return RETCODE_INVALID_ARGUMENTS;
  }

  display_init();

  Z80_State z80_state;
  z80_init(&z80_state);

  const char* romName = "48.rom";
  const char* snapshotName = argv[1];

  if (!load_rom(romName)) {
    display_cleanup();
    printf("Error: Unable to load ROM\n");
    return RETCODE_ROM_LOADING_FAILED;
  }

  char* snapshotExt = strrchr(snapshotName, '.');
  if (snapshotExt && (strcmp(snapshotExt, ".z80") == 0)) {
    if (!load_z80_snapshot(snapshotName, &z80_state)) {
      display_cleanup();
      printf("Error: Unable to load Z80 Snapshot\n");
      return RETCODE_Z80_SNAPSHOT_LOADING_FAILED;
    }
  } else if (snapshotExt && (strcmp(snapshotExt, ".sna") == 0)) {
    if (!load_sna(snapshotName, &z80_state)) {
      display_cleanup();
      printf("Error: Unable to load SNA snapshot\n");
      return RETCODE_Z80_SNAPSHOT_LOADING_FAILED;
    }
  } else {
    display_cleanup();
    printf("Error: Unknown snapshot format (%s)\n", snapshotExt);
    return RETCODE_Z80_SNAPSHOT_LOADING_FAILED;
  }

  while (1) {
    input_handle(&z80_state);
    display_update(memory);
    z80_step(&z80_state);
    //perform_sleep();
  }

  display_cleanup();
  return RETCODE_NO_ERROR;
}