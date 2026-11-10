//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
// 
// NamedPipesAGSDebugger exchanges messages through named pipes, using
// instance token to identify a pipe.
// 
// Currently this is a Windows-only implementation.
// 
//=============================================================================
#ifndef __AC_NAMEDPIPESAGSDEBUGGER_H
#define __AC_NAMEDPIPESAGSDEBUGGER_H

#include "platform/platform.h"

#if AGS_PLATFORM_OS_WINDOWS

#include <vector>
#include "platform/windows/windows.h"
#include "debug/agseditordebugger.h"
#include "util/string.h"

class NamedPipesAGSDebugger : public IAGSEditorDebugger
{
public:
    NamedPipesAGSDebugger(const AGS::Common::String &instanceToken);

    bool Initialize() override;
    void Shutdown() override;
    bool SendMessageToEditor(const char *message) override;
    bool IsMessageAvailable() override;
    AGS::Common::String GetNextMessage() override;

private:
    AGS::Common::String _instanceToken;
    HANDLE _hPipeSending = NULL;
    HANDLE _hPipeReading = NULL;
    std::vector<char> _buffer;

    void SendAcknowledgement();
};

#endif // AGS_PLATFORM_OS_WINDOWS
#endif // __AC_NAMEDPIPESAGSDEBUGGER_H
