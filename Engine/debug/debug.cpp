/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
/*
#ifdef WINDOWS_VERSION
#include <windows.h>    // for HWND
#else
// ???
#endif
*/
#include <stdio.h>
#include "util/wgt2allg.h"
#include "ac/common.h"
#include "ac/roomstruct.h"
#include "ac/runtime_defines.h"
#include "debug/debug.h"
#include "gui/dynamicarray.h"
#include "media/audio/audio.h"
#include "script/script.h"
#include "script/script_common.h"
#include "script/cc_error.h"

using AGS::Common::CString;

extern char check_dynamic_sprites_at_exit;
extern int displayed_room;
extern roomstruct thisroom;
extern volatile char want_exit, abort_engine;
extern ccScript* gamescript;
extern ccScript* dialogScriptsScript;


int use_compiled_folder_as_current_dir = 0;
int editor_debugging_enabled = 0;
int editor_debugging_initialized = 0;
CString editor_debugger_instance_token;
IAGSEditorDebugger *editor_debugger = NULL;
int break_on_next_script_step = 0;
volatile int game_paused_in_debugger = 0;
HWND editor_window_handle = NULL;

#ifdef WINDOWS_VERSION

#include "platform/windows/debug/namedpipesagsdebugger.h"

IAGSEditorDebugger *GetEditorDebugger(const CString &instanceToken)
{
    return new NamedPipesAGSDebugger(instanceToken);
}

#else   // WINDOWS_VERSION

IAGSEditorDebugger *GetEditorDebugger(const CString &instanceToken)
{
    return NULL;
}

#endif

int debug_flags=0;

DebugConsoleText debug_line[DEBUG_CONSOLE_NUMLINES];
int first_debug_line = 0, last_debug_line = 0, display_console = 0;

int fps=0,display_fps=0;

void quitprintf(char*texx, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    vsprintf(displbuf,texx,ap);
    va_end(ap);
    quit(displbuf);
}

void write_log(char*msg) {
    /*
    FILE*ooo=fopen("ac.log","at");
    fprintf(ooo,"%s\n",msg);
    fclose(ooo);
    */
    platform->WriteDebugString(msg);
}



/* The idea of this is that non-essential errors such as "sound file not
found" are logged instead of exiting the program.
*/
void debug_log(char*texx, ...) {
    // if not in debug mode, don't print it so we don't worry the
    // end player
    if (play.debug_mode == 0)
        return;
    static int first_time = 1;
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,texx);
    vsprintf(displbuf,texx,ap);
    va_end(ap);

    /*if (true) {
    char buffer2[STD_BUFFER_SIZE];
    strcpy(buffer2, "%");
    strcat(buffer2, displbuf);
    quit(buffer2);
    }*/

    char*openmode = "at";
    if (first_time) {
        openmode = "wt";
        first_time = 0;
    }
    FILE*outfil = fopen("warnings.log",openmode);
    if (outfil == NULL)
    {
        debug_write_console("* UNABLE TO WRITE TO WARNINGS.LOG");
        debug_write_console(displbuf);
    }
    else
    {
        fprintf(outfil,"(in room %d): %s\n",displayed_room,displbuf);
        fclose(outfil);
    }
}


void debug_write_console (char *msg, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,msg);
    vsprintf(displbuf,msg,ap);
    va_end(ap);
    displbuf[99] = 0;

    strcpy (debug_line[last_debug_line].text, displbuf);
    ccInstance*curinst = ccGetCurrentInstance();
    if (curinst != NULL) {
        char scriptname[20];
        if (curinst->instanceof == gamescript)
            strcpy(scriptname,"G ");
        else if (curinst->instanceof == thisroom.compiled_script)
            strcpy (scriptname, "R ");
        else if (curinst->instanceof == dialogScriptsScript)
            strcpy(scriptname,"D ");
        else
            strcpy(scriptname,"? ");
        sprintf(debug_line[last_debug_line].script,"%s%d",scriptname,currentline);
    }
    else debug_line[last_debug_line].script[0] = 0;

    platform->WriteDebugString("%s (%s)", displbuf, debug_line[last_debug_line].script);

    last_debug_line = (last_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

    if (last_debug_line == first_debug_line)
        first_debug_line = (first_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

}


CString get_cur_script(int numberOfLinesOfCallStack) {
    CString buffer;
    char * buf = buffer.GetBuffer(3000);
    // FIXME
    ccGetCallStack(ccGetCurrentInstance(), buf, numberOfLinesOfCallStack);
    buffer.ReleaseBuffer();

    if (buffer.IsEmpty())
    {
        buffer = ccErrorCallStack;
    }

    return buffer;
}

const CString BREAK_MESSAGE = "BREAK";

struct Breakpoint
{
    CString scriptName;
    int lineNumber;
};

DynamicArray<Breakpoint> breakpoints;
int numBreakpoints = 0;

bool send_message_to_editor(const CString &msg, const CString &errorMsg) 
{
    CString callStack = get_cur_script(25);
    if (callStack.IsEmpty())
        return false;

    CString messageToSend;
    messageToSend.Format("<?xml version=\"1.0\" encoding=\"Windows-1252\"?><Debugger Command=\"%s\">", msg);
#ifdef WINDOWS_VERSION
    messageToSend.Append(CString::MakeString("  <EngineWindow>%d</EngineWindow> ", win_get_window()));
#endif
    messageToSend.Append(CString::MakeString("  <ScriptState><![CDATA[%s]]></ScriptState> ", callStack));
    if (!errorMsg.IsEmpty())
    {
        messageToSend.Append(CString::MakeString("  <ErrorMessage><![CDATA[%s]]></ErrorMessage> ", errorMsg));
    }
    messageToSend.Append("</Debugger>");

    editor_debugger->SendMessageToEditor(messageToSend);

    return true;
}

bool send_message_to_editor(const CString &msg) 
{
    return send_message_to_editor(msg, "");
}

bool init_editor_debugging() 
{
#ifdef WINDOWS_VERSION
    editor_debugger = GetEditorDebugger(editor_debugger_instance_token);
#else
    // Editor isn't ported yet
    editor_debugger = NULL;
#endif

    if (editor_debugger == NULL)
        quit("editor_debugger is NULL but debugger enabled");

    if (editor_debugger->Initialize())
    {
        editor_debugging_initialized = 1;

        // Wait for the editor to send the initial breakpoints
        // and then its READY message
        while (check_for_messages_from_editor() != 2) 
        {
            platform->Delay(10);
        }

        send_message_to_editor("START");
        return true;
    }

    return false;
}

int check_for_messages_from_editor()
{
    if (editor_debugger->IsMessageAvailable())
    {
        CString msg = editor_debugger->GetNextMessage();
        if (msg.IsEmpty())
        {
            return 0;
        }

        if (msg.CompareLeft("<Engine Command=\"") != 0) 
        {
            //OutputDebugString("Faulty message received from editor:");
            //OutputDebugString(msg);
            return 0;
        }

        msg = msg.Mid(17);

        if (msg.CompareLeft("START") == 0)
        {
            // FIXME
            const char *windowHandle = strstr(msg.GetCStr(), "EditorWindow") + 14;
            editor_window_handle = (HWND)atoi(windowHandle);
        }
        else if (msg.CompareLeft("READY") == 0)
        {
            return 2;
        }
        else if (msg.CompareLeft("SETBREAK") == 0 ||
            msg.CompareLeft("DELBREAK") == 0)
        {
            bool isDelete = (msg[0] == 'D');
            // Format:  SETBREAK $scriptname$lineNumber$
            // FIXME: use some kind of fixed buffer iterator here
            const char *msgPtr = msg.Mid(10);
            char scriptNameBuf[100];
            int i = 0;
            while (msgPtr[0] != '$')
            {
                scriptNameBuf[i] = msgPtr[0];
                msgPtr = msgPtr++;
                i++;
            }
            scriptNameBuf[i] = 0;
            msgPtr = msgPtr++;

            int lineNumber = atoi(msgPtr);

            if (isDelete) 
            {
                for (i = 0; i < numBreakpoints; i++)
                {
                    if ((breakpoints[i].lineNumber == lineNumber) &&
                        (breakpoints[i].scriptName.Compare(scriptNameBuf) == 0))
                    {
                        numBreakpoints--;
                        for (int j = i; j < numBreakpoints; j++)
                        {
                            breakpoints[j] = breakpoints[j + 1];
                        }
                        break;
                    }
                }
            }
            else 
            {
                breakpoints[numBreakpoints].scriptName = scriptNameBuf;
                breakpoints[numBreakpoints].lineNumber = lineNumber;
                numBreakpoints++;
            }
        }
        else if (msg.CompareLeft("RESUME") == 0)
        {
            game_paused_in_debugger = 0;
        }
        else if (msg.CompareLeft("STEP") == 0) 
        {
            game_paused_in_debugger = 0;
            break_on_next_script_step = 1;
        }
        else if (msg.CompareLeft("EXIT") == 0) 
        {
            want_exit = 1;
            abort_engine = 1;
            check_dynamic_sprites_at_exit = 0;
        }

        return 1;
    }

    return 0;
}




bool send_exception_to_editor(const CString &qmsg)
{
#ifdef WINDOWS_VERSION
    want_exit = 0;
    // allow the editor to break with the error message
    CString errorMsgToSend;
    if (qmsg[0] == '?')
    {
        errorMsgToSend = qmsg.Mid(1);
    }
    else
    {
        errorMsgToSend = qmsg;
    }

    if (editor_window_handle != NULL)
        SetForegroundWindow(editor_window_handle);

    if (!send_message_to_editor("ERROR", errorMsgToSend))
        return false;

    while ((check_for_messages_from_editor() == 0) && (want_exit == 0))
    {
        UPDATE_MP3
            platform->Delay(10);
    }
#endif
    return true;
}


void break_into_debugger() 
{
#ifdef WINDOWS_VERSION

    if (editor_window_handle != NULL)
        SetForegroundWindow(editor_window_handle);

    send_message_to_editor("BREAK");
    game_paused_in_debugger = 1;

    while (game_paused_in_debugger) 
    {
        update_polled_stuff_if_runtime();
        platform->YieldCPU();
    }

#endif
}

int scrDebugWait = 0;
extern int pluginsWantingDebugHooks;

// allow LShift to single-step,  RShift to pause flow
void scriptDebugHook (ccInstance *ccinst, int linenum) {

    if (pluginsWantingDebugHooks > 0) {
        // a plugin is handling the debugging
        CString scname = get_script_name(ccinst);
        platform->RunPluginDebugHooks(scname, linenum);
        return;
    }

    // no plugin, use built-in debugger

    if (ccinst == NULL) 
    {
        // come out of script
        return;
    }

    if (break_on_next_script_step) 
    {
        break_on_next_script_step = 0;
        break_into_debugger();
        return;
    }

    CString scriptName = ccGetSectionNameAtOffs(ccinst->runningInst->instanceof, ccinst->pc);

    for (int i = 0; i < numBreakpoints; i++)
    {
        if ((breakpoints[i].lineNumber == linenum) &&
            (breakpoints[i].scriptName.Compare(scriptName) == 0))
        {
            break_into_debugger();
            break;
        }
    }
}

int scrlockWasDown = 0;

void check_debug_keys() {
    if (play.debug_mode) {
        // do the run-time script debugging

        if ((!key[KEY_SCRLOCK]) && (scrlockWasDown))
            scrlockWasDown = 0;
        else if ((key[KEY_SCRLOCK]) && (!scrlockWasDown)) {

            break_on_next_script_step = 1;
            scrlockWasDown = 1;
        }

    }

}
