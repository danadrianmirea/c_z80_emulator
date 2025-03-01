/* main.c */
#include "zx_spectrum.h"
#include <stdio.h>


void display_update() {}

void input_handle() {}

int main() {
  Z80_State z80_state;
  z80_init(&z80_state);

  const char* romName = "48.rom";
  //const char* z80Name = "deathchase.z80";
  const char* z80Name = "mm2000.z80";

  if (!load_rom(romName))
  {
    printf("Error: Unable to load rom, exiting\n");
    return RETCODE_ROM_LOADING_FAILED;
  }

  if (!load_z80_snapshot(z80Name)) {
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