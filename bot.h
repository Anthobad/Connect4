#ifndef BOT_H
#define BOT_H
#define WINDOW_SIZE 4
#pragma once

#include "gamelogic.h"

int bot_choose_move(const Board* g);
int bot_choose_move_medium(const Board* g);
int pick_best_move(Board* g);
void zobrist_init();
void tt_init();
void shutdown_bot();

#endif
