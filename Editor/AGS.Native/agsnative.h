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
// Definitions required for shared code between Editor and Engine.
//
//=============================================================================
#pragma once
#include "Common/ac/common.h"
#include "Common/ac/characterinfo.h"
#include "Common/ac/dialogtopic.h"
#include "Common/ac/gamestructdefines.h"
#include "Common/ac/inventoryiteminfo.h"
#include "Common/ac/mousecursor.h"
#include "Common/ac/view.h"
#include "Common/ac/wordsdictionary.h"
#include "Common/gui/guidefines.h"
#include "Common/game/customproperties.h"

#define MAXMULTIFILES 25
#define CHUNKSIZE 256000
#define MAX_FILENAME_LENGTH 100
#define SAVEBUFFERSIZE 5120
#define MAX_PLUGINS 40

extern const char *sprsetname;
