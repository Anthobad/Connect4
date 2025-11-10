#include "gamelogic.h"
#include "ui.h"
#include "bot.h"
#include <time.h>
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

static void reset_history(void) {
	move_count = 0;
	current_index = 0;
}

static void record_move(int row, int col, char player) {
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

static int undo_move(Board* G, int s) {
	if (current_index == 0)
		return 0;
	for(int i = 0; i < s; i++) {
		if (current_index == 0)
			return 0;
		current_index--;
		Move m = history[current_index];
		G->cells[m.row][m.col] = EMPTY;
		G->current = m.player;
	}
	return 1;
}

static int redo_move(Board* G, int s) {
	Move m = history[current_index];
	for(int i = 0; i < s; i++) {
		if (current_index == move_count)
			return 0;
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

static int read_line(char *buf, int n) {
	if (!fgets(buf, n, stdin))
		return 0;
	return 1;
}

static int parse_action(const char *s, int *out_col0) {
	while (isspace((unsigned char)*s))
		s++;
	if (*s == 'q' || *s == 'Q')
		return -1;
	if (*s == 'u' || *s == 'U')
		return -2;
	if (*s == 'r' || *s == 'R')
		return -3;
	char *end = NULL;
	long v = strtol(s, &end, 10);
	if (end == s)
		return 0;
	if (v < 1 || v > COLS)
		return 0;
	*out_col0 = (int)(v - 1);
	return 1;
}

static void switch_player(Board* G) {
	if (G->current == 'A')
		G->current = 'B';
	else
		G->current = 'A';
}

static int do_drop(Board* G, int col0, int use_anim, int anim_ms) {
	if (use_anim) {
		UiOptions opt;
		opt.use_color = 1;
		opt.delay_ms = anim_ms;
		return ui_drop_with_animation(G, col0, G->current, &opt);
	} else {
		return game_drop(G, col0, G->current);
	}
}

static int handle_turn(Board* G, int choice, int use_anim, int anim_ms) {
	char line[128];
	printf("Player %c, choose (1-%d), 'u' undo, 'r' redo, or 'q' quit: ", G->current, COLS);
    fflush(stdout);
	if (!read_line(line, sizeof line))
		return -1;
	int col0 = -1;
	int a = parse_action(line, &col0);
	if (a == -1)
		return -2;
	if (a == -2) {
		if (undo_move(G, choice))
			ui_print_board(G, 1);
		else
			puts("Nothing to undo.");
		return 0;
	}
	if (a == -3) {
		if (redo_move(G, choice))
			ui_print_board(G, 1);
		else
			puts("Nothing to redo.");
		return 0;
	}
	if (a == 0) {
		puts("Invalid input.");
		return 0;
	}
	int row = do_drop(G, col0, use_anim, anim_ms);
	if (row == -1) {
		puts("Column is full. Choose another.");
		return 0;
	}
	record_move(row, col0, G->current);
	if (!use_anim)
		ui_print_board(G, 1);
	if (checkWin(G, row, col0, G->current)) {
		printf("Player %c wins!\n", G->current);
		return 1;
	}
	if (checkDraw(G)) {
		puts("It's a draw! Board is full.");
		return 1;
	}
	switch_player(G);
	return 0;
}

static int play_again_prompt(void) {
	char choice[8];
	while (1) {
		printf("\nPlay again? (y/n): ");
        fflush(stdout);
		if (!fgets(choice, sizeof(choice), stdin))
			return 0;
		if (choice[0] == 'y' || choice[0] == 'Y')
			return 1;
		if (choice[0] == 'n' || choice[0] == 'N')
			return 0;
		puts("Invalid choice. Enter 'y' or 'n'.");
	}
}

int main(int argc, char** argv) {
	int use_anim = 1;
	int anim_ms = 110;
	if (argc > 1 && strcmp(argv[1], "--no-anim") == 0)
		use_anim = 0;
	srand((unsigned)time(NULL));

	while (1) {
		ui_clear_screen();
		puts("Connect Four\n");
		puts("1) Play 1v1 (human vs human)");
		puts("2) Play 1 vs Easy Bot (human vs computer)");
		puts("q) Quit");
		printf("Select an option: ");
		fflush(stdout);
		char choice[8];
		if (!fgets(choice, sizeof(choice), stdin)) {
			puts("\nInput ended. Exiting.");
			return 0;
		}
		if (choice[0] == 'q' || choice[0] == 'Q') {
			puts("Goodbye!");
			return 0;
		}
		if (choice[0] == '1') {
			char turn[8];
			printf("Which player goes first, A or B:");
			if(!fgets(turn, sizeof(turn), stdin)){
				puts("\nInput ended. Exiting.");
				return 0;
			}
			while(turn[0] != 'A' && turn[0] != 'B'){
				puts("Invalid letter label, try again.");
				printf("Which player goes first, A or B:");
				if(!fgets(turn, sizeof(turn), stdin)){
					puts("\nInput ended. Exiting.");
					return 0;
				}
			}
			Board G;
			initializeBoard(&G, turn[0]);
			reset_history();
			ui_print_board(&G, 1);

			int game_over = 0;
			while (!game_over) {
				int r = handle_turn(&G, choice[0] - '0', use_anim, anim_ms);
				if (r == -2) {
					puts("Goodbye!");
					return 0;
				}
				if (r == -1) {
					puts("\nInput ended. Exiting.");
					return 0;
				}
				if (r == 1)
					game_over = 1;
			}

			if (!play_again_prompt()) {
				puts("Thanks for playing!");
				return 0;
			}
		} 
		else if (choice[0] == '2') {
			char turn[8];
			printf("Which player goes first, You(A) or the Bot(B):");
			if(!fgets(turn, sizeof(turn), stdin)){
				puts("\nInput ended. Exiting.");
				return 0;
			}
			while(turn[0] != 'A' && turn[0] != 'B'){
				puts("Invalid letter label, try again.");
				printf("Which player goes first, You(A) or the Bot(B):");
				if(!fgets(turn, sizeof(turn), stdin)){
					puts("\nInput ended. Exiting.");
					return 0;
				}
			}
			int play_more = 1;
			while (play_more) {
				Board G;
				initializeBoard(&G, turn[0]);
				reset_history();
				ui_print_board(&G, 1);

				int game_over = 0;
				while (!game_over) {
					if (G.current == 'A') {
						int r = handle_turn(&G, choice[0] - '0', use_anim, anim_ms);
						if (r == -2) {
							puts("Goodbye!");
							return 0;
						}
						if (r == -1) {
							puts("\nInput ended. Exiting.");
							return 0;
						}
						if (r == 1) {
							game_over = 1;
							break;
						}
					} 
					else {
						int col0 = bot_choose_move(&G);
						if (col0 == -1) {
							if (checkDraw(&G)) {
								puts("It's a draw! Board is full.");
								game_over = 1;
								break;
							}
						} 
						else {
							int row = do_drop(&G, col0, use_anim, anim_ms);
							if (row != -1) {
								record_move(row, col0, G.current);
								if (!use_anim)
									ui_print_board(&G, 1);
								if (checkWin(&G, row, col0, G.current)) {
									printf("Player %c (bot) wins!\n", G.current);
									game_over = 1;
									break;
								}
								if (checkDraw(&G)) {
									puts("It's a draw! Board is full.");
									game_over = 1;
									break;
								}
								switch_player(&G);
							}
						}
					}
				}

				play_more = !play_again_prompt();
				if (play_more) {
					puts("Thanks for playing!");
					return 0;
				}
			}
		} 
		else {
			puts("Invalid selection. Press Enter to continue.");
			if(!fgets(choice, sizeof(choice), stdin)){
				puts("\nInput ended. Exiting.");
				return 0;
			}
		}
	}
}
