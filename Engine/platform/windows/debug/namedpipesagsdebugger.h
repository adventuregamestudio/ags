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

#ifndef __AC_NAMEDPIPESAGSDEBUGGER_H
#define __AC_NAMEDPIPESAGSDEBUGGER_H

#include <windows.h>
#include <io.h>
#include "debug/debugger.h"

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
