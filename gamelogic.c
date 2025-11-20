#include "gamelogic.h"
#include <stdlib.h>
#include <string.h>

// static int count_connected_checkers(const Board* g, int r, int c, int rowDir, int colDir, char player) {
// 	int streak = 0;
// 	int temp_r = r;
// 	int temp_c = c;
// 	while (temp_r >= 0 && temp_r < ROWS && temp_c >= 0 && temp_c < COLS && g->cells[temp_r][temp_c] == player) {
// 		streak++;
// 		temp_r += rowDir;
// 		temp_c += colDir;
// 	}
// 	return streak;
// }
void setChar(Board* g, int r, int c, char player){
	uint64_t move = 1ULL << (r + c * 7);
	if (player == EMPTY) {
		/* clear the bit from mask and both player bitboards */
		g->mask &= ~move;
		g->playerA &= ~move;
		g->playerB &= ~move;
	} else {
		g->mask |= move;
		if (player == 'A')
			g->playerA |= move;
		else
			g->playerB |= move;
	}
}

char getChar(const Board* g, int r, int c){
	uint64_t bit = 1ULL << (r + c * 7);
	if(g->playerA & bit)
		return 'A';
	if(g->playerB & bit)
		return 'B';
	return EMPTY;
}

void initializeBoard(Board* g, char turn) {
	g->playerA = g->playerB = g->mask = 0ULL;
	g->current = turn;
}

// void setCurrent(Board* g, char turn){
// 	g->current = turn;
// }

// int game_in_bounds(int r, int c) {
// 	if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
// 		return 1;
// 	return 0;
// }
// the bitboard takes the LSB of the to be 5,0 spot in the 2d array, and left bit is the one below and so on umtil the highest row.
// then the 7th bit from the end (LSB) is the ROW5 , COL1.
// there is an extra bit in every col for spacing. instead of 6 bits there is 7 bits.
int game_can_drop(const Board* g, int col) {
	if (col < 0 || col >= COLS)
		return -1;
	for (int r = ROWS - 1; r >= 0; r--) {
		uint64_t bit = 1ULL << (r + col * 7);
		if (!(g->mask & bit))
			return r;
	}
	return -1;
}

int game_drop(Board* g, int col, char player) {
	int landing = game_can_drop(g, col);
	if (landing == -1)
		return -1;

	uint64_t move = 1ULL << (landing + col * 7);
	g->mask |= move;

	if (player == 'A')
		g->playerA |= move;
	else
		g->playerB |= move;

	/* Do not switch g->current here. Controller manages turn switching. */
	return landing;
}
// ...-0000000-0000000-0100001
// ...-0000000-0100001-0000000
// ...-0100001-0000000-0000000
int checkWin(const Board* g, char player) {
	uint64_t bb;
	if(player == 'A')
		bb = g->playerA;
	else
		bb = g->playerB;
	uint64_t m;
	//horizontal check acc to bitboard
	m = bb & (bb >> 7);
	if(m & (m >> 14))
		return 1;
	// / check acc to bitboard
	m = bb & (bb >> 6);
	if(m & (m >> 12))
		return 1;
	// \ check acc to bitboard
	m = bb & (bb >> 8);
	if(m & (m >> 16))
		return 1;
	//vertical check acc to bitboard
	m = bb & (bb >> 1);
	if(m & (m >> 2))
		return 1;
	
	return 0;
}

int checkDraw(const Board* g) {
	return g->mask == ((1ULL << 49) - 1ULL);
}

// void board_to_string(const Board* g, char out[ROWS*COLS + 1]) {
// 	int k = 0;
// 	for (int r = 0; r < ROWS; r++) {
// 		for (int c = 0; c < COLS; c++) {
// 			out[k++] = g->cells[r][c];
// 		}
// 	}
// 	out[k] = '\0';
// }

// void string_to_board(Board* g, const char* s) {
// 	int k = 0;
// 	for (int r = 0; r < ROWS; r++) {
// 		for (int c = 0; c < COLS; c++) {
// 			g->cells[r][c] = s[k++];
// 		}
// 	}
// }
