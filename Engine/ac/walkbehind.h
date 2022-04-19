//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__WALKBEHIND_H
#define __AGS_EE_AC__WALKBEHIND_H

// A method of rendering walkbehinds on screen:
// DrawAsSeparateSprite - draws whole walkbehind as a sprite;
//     this method is most simple and is optimal for 3D renderers.
// DrawOverCharSprite - turns parts of the character and object sprites
//     transparent when they are covered by walkbehind (walkbehind itself
//     is not drawn separately in this case);
//     this method is optimized for software render.
enum WalkBehindMethodEnum
{
    DrawAsSeparateSprite,
    DrawOverCharSprite,
};

void update_walk_behind_images();
void recache_walk_behinds ();

#endif // __AGS_EE_AC__WALKBEHIND_H
