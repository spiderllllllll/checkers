
#include "hash.h"

unsigned int hash(BOARD *key) {
    unsigned int a,b,c;
    a = key->black + 0xcafecafe;
    b = key->white + 0xcafecafe;
    c = key->kings + 0xcafecafe;
    mix(a,b,c);
    return c;
}



    