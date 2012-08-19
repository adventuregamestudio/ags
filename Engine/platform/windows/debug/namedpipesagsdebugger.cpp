
#include <stdio.h>
#include "platform/windows/debug/namedpipesagsdebugger.h"

using AGS::Common::CString;

void NamedPipesAGSDebugger::SendAcknowledgement()
{
    DWORD bytesWritten;
    WriteFile(_hPipeSending, "MSGACK", 6, &bytesWritten, NULL);
}


NamedPipesAGSDebugger::NamedPipesAGSDebugger(const CString &instanceToken)
{
    _instanceToken = instanceToken;
}

bool NamedPipesAGSDebugger::Initialize()
{
    // can't use a single duplex pipe as it was deadlocking
    CString pipeNameBuffer;
    pipeNameBuffer.Format("\\\\.\\pipe\\AGSEditorDebuggerGameToEd%s", _instanceToken);
    _hPipeSending = CreateFile(pipeNameBuffer, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,0, NULL);

    pipeNameBuffer.Format("\\\\.\\pipe\\AGSEditorDebuggerEdToGame%s", _instanceToken);
    _hPipeReading = CreateFile(pipeNameBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING,0, NULL);

    if ((_hPipeReading == INVALID_HANDLE_VALUE) ||
        (_hPipeSending == INVALID_HANDLE_VALUE))
        return false;

    return true;
}

void NamedPipesAGSDebugger::Shutdown()
{
    if (_hPipeReading != NULL)
    {
        CloseHandle(_hPipeReading);
        CloseHandle(_hPipeSending);
        _hPipeReading = NULL;
    }
}

bool NamedPipesAGSDebugger::SendMessageToEditor(const CString &message) 
{
    DWORD bytesWritten;
    return (WriteFile(_hPipeSending, message.GetCStr(), message.GetLength(), &bytesWritten, NULL ) != 0);
}

bool NamedPipesAGSDebugger::IsMessageAvailable()
{
    DWORD bytesAvailable = 0;
    PeekNamedPipe(_hPipeReading, NULL, 0, NULL, &bytesAvailable, NULL);

    return (bytesAvailable > 0);
}

CString NamedPipesAGSDebugger::GetNextMessage()
{
    DWORD bytesAvailable = 0;
    PeekNamedPipe(_hPipeReading, NULL, 0, NULL, &bytesAvailable, NULL);

    if (bytesAvailable > 0)
    {
        char* buffer = (char*)malloc(bytesAvailable + 1);
        DWORD bytesRead = 0;
        ReadFile(_hPipeReading, buffer, bytesAvailable, &bytesRead, NULL);
        buffer[bytesRead] = 0;

        SendAcknowledgement();
        return buffer;
    }
    else
    {
        return NULL;
    }

}
