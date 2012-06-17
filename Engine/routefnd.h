#ifndef __AC_ROUTEFND_H
#define __AC_ROUTEFND_H

#include "ac/ac_move.h"

void calculate_move_stage(MoveList * mlsp, int aaa);
int can_see_from(int x1, int y1, int x2, int y2);

// Why is this in routefnd?
void print_welcome_text(char *verno, char *aciverno);
void init_pathfinder();

extern block wallscreen;
extern int lastcx, lastcy;

#endif // __AC_ROUTEFND_H