
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__WALKBEHIND_H
#define __AGS_EE_AC__WALKBEHIND_H

enum WalkBehindMethodEnum
{
    DrawOverCharSprite,
    DrawAsSeparateSprite,
    DrawAsSeparateCharSprite
};

void update_walk_behind_images();
void recache_walk_behinds ();

#endif // __AGS_EE_AC__WALKBEHIND_H
