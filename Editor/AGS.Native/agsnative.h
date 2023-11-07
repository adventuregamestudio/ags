//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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

// TODO: maybe remove this limit completely when engine's sprite cache
// implements optimized container; see comments for SpriteCache class.
#define MAX_STATIC_SPRITES 90000

extern const char *sprsetname;
