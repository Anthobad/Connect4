#include "gamelogic.h"
#include <string.h>
#include <unistd.h>
#include <stdlib.h>


static int count_dir(const Board* g, int r, int c, int dr, int dc, char p){
    int cnt=0, rr=r, cc=c;
    while (rr>=0 && rr<ROWS && cc>=0 && cc<COLS && g->cells[rr][cc]==p){ 
	cnt++; 
	rr+=dr; 
	cc+=dc;
    }
    return cnt;
}

void game_init(Board* g){
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) g->cells[r][c]=EMPTY;
    g->current='A';
}

int game_in_bounds(int r,int c){ return r>=0 && r<ROWS && c>=0 && c<COLS; }

int game_can_drop(const Board* g, int col){
    if(col<0 || col>=COLS) return -1;
    for(int r=ROWS-1;r>=0;--r){
        if(g->cells[r][col]==EMPTY) return r;
    }
    return -1;
}

int game_drop(Board* g, int col, char player){
    int row = game_can_drop(g, col);
    if(row==-1) return -1;
    g->cells[row][col]=player;
    return row;
}

int game_is_win_at(const Board* g, int r, int c, char p){
    const int dirs[4][2]={{0,1},{1,0},{1,1},{-1,1}};
    for(int i=0;i<4;i++){
        int dr=dirs[i][0], dc=dirs[i][1];
        int tot = count_dir(g,r,c,dr,dc,p)+count_dir(g,r,c,-dr,-dc,p)-1;
        if(tot>=4) return 1;
    }
    return 0;
}

int game_is_draw(const Board* g){
    for(int c=0;c<COLS;c++) if(g->cells[0][c]==EMPTY) return 0;
    return 1;
}

void game_print(const Board* g){
    puts("");
    printf("   ");
    for(int c=0;c<COLS;c++) printf("%d ", c+1);
    puts("\n  +---------------+");
    for(int r=0;r<ROWS;r++){
        printf("%d | ", ROWS-r);
        for(int c=0;c<COLS;c++){
            printf("%c ", g->cells[r][c]);
        }
        puts("|");
    }
    puts("  +---------------+");
}

void game_flatten(const Board* g, char out[ROWS*COLS+1]){
    int k=0;
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) out[k++]=g->cells[r][c];
    out[k]='\0';
}

void game_unflatten(Board* g, const char* s){
    int k=0;
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) g->cells[r][c]=s[k++];
}

// ----- animation helpers -----
static void clear_screen(){ fputs("\033[H\033[J", stdout); }

static void print_with_colors(const Board* g, int use_color){
    if(!use_color){ game_print(g); return; }
    puts("");
    printf("   ");
    for(int c=0;c<COLS;c++) printf("%d ", c+1);
    puts("\n  +---------------+");
    for(int r=0;r<ROWS;r++){
        printf("%d | ", ROWS-r);
        for(int c=0;c<COLS;c++){
            char ch = g->cells[r][c];
            if(ch=='A')      fputs("\033[33mA\033[0m ", stdout);
            else if(ch=='B') fputs("\033[31mB\033[0m ", stdout);
            else             printf("%c ", ch);
        }
        puts("|");
    }
    puts("  +---------------+");
}

int game_drop_with_animation(Board* g, int col, char player, int use_color, int delay_ms){
    int landing = game_can_drop(g, col);
    if(landing==-1) return -1;

    Board temp = *g;
    for(int r=0;r<=landing;r++){
        if(temp.cells[r][col]!=EMPTY) continue;
        temp = *g;
        temp.cells[r][col]=player;
        clear_screen();
        print_with_colors(&temp, use_color);
        fflush(stdout);
        usleep((use_color? delay_ms : delay_ms)*1000);
    }
    g->cells[landing][col]=player;
    clear_screen();
    print_with_colors(g, use_color);
    return landing;
}
