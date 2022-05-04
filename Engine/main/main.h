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
#ifndef __AGS_EE_MAIN__MAIN_H
#define __AGS_EE_MAIN__MAIN_H

#include "core/platform.h"
#include "util/version.h"

// Current engine version
extern AGS::Common::Version EngineVersion;
// Lowest savedgame version, accepted by this engine
extern AGS::Common::Version SavedgameLowestBackwardCompatVersion;
// Lowest engine version, which would accept current savedgames
extern AGS::Common::Version SavedgameLowestForwardCompatVersion;

//=============================================================================

// Full path to the engine executable
extern AGS::Common::String appPath;
// Engine executable's directory
extern AGS::Common::String appDirectory;
// Game path from the startup options (before reading config)
extern AGS::Common::String cmdGameDataPath;

// Startup flags, set from parameters to engine
extern int override_start_room;
extern bool justTellInfo;
extern AGS::Common::String loadSaveGameOnStartup;

void main_print_help();

int ags_entry_point(int argc, char *argv[]);

#endif // __AGS_EE_MAIN__MAIN_H