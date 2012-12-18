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
// Exporting global script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "util/wgt2allg.h"
#include "ac/gamestate.h"
#include "script/symbol_registry.h"
#include "ac/statobj/agsstaticobject.h"

extern AGSStaticObject GlobalStaticManager;

void register_global_script_functions()
{
	ccAddExternalStaticObject("game",&play, &GlobalStaticManager);
	ccAddExternalStaticObject("gs_globals",&play.globalvars[0], &GlobalStaticManager);
	ccAddExternalStaticObject("mouse",&scmouse, &GlobalStaticManager);
	ccAddExternalStaticObject("palette",&palette[0], &GlobalStaticManager);
	ccAddExternalStaticObject("system",&scsystem, &GlobalStaticManager);
	ccAddExternalStaticObject("savegameindex",&play.filenumbers[0], &GlobalStaticManager);
}
