//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// AGS Plugin interface header file.
// Engine events definition.
//
// If you are writing a plugin, include agsplugin.h instead.
//
//=============================================================================
#ifndef _AGS_PLUGIN_EVTS_H
#define _AGS_PLUGIN_EVTS_H

// Below are interface 3 and later
#define AGSE_KEYPRESS        0x01
#define AGSE_MOUSECLICK      0x02
#define AGSE_POSTSCREENDRAW  0x04
// Below are interface 4 and later
#define AGSE_PRESCREENDRAW   0x08
// Below are interface 5 and later
#define AGSE_SAVEGAME        0x10
#define AGSE_RESTOREGAME     0x20
// Below are interface 6 and later
#define AGSE_PREGUIDRAW      0x40
#define AGSE_LEAVEROOM       0x80
#define AGSE_ENTERROOM       0x100
#define AGSE_TRANSITIONIN    0x200
#define AGSE_TRANSITIONOUT   0x400
// Below are interface 12 and later
#define AGSE_FINALSCREENDRAW 0x800
#define AGSE_TRANSLATETEXT   0x1000
// Below are interface 13 and later
#define AGSE_SCRIPTDEBUG     0x2000
// AGSE_AUDIODECODE is no longer supported
#define AGSE_AUDIODECODE     0x4000
// Below are interface 18 and later
#define AGSE_SPRITELOAD      0x8000
// Below are interface 21 and later
#define AGSE_PRERENDER       0x10000
// Below are interface 24 and later
#define AGSE_PRESAVEGAME     0x20000
#define AGSE_POSTRESTOREGAME 0x40000
// Below are interface 26 and later
#define AGSE_POSTROOMDRAW    0x80000
#define AGSE_TOOHIGH         0x100000

#endif // _AGS_PLUGIN_EVTS_H
