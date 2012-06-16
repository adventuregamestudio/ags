/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_FILEBASEDAGSDEBUGGER_H
#define __AC_FILEBASEDAGSDEBUGGER_H

#include "acdebug/ac_debug.h"

struct FileBasedAGSDebugger : IAGSEditorDebugger
{
public:

    bool Initialize();
    void Shutdown();
    bool SendMessageToEditor(const char *message);
    bool IsMessageAvailable();
    char* GetNextMessage();

};

extern const char* SENT_MESSAGE_FILE_NAME;

#endif // __AC_FILEBASEDAGSDEBUGGER_H