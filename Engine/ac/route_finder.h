#ifndef __AC_ROUTEFND_H
#define __AC_ROUTEFND_H

#include "ac/movelist.h"

void calculate_move_stage(MoveList * mlsp, int aaa);
int can_see_from(int x1, int y1, int x2, int y2);

// Why is this in routefnd?
void print_welcome_text(char *verno, char *aciverno);

void init_pathfinder();
void set_route_move_speed(int speed_x, int speed_y);
int find_route(short srcx, short srcy, short xx, short yy, Common::IBitmap *onscreen, int movlst, int nocross =
               0, int ignore_walls = 0);

extern Common::IBitmap *wallscreen;
extern int lastcx, lastcy;

#endif // __AC_ROUTEFND_H