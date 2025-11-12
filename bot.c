#include "bot.h"
#include <stdlib.h>

int bot_choose_move(const Board* g) {
	int valid[COLS];
	int n = 0;
	for (int c = 0; c < COLS; c++) {
		if (getChar(g, 0, c) == EMPTY) {
			valid[n++] = c;
		}
	}
	if (n == 0)
		return -1;
	int i = rand() % n;
	return valid[i];
}

int bot_choose_move_medium(const Board* g) { //Time complexity of this algorith is O(ROWS * COLS) + O(COLS) worst case, O(ROWS * COLS) average case
	int blocking_cols[COLS];
	int num_blocking = 0;
	char human = 'A';

	for (int c = 0; c < COLS; c++) { //O(COLS)
		int r = game_can_drop(g, c); //O(ROWS)
		if (r == -1) {
			continue;
		}
		Board temp = *g;
		setChar(&temp, r, c, human);
		if (checkWin(&temp, human)) {
			blocking_cols[num_blocking++] = c;
		}
	}

	if (num_blocking > 0) {
		int idx = rand() % num_blocking;
		return blocking_cols[idx];
	}

	return bot_choose_move(g); //O(COLS)
}