#include <cstdio>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "util/path.h"
#include "util/cmdlineopts.h"
#include "compiler.h"
#include "core/def_version.h"

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;

const char *HELP_STRING = R"EOS(Usage: agscc [options] <INPUT.asc>
-A <version>                 Script API Version               (default:Highest)
-C <version>                 Script API Compatibility version (default:Highest)
-H, --Headers <H1>[:<H2>...] Header Files in order  (; as separator in cmd.exe)
-D <macro>[=<val>]           Define <macro> to <val> (or 1 if <val> omitted)
-E                           Only run the preprocessor
-fshowwarnings[=0]           Print warnings to console              (default:1)
-fexportall[=0]              Exports all functions automatically    (default:1)
-flinenumbers[=0]            Include line numbers in compiled code  (default:1)
-fautoimport[=0]             Instancing exports funcs to other script
-fdebugrun[=0]               Log instructions as they are processed
-fnoimportoverride[=0]       Do not allow import to be re-declared
-fforceobjectbasedscript[=0] Enforce Object based scripting         (default:1)
-fforcenewstrings[=0]        Enforce new strings                    (default:1)
-fforcenewaudio[=0]          Enforce new audio system               (default:1)
-foldcustomdialogopt[=0]     Use old custom dialog API
-g                           Generate debug information
--tell-api-versions          Returns supported Script API Versions
-o <OUT.o>, --output <OUT.o> Place output in specified file.  (default:INPUT.o)
--override-version <VERSION> Overrides editor version
-h, --help                   Print this usage message
)EOS";

struct ParsedOptions {
    CompilerOptions Options;
    bool Exit = false;
    int ErrorCode = 0;
    ParsedOptions() = default;
    explicit ParsedOptions(int error_code) { Exit = true; ErrorCode = error_code; }
};

bool IsApiVersionValid(const String& api_version)
{
    for(auto v : GetScriptAPIs())
    {
        if(api_version == v) return true;
    }
    return false;
}

void PrintAPIVersions()
{
    printf("Valid API values to use with -A and -C options:\n");
    std::vector<const char *> apis = GetScriptAPIs();
    bool first = true;
    for(auto api : apis) {
        if (!first) printf(", ");
        printf("%s", api);
        first = false;
    }
    printf("\n");
}

ParsedOptions parser_to_compiler_opts(const ParseResult& parseResult)
{
    CompilerOptions compilerOptions;

    if(parseResult.HelpRequested) {
        printf("%s", HELP_STRING);
        return ParsedOptions(0); // display help and bail out
    }

    if(parseResult.Opt.count("--tell-api-versions")) {
        PrintAPIVersions();
        return ParsedOptions(0); // display help and bail out
    }

    if(parseResult.PosArgs.size() < 1) {
        std::cerr << "Error: not enough arguments" << std::endl;
        printf("%s", HELP_STRING);
        return ParsedOptions(-1);
    }

    compilerOptions.PreprocessOnly = parseResult.Opt.count("-E");
    compilerOptions.DebugMode = parseResult.Opt.count("-g");

    for(const auto& opt_with_value : parseResult.OptWithValue)
    {
        if(opt_with_value.first == "-H" || opt_with_value.first == "--Headers")
        {
            std::string headers_str = opt_with_value.second.GetCStr();

            size_t pos = 0;
            std::string token;
            bool found_semi_colon = headers_str.find(';') != std::string::npos;
            size_t first_colon_dist = headers_str.find(':');
            bool found_colon = first_colon_dist != std::string::npos;

            if (found_semi_colon) {
                while ((pos = headers_str.find(';')) != std::string::npos) {
                    token = headers_str.substr(0, pos);
                    compilerOptions.HeaderFiles.push_back(token);
                    headers_str.erase(0, pos + 1);
                }
            }
            else if(found_colon && first_colon_dist > 2) {
                // this is a good guess it's not a drive letter colon, proceed to treat as separator
                while ((pos = headers_str.find(':')) != std::string::npos) {
                    token = headers_str.substr(0, pos);
                    compilerOptions.HeaderFiles.push_back(token);
                    headers_str.erase(0, pos + 1);
                }
            }

            compilerOptions.HeaderFiles.push_back(headers_str);
            continue;
        }

        if(opt_with_value.first == "-D")  // defines a new macro
        {
            std::string macro_str = opt_with_value.second.GetCStr();
            auto equal_sign_pos = macro_str.find('=');
            bool has_equal_sign = equal_sign_pos != std::string::npos;
            std::string macro_name = macro_str;
            if(has_equal_sign) macro_name = macro_str.substr(0, equal_sign_pos);
            std::string macro_value = has_equal_sign ? macro_str.substr(equal_sign_pos + 1, macro_str.length()): "1";
            compilerOptions.Macros.emplace_back(macro_name, macro_value);
            continue;
        }

        if(opt_with_value.first == "-A")
        {
            if(!IsApiVersionValid(opt_with_value.second)){
                std::cerr << "Error: invalid API version " << opt_with_value.second.GetCStr() << std::endl;
                PrintAPIVersions();
                return ParsedOptions(-1);
            }
            compilerOptions.ScriptAPI.ScriptAPIVersion = opt_with_value.second.GetCStr();
            continue;
        }

        if(opt_with_value.first == "-C")
        {
            if(!IsApiVersionValid(opt_with_value.second)){
                std::cerr << "Error: invalid API version " << opt_with_value.second.GetCStr() << std::endl;
                PrintAPIVersions();
                return ParsedOptions(-1);
            }
            compilerOptions.ScriptAPI.ScriptCompatLevel = opt_with_value.second.GetCStr();
            continue;
        }

        if(opt_with_value.first == "-o" || opt_with_value.first == "--output")
        {
            compilerOptions.OutputObjFile = opt_with_value.second.GetCStr();
            continue;
        }

        if(opt_with_value.first == "--override-version")
        {
            compilerOptions.Version = opt_with_value.second.GetCStr();
            continue;
        }

        if(opt_with_value.first == "-f") // compiler flag
        {
            std::string flag_str = opt_with_value.second.GetCStr();
            auto equal_sign_pos = flag_str.find('=');
            bool has_equal_sign = equal_sign_pos != std::string::npos;
            std::string flag_name = flag_str;
            bool flag_value = true;
            if(has_equal_sign) {
                flag_name = flag_str.substr(0, equal_sign_pos);
                std::string flag_val_str = flag_str.substr(equal_sign_pos + 1, flag_str.length());
                flag_value = !(flag_val_str == "0");
            }

            if(flag_name == "showwarnings") {
                compilerOptions.Flags.ShowWarnings = flag_value;
                continue;
            }
            if(flag_name == "exportall") {
                compilerOptions.Flags.ExportAll = flag_value;
                continue;
            }
            if(flag_name == "linenumbers") {
                compilerOptions.Flags.LineNumbers = flag_value;
                continue;
            }
            if(flag_name == "autoimport") {
                compilerOptions.Flags.AutoImport = flag_value;
                continue;
            }
            if(flag_name == "debugrun") {
                compilerOptions.Flags.DebugRun = flag_value;
                continue;
            }
            if(flag_name == "noimportoverride") {
                compilerOptions.Flags.NoImportOverride = flag_value;
                continue;
            }
            if(flag_name == "forceobjectbasedscript") {
                compilerOptions.Flags.EnforceObjectBasedScript = flag_value;
                continue;
            }
            if(flag_name == "lefttoright") {
                printf("Warning: lefttoright flag is deprecated\n");
                continue;
            }
            if(flag_name == "forcenewstrings") {
                compilerOptions.Flags.EnforceNewStrings = flag_value;
                continue;
            }
            if(flag_name == "forcenewaudio") {
                compilerOptions.Flags.EnforceNewAudio = flag_value;
                continue;
            }
            if(flag_name == "oldcustomdialogopt") {
                compilerOptions.Flags.UseOldCustomDialogOptionsAPI = flag_value;
                continue;
            }
        }
    }

    compilerOptions.InputScriptFile = parseResult.PosArgs[0].GetCStr();

    if(compilerOptions.OutputObjFile.empty()) {
        // no output file explicitly set, let's use input.o instead
        std::string filename = Path::RemoveExtension(compilerOptions.InputScriptFile.c_str()).GetCStr();
        compilerOptions.OutputObjFile = filename + ".o";
    }

    if(compilerOptions.Version.empty()) {
        compilerOptions.Version = ACI_VERSION_STR;
    }

    ParsedOptions parsedOptions;
    parsedOptions.Options = compilerOptions;
    return parsedOptions;
}


int main(int argc, char* argv[])
{
    CompilerOptions compilerOptions;
    printf(R"EOS(agscc v0.1.0 - A Compiler for AGS Script
Copyright (c) 2021 AGS Team and contributors
)EOS"
    );

    ParseResult parseResult = Parse(argc,argv,{"-D", "-H", "--Headers", "-A", "-C", "-f"});
    ParsedOptions parsedOptions = parser_to_compiler_opts(parseResult);

    if(parsedOptions.Exit) return parsedOptions.ErrorCode;

    compilerOptions = parsedOptions.Options;
    return Compile(compilerOptions);
}
