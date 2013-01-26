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
// Implementation from acfonts.cpp specific to Engine runtime
//
//=============================================================================

// Headers, as they are in acfonts.cpp
#pragma unmanaged
#ifndef USE_ALFONT
#define USE_ALFONT
#endif
#include "util/wgt2allg.h"
#include "ac/gamesetupstruct.h"

#ifdef USE_ALFONT
#include "alfont.h"
#endif

#include "core/assetmanager.h"
#include "util/stream.h"

using AGS::Common::Stream;

// For engine these are defined in ac.cpp
extern int our_eip;
extern GameSetupStruct game;

//=============================================================================
// Engine-specific implementation split out of acfonts.cpp
//=============================================================================

void set_our_eip(int eip)
{
  our_eip = eip;
}

int get_our_eip()
{
  return our_eip;
}

Stream *fopen_shared(char *filnamm,
                          Common::FileOpenMode open_mode,
                          Common::FileWorkMode work_mode)
{
  return Common::AssetManager::OpenAsset(filnamm, open_mode, work_mode);
}

int flength_shared(Stream *ffi)
{
  // Common::AssetManager::OpenAsset will have set Common::AssetManager::GetLastAssetSize()
  return Common::AssetManager::GetLastAssetSize();
}

void set_font_outline(int font_number, int outline_type)
{
    game.fontoutline[font_number] = FONT_OUTLINE_AUTO;
}
