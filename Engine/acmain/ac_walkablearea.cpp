
#include "acmain/ac_maindefines.h"



void RemoveWalkableArea(int areanum) {
  if ((areanum<1) | (areanum>15))
    quit("!RemoveWalkableArea: invalid area number specified (1-15).");
  play.walkable_areas_on[areanum]=0;
  redo_walkable_areas();
  DEBUG_CONSOLE("Walkable area %d removed", areanum);
}

void RestoreWalkableArea(int areanum) {
  if ((areanum<1) | (areanum>15))
    quit("!RestoreWalkableArea: invalid area number specified (1-15).");
  play.walkable_areas_on[areanum]=1;
  redo_walkable_areas();
  DEBUG_CONSOLE("Walkable area %d restored", areanum);
}


int GetWalkableAreaAt(int xxx,int yyy) {
  xxx += divide_down_coordinate(offsetx);
  yyy += divide_down_coordinate(offsety);
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return 0;
  int result = get_walkable_area_pixel(xxx, yyy);
  if (result <= 0)
    return 0;
  return result;
}
