//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AC_DUMMYAGSDEBUGGER_H
#define __AC_DUMMYAGSDEBUGGER_H

#include "debug/debugger.h"

struct DummyAGSDebugger : IAGSEditorDebugger
{
public:

    bool Initialize() override { return false; }
    void Shutdown() override { }
    bool SendMessageToEditor(const char *message) override { return false; }
    bool IsMessageAvailable() override { return false; }
    char* GetNextMessage() override { return NULL; }
};

#endif // __AC_DUMMYAGSDEBUGGER_H
