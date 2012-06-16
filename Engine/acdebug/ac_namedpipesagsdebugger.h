/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_NAMEDPIPESAGSDEBUGGER_H
#define __AC_NAMEDPIPESAGSDEBUGGER_H

#include <windows.h>
#include <io.h>
#include "acdebug/ac_debug.h"

struct NamedPipesAGSDebugger : IAGSEditorDebugger
{
private:
    HANDLE _hPipeSending;
    HANDLE _hPipeReading;
    const char *_instanceToken;

    void SendAcknowledgement();
public:

    NamedPipesAGSDebugger(const char *instanceToken);
    virtual bool Initialize();
    virtual void Shutdown();
    virtual bool SendMessageToEditor(const char *message);
    virtual bool IsMessageAvailable();
    virtual char* GetNextMessage();
};

#endif // __AC_NAMEDPIPESAGSDEBUGGER_H