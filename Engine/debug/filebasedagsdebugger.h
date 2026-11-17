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
// FileBasedAGSDebugger exchanges messages through files named after the
// instance token.
// 
//=============================================================================
#ifndef __AC_FILEBASEDAGSDEBUGGER_H
#define __AC_FILEBASEDAGSDEBUGGER_H

#include "debug/agseditordebugger.h"
#include "util/string.h"

class FileBasedAGSDebugger : public IAGSEditorDebugger
{
public:
    // config is a comma-separated list of key:value pairs:
    //   dir - folder for the files (current directory by default)
    FileBasedAGSDebugger(const AGS::Common::String &config);

    bool Initialize() override;
    void Shutdown() override;
    bool SendMessageToEditor(const char *message) override;
    bool IsMessageAvailable() override;
    AGS::Common::String GetNextMessage() override;

private:
    // engine -> editor; written to a temporary file first and then renamed
    static const char *SentMessageFileName;
    static const char *SentMessagePartFileName;
    // editor -> engine
    static const char *ReceivedMessageFileName;

    AGS::Common::String _sentMessageFile;
    AGS::Common::String _sentMessagePartFile;
    AGS::Common::String _receivedMessageFile;
};

#endif // __AC_FILEBASEDAGSDEBUGGER_H
