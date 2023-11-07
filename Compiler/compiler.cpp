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
#include <utility>
#include <iostream>

#include "compiler.h"
#include "script/cs_compiler.h"
#include "script/cc_common.h"
#include "script/cc_internal.h"
#include "util/filestream.h"
#include "util/file.h"
#include "util/path.h"
#include "util/textstreamreader.h"
#include "util/string_compat.h"
#include "preproc/preprocessor.h"
#include "compiler.h"

using namespace AGS::Common;

// Reimplementation of project-dependent functions from Common
String cc_format_error(const String &message)
{
    if (currentline > 0)
        return String::FromFormat("Error (line %d): %s", currentline, message.GetCStr());
    else
        return String::FromFormat("Error (line unknown): %s", message.GetCStr());
}

String cc_get_callstack(int max_lines)
{
    return "";
}


const char* PREFIX_SCRIPT_COMPAT = "SCRIPT_COMPAT_";
const char* PREFIX_SCRIPT_API = "SCRIPT_API_";
std::vector<const char *> ScriptAPIs = {"v321",
                                        "v330",
                                        "v334",
                                        "v335",
                                        "v340",
                                        "v341",
                                        "v350",
                                        "v3507",
                                        "v351",
                                        "v360",
                                        HIGHEST_SCRIPT_API};

std::vector<const char *> GetScriptAPIs()
{
    return ScriptAPIs;
}


void CompilerOptions::PrintToStdout() const {
    printf("\n--- Compiler Settings ---\n");
    printf("Input: %s\n", InputScriptFile.c_str());
    printf("Output: %s\n", OutputObjFile.c_str());
    printf("Headers:");
    bool comma = false;
    for (const auto& header : HeaderFiles)
    {
        if (comma) printf(", ");
        printf("%s", header.c_str());
        comma = true;
    }
    printf("\nCustom Macros:");
    comma = false;
    for (const auto& macro : Macros)
    {
        if (comma) printf(", ");
        printf("%s:%s", macro.first.c_str(), macro.second.c_str());
        comma = true;
    }
    printf("\nVersion: %s\n", Version.c_str());
    printf("ScriptAPIVersion: %s\n", ScriptAPI.ScriptAPIVersion.c_str());
    printf("ScriptCompatLevel: %s\n", ScriptAPI.ScriptCompatLevel.c_str());
    printf("Flags: ");
    if (Flags.ExportAll) printf("ExportAll; ");
    if (Flags.ShowWarnings) printf("ShowWarnings; ");
    if (Flags.LineNumbers) printf("LineNumbers; ");
    if (Flags.AutoImport) printf("AutoImport; ");
    if (Flags.DebugRun) printf("DebugRun; ");
    if (Flags.NoImportOverride) printf("NoImportOverride; ");
    if (Flags.EnforceObjectBasedScript) printf("EnforceObjectBasedScript; ");
    if (Flags.EnforceNewStrings) printf("EnforceNewStrings; ");
    if (Flags.EnforceNewAudio) printf("EnforceNewAudio; ");
    if (Flags.UseOldCustomDialogOptionsAPI) printf("UseOldCustomDialogOptionsAPI; ");
    if(DebugMode) printf("\nDebugMode\n");
}


int Compile(const CompilerOptions& comp_opts)
{
    comp_opts.PrintToStdout();
    AGS::Preprocessor::Preprocessor pp = AGS::Preprocessor::Preprocessor();
    std::vector<std::string> scriptAPIVersionMacros;
    std::vector<std::string> scriptCompatLevelMacros;

    for(int i=0; i<ScriptAPIs.size()-1; i++)
    {
        scriptAPIVersionMacros.emplace_back(std::string(PREFIX_SCRIPT_API) + ScriptAPIs[i]);
        scriptCompatLevelMacros.emplace_back(std::string(PREFIX_SCRIPT_COMPAT) + ScriptAPIs[i]);
    }

    //-----------------------------------------------------------------------//
    // Configure macros
    //-----------------------------------------------------------------------//
    pp.DefineMacro("AGS_NEW_STRINGS", "1");
    pp.DefineMacro("AGS_SUPPORTS_IFVER", "1");

    if(comp_opts.DebugMode) {
        pp.DefineMacro("DEBUG", "1");
    }

    if (comp_opts.Flags.EnforceObjectBasedScript)
    {
        pp.DefineMacro("STRICT", "1");
    }
    if (comp_opts.Flags.EnforceNewStrings)
    {
        pp.DefineMacro("STRICT_STRINGS", "1");
    }
    if (comp_opts.Flags.EnforceNewAudio)
    {
        pp.DefineMacro("STRICT_AUDIO", "1");
    }
    if (!comp_opts.Flags.UseOldCustomDialogOptionsAPI)
    {
        pp.DefineMacro("NEW_DIALOGOPTS_API", "1");
    }
    for(int i=0; i<ScriptAPIs.size()-1; i++) // Set Script API macros
    {
        pp.DefineMacro(scriptAPIVersionMacros[i].c_str(), "1");
        if(comp_opts.ScriptAPI.ScriptAPIVersion == ScriptAPIs[i]) {
            break;
        }
    }
    if(comp_opts.ScriptAPI.ScriptCompatLevel != HIGHEST_SCRIPT_API) {
        // Set API Compatibility macros
        for (int i = ScriptAPIs.size() - 2; i >= 0; i--) {
            pp.DefineMacro(scriptCompatLevelMacros[i].c_str(), "1");
            if (comp_opts.ScriptAPI.ScriptCompatLevel == ScriptAPIs[i]) {
                break;
            }
        }
    }

    // define custom macros
    for(const auto& macro: comp_opts.Macros)
    {
        pp.DefineMacro(macro.first.c_str(), macro.second.c_str());
    }

    //-----------------------------------------------------------------------//
    // Configure compiler
    //-----------------------------------------------------------------------//
    ccSetSoftwareVersion(comp_opts.Version.c_str());

    ccSetOption(SCOPT_SHOWWARNINGS, comp_opts.Flags.ShowWarnings);

    ccSetOption(SCOPT_EXPORTALL, comp_opts.Flags.ExportAll);
    ccSetOption(SCOPT_LINENUMBERS, comp_opts.Flags.LineNumbers);
    // now deprecated, was used to prevent override imports in the room script
    ccSetOption(SCOPT_NOIMPORTOVERRIDE, comp_opts.Flags.NoImportOverride);

    ccSetOption(SCOPT_OLDSTRINGS, !comp_opts.Flags.EnforceNewStrings);

    ccRemoveDefaultHeaders();

    //-----------------------------------------------------------------------//
    // Read input files
    //-----------------------------------------------------------------------//
    std::vector<std::pair<String, String>> heads;
    for(const auto& header: comp_opts.HeaderFiles)
    {
        if (header.empty())
        {
            std::cerr << "Error: empty header filename. Do you have a trailing `:` or `;`? "<< std::endl;
            return -1;
        }

        std::unique_ptr<Stream> in (File::OpenFileRead(header.c_str()));
        if (!in)
        {
            std::cerr << "Error: failed to open header for reading: " << header << std::endl;
            return -1;
        }

        String headername = Path::GetFilename(header.c_str());
        headername = Path::RemoveExtension(headername);

        TextStreamReader sr(in.get());
        heads.emplace_back(sr.ReadAll(), headername);
        sr.ReleaseStream();
    }

    String script_input;
    const char* src = nullptr;

    if (comp_opts.InputScriptFile.empty())
    {
        std::cerr << "Error: empty script filename." << std::endl;
        return -1;
    }

    src = comp_opts.InputScriptFile.c_str();
    std::unique_ptr<Stream> in (File::OpenFileRead(src));
    if (!in)
    {
        std::cerr << "Error: failed to open script for reading: " << src << std::endl;
        return -1;
    }
    TextStreamReader sr(in.get());
    script_input = sr.ReadAll();
    sr.ReleaseStream();

    //-----------------------------------------------------------------------//
    // Preprocess headers and set them for use when compiling
    //-----------------------------------------------------------------------//
    std::vector<std::pair<String, String>> preprocessed_heads;
    for(const auto& head: heads)
    {
        String preprocessed_header = pp.Preprocess(head.first,head.second);
        preprocessed_heads.emplace_back(preprocessed_header.GetCStr(),head.second);

        ccAddDefaultHeader((char *) preprocessed_heads.back().first.GetCStr(), (char *) preprocessed_heads.back().second.GetCStr());
    }
    heads.clear();

    //-----------------------------------------------------------------------//
    // Preprocess script
    //-----------------------------------------------------------------------//
    String script_pp = nullptr;
    String filename = Path::GetFilename(comp_opts.InputScriptFile.c_str());
    String script_name = Path::RemoveExtension(filename);

    script_pp = pp.Preprocess(script_input,script_name);
    if ((script_pp == nullptr) || (cc_has_error()))
    {
        const auto &error = cc_get_error();
        std::cerr << "Error: preprocessor failed at " << script_name.GetCStr() <<
            ", line " << error.Line << " : " << error.ErrorString.GetCStr() << std::endl;
        return -1;
    }

    if(comp_opts.PreprocessOnly)
    {
        std::unique_ptr<Stream> out (File::CreateFile(comp_opts.OutputObjFile.c_str()));
        if (!out || !(out->CanWrite())) {
            std::cerr << "Error: failed to open for writing: " << comp_opts.OutputObjFile << std::endl;
            return -1;
        }
        script_pp.Write(out.get());
        return 0;
    }

    //-----------------------------------------------------------------------//
    // Compile script
    //-----------------------------------------------------------------------//
    ccScript* script = ccCompileText(script_pp.GetCStr(), script_name.GetCStr());
    if ((script == nullptr) || (cc_has_error()))
    {
        const auto &error = cc_get_error();
        std::cerr << "Error: compile failed at " << ccCurScriptName << ", line " << error.Line << " : " << error.ErrorString.GetCStr() << std::endl;
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write script object
    //-----------------------------------------------------------------------//
    if(!comp_opts.OutputObjFile.empty())
    {
        std::unique_ptr<Stream> out (File::CreateFile(comp_opts.OutputObjFile.c_str()));
        if (!out || !(out->CanWrite())) {
            std::cerr << "Error: failed to open for writing: " << comp_opts.OutputObjFile << std::endl;
            return -1;
        }
        script->Write(out.get());
    }

    return 0;
}