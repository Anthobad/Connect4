#include "bot.h"
#include <stdlib.h>

int bot_choose_move(const Board* g) {
	int valid[COLS];
	int n = 0;
	for (int c = 0; c < COLS; c++) {
		if (g->cells[0][c] == EMPTY) {
			valid[n++] = c;
		}
	}
	if (n == 0)
		return -1;
	int i = rand() % n;
	return valid[i];
}

int bot_choose_move_medium(const Board* g) {
	int blocking_cols[COLS];
	int num_blocking = 0;
	char human = 'A';

	for (int c = 0; c < COLS; c++) {
		int r = game_can_drop(g, c);
		if (r == -1) {
			continue;
		}
		Board temp = *g;
		temp.cells[r][c] = human;
		if (checkWin(&temp, r, c, human)) {
			blocking_cols[num_blocking++] = c;
		}
	}

	if (num_blocking > 0) {
		int idx = rand() % num_blocking;
		return blocking_cols[idx];
	}

	return bot_choose_move(g);
}