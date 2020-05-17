
#include "movegen.h"

int move_is_legal(BOARD *b, int btm, MOVE *m) {

    MOVE m2[64];
    int i;
    int n;

    n = movegen(b, btm, m2);

    for(i = 0; i < n; i++) {
        if(movecmp(&m2[i], m))
            return 1;
    }

    return 0;

}

