#define _XOPEN_SOURCE 500 // this enables usleep in c11 because it hides it
#include "ui.h"
#include <stdio.h>
#include <unistd.h>

static void ui_print_header_row(void) {
	printf("   ");
	for (int c = 0; c < COLS; c++)
		printf("%d ", c + 1);
	puts("\n  +---------------+");
}

void ui_clear_screen(void) {
	fputs("\033[H\033[J", stdout);
}

void ui_print_board(const Board* g, int use_color) {
	puts("");
	ui_print_header_row();
	for (int r = 0; r < ROWS; r++) {
		printf("%d | ", ROWS - r);
		for (int c = 0; c < COLS; c++) {
			char ch = g->cells[r][c];
			if (use_color == 0) {
				printf("%c ", ch);
			} else {
				if (ch == 'A')
					fputs("\033[33mA\033[0m ", stdout);
				else if (ch == 'B')
					fputs("\033[31mB\033[0m ", stdout);
				else
					printf("%c ", ch);
			}
		}
		puts("|");
	}
	puts("  +---------------+");
}

int ui_drop_with_animation(Board* g, int col, char player, const UiOptions* opt) {
	int landing = game_can_drop(g, col);
	if (landing == -1)
		return -1;
	Board temp = *g;
	for (int r = 0; r <= landing; r++) {
		temp = *g;
		if (temp.cells[r][col] != EMPTY)
			continue;
		temp.cells[r][col] = player;
		ui_clear_screen();
		int use_color = 0;
		if (opt)
			use_color = opt->use_color;
		ui_print_board(&temp, use_color);
		fflush(stdout);
		int ms;
		if (opt)
			ms = opt->delay_ms;
		else
			ms = 100;
		if (ms < 0)
			ms = 0;
		usleep((unsigned)(ms) * 1000);
	}
	g->cells[landing][col] = player;
	ui_clear_screen();
	int use_color_final = 0;
	if (opt)
		use_color_final = opt->use_color;
	ui_print_board(g, use_color_final);
	return landing;
}

void ui_wait_for_enter(void) {
	char buf[16];
	printf("\nPress Enter to continue...");
	fflush(stdout);
	fgets(buf, sizeof buf, stdin);
}

int ui_main_menu(void) {
	while (1) {
		ui_clear_screen();
		puts("Connect Four\n");
		puts("1) Play directly (human vs human)");
		puts("2) Play online with friends");
		puts("3) Play vs Bot");
		puts("4) About game");
		puts("q) Quit");
		printf("Select an option: ");
		fflush(stdout);

		char line[16];
		if (!fgets(line, sizeof line, stdin)) {
			return 0;
		}

		char ch = line[0];
		if (ch == 'q' || ch == 'Q') return 0;
		if (ch == '1') return 1;
		if (ch == '2') return 2;
		if (ch == '3') return 3;
		if (ch == '4') return 4;

		puts("Invalid selection.");
		ui_wait_for_enter();
	}
}

int ui_bot_menu(void) {
	while (1) {
		ui_clear_screen();
		puts("Play vs Bot\n");
		puts("1) Easy");
		puts("2) Medium");
		puts("3) Hard");
		puts("b) Back to main menu");
		printf("Select difficulty: ");
		fflush(stdout);

		char line[16];
		if (!fgets(line, sizeof line, stdin)) {
			return 0;
		}

		char ch = line[0];
		if (ch == 'b' || ch == 'B') return 0;
		if (ch == '1') return 1;
		if (ch == '2') return 2;
		if (ch == '3') return 3;

		puts("Invalid selection.");
		ui_wait_for_enter();
	}
}

void ui_show_about(void) {
	ui_clear_screen();
	puts("About Connect Four\n");
	puts("This is a console-based Connect Four game built as a CMPS 241 project.");
	puts("- Board size: 7 columns x 6 rows");
	puts("- Players: A and B");
	puts("- Win by connecting 4 in a row horizontally, vertically, or diagonally.");
	puts("");
	puts("Current features:");
	puts("- Human vs human mode");
	puts("- Easy and Medium bots");
	puts("- Undo/redo");
	puts("- Optional drop animation and colored output");
	puts("");
	puts("Future work:");
	puts("- Online multiplayer (Play online with friends)");
	puts("- Hard-level bot (stronger AI).");
}