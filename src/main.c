/* main.c */
#include "zx_spectrum.h"
#include <stdio.h>

int main() {
  load_rom("48.rom");
  cpu_init();
  while (1) {
    cpu_step();
    display_update();
    input_handle();
  }
  return 0;
}
