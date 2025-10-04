//core game model
#pragma once
#include <stdio.h>

#define ROWS 6
#define COLS 7
#define EMPTY '.'

typedef struct {
    char cells[ROWS][COLS];
    char current; // 'A' or 'B'
} Board;

// Model
void game_init(Board* g);
int  game_in_bounds(int r, int c);
int  game_can_drop(const Board* g, int col);        // landing row or -1
int  game_drop(Board* g, int col, char player);     // landing row or -1
int  game_is_win_at(const Board* g, int r, int c, char p);
int  game_is_draw(const Board* g);

// View (console)
void game_print(const Board* g);

// Serialization
void game_flatten(const Board* g, char out[ROWS*COLS+1]);
void game_unflatten(Board* g, const char* s);

// Optional ASCII falling animation
int  game_drop_with_animation(Board* g, int col, char player, int use_color, int delay_ms);
