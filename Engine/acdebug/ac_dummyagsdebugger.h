/*
Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
All rights reserved.

The AGS Editor Source Code is provided under the Artistic License 2.0
http://www.opensource.org/licenses/artistic-license-2.0.php

You MAY NOT compile your own builds of the engine without making it EXPLICITLY
CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_DUMMYAGSDEBUGGER_H
#define __AC_DUMMYAGSDEBUGGER_H

#include "acdebug/ac_debug.h"

struct DummyAGSDebugger : IAGSEditorDebugger
{
public:

    virtual bool Initialize() { return false; }
    virtual void Shutdown() { }
    virtual bool SendMessageToEditor(const char *message) { return false; }
    virtual bool IsMessageAvailable() { return false; }
    virtual char* GetNextMessage() { return NULL; }
};

#endif // __AC_DUMMYAGSDEBUGGER_H