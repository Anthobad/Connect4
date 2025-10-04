// play.c — Sprint 1 console game using gamelogic, with undo/redo + replay menu
#include "gamelogic.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_MOVES (ROWS * COLS)

typedef struct {
    int row;
    int col;
    char player;
} Move;

static Move history[MAX_MOVES];
static int move_count = 0;
static int current_index = 0;

// record a move in history
static void record_move(int row, int col, char player) {
    if (current_index < move_count) {
        move_count = current_index; // discard future moves if undone
    }
    if (move_count < MAX_MOVES) {
        history[move_count].row = row;
        history[move_count].col = col;
        history[move_count].player = player;
        move_count++;
        current_index = move_count;
    }
}

static int undo_move(Board* G) {
    if (current_index == 0) return 0;
    current_index--;
    Move m = history[current_index];
    G->cells[m.row][m.col] = EMPTY;
    G->current = m.player; // give turn back to undone player
    return 1;
}

static int redo_move(Board* G) {
    if (current_index == move_count) return 0;
    Move m = history[current_index];
    G->cells[m.row][m.col] = m.player;
    current_index++;
    G->current = (m.player == 'A' ? 'B' : 'A');
    return 1;
}

static int parse_move(const char *s, int *out_col0) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == 'q' || *s == 'Q') return -1;
    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (end == s) return 0;
    if (v < 1 || v > COLS) return 0;
    *out_col0 = (int)(v - 1);
    return 1;
}

// reset move history between games
static void reset_history() {
    move_count = 0;
    current_index = 0;
}

int main(int argc, char** argv) {
    int use_anim = 1;
    int anim_ms  = 110;
    if (argc > 1 && strcmp(argv[1], "--no-anim") == 0) use_anim = 0;

    while (1) {  // game session loop
        Board G;
        game_init(&G);
        reset_history();
        game_print(&G);

        char line[128];
        int game_over = 0;

        while (!game_over) {
            printf("Player %c, choose (1-%d), 'u' undo, 'r' redo, or 'q' quit: ",
                   G.current, COLS);
            if (!fgets(line, sizeof line, stdin)) {
                puts("\nInput ended. Exiting.");
                return 0;
            }

            if (line[0] == 'u' || line[0] == 'U') {
                if (undo_move(&G)) game_print(&G);
                else puts("Nothing to undo.");
                continue;
            }

            if (line[0] == 'r' || line[0] == 'R') {
                if (redo_move(&G)) game_print(&G);
                else puts("Nothing to redo.");
                continue;
            }

            int col0 = -1;
            int parsed = parse_move(line, &col0);
            if (parsed == -1) { puts("Goodbye!"); return 0; }
            if (parsed == 0)  { puts("Invalid input."); continue; }

            int row = use_anim
                ? game_drop_with_animation(&G, col0, G.current, 1, anim_ms)
                : game_drop(&G, col0, G.current);

            if (row == -1) {
                puts("Column is full. Choose another.");
                continue;
            }

            record_move(row, col0, G.current);

            if (!use_anim) game_print(&G);

            if (game_is_win_at(&G, row, col0, G.current)) {
                printf("🎉 Player %c wins!\n", G.current);
                game_over = 1;
            } else if (game_is_draw(&G)) {
                puts("It's a draw! Board is full.");
                game_over = 1;
            } else {
                G.current = (G.current == 'A' ? 'B' : 'A');
            }
        }

        // ---- Play Again Menu ----
        char choice[8];
        while (1) {
            printf("\nPlay again? (y/n): ");
            if (!fgets(choice, sizeof(choice), stdin)) return 0;
            if (choice[0] == 'y' || choice[0] == 'Y') {
                break; // restart outer while(1)
            } else if (choice[0] == 'n' || choice[0] == 'N') {
                puts("Thanks for playing!");
                return 0;
            } else {
                puts("Invalid choice. Enter 'y' or 'n'.");
            }
        }
    }
}
