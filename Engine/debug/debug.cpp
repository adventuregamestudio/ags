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
#include <limits>
#include <inttypes.h>
#include <memory>
#include <stdio.h>
#include "core/platform.h"
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#endif
#include <SDL.h>
#include "ac/common.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/runtime_defines.h"
#include "debug/agseditordebugger.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/debugmanager.h"
#include "debug/out.h"
#include "debug/logfile.h"
#include "debug/messagebuffer.h"
#include "main/config.h"
#include "main/game_run.h"
#include "media/audio/audio_system.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/sys_main.h"
#include "plugin/plugin_engine.h"
#include "script/script.h"
#include "script/cc_common.h"
#include "util/path.h"
#include "util/string_utils.h"
#include "util/textstreamwriter.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern char check_dynamic_sprites_at_exit;
extern int displayed_room;
extern RoomStruct thisroom;
extern volatile bool want_exit, abort_engine;
extern GameSetupStruct game;


int editor_debugging_enabled = 0;
int editor_debugging_initialized = 0;
char editor_debugger_instance_token[100];
IAGSEditorDebugger *editor_debugger = nullptr;
int break_on_next_script_step = 0;
volatile int game_paused_in_debugger = 0;

#if AGS_PLATFORM_OS_WINDOWS

#include "platform/windows/debug/namedpipesagsdebugger.h"

HWND editor_window_handle = 0;

IAGSEditorDebugger *GetEditorDebugger(const char *instanceToken)
{
    return new NamedPipesAGSDebugger(instanceToken);
}

#else   // AGS_PLATFORM_OS_WINDOWS

IAGSEditorDebugger *GetEditorDebugger(const char* /*instanceToken*/)
{
    return nullptr;
}

#endif

int debug_flags=0;

FPSDisplayMode display_fps = kFPS_Hide;

void send_message_to_debugger(IAGSEditorDebugger *ide_debugger,
    const std::vector<std::pair<String, String>>& tag_values, const String& command)
{
    String messageToSend = String::FromFormat(R"(<?xml version="1.0" encoding="Windows-1252"?><Debugger Command="%s">)", command.GetCStr());
#if AGS_PLATFORM_OS_WINDOWS
    messageToSend.Append(String::FromFormat("  <EngineWindow>%" PRIdPTR "</EngineWindow> ", sys_win_get_window()));
#endif

    for(const auto& tag_value : tag_values)
    {
        messageToSend.AppendFmt("  <%s><![CDATA[%s]]></%s> ",
                                tag_value.first.GetCStr(), tag_value.second.GetCStr(), tag_value.first.GetCStr());
    }

    messageToSend.Append("</Debugger>\n");

    ide_debugger->SendMessageToEditor(messageToSend.GetCStr());
}

class DebuggerLogOutputTarget : public AGS::Common::IOutputHandler
{
public:
    DebuggerLogOutputTarget(IAGSEditorDebugger *ide_debugger)
        : _ideDebugger(ide_debugger) {};
    virtual ~DebuggerLogOutputTarget() {};

    void PrintMessage(const DebugMessage &msg) override
    {
        assert(_ideDebugger);
        std::vector<std::pair<String, String>> log_info =
                {
                        {"Text", msg.Text},
                        {"GroupID", StrUtil::IntToString(msg.GroupID)},
                        {"MTID", StrUtil::IntToString(msg.MT)}
                };
        send_message_to_debugger(_ideDebugger, log_info, "LOG");
    }
private:
    IAGSEditorDebugger *_ideDebugger = nullptr;
};

std::unique_ptr<MessageBuffer> DebugMsgBuff;
std::unique_ptr<LogFile> DebugLogFile;
std::unique_ptr<DebuggerLogOutputTarget> DebuggerLog;

const String OutputMsgBufID = "buffer";
const String OutputFileID = "file";
const String OutputSystemID = "stdout";
const String OutputDebuggerLogID = "debugger";


// ----------------------------------------------------------------------------
// SDL log output
// ----------------------------------------------------------------------------

// SDL log priority names
static const char *SDL_priority[SDL_NUM_LOG_PRIORITIES] = {
    nullptr, "VERBOSE", "DEBUG", "INFO", "WARN", "ERROR", "CRITICAL"
};
// SDL log category names
static const char *SDL_category[SDL_LOG_CATEGORY_RESERVED1] = {
    "APP", "ERROR", "SYSTEM", "AUDIO", "VIDEO", "RENDER", "INPUT"
};
// Conversion between SDL priorities and our MessageTypes
static MessageType SDL_to_MT[SDL_NUM_LOG_PRIORITIES] = {
    kDbgMsg_None, kDbgMsg_All, kDbgMsg_Debug, kDbgMsg_Info, kDbgMsg_Warn, kDbgMsg_Error, kDbgMsg_Alert
};
// Print SDL message through our own log
void SDL_Log_Output(void* /*userdata*/, int category, SDL_LogPriority priority, const char *message) {
    DbgMgr.Print(kDbgGroup_SDL, SDL_to_MT[priority],
        String::FromFormat("%s: %s: %s", SDL_category[category], SDL_priority[priority], message));
}

// ----------------------------------------------------------------------------
// Log configuration
// ----------------------------------------------------------------------------

PDebugOutput create_log_output(const String &name, const String &path = "", LogFile::OpenMode open_mode = LogFile::kLogFile_Overwrite)
{
    // Else create new one, if we know this ID
    if (name.CompareNoCase(OutputSystemID) == 0)
    {
        return DbgMgr.RegisterOutput(OutputSystemID, AGSPlatformDriver::GetDriver(), kDbgMsg_None);
    }
    else if (name.CompareNoCase(OutputFileID) == 0)
    {
        DebugLogFile.reset(new LogFile());
        String logfile_path = path;
        if (logfile_path.IsEmpty())
        {
            FSLocation fs = platform->GetAppOutputDirectory();
            CreateFSDirs(fs);
            logfile_path = Path::ConcatPaths(fs.FullDir, "ags.log");
        }
        if (!DebugLogFile->OpenFile(logfile_path, open_mode))
            return nullptr;
        Debug::Printf(kDbgMsg_Info, "Logging to %s", logfile_path.GetCStr());
        auto dbgout = DbgMgr.RegisterOutput(OutputFileID, DebugLogFile.get(), kDbgMsg_None);
        return dbgout;
    }
    else if (name.CompareNoCase(OutputDebuggerLogID) == 0 &&
        editor_debugger != nullptr)
    {
        DebuggerLog.reset(new DebuggerLogOutputTarget(editor_debugger));
        return DbgMgr.RegisterOutput(OutputDebuggerLogID, DebuggerLog.get(), kDbgMsg_All);
    }
    return nullptr;
}

// Parses a string where each character defines a single log group; returns list of real group names.
std::vector<String> parse_log_multigroup(const String &group_str)
{
    std::vector<String> grplist;
    for (size_t i = 0; i < group_str.GetLength(); ++i)
    {
        switch (group_str[i])
        {
        case 'm': grplist.emplace_back("main"); break;
        case 'g': grplist.emplace_back("game"); break;
        case 's': grplist.emplace_back("script"); break;
        case 'c': grplist.emplace_back("sprcache"); break;
        case 'o': grplist.emplace_back("manobj"); break;
        case 'l': grplist.emplace_back("sdl"); break;
        }
    }
    return grplist;
}

MessageType get_messagetype_from_string(const String &option)
{
    if (option.CompareNoCase("all") == 0) return kDbgMsg_All;
    return StrUtil::ParseEnumAllowNum<MessageType>(option,
        CstrArr<kNumDbgMsg>{"", "alert", "fatal", "error", "warn", "info", "debug"},
        kDbgMsg_None);
}

typedef std::pair<CommonDebugGroup, MessageType> DbgGroupOption;

void apply_log_config(const ConfigTree &cfg, const String &log_id,
                      bool def_enabled,
                      std::initializer_list<DbgGroupOption> def_opts)
{
    String value = CfgReadString(cfg, "log", log_id);
    if (value.IsEmpty() && !def_enabled)
        return;

    // First test if already registered, if not then try create it
    auto dbgout = DbgMgr.GetOutput(log_id);
    const bool was_created_earlier = dbgout != nullptr;
    if (!dbgout)
    {
        String path = CfgReadString(cfg, "log", String::FromFormat("%s-path", log_id.GetCStr()));
        dbgout = create_log_output(log_id, path);
        if (!dbgout)
            return; // unknown output type
    }
    dbgout->ClearGroupFilters();

    if (value.IsEmpty() || value.CompareNoCase("default") == 0)
    {
        for (const auto &opt : def_opts)
            dbgout->SetGroupFilter(opt.first, opt.second);
    }
    else
    {
        const auto options = value.Split(',');
        for (const auto &opt : options)
        {
            String groupname = opt.LeftSection(':');
            MessageType msgtype = kDbgMsg_All;
            if (opt.GetLength() >= groupname.GetLength() + 1)
            {
                String msglevel = opt.Mid(groupname.GetLength() + 1);
                msglevel.Trim();
                if (msglevel.GetLength() > 0)
                    msgtype = get_messagetype_from_string(msglevel);
            }
            groupname.Trim();
            if (groupname.CompareNoCase("all") == 0 || groupname.IsEmpty())
            {
                dbgout->SetAllGroupFilters(msgtype);
            }
            else if (groupname[0u] != '+')
            {
                dbgout->SetGroupFilter(groupname, msgtype);
            }
            else
            {
                const auto groups = parse_log_multigroup(groupname);
                for (const auto &g : groups)
                    dbgout->SetGroupFilter(g, msgtype);
            }
        }
    }

    // Delegate buffered messages to this new output
    if (DebugMsgBuff && !was_created_earlier)
        DebugMsgBuff->Send(log_id);
}

void init_debug(const ConfigTree &cfg, bool stderr_only)
{
    // Setup SDL output
    SDL_LogSetOutputFunction(SDL_Log_Output, nullptr);
    String sdl_log = CfgReadString(cfg, "log", "sdl");
    SDL_LogPriority priority = StrUtil::ParseEnumAllowNum<SDL_LogPriority>(sdl_log,
        CstrArr<SDL_NUM_LOG_PRIORITIES>{"", "verbose", "debug", "info", "warn", "error", "critical"}, SDL_LOG_PRIORITY_INFO);
    SDL_LogSetAllPriority(priority);

    // Register outputs
    apply_debug_config(cfg);
    platform->SetOutputToErr(stderr_only);

    if (stderr_only)
        return;

    // Message buffer to save all messages in case we read different log settings from config file
    DebugMsgBuff.reset(new MessageBuffer());
    DbgMgr.RegisterOutput(OutputMsgBufID, DebugMsgBuff.get(), kDbgMsg_All);
}

void apply_debug_config(const ConfigTree &cfg)
{
    apply_log_config(cfg, OutputSystemID, /* defaults */ true,
        { DbgGroupOption(kDbgGroup_Main, kDbgMsg_Info),
          DbgGroupOption(kDbgGroup_SDL, kDbgMsg_Info),
        });
    bool legacy_log_enabled = CfgReadBoolInt(cfg, "misc", "log", false);

    apply_log_config(cfg, OutputFileID,
        /* defaults */
        legacy_log_enabled,
        { DbgGroupOption(kDbgGroup_Main, kDbgMsg_All),
          DbgGroupOption(kDbgGroup_SDL, kDbgMsg_Info),
          DbgGroupOption(kDbgGroup_Game, kDbgMsg_Info),
          DbgGroupOption(kDbgGroup_Script, kDbgMsg_All),
#if DEBUG_SPRITECACHE
          DbgGroupOption(kDbgGroup_SprCache, kDbgMsg_All),
#else
          DbgGroupOption(kDbgGroup_SprCache, kDbgMsg_Info),
#endif
#if DEBUG_MANAGED_OBJECTS
          DbgGroupOption(kDbgGroup_ManObj, kDbgMsg_All),
#else
          DbgGroupOption(kDbgGroup_ManObj, kDbgMsg_Info),
#endif
        });

    // If the game was compiled in Debug mode *and* there's no regular file log,
    // then open "warnings.log" for printing script warnings.
    if (game.options[OPT_DEBUGMODE] != 0 && !DebugLogFile)
    {
        auto dbgout = create_log_output(OutputFileID, "warnings.log", LogFile::kLogFile_OverwriteAtFirstMessage);
        if (dbgout)
        {
            dbgout->SetGroupFilter(kDbgGroup_Game, kDbgMsg_Warn);
            dbgout->SetGroupFilter(kDbgGroup_Script, kDbgMsg_Warn);
        }
    }

    // Engine -> editor logging
    if (editor_debugging_initialized)
    {
        apply_log_config(cfg, OutputDebuggerLogID, false, {});
    }

    // We don't need message buffer beyond this point
    DbgMgr.UnregisterOutput(OutputMsgBufID);
    DebugMsgBuff.reset();
}

void shutdown_debug()
{
    // Shutdown output subsystem
    DbgMgr.UnregisterAll();

    DebugMsgBuff.reset();
    DebugLogFile.reset();
    DebuggerLog.reset();
}

// Prepends message text with current room number and running script info, then logs result
static void debug_script_print_impl(const String &msg, MessageType mt)
{
    String script_ref;
    ccInstance *curinst = ccInstance::GetCurrentInstance();
    if (curinst != nullptr) {
        String scriptname;
        if (curinst->instanceof == gamescript)
            scriptname = "G ";
        else if (curinst->instanceof == thisroom.CompiledScript)
            scriptname = "R ";
        else if (curinst->instanceof == dialogScriptsScript)
            scriptname = "D ";
        else
            scriptname = "? ";
        script_ref.Format("[%s%d]", scriptname.GetCStr(), currentline);
    }

    Debug::Printf(kDbgGroup_Game, mt, "(room:%d)%s %s", displayed_room, script_ref.GetCStr(), msg.GetCStr());
}

void debug_script_print(MessageType mt, const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    String full_msg = String::FromFormatV(msg, ap);
    va_end(ap);
    debug_script_print_impl(full_msg, mt);
}

void debug_script_warn(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    String full_msg = String::FromFormatV(msg, ap);
    va_end(ap);
    debug_script_print_impl(full_msg, kDbgMsg_Warn);
}

void debug_script_log(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    String full_msg = String::FromFormatV(msg, ap);
    va_end(ap);
    debug_script_print_impl(full_msg, kDbgMsg_Debug);
}


struct Breakpoint
{
    char scriptName[80]{};
    int lineNumber = 0;
};

std::vector<Breakpoint> breakpoints;

bool send_state_to_debugger(const String& msg, const String& errorMsg)
{
    // Get either saved callstack from a script error, or current execution point
    String callStack = (!errorMsg.IsEmpty() && cc_has_error()) ?
        cc_get_error().CallStack : cc_get_callstack();
    if (callStack.IsEmpty())
        return false;

    std::vector<std::pair<String, String>> scipt_info = { { "ScriptState", callStack } };

    if (!errorMsg.IsEmpty())
    {
        scipt_info.emplace_back( "ErrorMessage", errorMsg );
    }

    send_message_to_debugger(editor_debugger, scipt_info, msg);
    return true;
}

bool send_state_to_debugger(const char *msg)
{
    return send_state_to_debugger(String(msg), String());
}

bool init_editor_debugging(const ConfigTree &cfg) 
{
#if AGS_PLATFORM_OS_WINDOWS
    editor_debugger = GetEditorDebugger(editor_debugger_instance_token);
#else
    // Editor isn't ported yet
    editor_debugger = nullptr;
#endif

    if (editor_debugger == nullptr)
        quit("editor_debugger is NULL but debugger enabled");

    if (editor_debugger->Initialize())
    {
        editor_debugging_initialized = 1;

        // Wait for the editor to send the initial breakpoints
        // and then its READY message
        while (check_for_messages_from_debugger() != 2)
        {
            platform->Delay(10);
        }

        send_state_to_debugger("START");
        Debug::Printf(kDbgMsg_Info, "External debugger initialized");
        // Create and configure engine->editor log output
        apply_log_config(cfg, OutputDebuggerLogID, false, {});
        return true;
    }

    Debug::Printf(kDbgMsg_Error, "Failed to initialize external debugger");
    return false;
}

int check_for_messages_from_debugger()
{
    if (editor_debugger->IsMessageAvailable())
    {
        char *msg = editor_debugger->GetNextMessage();
        if (msg == nullptr)
        {
            return 0;
        }

        if (strncmp(msg, "<Engine Command=\"", 17) != 0) 
        {
            //Debug::Printf("Faulty message received from editor:");
            //Debug::Printf(msg);
            free(msg);
            return 0;
        }

        const char *msgPtr = &msg[17];

        if (strncmp(msgPtr, "START", 5) == 0)
        {
#if AGS_PLATFORM_OS_WINDOWS
            const char *windowHandle = strstr(msgPtr, "EditorWindow") + 14;
            editor_window_handle = (HWND)atoi(windowHandle);
#endif
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
            char scriptNameBuf[sizeof(Breakpoint::scriptName)]{};
            for (size_t i = 0; msgPtr[0] != '$'; ++msgPtr, ++i)
            {
                if (i < sizeof(scriptNameBuf) - 1)
                    scriptNameBuf[i] = msgPtr[0];
            }
            msgPtr++;

            int lineNumber = atoi(msgPtr);

            if (isDelete) 
            {
                for (size_t i = 0; i < breakpoints.size(); ++i)
                {
                    if ((breakpoints[i].lineNumber == lineNumber) &&
                        (strcmp(breakpoints[i].scriptName, scriptNameBuf) == 0))
                    {
                        breakpoints.erase(breakpoints.begin() + i);
                        break;
                    }
                }
            }
            else 
            {
                Breakpoint bp;
                snprintf(bp.scriptName, sizeof(Breakpoint::scriptName), "%s", scriptNameBuf);
                bp.lineNumber = lineNumber;
                breakpoints.push_back(bp);
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
            want_exit = true;
            abort_engine = true;
            check_dynamic_sprites_at_exit = 0;
        }

        free(msg);
        return 1;
    }

    return 0;
}




bool send_exception_to_debugger(const char *qmsg)
{
#if AGS_PLATFORM_OS_WINDOWS
    want_exit = false;
    // allow the editor to break with the error message
    if (editor_window_handle != NULL)
        SetForegroundWindow(editor_window_handle);

    if (!send_state_to_debugger("ERROR", qmsg))
        return false;

    while ((check_for_messages_from_debugger() == 0) && (!want_exit))
    {
        platform->Delay(10);
    }
#else
    (void)qmsg;
#endif
    return true;
}


void break_into_debugger() 
{
#if AGS_PLATFORM_OS_WINDOWS

    if (editor_window_handle != NULL)
        SetForegroundWindow(editor_window_handle);

    send_state_to_debugger("BREAK");
    game_paused_in_debugger = 1;

    while (game_paused_in_debugger) 
    {
        update_polled_stuff();
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
        String scname = GetScriptName(ccinst);
        pl_run_plugin_debug_hooks(scname.GetCStr(), linenum);
        return;
    }

    // no plugin, use built-in debugger

    if (ccinst == nullptr) 
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

    for (const auto & breakpoint : breakpoints)
    {
        if ((breakpoint.lineNumber == linenum) &&
            (strcmp(breakpoint.scriptName, scriptName) == 0))
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

        const Uint8 *ks = SDL_GetKeyboardState(nullptr);
        if ((!ks[SDL_SCANCODE_SCROLLLOCK]) && (scrlockWasDown))
            scrlockWasDown = 0;
        else if ((ks[SDL_SCANCODE_SCROLLLOCK]) && (!scrlockWasDown)) {

            break_on_next_script_step = 1;
            scrlockWasDown = 1;
        }

    }

}
