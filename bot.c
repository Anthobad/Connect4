#include "gamelogic.h"
#include "bot.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#define TT_SIZE (1<<22)  // ~100 MB persistent TT 

// --- Constants ---
static const int column_order[7] = {3,4,2,5,1,6,0};

// --- Bit helpers ---
static inline uint64_t column_mask(int col) {
	return (((uint64_t)1 << 6) - 1) << (col * 7);
}

static inline int can_play(const Board* b, int col) {
	return (b->mask & (1ULL << (col * 7 + 5))) == 0;
}

static inline uint64_t make_move_bit(const Board* b, int col) {
	uint64_t bottom = (1ULL << (col * 7));
	uint64_t colmask = column_mask(col);
	return (b->mask + bottom) & colmask;
}

static inline void apply_move(Board* b, int col) {
	uint64_t mv = make_move_bit(b, col);
	if (b->current == 'A') b->playerA ^= mv;
	else b->playerB ^= mv;
	b->mask |= mv;
}

static inline void undo_move(Board* b, int col) {
	uint64_t colm = column_mask(col) & b->mask;
	for(int r = ROWS; r >= 0; r--) {
		int bit = col * 7 + r;
		uint64_t m = (1ULL << bit);
		if (b->mask & m) {
			b->mask ^= m;
			if (b->playerA & m)
				b->playerA ^= m;
			else if (b->playerB & m)
				b->playerB ^= m;
			b->current = (b->current == 'A') ? 'B' : 'A';
			return;
		}
	}
}

//static inline int bitboard_win(uint64_t bb) {
//	uint64_t m;
//	m = bb & (bb >> 7);
//	if (m&(m>>14)) return 1;
//	m=bb&(bb>>6);
//	if (m&(m>>12)) return 1;
//	m=bb&(bb>>8);
//	if (m&(m>>16)) return 1;
//	m=bb&(bb>>1);
//	if (m&(m>>2)) return 1;
//	return 0;
//}

// --- Zobrist & TT ---
static uint64_t zobrist[2][42];

static uint64_t zobrist_side;

typedef enum { EXACT, LOWERBOUND, UPPERBOUND } TTFlag;

typedef struct {
	uint64_t key;
	int value;
	int depth;
	unsigned char best_move;
	unsigned char flag;
} TTEntry;

static TTEntry* tt = NULL;

static void zobrist_init() {
	srand((unsigned)time(NULL));
	for(int p = 0; p < 2; p++) {
		for(int i = 0; i < 42; i++) {
			zobrist[p][i] = rand()*UINT64_MAX;
			zobrist_side=rand()*UINT64_MAX;
		}
	}
}

static void tt_init() {
	tt = calloc(TT_SIZE,sizeof(TTEntry)); // Try to load persistent TT
	FILE* f = fopen("tt.bin","rb");
	if(f) {
		fread(tt,sizeof(TTEntry),TT_SIZE,f);
		fclose(f);
	}
}

static void tt_save() {
	if(!tt) return;
	FILE* f = fopen("tt.bin","wb");
	if(f) {
		fwrite(tt,sizeof(TTEntry),TT_SIZE,f);
		fclose(f);
	}
}

static void tt_store(uint64_t key, int value, int depth, TTFlag flag, int best_move) {
	size_t idx = key & (TT_SIZE - 1);
	TTEntry* e =& tt[idx];
	if(e->key == 0 || e->depth <= depth) {
		e->key = key;
		e->value = value;
		e->depth = depth;
		e->best_move = (best_move < 0) ? 255 : (unsigned char)best_move;
		e->flag = (unsigned char)flag;
	}
}

static int tt_probe(uint64_t key, int depth, int alpha, int beta, int* out_value, int* out_move) {
	size_t idx = key & (TT_SIZE - 1);
	TTEntry* e = &tt[idx];
	if(e->key == key) {
		if(e->depth >= depth) {
			if(e->flag == EXACT) {
				*out_value = e->value;
				if(out_move) *out_move = (e->best_move == 255) ? -1 : e->best_move;
				return 1;
			}
			if(e->flag == LOWERBOUND && e->value > alpha) alpha = e->value;
			if(e->flag == UPPERBOUND && e->value < beta) beta = e->value;
			if(alpha >= beta) {
				*out_value = e->value;
				if(out_move) *out_move = (e->best_move == 255) ? -1 : e->best_move;
				return 1;
			}
		}
		else {
			if(out_move) *out_move = (e->best_move == 255) ? -1 : e->best_move;
		}
	}
	return 0;
}

// --- Mate-distance scoring ---
static const int MATE = 10000000;

static inline int encode_win(int ply) {
	return MATE - ply;
}

static inline int encode_loss(int ply) {
	return -MATE + ply;
}

// --- Negamax solver with TT ---
static int negamax_solve(Board* b, int alpha, int beta, char side, int ply) {
	uint64_t key = 0;
	{
		uint64_t bb = b->playerA;
		while(bb) {
			int idx = __builtin_ctzll(bb);
			key ^= zobrist[0][idx];
			bb &= bb - 1;
		}
		bb= b->playerB;
		while(bb) {
			int idx = __builtin_ctzll(bb);
			key ^= zobrist[1][idx];
			bb &= bb - 1;
		}
		if(side == 'B') key ^= zobrist_side;
	}
	int tt_val;
	if(tt && tt_probe(key, 64, alpha, beta, &tt_val, NULL)) return tt_val;
	//uint64_t me = (side == 'A') ? b->playerA : b->playerB;
	//uint64_t opp = (side == 'A') ? b->playerB : b->playerA;
	char  me = (side == 'A') ? 'A' : 'B';
	char  opp = (side == 'A') ? 'B' : 'A';
	//if(bitboard_win(me)) return encode_win(ply);
	//if(bitboard_win(opp)) return encode_loss(ply);
	if(checkWin(b, me)) return encode_win(ply);
	if(checkWin(b, opp)) return encode_loss(ply);
	if((b->mask & 0x1FFFFFFFFFFFFFULL) == 0x1FFFFFFFFFFFFFULL) return 0;
	int moves[7];
	int n = 0;
	for(int i = 0; i < 7; i++) {
		int col = column_order[i];
		if(can_play(b,col)) moves[n++] = col;
	}
	if(n == 0) return 0;
	int best = -MATE;
	int best_move = -1;
	int alpha_orig = alpha;
	for(int i = 0; i < n; i++) {
		int col = moves[i];
		apply_move(b,col);
		int val = -negamax_solve(b, -beta, -alpha, (side == 'A') ? 'B' : 'A', ply+1);
		undo_move(b,col);
		if(val > best) {
			best = val;
			best_move = col;
		}
		if(best > alpha) alpha = best;
		if(alpha >= beta) break;
	}
	if(tt) tt_store(key, best, 64, EXACT, best_move);
	return best;
}

int solve_position(Board* b) {
	int res = negamax_solve(b, -MATE, MATE, b->current, 0);
	if(res > 0) return 1;
	if(res < 0) return -1;
	return 0;
}
const char* solve_str(Board* b) {
	int res = solve_position(b);
	if(res > 0)return "WIN for side to move";
	if(res < 0)return "LOSS for side to move";
	return "DRAW";
}

// --- Opening book ---
static const int opening_book[][2] = { {0,3}, {1,2}, {2,3}, {3,2}, {4,3}, {5,2}, {6,3}, {7,2} };
static int opening_book_size = 8;
int opening_book_move(Board* b, int ply) {
	if(ply >= opening_book_size) return -1;
	return opening_book[ply][1];
}

// --- Wrapper: automatic best move ---
int pick_best_move(Board* b) {
	int ply = __builtin_popcountll(b->mask);
	int move = opening_book_move(b, ply);
	if(move != -1) return move;
	int best = -1;
	int alpha = -MATE;
	int beta  = MATE;
	int best_val = -MATE;
	int moves[COLS];
	int n = 0;
	for(int i = 0; i < COLS; i++) if(can_play(b,i)) moves[n++] = i;
	for(int i = 0; i < n; i++) {
		int col = moves[i];
		apply_move(b,col);
		int val = -negamax_solve(b, -beta, -alpha, (b->current == 'A' ) ? 'B' : 'A', ply+1);
		undo_move(b,col);
		if(val > best_val) {
			best_val = val;
			best = col;
		}
		if(best_val > alpha) alpha = best_val;
	}

	return best;

}

// --- Call before exit to save TT ---
void shutdown_bot() {
	tt_save();
	free(tt);
}

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
