/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#include "acdebug/ac_debug.h"

#ifdef WINDOWS_VERSION

#include "acdebug/ac_namedpipesagsdebugger.h"

IAGSEditorDebugger *GetEditorDebugger(const char *instanceToken)
{
    return new NamedPipesAGSDebugger(instanceToken);
}

#else   // WINDOWS_VERSION

IAGSEditorDebugger *GetEditorDebugger(const char *instanceToken)
{
    return NULL;
}

#endif
