#include "gamelogic.h"
#include "bot.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>

#define TT_SIZE (1<<24)  // Increased to 16M entries (~400MB)
#define MAX_DEPTH 20

// --- Constants ---
static const int column_order[7] = {3,4,2,5,1,6,0};

// --- Bit helpers ---
static inline int can_play(const Board* b, int col) {
    return game_can_drop(b, col) != -1;
}

static inline uint64_t make_move_bit(const Board* b, int col) {
    int landing = game_can_drop(b, col);
    if (landing == -1) return 0ULL;
    return 1ULL << (landing + col * 7);
}

static inline void apply_move(Board* b, int col, char side) {
    uint64_t mv = make_move_bit(b, col);
    if (!mv) return; // Safety check
    if (side == 'A') b->playerA ^= mv;
    else             b->playerB ^= mv;
    b->mask |= mv;
}

static inline void undo_move(Board* b, int col) {
    for (int r = ROWS-1; r >= 0; r--) {
        int bit = col * 7 + r;
        uint64_t m = (1ULL << bit);
        if (b->mask & m) {
            b->mask ^= m;
            if (b->playerA & m)
                b->playerA ^= m;
            else // Must be playerB
                b->playerB ^= m;
            return;
        }
    }
}

// Fast bitboard win check
static inline int bitboard_win(uint64_t bb) {
    uint64_t m;
    // Horizontal
    m = bb & (bb >> 7);
    if (m & (m >> 14)) return 1;
    // Diagonal forward slash
    m = bb & (bb >> 6);
    if (m & (m >> 12)) return 1;
    // Diagonal backslash
    m = bb & (bb >> 8);
    if (m & (m >> 16)) return 1;
    // Vertical
    m = bb & (bb >> 1);
    if (m & (m >> 2)) return 1;
    return 0;
}

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

void zobrist_init() {
    srand((unsigned)time(NULL));
    for(int p = 0; p < 2; p++) {
        for(int i = 0; i < 42; i++) {
            // FIX: Proper 64-bit random generation
            zobrist[p][i] = ((uint64_t)rand() << 32) | rand();
        }
    }
    zobrist_side = ((uint64_t)rand() << 32) | rand();
}

void tt_init() {
    tt = calloc(TT_SIZE, sizeof(TTEntry));
    if (!tt) {
        fprintf(stderr, "ERROR: Failed to allocate transposition table\n");
        exit(1);
    }
    
    // Try to load persistent TT
    FILE* f = fopen("tt.bin","rb");
    if(f) {
        size_t read = fread(tt, sizeof(TTEntry), TT_SIZE, f);
        printf("Loaded %zu TT entries from disk\n", read);
        fclose(f);
    }
}

static void tt_save() {
    if(!tt) return;
    FILE* f = fopen("tt.bin","wb");
    if(f) {
        fwrite(tt, sizeof(TTEntry), TT_SIZE, f);
        fclose(f);
        printf("Saved TT to disk\n");
    }
}

static void tt_store(uint64_t key, int value, int depth, TTFlag flag, int best_move) {
    size_t idx = key & (TT_SIZE - 1);
    TTEntry* e = &tt[idx];
    // Always replace: higher depth or empty slot
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
        // Always extract best move hint
        if(out_move && e->best_move != 255) {
            *out_move = e->best_move;
        }
        
        if(e->depth >= depth) {
            if(e->flag == EXACT) {
                *out_value = e->value;
                return 1;
            }
            if(e->flag == LOWERBOUND && e->value > alpha) alpha = e->value;
            if(e->flag == UPPERBOUND && e->value < beta) beta = e->value;
            if(alpha >= beta) {
                *out_value = e->value;
                return 1;
            }
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

// --- Compute Zobrist key ---
static inline uint64_t compute_key(const Board* b, char side) {
    uint64_t key = 0;
    uint64_t bb = b->playerA;
    while(bb) {
        int idx = __builtin_ctzll(bb);
        key ^= zobrist[0][idx];
        bb &= bb - 1;
    }
    bb = b->playerB;
    while(bb) {
        int idx = __builtin_ctzll(bb);
        key ^= zobrist[1][idx];
        bb &= bb - 1;
    }
    if(side == 'B') key ^= zobrist_side;
    return key;
}

// --- Debug configuration ---
#define DEBUG_ENABLED 1      // Set to 1 to enable detailed node printing
#define DEBUG_MAX_DEPTH 10    // Only print nodes up to this depth (when DEBUG_ENABLED=1)

// --- Node counter for stats ---
static unsigned long long nodes_searched = 0;
static unsigned long long tt_hits = 0;

// --- Helper to interpret scores ---
static const char* score_to_string(int score, int ply) {
    static char buf[64];
    if (score > MATE - 100) {
        int moves_to_win = (MATE - score + ply) / 2;
        snprintf(buf, sizeof(buf), "WIN in %d moves", moves_to_win);
    } else if (score < -MATE + 100) {
        int moves_to_loss = (MATE + score - ply) / 2;
        snprintf(buf, sizeof(buf), "LOSS in %d moves", moves_to_loss);
    } else if (score == 0) {
        snprintf(buf, sizeof(buf), "DRAW");
    } else {
        snprintf(buf, sizeof(buf), "Score: %+d", score);
    }
    return buf;
}

#if DEBUG_ENABLED
// --- Helper to print indented debug info ---
static void print_node_info(int ply, char side, int alpha, int beta, const char* msg) {
    if (ply > DEBUG_MAX_DEPTH) return; // Only print up to max depth
    for(int i = 0; i < ply; i++) printf("  ");
    printf("[Ply %2d | %c | α=%+7d β=%+7d] %s\n", ply, side, alpha, beta, msg);
}
#else
// No-op when debug is disabled
static inline void print_node_info(int ply, char side, int alpha, int beta, const char* msg) {
    (void)ply; (void)side; (void)alpha; (void)beta; (void)msg;
}
#endif

// --- Negamax solver with TT and optimizations ---
static int negamax_solve(Board* b, int alpha, int beta, char side, int ply, int depth) {
	//Base case
	if(depth == 0) return -INT_MAX;

    nodes_searched++;
    
    // Generate key
    uint64_t key = compute_key(b, side);
    
#if DEBUG_ENABLED
    char entry_msg[128];
    snprintf(entry_msg, sizeof(entry_msg), "Entering node (pieces: %d)", 
             __builtin_popcountll(b->mask));
    print_node_info(ply, side, alpha, beta, entry_msg);
#endif
    
    // TT probe
    int tt_val, tt_move = -1;
    if(tt && tt_probe(key, MAX_DEPTH - ply, alpha, beta, &tt_val, &tt_move)) {
        tt_hits++;
#if DEBUG_ENABLED
        char tt_msg[128];
        snprintf(tt_msg, sizeof(tt_msg), "TT HIT! Returning %+d (move hint: %d)", 
                 tt_val, tt_move >= 0 ? tt_move + 1 : -1);
        print_node_info(ply, side, alpha, beta, tt_msg);
#endif
        return tt_val;
    }
    
    uint64_t me = (side == 'A') ? b->playerA : b->playerB;
    uint64_t opp = (side == 'A') ? b->playerB : b->playerA;
    
    // Terminal checks using fast bitboard
    if(bitboard_win(opp)) {
        int score = encode_loss(ply);
#if DEBUG_ENABLED
        char msg[64];
        snprintf(msg, sizeof(msg), "OPPONENT WON! Returning %+d", score);
        print_node_info(ply, side, alpha, beta, msg);
#endif
        return score;
    }
    
    if((b->mask & 0x1FFFFFFFFFFFFFULL) == 0x1FFFFFFFFFFFFFULL) {
#if DEBUG_ENABLED
        print_node_info(ply, side, alpha, beta, "BOARD FULL - DRAW! Returning 0");
#endif
        return 0;
    }
    
    // Win-in-1 pruning: check if we can win immediately
    for(int i = 0; i < 7; i++) {
        int col = column_order[i];
        if(!can_play(b, col)) continue;
        
        uint64_t move = make_move_bit(b, col);
        if(bitboard_win(me | move)) {
            int score = encode_win(ply);
#if DEBUG_ENABLED
            char msg[64];
            snprintf(msg, sizeof(msg), "IMMEDIATE WIN at col %d! Returning %+d", col + 1, score);
            print_node_info(ply, side, alpha, beta, msg);
#endif
            if(tt) tt_store(key, score, MAX_DEPTH - ply, EXACT, col);
            return score;
        }
    }
    
    // Generate moves in order
    int moves[7];
    int n = 0;
    
    // Try TT move first
    if(tt_move >= 0 && can_play(b, tt_move)) {
        moves[n++] = tt_move;
    }
    
    // Add remaining moves
    for(int i = 0; i < 7; i++) {
        int col = column_order[i];
        if(col == tt_move) continue;
        if(can_play(b, col)) moves[n++] = col;
    }
    
    if(n == 0) {
#if DEBUG_ENABLED
        print_node_info(ply, side, alpha, beta, "NO MOVES - DRAW! Returning 0");
#endif
        return 0;
    }
    
#if DEBUG_ENABLED
    char moves_msg[64];
    snprintf(moves_msg, sizeof(moves_msg), "Trying %d moves", n);
    print_node_info(ply, side, alpha, beta, moves_msg);
#endif
    
    int best = -MATE;
    int best_move = moves[0];
    int alpha_orig = alpha;
    
    char next_side = (side == 'A') ? 'B' : 'A';
    
    for(int i = 0; i < n; i++) {
        int col = moves[i];
        
#if DEBUG_ENABLED
        char move_msg[64];
        snprintf(move_msg, sizeof(move_msg), "→ Trying move: Column %d", col + 1);
        print_node_info(ply, side, alpha, beta, move_msg);
#endif
        
        apply_move(b, col, side);
        int val = -negamax_solve(b, -beta, -alpha, next_side, ply + 1, depth-1);
        undo_move(b, col);
        
#if DEBUG_ENABLED
        char result_msg[128];
        snprintf(result_msg, sizeof(result_msg), "← Column %d returned %+d (best so far: %+d)", 
                 col + 1, val, best);
        print_node_info(ply, side, alpha, beta, result_msg);
#endif
        
        if(val > best) {
            best = val;
            best_move = col;
            
#if DEBUG_ENABLED
            char new_best_msg[64];
            snprintf(new_best_msg, sizeof(new_best_msg), "★ NEW BEST: %+d", best);
            print_node_info(ply, side, alpha, beta, new_best_msg);
#endif
        }
        
        alpha = (alpha > best) ? alpha : best;
        if(alpha >= beta) {
#if DEBUG_ENABLED
            char cutoff_msg[64];
            snprintf(cutoff_msg, sizeof(cutoff_msg), "✂ BETA CUTOFF! (α=%+d ≥ β=%+d)", alpha, beta);
            print_node_info(ply, side, alpha, beta, cutoff_msg);
#endif
            break;
        }
    }
    
    // Store in TT
    TTFlag flag;
    if(best <= alpha_orig) flag = UPPERBOUND;
    else if(best >= beta) flag = LOWERBOUND;
    else flag = EXACT;
    
#if DEBUG_ENABLED
    char exit_msg[128];
    snprintf(exit_msg, sizeof(exit_msg), "Exiting with score %+d (best move: col %d, flag: %s)", 
             best, best_move + 1, 
             flag == EXACT ? "EXACT" : flag == LOWERBOUND ? "LOWER" : "UPPER");
    print_node_info(ply, side, alpha, beta, exit_msg);
#endif
    
    if(tt) tt_store(key, best, MAX_DEPTH - ply, flag, best_move);
    
    return best;
}

int solve_position(Board* b) {
    int res = negamax_solve(b, -MATE, MATE, b->current, 0, MAX_DEPTH);
    if(res > 0) return 1;
    if(res < 0) return -1;
    return 0;
}

const char* solve_str(Board* b) {
    int res = solve_position(b);
    if(res > 0) return "WIN for side to move";
    if(res < 0) return "LOSS for side to move";
    return "DRAW";
}

// --- Opening book ---
static const int opening_book[][2] = {
    {0,3}, {1,2}, {2,3}, {3,2}, {4,3}, {5,2}, {6,3}, {7,2}
};
static int opening_book_size = 8;

int opening_book_move(Board* b, int ply) {
    if(ply >= opening_book_size) return -1;
    return opening_book[ply][1];
}

// --- Wrapper: automatic best move ---
int pick_best_move(Board* b) {
    int ply = __builtin_popcountll(b->mask);

    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║        BOT ANALYSIS (Player %c)            ║\n", b->current);
    printf("╚════════════════════════════════════════════╝\n");
    printf("Position: Move %d/42\n", ply + 1);
    printf("Legal moves:");
    for (int i = 0; i < COLS; ++i)
        if (can_play(b, i)) printf(" %d", i+1);
    printf("\n");

    // Opening book
    int book = opening_book_move(b, ply);
    if (book != -1 && can_play(b, book)) {
        printf("\n✓ Using opening book move: Column %d\n", book + 1);
        printf("════════════════════════════════════════════\n\n");
        return book;
    }

    int best = -1;
    int best_val = -MATE;
    char side = b->current;

    int moves[COLS];
    int n = 0;
    for (int i = 0; i < COLS; i++)
        if (can_play(b, i)) moves[n++] = i;

    if (n == 0) return -1;

    printf("\nSearching to depth %d...\n", MAX_DEPTH - ply);
    printf("════════════════════════════════════════════\n");

    // Reset stats
    nodes_searched = 0;
    tt_hits = 0;

    for (int i = 0; i < n; i++) {
        int col = moves[i];
        
        printf("Analyzing Column %d... ", col + 1);
        fflush(stdout);
        
        apply_move(b, col, side);
        int val = -negamax_solve(b, -MATE, MATE, (side == 'A') ? 'B' : 'A', ply + 1, MAX_DEPTH);
        undo_move(b, col);

        printf("%s", score_to_string(val, ply));
        
        if (val > best_val) {
            printf(" ← BEST!");
            best_val = val;
            best = col;
        }
        printf("\n");
    }

    printf("════════════════════════════════════════════\n");
    printf("SEARCH STATS:\n");
    printf("  Nodes searched: %llu\n", nodes_searched);
    printf("  TT hits: %llu (%.1f%%)\n", tt_hits, 
           nodes_searched > 0 ? (100.0 * tt_hits / nodes_searched) : 0.0);
    printf("════════════════════════════════════════════\n");
    printf("✓ DECISION: Column %d - %s\n", best + 1, score_to_string(best_val, ply));
    printf("════════════════════════════════════════════\n\n");
    
    return best;
}

void shutdown_bot() {
    tt_save();
    free(tt);
    tt = NULL;
}

// Simple fallback bots
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

int bot_choose_move_medium(const Board* g) {
    int blocking_cols[COLS];
    int num_blocking = 0;
    char human = 'A';

    for (int c = 0; c < COLS; c++) {
        int r = game_can_drop(g, c);
        if (r == -1) continue;
        
        Board temp = *g;
        setChar(&temp, r, c, human);
        if (checkWin(&temp, human)) {
            blocking_cols[num_blocking++] = c;
        }
    }

    if (num_blocking > 0) {
        return blocking_cols[rand() % num_blocking];
    }

    return bot_choose_move(g);
}
