// AGS Editor ImGui - Build System implementation
// Implements the full game build pipeline:
//   1. Pre-build validation
//   2. Auto-generated script header creation
//   3. Script compilation pipeline (preprocessor + compiler)
//   4. Game data file generation (ac2game.dta)
//   5. Asset packaging (game.ags CLIB pack)
//   6. Engine file copying per platform
//   7. Config file generation (acsetup.cfg)
//   8. Game launching (fork+exec)

#include "build_system.h"
#include "game_data_writer.h"
#include "default_fonts.h"
#include "project/project.h"
#include "project/game_data.h"
#include "compiler/compiler_bridge.h"  // for kScriptAPIVersions, kScriptAPIHighest
#include "core/logger.h"

#include "script/cs_compiler.h"
#include "script/cc_common.h"
#include "script/cc_script.h"
#include "preproc/preprocessor.h"

#include "util/file.h"
#include "util/path.h"
#include "util/multifilelib.h"
#include "core/asset.h"
#include "data/dialogscriptconv.h"
#include "data/script_utils.h"
#include "data/scriptgen.h"
#include "data/tra_utils.h"
#include "data/mfl_utils.h"
#include "game/tra_file.h"

#include <algorithm>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#endif

namespace AGSEditor
{

// =========================================================================
// BuildTarget helpers
// =========================================================================
const char* BuildTargetName(BuildTarget target)
{
    switch (target) {
        case BuildTarget::DataFile: return "Data File";
        case BuildTarget::Linux:    return "Linux";
        case BuildTarget::Windows:  return "Windows";
        case BuildTarget::MacOS:    return "macOS";
        case BuildTarget::Web:      return "Web";
        case BuildTarget::Debug:    return "Debug";
        default: return "Unknown";
    }
}

const char* BuildTargetDirName(BuildTarget target)
{
    switch (target) {
        case BuildTarget::DataFile: return "Data";
        case BuildTarget::Linux:    return "Linux";
        case BuildTarget::Windows:  return "Windows";
        case BuildTarget::MacOS:    return "macOS";
        case BuildTarget::Web:      return "Web";
        case BuildTarget::Debug:    return "Debug";
        default: return "Unknown";
    }
}

// =========================================================================
// BuildConfig
// =========================================================================
void BuildConfig::SetDefaults(const std::string& project_dir)
{
    output_base_dir = project_dir + "/Compiled";

    // Try to find the engine executable relative to the editor binary
    // or from the build directory
#ifdef _WIN32
    engine_windows_path = "acwin.exe";
#else
    // On Linux, try common locations
    engine_linux_path = "";
    // Check if 'ags' is in the same build directory as the editor
    // This will be resolved at runtime
#endif
}

// =========================================================================
// BuildResult
// =========================================================================
int BuildResult::ErrorCount() const
{
    int count = 0;
    for (const auto& m : messages)
        if (m.type == BuildMessageType::Error) count++;
    return count;
}

int BuildResult::WarningCount() const
{
    int count = 0;
    for (const auto& m : messages)
        if (m.type == BuildMessageType::Warning) count++;
    return count;
}

void BuildResult::AddError(const std::string& file, int line, const std::string& msg,
                            const std::string& module)
{
    messages.push_back({BuildMessageType::Error, file, line, 0, msg, module});
}

void BuildResult::AddWarning(const std::string& file, int line, const std::string& msg,
                              const std::string& module)
{
    messages.push_back({BuildMessageType::Warning, file, line, 0, msg, module});
}

void BuildResult::AddInfo(const std::string& msg)
{
    messages.push_back({BuildMessageType::Info, "", 0, 0, msg, ""});
}

// =========================================================================
// BuildSystem
// =========================================================================
BuildSystem::BuildSystem() = default;
BuildSystem::~BuildSystem()
{
    StopGame();
}

// -------------------------------------------------------------------------
// Logging helper
// -------------------------------------------------------------------------
void BuildSystem::Log(const char* fmt, ...)
{
    if (!log_cb_) return;
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    log_cb_(std::string(buf));
}

// -------------------------------------------------------------------------
// Path helpers
// -------------------------------------------------------------------------
std::string BuildSystem::GetCompiledDir(const BuildConfig& config) const
{
    return config.output_base_dir;
}

std::string BuildSystem::GetDataDir(const BuildConfig& config) const
{
    return config.output_base_dir + "/Data";
}

std::string BuildSystem::GetTargetDir(const BuildConfig& config, BuildTarget target) const
{
    return config.output_base_dir + "/" + BuildTargetDirName(target);
}

std::string BuildSystem::FindEngineExecutable(const BuildConfig& config) const
{
#ifndef _WIN32
    // Check explicit config path first
    if (!config.engine_linux_path.empty())
    {
        if (std::filesystem::is_regular_file(config.engine_linux_path))
            return config.engine_linux_path;
    }
    // Resolve paths relative to the editor binary's directory
    std::string editor_dir;
    {
        char exe_path[PATH_MAX] = {};
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len > 0) {
            exe_path[len] = '\0';
            // Strip the binary name to get the directory
            char *last_slash = strrchr(exe_path, '/');
            if (last_slash) {
                *last_slash = '\0';
                editor_dir = exe_path;
            }
        }
    }

    // Try to find 'ags' in common locations
    std::vector<std::string> search_paths;
    if (!editor_dir.empty()) {
        // Same directory as the editor binary (typical build layout)
        search_paths.push_back(editor_dir + "/../../ags");
        search_paths.push_back(editor_dir + "/../ags");
        search_paths.push_back(editor_dir + "/ags");
    }
    // CWD-relative fallbacks
    search_paths.push_back("./ags");
    // System paths
    search_paths.push_back("/usr/bin/ags");
    search_paths.push_back("/usr/local/bin/ags");
    search_paths.push_back("/usr/games/ags");

    for (const auto& path : search_paths)
    {
        auto fst = std::filesystem::status(path);
        if (std::filesystem::is_regular_file(fst) &&
            (fst.permissions() & std::filesystem::perms::owner_exec) != std::filesystem::perms::none)
        {
            // Return absolute path via realpath
            char resolved[PATH_MAX] = {};
            if (realpath(path.c_str(), resolved))
                return std::string(resolved);
            return path;
        }
    }
#else
    if (!config.engine_windows_path.empty())
    {
        if (std::filesystem::exists(config.engine_windows_path))
            return config.engine_windows_path;
    }
#endif

    return "";
}

// -------------------------------------------------------------------------
// Directory creation (recursive)
// -------------------------------------------------------------------------
static bool MakeDirRecursive(const std::string& path)
{
    if (path.empty()) return false;
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    return !ec;
}

static bool FileExists(const std::string& path)
{
    return std::filesystem::exists(path);
}

// Check if source_file is newer than dest_file (needs recompile)
static bool DoesFileNeedRecompile(const std::string& source_file, const std::string& dest_file)
{
    if (!FileExists(source_file))
        return false;
    if (!FileExists(dest_file))
        return true;
    return std::filesystem::last_write_time(source_file) >= std::filesystem::last_write_time(dest_file);
}

// Case-insensitive file lookup: if <dir>/<filename> doesn't exist,
// scan the directory for a case-insensitive match.
// Returns the resolved path, or the original path if no match found.
static std::string FindFileCaseInsensitive(const std::string& dir,
                                           const std::string& filename)
{
    std::string full = dir + "/" + filename;
    if (FileExists(full)) return full;

#ifndef _WIN32
    // Lower-case the target filename for comparison
    std::string lower_target = filename;
    for (auto& c : lower_target) c = (char)tolower(c);

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        std::string entry_name = entry.path().filename().string();
        std::string lower_entry = entry_name;
        for (auto& c : lower_entry) c = (char)tolower(c);
        if (lower_entry == lower_target)
            return dir + "/" + entry_name;
    }
#endif

    return full;
}

static bool CopyFile_(const std::string& src, const std::string& dst)
{
    std::ifstream in(src, std::ios::binary);
    if (!in.is_open())
    {
        fprintf(stderr, "[Build] CopyFile_ failed: cannot read '%s'\n", src.c_str());
        return false;
    }
    std::ofstream out(dst, std::ios::binary);
    if (!out.is_open())
    {
        fprintf(stderr, "[Build] CopyFile_ failed: cannot write '%s'\n", dst.c_str());
        return false;
    }
    out << in.rdbuf();
    return true;
}

// -------------------------------------------------------------------------
// Create build output directories
// -------------------------------------------------------------------------
bool BuildSystem::CreateBuildDirectories(const BuildConfig& config, BuildResult& result)
{
    std::string base = GetCompiledDir(config);
    MakeDirRecursive(base);
    MakeDirRecursive(GetDataDir(config));

    for (int i = 0; i < (int)BuildTarget::Count; i++)
    {
        if (config.targets[i])
        {
            std::string dir = GetTargetDir(config, (BuildTarget)i);
            MakeDirRecursive(dir);
        }
    }

    // Temp directory for intermediate files
    MakeDirRecursive(base + "/_temp");

    return true;
}

// -------------------------------------------------------------------------
// Ensure default WFN font files exist in the project directory.
// The C# editor does this at project load time (Tasks.AddFontIfNotAlreadyThere).
// Creates agsfntN.wfn for fonts 0-2 if neither .wfn nor .ttf exists.
// -------------------------------------------------------------------------
void BuildSystem::EnsureDefaultFonts(Project& project)
{
    std::string dir = project.GetProjectDir();
    for (int i = 0; i < AGSEditor::kDefaultFontCount; i++)
    {
        std::string ttf_path = dir + "/agsfnt" + std::to_string(i) + ".ttf";
        std::string wfn_path = dir + "/agsfnt" + std::to_string(i) + ".wfn";
        if (FileExists(ttf_path) || FileExists(wfn_path))
            continue;
        // Write default WFN
        std::ofstream out(wfn_path, std::ios::binary);
        if (out.is_open())
        {
            out.write(reinterpret_cast<const char*>(AGSEditor::kDefaultFonts[i].data),
                      (std::streamsize)AGSEditor::kDefaultFonts[i].size);
            out.close();
            Log("[Build] Created default font: agsfnt%d.wfn", i);
        }
    }
}

// -------------------------------------------------------------------------
// Pre-build validation
// -------------------------------------------------------------------------
bool BuildSystem::PreBuildChecks(Project& project, BuildResult& result)
{
    if (!project.IsLoaded())
    {
        result.AddError("", 0, "No project is loaded.");
        return false;
    }

    GameData* gd = project.GetGameData();
    if (!gd)
    {
        result.AddError("", 0, "Game data is null.");
        return false;
    }

    // Check that we have at least one script module
    if (gd->script_modules.empty())
    {
        result.AddError("", 0, "No script modules found in the project.");
        return false;
    }

    // Check for player character
    bool has_player = false;
    for (const auto& ch : gd->characters)
    {
        if (ch.id == 0)
        {
            has_player = true;
            break;
        }
    }
    if (!has_player && !gd->characters.empty())
        result.AddWarning("", 0, "No player character (ID 0) found.");

    // Check for at least one room
    if (gd->rooms.empty())
        result.AddWarning("", 0, "No rooms defined in the project.");

    // Ensure default font files exist in project directory
    // (the C# editor does this at project load time via SetDefaultGameContentIfMissing)
    EnsureDefaultFonts(project);

    // Check that audio clip source files exist
    std::string proj_dir = project.GetProjectDir();
    for (const auto& clip : gd->audio_clips)
    {
        if (!clip.filename.empty())
        {
            std::string clip_path = proj_dir + "/" + clip.filename;
            if (!FileExists(clip_path))
            {
                result.AddWarning("", 0,
                    "Audio clip '" + clip.name + "' references missing file: " + clip.filename);
            }
        }
    }

    // Check that room files exist
    for (const auto& room : gd->rooms)
    {
        char room_name[64];
        snprintf(room_name, sizeof(room_name), "room%d.crm", room.number);
        std::string room_path = proj_dir + "/" + room_name;
        if (!FileExists(room_path))
        {
            result.AddWarning("", 0,
                "Room " + std::to_string(room.number) + " is defined but " +
                room_name + " was not found in the project directory.");
        }
    }

    // Check that font files exist
    for (const auto& font : gd->fonts)
    {
        char ttf_name[64], wfn_name[64];
        snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", font.id);
        snprintf(wfn_name, sizeof(wfn_name), "agsfnt%d.wfn", font.id);
        std::string ttf_path = proj_dir + "/" + ttf_name;
        std::string wfn_path = proj_dir + "/" + wfn_name;
        if (!FileExists(ttf_path) && !FileExists(wfn_path))
        {
            result.AddWarning("", 0,
                "Font " + std::to_string(font.id) + " has no file (" +
                ttf_name + " or " + wfn_name + ") in the project directory.");
        }
    }

    // Check room script-header dependencies (recompile stale rooms)
    // A room needs recompile if its .crm is older than its .asc or any .ash header.
    // Mirrors C# editor's RecompileAnyRoomsWhereTheScriptHasChanged().
    for (const auto& room : gd->rooms)
    {
        char crm_name[64], asc_name[64];
        snprintf(crm_name, sizeof(crm_name), "room%d.crm", room.number);
        snprintf(asc_name, sizeof(asc_name), "room%d.asc", room.number);
        std::string crm_path = proj_dir + "/" + crm_name;
        std::string asc_path = proj_dir + "/" + asc_name;

        if (!FileExists(crm_path))
            continue; // already warned above

        bool stale = false;
        std::string reason;

        // Check if room script is newer than compiled room
        if (FileExists(asc_path) && DoesFileNeedRecompile(asc_path, crm_path))
        {
            stale = true;
            reason = "room script changed";
        }

        // Check if any script header (.ash) is newer than compiled room
        if (!stale)
        {
            for (const auto& mod : gd->script_modules)
            {
                if (!mod.header_file.empty())
                {
                    std::string ash_path = proj_dir + "/" + mod.header_file;
                    if (DoesFileNeedRecompile(ash_path, crm_path))
                    {
                        stale = true;
                        reason = "script header '" + mod.header_file + "' changed";
                        break;
                    }
                }
            }
        }

        if (stale)
        {
            result.AddWarning("", 0,
                "Room " + std::to_string(room.number) + " may need recompilation (" +
                reason + "). Its .crm file is older than dependent sources.");
        }
    }

    return true;
}

// -------------------------------------------------------------------------
// Auto-generated script header (_AutoGenerated.ash)
// Mirrors the C# editor's Tasks.RegenerateScriptHeader()
// -------------------------------------------------------------------------

// Build a GameRef from the editor's GameData — provides the bridge between
// the editor's rich data model and the lightweight scriptgen structures.
static AGS::DataUtil::GameRef BuildGameRef(const GameData& gd)
{
    using namespace AGS::DataUtil;
    GameRef ref;

    // Helper: strip non-alphanumeric characters (except underscore) from names,
    // matching the sanitization in agfreader's ReadEntityRef.
    auto sanitize = [](const std::string& name) -> String {
        String result;
        for (char ch : name)
        {
            if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')
                result.AppendChar(ch);
        }
        return result;
    };

    // Audio clips
    for (const auto& ac : gd.audio_clips)
    {
        EntityRef e;
        e.TypeName = "AudioClip";
        e.ID = ac.id;
        e.ScriptName = sanitize(ac.name);
        ref.AudioClips.push_back(e);
    }

    // Audio clip types
    for (const auto& at : gd.audio_clip_types)
    {
        EntityRef e;
        e.ID = at.id;
        e.ScriptName = sanitize(at.name);
        ref.AudioTypes.push_back(e);
    }

    // Characters
    for (const auto& ch : gd.characters)
    {
        CharacterRef e;
        e.TypeName = "Character";
        e.ID = ch.id;
        e.ScriptName = sanitize(ch.script_name);
        ref.Characters.push_back(e);
    }

    // Cursors
    for (const auto& c : gd.cursors)
    {
        EntityRef e;
        e.ID = c.id;
        e.ScriptName = sanitize(c.name);
        ref.Cursors.push_back(e);
    }

    // Dialogs
    for (const auto& d : gd.dialogs)
    {
        DialogRef dr;
        dr.TypeName = "Dialog";
        dr.ID = d.id;
        dr.ScriptName = sanitize(d.script_name);
        dr.OptionCount = d.option_count;
        ref.Dialogs.push_back(dr);
    }

    // Fonts
    for (const auto& f : gd.fonts)
    {
        EntityRef e;
        e.ID = f.id;
        e.ScriptName = sanitize(f.name);
        ref.Fonts.push_back(e);
    }

    // GUIs (with controls)
    for (const auto& g : gd.guis)
    {
        GUIRef gr;
        gr.TypeName = "GUI";
        gr.ID = g.id;
        gr.ScriptName = sanitize(g.name);
        for (const auto& ctrl : g.controls)
        {
            EntityRef ce;
            ce.ID = ctrl.id;
            ce.ScriptName = sanitize(ctrl.name);
            // Map type_tag to script class type (matches C# GUIControl.ScriptClassType)
            if (ctrl.type_tag == "GUIButton")          ce.TypeName = "Button";
            else if (ctrl.type_tag == "GUILabel")      ce.TypeName = "Label";
            else if (ctrl.type_tag == "GUISlider")     ce.TypeName = "Slider";
            else if (ctrl.type_tag == "GUIListBox")    ce.TypeName = "ListBox";
            else if (ctrl.type_tag == "GUITextBox")    ce.TypeName = "TextBox";
            else if (ctrl.type_tag == "GUIInventory")  ce.TypeName = "InvWindow";
            else if (ctrl.type_tag == "GUITextWindowEdge") ce.TypeName = "Button";
            gr.Controls.push_back(ce);
        }
        ref.GUI.push_back(gr);
    }

    // Inventory items
    for (const auto& inv : gd.inventory_items)
    {
        EntityRef e;
        e.TypeName = "InventoryItem";
        e.ID = inv.id;
        e.ScriptName = sanitize(inv.script_name);
        ref.Inventory.push_back(e);
    }

    // Views
    for (const auto& v : gd.views)
    {
        EntityRef e;
        e.ID = v.id;
        e.ScriptName = sanitize(v.name);
        ref.Views.push_back(e);
    }

    // Global variables
    for (const auto& gv : gd.global_variables)
    {
        Variable var;
        var.Type = gv.type_name.c_str();
        var.Name = gv.name.c_str();
        var.Value = gv.default_value.c_str();
        ref.GlobalVars.push_back(var);
    }

    // Settings
    ref.Settings.SayFunction = gd.dialog_say_function.c_str();
    ref.Settings.NarrateFunction = gd.dialog_narrate_function.c_str();

    return ref;
}

bool BuildSystem::GenerateAutoHeader(Project& project, std::string& out_header)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    AGS::DataUtil::GameRef game_ref = BuildGameRef(*gd);
    AGS::DataUtil::String header = AGS::DataUtil::MakeGameAutoScriptHeader(game_ref);
    out_header = header.GetCStr();
    return true;
}

// -------------------------------------------------------------------------
// Read a text file into a string
// -------------------------------------------------------------------------
static bool ReadTextFile(const std::string& path, std::string& out)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;
    std::ostringstream ss;
    ss << file.rdbuf();
    out = ss.str();
    return true;
}

// -------------------------------------------------------------------------
// Configure a fresh preprocessor with all macros driven by game settings.
// Matches the C# editor's DefineMacrosAccordingToGameSettings().
// -------------------------------------------------------------------------
static void ConfigurePreprocessor(AGS::Preprocessor::Preprocessor& pp,
                                   const GameData& gd,
                                   const BuildConfig& config)
{
    pp.SetAppVersion("3.6.3");

    pp.DefineMacro("AGS_SUPPORTS_IFVER", "1");
    if (gd.enforce_new_strings)
        pp.DefineMacro("AGS_NEW_STRINGS", "1");

    // STRICT macro: The modern agsdefns.sh from our repo moves more functions
    // (GetGlobalInt, SetGlobalInt, etc.) behind #ifndef STRICT than older versions
    // did. Old projects (pre-3.4.0) were created when those functions were still
    // available with STRICT. To avoid false compile errors with old projects when
    // using the modern agsdefns.sh, only define STRICT for projects saved with
    // AGS 3.4.0 or later.
    {
        bool is_old_project = false;
        if (!gd.saved_editor_version.empty())
        {
            int major = 0, minor = 0, patch = 0;
            sscanf(gd.saved_editor_version.c_str(), "%d.%d.%d", &major, &minor, &patch);
            if (major < 3 || (major == 3 && minor < 4))
                is_old_project = true;
        }
        if (gd.enforce_object_scripting && !is_old_project)
            pp.DefineMacro("STRICT", "1");
    }

    if (gd.left_to_right_precedence)
        pp.DefineMacro("LRPRECEDENCE", "1");
    if (gd.enforce_new_strings)
        pp.DefineMacro("STRICT_STRINGS", "1");
    if (gd.enforce_new_audio)
        pp.DefineMacro("STRICT_AUDIO", "1");
    if (!gd.use_old_custom_dialog_api)
        pp.DefineMacro("NEW_DIALOGOPTS_API", "1");
    if (!gd.use_old_keyboard_handling)
        pp.DefineMacro("NEW_KEYINPUT_API", "1");

    if (config.debug_mode)
        pp.DefineMacro("DEBUG", "1");

    int api_ver = gd.script_api_version;
    for (int i = 0; i < kScriptAPIVersionCount; i++)
    {
        std::string macro = "SCRIPT_API_" + std::string(kScriptAPIVersions[i].name);
        pp.DefineMacro(macro.c_str(), "1");
        if (api_ver != kScriptAPIHighest &&
            kScriptAPIVersions[i].value >= api_ver)
            break;
    }

    int compat_ver = gd.script_compat_level;
    if (compat_ver == kScriptAPIHighest)
        compat_ver = kScriptAPIVersions[kScriptAPIVersionCount - 1].value;
    for (int i = 0; i < kScriptAPIVersionCount; i++)
    {
        if (kScriptAPIVersions[i].value >= compat_ver)
        {
            std::string macro = "SCRIPT_COMPAT_" + std::string(kScriptAPIVersions[i].name);
            pp.DefineMacro(macro.c_str(), "1");
        }
    }
}

// -------------------------------------------------------------------------
// Preprocess all accumulated raw headers + a script body using a FRESH
// preprocessor, then set up ccDefaultHeaders and compile.
// Matches the C# editor's per-module CompileScript() pattern:
//   1. Create fresh Preprocessor (clean macro state)
//   2. Preprocess each raw header in order
//   3. Preprocess the script body
//   4. ccRemoveDefaultHeaders() + ccAddDefaultHeader() for each header
//   5. ccCompileText() with the preprocessed body
// This avoids macro state leaking between modules (e.g. agsdefns.sh
// #define/#undef cycles causing "macro already defined" errors).
// -------------------------------------------------------------------------
struct RawHeader
{
    std::string text;
    std::string name;
};

static ccScript* PreprocessAndCompile(
    const GameData& gd, const BuildConfig& config,
    const std::vector<RawHeader>& raw_headers,
    const std::string& script_text, const std::string& script_name,
    BuildResult& result, const std::string& module_name,
    const std::string& temp_dir)
{
    // Fresh preprocessor per module — no macro state carried over
    AGS::Preprocessor::Preprocessor pp;
    ConfigurePreprocessor(pp, gd, config);

    // Preprocess all accumulated headers from scratch
    std::vector<AGS::Common::String> pp_headers;
    pp_headers.reserve(raw_headers.size());
    for (const auto& hdr : raw_headers)
    {
        AGS::Common::String pp_hdr = pp.Preprocess(
            hdr.text.c_str(), hdr.name.c_str());
        const auto& pp_err = pp.GetLastError();
        if (pp_err.Type != AGS::Preprocessor::ErrorCode::None)
        {
            result.AddError(hdr.name, 0,
                std::string("Preprocessor error in header: ") +
                pp_err.Message.GetCStr(), module_name);
            return nullptr;
        }
        pp_headers.push_back(pp_hdr);
    }

    // Preprocess script body
    AGS::Common::String pp_script = pp.Preprocess(
        script_text.c_str(), script_name.c_str());
    {
        const auto& pp_err = pp.GetLastError();
        if (pp_err.Type != AGS::Preprocessor::ErrorCode::None)
        {
            result.AddError(script_name, 0,
                std::string("Preprocessor error: ") +
                pp_err.Message.GetCStr(), module_name);
            return nullptr;
        }
    }

    // DEBUG: dump preprocessed script
    if (!temp_dir.empty())
    {
        std::string dump_path = temp_dir + "/" + module_name + "_pp_script.txt";
        std::ofstream dump(dump_path);
        if (dump.is_open())
            dump << pp_script.GetCStr();
    }

    // Set up native compiler headers: clear all prior, re-add preprocessed
    ccRemoveDefaultHeaders();
    for (size_t i = 0; i < pp_headers.size(); i++)
    {
        ccAddDefaultHeader(pp_headers[i].GetCStr(), raw_headers[i].name.c_str());
    }

    // Compile
    cc_clear_error();
    ccScript* compiled = ccCompileText(pp_script.GetCStr(), module_name.c_str());

    if (!compiled || cc_has_error())
    {
        const ScriptError& err = cc_get_error();
        std::string error_msg = err.HasError
            ? err.ErrorString.GetCStr()
            : "Unknown compilation error";
        int error_line = err.HasError ? err.Line : 0;

        result.AddError(script_name, error_line, error_msg, module_name);
        return nullptr;
    }

    return compiled;
}

// -------------------------------------------------------------------------
// Post-process converted dialog script to escape bare double quotes in
// character speech string arguments.
// The C# editor's DialogScriptConverter.ProcessCharacterSpeech does:
//   textToSay = textToSay.Replace("\"", "\\\"");
// The C++ converter in Tools/data/ has this commented out (TODO marker).
// Since we cannot modify files outside EditorImGui/, we apply the
// equivalent fix as a post-processing step on the converter output.
//
// The converter always formats speech as one of:
//   cChar.Say("speech text");          -- opening " right after (
//   cChar.SayAt(x, y, w, "text");     -- opening " after last ,<space>
//   Display("speech text");            -- opening " right after (
// We find the opening " (first " after '('), the closing " (last before ");"),
// and escape all bare " between them.
// -------------------------------------------------------------------------
static std::string FixDialogSpeechQuotes(const std::string& script)
{
    std::string result;
    result.reserve(script.size() + 64);
    size_t pos = 0;
    while (pos < script.size())
    {
        size_t eol = script.find('\n', pos);
        if (eol == std::string::npos)
            eol = script.size();

        size_t len = eol - pos;

        // Line must end with ");
        if (len >= 4
            && script[pos + len - 1] == ';'
            && script[pos + len - 2] == ')')
        {
            // Find closing " — the last " before ");"
            // It may be preceded by \" (escaped quote in text), so we need
            // the " that is NOT preceded by backslash, scanning from right.
            size_t closing = std::string::npos;
            for (size_t i = pos + len - 3; i > pos; --i)
            {
                if (script[i] == '"' && (i == 0 || script[i - 1] != '\\'))
                {
                    closing = i;
                    break;
                }
            }

            // Find opening " — search forward for first ("
            size_t opening = std::string::npos;
            if (closing != std::string::npos)
            {
                for (size_t i = pos; i < closing; ++i)
                {
                    if (script[i] == '(' && i + 1 < closing && script[i + 1] == '"')
                    {
                        opening = i + 1;
                        break;
                    }
                }
                // For SayAt(x, y, w, "text") — opening may be after ", "
                if (opening == std::string::npos)
                {
                    for (size_t i = pos; i < closing; ++i)
                    {
                        if (script[i] == ',' && i + 2 < closing
                            && script[i + 1] == ' ' && script[i + 2] == '"')
                        {
                            opening = i + 2;
                            // Don't break — take the LAST ", " before closing
                        }
                    }
                }
            }

            if (opening != std::string::npos && closing != std::string::npos
                && opening < closing)
            {
                // Check if there are bare quotes between delimiters
                bool needs_fix = false;
                for (size_t i = opening + 1; i < closing; ++i)
                {
                    if (script[i] == '"' && script[i - 1] != '\\')
                    {
                        needs_fix = true;
                        break;
                    }
                }

                if (needs_fix)
                {
                    // Copy everything up to and including opening "
                    result.append(script, pos, opening + 1 - pos);
                    // Escape bare quotes in the text content
                    for (size_t i = opening + 1; i < closing; ++i)
                    {
                        if (script[i] == '"' && script[i - 1] != '\\')
                        {
                            result += '\\';
                            result += '"';
                        }
                        else
                        {
                            result += script[i];
                        }
                    }
                    // Copy from closing " to end of line
                    result.append(script, closing, eol - closing);
                    if (eol < script.size())
                        result += '\n';
                    pos = eol + 1;
                    continue;
                }
            }
        }

        result.append(script, pos, eol - pos);
        if (eol < script.size())
            result += '\n';
        pos = eol + 1;
    }

    return result;
}

// -------------------------------------------------------------------------
// Fix import/export param-count mismatches across compiled scripts.
//
// Some AGS game projects have import declarations in headers that list
// fewer parameters than the real function definition in the implementing
// module (e.g.  import function options_Click();  vs.  function
// options_Click(GUIControl*, MouseButton)).
//
// The compiler stores imports with  name^argcount  (using '^') and exports
// with  name$argcount  (using '$').  At link-time the engine's
// ScriptSymbolsMap::GetIndexOfAny would need a fuzzy-match fallback to
// tolerate the discrepancy; rather than patching the engine we fix up the
// compiled .o files here so the param counts are consistent.
//
// Algorithm
//   1. Read every compiled .o, collect ALL exports  (base_name -> argcount).
//   2. For each .o, scan its imports: if  base^N  has no matching export
//      base$N  but  base$M  exists, rewrite the import to  base^M.
//   3. Rewrite only the .o files that were modified.
// -------------------------------------------------------------------------
static void FixImportExportMismatches(
    const std::vector<CompiledScriptInfo>& compiled_scripts)
{
    // Step 1 — collect every export's base name → full export string
    //   Export format: "name$argcount"
    //   We also build a set of all full export names for quick exact-match.
    std::unordered_map<std::string, std::string> export_base_to_full;
    std::unordered_set<std::string> all_exports_as_imports; // with '^' separator

    for (const auto& cs : compiled_scripts)
    {
        if (cs.is_header) continue;
        auto stream = AGS::Common::File::OpenFileRead(cs.obj_file.c_str());
        if (!stream) continue;

        ccScript script;
        if (!script.Read(stream.get())) continue;

        for (const auto& exp : script.exports)
        {
            size_t dollar = exp.find('$');
            if (dollar != std::string::npos)
            {
                std::string base = exp.substr(0, dollar);
                std::string argcount = exp.substr(dollar + 1);
                export_base_to_full[base] = exp;
                // Store as import-style (^) for quick lookup
                all_exports_as_imports.insert(base + "^" + argcount);
            }
        }
    }

    // Step 2 — patch each script's imports where needed
    for (const auto& cs : compiled_scripts)
    {
        if (cs.is_header) continue;
        auto in_stream = AGS::Common::File::OpenFileRead(cs.obj_file.c_str());
        if (!in_stream) continue;

        ccScript script;
        if (!script.Read(in_stream.get())) continue;
        in_stream.reset();

        bool modified = false;
        for (auto& imp : script.imports)
        {
            if (imp.empty()) continue;

            size_t caret = imp.find('^');
            if (caret == std::string::npos) continue;

            // Already resolvable? skip
            if (all_exports_as_imports.count(imp)) continue;

            // Look for an export with the same base name
            std::string base = imp.substr(0, caret);
            auto it = export_base_to_full.find(base);
            if (it == export_base_to_full.end()) continue;

            // Found export with different param count — fix the import
            // Convert export "name$M" → import "name^M"
            size_t dollar = it->second.find('$');
            if (dollar == std::string::npos) continue;
            std::string fixed_import = base + "^" + it->second.substr(dollar + 1);

            AGSEditor::Logger::Log("[Build] Fixing import mismatch in '%s': %s -> %s",
                cs.name.c_str(), imp.c_str(), fixed_import.c_str());
            imp = fixed_import;
            modified = true;
        }

        if (modified)
        {
            auto out_stream = AGS::Common::File::CreateFile(cs.obj_file.c_str());
            if (out_stream)
            {
                script.Write(out_stream.get());
            }
        }
    }
}

// -------------------------------------------------------------------------
// Full script compilation pipeline
// Mirrors the C# editor's AGSEditor.CompileScripts()
// Order: auto-header → internal headers → script module headers+bodies → dialog scripts
//
// Key design (matching C# editor):
//   - Raw (unpreprocessed) header texts accumulate as modules are processed
//   - For each module, a FRESH preprocessor re-processes all accumulated
//     headers + the module body. This prevents macro state leaking between
//     modules and avoids the "macro already defined" error.
//   - ccRemoveDefaultHeaders() + ccAddDefaultHeader() are called per module
// -------------------------------------------------------------------------
bool BuildSystem::CompileAllScripts(Project& project, const BuildConfig& config,
                                     BuildResult& result)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    compiled_scripts_.clear();
    std::string temp_dir = GetCompiledDir(config) + "/_temp";

    // --- Setup compiler options (driven by game settings) ---
    ccSetSoftwareVersion("3.6.3");
    ccSetOption(SCOPT_EXPORTALL, 1);
    ccSetOption(SCOPT_LINENUMBERS, config.line_numbers ? 1 : 0);
    ccSetOption(SCOPT_LEFTTORIGHT, gd->left_to_right_precedence ? 1 : 0);
    ccSetOption(SCOPT_OLDSTRINGS, gd->enforce_new_strings ? 0 : 1);

    // Accumulate raw (unpreprocessed) header texts. Each module compilation
    // creates a fresh Preprocessor and re-preprocesses ALL headers from scratch.
    // This matches the C# editor pattern and avoids macro state leaking.
    std::vector<RawHeader> raw_headers;

    // --- 0. Load built-in header (agsdefns.sh) - store raw text ---
    {
        std::string builtin_header;
        std::vector<std::string> search_paths = {
#ifdef SOURCE_DIR
            std::string(SOURCE_DIR) + "/../Editor/AGS.Editor/Resources/agsdefns.sh",
#endif
            "../Editor/AGS.Editor/Resources/agsdefns.sh",
            "Editor/AGS.Editor/Resources/agsdefns.sh",
            "/usr/share/ags/agsdefns.sh",
        };
        for (const auto& path : search_paths)
        {
            std::ifstream file(path);
            if (file.is_open())
            {
                std::ostringstream ss;
                ss << file.rdbuf();
                builtin_header = ss.str();
                Log("[Build] Loaded built-in header: %s (%d bytes)",
                    path.c_str(), (int)builtin_header.size());
                break;
            }
        }
        if (builtin_header.empty())
        {
            result.AddError("", 0,
                "Could not find agsdefns.sh (built-in script header). "
                "Scripts using AGS API types will fail to compile.");
            return false;
        }
        raw_headers.push_back({builtin_header, "_BuiltInScriptHeader"});
    }

    // --- Generate auto-header ---
    std::string auto_header;
    if (!GenerateAutoHeader(project, auto_header))
    {
        result.AddError("", 0, "Failed to generate auto-header.");
        return false;
    }

    Log("[Build] Generated _AutoGenerated.ash (%d bytes)", (int)auto_header.size());

    // Save auto-header for debugging
    {
        std::string auto_hdr_path = temp_dir + "/_AutoGenerated.ash";
        std::ofstream out(auto_hdr_path);
        if (out.is_open()) {
            out << auto_header;
            out.close();
        }
    }

    // --- Accumulate raw headers and compile each module ---
    // Raw auto-generated header
    raw_headers.push_back({auto_header, "_AutoGenerated"});

    // Step 1b: Generate and compile _GlobalVariables module
    if (!gd->global_variables.empty())
    {
        // Generate header (_GlobalVariables.ash) -- import declarations
        std::string gv_header;
        for (const auto& gv : gd->global_variables)
        {
            gv_header += "import ";
            gv_header += gv.type_name;
            gv_header += " ";
            gv_header += gv.name;
            gv_header += ";\n";
        }

        // Accumulate raw header
        raw_headers.push_back({gv_header, "_GlobalVariables"});

        // Generate body (_GlobalVariables.asc) -- declarations + exports + game_start()
        std::string gv_body;
        std::string game_start_init; // String initializations go in game_start()

        for (const auto& gv : gd->global_variables)
        {
            // Declaration
            gv_body += gv.type_name;
            gv_body += " ";
            gv_body += gv.name;

            // Default value (not for String -- those go in game_start)
            if (!gv.default_value.empty() && gv.type_name != "String")
            {
                gv_body += " = ";
                // bool values should be lowercase
                if (gv.type_name == "bool")
                {
                    std::string val = gv.default_value;
                    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
                    gv_body += val;
                }
                else
                {
                    gv_body += gv.default_value;
                }
            }
            gv_body += ";\n";

            // Export
            gv_body += "export ";
            gv_body += gv.name;
            gv_body += ";\n";

            // String initialization goes in game_start()
            if (gv.type_name == "String" && !gv.default_value.empty())
            {
                // Escape the string value
                std::string escaped = gv.default_value;
                std::string result_str;
                for (char ch : escaped)
                {
                    if (ch == '\\') result_str += "\\\\";
                    else if (ch == '"') result_str += "\\\"";
                    else result_str += ch;
                }
                game_start_init += gv.name;
                game_start_init += " = \"";
                game_start_init += result_str;
                game_start_init += "\";\n";
            }
        }

        // Add game_start() if any String initializations needed
        if (!game_start_init.empty())
        {
            gv_body += "function game_start() {\n";
            gv_body += game_start_init;
            gv_body += "}\n";
        }

        // Compile _GlobalVariables using fresh-preprocessor pattern
        ccScript* gv_compiled = PreprocessAndCompile(
            *gd, config, raw_headers, gv_body, "_GlobalVariables.asc",
            result, "_GlobalVariables", temp_dir);

        if (!gv_compiled)
            return false;

        // Write compiled script to temp file
        std::string gv_obj_path = temp_dir + "/_GlobalVariables.o";
        {
            std::unique_ptr<AGS::Common::Stream> out(
                AGS::Common::File::CreateFile(gv_obj_path.c_str()));
            if (out)
            {
                gv_compiled->Write(out.get());
            }
            else
            {
                result.AddError("", 0, "Failed to write compiled script: " + gv_obj_path);
                delete gv_compiled;
                return false;
            }
        }

        compiled_scripts_.push_back({"_GlobalVariables", gv_obj_path, false});
        delete gv_compiled;

        Log("[Build] _GlobalVariables compiled (%d variables).", (int)gd->global_variables.size());
    }

    // Step 2: Compile each script module
    int module_count = (int)gd->script_modules.size();
    int compiled_count = 0;

    for (int i = 0; i < module_count; i++)
    {
        const auto& mod = gd->script_modules[i];
        progress_.Advance("Compiling: " + mod.name);
        Log("[Build] Compiling module: %s", mod.name.c_str());

        // Read header file (if exists) and accumulate raw text
        // (matching C# pattern: headers.Add(scripts.Header) before CompileScript)
        if (!mod.header_file.empty())
        {
            std::string header_path = FindFileCaseInsensitive(
                project.GetProjectDir(), mod.header_file);
            if (FileExists(header_path))
            {
                std::string header_content;
                ReadTextFile(header_path, header_content);
                if (!header_content.empty())
                {
                    std::string hdr_name = mod.name + "_header";
                    raw_headers.push_back({header_content, hdr_name});
                }
            }
            else
            {
                Log("[Build]   Warning: header file not found: %s", header_path.c_str());
            }
        }

        // Read script body (case-insensitive lookup for Windows projects on Linux)
        std::string script_content;
        if (!mod.script_file.empty())
        {
            std::string script_path = FindFileCaseInsensitive(
                project.GetProjectDir(), mod.script_file);
            if (FileExists(script_path))
            {
                ReadTextFile(script_path, script_content);
            }
            else
            {
                Log("[Build]   Error: script file not found: %s", script_path.c_str());
                result.AddError(mod.script_file, 0,
                    "Script file not found: " + script_path, mod.name);
                return false;
            }
        }

        if (script_content.empty())
        {
            Log("[Build]   Skipping module '%s' (no script body)", mod.name.c_str());
            continue;
        }

        // Compile with fresh preprocessor (re-processes all headers from scratch)
        ccScript* compiled = PreprocessAndCompile(
            *gd, config, raw_headers, script_content, mod.script_file,
            result, mod.name, temp_dir);

        if (!compiled)
            return false;

        // Write compiled script object to temp file
        std::string obj_path = temp_dir + "/" + mod.name + ".o";
        {
            std::unique_ptr<AGS::Common::Stream> out(
                AGS::Common::File::CreateFile(obj_path.c_str()));
            if (out)
            {
                compiled->Write(out.get());
            }
            else
            {
                result.AddError("", 0, "Failed to write compiled script: " + obj_path);
                delete compiled;
                return false;
            }
        }

        compiled_scripts_.push_back({mod.name, obj_path, false});
        compiled_count++;
        delete compiled;

        Log("[Build]   %s compiled successfully.", mod.name.c_str());
    }

    Log("[Build] Script compilation complete: %d/%d modules compiled.",
        compiled_count, module_count);

    // --- Step 3: Compile dialog scripts ---
    // Dialog scripts use a special dialect that gets converted to regular
    // AGS script by DialogScriptConverter. All dialogs are merged into a
    // single "__DialogScripts" compilation unit, matching the C# editor.
    // Even with zero dialogs, we must produce an empty dialog script.
    {
        progress_.Advance("Compiling dialog scripts");
        Log("[Build] Compiling dialog scripts (%d dialogs)...",
            (int)gd->dialogs.size());

        // Build GameRef for the converter
        AGS::DataUtil::GameRef game_ref;
        game_ref.Settings.SayFunction =
            AGS::Common::String(gd->dialog_say_function.c_str());
        game_ref.Settings.NarrateFunction =
            AGS::Common::String(gd->dialog_narrate_function.c_str());

        for (const auto& ch : gd->characters)
        {
            AGS::DataUtil::CharacterRef cr;
            cr.ID = ch.id;
            cr.ScriptName = AGS::Common::String(ch.script_name.c_str());
            game_ref.Characters.push_back(cr);
        }

        // Populate GameRef.Dialogs so goto-dialog can look up by name
        for (const auto& d : gd->dialogs)
        {
            AGS::DataUtil::DialogRef dr;
            dr.ID = d.id;
            dr.ScriptName = AGS::Common::String(d.name.c_str());
            dr.OptionCount = d.option_count;
            game_ref.Dialogs.push_back(dr);
        }

        // Start with the default dialog script preamble
        std::string dialog_body = AGS::DataUtil::DialogScriptDefault;

        bool has_errors = false;
        for (const auto& d : gd->dialogs)
        {
            AGS::DataUtil::DialogRef dlg_ref;
            dlg_ref.ID = d.id;
            dlg_ref.ScriptName = AGS::Common::String(d.name.c_str());
            dlg_ref.OptionCount = d.option_count;

            AGS::DataUtil::DialogScriptConverter conv(
                AGS::Common::String(d.script.c_str()), game_ref, dlg_ref);
            AGS::Common::String converted = conv.Convert();

            // Check for conversion errors
            for (const auto& e : conv.GetErrors())
            {
                if (e.Error)
                {
                    std::string msg = "Dialog " + std::to_string(d.id) +
                        " (" + d.name + "): " + e.Message.GetCStr();
                    result.AddError("__DialogScripts", (int)e.LineNumber, msg);
                    Log("[Error] %s", msg.c_str());
                    has_errors = true;
                }
            }

            // Append the new-script marker + converted script
            dialog_body += AGS::DataUtil::NEW_SCRIPT_MARKER;
            dialog_body += "Dialog ";
            dialog_body += std::to_string(d.id);
            dialog_body += "\"\n";
            dialog_body += FixDialogSpeechQuotes(converted.GetCStr());
        }

        if (has_errors)
            return false;

        // DEBUG: dump merged dialog script
        {
            std::string dump_path = temp_dir + "/__DialogScripts_merged.txt";
            std::ofstream dump(dump_path);
            if (dump.is_open()) {
                dump << dialog_body;
                dump.close();
            }
        }

        // Compile dialog scripts with fresh preprocessor
        ccScript* dlg_compiled = PreprocessAndCompile(
            *gd, config, raw_headers, dialog_body, "__DialogScripts",
            result, "__DialogScripts", temp_dir);

        if (!dlg_compiled)
            return false;

        // Write compiled dialog script to temp file
        std::string dlg_obj_path = temp_dir + "/__DialogScripts.o";
        {
            std::unique_ptr<AGS::Common::Stream> out(
                AGS::Common::File::CreateFile(dlg_obj_path.c_str()));
            if (out)
            {
                dlg_compiled->Write(out.get());
            }
            else
            {
                result.AddError("", 0,
                    "Failed to write compiled dialog script: " + dlg_obj_path);
                delete dlg_compiled;
                return false;
            }
        }

        compiled_scripts_.push_back(
            {"__DialogScripts", dlg_obj_path, false});
        delete dlg_compiled;

        Log("[Build] Dialog scripts compiled successfully.");
    }

    // --- Step 4: Fix import/export param-count mismatches ---
    // Some games have buggy import declarations (e.g. 0-param import for a
    // 2-param function). The engine's fuzzy-match would accept them at
    // link-time, but we can fix them here so the vanilla engine works.
    FixImportExportMismatches(compiled_scripts_);

    return true;
}

// -------------------------------------------------------------------------
// Game data file generation (ac2game.dta)
// Delegates to the game_data_writer module which produces the exact binary
// format that the AGS engine expects.
// -------------------------------------------------------------------------
bool BuildSystem::WriteGameDataFile(Project& project, const BuildConfig& config,
                                     BuildResult& result, const std::string& output_path)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    progress_.Advance("Writing game data file");
    Log("[Build] Writing game data: %s", output_path.c_str());

    // Convert compiled script info to the writer's format.
    // The DTA format requires scripts in this exact order:
    //   [0]  = GlobalScript
    //   [1]  = __DialogScripts
    //   [2+] = script modules (_GlobalVariables first, then user modules)
    // This matches the C# editor's DataFileWriter.WriteCompiledScript order.
    std::vector<AGSEditor::CompiledScriptRef> scripts;

    // Classify compiled scripts into their DTA slots
    const CompiledScriptInfo* global_script = nullptr;
    const CompiledScriptInfo* dialog_script = nullptr;
    const CompiledScriptInfo* global_vars = nullptr;
    std::vector<const CompiledScriptInfo*> modules;

    for (const auto& cs : compiled_scripts_) {
        if (cs.is_header) continue;
        if (cs.name == "GlobalScript")
            global_script = &cs;
        else if (cs.name == "__DialogScripts")
            dialog_script = &cs;
        else if (cs.name == "_GlobalVariables")
            global_vars = &cs;
        else
            modules.push_back(&cs);
    }

    if (global_script)
        scripts.push_back({global_script->name, global_script->obj_file});
    if (dialog_script)
        scripts.push_back({dialog_script->name, dialog_script->obj_file});
    // _GlobalVariables is a script module that must come first (before user modules)
    if (global_vars)
        scripts.push_back({global_vars->name, global_vars->obj_file});
    for (const auto* m : modules)
        scripts.push_back({m->name, m->obj_file});

    if (!AGSEditor::WriteGameDataFile(*gd, scripts, output_path, result)) {
        return false;
    }

    Log("[Build] Game data written successfully.");
    result.output_files.push_back(output_path);
    return true;
}

// -------------------------------------------------------------------------
// Write a CLIB/MFL archive from a list of (name, source_path) pairs.
// Similar to AGS::DataUtil::WriteLibraryFile() but supports assets
// sourced from different directories.
// -------------------------------------------------------------------------
static AGS::Common::HError WriteMFLFromEntries(
    const std::string &output_path,
    const std::vector<std::pair<std::string, std::string>> &entries)
{
    using namespace AGS::Common;

    auto out = std::unique_ptr<Stream>(File::CreateFile(output_path.c_str()));
    if (!out)
        return new Error("Failed to open pack file for writing.");

    AssetLibInfo lib;
    lib.LibFileNames.push_back(Path::GetFilename(output_path.c_str()));

    for (const auto &e : entries)
    {
        AssetInfo ai;
        ai.FileName = e.first.c_str();
        ai.LibUid = 0;
        ai.Size = File::GetFileSize(String(e.second.c_str()));
        lib.AssetInfos.push_back(ai);
    }

    soff_t s_offset = out->GetPosition();
    MFLUtil::WriteHeader(lib, MFLUtil::kMFLVersion_MultiV30, 0, out.get());

    for (size_t i = 0; i < entries.size(); i++)
    {
        lib.AssetInfos[i].Offset = out->GetPosition() - s_offset;
        auto in = std::unique_ptr<Stream>(
            File::OpenFileRead(String(entries[i].second.c_str())));
        if (!in)
            return new Error(String::FromFormat(
                "Failed to open asset '%s' for reading.", entries[i].first.c_str()));
        if (CopyStream(in.get(), out.get(), lib.AssetInfos[i].Size) < lib.AssetInfos[i].Size)
            return new Error(String::FromFormat(
                "Failed to write asset '%s'.", entries[i].first.c_str()));
    }

    out->Seek(s_offset, kSeekBegin);
    MFLUtil::WriteHeader(lib, MFLUtil::kMFLVersion_MultiV30, 0, out.get());
    out->Seek(0, kSeekEnd);
    MFLUtil::WriteEnder(s_offset, MFLUtil::kMFLVersion_MultiV30, out.get());
    return HError::None();
}

// -------------------------------------------------------------------------
// Asset packaging (game.ags)
// Creates a CLIB-format package containing all game assets.
// -------------------------------------------------------------------------
bool BuildSystem::PackageAssets(Project& project, const BuildConfig& config,
                                 BuildResult& result, const std::string& data_dir)
{
    progress_.Advance("Packaging assets");
    Log("[Build] Packaging assets...");

    std::string pak_path = data_dir + "/game.ags";

    // Collect assets to package: (name_in_archive, source_path_on_disk)
    using AssetEntry = std::pair<std::string, std::string>;
    std::vector<AssetEntry> assets;

    // 1. Game data file (ac2game.dta)
    std::string dta_path = GetCompiledDir(config) + "/_temp/ac2game.dta";
    if (FileExists(dta_path))
        assets.push_back({"ac2game.dta", dta_path});

    // 2. Sprite file (acsprset.spr) — copy from project dir
    std::string sprite_path = project.GetProjectDir() + "/acsprset.spr";
    if (FileExists(sprite_path))
        assets.push_back({"acsprset.spr", sprite_path});

    // 3. Sprite index (sprindex.dat)
    std::string spridx_path = project.GetProjectDir() + "/sprindex.dat";
    if (FileExists(spridx_path))
        assets.push_back({"sprindex.dat", spridx_path});

    // 4. Room files (room*.crm)
    GameData* gd = project.GetGameData();
    if (gd)
    {
        for (const auto& room : gd->rooms)
        {
            char room_name[64];
            snprintf(room_name, sizeof(room_name), "room%d.crm", room.number);
            std::string room_path = project.GetProjectDir() + "/" + room_name;
            if (FileExists(room_path))
                assets.push_back({room_name, room_path});
        }
    }

    // 5. Font files (agsfntN.ttf or agsfntN.wfn) — look for project files,
    //    not source files (SourceFilename is just import metadata)
    if (gd)
    {
        for (const auto& font : gd->fonts)
        {
            char ttf_name[64], wfn_name[64];
            snprintf(ttf_name, sizeof(ttf_name), "agsfnt%d.ttf", font.id);
            snprintf(wfn_name, sizeof(wfn_name), "agsfnt%d.wfn", font.id);

            std::string ttf_path = project.GetProjectDir() + "/" + ttf_name;
            std::string wfn_path = project.GetProjectDir() + "/" + wfn_name;

            if (FileExists(ttf_path))
                assets.push_back({ttf_name, ttf_path});
            else if (FileExists(wfn_path))
                assets.push_back({wfn_name, wfn_path});
            else
            {
                Log("[Build] Warning: font file not found for font %d (%s / %s)",
                    font.id, ttf_name, wfn_name);
            }
        }
    }

    // 6. Audio clips (from AudioClips folder) — only InGameEXE bundling type
    if (gd)
    {
        for (const auto& clip : gd->audio_clips)
        {
            if (!clip.filename.empty() && clip.bundling_type == 0) // 0=InGameEXE
            {
                std::string clip_path = project.GetProjectDir() + "/" + clip.filename;
                if (FileExists(clip_path))
                    assets.push_back({clip.filename, clip_path});
            }
        }
    }

    // 7. Translation files (.trs -> .tra compilation)
    {
        std::string proj_dir = project.GetProjectDir();
        std::string temp_dir = GetCompiledDir(config) + "/_temp";
        std::error_code ec;
        for (const auto& dir_entry : std::filesystem::directory_iterator(proj_dir, ec)) {
            if (!dir_entry.is_regular_file()) continue;
            std::string trs_name = dir_entry.path().filename().string();
            {
                if (trs_name.size() < 4) continue;
                std::string ext = trs_name.substr(trs_name.size() - 4);
                for (auto& c : ext) c = (char)tolower(c);
                if (ext != ".trs") continue;
                std::string trs_path = proj_dir + "/" + trs_name;
                std::string stem = trs_name.substr(0, trs_name.size() - 4);
                std::string tra_name = stem + ".tra";
                std::string tra_path = temp_dir + "/" + tra_name;

                // Parse .trs using shared ReadTRS and compile to .tra using WriteTRA
                auto trs_in = std::unique_ptr<AGS::Common::Stream>(
                    AGS::Common::File::OpenFileRead(AGS::Common::String(trs_path.c_str())));
                if (trs_in)
                {
                    AGS::Common::Translation tra;
                    tra.GameUid = 0;
                    if (gd)
                        tra.GameName = AGS::Common::String(gd->game_title.c_str());

                    AGS::Common::HError err = AGS::DataUtil::ReadTRS(tra, std::move(trs_in));
                    if (err)
                    {
                        auto tra_out = std::unique_ptr<AGS::Common::Stream>(
                            AGS::Common::File::CreateFile(AGS::Common::String(tra_path.c_str())));
                        if (tra_out)
                        {
                            AGS::DataUtil::WriteTRA(tra, std::move(tra_out));
                            assets.push_back({tra_name, tra_path});
                            Log("[Build] Compiled translation: %s -> %s (%d entries)",
                                trs_name.c_str(), tra_name.c_str(), (int)tra.Dict.size());
                        }
                    }
                    else
                    {
                        Log("[Build] Warning: failed to parse %s: %s",
                            trs_name.c_str(), err->FullMessage().GetCStr());
                    }
                }
            }
        }
    }

    if (assets.empty())
    {
        result.AddWarning("", 0, "No assets found to package.");
    }

    // Write the CLIB/MFL archive
    auto mfl_err = WriteMFLFromEntries(pak_path, assets);
    if (!mfl_err)
    {
        result.AddError("", 0, "Failed to write package: " + std::string(mfl_err->FullMessage().GetCStr()));
        return false;
    }

    Log("[Build] Package created: %s (%d assets)", pak_path.c_str(), (int)assets.size());
    result.output_files.push_back(pak_path);
    return true;
}

// -------------------------------------------------------------------------
// Helper: write a CLIB VOX file from a list of asset entries
// -------------------------------------------------------------------------
static bool WriteVOXFile(const std::string& vox_path,
                         const std::vector<std::pair<std::string, std::string>>& entries)
{
    if (entries.empty())
        return true;

    auto err = WriteMFLFromEntries(vox_path, entries);
    return static_cast<bool>(err);
}

// Gather files matching extensions in a directory (non-recursive)
static void CollectFilesByExtension(const std::string& dir,
                                    const std::vector<std::string>& extensions,
                                    std::vector<std::string>& out_files)
{
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
    {
        if (!entry.is_regular_file()) continue;
        std::string fname = entry.path().filename().string();
        size_t dot = fname.rfind('.');
        if (dot == std::string::npos) continue;
        std::string ext = fname.substr(dot);
        for (auto& c : ext) c = (char)tolower(c);
        for (const auto& wanted : extensions)
        {
            if (ext == wanted)
            {
                out_files.push_back(dir + "/" + fname);
                break;
            }
        }
    }
}

// -------------------------------------------------------------------------
// Create audio.vox and speech.vox files
// -------------------------------------------------------------------------
bool BuildSystem::CreateVOXFiles(Project& project, const BuildConfig& config,
                                  BuildResult& result, const std::string& data_dir)
{
    progress_.Advance("Creating VOX files");
    Log("[Build] Creating VOX files...");

    GameData* gd = project.GetGameData();

    // --- audio.vox ---
    // Collect audio clips with bundling_type == 1 (InSeparateVOX)
    {
        std::vector<std::pair<std::string, std::string>> vox_entries; // {asset_name, source_path}
        if (gd)
        {
            for (const auto& clip : gd->audio_clips)
            {
                if (clip.bundling_type == 1 && !clip.filename.empty()) // InSeparateVOX
                {
                    std::string clip_path = project.GetProjectDir() + "/" + clip.filename;
                    if (FileExists(clip_path))
                    {
                        vox_entries.push_back({clip.filename, clip_path});
                    }
                }
            }
        }

        std::string audio_vox = data_dir + "/audio.vox";
        if (!vox_entries.empty())
        {
            if (WriteVOXFile(audio_vox, vox_entries))
            {
                Log("[Build] Created audio.vox with %d clips.", (int)vox_entries.size());
                result.output_files.push_back(audio_vox);
            }
            else
            {
                result.AddWarning("", 0, "Failed to create audio.vox");
            }
        }
        else
        {
            // Remove stale audio.vox if no clips need separate VOX
            if (FileExists(audio_vox))
                std::remove(audio_vox.c_str());
        }
    }

    // --- speech.vox ---
    // Collect speech files from the Speech/ directory
    {
        std::string speech_dir = project.GetProjectDir() + "/Speech";
        std::vector<std::string> speech_extensions = {".mp3", ".ogg", ".wav", ".dat"};

        auto buildVox = [&](const std::string& src_dir, const std::string& vox_name) {
            std::vector<std::string> files;
            CollectFilesByExtension(src_dir, speech_extensions, files);

            // Also include syncdata.dat if present
            std::string sync_path = src_dir + "/syncdata.dat";
            bool sync_already = false;
            for (const auto& f : files)
            {
                if (f == sync_path) { sync_already = true; break; }
            }
            // syncdata.dat matches .dat, so it's already included — but keep the check
            (void)sync_already;

            if (files.empty()) return;

            std::vector<std::pair<std::string, std::string>> vox_entries;
            for (const auto& f : files)
            {
                // Asset name = path relative to speech_dir (just filename for top-level)
                std::string relative;
                if (f.size() > src_dir.size() + 1)
                    relative = f.substr(src_dir.size() + 1);
                else
                    relative = f;
                vox_entries.push_back({relative, f});
            }

            std::string vox_path = data_dir + "/" + vox_name;
            if (WriteVOXFile(vox_path, vox_entries))
            {
                Log("[Build] Created %s with %d files.", vox_name.c_str(), (int)vox_entries.size());
                result.output_files.push_back(vox_path);
            }
            else
            {
                result.AddWarning("", 0, "Failed to create " + vox_name);
            }
        };

        if (FileExists(speech_dir))
        {
            buildVox(speech_dir, "speech.vox");

            // Create sp_*.vox for each subdirectory (multilingual support)
            std::error_code ec;
            for (const auto& sub_entry : std::filesystem::directory_iterator(speech_dir, ec))
            {
                if (!sub_entry.is_directory()) continue;
                std::string subdir_name = sub_entry.path().filename().string();
                std::string subdir_path = speech_dir + "/" + subdir_name;
                // Lowercase the folder name for the VOX filename
                std::string lower_name = subdir_name;
                for (auto& c : lower_name) c = (char)tolower(c);
                buildVox(subdir_path, "sp_" + lower_name + ".vox");
            }
        }
        else
        {
            // Remove stale speech.vox if Speech/ directory doesn't exist
            std::string speech_vox = data_dir + "/speech.vox";
            if (FileExists(speech_vox))
                std::remove(speech_vox.c_str());
        }
    }

    return true;
}

// -------------------------------------------------------------------------
// Copy engine files for a specific platform
// -------------------------------------------------------------------------
bool BuildSystem::CopyEngineFiles(const BuildConfig& config, BuildTarget target,
                                   BuildResult& result, const std::string& target_dir)
{
    progress_.Advance("Copying engine files for " + std::string(BuildTargetName(target)));
    Log("[Build] Copying engine files for target: %s", BuildTargetName(target));

    // Copy data files to target directory
    std::string data_dir = GetDataDir(config);

    // Copy game.ags
    std::string pak_src = data_dir + "/game.ags";
    std::string pak_dst = target_dir + "/game.ags";
    if (FileExists(pak_src))
    {
        if (!CopyFile_(pak_src, pak_dst))
        {
            result.AddWarning("", 0, "Failed to copy game.ags to " + target_dir);
        }
    }

    // Copy VOX files (audio.vox, speech.vox, sp_*.vox)
    auto copyVoxFiles = [&](const std::string& dst_dir) {
        // Copy specific named VOX files
        for (const auto& vox_name : {"audio.vox", "speech.vox"})
        {
            std::string vox_src = data_dir + "/" + vox_name;
            if (FileExists(vox_src))
            {
                CopyFile_(vox_src, dst_dir + "/" + vox_name);
                Log("[Build]   Copied %s", vox_name);
            }
        }
        // Copy sp_*.vox multilingual speech files
        {
            std::error_code ec;
            for (const auto& entry : std::filesystem::directory_iterator(data_dir, ec))
            {
                if (!entry.is_regular_file()) continue;
                std::string fn = entry.path().filename().string();
                if (fn.size() > 7 && fn.substr(0, 3) == "sp_" &&
                    fn.substr(fn.size() - 4) == ".vox")
                {
                    CopyFile_(data_dir + "/" + fn, dst_dir + "/" + fn);
                    Log("[Build]   Copied %s", fn.c_str());
                }
            }
        }
    };

    // Copy VOX files to the same directory as game.ags
    copyVoxFiles(target_dir);

    switch (target)
    {
    case BuildTarget::Linux:
    {
        // Copy the AGS engine binary
        std::string engine = FindEngineExecutable(config);
        if (!engine.empty())
        {
            std::string dst = target_dir + "/ags";
            CopyFile_(engine, dst);
            // Make executable
#ifndef _WIN32
            std::filesystem::permissions(dst, std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
#endif
            Log("[Build]   Copied engine: %s", engine.c_str());
        }
        else
        {
            result.AddWarning("", 0, "Linux engine binary not found. Game data packaged but no engine copied.");
        }

        // Create launcher script
        std::string launcher = target_dir + "/run.sh";
        std::ofstream sh(launcher);
        if (sh.is_open())
        {
            sh << "#!/bin/bash\n";
            sh << "# AGS Game Launcher\n";
            sh << "DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"\n";
            sh << "cd \"$DIR\"\n";
            sh << "exec ./ags game.ags \"$@\"\n";
            sh.close();
#ifndef _WIN32
            std::filesystem::permissions(launcher, std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
#endif
        }
        break;
    }

    case BuildTarget::Windows:
    {
        if (!config.engine_windows_path.empty() && FileExists(config.engine_windows_path))
        {
            std::string dst = target_dir + "/game.exe";
            CopyFile_(config.engine_windows_path, dst);
            Log("[Build]   Copied Windows engine: %s", config.engine_windows_path.c_str());
        }
        else
        {
            result.AddWarning("", 0, "Windows engine (acwin.exe) not found. Set path in Build Settings.");
        }
        break;
    }

    case BuildTarget::MacOS:
    {
        // Create a macOS .app bundle structure
        // GameName.app/
        //   Contents/
        //     Info.plist
        //     MacOS/
        //       ags          (engine binary)
        //     Resources/
        //       game.ags     (game data)
        //       acsetup.cfg  (config)
        std::string game_name = "AGSGame";
        if (!build_game_title_.empty())
        {
            game_name = build_game_title_;
            // Sanitize for filesystem
            for (auto& ch : game_name)
                if (ch == '/' || ch == '\\' || ch == ':') ch = '_';
        }

        std::string bundle_dir = target_dir + "/" + game_name + ".app";
        std::string contents_dir = bundle_dir + "/Contents";
        std::string macos_dir = contents_dir + "/MacOS";
        std::string resources_dir = contents_dir + "/Resources";

        MakeDirRecursive(macos_dir);
        MakeDirRecursive(resources_dir);

        // Write Info.plist
        {
            std::string plist_path = contents_dir + "/Info.plist";
            std::ofstream plist(plist_path);
            if (plist.is_open())
            {
                plist << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
                plist << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" "
                         "\"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";
                plist << "<plist version=\"1.0\">\n";
                plist << "<dict>\n";
                plist << "  <key>CFBundleName</key>\n";
                plist << "  <string>" << game_name << "</string>\n";
                plist << "  <key>CFBundleIdentifier</key>\n";
                plist << "  <string>com.ags.game." << game_name << "</string>\n";
                plist << "  <key>CFBundleVersion</key>\n";
                plist << "  <string>1.0</string>\n";
                plist << "  <key>CFBundleExecutable</key>\n";
                plist << "  <string>ags</string>\n";
                plist << "  <key>CFBundlePackageType</key>\n";
                plist << "  <string>APPL</string>\n";
                plist << "  <key>NSHighResolutionCapable</key>\n";
                plist << "  <true/>\n";
                plist << "</dict>\n";
                plist << "</plist>\n";
                plist.close();
                Log("[Build]   Created Info.plist");
            }
        }

        // Copy macOS engine binary if available
        if (!config.engine_macos_path.empty() && FileExists(config.engine_macos_path))
        {
            std::string dst = macos_dir + "/ags";
            CopyFile_(config.engine_macos_path, dst);
#ifndef _WIN32
            std::filesystem::permissions(dst, std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
#endif
            Log("[Build]   Copied macOS engine: %s", config.engine_macos_path.c_str());
        }
        else
        {
            result.AddWarning("", 0,
                "macOS engine binary not found. Set engine_macos_path in Build Settings. "
                "Bundle structure created but no engine copied.");
        }

        // Copy game data into Resources
        std::string pak_src2 = data_dir + "/game.ags";
        if (FileExists(pak_src2))
        {
            CopyFile_(pak_src2, resources_dir + "/game.ags");
            Log("[Build]   Copied game.ags to Resources/");
        }
        // Copy VOX files into Resources
        copyVoxFiles(resources_dir);

        Log("[Build]   macOS bundle created: %s", bundle_dir.c_str());
        break;
    }

    case BuildTarget::Web:
    {
        // Create a web deployment directory with:
        //   index.html         (HTML shell page)
        //   game.ags           (game data)
        //   ags.js / ags.wasm  (engine files, if available)
        Log("[Build]   Preparing Web (Emscripten) build...");

        // Copy game data
        std::string pak_src3 = data_dir + "/game.ags";
        if (FileExists(pak_src3))
        {
            CopyFile_(pak_src3, target_dir + "/game.ags");
            Log("[Build]   Copied game.ags to web output");
        }
        // Copy VOX files for web target
        copyVoxFiles(target_dir);

        // Copy Emscripten engine files if configured
        if (!config.engine_web_path.empty())
        {
            // engine_web_path should point to a directory containing ags.js and ags.wasm
            std::string js_path = config.engine_web_path + "/ags.js";
            std::string wasm_path = config.engine_web_path + "/ags.wasm";
            std::string data_path = config.engine_web_path + "/ags.data";

            if (FileExists(js_path))
            {
                CopyFile_(js_path, target_dir + "/ags.js");
                Log("[Build]   Copied ags.js");
            }
            if (FileExists(wasm_path))
            {
                CopyFile_(wasm_path, target_dir + "/ags.wasm");
                Log("[Build]   Copied ags.wasm");
            }
            if (FileExists(data_path))
            {
                CopyFile_(data_path, target_dir + "/ags.data");
                Log("[Build]   Copied ags.data");
            }
        }
        else
        {
            result.AddWarning("", 0,
                "Emscripten engine path not set. Set engine_web_path in Build Settings "
                "to a directory containing ags.js and ags.wasm.");
        }

        // Generate index.html shell
        {
            std::string game_name = build_game_title_;
            std::string html_path = target_dir + "/index.html";
            std::ofstream html(html_path);
            if (html.is_open())
            {
                html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
                html << "  <meta charset=\"UTF-8\">\n";
                html << "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
                html << "  <title>" << game_name << "</title>\n";
                html << "  <style>\n";
                html << "    body { margin: 0; background: #000; display: flex; ";
                html << "justify-content: center; align-items: center; min-height: 100vh; }\n";
                html << "    canvas { border: 1px solid #333; }\n";
                html << "    #status { color: #fff; font-family: monospace; text-align: center; ";
                html << "padding: 20px; }\n";
                html << "  </style>\n</head>\n<body>\n";
                html << "  <div id=\"status\">Loading " << game_name << "...</div>\n";
                html << "  <canvas id=\"canvas\" oncontextmenu=\"event.preventDefault()\" ";
                html << "tabindex=\"-1\"></canvas>\n";
                html << "  <script>\n";
                html << "    var Module = {\n";
                html << "      canvas: document.getElementById('canvas'),\n";
                html << "      arguments: ['game.ags'],\n";
                html << "      setStatus: function(text) {\n";
                html << "        document.getElementById('status').innerText = text;\n";
                html << "        if (!text) document.getElementById('status').style.display = 'none';\n";
                html << "      },\n";
                html << "      onRuntimeInitialized: function() {\n";
                html << "        document.getElementById('status').style.display = 'none';\n";
                html << "      }\n";
                html << "    };\n";
                html << "  </script>\n";
                html << "  <script src=\"ags.js\"></script>\n";
                html << "</body>\n</html>\n";
                html.close();
                Log("[Build]   Generated index.html");
            }
        }

        Log("[Build]   Web build output: %s", target_dir.c_str());
        break;
    }

    case BuildTarget::Debug:
    {
        // For debug, copy engine and data to a debug subdir
        std::string engine = FindEngineExecutable(config);
        if (!engine.empty())
        {
            std::string dst = target_dir + "/ags";
            CopyFile_(engine, dst);
#ifndef _WIN32
            std::filesystem::permissions(dst, std::filesystem::perms::owner_all | std::filesystem::perms::group_read | std::filesystem::perms::group_exec | std::filesystem::perms::others_read | std::filesystem::perms::others_exec);
#endif
        }
        break;
    }

    default:
        break;
    }

    return true;
}

// -------------------------------------------------------------------------
// Generate acsetup.cfg configuration file
// -------------------------------------------------------------------------
bool BuildSystem::GenerateConfigFile(Project& project, const BuildConfig& config,
                                      const std::string& target_dir)
{
    GameData* gd = project.GetGameData();
    if (!gd) return false;

    std::string cfg_path = target_dir + "/acsetup.cfg";
    std::ofstream cfg(cfg_path);
    if (!cfg.is_open()) return false;

    cfg << "[misc]\n";
    cfg << "gamecolordepth=" << gd->color_depth << "\n";
    cfg << "defaultres=" << gd->resolution_width << "x" << gd->resolution_height << "\n";
    cfg << "titletext=" << gd->game_title << "\n";
    cfg << "\n";

    cfg << "[graphics]\n";
    cfg << "driver=auto\n";
    cfg << "windowed=1\n";
    cfg << "screen_def=scaling\n";
    cfg << "game_scale_fs=proportional\n";
    cfg << "game_scale_win=round\n";
    cfg << "filter=stdscale\n";
    cfg << "refresh=0\n";
    cfg << "render_at_screenres=0\n";
    cfg << "supersampling=1\n";
    cfg << "vsync=0\n";
    cfg << "\n";

    cfg << "[sound]\n";
    cfg << "enabled=1\n";
    cfg << "digiid=auto\n";
    cfg << "midiid=auto\n";
    cfg << "threaded=1\n";
    cfg << "\n";

    if (config.debug_mode)
    {
        cfg << "[debug]\n";
        cfg << "enabled=1\n";
        cfg << "debugger=1\n";
        cfg << "\n";
    }

    cfg.close();
    return true;
}

// =========================================================================
// Main build pipeline
// =========================================================================
BuildResult BuildSystem::BuildGame(Project& project, const BuildConfig& config)
{
    BuildResult result;
    building_ = true;
    config_ = config;

    auto start_time = std::chrono::steady_clock::now();

    // Calculate total steps
    int total_steps = 3; // pre-check + compile + data file
    total_steps += 1;    // packaging
    for (int i = 0; i < (int)BuildTarget::Count; i++)
        if (config.targets[i]) total_steps++;
    total_steps += 1;    // config file

    progress_.Reset(total_steps);

    Log("[Build] ========================================");
    Log("[Build] Starting game build...");
    Log("[Build] ========================================");

    // Cache game title for platform-specific builds
    {
        GameData* gd = project.GetGameData();
        build_game_title_ = (gd && !gd->game_title.empty()) ? gd->game_title : "AGSGame";
    }

    // Step 1: Create directories
    progress_.Advance("Creating build directories");
    if (!config.output_base_dir.empty())
        config_.output_base_dir = config.output_base_dir;
    else
        config_.SetDefaults(project.GetProjectDir());

    CreateBuildDirectories(config_, result);

    // Step 2: Pre-build checks
    progress_.Advance("Validating project");
    Log("[Build] Validating project...");
    if (!PreBuildChecks(project, result))
    {
        Log("[Error] Pre-build validation failed.");
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }
    Log("[Build] Project validated OK.");

    // Step 3: Compile all scripts
    Log("[Build] Compiling scripts...");
    if (!CompileAllScripts(project, config_, result))
    {
        Log("[Error] Script compilation failed with %d error(s).", result.ErrorCount());
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }

    // Step 4: Write game data file
    std::string dta_path = GetCompiledDir(config_) + "/_temp/ac2game.dta";
    if (!WriteGameDataFile(project, config_, result, dta_path))
    {
        Log("[Error] Failed to write game data file.");
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }

    // Step 5: Package assets
    std::string data_dir = GetDataDir(config_);
    if (!PackageAssets(project, config_, result, data_dir))
    {
        Log("[Error] Asset packaging failed.");
        result.success = false;
        building_ = false;
        auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
        last_result_ = result;
        return result;
    }

    // Step 5b: Create VOX files (audio.vox, speech.vox)
    CreateVOXFiles(project, config_, result, data_dir);

    // Step 6: Copy engine files per target
    for (int i = 0; i < (int)BuildTarget::Count; i++)
    {
        if (!config_.targets[i]) continue;
        BuildTarget target = (BuildTarget)i;
        if (target == BuildTarget::DataFile) continue; // Already handled

        std::string target_dir = GetTargetDir(config_, target);
        CopyEngineFiles(config_, target, result, target_dir);
        GenerateConfigFile(project, config_, target_dir);
    }

    // Also generate config in data dir
    GenerateConfigFile(project, config_, data_dir);

    // Done!
    auto end_time = std::chrono::steady_clock::now();
    result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();
    result.success = (result.ErrorCount() == 0);
    building_ = false;
    last_result_ = result;

    if (result.success)
    {
        Log("[Build] ========================================");
        Log("[Build] Build succeeded in %.2f seconds.", result.elapsed_seconds);
        Log("[Build]   %d warning(s)", result.WarningCount());
        Log("[Build]   Output: %s", data_dir.c_str());
        Log("[Build] ========================================");
    }
    else
    {
        Log("[Build] ========================================");
        Log("[Error] Build FAILED: %d error(s), %d warning(s) (%.2f s)",
            result.ErrorCount(), result.WarningCount(), result.elapsed_seconds);
        Log("[Build] ========================================");
    }

    return result;
}

// =========================================================================
// Run game
// =========================================================================
bool BuildSystem::RunGame(Project& project, const BuildConfig& config, bool debug)
{
    if (IsRunning())
    {
        Log("[Info] Game is already running.");
        return false;
    }

    // Find the appropriate directory
    BuildConfig run_config = config;

    std::string run_dir;
    if (debug)
    {
        run_dir = GetTargetDir(run_config, BuildTarget::Debug);
    }
    else
    {
        // Prefer Linux target, then Data
#ifdef _WIN32
        run_dir = GetTargetDir(run_config, BuildTarget::Windows);
#else
        if (run_config.targets[(int)BuildTarget::Linux])
            run_dir = GetTargetDir(run_config, BuildTarget::Linux);
        else
            run_dir = GetDataDir(run_config);
#endif
    }

    // Find engine
    std::string engine = FindEngineExecutable(run_config);
    if (engine.empty())
    {
        // Try in the run directory
        std::string local_engine = run_dir + "/ags";
        if (FileExists(local_engine))
            engine = local_engine;
    }

    if (engine.empty())
    {
        Log("[Error] Cannot find AGS engine executable. Set the engine path in Build Settings.");
        return false;
    }

    // Check for game data
    std::string game_data = run_dir + "/game.ags";
    if (!FileExists(game_data))
    {
        // Try Data dir
        game_data = GetDataDir(run_config) + "/game.ags";
        if (!FileExists(game_data))
        {
            Log("[Error] Game data not found. Build the game first (F7).");
            return false;
        }
    }

    Log("[Build] Running game: %s", engine.c_str());
    Log("[Build]   Working dir: %s", run_dir.c_str());

    // Debug controller setup
    const std::string debugger_token = "agsedit";
    debug_run_active_ = debug;
    if (debug)
    {
        debug_controller_.StartSession(run_dir);
    }

#ifdef _WIN32
    // Windows: use CreateProcess
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    std::string cmd = "\"" + engine + "\" \"" + game_data + "\"";
    if (debug) cmd += " --tell --no-message-box --enabledebugger " + debugger_token;

    if (CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr,
                       FALSE, 0, nullptr, run_dir.c_str(), &si, &pi))
    {
        game_pid_ = (int)pi.dwProcessId;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Log("[Build] Game started (PID: %d)", game_pid_);
        return true;
    }
    else
    {
        Log("[Error] Failed to start game process.");
        if (debug) { debug_controller_.EndSession(); debug_run_active_ = false; }
        return false;
    }
#else
    // Unix: fork + exec with stderr/stdout capture
    int pipefd[2] = {-1, -1};
    if (pipe(pipefd) == 0)
    {
        // Will capture game's stderr+stdout
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        if (pipefd[0] >= 0)
        {
            close(pipefd[0]); // close read end in child
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
        }

        if (chdir(run_dir.c_str()) != 0)
            _exit(1);

        if (debug)
            execlp(engine.c_str(), engine.c_str(), game_data.c_str(),
                   "--tell", "--no-message-box",
                   "--enabledebugger", debugger_token.c_str(), nullptr);
        else
            execlp(engine.c_str(), engine.c_str(), game_data.c_str(), nullptr);
        _exit(1);
    }
    else if (pid > 0)
    {
        if (pipefd[1] >= 0) close(pipefd[1]); // close write end in parent
        game_pid_ = (int)pid;
        game_output_fd_ = pipefd[0]; // store read end for later polling
        Log("[Build] Game started (PID: %d)", game_pid_);
        return true;
    }
    else
    {
        if (pipefd[0] >= 0) { close(pipefd[0]); close(pipefd[1]); }
        Log("[Error] Failed to fork game process.");
        if (debug) { debug_controller_.EndSession(); debug_run_active_ = false; }
        return false;
    }
#endif
}

// =========================================================================
// Run game setup dialog
// =========================================================================
bool BuildSystem::RunSetup(Project& project, const BuildConfig& config)
{
    if (IsRunning())
    {
        Log("[Info] Game is already running.");
        return false;
    }

    // Determine run directory (same as RunGame logic)
    BuildConfig run_config = config;
    std::string run_dir;
#ifdef _WIN32
    run_dir = GetTargetDir(run_config, BuildTarget::Windows);
#else
    if (run_config.targets[(int)BuildTarget::Linux])
        run_dir = GetTargetDir(run_config, BuildTarget::Linux);
    else
        run_dir = GetDataDir(run_config);
#endif

    // Find engine
    std::string engine = FindEngineExecutable(run_config);
    if (engine.empty())
    {
        std::string local_engine = run_dir + "/ags";
        if (FileExists(local_engine))
            engine = local_engine;
    }
    if (engine.empty())
    {
        Log("[Error] Cannot find AGS engine executable. Set the engine path in Build Settings.");
        return false;
    }

    // Check for game data
    std::string game_data = run_dir + "/game.ags";
    if (!FileExists(game_data))
    {
        game_data = GetDataDir(run_config) + "/game.ags";
        if (!FileExists(game_data))
        {
            Log("[Error] Game data not found. Build the game first (F7).");
            return false;
        }
    }

    Log("[Build] Running game setup: %s --setup", engine.c_str());
    Log("[Build]   Working dir: %s", run_dir.c_str());

#ifdef _WIN32
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    std::string cmd = "\"" + engine + "\" \"" + game_data + "\" --setup";
    if (CreateProcessA(nullptr, (LPSTR)cmd.c_str(), nullptr, nullptr,
                       FALSE, 0, nullptr, run_dir.c_str(), &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Log("[Info] Setup dialog launched.");
        return true;
    }
    else
    {
        Log("[Error] Failed to start setup process.");
        return false;
    }
#else
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        if (chdir(run_dir.c_str()) != 0)
            _exit(1);
        execlp(engine.c_str(), engine.c_str(), game_data.c_str(),
               "--setup", nullptr);
        _exit(1);
    }
    else if (pid > 0)
    {
        Log("[Info] Setup dialog launched (PID: %d).", (int)pid);
        // Don't track this PID — setup runs independently
        return true;
    }
    else
    {
        Log("[Error] Failed to fork setup process.");
        return false;
    }
#endif
}

// =========================================================================
// Stop game
// =========================================================================
void BuildSystem::StopGame()
{
    if (game_pid_ <= 0) return;

    if (debug_run_active_)
    {
        debug_controller_.EndSession();
        debug_run_active_ = false;
    }

    Log("[Build] Stopping game (PID: %d)...", game_pid_);

#ifdef _WIN32
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)game_pid_);
    if (hProcess)
    {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
#else
    kill((pid_t)game_pid_, SIGTERM);
    // Wait briefly, then force kill
    int status;
    pid_t result = waitpid((pid_t)game_pid_, &status, WNOHANG);
    if (result == 0)
    {
        usleep(500000); // 500ms
        kill((pid_t)game_pid_, SIGKILL);
        waitpid((pid_t)game_pid_, &status, 0);
    }
    // Close output pipe
    if (game_output_fd_ >= 0)
    {
        close(game_output_fd_);
        game_output_fd_ = -1;
    }
#endif

    game_pid_ = 0;
    Log("[Info] Game stopped.");
}

// =========================================================================
// Poll game process output
// =========================================================================
void BuildSystem::PollGameOutput()
{
#ifndef _WIN32
    if (game_output_fd_ < 0) return;

    // Non-blocking read from the pipe
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(game_output_fd_, &fds);
    struct timeval tv = {0, 0}; // no wait

    while (select(game_output_fd_ + 1, &fds, nullptr, nullptr, &tv) > 0)
    {
        char buf[1024];
        ssize_t n = read(game_output_fd_, buf, sizeof(buf) - 1);
        if (n > 0)
        {
            buf[n] = '\0';
            // Split by lines and log each
            std::string chunk(buf, n);
            size_t pos = 0;
            while (pos < chunk.size())
            {
                size_t nl = chunk.find('\n', pos);
                if (nl == std::string::npos)
                {
                    Log("[Game] %s", chunk.substr(pos).c_str());
                    break;
                }
                Log("[Game] %s", chunk.substr(pos, nl - pos).c_str());
                pos = nl + 1;
            }
        }
        else
        {
            // EOF or error — pipe closed (game exited)
            close(game_output_fd_);
            game_output_fd_ = -1;
            break;
        }

        FD_ZERO(&fds);
        FD_SET(game_output_fd_, &fds);
        tv = {0, 0};
    }

    // Check if game process has exited
    if (game_pid_ > 0)
    {
        int status;
        pid_t result = waitpid((pid_t)game_pid_, &status, WNOHANG);
        if (result > 0)
        {
            int exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            Log("[Build] Game process exited (code: %d)", exit_code);
            if (debug_run_active_)
            {
                debug_controller_.EndSession();
                debug_run_active_ = false;
            }
            game_pid_ = 0;
            if (game_output_fd_ >= 0)
            {
                close(game_output_fd_);
                game_output_fd_ = -1;
            }
        }
    }
#endif

    // Poll debug controller for messages from engine
    if (debug_run_active_ && debug_controller_.IsActive())
    {
        debug_controller_.Poll();
    }
}

} // namespace AGSEditor
