/* main.c */
#include "zx_spectrum.h"
#include <stdio.h>

enum RETURN_CODES
{
  RETCODE_NO_ERROR = 0,
  RETCODE_ROM_LOADING_FAILED
};

void display_update() {}

void input_handle() {}

int main() {
  bool romLoaded = load_rom("48.rom");
  if(romLoaded==false)
  {
    printf("Error: Unable to load 48.rom, exiting\n");
    return RETCODE_ROM_LOADING_FAILED;
  }

  cpu_init();
  while (1) {
    cpu_step();
    display_update();
    input_handle();
  }
  return RETCODE_NO_ERROR;
}
