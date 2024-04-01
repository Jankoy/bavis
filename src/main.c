#include "game.h"

#define SV_IMPLEMENTATION
#include "sv.h"
#include <stdio.h>

#define INPUT_BUF_CAP 512
static char input_buffer[INPUT_BUF_CAP] = {0};

int main(void) {
  game_init();
  
  while (true) {
    const char* line = fgets(input_buffer, INPUT_BUF_CAP, stdin);
    if (!line)
      break;
    String_View sv = sv_trim(SV(line));
    if (sv_eq(sv, SV("exit")) || sv_eq(sv, SV("quit")))
      break;
    game_update(sv);
  }

  return 0;
}
