# Connect 4 - Console Game in C

This repository contains a small console-based Connect Four implementation in C. It supports a human vs human mode, a simple "easy" bot, undo/redo, and optional drop animations and color output.

This README documents how to build and run the game, explains public header files and functions, and describes gameplay controls and the bot's behavior.

A two-player Connect 4 game implemented in C, featuring:

- Undo/redo functionality  
- Animated piece drops  
- Colored board output in the terminal 
- Play against an easy bot 
- Cross-platform support (Linux, Windows with `gcc`)  

---

## Table of Contents

- [Gameplay](#gameplay)  
- [Features](#features)  
- [Requirements](#requirements)  
- [Compilation](#compilation)  
- [Quick start](#quick-start)  
- [Controls](#controls)  
- [Files and headers](#files-and-headers) 
- [Build notes](#build-notes) 
- [Runtime examples](#runtime-examples) 
- [Files and headers](#files-and-headers) 
- [Extending the project](#extending-the-project) 
- [Credits and license](#credits-and-license) 


---

## Gameplay

Players take turns dropping pieces into a 7-column, 6-row grid. The first player to connect **4 pieces in a row, column, or diagonal** wins. If the board fills up without a winner, the game ends in a draw.

---

## Features

- Undo and redo previous moves  
- Optional animated piece drops  
- Color-coded board for better visualization  
- Console-based, lightweight, and fast  

---

## Requirements

- GCC (tested on Alpine Linux and standard Linux)  
- POSIX-compliant terminal for colored output  

---

## Compilation

Clone the repository:

```bash
git clone https://github.com/your-username/Connect4.git
cd Connect4
```

## Quick start

Build (example with gcc):

```bash
gcc -std=c11 -Wall -Werror -o connect4 play.c gamelogic.c ui.c bot.c
```
Or use makefile

```Bash
make
```

Run:

```bash
./connect4        # run with animations and colors (default)
./connect4 --no-anim  # disable falling animation
```

## Controls

On start the program shows a main menu letting you choose:
- `1` → Play human vs human (1v1)
- `2` → Play human vs easy bot (human is player A, bot is player B)
- `q` → Quit

During a human player's turn you can enter:
- `1`..`7` to drop a piece in that column
- `u` to undo the last move
- `r` to redo a previously undone move
- `q` to quit immediately

After each finished game you're prompted to play again (`y`/`n`).

## Files and headers

Top-level source files:
- `play.c` — main program, menu, and game loop(s).
- `gamelogic.c` / `gamelogic.h` — board data structure and game rules (drop, win/draw detection).
- `ui.c` / `ui.h` — terminal UI functions (print board, animation, clear screen).
- `bot.c` / `bot.h` — simple "easy" bot that chooses a random valid column.

Read the headers for function-level details below.

### gamelogic.h

Public API (what other modules use):

```c
#define ROWS 6
#define COLS 7
#define EMPTY '.'

typedef struct {
	char cells[ROWS][COLS];
	char current; // 'A' or 'B'
} Board;

void initializeBoard(Board* g, char turn);
void setCurrent(Board* g, char turn);
int game_in_bounds(int r, int c);
int game_can_drop(const Board* g, int col);
int game_drop(Board* g, int col, char player);
int checkWin(const Board* g, int r, int c, char player);
int checkDraw(const Board* g);
void board_to_string(const Board* g, char out[ROWS*COLS + 1]);
void string_to_board(Board* g, const char* s);
```

Function notes:
- `initializeBoard(Board* g, char turn)` — Fill the board with `EMPTY` and set `current = turn` by calling the `setCurrent` function.
- `void setCurrent(Board* g, char turn)` — Sets the current player of the board to A or B.
- `game_in_bounds(r,c)` — Return non-zero if (r,c) is within the grid.
- `game_can_drop(const Board* g, int col)` — Return the row index (0-based) of where a piece would land if dropped into `col`, or `-1` if the column is full or out of range.
- `game_drop(Board* g, int col, char player)` — If possible, place `player` (`'A'`/`'B'`) into the correct row for `col` and return the row; otherwise return `-1`.
- `checkWin(const Board* g, int r, int c, char player)` — After placing at (r,c), check whether `player` has connected 4 in any direction; returns non-zero on win.
- `checkDraw(const Board* g)` — Returns non-zero if the top row is full (board full → draw).
- `board_to_string` / `string_to_board` — Helpers to serialize/deserialize the board to/from a flat string (ROW-major order). Useful for tests or saving state.

### ui.h

Public API:

```c
typedef struct { int use_color; int delay_ms; } UiOptions;

void ui_clear_screen(void);
void ui_print_board(const Board* g, int use_color);
int ui_drop_with_animation(Board* g, int col, char player, const UiOptions* opt);
```

Function notes:
- `ui_clear_screen()` — Uses ANSI escape codes to clear the terminal.
- `ui_print_board(const Board* g, int use_color)` — Prints the board with optional ANSI color codes for players.
- `ui_drop_with_animation(Board* g, int col, char player, const UiOptions* opt)` — Animates a piece falling down the column by repeatedly printing intermediate states. Returns the landing row index (or -1 if column full). The `UiOptions` lets you set `use_color` and `delay_ms`.

### bot.h

Public API:

```c
int bot_choose_move(const Board* g);
```

Behavior:
- `bot_choose_move` examines the board and returns a 0-based column index to drop into. The current "easy" implementation selects a random valid column uniformly. It returns `-1` if no valid moves are available.

Note: The bot is intentionally simple (random). You can replace the implementation with stronger heuristics or minimax search later.

### play.c (high level)

`play.c` contains the program entry point and uses the above modules to present a menu and run two main modes:

- Human vs human: reuses the existing turn handling, undo/redo, and animation.
- Human vs bot: when it's player B's turn, `bot_choose_move` is called and the move applied; undo/redo still works for human moves performed earlier.

Key helper functions in `play.c` (not necessarily exported):
- `record_move(row,col,player)` — push a move into an internal history buffer for undo/redo.
- `undo_move(Board* G)` / `redo_move(Board* G)` — perform undo/redo using the history buffer and update `G->current`.
- `handle_turn(Board* G, int choice, int use_anim, int anim_ms)` — prompt the current human player for input and perform their action (drop/undo/redo/quit). The `choice` parameter is the top-level menu choice (`'1'` for 1v1, `'2'` for 1vBot) so undo/redo can act on single or paired moves. Returns codes to indicate game-over or quit.
- `do_drop(Board* G, int col0, int use_anim, int anim_ms)` — helper that calls `ui_drop_with_animation` or `game_drop` depending on `use_anim`.

## Build notes

- Build must include all .c files. Example:

```bash
gcc -Wall -Werror -o connect4 play.c gamelogic.c ui.c bot.c
```
- Or use the provided makefile.
- If you add new files, include them on the command line or update your Makefile.

## Runtime examples

Start and pick mode 1 (1v1):

1. Run `./connect4`.
2. Press `1` + Enter.
3. Players alternate entering column numbers or `u`/`r` for undo/redo.

Start and pick mode 2 (1 vs bot):

1. Run `./connect4`.
2. Press `2` + Enter.
3. You are player `A` (yellow); the bot plays `B` (red). On bot turns the program will show the animated drop and then return control to you.

## Extending the project

- To improve the bot, replace `bot.c` with a minimax or heuristic-based move chooser.
- Make the game `multiplayer` through the `network`.
- Add a save/load feature using `board_to_string` / `string_to_board`.

## Credits and license

This code was created as a course project.