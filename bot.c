#include "bot.h"
#include <stdlib.h>

int bot_choose_move(const Board* g) {
    int valid[COLS];
    int n = 0;
    for (int c = 0; c < COLS; c++) {
        if (g->cells[0][c] == EMPTY) {
            valid[n++] = c;
        }
    }
    if (n == 0)
        return -1;
    int i = rand() % n;
    return valid[i];
}
