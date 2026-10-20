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

FileBasedAGSDebugger::FileBasedAGSDebugger(const char *directory)
{
    if (directory && File::IsDirectory(directory))
        _directory = directory;
}

String FileBasedAGSDebugger::FilePath(const char *name) const
{
    return _directory.IsEmpty() ? String(name) : Path::ConcatPaths(_directory, name);
}

bool FileBasedAGSDebugger::Initialize()
{
    if (File::IsFile(FilePath(SENT_MESSAGE_FILE_NAME)))
    {
        File::DeleteFile(FilePath(SENT_MESSAGE_FILE_NAME));
    }
    return true;
}

void FileBasedAGSDebugger::Shutdown()
{
}

bool FileBasedAGSDebugger::SendMessageToEditor(const char *message) 
{
    while (File::IsFile(FilePath(SENT_MESSAGE_FILE_NAME)))
    {
        platform->YieldCPU();
    }

    // Write a temporary file and rename it, so the editor never reads a partial message
    const String tempName = FilePath("dbgrecv.part");
    {
        auto out = File::CreateFile(tempName);
        if (!out)
            return false;
        out->Write(message, strlen(message));
    }
    return File::RenameFile(tempName, FilePath(SENT_MESSAGE_FILE_NAME));
}

bool FileBasedAGSDebugger::IsMessageAvailable()
{
    return (File::IsFile(FilePath("dbgsend.tmp")) != 0);
}

char* FileBasedAGSDebugger::GetNextMessage()
{
    auto in = File::OpenFileRead(FilePath("dbgsend.tmp"));
    if (in == nullptr)
    {
        // check again, because the editor might have deleted the file in the meantime
        return nullptr;
    }
    size_t fileSize = (size_t)std::min((soff_t)std::numeric_limits<size_t>::max, in->GetLength());
    char *msg = (char*)malloc(fileSize + 1);
    in->Read(msg, fileSize);
    File::DeleteFile(FilePath("dbgsend.tmp"));
    msg[fileSize] = 0;
    return msg;
}
