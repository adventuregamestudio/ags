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
#include "platform/base/agsplatformdriver.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stream.h"
#include "util/string_utils.h"
#include "util/textstreamwriter.h"

using namespace AGS::Common;

const char *FileBasedAGSDebugger::SentMessageFileName = "dbgrecv_%s.tmp";
const char *FileBasedAGSDebugger::SentMessagePartFileName = "dbgrecv_%s.part";
const char *FileBasedAGSDebugger::ReceivedMessageFileName = "dbgsend_%s.tmp";

FileBasedAGSDebugger::FileBasedAGSDebugger(const String &config)
{
    String instance_token;
    String directory;
    for (const auto &opt : StrUtil::SplitPreservingDoublequoted(config, ','))
    {
        const auto kv = StrUtil::GetKeyValue(opt, ':');
        if (kv.first == "token")
            instance_token = kv.second;
        else if (kv.first == "dir")
            directory = kv.second;
    }
    _sentMessageFile = Path::ConcatPaths(directory, String::FromFormat(SentMessageFileName, instance_token.GetCStr()));
    _sentMessagePartFile = Path::ConcatPaths(directory, String::FromFormat(SentMessagePartFileName, instance_token.GetCStr()));
    _receivedMessageFile = Path::ConcatPaths(directory, String::FromFormat(ReceivedMessageFileName, instance_token.GetCStr()));
}

bool FileBasedAGSDebugger::Initialize()
{
    if (File::IsFile(_sentMessageFile))
    {
        File::DeleteFile(_sentMessageFile);
    }
    return true;
}

void FileBasedAGSDebugger::Shutdown()
{
}

bool FileBasedAGSDebugger::SendMessageToEditor(const char *message) 
{
    while (File::IsFile(_sentMessageFile))
    {
        platform->YieldCPU();
    }

    // Write a temporary file and rename it, so the editor never reads a partial message
    {
        auto out = File::CreateFile(_sentMessagePartFile);
        if (!out)
            return false;
        out->Write(message, strlen(message));
    }
    return File::RenameFile(_sentMessagePartFile, _sentMessageFile);
}

bool FileBasedAGSDebugger::IsMessageAvailable()
{
    return (File::IsFile(_receivedMessageFile) != 0);
}

String FileBasedAGSDebugger::GetNextMessage()
{
    auto in = File::OpenFileRead(_receivedMessageFile);
    if (in == nullptr)
    {
        // check again, because the editor might have deleted the file in the meantime
        return {};
    }
    size_t file_size = (size_t)std::min((soff_t)std::numeric_limits<size_t>::max, in->GetLength());
    String msg = String::FromStreamCount(in.get(), file_size);
    File::DeleteFile(_receivedMessageFile);
    return msg;
}
