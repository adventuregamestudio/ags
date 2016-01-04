//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include <stdio.h>
#include "ac/common.h"
#include "ac/roomstruct.h"
#include "ac/runtime_defines.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "gui/dynamicarray.h"
#include "debug/out.h"
#include "debug/consoleoutputtarget.h"
#include "debug/logfile.h"
#include "media/audio/audio.h"
#include "media/audio/soundclip.h"
#include "script/script.h"
#include "script/script_common.h"
#include "script/cc_error.h"
#include "util/filestream.h"
#include "util/textstreamwriter.h"

using AGS::Common::Stream;
using AGS::Common::String;
using AGS::Common::TextStreamWriter;
using AGS::Engine::Out::ConsoleOutputTarget;
using AGS::Engine::Out::LogFile;
namespace Out = AGS::Common::Out;

extern char check_dynamic_sprites_at_exit;
extern int displayed_room;
extern roomstruct thisroom;
extern char pexbuf[STD_BUFFER_SIZE];
extern volatile char want_exit, abort_engine;
extern ccScript* gamescript;
extern ccScript* dialogScriptsScript;


int use_compiled_folder_as_current_dir = 0;
int editor_debugging_enabled = 0;
int editor_debugging_initialized = 0;
char editor_debugger_instance_token[100];
IAGSEditorDebugger *editor_debugger = NULL;
int break_on_next_script_step = 0;
volatile int game_paused_in_debugger = 0;
HWND editor_window_handle = NULL;

#ifdef WINDOWS_VERSION

#include "platform/windows/debug/namedpipesagsdebugger.h"

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

int debug_flags=0;
bool enable_log_file = false;
bool disable_log_file = false;

DebugConsoleText debug_line[DEBUG_CONSOLE_NUMLINES];
int first_debug_line = 0, last_debug_line = 0, display_console = 0;

int fps=0,display_fps=0;

LogFile *DebugLogFile = NULL;

enum
{
    TARGET_FILE,
    TARGET_SYSTEMDEBUGGER,
    TARGET_GAMECONSOLE,
    TARGET_FILE_EXTRA_TEST,
};

void initialize_output_subsystem()
{
    DebugLogFile = new LogFile();

    Out::Init(0, NULL);
    Out::AddOutputTarget(TARGET_FILE, DebugLogFile, Out::kVerbose_NoDebug, true);
    Out::AddOutputTarget(TARGET_SYSTEMDEBUGGER, AGSPlatformDriver::GetDriver(),
        Out::kVerbose_WarnErrors, true);
	Out::AddOutputTarget(TARGET_GAMECONSOLE, new AGS::Engine::Out::ConsoleOutputTarget(),
        Out::kVerbose_Always, false);
}

void apply_output_configuration()
{
    if (disable_log_file)
    {
        enable_log_file = false;
    }
    else if (enable_log_file)
    {
        String logfile_path = platform->GetAppOutputDirectory();
        logfile_path.Append("/ags.log");
        if (DebugLogFile->OpenFile(logfile_path))
        {
            platform->WriteStdOutF("AGS: Logging to %s", logfile_path.GetCStr());
        }
        else
        {
            enable_log_file = false;
        }
    }

    if (!enable_log_file)
    {
        Out::RemoveOutputTarget(TARGET_FILE);
        delete DebugLogFile;
        DebugLogFile = NULL;
    }
}

void initialize_debug_system()
{
    initialize_output_subsystem();
}

void shutdown_debug_system()
{
    // Shutdown output subsystem
    Out::Shutdown();

    delete DebugLogFile;
    DebugLogFile = NULL;
}

void quitprintf(const char *texx, ...) {
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
    Out::FPrint(msg);
}

/* The idea of this is that non-essential errors such as "sound file not
found" are logged instead of exiting the program.
*/
void debug_log(const char *texx, ...) {
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

    //char*openmode = "at";
    if (first_time) {
        //openmode = "wt";
        first_time = 0;
    }
    Stream *outfil = Common::File::OpenFileWrite("warnings.log");
    if (outfil == NULL)
    {
        debug_write_console("* UNABLE TO WRITE TO WARNINGS.LOG");
        debug_write_console(displbuf);
    }
    else
    {
        TextStreamWriter writer(outfil);
        writer.WriteFormat("(in room %d): %s\n",displayed_room,displbuf);
    }
}


void debug_write_console (const char *msg, ...) {
    char displbuf[STD_BUFFER_SIZE];
    va_list ap;
    va_start(ap,msg);
    vsprintf(displbuf,msg,ap);
    va_end(ap);
    displbuf[99] = 0;

    strcpy (debug_line[last_debug_line].text, displbuf);
    ccInstance*curinst = ccInstance::GetCurrentInstance();
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

    platform->WriteStdOutF("AGS: %s (%s)", displbuf, debug_line[last_debug_line].script);

    last_debug_line = (last_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

    if (last_debug_line == first_debug_line)
        first_debug_line = (first_debug_line + 1) % DEBUG_CONSOLE_NUMLINES;

}


const char *get_cur_script(int numberOfLinesOfCallStack) {
    ccInstance::GetCurrentInstance()->GetCallStack(pexbuf, numberOfLinesOfCallStack);

    if (pexbuf[0] == 0)
        strcpy(pexbuf, ccErrorCallStack);

    return &pexbuf[0];
}

bool get_script_position(ScriptPosition &script_pos)
{
    ccInstance *cur_instance = ccInstance::GetCurrentInstance();
    if (cur_instance)
    {
        cur_instance->GetScriptPosition(script_pos);
        return true;
    }
    return false;
}

static const char* BREAK_MESSAGE = "BREAK";

struct Breakpoint
{
    char scriptName[80];
    int lineNumber;
};

DynamicArray<Breakpoint> breakpoints;
int numBreakpoints = 0;

bool send_message_to_editor(const char *msg, const char *errorMsg) 
{
    const char *callStack = get_cur_script(25);
    if (callStack[0] == 0)
        return false;

    char messageToSend[STD_BUFFER_SIZE];
    sprintf(messageToSend, "<?xml version=\"1.0\" encoding=\"Windows-1252\"?><Debugger Command=\"%s\">", msg);
#ifdef WINDOWS_VERSION
    sprintf(&messageToSend[strlen(messageToSend)], "  <EngineWindow>%d</EngineWindow> ", win_get_window());
#endif
    sprintf(&messageToSend[strlen(messageToSend)], "  <ScriptState><![CDATA[%s]]></ScriptState> ", callStack);
    if (errorMsg != NULL)
    {
        sprintf(&messageToSend[strlen(messageToSend)], "  <ErrorMessage><![CDATA[%s]]></ErrorMessage> ", errorMsg);
    }
    strcat(messageToSend, "</Debugger>");

    editor_debugger->SendMessageToEditor(messageToSend);

    return true;
}

bool send_message_to_editor(const char *msg) 
{
    return send_message_to_editor(msg, NULL);
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
        char *msg = editor_debugger->GetNextMessage();
        if (msg == NULL)
        {
            return 0;
        }

        if (strncmp(msg, "<Engine Command=\"", 17) != 0) 
        {
            //Out::FPrint("Faulty message received from editor:");
            //Out::FPrint(msg);
            free(msg);
            return 0;
        }

        const char *msgPtr = &msg[17];

        if (strncmp(msgPtr, "START", 5) == 0)
        {
            const char *windowHandle = strstr(msgPtr, "EditorWindow") + 14;
            editor_window_handle = (HWND)atoi(windowHandle);
        }
        else if (strncmp(msgPtr, "READY", 5) == 0)
        {
            free(msg);
            return 2;
        }
        else if ((strncmp(msgPtr, "SETBREAK", 8) == 0) ||
            (strncmp(msgPtr, "DELBREAK", 8) == 0))
        {
            bool isDelete = (msgPtr[0] == 'D');
            // Format:  SETBREAK $scriptname$lineNumber$
            msgPtr += 10;
            char scriptNameBuf[100];
            int i = 0;
            while (msgPtr[0] != '$')
            {
                scriptNameBuf[i] = msgPtr[0];
                msgPtr++;
                i++;
            }
            scriptNameBuf[i] = 0;
            msgPtr++;

            int lineNumber = atoi(msgPtr);

            if (isDelete) 
            {
                for (i = 0; i < numBreakpoints; i++)
                {
                    if ((breakpoints[i].lineNumber == lineNumber) &&
                        (strcmp(breakpoints[i].scriptName, scriptNameBuf) == 0))
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
                strcpy(breakpoints[numBreakpoints].scriptName, scriptNameBuf);
                breakpoints[numBreakpoints].lineNumber = lineNumber;
                numBreakpoints++;
            }
        }
        else if (strncmp(msgPtr, "RESUME", 6) == 0) 
        {
            game_paused_in_debugger = 0;
        }
        else if (strncmp(msgPtr, "STEP", 4) == 0) 
        {
            game_paused_in_debugger = 0;
            break_on_next_script_step = 1;
        }
        else if (strncmp(msgPtr, "EXIT", 4) == 0) 
        {
            want_exit = 1;
            abort_engine = 1;
            check_dynamic_sprites_at_exit = 0;
        }

        free(msg);
        return 1;
    }

    return 0;
}




bool send_exception_to_editor(const char *qmsg)
{
#ifdef WINDOWS_VERSION
    want_exit = 0;
    // allow the editor to break with the error message
    if (editor_window_handle != NULL)
        SetForegroundWindow(editor_window_handle);

    if (!send_message_to_editor("ERROR", qmsg))
        return false;

    while ((check_for_messages_from_editor() == 0) && (want_exit == 0))
    {
        update_mp3();
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
        char scname[40];
        ccinst->GetScriptName(scname);
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

    const char *scriptName = ccinst->runningInst->instanceof->GetSectionName(ccinst->pc);

    for (int i = 0; i < numBreakpoints; i++)
    {
        if ((breakpoints[i].lineNumber == linenum) &&
            (strcmp(breakpoints[i].scriptName, scriptName) == 0))
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
