/* main.c */
#include "zx_spectrum.h"
#include <stdio.h>

enum RETURN_CODES
{
  RETCODE_NO_ERROR = 0,
  RETCODE_ROM_LOADING_FAILED,
  RETCODE_Z80_SNAPSHOT_LOADING_FAILED
};

void display_update() {}

void input_handle() {}

int main() {
  Z80_State z80_state;
  z80_init(&z80_state);

  if (!load_rom("48.rom"))
  {
    printf("Error: Unable to load rom, exiting\n");
    return RETCODE_ROM_LOADING_FAILED;
  }

  if (!load_z80_snapshot("MM-2000.z80")) {
    printf("Error: Unable to load Z80 Snapshot, exiting\n");
    return RETCODE_Z80_SNAPSHOT_LOADING_FAILED;
  }

  while (1) {
    z80_step(&z80_state);
    display_update();
    input_handle();
  }
  return RETCODE_NO_ERROR;
}