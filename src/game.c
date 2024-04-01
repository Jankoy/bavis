#include "game.h"

#include <stdio.h>

static size_t bavis_count = 0;

static void print_bavis_count() {
  printf("Bavis count: %zu\n", bavis_count);
}

void game_init() {
  printf("Welcome to Bavis!\n");
}

static void game_reset() {
  bavis_count = 0;
  game_init();
}

static bool game_is_over = false;

void game_over() {
  game_is_over = true;
  printf("Game over! Type \"retry\" to restart!\n");
}

void game_restart() {
  game_is_over = false;
  game_reset();
}

void game_update(String_View input) {
  if (!input.count)
    return;
  
  if (sv_eq(input, SV("retry"))) {
    game_restart();
    return;
  }
  
  if (game_is_over)
    return;
  
  if (sv_eq(input, SV("bavis"))) {
    ++bavis_count;
    print_bavis_count();
  } else if (sv_eq(input, SV("help"))) {
    printf("To play type \"bavis\". Type \"exit\" or \"quit\" to quit.\n");
  } else {
    game_over();
  }
}
