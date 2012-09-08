
#include "util/wgt2allg.h"                           // exists()
#include "debug/filebasedagsdebugger.h"
#include "ac/file.h"                     // filelength()
#include "util/filestream.h"
#include "util/textstreamwriter.h"

using AGS::Common::CDataStream;
using AGS::Common::CTextStreamWriter;

const char* SENT_MESSAGE_FILE_NAME = "dbgrecv.tmp";

bool FileBasedAGSDebugger::Initialize()
{
    if (exists(SENT_MESSAGE_FILE_NAME))
    {
        unlink(SENT_MESSAGE_FILE_NAME);
    }
    return true;
}

void FileBasedAGSDebugger::Shutdown()
{
}

bool FileBasedAGSDebugger::SendMessageToEditor(const char *message) 
{
    while (exists(SENT_MESSAGE_FILE_NAME))
    {
        Sleep(1);
    }

    CDataStream *out = Common::File::CreateFile(SENT_MESSAGE_FILE_NAME);
    // CHECKME: originally the file was opened as "wb" for some reason,
    // which means the message should be written as a binary array;
    // or shouldn't it?
    out->Write(message, strlen(message));
    delete out;
    return true;
}

bool FileBasedAGSDebugger::IsMessageAvailable()
{
    return (exists("dbgsend.tmp") != 0);
}

char* FileBasedAGSDebugger::GetNextMessage()
{
    CDataStream *in = Common::File::OpenFileRead("dbgsend.tmp");
    if (in == NULL)
    {
        // check again, because the editor might have deleted the file in the meantime
        return NULL;
    }
    int fileSize = in->GetLength();
    char *msg = (char*)malloc(fileSize + 1);
    in->ReadArray(msg, fileSize, 1);
    delete in;
    unlink("dbgsend.tmp");
    msg[fileSize] = 0;
    return msg;
}
