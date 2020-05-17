
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <conio.h>

#include "movegen.h"
#include "terminal.h"
#include "square.h"
#include "eval.h"
#include "makemove.h"
#include "hash.h"

static void rate_moves(MOVE m[], int n, int ttmove, int ply, int rate[]);
static int pick(int score[], int n, int *k);
static int quiscence(BOARD *b, int btm, int alpha, int beta, int ply);
static int negamax(BOARD *b, int btm, int alpha, int beta, int depth, int ply, MOVE *rootbest);
static char *pv_to_string(char *s);
static char *short_move_to_string(int move, char *s);
static int get_short_move(MOVE *m);
static void store(BOARD *key, int btm, int depth, int score, int type, int move);
static int probe(BOARD *key, int btm, int depth, int *score, int *type, int *move);
static void setrep(BOARD *key, int btm);
static int repchk(BOARD *key, int btm);
static void delrep(BOARD *key, int btm);

typedef struct {
    int occupied;
    BOARD key;
    int score;
    int depth;
    int type;
    int move;
} TTBUCKET;

typedef struct {
    int occupied;
    BOARD key;
} RTBUCKET;

#define TTSIZE          (1 << 20)
#define RTSIZE          2 * (1 << 10)

#define MAXDEPTH        64
#define MAXPLY          64

#define INFINITY        1000000

#define TTMOVE_SCORE    3000
#define PVMOVE_SCORE    2000
#define KILLER_1_SCORE  1000
#define KILLER_2_SCORE  500

#define UPPERBOUND  0
#define LOWERBOUND  1
#define EXACT       2

#define SRC(x)  (((x) >> 5) & 0x1f) 
#define DST(x)  ((x) & 0x1f)

static TTBUCKET *TT;
static RTBUCKET *RT;

static int pv[MAXPLY][MAXPLY];
static int killer[MAXPLY][2];
static int pvlen[MAXPLY];

static int terminate;
static int nodecount;
static time_t endtime;

MOVE *search(BOARD *b, int btm, int maxtime, MOVE *best) {
    
    MOVE m[64];
    MOVE rootbest; 
    char s[512];
    int depth;
    int score;
    int i;
    int j;
    int n;

    n = movegen(b, btm, m);
    
    if(n == 0) {
        return 0;
    }

    *best = m[0];
    
    if(n == 1) {
        return best;
    }

    if((TT = (TTBUCKET *) malloc(sizeof(TTBUCKET)*TTSIZE)) != 0) {
        memset(TT, 0, sizeof(TTBUCKET)*TTSIZE);
    }
    else {
        printf("%s \n", "Memory allocation error");
        exit(1);
    }

    if((RT = (RTBUCKET *) malloc(sizeof(RTBUCKET)*RTSIZE)) != 0) {
        memset(RT, 0, sizeof(RTBUCKET)*RTSIZE);
    }
    else {
        printf("%s \n", "Memory allocation error");
        exit(1);
    }

    for(i = 0; i < MAXPLY; i++) {
    for(j = 0; j < MAXPLY; j++) {
        pv[i][j] = 0;           
    }
    }
        
    for(i = 0; i < MAXPLY; i++) {
        pvlen[i] = 0;
    }
    
    for(i = 0; i < MAXPLY; i++) {
        killer[i][0] = 0;
        killer[i][1] = 0;
    }

    terminate = 0;
    nodecount = 0;
    endtime = time(0) + maxtime;

    for(depth = 2; depth <= MAXDEPTH; depth += 2) {
        score = negamax(b, btm, -INFINITY, +INFINITY, depth, 0, &rootbest);
        if(terminate) {
            break;
        }
        else {
            *best = rootbest;       
            printf("Node Count %d Depth %d Score %d\n PV %s\n", nodecount, depth, score, pv_to_string(s));
        }
    }

    free(TT);
    free(RT);


    return best;
}



static char *short_move_to_string(int move, char *s) {

    char temp[16];
    
    s[0] = '\0';

    strcat(s, _itoa(sqr2brd(SRC(move)), temp, 10));
    strcat(s, "-");
    strcat(s, _itoa(sqr2brd(DST(move)), temp, 10));
    
    return s;
}



static int get_short_move(MOVE *m) {
    return (m->path[0] << 5) | m->path[m->pathlen-1];
}



static char *pv_to_string(char *s) {
        
    int i;
    char temp[16];

    s[0] = '\0';

    for(i = 0; i < pvlen[0]; i++) {
        strcat(s, short_move_to_string(pv[0][i], temp));
        strcat(s, ";");
    }

    return s;
    
}

static int negamax(BOARD *b, int btm, int alpha, int beta, int depth, int ply, MOVE *rootbest) {

    MOVE m[64];
    int rate[64];
    int original_alpha;
    int score;
    int k;
    int i;
    int n;
    int best;
    int ttmove;
    int RESULT;
    int type;
    BOARD b2;

    if((++nodecount & 0xfff) == 0 && time(0) >= endtime) {
        terminate = 1;
        return 0;
    }

    if(terminal(b, btm)) {
        if(-INFINITY + ply <= alpha) {
            return alpha;
        }
        else if(-INFINITY + ply >= beta) {
            return beta;
        }
        else {
            return -INFINITY + ply;
        }
    }

    if(repchk(b, btm)) {
        if(0 <= alpha)
            return alpha;
        else if(0 >= beta)
            return beta;
        else 
            return 0;
    }
    
    RESULT = probe(b, btm, depth, &score, &type, &ttmove); 

	if(RESULT == 2) {
		if(alpha + 1 == beta) {
			if(type == LOWERBOUND) {
				if(score >= beta) {
					return beta;
				}
			}
			else if(type == UPPERBOUND) {
				if(score <= alpha) {
					return alpha;
				}
			}
			else if(score <= alpha) {
				return alpha;
			}
			else {
				return beta;
			}
		}
    }
    else if(RESULT == 0) {
        ttmove = 0;
    }

    if(depth == 0) {
        return quiscence(b, btm, alpha, beta, ply);
    }

    if(depth > 3 && alpha + 1 == beta) {
        if(beta < (INFINITY - MAXPLY - 180)) {
            if((score = negamax(b, btm, beta + 180 - 1, beta + 180, depth / 2, ply, rootbest)) >= beta + 180) {
                return beta;
            }
        }
    }
    
    setrep(b, btm);
    
    n = movegen(b, btm, m);

    rate_moves(m, n, ttmove, ply, rate);

    original_alpha = alpha;

    for(i = 0; i < n; i++) {
        pick(rate, n, &k);
        makemove(copyboard(&b2, b), &m[k]);
        if(i == 0) {
            score = -negamax(&b2, !btm, -beta, -alpha, depth - 1, ply + 1, rootbest);
        }
        else {
            score = -negamax(&b2, !btm, -(alpha + 1), -alpha, depth - 1, ply + 1, rootbest);
            if(score > alpha && score < beta) {
                score = -negamax(&b2, !btm, -beta, -alpha, depth - 1, ply + 1, rootbest);
            }
        }
        if(terminate) {
            delrep(b, btm);
            return 0;
        }
        if(score > alpha) {
            alpha = score;     
            best = get_short_move(&m[k]); 
            if(ply == 0) 
                *rootbest = m[k];
            if(alpha >= beta)   
                break;
        }
    }

    delrep(b, btm);
    
    if(alpha >= beta) {
        if(ply < MAXPLY) {
            killer[ply][1] = killer[ply][0];
            killer[ply][0] = best;
        }
    }
    else if(alpha > original_alpha) {
        if(ply < MAXPLY) {
            pv[ply][ply] = best;
            if(ply < (MAXPLY - 1)) {
                for(i = 1; i <= pvlen[ply+1]; i++) 
                    pv[ply][ply+i] = pv[ply+1][ply+i];
                pvlen[ply] = pvlen[ply+1] + 1;
            }
        }
    }

    if(alpha >= beta) {
        store(b, btm, depth, beta, LOWERBOUND, best); 
        return beta;
    }
    else if(alpha > original_alpha) {
        store(b, btm, depth, alpha, EXACT, best); 
        return alpha;
    }
    else {
        store(b, btm, depth, alpha, UPPERBOUND, 0); 
        return alpha;
    }

}

static int pick(int score[], int n, int *k) {

    int bestscore;
    int bestindex; 
    int i;
    
    bestscore = -1;
    bestindex = -1;
    
    for(i = 0; i < n; i++) {
        if(score[i] > bestscore) {
            bestscore = score[i];
            bestindex = i;
        }
    }
    
    if(bestindex != -1) {
        score[bestindex] = -1;
        *k = bestindex;
        return 1;
    }
    
    return 0;
}


static int quiscence(BOARD *b, int btm, int alpha, int beta, int ply) {

    MOVE m[64];
    int i;
    int n;
    int score;
    BOARD b2;
        
    if((n = getjumps(b, btm, m)) == 0) {
        if(terminal(b, btm)) {
            if(-INFINITY + ply <= alpha) {
                return alpha;
            }
            else if(-INFINITY + ply >= beta) {
                return beta;
            }
            else {
                return -INFINITY + ply;
            }
        }
        score = eval(b, btm);
        if(score <= alpha)
            return alpha;
        else if(score >= beta)
            return beta;
        else 
            return score;
    }

    for(i = 0; i < n; i++) {
        makemove(copyboard(&b2, b), &m[i]);
        score = -quiscence(&b2, !btm, -beta, -alpha, ply + 1);
        if(score > alpha) {
            alpha = score;
            if(alpha >= beta)
                return beta;
        }
    }
    
    return alpha;

}


static void rate_moves(MOVE m[], int n, int ttmove, int ply, int rate[]) {
    
    int i;

    for(i = 0; i < n; i++) {
        if(get_short_move(&m[i]) == ttmove)  
            rate[i] = TTMOVE_SCORE;
        else if(get_short_move(&m[i]) == pv[0][ply])  
            rate[i] = PVMOVE_SCORE;
        else if(get_short_move(&m[i]) == killer[ply][0]) 
            rate[i] = KILLER_1_SCORE; 
        else if(get_short_move(&m[i]) == killer[ply][1]) 
            rate[i] = KILLER_2_SCORE;
        else 
            rate[i] = 0;
    }
}


static void store(BOARD *key, int btm, int depth, int score, int type, int move) {
    
    unsigned int i;
    
    if(btm) {
        i = (2 * hash(key) + 0) & (TTSIZE - 1);
    }
    else {
        i = (2 * hash(key) + 1) & (TTSIZE - 1);
    }

    if(!TT[i].occupied || depth >= TT[i].depth) {
        TT[i].occupied = 1;
        TT[i].key = *key;
        TT[i].depth = depth;
        TT[i].score = score;
        TT[i].type = type;
        TT[i].move = move;

    }

}


static int probe(BOARD *key, int btm, int depth, int *score, int *type, int *move) {

    unsigned int i;
    
    if(btm) {
        i = (2 * hash(key) + 0) & (TTSIZE - 1);
    }
    else {
        i = (2 * hash(key) + 1) & (TTSIZE - 1);
    }

    if(TT[i].occupied && boardequals(&TT[i].key, key)) {
        *move = TT[i].move;
        if(TT[i].depth >= depth) {
            *score = TT[i].score;
            *type = TT[i].type;
            return 2;
        }
        return 1;
    }
    return 0;
}


static void setrep(BOARD *key, int btm) {
    
    unsigned int i;
    
    if(btm) {
        i = (2 * hash(key) + 0) & (RTSIZE - 1);
    }
    else {
        i = (2 * hash(key) + 1) & (RTSIZE - 1);
    }
    
    for(;;) {
        if(!RT[i].occupied) {
            RT[i].occupied = 1;
            RT[i].key = *key;
            return;
        }
        i = (i + 2) & (RTSIZE - 1);
    }

}


static int repchk(BOARD *key, int btm) {
    
    unsigned int i;

    if(btm) {
        i = (2 * hash(key) + 0) & (RTSIZE - 1);
    }
    else {
        i = (2 * hash(key) + 1) & (RTSIZE - 1);
    }

    while(RT[i].occupied) {
		
		if(RT[i].key.black == key->black && \
		   RT[i].key.white == key->white && \
		   RT[i].key.kings == key->kings) 
				return 1;
		
		i = (i + 2) & (RTSIZE - 1);
    
	}

    return 0;
}


static void delrep(BOARD *key, int btm) {
    
    unsigned int i;

    if(btm) {
        i = (2 * hash(key) + 0) & (RTSIZE - 1);
    }
    else {
        i = (2 * hash(key) + 1) & (RTSIZE - 1);
    }

    while(RT[i].occupied) {
		
		if(RT[i].key.black == key->black && \
		   RT[i].key.white == key->white && \
		   RT[i].key.kings == key->kings) {
				RT[i].occupied = 0;
				return;
        }

        i = (i + 2) & (RTSIZE - 1);
    }

}

