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

#ifndef __AC_FILEBASEDAGSDEBUGGER_H
#define __AC_FILEBASEDAGSDEBUGGER_H

#include "debug/agseditordebugger.h"
#include "util/string.h"

struct FileBasedAGSDebugger : IAGSEditorDebugger
{
public:
    FileBasedAGSDebugger() = default;
    // Exchange files in the given folder (current directory if empty or not a folder)
    explicit FileBasedAGSDebugger(const char *directory);

    bool Initialize() override;
    void Shutdown() override;
    bool SendMessageToEditor(const char *message) override;
    bool IsMessageAvailable() override;
    char* GetNextMessage() override;

private:
    AGS::Common::String FilePath(const char *name) const;
    AGS::Common::String _directory;
};

extern const char* SENT_MESSAGE_FILE_NAME;

#endif // __AC_FILEBASEDAGSDEBUGGER_H
