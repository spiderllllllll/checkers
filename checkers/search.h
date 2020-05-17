
#ifndef SEARCH_H
#define SEARCH_H

#include "board.h"
#include "move.h"

MOVE *search(BOARD *b, int btm, int maxtime, MOVE *best);

#endif
