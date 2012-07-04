
#include "wgt2allg.h"                           // exists()
#include "debug/filebasedagsdebugger.h"
#include "ac/file.h"                     // filelength()

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

    FILE *outt = fopen(SENT_MESSAGE_FILE_NAME, "wb");
    fprintf(outt, message);
    fclose(outt);
    return true;
}

bool FileBasedAGSDebugger::IsMessageAvailable()
{
    return (exists("dbgsend.tmp") != 0);
}

char* FileBasedAGSDebugger::GetNextMessage()
{
    FILE *inn = fopen("dbgsend.tmp", "rb");
    if (inn == NULL)
    {
        // check again, because the editor might have deleted the file in the meantime
        return NULL;
    }
    int fileSize = filelength(fileno(inn));
    char *msg = (char*)malloc(fileSize + 1);
    fread(msg, fileSize, 1, inn);
    fclose(inn);
    unlink("dbgsend.tmp");
    msg[fileSize] = 0;
    return msg;
}
