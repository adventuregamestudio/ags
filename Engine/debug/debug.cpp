//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <algorithm>
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
#include "ac/dynobj/dynobj_manager.h"
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
#include "util/memory_compat.h"
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

    void OnRegister() override
    {
        // do nothing
    }

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

// TODO: return a std::unique_ptr<IOutputHandler>
static std::unique_ptr<IOutputHandler> create_log_output(const String &name,
    const String &path = "", LogFile::OpenMode open_mode = LogFile::kLogFile_Overwrite)
{
    // Else create new one, if we know this ID
    if (name.CompareNoCase(OutputSystemID) == 0)
    {
        return platform->GetStdOut();
    }
    else if (name.CompareNoCase(OutputFileID) == 0)
    {
        auto log_file = std::make_unique<LogFile>();
        String logfile_path = path;
        if (logfile_path.IsEmpty())
        {
            FSLocation fs = platform->GetAppOutputDirectory();
            CreateFSDirs(fs);
            logfile_path = Path::ConcatPaths(fs.FullDir, "ags.log");
        }
        if (!log_file->OpenFile(logfile_path, open_mode))
            return nullptr;
        return std::move(log_file);
    }
    else if (name.CompareNoCase(OutputDebuggerLogID) == 0 &&
        editor_debugger != nullptr)
    {
        return std::make_unique<DebuggerLogOutputTarget>(editor_debugger);
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

static void apply_log_config(const ConfigTree &cfg, const String &log_id,
                      bool def_enabled,
                      std::initializer_list<DbgGroupOption> def_opts)
{
    const String value = CfgReadString(cfg, "log", log_id);
    if (value.IsEmpty() && !def_enabled)
        return;

    // Setup message group filters
    MessageType def_verbosity = kDbgMsg_None;
    std::vector<std::pair<DebugGroupID, MessageType>> group_filters;
    if (value.IsEmpty() || value.CompareNoCase("default") == 0)
    {
        for (const auto &opt : def_opts)
            group_filters.push_back(std::make_pair(opt.first, opt.second));
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
                def_verbosity = msgtype;
            }
            else if (groupname[0u] != '+')
            {
                group_filters.push_back(std::make_pair(groupname, msgtype));
            }
            else
            {
                const auto groups = parse_log_multigroup(groupname);
                for (const auto &g : groups)
                    group_filters.push_back(std::make_pair(g, msgtype));
            }
        }
    }

    // Test if already registered, if not then try create it,
    // if it exists, then reset the filter settings
    if (DbgMgr.HasOutput(log_id))
    {
        DbgMgr.SetOutputFilters(log_id, def_verbosity, &group_filters);
    }
    else
    {
        String path = CfgReadString(cfg, "log", String::FromFormat("%s-path", log_id.GetCStr()));
        auto dbgout = create_log_output(log_id, path);
        if (!dbgout)
            return;
        DbgMgr.RegisterOutput(log_id, std::move(dbgout), def_verbosity, &group_filters);
    }
}

void init_debug(const ConfigTree &cfg, bool stderr_only)
{
    // Setup SDL output
    SDL_LogSetOutputFunction(SDL_Log_Output, nullptr);
    String sdl_log = CfgReadString(cfg, "log", "sdl");
    SDL_LogPriority priority = StrUtil::ParseEnumAllowNum<SDL_LogPriority>(sdl_log,
        CstrArr<SDL_NUM_LOG_PRIORITIES>{"", "verbose", "debug", "info", "warn", "error", "critical"}, SDL_LOG_PRIORITY_INFO);
    SDL_LogSetAllPriority(priority);

    // Init platform's stdout setting
    platform->SetOutputToErr(stderr_only);

    // Register outputs
    apply_debug_config(cfg);
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
    if (game.options[OPT_DEBUGMODE] != 0 && !DbgMgr.HasOutput(OutputFileID))
    {
        auto dbgout = create_log_output(OutputFileID, "warnings.log", LogFile::kLogFile_OverwriteAtFirstMessage);
        if (dbgout)
        {
            std::vector<std::pair<DebugGroupID, MessageType>> group_filters;
            group_filters.push_back(std::make_pair(kDbgGroup_Game, kDbgMsg_Warn));
            group_filters.push_back(std::make_pair(kDbgGroup_Script, kDbgMsg_Warn));
            DbgMgr.RegisterOutput(OutputFileID, std::move(dbgout), kDbgMsg_None, &group_filters);
        }
    }

    // Engine -> editor logging
    if (editor_debugging_initialized)
    {
        apply_log_config(cfg, OutputDebuggerLogID, false, {});
    }

    // We don't need message buffer beyond this point
    DbgMgr.StopMessageBuffering();
}

void shutdown_debug()
{
    // Shutdown output subsystem
    DbgMgr.UnregisterAll();
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

bool resolve_memory(const String &mem_id, size_t from, String &value,
    const uint8_t *mem_ptr, size_t memsize);

// Query memory using direct address instructions
bool query_memory_direct(const String &mem_id, String &value)
{
    // Format for DRAFT testing only:
    // x[N]:offset[,type[:offset,type[:...]]
    // where x can be -
    //  - g    - globalscript
    //  - m[N] - module
    //  - r    - room state
    // offset is in bytes
    // type can be:
    //  - c    - char (1 byte integer)
    //  - iN   - integer of given size in bytes, e.g. i1, i2, i4
    //  - fN   - float of given size in bytes, e.g. f4
    //  - dN   - plain data (struct, plain array), optionally of given size in bytes
    //  - s    - plain string (null-terminated sequences of chars)
    //  - p    - pointer, reserved
    //  - h    - handle (managed), an int32 that may be resolved to a pointer
    if (mem_id.GetLength() < 1)
        return false;

    ccInstance *inst = nullptr;
    switch (mem_id[0])
    {
    case '?':
        inst = ccInstance::GetCurrentInstance();
        break;
    case 'g':
        inst = gameinst.get();
        break;
    case 'm':
    {
        int module_id = atoi(&mem_id[1]);
        if (module_id < 0)
            return false;
        inst = moduleInst[module_id].get();
    }
    break;
    case 'r':
        inst = roominst.get();
        break;
    default:
        return false;
    }

    if (!inst)
        return false;

    size_t off_at = mem_id.FindChar(':');
    if (off_at == -1)
        return false;

    return resolve_memory(mem_id, off_at + 1, value,
        reinterpret_cast<const uint8_t*>(inst->globaldata), inst->globaldatasize);
}

bool resolve_memory(const String &mem_id, size_t from, String &value,
    const uint8_t *mem_ptr, size_t memsize)
{
    const size_t off_at = from;
    int offset = atoi(&mem_id[off_at]);
    char type = 0;
    int size = 0;

    size_t type_at = mem_id.FindChar(',', off_at);
    size_t next_off_at = mem_id.FindChar(':', off_at);
    
    if ((type_at != -1) && (type_at < next_off_at))
    {
        type = mem_id[type_at + 1];
        if (type_at + 2 < mem_id.GetLength())
            size = atoi(&mem_id[type_at + 2]);
    }
    else
    {
        // default tp 'i4'
        type = 'i';
        size = 4;
    }

    if (offset < 0 || ((memsize != -1) && (offset >= memsize + size)))
        return false;

    mem_ptr = mem_ptr + offset;

    if (next_off_at != -1)
    {
        // Resolve memory chain, if possible
        switch (type)
        {
        case 'd': // plain data, use same mem ptr
            memsize -= offset;
            break;
        case 'p': // value of a address, reserved but not supported atm
            return false;
        case 'h': // managed handle, resolve to managed object addr
        {
            const int32_t *value_ptr = reinterpret_cast<const int32_t*>(mem_ptr);
            const void *addr = ccGetObjectAddressFromHandle(*value_ptr);
            if (!addr)
                return false; // object not found
            mem_ptr = reinterpret_cast<const uint8_t*>(addr);
            memsize = -1; // FIXME... how to get it? use dynamic manager? need to expand iface
            break;
        }
        default: // invalid type for resolving, fail
            return false;
        }
        return resolve_memory(mem_id, next_off_at + 1, value,
            mem_ptr, memsize);
    }

    switch (type)
    {
    case 'c':
    {
        const int8_t *value_ptr = reinterpret_cast<const int8_t*>(mem_ptr);
        value = String::FromFormat("%c", *value_ptr);
        break;
    }
    case 'i':
        switch (size)
        {
        case 1:
        {
            const int8_t *value_ptr = reinterpret_cast<const int8_t*>(mem_ptr);
            value = String::FromFormat("%d", *value_ptr);
            break;
        }
        case 2:
        {
            const int16_t *value_ptr = reinterpret_cast<const int16_t*>(mem_ptr);
            value = String::FromFormat("%d", *value_ptr);
            break;
        }
        case 4:
        {
            const int32_t *value_ptr = reinterpret_cast<const int32_t*>(mem_ptr);
            value = String::FromFormat("%d", *value_ptr);
            break;
        }
        default: // unknown type, fail
            return false;
        }
        break;
    case 'f':
        switch (size)
        {
        case 4:
        {
            const float *value_ptr = reinterpret_cast<const float*>(mem_ptr);
            value = String::FromFormat("%f", *value_ptr);
            break;
        }
        default: // unknown type, fail
            return false;
        }
        break;
    case 'd':
        // TODO: convert requested size to base64?
        break;
    case 's':
        value = reinterpret_cast<const char*>(mem_ptr);
        break;
    case 'p': // value of a address, reserved
        break;
    case 'h': // value of a managed handle (int32)
    {
        const int32_t *value_ptr = reinterpret_cast<const int32_t*>(mem_ptr);
        value = String::FromFormat("%d", *value_ptr);
        break;
    }
    default: // unknown type, fail
        return false;
    }

    return true;
}

bool append_memid_item(const String &item, String &mem_id,
     uint32_t f_local_typeid, uint32_t f_flags, uint32_t f_offset, uint32_t arr_index,
    const RTTI::Type **next_type);

ccInstance *get_instance_by_locid(const String &loc_id)
{
    // ugly....
    if (loc_id.Compare(gameinst->instanceof->sectionNames[0].c_str()) == 0)
    {
        return gameinst.get();
    }
    else if (roominst && (loc_id.Compare(roominst->instanceof->sectionNames[0].c_str()) == 0))
    {
        return roominst.get();
    }
    else
    {
        for (size_t i = 0; i < moduleInst.size(); ++i)
        {
            if (loc_id.Compare(moduleInst[i]->instanceof->sectionNames[0].c_str()) == 0)
            {
                return moduleInst[i].get();
            }
        }
    }

    return nullptr;
}

// Query memory using variable names;
// resolves a name.name.name string to a address instruction and calls resolve_memory
bool query_memory_bytoc(const String &var_ref, String &value)
{
    // Format for DRAFT testing only:
    // [$scriptname:]symbolname[.symbolname[.symbolname[,...]]]
    if (var_ref.IsEmpty())
        return false;

    String mem_id;
    String items;
    ccInstance *inst = nullptr;
    if (var_ref[0] == '$')
    {
        String loc_id = var_ref.LeftSection('.');
        inst = get_instance_by_locid(loc_id);
        if (!inst)
            return false; // unknown script?

        items = var_ref.Mid(loc_id.GetLength() + 1);
    }
    else
    {
        inst = ccInstance::GetCurrentInstance();
        if (!inst)
            return false; // not in running script

        items = var_ref;
    }

    size_t index = 0;
    String item;
    const RTTI::Type *last_type = nullptr;
    const RTTI::Type *next_type = nullptr;
    const auto *datatoc = inst->GetDataTOC();
    const auto *local_rtti = inst->instanceof->rtti.get();
    const auto *l2gtypes = &inst->GetLocal2GlobalTypeMap();
    const void *mem_ptr = nullptr;
    size_t mem_size = 0u;

    for (item = items.Section('.', index, index);
        !item.IsEmpty();
         ++index, item = items.Section('.', index, index))
    {
        // fixme... this is not fully correct
        size_t has_array_at = item.FindChar('[');
        uint32_t array_index = -1;
        if (has_array_at != -1)
        {
            array_index = atoi(&item[has_array_at + 1]);
            item = item.Left(has_array_at);
        }

        if (next_type)
        {
            // get next member of type
            // todo: is it possible to speed this up, making a field lookup,
            // or that would be too costly to do per type?
            const RTTI::Field *next_field = nullptr;
            for (const auto *field = next_type->first_field; field; field = field->next_field)
            {
                if (strcmp(field->name, item.GetCStr()) == 0)
                {
                    next_field = field;
                    break;
                }
            }

            if (!next_field)
                return false; // cannot find next item

            mem_id.AppendChar(':');
            // note: f_typeid here is already resolved to global type id,
            // because we're using joint rtti, not local script's one
            if (!append_memid_item(item, mem_id, next_field->f_typeid, next_field->flags,
                next_field->offset, array_index, &next_type))
                return false;

            last_type = next_type;
        }
        else
        {
            // get variable from the script data
            const ScriptDataTOC::VariableDef *var_ptr = nullptr;
            
            // first try local variables
            bool found_local_var = false;
            uint32_t var_offset = 0u;
            if (datatoc->_sortedLocalVars.size() > 0)
            {
                // FIXME: think if this may be optimized...
                // FIXME: better way to pass arbitrary key?
                ScriptDataTOC::FunctionDef dummy_func; dummy_func.scope_begin = inst->pc;
                auto func_it = std::upper_bound(datatoc->_sortedFuncDefs.begin(), datatoc->_sortedFuncDefs.end(), dummy_func, FuncDef_Less);
                //if (func_it != datatoc->_sortedFuncDefs.end()) // FIXME?
                {
                    const auto &func = *(--func_it);
                    // NOTE: use lower_bound when searching for variable,
                    // because current stack offset is PAST the latest allocated local var,
                    // and may correspond to the next var. We find next var, and decrement once to get to the last allocated.
                    ScriptDataTOC::VariableDef dummy_var; dummy_var.scope_begin = inst->pc;// dummy_var.offset = (inst->stackdata_ptr - inst->stackdata);
                    auto var_it = std::lower_bound(datatoc->_sortedLocalVars.begin(), datatoc->_sortedLocalVars.end(), dummy_var, VarDef_Less);
                    if (var_it > datatoc->_sortedLocalVars.begin()) // cannot be begin, see above for explanation
                    {
                        const ScriptDataTOC::VariableDef *last_var = (var_it == datatoc->_sortedLocalVars.end()) ?
                            nullptr : &*var_it;
                        //const char *stackdata_ptr = inst->stackdata_ptr;
                        // FIXME: helper method returning stack? don't direct access!
                        const auto *stack_ptr = inst->registers[SREG_SP].RValue;
                        for (;
                               (var_it > datatoc->_sortedLocalVars.begin()) &&
                               (var_it == datatoc->_sortedLocalVars.end() ||
                               ((var_it - 1)->scope_begin >= func.scope_begin)); // next is not past function's declaration
                            )
                        {
                            --var_it; // begin searching with the previous entry
                            const ScriptDataTOC::VariableDef *var = &*var_it;

                            // skip if local variable's scope ends before current pos
                            if (var_it->scope_end < inst->pc)
                            {
                                continue;
                            }

                            //stackdata_ptr -= local_rtti->GetTypes()[var->f_typeid].size * std::min(1u, var->num_elems);
                            --stack_ptr;

                            // FIXME: very dirty hack: detect when we reach function parameter list,
                            // and skip 1 extra entry on stack, reserved for the return value
                            if ((var->flags & RTTI::kField_Parameter) &&
                                (!last_var || (last_var->flags & RTTI::kField_Parameter) == 0))
                            {
                                --stack_ptr;
                            }

                            if (var->name.Compare(item) == 0)
                            {
                                found_local_var = true;
                                var_ptr = var;
                                //mem_ptr = stackdata_ptr;
                                //mem_size = inst->stackdata_ptr - stackdata_ptr;
                                // FIXME: this is horrible....
                                if (stack_ptr->Type < kScValStackPtr)
                                    mem_ptr = &stack_ptr->IValue;
                                else
                                    mem_ptr = stack_ptr->GetDirectPtr();
                                mem_size = stack_ptr->Size;
                                break;
                            }

                            last_var = var;
                        }
                    }
                }
            }


            // if not found among locals, lookup the global one
            if (!found_local_var)
            {
                auto var_it = datatoc->_varLookup.find(item);
                if (var_it == datatoc->_varLookup.end())
                    return false; // cannot find next item

                const auto &var = datatoc->VarDefs[var_it->second];;
                var_ptr = &var;

                // fixup instance if it's an imported variable
                // FIXME: following variants may be simplified by recording variable ptr
                if ((var.flags & RTTI::kField_Import) != 0)
                {
                    if (var.loc_id.IsEmpty())
                    {
                        mem_ptr = var.mem_ptr;
                        mem_size = -1; // ?
                    }
                    else
                    {
                        inst = get_instance_by_locid(var.loc_id);
                        if (!inst)
                            return false; // unknown script?
                        mem_ptr = inst->globaldata;
                        mem_size = inst->globaldatasize;
                    }
                }
                else
                {
                    mem_ptr = inst->globaldata;
                    mem_size = inst->globaldatasize;
                }

                var_offset = var.offset;
            }

            const auto &var = *var_ptr;
            // resolve local script's type to a global type index
            // todo: this should be resolved after loading script, similar to RTTI!
            auto type_it = l2gtypes->find(var.f_typeid);
            if (type_it == l2gtypes->end())
                return false; // cannot find global type
            uint32_t g_typeid = type_it->second;
            
            if (!append_memid_item(item, mem_id, g_typeid, var.flags,
                    var_offset, array_index, &next_type))
                return false;

            last_type = next_type;
        }
    }

    if (mem_id.IsEmpty())
        return false;

    // FIXME: this is a hack, force resolve managed "String" type
    // so that user receives a string value instead of a handle;
    // need to think how to deal with this situation in a generic way.
    if (((last_type->flags & RTTI::kType_Managed) != 0) &&
        (strcmp(last_type->name, "String") == 0))
    {
        mem_id.Append(":0,s");
    }

    return resolve_memory(mem_id, 0, value,
        reinterpret_cast<const uint8_t*>(mem_ptr), mem_size);
}

bool append_memid_item(const String &item, String &mem_id,
    uint32_t g_typeid, uint32_t f_flags, uint32_t f_offset, uint32_t arr_index,
    const RTTI::Type **next_type)
{
    const auto *rtti = ccInstance::GetRTTI();
    const auto &type = rtti->GetTypes()[g_typeid];
    *next_type = &type;

    if ((f_flags & RTTI::kField_Array) != 0)
    {
        // dynamic array or regular array
        if ((f_flags & RTTI::kField_ManagedPtr) != 0)
            mem_id.AppendFmt("%u,h", f_offset);
        else
            mem_id.AppendFmt("%u,d0", f_offset); // fixme pass full array size (in bytes)

        if (arr_index == UINT32_MAX)
            return true; // return array itself... ?

        mem_id.AppendChar(':');
        // new offset is relative to array
        f_offset = arr_index * type.size;
    }

    // erm... using hardcoded type names here, no other way?...
    // todo: generate a table of basic types, avoid checking name every time,
    // check uint id instead!
    char typec = '?';
    uint32_t typesz = 0;
    if (strcmp(type.name, "char") == 0)
    {
        typec = 'i'; typesz = 1;
    }
    else if (strcmp(type.name, "short") == 0)
    {
        typec = 'i'; typesz = 2;
    }
    else if (strcmp(type.name, "int") == 0)
    {
        typec = 'i'; typesz = 4;
    }
    else if (strcmp(type.name, "float") == 0)
    {
        typec = 'f'; typesz = 4;
    }
    else if (strcmp(type.name, "string") == 0)
    {
        typec = 's'; typesz = 200; // old-style string of fixed size
    }
    else if ((type.flags & RTTI::kType_Managed) != 0)
    {
        typec = 'h'; typesz = 4;
    }
    else
    {
        typec = 'd'; typesz = type.size; // ?
    }

    mem_id.AppendFmt("%u,%c%u", f_offset, typec, typesz);
    return true;
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
        else if (strncmp(msgPtr, "GETMEM2", 7) == 0)
        {
            // Format:  GETMEM $requestID$variableChain$
            const char *req_id_str = strstr(msgPtr + 6, "$");
            if (!req_id_str)
            {
                free(msg);
                return 0;
            }
            const char *var_ref_str = strstr(req_id_str + 1, "$");
            if (!var_ref_str)
            {
                free(msg);
                return 0;
            }
            const char *end_str = strstr(var_ref_str + 1, "$");
            if (!end_str)
            {
                free(msg);
                return 0;
            }

            String req_id(req_id_str + 1, var_ref_str - req_id_str - 1);
            String var_ref(var_ref_str + 1, end_str - var_ref_str - 1);
            String mem_value;
            if (!query_memory_bytoc(var_ref, mem_value))
            {
                mem_value = "NOT FOUND";
            }
            std::vector<std::pair<String, String>> values;
            values.push_back(std::make_pair("ReqID", req_id));
            values.push_back(std::make_pair("Value", mem_value));
            send_message_to_debugger(editor_debugger, values, "REVMEM");
        }
        else if (strncmp(msgPtr, "GETMEM", 6) == 0)
        {
            // Format:  GETMEM $requestID$memoryID$
            const char *req_id_str = strstr(msgPtr + 6, "$");
            if (!req_id_str)
            {
                free(msg);
                return 0;
            }
            const char *mem_id_str = strstr(req_id_str + 1, "$");
            if (!mem_id_str)
            {
                free(msg);
                return 0;
            }
            const char *end_str = strstr(mem_id_str + 1, "$");
            if (!end_str)
            {
                free(msg);
                return 0;
            }

            String req_id(req_id_str + 1, mem_id_str - req_id_str - 1);
            String mem_id(mem_id_str + 1, end_str - mem_id_str - 1);
            String mem_value;
            if (!query_memory_direct(mem_id, mem_value))
            {
                mem_value = "NOT FOUND";
            }
            std::vector<std::pair<String, String>> values;
            values.push_back(std::make_pair("ReqID", req_id));
            values.push_back(std::make_pair("Value", mem_value));
            send_message_to_debugger(editor_debugger, values, "REVMEM");
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
