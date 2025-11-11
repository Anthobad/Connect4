#include "gamelogic.h"
#include <stdlib.h>
#include <string.h>

static int count_connected_checkers(const Board* g, int r, int c, int rowDir, int colDir, char player) {
	int streak = 0;
	int temp_r = r;
	int temp_c = c;
	while (temp_r >= 0 && temp_r < ROWS && temp_c >= 0 && temp_c < COLS && g->cells[temp_r][temp_c] == player) {
		streak++;
		temp_r += rowDir;
		temp_c += colDir;
	}
	return streak;
}

void initializeBoard(Board* g, char turn) {
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			g->cells[r][c] = EMPTY;
		}
	}
	setCurrent(g, turn);
}

void setCurrent(Board* g, char turn){
	g->current = turn;
}

int game_in_bounds(int r, int c) {
	if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
		return 1;
	return 0;
}

int game_can_drop(const Board* g, int col) {
	if (col < 0 || col >= COLS)
		return -1;
	for (int r = ROWS - 1; r >= 0; r--) {
		if (g->cells[r][col] == EMPTY)
			return r;
	}
	return -1;
}

int game_drop(Board* g, int col, char player) {
	int row = game_can_drop(g, col);
	if (row == -1)
		return -1;
	g->cells[row][col] = player;
	return row;
}

int checkWin(const Board* g, int r, int c, char player) {
	const int dirs[4][2] = { {0,1}, {1,0}, {1,1}, {-1,1} };
	for (int i = 0; i < 4; i++) {
		int rowDir = dirs[i][0];
		int colDir = dirs[i][1];
		int total = count_connected_checkers(g, r, c, rowDir, colDir, player)
		          + count_connected_checkers(g, r, c, -rowDir, -colDir, player) - 1;
		if (total >= 4)
			return 1;
	}
	return 0;
}

int checkDraw(const Board* g) {
	for (int c = 0; c < COLS; c++) {
		if (g->cells[0][c] == EMPTY)
			return 0;
	}
	return 1;
}

void board_to_string(const Board* g, char out[ROWS*COLS + 1]) {
	int k = 0;
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			out[k++] = g->cells[r][c];
		}
	}
	out[k] = '\0';
}

void string_to_board(Board* g, const char* s) {
	int k = 0;
	for (int r = 0; r < ROWS; r++) {
		for (int c = 0; c < COLS; c++) {
			g->cells[r][c] = s[k++];
		}
	}
}