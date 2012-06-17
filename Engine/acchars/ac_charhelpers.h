#ifndef __AC_CHARHELPERS_H
#define __AC_CHARHELPERS_H

#include "ac/ac_characterinfo.h"
#include "ac/ac_move.h"

#define CHECK_DIAGONAL(maindir,othdir,codea,codeb) \
    if (no_diagonal) ;\
  else if (abs(maindir) > abs(othdir) / 2) {\
  if (maindir < 0) useloop=codea;\
    else useloop=codeb;\
}

extern int current_screen_resolution_multiplier;

void StopMoving(int chaa);
void ReleaseCharacterView(int chat);
void walk_character(int chac,int tox,int toy,int ignwal, bool autoWalkAnims);
int find_looporder_index (int curloop);
// returns 0 to use diagonal, 1 to not
int useDiagonal (CharacterInfo *char1);
// returns 1 normally, or 0 if they only have horizontal animations
int hasUpDownLoops(CharacterInfo *char1);
void start_character_turning (CharacterInfo *chinf, int useloop, int no_diagonal);
void fix_player_sprite(MoveList*cmls,CharacterInfo*chinf);
// Check whether two characters have walked into each other
int has_hit_another_character(int sourceChar);
int doNextCharMoveStep (int aa, CharacterInfo *chi);
int find_nearest_walkable_area_within(int *xx, int *yy, int range, int step);
void find_nearest_walkable_area (int *xx, int *yy);
void MoveToWalkableArea(int charid);
void FaceLocation(int cha, int xx, int yy);
void FaceCharacter(int cha,int toface);

#endif // __AC_CHARHELPERS_H