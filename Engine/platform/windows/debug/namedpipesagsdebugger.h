//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_NAMEDPIPESAGSDEBUGGER_H
#define __AC_NAMEDPIPESAGSDEBUGGER_H

#include <windows.h>
#include "debug/agseditordebugger.h"

struct NamedPipesAGSDebugger : IAGSEditorDebugger
{
private:
    HANDLE _hPipeSending;
    HANDLE _hPipeReading;
    const char *_instanceToken;

    void SendAcknowledgement();
public:

    NamedPipesAGSDebugger(const char *instanceToken);
    bool Initialize() override;
    void Shutdown() override;
    bool SendMessageToEditor(const char *message) override;
    bool IsMessageAvailable() override;
    char* GetNextMessage() override;
};

#endif // __AC_NAMEDPIPESAGSDEBUGGER_H
