#pragma once
#include "gamelogic.h"

typedef struct {
	int use_color;
	int delay_ms;
} UiOptions;

void ui_clear_screen(void);
void ui_print_board(const Board* g, int use_color);
int ui_drop_with_animation(Board* g, int col, char player, const UiOptions* opt);
