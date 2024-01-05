//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <algorithm>
#include <cstdio>
#include <limits>
#include <string.h>
#include "debug/filebasedagsdebugger.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"
#include "util/textstreamwriter.h"
#include "platform/base/agsplatformdriver.h"

using namespace AGS::Common;

const char* SENT_MESSAGE_FILE_NAME = "dbgrecv.tmp";

bool FileBasedAGSDebugger::Initialize()
{
    if (File::IsFile(SENT_MESSAGE_FILE_NAME))
    {
        File::DeleteFile(SENT_MESSAGE_FILE_NAME);
    }
    return true;
}

void FileBasedAGSDebugger::Shutdown()
{
}

bool FileBasedAGSDebugger::SendMessageToEditor(const char *message) 
{
    while (File::IsFile(SENT_MESSAGE_FILE_NAME))
    {
        platform->YieldCPU();
    }

    Stream *out = Common::File::CreateFile(SENT_MESSAGE_FILE_NAME);
    // CHECKME: originally the file was opened as "wb" for some reason,
    // which means the message should be written as a binary array;
    // or shouldn't it?
    out->Write(message, strlen(message));
    delete out;
    return true;
}

bool FileBasedAGSDebugger::IsMessageAvailable()
{
    return (File::IsFile("dbgsend.tmp") != 0);
}

char* FileBasedAGSDebugger::GetNextMessage()
{
    Stream *in = Common::File::OpenFileRead("dbgsend.tmp");
    if (in == nullptr)
    {
        // check again, because the editor might have deleted the file in the meantime
        return nullptr;
    }
    size_t fileSize = (size_t)std::min((soff_t)std::numeric_limits<size_t>::max, in->GetLength());
    char *msg = (char*)malloc(fileSize + 1);
    in->Read(msg, fileSize);
    delete in;
    File::DeleteFile("dbgsend.tmp");
    msg[fileSize] = 0;
    return msg;
}
