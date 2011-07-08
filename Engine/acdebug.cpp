/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#include <windows.h>
#include <acdebug.h>
#include <stdio.h>
#include <io.h>

extern int exists(const char *filename);

struct DummyAGSDebugger : IAGSEditorDebugger
{
public:

  virtual bool Initialize() { return false; }
  virtual void Shutdown() { }
  virtual bool SendMessageToEditor(const char *message) { return false; }
  virtual bool IsMessageAvailable() { return false; }
  virtual char* GetNextMessage() { return NULL; }
};


#ifdef WINDOWS_VERSION

struct NamedPipesAGSDebugger : IAGSEditorDebugger
{
private:
  HANDLE _hPipeSending;
  HANDLE _hPipeReading;
  const char *_instanceToken;

  void SendAcknowledgement()
  {
    DWORD bytesWritten;
    WriteFile(_hPipeSending, "MSGACK", 6, &bytesWritten, NULL);
  }

public:

  NamedPipesAGSDebugger(const char *instanceToken)
  {
    _instanceToken = instanceToken;
  }

  virtual bool Initialize()
  {
    // can't use a single duplex pipe as it was deadlocking
    char pipeNameBuffer[MAX_PATH];
    sprintf(pipeNameBuffer, "\\\\.\\pipe\\AGSEditorDebuggerGameToEd%s", _instanceToken);
    _hPipeSending = CreateFile(pipeNameBuffer, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,0, NULL);

    sprintf(pipeNameBuffer, "\\\\.\\pipe\\AGSEditorDebuggerEdToGame%s", _instanceToken);
    _hPipeReading = CreateFile(pipeNameBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING,0, NULL);

    if ((_hPipeReading == INVALID_HANDLE_VALUE) ||
        (_hPipeSending == INVALID_HANDLE_VALUE))
      return false;

    return true;
  }

  virtual void Shutdown()
  {
    if (_hPipeReading != NULL)
    {
      CloseHandle(_hPipeReading);
      CloseHandle(_hPipeSending);
      _hPipeReading = NULL;
    }
  }

  virtual bool SendMessageToEditor(const char *message) 
  {
    DWORD bytesWritten;
    return (WriteFile(_hPipeSending, message, strlen(message), &bytesWritten, NULL ) != 0);
  }

  virtual bool IsMessageAvailable()
  {
    DWORD bytesAvailable = 0;
    PeekNamedPipe(_hPipeReading, NULL, 0, NULL, &bytesAvailable, NULL);
    
    return (bytesAvailable > 0);
  }

  virtual char* GetNextMessage()
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

};



static const char* SENT_MESSAGE_FILE_NAME = "dbgrecv.tmp";

struct FileBasedAGSDebugger : IAGSEditorDebugger
{
private:

public:

  virtual bool Initialize()
  {
    if (exists(SENT_MESSAGE_FILE_NAME))
    {
      unlink(SENT_MESSAGE_FILE_NAME);
    }
    return true;
  }

  virtual void Shutdown()
  {
  }

  virtual bool SendMessageToEditor(const char *message) 
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

  virtual bool IsMessageAvailable()
  {
    return (exists("dbgsend.tmp") != 0);
  }

  virtual char* GetNextMessage()
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

};



IAGSEditorDebugger *GetEditorDebugger(const char *instanceToken)
{
  return new NamedPipesAGSDebugger(instanceToken);
}


#else   // WINDOWS_VERSION


IAGSEditorDebugger *GetEditorDebugger(const char *instanceToken)
{
  return NULL;
}

#endif
