#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#pragma once

#include <stdio.h>
#include <stdint.h>

#define ROWS 6
#define COLS 7
#define EMPTY '.'

typedef struct {
	uint64_t playerA;
	uint64_t playerB;
	uint64_t mask;
	char current;
} Board;

void setChar(Board* g, int r, int c, char player);
char getChar(const Board* g, int r, int c);
void initializeBoard(Board* g, char turn);
// void setCurrent(Board* g, char turn);
// int game_in_bounds(int r, int c);
int game_can_drop(const Board* g, int col);
int game_drop(Board* g, int col, char player);
int checkWin(const Board* g, char player);
int checkDraw(const Board* g);
// void board_to_string(const Board* g, char out[ROWS*COLS + 1]);
// void string_to_board(Board* g, const char* s);

#endif // GAMELOGIC_H