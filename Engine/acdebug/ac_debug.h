/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/

#ifndef __AC_DEBUG_H
#define __AC_DEBUG_H

struct IAGSEditorDebugger
{
public:
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool SendMessageToEditor(const char *message) = 0;
    virtual bool IsMessageAvailable() = 0;
    // Message will be allocated on heap with malloc
    virtual char* GetNextMessage() = 0;
};

IAGSEditorDebugger *GetEditorDebugger(const char *instanceToken);

#endif // __AC_DEBUG_H