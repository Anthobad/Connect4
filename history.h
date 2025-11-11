#ifndef HISTORY_H
#define HISTORY_H

#include "gamelogic.h"

#define MAX_MOVES (ROWS * COLS)

typedef struct {
	int row;
	int col;
	char player;
} Move;

void history_reset(void);
void history_record_move(int row, int col, char player);
int history_undo(Board* G, int steps);
int history_redo(Board* G, int steps);

#endif