
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>

#include "board.h"
#include "movegen.h"
#include "search.h"
#include "move.h"
#include "legal.h"
#include "perft.h"
#include "square.h"
#include "makemove.h"
#include "fen.h"
#include "eval.h"


int main() {

    BOARD b; 
    int computer_secs_per_move;
    int btm;
    int flip;
    MOVE mymove;

    char s[256];
    int i;
    int n;
    MOVE m[64];
	char *endptr;
    
    newboard(&b);
    btm = 1;
    computer_secs_per_move = 5;
    flip = 0;
    
    while(1) {

        printboard(&b, flip);

        printf("%s %s\n",  btm ? "Black" : "White", "to move");
        
        printf("%d %s\n", computer_secs_per_move, "secs per move"); 

        printf("%s\n", "Your move ? (type help for menu)");

		gets(s);

        if(!strcmp(s, "help")) {
            printf("%s\n", "help - this menu");
            printf("%s\n", "newgame - start a new game");
            printf("%s\n", "quit - quit program");
            printf("%s\n", "invert - invert board");
            printf("%s\n", "+ - increase computer thinking time by 5 secs");
            printf("%s\n", "- - decrease computer thinking time by 5 secs");
            printf("%s\n", "move - force computer to play your move");
            printf("%s\n", "perft N - perform a perft test to depth N");
			printf("%s\n", "FEN s - set up a position (e.g., FEN B:BK1,K2,14:W8,15)");
            printf("%s\n", "movegen - display all legal moves for the current position");
            printf("%s\n", "eval - evaluate position");
            printf("%s\n", "Press any key to continue...");
            while(!_kbhit());
        }
        else if(!strcmp(s, "newgame")) {
            newboard(&b);
            btm = 1;
        }
        else if(!strcmp(s, "quit")) { 
            exit(0);
        }
        else if(!strcmp(s, "invert")) {
            flip = !flip;   
        }
        else if(!strcmp(s, "+")) {
            computer_secs_per_move += 5;
        }
        else if(!strcmp(s, "-")) {
            if(computer_secs_per_move > 5) {
                computer_secs_per_move -= 5;
            }
        }
        else if(!strcmp(s, "move")) {
            if(search(&b, btm, computer_secs_per_move, &mymove)) {
                makemove(&b, &mymove);
                btm = !btm;
            }
        }
        else if(!strncmp(s, "perft", 5)) {
    		n = strtol(&s[5], &endptr, 10);  
            for(i = 1; i <= n; i++) {
                printf("depth %d nodes %d\n", i, perft(&b, btm, i));
            }
        }
		else if(!strncmp(s, "FEN ", 4)) {
			parse_fen(&s[4], &b, &btm);
        }
        else if(!strcmp(s, "movegen")) {
            n = movegen(&b, btm, m);
            for(i = 0; i < n; i++) {
                printf("%s\n", move_to_string(&m[i], s));
            }
        }
		else if(!strcmp(s, "eval")) {
            printf("score: %d\n", eval(&b, btm));
        }
        else if(move_from_string(s, &mymove)) {
            if(move_is_legal(&b, btm, &mymove)) {
                makemove(&b, &mymove);
                btm = !btm;
                printboard(&b, flip);
                if(search(&b, btm, computer_secs_per_move, &mymove)) {
                    makemove(&b, &mymove);
                    btm = !btm;
                }
            }
        }
    }
}

