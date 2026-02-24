// AGS Editor ImGui - Compiler Bridge
// Interfaces between the ImGui editor and the AGS Compiler library.
// Mirrors the C# editor's compilation pipeline:
//   1. Preprocessor strips comments and handles #ifdef/#define
//   2. Built-in header (agsdefns.sh) provides AGS API types
//   3. ccCompileText compiles preprocessed code
#pragma once

#include <string>
#include <vector>
#include <climits>

namespace AGSEditor
{

// ScriptAPIVersion enum values matching Editor/AGS.Types/Enums/ScriptAPIVersion.cs
// The integer values must match exactly for correct SCRIPT_API_xxx macro generation.
struct ScriptAPIVersionEntry
{
    int value;          // enum integer value
    const char* name;   // macro suffix e.g. "v321"
};

// Ordered list of all script API versions (ascending order)
inline const ScriptAPIVersionEntry kScriptAPIVersions[] = {
    {         0, "v321"   },
    {         1, "v330"   },
    {         2, "v334"   },
    {         3, "v335"   },
    {         4, "v340"   },
    {         5, "v341"   },
    {         6, "v350"   },
    {         7, "v3507"  },
    {         8, "v351"   },
    {   3060000, "v360"   },
    {   3060026, "v36026" },
    {   3060100, "v361"   },
    {   3060200, "v362"   },
    {   3060300, "v363"   },
};
inline constexpr int kScriptAPIVersionCount = sizeof(kScriptAPIVersions) / sizeof(kScriptAPIVersions[0]);
inline constexpr int kScriptAPIHighest = INT_MAX;

// Settings that control script compilation, derived from project GameData settings.
struct ScriptCompileSettings
{
    // Script API version (as ScriptAPIVersion enum int value, or INT_MAX for Highest)
    int script_api_version = kScriptAPIHighest;
    // Script compatibility level (as ScriptAPIVersion enum int value, or INT_MAX for Highest)
    int script_compat_level = kScriptAPIHighest;

    // Feature flags (drive preprocessor macros)
    bool enforce_object_scripting = false;  // → STRICT
    bool enforce_new_strings = true;        // → STRICT_STRINGS, AGS_NEW_STRINGS
    bool left_to_right_precedence = true;   // → LRPRECEDENCE
    bool enforce_new_audio = true;          // → STRICT_AUDIO
    bool use_old_custom_dialog_api = false; // → !NEW_DIALOGOPTS_API
    bool use_old_keyboard_handling = false; // → !NEW_KEYINPUT_API

    bool debug_mode = false;                // → DEBUG
};

struct CompileError
{
    std::string file;
    int line;
    std::string message;
};

struct CompileResult
{
    bool success;
    std::vector<CompileError> errors;
    std::vector<std::string> warnings;
};

class CompilerBridge
{
public:
    CompilerBridge();
    ~CompilerBridge();

    // Configure compilation settings from project data.
    // Must be called before CompileScript/CompileScriptText if you want
    // project-specific macros instead of defaults.
    void Configure(const ScriptCompileSettings& settings);

    // Compile a single script file (reads from disk, preprocesses, compiles)
    CompileResult CompileScript(const std::string& script_path,
                                 const std::string& header_content);

    // Compile script text directly (preprocesses, compiles — no file I/O)
    CompileResult CompileScriptText(const std::string& script_text,
                                     const std::string& script_name,
                                     const std::string& header_content);

    // Compile all game scripts
    CompileResult CompileAll(const std::string& game_path);

    // Get the compiler version string
    std::string GetCompilerVersion() const;

private:
    // Load the built-in AGS script API header (agsdefns.sh)
    bool LoadBuiltInHeader();

    // The built-in header content, loaded once
    std::string builtin_header_;
    bool builtin_header_loaded_ = false;

    // Compilation settings (set via Configure(), or defaults)
    ScriptCompileSettings settings_;
};

} // namespace AGSEditor
