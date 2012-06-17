#ifndef __AC_CHARACTER_H
#define __AC_CHARACTER_H

#include "acrun/ac_rundefines.h"
#include "ac/ac_characterinfo.h"
#include "acrun/ac_scriptobject.h"

struct CharacterExtras {
    // UGLY UGLY UGLY!! The CharacterInfo struct size is fixed because it's
    // used in the scripts, therefore overflowing stuff has to go here
    short invorder[MAX_INVORDER];
    short invorder_count;
    short width,height;
    short zoom;
    short xwas, ywas;
    short tint_r, tint_g;
    short tint_b, tint_level;
    short tint_light;
    char  process_idle_this_time;
    char  slow_move_counter;
    short animwait;
};

void Character_Walk(CharacterInfo *chaa, int x, int y, int blocking, int direct);
void Character_FaceLocation(CharacterInfo *char1, int xx, int yy, int blockingStyle);
void Character_StopMoving(CharacterInfo *charp);
int GetCharacterWidth(int ww);
int GetCharacterHeight(int charid);
void Character_UnlockView(CharacterInfo *chaa);
void Character_SetManualScaling(CharacterInfo *chaa, int yesorno);
void setup_player_character(int charid);
void Character_SetIgnoreScaling(CharacterInfo *chaa, int yesorno);
void animate_character(CharacterInfo *chap, int loopn,int sppd,int rept, int noidleoverride = 0, int direction = 0);
int is_valid_character(int newchar);
int Character_IsCollidingWithObject(CharacterInfo *chin, ScriptObject *objid);
int Character_IsCollidingWithChar(CharacterInfo *char1, CharacterInfo *char2);
void SetCharacterSpeechView (int chaa, int vii);
void Character_SetOption(CharacterInfo *chaa, int flag, int yesorno);
void MoveCharacterToHotspot(int chaa,int hotsp);

extern CharacterInfo*playerchar;
extern CharacterExtras *charextra;
extern long _sc_PlayerCharPtr;

#endif // __AC_CHARACTER_H