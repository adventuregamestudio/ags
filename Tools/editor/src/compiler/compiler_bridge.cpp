// AGS Editor ImGui - Compiler Bridge implementation
// Mirrors the C# editor's compilation pipeline:
//   1. Load built-in header (agsdefns.sh) -- provides all AGS API types
//   2. Preprocess all code (strip comments, handle #ifdef/#define)
//   3. Add preprocessed headers via ccAddDefaultHeader()
//   4. Compile preprocessed script body via ccCompileText()
#include "compiler_bridge.h"
#include "core/logger.h"

#include "script/cs_compiler.h"
#include "script/cc_common.h"
#include "script/cc_script.h"
#include "preproc/preprocessor.h"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace AGSEditor
{

CompilerBridge::CompilerBridge()
{
    ccSetSoftwareVersion("3.6.3");
    LoadBuiltInHeader();
}

CompilerBridge::~CompilerBridge()
{
    ccRemoveDefaultHeaders();
}

void CompilerBridge::Configure(const ScriptCompileSettings& settings)
{
    settings_ = settings;
}

bool CompilerBridge::LoadBuiltInHeader()
{
    if (builtin_header_loaded_)
        return !builtin_header_.empty();

    builtin_header_loaded_ = true;

    // TODO: this needs proper packaging
    // Try to find agsdefns.sh at various locations relative to the build
    std::vector<std::string> search_paths = {
        // Relative to SOURCE_DIR (defined by CMake)
#ifdef SOURCE_DIR
        std::string(SOURCE_DIR) + "/../Editor/AGS.Editor/Resources/agsdefns.sh",
#endif
        // Relative to current working directory (common in dev builds)
        "../Editor/AGS.Editor/Resources/agsdefns.sh",
        "Editor/AGS.Editor/Resources/agsdefns.sh",
        // Installed location
        "/usr/share/ags/agsdefns.sh",
    };

    for (const auto& path : search_paths)
    {
        std::ifstream file(path);
        if (file.is_open())
        {
            std::ostringstream ss;
            ss << file.rdbuf();
            builtin_header_ = ss.str();
            Logger::Log("[Compiler] Loaded built-in header: %s (%zu bytes)",
                    path.c_str(), builtin_header_.size());
            return true;
        }
    }

    Logger::Warn("[Compiler] Could not find agsdefns.sh (built-in script header).");
    Logger::Warn("[Compiler]   Scripts using AGS API types will fail to compile.");
    Logger::Warn("[Compiler]   Searched paths:");
    for (const auto& path : search_paths)
        Logger::Warn("[Compiler]     %s", path.c_str());
    return false;
}

CompileResult CompilerBridge::CompileScript(const std::string& script_path,
                                              const std::string& header_content)
{
    CompileResult result;
    result.success = false;

    // Read script file
    std::ifstream file(script_path);
    if (!file.is_open())
    {
        std::string msg = "Cannot open script file: " + script_path;
        Logger::Error("[Compiler] %s", msg.c_str());
        result.errors.push_back({script_path, 0, msg});
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string script_content = buffer.str();
    file.close();

    return CompileScriptText(script_content, script_path, header_content);
}

CompileResult CompilerBridge::CompileScriptText(const std::string& script_text,
                                                  const std::string& script_name,
                                                  const std::string& header_content)
{
    CompileResult result;
    result.success = false;

    // --- Setup preprocessor (macros driven by project settings) ---
    AGS::Preprocessor::Preprocessor pp;
    pp.SetAppVersion("3.6.3");
    pp.DefineMacro("AGS_SUPPORTS_IFVER", "1");

    // NOTE: FontType, CursorMode, AudioType are NOT pre-defined here.
    // They are #define'd inside agsdefns.sh and #undef'd at its end,
    // making room for the corresponding enum declarations in the
    // auto-generated header.  The preprocessor preserves macro state
    // across Preprocess() calls, so the agsdefns.sh definitions suffice.

    // Feature macros -- driven by ScriptCompileSettings
    if (settings_.enforce_new_strings)
        pp.DefineMacro("AGS_NEW_STRINGS", "1");
    if (settings_.enforce_object_scripting)
        pp.DefineMacro("STRICT", "1");
    if (settings_.left_to_right_precedence)
        pp.DefineMacro("LRPRECEDENCE", "1");
    if (settings_.enforce_new_strings)
        pp.DefineMacro("STRICT_STRINGS", "1");
    if (settings_.enforce_new_audio)
        pp.DefineMacro("STRICT_AUDIO", "1");
    if (!settings_.use_old_custom_dialog_api)
        pp.DefineMacro("NEW_DIALOGOPTS_API", "1");
    if (!settings_.use_old_keyboard_handling)
        pp.DefineMacro("NEW_KEYINPUT_API", "1");
    if (settings_.debug_mode)
        pp.DefineMacro("DEBUG", "1");

    // game_data stores actual kScriptAPI_* enum values (INT_MAX = Highest)
    int resolved_api_ver = settings_.script_api_version;

    // Define SCRIPT_API version macros: all versions up to script_api_version
    // (Highest = INT_MAX means define all)
    for (int i = 0; i < kScriptAPIVersionCount; i++)
    {
        std::string macro = "SCRIPT_API_" + std::string(kScriptAPIVersions[i].name);
        pp.DefineMacro(macro.c_str(), "1");
        // Stop if we've reached the target API version (unless Highest)
        if (resolved_api_ver != kScriptAPIHighest &&
            kScriptAPIVersions[i].value >= resolved_api_ver)
            break;
    }

    // game_data stores actual kScriptAPI_* enum values (INT_MAX = Highest)
    int resolved_compat_ver = settings_.script_compat_level;
    // Resolve "Highest" to actual latest version (matches C# GetActualAPI)
    if (resolved_compat_ver == kScriptAPIHighest)
        resolved_compat_ver = kScriptAPIVersions[kScriptAPIVersionCount - 1].value;

    // Define SCRIPT_COMPAT version macros: all versions from script_compat_level onward
    for (int i = 0; i < kScriptAPIVersionCount; i++)
    {
        if (kScriptAPIVersions[i].value >= resolved_compat_ver)
        {
            std::string macro = "SCRIPT_COMPAT_" + std::string(kScriptAPIVersions[i].name);
            pp.DefineMacro(macro.c_str(), "1");
        }
    }

    // --- Setup compiler options (driven by settings) ---
    ccSetOption(SCOPT_EXPORTALL, 1);
    ccSetOption(SCOPT_LINENUMBERS, 1);
    ccSetOption(SCOPT_LEFTTORIGHT, settings_.left_to_right_precedence ? 1 : 0);
    ccSetOption(SCOPT_OLDSTRINGS, settings_.enforce_new_strings ? 0 : 1);
    ccRemoveDefaultHeaders();

    // Keep preprocessed header strings alive until compilation completes.
    // ccAddDefaultHeader() stores raw const char* pointers (no copy),
    // so the underlying data must outlive the ccCompileText() call.
    std::vector<AGS::Common::String> kept_headers;

    // --- 1. Preprocess and add built-in header (agsdefns.sh) ---
    if (!builtin_header_.empty())
    {
        AGS::Common::String pp_builtin = pp.Preprocess(
            builtin_header_.c_str(), "_BuiltInScriptHeader");
        const auto& pp_err = pp.GetLastError();
        if (pp_err.Type != AGS::Preprocessor::ErrorCode::None)
        {
            std::string msg = "Preprocessor error in built-in header: " +
                              std::string(pp_err.Message.GetCStr());
            Logger::Error("[Compiler] %s", msg.c_str());
            result.errors.push_back({script_name, 0, msg});
            return result;
        }
        kept_headers.push_back(pp_builtin);
        ccAddDefaultHeader(kept_headers.back().GetCStr(), "_BuiltInScriptHeader");
    }

    // --- 2. Preprocess and add user header if provided ---
    if (!header_content.empty())
    {
        AGS::Common::String pp_header = pp.Preprocess(
            header_content.c_str(), "header");
        const auto& pp_err = pp.GetLastError();
        if (pp_err.Type != AGS::Preprocessor::ErrorCode::None)
        {
            std::string msg = "Preprocessor error in header: " +
                              std::string(pp_err.Message.GetCStr());
            Logger::Error("[Compiler] %s", msg.c_str());
            result.errors.push_back({script_name, 0, msg});
            return result;
        }
        kept_headers.push_back(pp_header);
        ccAddDefaultHeader(kept_headers.back().GetCStr(), "header");
    }

    // --- 3. Preprocess the main script body ---
    AGS::Common::String pp_script = pp.Preprocess(
        script_text.c_str(), script_name.c_str());
    {
        const auto& pp_err = pp.GetLastError();
        if (pp_err.Type != AGS::Preprocessor::ErrorCode::None)
        {
            std::string msg = "Preprocessor error: " +
                              std::string(pp_err.Message.GetCStr());
            Logger::Error("[Compiler] %s:%d: %s",
                    script_name.c_str(), 0, msg.c_str());
            result.errors.push_back({script_name, 0, msg});
            return result;
        }
    }

    // --- 4. Compile ---
    cc_clear_error();
    ccScript* compiled = ccCompileText(pp_script.GetCStr(), script_name.c_str());

    if (!compiled)
    {
        // Get error info from the global error state
        const ScriptError& err = cc_get_error();
        if (err.HasError)
        {
            Logger::Error("[Compiler] %s:%d: error: %s",
                    script_name.c_str(), err.Line, err.ErrorString.GetCStr());
            result.errors.push_back({
                script_name,
                err.Line,
                err.ErrorString.GetCStr()
            });
        }
        else
        {
            Logger::Error("[Compiler] %s: unknown compilation error", script_name.c_str());
            result.errors.push_back({script_name, 0, "Unknown compilation error"});
        }
        return result;
    }

    // Success
    Logger::Info("[Compiler] %s: compiled successfully", script_name.c_str());
    delete compiled;
    result.success = true;
    return result;
}

CompileResult CompilerBridge::CompileAll(const std::string& game_path)
{
    CompileResult result;
    result.success = false;

    // Full compilation is now handled by BuildSystem.
    // This method remains for single-script compilation workflows.
    // Use BuildSystem::BuildGame() for the full pipeline.
    result.errors.push_back({game_path, 0,
        "Use Build > Build Game (F7) for full compilation. "
        "This compiles via the BuildSystem pipeline."});

    return result;
}

std::string CompilerBridge::GetCompilerVersion() const
{
    return "AGS Script Compiler 3.6.3";
}

} // namespace AGSEditor
