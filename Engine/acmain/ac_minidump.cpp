
#include "acmain/ac_maindefines.h"

#ifdef WINDOWS_VERSION
CONTEXT cpustate;
EXCEPTION_RECORD excinfo;
int miniDumpResultCode = 0;

typedef enum _MINIDUMP_TYPE {
    MiniDumpNormal                         = 0x0000,
    MiniDumpWithDataSegs                   = 0x0001,
    MiniDumpWithFullMemory                 = 0x0002,
    MiniDumpWithHandleData                 = 0x0004,
    MiniDumpFilterMemory                   = 0x0008,
    MiniDumpScanMemory                     = 0x0010,
    MiniDumpWithUnloadedModules            = 0x0020,
    MiniDumpWithIndirectlyReferencedMemory = 0x0040,
    MiniDumpFilterModulePaths              = 0x0080,
    MiniDumpWithProcessThreadData          = 0x0100,
    MiniDumpWithPrivateReadWriteMemory     = 0x0200,
    MiniDumpWithoutOptionalData            = 0x0400,
} MINIDUMP_TYPE;

typedef struct _MINIDUMP_EXCEPTION_INFORMATION {
    DWORD ThreadId;
    PEXCEPTION_POINTERS ExceptionPointers;
    BOOL ClientPointers;
} MINIDUMP_EXCEPTION_INFORMATION, *PMINIDUMP_EXCEPTION_INFORMATION;

typedef BOOL (WINAPI * MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD ProcessId, 
  HANDLE hFile, MINIDUMP_TYPE DumpType, 
  CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, 
  CONST void* UserStreamParam, 
  CONST void* CallbackParam); 

MINIDUMPWRITEDUMP _MiniDumpWriteDump;


void CreateMiniDump( EXCEPTION_POINTERS* pep ) 
{
  HMODULE dllHandle = LoadLibrary(L"dbghelp.dll");
  if (dllHandle == NULL)
  {
    miniDumpResultCode = 1;
    return;
  }

  _MiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(dllHandle, "MiniDumpWriteDump");
  if (_MiniDumpWriteDump == NULL)
  {
    FreeLibrary(dllHandle);
    miniDumpResultCode = 2;
    return;
  }

  char fileName[80];
  sprintf(fileName, "CrashInfo.%s.dmp", ACI_VERSION_TEXT);
  HANDLE hFile = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 
    0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 

  if((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
  {
    MINIDUMP_EXCEPTION_INFORMATION mdei; 

    mdei.ThreadId = GetCurrentThreadId();
    mdei.ExceptionPointers = pep;
    mdei.ClientPointers = FALSE;

    MINIDUMP_TYPE mdt = MiniDumpNormal; //MiniDumpWithPrivateReadWriteMemory;

    BOOL rv = _MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
      hFile, mdt, (pep != 0) ? &mdei : 0, NULL, NULL); 

    if (!rv)
      miniDumpResultCode = 4;

    CloseHandle(hFile); 
  }
  else
    miniDumpResultCode = 3;

  FreeLibrary(dllHandle);
}

int CustomExceptionHandler (LPEXCEPTION_POINTERS exinfo) {
  cpustate = exinfo->ContextRecord[0];
  excinfo = exinfo->ExceptionRecord[0];
  CreateMiniDump(exinfo);
  
  return EXCEPTION_EXECUTE_HANDLER;
}

FILE *logfile;
int OurReportingFunction( int reportType, char *userMessage, int *retVal ) {

  fprintf(logfile,"%s: %s\n",(reportType == _CRT_ASSERT) ? "Assertion failed" : "Warning",userMessage);
  fflush (logfile);
  return 0;
}
#endif	// WINDOWS_VERSION
