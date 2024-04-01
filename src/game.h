#ifndef GAME_H_
#define GAME_H_

#include "sv.h"

void game_init();

void game_over();
void game_restart();

void game_update(String_View input);

#endif // GAME_H_
