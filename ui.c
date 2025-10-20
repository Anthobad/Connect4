#define _XOPEN_SOURCE 500
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
