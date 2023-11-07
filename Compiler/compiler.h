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
//
// script compiler core
//
//=============================================================================

#ifndef __CC_COMPILER_H
#define __CC_COMPILER_H

#define HIGHEST_SCRIPT_API "Highest"

#include <utility>
#include <string>
#include <vector>

class CompilerOptions{
    struct Flags {
        bool ExportAll = true;               // export all functions automatically
        bool ShowWarnings = true;             // printf warnings to console
        bool LineNumbers = true;             // include line numbers in compiled code
        bool AutoImport = false;              // when creating instance, export funcs to other scripts
        bool DebugRun = false;                // write instructions as they are processed to log file
        bool NoImportOverride = false;        // do not allow an import to be re-declared
        bool EnforceObjectBasedScript = true;
        bool EnforceNewStrings = true;        // do not allow old-style strings
        bool EnforceNewAudio = true;
        bool UseOldCustomDialogOptionsAPI = false;
    };

    struct ScriptAPI {
        std::string ScriptAPIVersion = HIGHEST_SCRIPT_API;
        std::string ScriptCompatLevel = HIGHEST_SCRIPT_API;
    };

public:
    ScriptAPI ScriptAPI;
    Flags Flags;
    bool PreprocessOnly = false;
    bool DebugMode = false; // build for debug
    std::vector<std::pair<std::string, std::string>> Macros{};
    std::vector<std::string> HeaderFiles{};
    std::string InputScriptFile{};
    std::string OutputObjFile{};
    std::string Version{};
    CompilerOptions() = default;
    ~CompilerOptions() = default;
    void PrintToStdout() const;
};

int Compile(const CompilerOptions& comp_opts);
std::vector<const char *> GetScriptAPIs();

#endif //__CC_COMPILER_H
