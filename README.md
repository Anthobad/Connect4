# Connect 4 - Console Game in C

This repository contains a small console-based Connect Four implementation in C.
It supports **human vs human**, **easy/medium bots**, **undo/redo**, and optional **drop animations** and **color output**.

This README documents how to build and run the game, explains public header files and functions, and describes gameplay controls, the bot’s behavior, and the updated modular structure.

---

## Table of Contents

* [Gameplay](#gameplay)
* [Features](#features)
* [Requirements](#requirements)
* [Compilation](#compilation)
* [Quick start](#quick-start)
* [Controls](#controls)
* [Files and headers](#files-and-headers)
* [Module overview](#module-overview)
* [Build notes](#build-notes)
* [Runtime examples](#runtime-examples)
* [Extending the project](#extending-the-project)
* [Credits and license](#credits-and-license)

---

## Gameplay

Players take turns dropping pieces into a 7-column, 6-row grid.
The first player to connect **4 pieces** in any row, column, or diagonal wins.
If the board fills up without a winner, the game ends in a draw.

---

## Features

* Undo and redo previous moves
* Optional animated piece drops
* Color-coded board output
* Console-based, lightweight, and fast
* Cross-platform (Linux, Windows)
* **Easy and Medium Bots**:

  * Easy → random valid move
  * Medium → **2-step lookahead** (blocks immediate wins and avoids giving instant wins next turn)
* **Reworked Main Mene**

  * Play Directly (Human vs Human)
  * Play vs Bot (Easy / Medium)
  * About Game
  * Quit
* Modular design: separated into logical subsystems for clarity and scalability
* Future-ready for multithreading or LAN play

---

## Requirements

* GCC (tested on Alpine Linux, Ubuntu, and MinGW on Windows)
* POSIX-compliant terminal (for ANSI colors and clear screen codes)

---

## Compilation

Clone the repository:

```bash
git clone https://github.com/your-username/Connect4.git
cd Connect4
```

### Build manually (example with GCC)

```bash
gcc -std=c11 -Wall -Werror -o connect4 \
	play.c gamelogic.c ui.c bot.c history.c input.c controller.c
```

Or use the provided Makefile:

```bash
make
```

---

## Quick start

Run the executable:

```bash
./connect4         # with animations and colors
./connect4 --no-anim   # disable falling animation
```

---

## Controls

### Main Menu

* `1` → Play Human vs Human (1v1)
* `2` → Play vs Bot
* `3` → About Game
* `q` → Quit

### Bot Difficulty Menu

* `1` → Easy Bot
* `2` → Medium Bot
* `b` → Back to Main Menu

### During Gameplay

* `1`–`7` → Drop a piece into that column
* `u` → Undo last move
* `r` → Redo an undone move
* `q` → Quit immediately

After each finished game, you’re prompted to play again (`y`/`n`).

---

## Files and headers

Top-level source files:

* `play.c` — entry point, main menu, and game initialization.
* `controller.c` / `controller.h` — manages flow between UI and gameplay logic (human vs human, vs bot).
* `gamelogic.c` / `gamelogic.h` — board rules (drop, win/draw detection).
* `ui.c` / `ui.h` — all terminal UI (board display, animation, menus).
* `bot.c` / `bot.h` — easy and medium bot implementations.
* `history.c` / `history.h` — undo/redo stack.
* `input.c` / `input.h` — parses player input (columns, undo/redo, quit).

---

## Module overview

### gamelogic.h

```c
#define ROWS 6
#define COLS 7
#define EMPTY '.'

typedef struct {
	char cells[ROWS][COLS];
	char current;
} Board;

void initializeBoard(Board* g, char turn);
int game_can_drop(const Board* g, int col);
int game_drop(Board* g, int col, char player);
int checkWin(const Board* g, int r, int c, char player);
int checkDraw(const Board* g);
```

---

### ui.h

```c
void ui_clear_screen(void);
void ui_print_board(const Board* g, int use_color);
int ui_drop_with_animation(Board* g, int col, char player, int delay_ms);
int ui_main_menu(void);
int ui_bot_menu(void);
```

Handles:

* Board rendering (color / non-color)
* Falling animation
* Menu system (main menu, bot difficulty menu)

---

### bot.h

```c
int bot_choose_move(const Board* g);
int bot_choose_move_medium(const Board* g);
```

**Easy bot**
Chooses a random valid column.

**Medium bot (new)**
Checks if the player can win next turn and blocks it,
and avoids playing into an immediate loss.

---

### history.h

```c
void history_reset(void);
void history_record_move(int row, int col, char player);
int history_undo(Board* G);
int history_redo(Board* G);
```

Implements an **undo/redo system** that tracks every move.

---

### input.h

```c
int read_line(char* buf, int size);
int parse_action(const char* s, int* out_col);
```

Handles input parsing for:

* Columns
* Undo/Redo commands
* Quitting the game

---

### controller.h

```c
void run_human_vs_human(int use_anim, int anim_ms);
void run_vs_bot(int use_anim, int anim_ms, int difficulty);
```

Encapsulates high-level game control logic between UI, game state, and player turns.

---

### play.c

Contains `main()` — the overall program entry point that initializes settings and calls controller functions based on the menu selections.

---

## Build notes

* Always include **all .c files** in the build:

```bash
gcc -Wall -Werror -o connect4 play.c gamelogic.c ui.c bot.c history.c input.c controller.c
```

* On Windows (MinGW):

```bash
gcc -std=c11 -Wall -Wextra -o connect4.exe play.c gamelogic.c ui.c bot.c history.c input.c controller.c
```

---

## Runtime examples

### Human vs Human

1. Run `./connect4`
2. Select `1` → Play directly
3. Players alternate moves or use `u` / `r` for undo/redo

### Human vs Bot

1. Run `./connect4`
2. Select `2` → Play vs Bot
3. Choose difficulty:

   * `1` Easy
   * `2` Medium
4. Player `A` goes first; bot plays as `B`

---

## Extending the project

* Add a **Hard bot** (minimax, alpha-beta pruning, or scoring heuristics)
* Implement **multithreading** for bot calculations and animations
* Add **LAN/Online mode** (structure already supports expansion)

---

## Credits and license

Developed by 
**Anthony Badawi (AnthoBad)**
**Moustafa Hoteit (IamTufa)**
AUB – CMPS 241 Systems Programming course Project
Fall 2025/26

