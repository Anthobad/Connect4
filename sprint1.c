// CMPS 241 – Sprint 1: Two-Player Connect 4 (Console)
// Quit anytime by entering 'q' instead of a column number.

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define ROWS 6
#define COLS 7
#define EMPTY '.'

static void init_board(char b[ROWS][COLS]) {
    for (int r = 0; r < ROWS; ++r)
        for (int c = 0; c < COLS; ++c)
            b[r][c] = EMPTY;
}

static void print_board(const char b[ROWS][COLS]) {
    puts("");
    // Column header
    printf("   ");
    for (int c = 0; c < COLS; ++c) printf("%d ", c + 1);
    puts("\n  +---------------+");
    // Rows (top to bottom visually)
    for (int r = 0; r < ROWS; ++r) {
        printf("%d | ", ROWS - r); // show row numbers (optional)
        for (int c = 0; c < COLS; ++c) {
            printf("%c ", b[r][c]);
        }
        puts("|");
    }
    puts("  +---------------+");
}

// Try to drop piece in column (0-based). Returns the row index where it landed, or -1 if column full.
static int drop_piece(char b[ROWS][COLS], int col, char player) {
    if (col < 0 || col >= COLS) return -1;
    for (int r = ROWS - 1; r >= 0; --r) {
        if (b[r][col] == EMPTY) {
            b[r][col] = player;
            return r;
        }
    }
    return -1; // full
}

static int in_bounds(int r, int c) {
    return (r >= 0 && r < ROWS && c >= 0 && c < COLS);
}

// Count consecutive pieces for (player) starting from (r,c) in a direction (dr,dc), including (r,c).
static int count_dir(const char b[ROWS][COLS], int r, int c, int dr, int dc, char player) {
    int cnt = 0;
    int rr = r, cc = c;
    while (in_bounds(rr, cc) && b[rr][cc] == player) {
        cnt++; rr += dr; cc += dc;
    }
    return cnt;
}

// Check if placing at (r,c) by 'player' makes 4 in a row
static int is_winning_move(const char b[ROWS][COLS], int r, int c, char player) {
    // Directions: horizontal (0,1), vertical (1,0), diag down-right (1,1), diag up-right (-1,1)
    const int dirs[4][2] = {{0,1},{1,0},{1,1},{-1,1}};
    for (int i = 0; i < 4; ++i) {
        int dr = dirs[i][0], dc = dirs[i][1];
        // Count both ways minus one (center counted twice)
        int total = count_dir(b, r, c, dr, dc, player) + count_dir(b, r, c, -dr, -dc, player) - 1;
        if (total >= 4) return 1;
    }
    return 0;
}

static int is_draw(const char b[ROWS][COLS]) {
    for (int c = 0; c < COLS; ++c) {
        if (b[0][c] == EMPTY) return 0;
    }
    return 1;
}

static int parse_move(const char *s, int *out_col0) {
    // Accept 'q' or 'Q' to quit; otherwise an integer 1..7
    while (isspace((unsigned char)*s)) s++;
    if (*s == 'q' || *s == 'Q') return -1;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s) return 0; // not a number
    if (v < 1 || v > COLS) return 0;
    *out_col0 = (int)(v - 1);
    return 1;
}

int main(void) {
    char board[ROWS][COLS];
    init_board(board);

    char current = 'A'; // players 'A' and 'B'
    print_board(board);

    char line[128];
    while (1) {
        printf("Player %c, choose a column (1-%d) or 'q' to quit: ", current, COLS);
        if (!fgets(line, sizeof line, stdin)) {
            puts("\nInput ended. Exiting.");
            return 0;
        }

        int col0 = -1;
        int parsed = parse_move(line, &col0);
        if (parsed == -1) {
            puts("Goodbye!");
            return 0;
        }
        if (parsed == 0) {
            puts("Invalid input. Please enter a number between 1 and 7, or 'q'.");
            continue;
        }

        int row = drop_piece(board, col0, current);
        if (row == -1) {
            puts("Column is full. Choose another column.");
            continue;
        }

        print_board(board);

        if (is_winning_move(board, row, col0, current)) {
            printf("Player %c wins!\n", current);
            break;
        }
        if (is_draw(board)) {
            puts("It's a draw! Board is full.");
            break;
        }

        current = (current == 'A') ? 'B' : 'A';
    }

    return 0;
}
