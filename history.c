#include "history.h"

static Move history[MAX_MOVES];
static int move_count = 0;
static int current_index = 0;

void history_reset(void) {
	move_count = 0;
	current_index = 0;
}

void history_record_move(int row, int col, char player) {
	if (current_index < move_count) {
		move_count = current_index;
	}
	if (move_count < MAX_MOVES) {
		history[move_count].row = row;
		history[move_count].col = col;
		history[move_count].player = player;
		move_count++;
		current_index = move_count;
	}
}

int history_undo(Board* G, int steps) {
	if (current_index == 0)
		return 0;

	for (int i = 0; i < steps; i++) {
		if (current_index == 0)
			return i > 0;
		current_index--;
		Move m = history[current_index];
		G->cells[m.row][m.col] = EMPTY;
		G->current = m.player;
	}
	return 1;
}

int history_redo(Board* G, int steps) {
	if (current_index == move_count)
		return 0;

	Move m = history[current_index];
	for (int i = 0; i < steps; i++) {
		if (current_index == move_count)
			return i > 0;
		m = history[current_index];
		G->cells[m.row][m.col] = m.player;
		current_index++;
	}

	if (m.player == 'A')
		G->current = 'B';
	else
		G->current = 'A';

	return 1;
}