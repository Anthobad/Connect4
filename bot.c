#include "gamelogic.h"
#include "bot.h"
#include "history.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// -----------------------------------------------------------------------------
// CONFIG
// -----------------------------------------------------------------------------

// Transposition table size: 2^22 entries (~100MB) – you can raise to 1<<24 (~400MB) if you want
#define TT_SIZE      (1 << 22)

// Depth for *interactive* hard bot (pick_best_move).
// 14–20 is usually a good balance; raise if it's fast enough on your machine.
#define MAX_DEPTH    14

// Depth for solve_position (offline solving / analysis).
// This is a hard upper bound; the search will usually terminate earlier
// when the game ends before depth runs out.
#define SOLVE_DEPTH  42

// Verbose logging: set to 1 if you want detailed console output, 0 for speed
#define BOT_VERBOSE  0

// Max threads (only used if OpenMP is enabled)
#define BOT_MAX_THREADS 6

// -----------------------------------------------------------------------------
// CONSTANTS & GLOBALS
// -----------------------------------------------------------------------------

static const int column_order[7] = {3, 4, 2, 5, 1, 6, 0};  // center-first ordering

// Fast "mate" scores
static const int MATE = 10000000;

// Node / TT stats (for debugging / info). These are accumulated totals.
static unsigned long long nodes_searched = 0;
static unsigned long long tt_hits        = 0;

// Zobrist hashing
static uint64_t zobrist[2][42];   // [playerIndex][squareIndex]
static uint64_t zobrist_side;     // side-to-move key

typedef enum { EXACT, LOWERBOUND, UPPERBOUND } TTFlag;

typedef struct {
    uint64_t      key;
    int           value;
    int           depth;
    unsigned char best_move;
    unsigned char flag;
} TTEntry;

static TTEntry* tt = NULL;

// -----------------------------------------------------------------------------
// BITBOARD & MOVE HELPERS
// -----------------------------------------------------------------------------

// Fast bitboard win check on 7x6 board with 7 bits per column
static inline int bitboard_win(uint64_t bb) {
    uint64_t m;

    // Horizontal (shift by 7)
    m = bb & (bb >> 7);
    if (m & (m >> 14)) return 1;

    // Diagonal "/" (shift by 6)
    m = bb & (bb >> 6);
    if (m & (m >> 12)) return 1;

    // Diagonal "\" (shift by 8)
    m = bb & (bb >> 8);
    if (m & (m >> 16)) return 1;

    // Vertical (shift by 1)
    m = bb & (bb >> 1);
    if (m & (m >> 2)) return 1;

    return 0;
}

// Initialize heights[] from a Board using the normal game logic
static void init_heights(const Board* b, int heights[COLS]) {
    for (int c = 0; c < COLS; c++) {
        int r = game_can_drop(b, c);   // -1 if full, else landing row
        heights[c] = r;
    }
}

// Can we play in this column? (heights-based)
static inline int can_play(const Board* b, const int heights[COLS], int col) {
    (void)b;                 // unused; we rely on heights[]
    return (col >= 0 && col < COLS && heights[col] >= 0);
}

// Bit for the next move in column col (based on heights[]).
static inline uint64_t move_bit_for_col(const int heights[COLS], int col) {
    int landing = heights[col];
    if (landing < 0) return 0ULL;
    return 1ULL << (landing + col * 7);
}

// Apply move for 'side' in column col, updating bitboards, mask, and heights
static inline void apply_move(Board* b, int heights[COLS], int col, char side) {
    uint64_t mv = move_bit_for_col(heights, col);
    if (!mv) return;  // safety: column full

    if (side == 'A') b->playerA ^= mv;
    else             b->playerB ^= mv;
    b->mask |= mv;

    // After placing a piece, the next free row is one above
    heights[col]--;
}

// Undo last move in column col, using heights[] to know which bit to clear
static inline void undo_move(Board* b, int heights[COLS], int col) {
    // We always undo the last move placed in this column: increment height
    heights[col]++;
    int row = heights[col];
    if (row < 0 || row >= ROWS) return;  // safety

    uint64_t m = 1ULL << (row + col * 7);
    b->mask ^= m;
    if (b->playerA & m) b->playerA ^= m;
    else if (b->playerB & m) b->playerB ^= m;
}

// -----------------------------------------------------------------------------
// ZOBRIST & TT
// -----------------------------------------------------------------------------

void zobrist_init() {
    srand((unsigned)time(NULL));
    for (int p = 0; p < 2; p++) {
        for (int i = 0; i < 42; i++) {
            // 64-bit random
            uint64_t hi  = (uint64_t)(rand() & 0xFFFF);
            uint64_t mid = (uint64_t)(rand() & 0xFFFF);
            uint64_t lo  = (uint64_t)(rand() & 0xFFFF);
            zobrist[p][i] = (hi << 48) ^ (mid << 32) ^ lo;
        }
    }
    {
        uint64_t hi  = (uint64_t)(rand() & 0xFFFF);
        uint64_t mid = (uint64_t)(rand() & 0xFFFF);
        uint64_t lo  = (uint64_t)(rand() & 0xFFFF);
        zobrist_side = (hi << 48) ^ (mid << 32) ^ lo;
    }
}

void tt_init() {
    tt = (TTEntry*)calloc(TT_SIZE, sizeof(TTEntry));
    if (!tt) {
        fprintf(stderr, "ERROR: Failed to allocate transposition table\n");
        exit(1);
    }

    // Try to load persistent TT (optional)
    FILE* f = fopen("tt.bin", "rb");
    if (f) {
        size_t read = fread(tt, sizeof(TTEntry), TT_SIZE, f);
#if BOT_VERBOSE
        printf("Loaded %zu TT entries from disk\n", read);
#endif
        fclose(f);
    }
}

static void tt_save() {
    if (!tt) return;
    FILE* f = fopen("tt.bin", "wb");
    if (f) {
        fwrite(tt, sizeof(TTEntry), TT_SIZE, f);
        fclose(f);
#if BOT_VERBOSE
        printf("Saved TT to disk\n");
#endif
    }
}

static inline void tt_store(uint64_t key, int value, int depth, TTFlag flag, int best_move) {
    size_t idx   = key & (TT_SIZE - 1);
    TTEntry* e   = &tt[idx];

    if (e->key == 0 || e->depth <= depth) {
        e->key       = key;
        e->value     = value;
        e->depth     = depth;
        e->best_move = (best_move < 0) ? 255 : (unsigned char)best_move;
        e->flag      = (unsigned char)flag;
    }
}

static inline int tt_probe(uint64_t key, int depth, int alpha, int beta,
                           int* out_value, int* out_move) {
    size_t idx = key & (TT_SIZE - 1);
    TTEntry* e = &tt[idx];

    if (e->key != key) return 0;

    if (out_move && e->best_move != 255) {
        *out_move = e->best_move;
    }

    if (e->depth < depth) {
        // Stored result is from a shallower search – still use move hint, but no bound
        return 0;
    }

    int val = e->value;
    if (e->flag == EXACT) {
        *out_value = val;
        return 1;
    } else if (e->flag == LOWERBOUND) {
        if (val > alpha) alpha = val;
    } else if (e->flag == UPPERBOUND) {
        if (val < beta) beta = val;
    }
    if (alpha >= beta) {
        *out_value = val;
        return 1;
    }

    return 0;
}

// Compute Zobrist key from scratch (used only at root)
static uint64_t compute_key(const Board* b, char side) {
    uint64_t key = 0;
    uint64_t bb;

    bb = b->playerA;
    while (bb) {
        int idx = __builtin_ctzll(bb);
        key ^= zobrist[0][idx];
        bb &= bb - 1;
    }

    bb = b->playerB;
    while (bb) {
        int idx = __builtin_ctzll(bb);
        key ^= zobrist[1][idx];
        bb &= bb - 1;
    }

    if (side == 'B') key ^= zobrist_side;
    return key;
}

// -----------------------------------------------------------------------------
// SCORING HELPERS
// -----------------------------------------------------------------------------

static inline int encode_win(int ply)  { return  MATE - ply; }
static inline int encode_loss(int ply) { return -MATE + ply; }

// Human-friendly text for scores (used in debug prints)
static const char* score_to_string(int score, int ply) {
    static char buf[64];
    if (score > MATE - 1000) {
        int moves_to_win = (MATE - score + ply + 1) / 2;
        snprintf(buf, sizeof(buf), "WIN in %d", moves_to_win);
    } else if (score < -MATE + 1000) {
        int moves_to_loss = (MATE + score - ply + 1) / 2;
        snprintf(buf, sizeof(buf), "LOSS in %d", moves_to_loss);
    } else if (score == 0) {
        snprintf(buf, sizeof(buf), "DRAW");
    } else {
        snprintf(buf, sizeof(buf), "Score %+d", score);
    }
    return buf;
}

// -----------------------------------------------------------------------------
// SIMPLE POSITIONAL EVALUATION (for depth limit)
// -----------------------------------------------------------------------------

// Evaluate a 4-cell window in direction (dr, dc) starting at (r,c)
static int eval_window(const Board* b, int r, int c, int dr, int dc, char side) {
    char opp = (side == 'A') ? 'B' : 'A';
    int me = 0, them = 0, empty = 0;

    for (int i = 0; i < 4; i++) {
        char ch = getChar(b, r + i*dr, c + i*dc);
        if (ch == side) me++;
        else if (ch == opp) them++;
        else empty++;
    }

    // Both colors present: blocked
    if (me > 0 && them > 0) return 0;

    int score = 0;

    // Favor our threats
    if (me == 4)                  score += 100000;
    else if (me == 3 && empty==1) score += 100;
    else if (me == 2 && empty==2) score += 10;
    else if (me == 1 && empty==3) score += 1;

    // Penalize opponent's almost-4
    if (them == 3 && empty == 1) score -= 120;

    return score;
}

// Basic heuristic evaluation from the POV of 'side'
static int evaluate(const Board* b, char side) {
    int score = 0;

    // Center column preference (encourage occupying column 3)
    for (int r = 0; r < ROWS; r++) {
        char ch = getChar(b, r, 3);
        if (ch == side) score += 3;
        else if (ch != EMPTY) score -= 3;
    }

    // Horizontal windows
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            score += eval_window(b, r, c, 0, 1, side);
        }
    }

    // Vertical
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r <= ROWS - 4; r++) {
            score += eval_window(b, r, c, 1, 0, side);
        }
    }

    // Diagonal "\"
    for (int r = 0; r <= ROWS - 4; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            score += eval_window(b, r, c, 1, 1, side);
        }
    }

    // Diagonal "/"
    for (int r = 3; r < ROWS; r++) {
        for (int c = 0; c <= COLS - 4; c++) {
            score += eval_window(b, r, c, -1, 1, side);
        }
    }

    return score;
}

// -----------------------------------------------------------------------------
// NEGAMAX + TT
// -----------------------------------------------------------------------------

static int negamax_solve(Board* b,
                         int alpha, int beta,
                         char side, int ply, int depth,
                         uint64_t key,
                         int heights[COLS],
                         unsigned long long* nodes_count,
                         unsigned long long* tt_hits_count)
{
    (*nodes_count)++;

    // Transposition table probe
    int tt_val;
    int tt_move = -1;
    if (tt && tt_probe(key, depth, alpha, beta, &tt_val, &tt_move)) {
        (*tt_hits_count)++;
        return tt_val;
    }

    uint64_t meBB  = (side == 'A') ? b->playerA : b->playerB;
    uint64_t oppBB = (side == 'A') ? b->playerB : b->playerA;

    // Terminal checks (wins)
    if (bitboard_win(meBB)) {
        return encode_win(ply);
    }
    if (bitboard_win(oppBB)) {
        return encode_loss(ply);
    }

    // Depth limit: use evaluation
    if (depth <= 0) {
        return evaluate(b, side);
    }

    // Generate moves
    int moves[COLS];
    int n = 0;

    // If TT suggested a move, try it first
    if (tt_move >= 0 && can_play(b, heights, tt_move)) {
        moves[n++] = tt_move;
    }

    // Add other moves in preferred order
    for (int i = 0; i < COLS; i++) {
        int c = column_order[i];
        if (c == tt_move) continue;
        if (can_play(b, heights, c)) moves[n++] = c;
    }

    if (n == 0) {
        // No legal moves: draw
        return 0;
    }

    // Win-in-1 pruning: check if side can win immediately
    for (int i = 0; i < n; i++) {
        int c = moves[i];
        uint64_t mv = move_bit_for_col(heights, c);
        if (bitboard_win(meBB | mv)) {
            int score = encode_win(ply);
            if (tt) tt_store(key, score, depth, EXACT, c);
            return score;
        }
    }

    int best      = -MATE;
    int best_move = moves[0];
    int alphaOrig = alpha;
    char next_side = (side == 'A') ? 'B' : 'A';

    for (int i = 0; i < n; i++) {
        int c = moves[i];

        // Update key incrementally:
        // piece index is (heights[c]) before apply_move
        int row     = heights[c];
        int idx     = row + c * 7;
        int sideIdx = (side == 'A') ? 0 : 1;

        uint64_t childKey = key;
        childKey ^= zobrist[sideIdx][idx];  // add piece
        childKey ^= zobrist_side;           // flip side to move

        apply_move(b, heights, c, side);
        int val = -negamax_solve(b, -beta, -alpha,
                                 next_side, ply + 1, depth - 1,
                                 childKey,
                                 heights,
                                 nodes_count,
                                 tt_hits_count);
        undo_move(b, heights, c);

        if (val > best) {
            best      = val;
            best_move = c;
        }
        if (best > alpha) alpha = best;
        if (alpha >= beta) break;  // beta cutoff
    }

    // Store to TT
    TTFlag flag;
    if      (best <= alphaOrig) flag = UPPERBOUND;
    else if (best >= beta)      flag = LOWERBOUND;
    else                        flag = EXACT;

    if (tt) tt_store(key, best, depth, flag, best_move);

    return best;
}

// -----------------------------------------------------------------------------
// SOLVER INTERFACE
// -----------------------------------------------------------------------------

int solve_position(Board* b) {
    int heights[COLS];
    init_heights(b, heights);
    char side = b->current;
    uint64_t key = compute_key(b, side);

    nodes_searched = 0;
    tt_hits        = 0;

    int res = negamax_solve(b, -MATE, MATE, side, 0, SOLVE_DEPTH, key,
                            heights, &nodes_searched, &tt_hits);

#if BOT_VERBOSE
    printf("Solve stats: nodes=%llu, tt_hits=%llu (%.1f%%)\n",
           nodes_searched, tt_hits,
           nodes_searched ? (100.0 * tt_hits / nodes_searched) : 0.0);
#endif

    if (res > 0) return 1;
    if (res < 0) return -1;
    return 0;
}

const char* solve_str(Board* b) {
    int r = solve_position(b);
    if (r > 0) return "WIN for side to move";
    if (r < 0) return "LOSS for side to move";
    return "DRAW";
}

// -----------------------------------------------------------------------------
// OPENING BOOK
// -----------------------------------------------------------------------------

static const int opening_book[][2] = {
    {0,3}, {1,3}, {2,3}, {3,3}, {4,3}, {5,3}
};
static const int opening_book_size = 1; //was 6

int opening_book_move(Board* b, int ply) {
	// Normal book for early plies: always play center (col 3, 0-based)
	if (ply >= 0 && ply < opening_book_size) {
		int col = opening_book[ply][1];
		if (game_can_drop(b, col) != -1)
			return col;
		// If center somehow not playable, fall through to search / other book
	}
	else if (ply < 6){
		Move last;
		if (!history_get_last_move(&last))
			return -1;          // no history? fall back to search

		if (last.player != 'A')
			return -1;          // last move must be human's

		int human_col = last.col;  // 0..6

		if (human_col == 3)
			return 3;
	}
	return -1; //remove

	// Special book at ply 6: after human's 3rd move, bot to move
	// Move order (bot starts as 'B'):
	//   0: B, 1: A, 2: B, 3: A, 4: B, 5: A  → now ply == 6, current == 'B'
	if (ply == 6 && b->current == 'B') {
		Move last;
		if (!history_get_last_move(&last))
			return -1;          // no history? fall back to search

		if (last.player != 'A')
			return -1;          // last move must be human's

		int human_col = last.col;  // 0..6

		int options[2];
		int num_opts = 0;

		switch (human_col) {
		case 0: // human played col 1
			options[0] = 2; options[1] = 4; num_opts = 2; break; // bot: 3 or 5
		case 1: // human played col 2
			options[0] = 1; num_opts = 1; break;                 // bot: 2
		case 2: // human played col 3
			options[0] = 2; options[1] = 5; num_opts = 2; break; // bot: 3 or 6
		case 3: // human played col 4
			options[0] = 2; options[1] = 4; num_opts = 2; break; // bot: 3 or 5
		case 4: // human played col 5
			options[0] = 4; options[1] = 1; num_opts = 2; break; // bot: 5 or 2
		case 5: // human played col 6
			options[0] = 5; num_opts = 1; break;                 // bot: 6
		case 6: // human played col 7
			options[0] = 4; options[1] = 2; num_opts = 2; break; // bot: 5 or 3
		default:
			return -1;
		}

		if (num_opts == 0) return -1;

		// Randomly choose among the options that are actually playable
		if (num_opts == 1) {
			int c = options[0];
			if (game_can_drop(b, c) != -1)
				return c;
			return -1;
		} else {
			int i  = rand() % 2;
			int c1 = options[i];
			int c2 = options[1 - i];

			if (game_can_drop(b, c1) != -1)
				return c1;
			if (game_can_drop(b, c2) != -1)
				return c2;

			return -1;
		}
	}

	// No book for other plies
	return -1;
}
// -----------------------------------------------------------------------------
// MAIN HARD BOT: pick_best_move (root-parallel with OpenMP)
// -----------------------------------------------------------------------------

int pick_best_move(Board* b) {
    int  ply  = __builtin_popcountll(b->mask);
    char side = b->current;

    int heights_root[COLS];
    init_heights(b, heights_root);

#if BOT_VERBOSE
    printf("\n[HARD BOT] Player %c, move %d/42\n", side, ply + 1);
    printf("Legal moves:");
    for (int c = 0; c < COLS; c++) {
        if (can_play(b, heights_root, c)) printf(" %d", c + 1);
    }
    printf("\n");
#endif

    // Opening book: if move is playable, just use it
    int book = opening_book_move(b, ply);
    if (book != -1 && can_play(b, heights_root, book)) {
#if BOT_VERBOSE
        printf("Using opening book move: column %d\n\n", book + 1);
#endif
        return book;
    }

    // Collect legal moves
    int moves[COLS];
    int n = 0;
    for (int c = 0; c < COLS; c++) {
        if (can_play(b, heights_root, c)) moves[n++] = c;
    }
    if (n == 0) return -1;

    uint64_t key = compute_key(b, side);

    nodes_searched = 0;
    tt_hits        = 0;

    int best_move = moves[0];
    int best_val  = -MATE;

#if BOT_VERBOSE
    printf("Searching to depth %d...\n", MAX_DEPTH);
#endif

#ifdef _OPENMP
    omp_set_num_threads(BOT_MAX_THREADS);
#pragma omp parallel
    {
        Board local_board;
        int   local_heights[COLS];
        unsigned long long local_nodes   = 0;
        unsigned long long local_tt_hits = 0;
        int   local_best_val  = -MATE;
        int   local_best_move = -1;

        // Parallel for over moves
#pragma omp for schedule(dynamic)
        for (int i = 0; i < n; i++) {
            int c = moves[i];

            // Copy board and heights for this thread's search
            local_board = *b;
            memcpy(local_heights, heights_root, sizeof(int) * COLS);

            // Build child key
            int row     = local_heights[c];
            int idx     = row + c * 7;
            int sideIdx = (side == 'A') ? 0 : 1;

            uint64_t childKey = key;
            childKey ^= zobrist[sideIdx][idx];
            childKey ^= zobrist_side;

            apply_move(&local_board, local_heights, c, side);
            int val = -negamax_solve(&local_board,
                                     -MATE, MATE,
                                     (side == 'A') ? 'B' : 'A',
                                     ply + 1, MAX_DEPTH - 1,
                                     childKey,
                                     local_heights,
                                     &local_nodes,
                                     &local_tt_hits);

#if BOT_VERBOSE
#pragma omp critical
            {
                printf("  Column %d: %s\n", c + 1, score_to_string(val, ply));
            }
#endif

            if (val > local_best_val) {
                local_best_val  = val;
                local_best_move = c;
            }
        }

        // Merge local stats & best into global
#pragma omp critical
        {
            nodes_searched += local_nodes;
            tt_hits        += local_tt_hits;
            if (local_best_val > best_val) {
                best_val  = local_best_val;
                best_move = local_best_move;
            }
        }
    }
#else
    // Single-threaded fallback (no OpenMP)
    for (int i = 0; i < n; i++) {
        int c = moves[i];

        int heights[COLS];
        memcpy(heights, heights_root, sizeof(int) * COLS);
        Board local_board = *b;

        int row     = heights[c];
        int idx     = row + c * 7;
        int sideIdx = (side == 'A') ? 0 : 1;
        uint64_t childKey = key ^ zobrist[sideIdx][idx] ^ zobrist_side;

        unsigned long long local_nodes   = 0;
        unsigned long long local_tt_hits = 0;

        apply_move(&local_board, heights, c, side);
        int val = -negamax_solve(&local_board,
                                 -MATE, MATE,
                                 (side == 'A') ? 'B' : 'A',
                                 ply + 1, MAX_DEPTH - 1,
                                 childKey,
                                 heights,
                                 &local_nodes,
                                 &local_tt_hits);

        nodes_searched += local_nodes;
        tt_hits        += local_tt_hits;

#if BOT_VERBOSE
        printf("  Column %d: %s\n", c + 1, score_to_string(val, ply));
#endif

        if (val > best_val) {
            best_val = val;
            best_move = c;
        }
    }
#endif

#if BOT_VERBOSE
    printf("Search stats: nodes=%llu, tt_hits=%llu (%.1f%%)\n",
           nodes_searched, tt_hits,
           nodes_searched ? (100.0 * tt_hits / nodes_searched) : 0.0);
    printf("Chosen move: column %d (%s)\n\n",
           best_move + 1, score_to_string(best_val, ply));
#endif

    return best_move;
}

// -----------------------------------------------------------------------------
// SHUTDOWN & SIMPLE BOTS
// -----------------------------------------------------------------------------

void shutdown_bot() {
    tt_save();
    free(tt);
    tt = NULL;
}

// Random bot (easy)
int bot_choose_move(const Board* g) {
    int valid[COLS];
    int n = 0;
    for (int c = 0; c < COLS; c++) {
        if (getChar(g, 0, c) == EMPTY) {
            valid[n++] = c;
        }
    }
    if (n == 0) return -1;
    return valid[rand() % n];
}

// Medium bot: blocks immediate wins + random
int bot_choose_move_medium(const Board* g) {
    int blocking[COLS];
    int nb = 0;
    char human = 'A';

    for (int c = 0; c < COLS; c++) {
        int r = game_can_drop(g, c);
        if (r == -1) continue;

        Board tmp = *g;
        setChar(&tmp, r, c, human);
        if (checkWin(&tmp, human)) {
            blocking[nb++] = c;
        }
    }

    if (nb > 0) {
        return blocking[rand() % nb];
    }

    return bot_choose_move(g);
}
