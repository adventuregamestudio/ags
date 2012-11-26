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
// Game version constants and information
//
//=============================================================================

#ifndef __AGS_CN_AC__GAMEVERSION_H
#define __AGS_CN_AC__GAMEVERSION_H

/*

Game data versions and changes:
-------------------------------

12 : 2.3 + 2.4

Versions above are incompatible at the moment.

19 : 2.5.1
22 : 2.5.5

Variable number of sprites.
24 : 2.5.6
25 : 2.6.0

Encrypted global messages and dialogs.
26 : 2.6.1

Wait() must be called with parameter > 0
GetRegionAt() clips the input values to the screen size
Color 0 now means transparent instead of black for text windows
27 : 2.6.2

Script modules. Fixes bug in the inventory display.
Clickable GUI is selected with regard for the drawing order.
Pointer to the "player" variable is now accessed via a dynamic object.
31 : 2.7.0
32 : 2.7.2

Interactions are now scripts. The number for "not set" changed from 0 to -1 for
a lot of variables (views, sounds).
37 : 3.0 + 3.1.0

Dialogs are now scripts. New character animation speed.
39 : 3.1.1
40 : 3.1.2

Audio clips
41 : 3.2.0
42 : 3.2.1

*/

enum GameDataVersion
{
    kGameVersion_Undefined      = 0,
    kGameVersion_230            = 12,
    kGameVersion_240            = 12,
    kGameVersion_250            = 18,
    kGameVersion_251            = 19,
    kGameVersion_255            = 22,
    kGameVersion_256            = 24,
    kGameVersion_260            = 25,
    kGameVersion_261            = 26,
    kGameVersion_262            = 27,
    kGameVersion_270            = 31,
    kGameVersion_272            = 32,
    kGameVersion_pre300         = 36, // exact version unknown
    kGameVersion_300            = 37,
    kGameVersion_310            = 37,
    kGameVersion_311            = 39,
    kGameVersion_312            = 40,
    kGameVersion_320            = 41,
    kGameVersion_321            = 42,
    kGameVersion_Current        = kGameVersion_321
};

extern GameDataVersion loaded_game_file_version;

#endif // __AGS_CN_AC__GAMEVERSION_H
