//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "core/platform.h"

// TODO: port exception handling to mingw
#if AGS_PLATFORM_OS_WINDOWS && !AGS_PLATFORM_WINDOWS_MINGW
#include <new.h>
#include <cinttypes>
#include <stdio.h>
#include "ac/common.h" // quit
#include "ac/common_defines.h"
#include "debug/out.h"
#include "util/ini_util.h"
#include "main/main.h"
#include "platform/base/sys_main.h"
#include "platform/windows/windows.h"
#include "script/cc_common.h"

#if !AGS_PLATFORM_DEBUG
#define USE_CUSTOM_EXCEPTION_HANDLER
#endif

using namespace AGS::Common;

extern int our_eip;
extern int eip_guinum;
extern int eip_guiobj;
extern int proper_exit;

char tempmsg[100];
#define PRINT_WORKSPACE_SIZE (7000u)
char *printfworkingspace;

#ifdef USE_CUSTOM_EXCEPTION_HANDLER
void CreateMiniDump(EXCEPTION_POINTERS *pep);

extern int CustomExceptionHandler(LPEXCEPTION_POINTERS exinfo);
extern EXCEPTION_RECORD excinfo;
extern int miniDumpResultCode;

static void DisplayException()
{
    const auto &sc_error = cc_get_error();
    snprintf(printfworkingspace, PRINT_WORKSPACE_SIZE, "An exception 0x%X occurred in ACWIN.EXE at EIP = 0x%0*" PRIXPTR "; program pointer is %+d, engine version %s, gtags (%d,%d)\n\n"
        "AGS cannot continue, this exception was fatal. Please note down the numbers above, remember what you were doing at the time and contact the game author for support "
        "or post these details on the AGS Technical Forum.\n\n%s\n\n"
        "Most versions of Windows allow you to press Ctrl+C now to copy this entire message to the clipboard for easy reporting.\n\n%s (code %d)",
        excinfo.ExceptionCode, (int)sizeof(intptr_t) * 2, (intptr_t)excinfo.ExceptionAddress, our_eip, EngineVersion.LongString.GetCStr(), eip_guinum, eip_guiobj, sc_error.CallStack.GetCStr(),
        (miniDumpResultCode == 0) ? "An error file CrashInfo.dmp has been created. You may be asked to upload this file when reporting this problem on the AGS Forums." :
        "Unable to create an error dump file.", miniDumpResultCode);
    MessageBoxA((HWND)sys_win_get_window(), printfworkingspace, "Illegal exception", MB_ICONSTOP | MB_OK);
}

int initialize_engine_with_exception_handling(
    int (initialize_engine)(const AGS::Common::ConfigTree &startup_opts),
    const ConfigTree &startup_opts)
{
    __try
    {
        Debug::Printf(kDbgMsg_Info, "Installing exception handler");
        return initialize_engine(startup_opts);
    }
    __except (CustomExceptionHandler(GetExceptionInformation()))
    {
        DisplayException();
        proper_exit = 1;
    }
    return EXIT_CRASH;
}
#endif // USE_CUSTOM_EXCEPTION_HANDLER


int malloc_fail_handler(size_t amountwanted)
{
#ifdef USE_CUSTOM_EXCEPTION_HANDLER
    CreateMiniDump(NULL);
#endif
    free(printfworkingspace);
    snprintf(tempmsg, sizeof(tempmsg), "Out of memory: failed to allocate %zu bytes (at PP=%d)", amountwanted, our_eip);
    quit(tempmsg);
    return 0;
}

void setup_malloc_handling()
{
    _set_new_handler(malloc_fail_handler);
    _set_new_mode(1);
    printfworkingspace = (char*)malloc(PRINT_WORKSPACE_SIZE);
}

#endif // AGS_PLATFORM_OS_WINDOWS
