#include "controller.h"

#include "gamelogic.h"
#include "ui.h"
#include "bot.h"
#include "history.h"
#include "input.h"

#include <stdio.h>
#include <stdlib.h>

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

static int handle_turn(Board* G, int undo_span, int use_anim, int anim_ms) {
	char line[128];

	printf("Player %c, choose (1-%d), 'u' undo, 'r' redo, or 'q' quit: ",
		G->current, COLS);
	fflush(stdout);

	if (!read_line(line, sizeof line))
		return -1;

	int col0 = -1;
	int a = parse_action(line, &col0);

	if (a == -1)
		return -2;

	if (a == -2) {
		if (history_undo(G, undo_span))
			ui_print_board(G, 1);
		else
			puts("Nothing to undo.");
		return 0;
	}

	if (a == -3) {
		if (history_redo(G, undo_span))
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

	history_record_move(row, col0, G->current);

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

void run_human_vs_human(int use_anim, int anim_ms) {
	char turn[8];

	printf("Which player goes first, A or B: ");
	if (!fgets(turn, sizeof(turn), stdin)) {
		puts("\nInput ended. Exiting.");
		exit(0);
	}

	while (turn[0] != 'A' && turn[0] != 'B') {
		puts("Invalid letter label, try again.");
		printf("Which player goes first, A or B: ");
		if (!fgets(turn, sizeof(turn), stdin)) {
			puts("\nInput ended. Exiting.");
			exit(0);
		}
	}

	int keep_playing = 1;
	while (keep_playing) {
		Board G;
		initializeBoard(&G, turn[0]);
		history_reset();
		ui_print_board(&G, 1);

		int game_over = 0;
		while (!game_over) {
			int r = handle_turn(&G, 1, use_anim, anim_ms);
			if (r == -2) {
				puts("Goodbye!");
				exit(0);
			}
			if (r == -1) {
				puts("\nInput ended. Exiting.");
				exit(0);
			}
			if (r == 1)
				game_over = 1;
		}

		keep_playing = play_again_prompt();
		if (!keep_playing) {
			puts("Thanks for playing!");
		}
	}
}

void run_vs_bot(int use_anim, int anim_ms, int difficulty) {
	char turn[8];

	printf("Which player goes first, You(A) or the Bot(B): ");
	if (!fgets(turn, sizeof(turn), stdin)) {
		puts("\nInput ended. Exiting.");
		exit(0);
	}

	while (turn[0] != 'A' && turn[0] != 'B') {
		puts("Invalid letter label, try again.");
		printf("Which player goes first, You(A) or the Bot(B): ");
		if (!fgets(turn, sizeof(turn), stdin)) {
			puts("\nInput ended. Exiting.");
			exit(0);
		}
	}

	int play_more = 1;
	while (play_more) {
		Board G;
		initializeBoard(&G, turn[0]);
		history_reset();
		ui_print_board(&G, 1);

		int game_over = 0;
		while (!game_over) {
			if (G.current == 'A') {
				int r = handle_turn(&G, 2, use_anim, anim_ms);
				if (r == -2) {
					puts("Goodbye!");
					exit(0);
				}
				if (r == -1) {
					puts("\nInput ended. Exiting.");
					exit(0);
				}
				if (r == 1) {
					game_over = 1;
					break;
				}
			} else {
				int col0;
				if (difficulty == 1)
					col0 = bot_choose_move(&G);
				else
					col0 = bot_choose_move_medium(&G);

				if (col0 == -1) {
					if (checkDraw(&G)) {
						puts("It's a draw! Board is full.");
						game_over = 1;
						break;
					}
				} else {
					int row = do_drop(&G, col0, use_anim, anim_ms);
					if (row != -1) {
						history_record_move(row, col0, G.current);
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

		play_more = play_again_prompt();
		if (!play_more) {
			puts("Thanks for playing!");
		}
	}
}

void run_human_online(int use_anim, int anim_ms) {
	(void)use_anim;
	(void)anim_ms;
}