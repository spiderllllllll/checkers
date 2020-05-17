
#include "perft.h"
#include "movegen.h"
#include "makemove.h"

int perft(BOARD *b, int btm, int depth) {

    MOVE m[64];
    BOARD b2;
    int sum;
    int i;
    int n;
    
    if(depth == 0)
        return 1;

    n = movegen(b, btm, m); 
    
    sum = 0;

    for(i = 0; i < n; i++) {
        makemove(copyboard(&b2, b), &m[i]);
        sum = sum + perft(&b2, !btm, depth-1);
    }

    return sum;

}

