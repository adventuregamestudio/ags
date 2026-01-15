//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SYSTEMAUDIO_H
#define __AGS_EE_AC__SYSTEMAUDIO_H

#include "ac/dynobj/scriptobjects.h"

int     System_GetColorDepth();
int     System_GetOS();
const char *System_GetVersion();
int     System_GetHardwareAcceleration();
int     System_GetNumLock();
int     System_GetCapsLock();
int     System_GetScrollLock();
int     System_GetVsync();
void    System_SetVsync(int newValue);
void    System_SetVSyncInternal(bool vsync);
int     System_GetWindowed();
int     System_GetSupportsGammaControl();
int     System_GetGamma();
void    System_SetGamma(int newValue);
int     System_GetAudioChannelCount();
ScriptAudioChannel* System_GetAudioChannels(int index);
int     System_GetVolume();
void    System_SetVolume(int newvol);
const char *System_GetRuntimeInfo();

// Engine value constants match the declarations in script API.
// Constant name pattern:
// ENGINE_VALUE_<I,II,S,SI>_NAME, where
//   I - integer, II - indexed integer, S - string, SI - indexed string.
// When adding indexed values - make sure to also add a value that tells their count.
enum EngineValueID
{
    ENGINE_VALUE_UNDEFINED = 0,            // formality...
    ENGINE_VALUE_SI_VALUENAME,
    ENGINE_VALUE_S_ENGINE_NAME,
    ENGINE_VALUE_S_ENGINE_VERSION,         // N.N.N.N (with an optional custom tag)
    ENGINE_VALUE_S_ENGINE_VERSION_FULL,    // full, with bitness, endianess and any tag list
    ENGINE_VALUE_S_DISPLAY_MODE_STR,
    ENGINE_VALUE_S_GFXRENDERER,
    ENGINE_VALUE_S_GFXFILTER,
    ENGINE_VALUE_I_SPRCACHE_MAXNORMAL,
    ENGINE_VALUE_I_SPRCACHE_NORMAL,
    ENGINE_VALUE_I_SPRCACHE_LOCKED,
    ENGINE_VALUE_I_SPRCACHE_EXTERNAL,
    ENGINE_VALUE_I_TEXCACHE_MAXNORMAL,
    ENGINE_VALUE_I_TEXCACHE_NORMAL,
    ENGINE_VALUE_I_FPS_MAX,
    ENGINE_VALUE_I_FPS,
    ENGINE_VALUE_LAST                      // in case user wants to iterate them
};

// Returns a runtime engine integer parameter, identified by a constant, and an optional index
bool GetEngineInteger(int &value, EngineValueID value_id, int index = 0);
// Returns a runtime engine string parameter, identified by a constant, and an optional index
bool GetEngineString(AGS::Common::String &value, EngineValueID value_id, int index = 0);
// Returns a engine value's description
AGS::Common::String GetEngineValueName(EngineValueID value_id);

#endif // __AGS_EE_AC_SYSTEMAUDIO_H
