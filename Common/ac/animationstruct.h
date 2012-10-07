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

#ifndef __AC_ANIMATIONSTRUCT_H
#define __AC_ANIMATIONSTRUCT_H

#define AE_WAITFLAG   0x80000000
#define MAXANIMSTAGES 10
struct AnimationStruct {
    int   x, y;
    int   data;
    int   object;
    int   speed;
    char  action;
    char  wait;
    AnimationStruct() { action = 0; object = 0; wait = 1; speed = 5; }
};

struct FullAnimation {
    AnimationStruct stage[MAXANIMSTAGES];
    int             numstages;
    FullAnimation() { numstages = 0; }
};

#endif // __AC_ANIMATIONSTRUCT_H