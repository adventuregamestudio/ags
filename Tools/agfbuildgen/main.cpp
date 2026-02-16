//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <iostream>
#include <algorithm>
#include "core/platform.h"
#include "util/path.h"
#include "util/cmdlineopts.h"
#include "data/agfreader.h"
#include "util/string_compat.h"
#include "generator_common.h"
#include "ninja_generator.h"
#include "makefile_generator.h"
#include "util/file.h"
#include "util/directory.h"
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#endif

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;

const char *HELP_STRING = ""
"Usage: agfbuildgen [OPTIONS] <GAME.AGF> <OUTPUT_DIR>\n"
"\n"
"        Generate build files in OUTPUT_DIR for an AGS GAME.AGF project.\n"
"\n"
"Options:\n"
"-f, --format FORMAT      Specify the build format to generate:\n"
"                         ninja, makefile.\n"
"                         Default: ninja\n"
"-v, --verbose            Enable verbose output for debugging.\n"
"-h, --help               Show this help message and exit.\n"
"";

enum Generator {
    kNinja,
    kMakefile
};

struct ParsedOptions {
    Generator Gen = Generator::kNinja;
    String GameAgf;
    String OutputDir {};
    bool Verbose = false;
    bool Exit = false;
    int ErrorCode = 0;
    ParsedOptions() = default;
    explicit ParsedOptions(int error_code) { Exit = true; ErrorCode = error_code; }
};

ParsedOptions parser_to_gen_opts(const ParseResult& parseResult)
{
    if(parseResult.HelpRequested) {
        printf("%s", HELP_STRING);
        return ParsedOptions(0); // display help and bail out
    }

    if(parseResult.PosArgs.size() < 2) {
        std::cerr << "Error: not enough arguments" << std::endl;
        printf("%s", HELP_STRING);
        return ParsedOptions(-1);
    }

    ParsedOptions parsedOptions;

    for(const auto& opt_with_value : parseResult.OptWithValue)
    {
        if(opt_with_value.first == "-f" || opt_with_value.first == "--format")
        {
            const char * format = opt_with_value.second.GetCStr();
            if(ags_stricmp(format, "makefile") == 0) parsedOptions.Gen = Generator::kMakefile;
            else if(ags_stricmp(format, "ninja") == 0) parsedOptions.Gen = Generator::kNinja;
        }
    }

    parsedOptions.GameAgf = parseResult.PosArgs[0];
    parsedOptions.OutputDir = parseResult.PosArgs[1];
    parsedOptions.Verbose = parseResult.Opt.count("-v") || parseResult.Opt.count("--verbose");

    return parsedOptions;
}


// instead of building allegro in libtool because of canonicalize filenames, we are making a simpler AbsPath equivalent,
// but it only works in files and dirs that actually exist
// picked these from https://stackoverflow.com/questions/229012/getting-absolute-path-of-a-file
#ifdef _WIN32
String ExistingPathAsAbsolute(const String &path) {
    if (path == nullptr) {
        return String();
    }

    char absPath[MAX_PATH];
    DWORD result = GetFullPathNameA(path.GetCStr(), MAX_PATH, absPath, nullptr);
    if (result == 0) {
        return String(); // Failed to get absolute path
    }
    return String(absPath);
}
#else
    // Unix (Linux or macOS)
String ExistingPathAsAbsolute(const String &path) {
    if (path == nullptr) {
        return String();
    }

    char absPath[PATH_MAX];
    if (realpath(path.GetCStr(), absPath) == nullptr) {
        return String(); // Failed to get absolute path
    }
    return String(absPath);
}
#endif

// TO-DO: make this return errors as needed
void fill_options_from_project(GeneratorOptions& opt, const AGF::AGFReader &reader)
{
    AGF::ReadScriptList(opt.ScriptFileList, reader.GetGameRoot());
    AGF::ReadScriptHeaderList(opt.HeaderFileList, reader.GetGameRoot());

    std::vector<int> rooms;
    std::vector<std::pair<int, String>> rooms_dsc;
    AGF::ReadRoomList(rooms_dsc, reader.GetGameRoot());
    rooms.reserve(rooms_dsc.size());
    opt.RoomFileList.reserve(rooms_dsc.size());
    for (const auto &rd: rooms_dsc)
        rooms.push_back(rd.first);

    std::sort(rooms.begin(), rooms.end());
    for (const auto &r: rooms)
        opt.RoomFileList.push_back(String::FromFormat("room%d.crm", r));

    opt.HasDialogScripts = false;
}

bool tool_exists_in_path(const char* tool_path)
{
    String tool_exe = String::FromFormat("%s.exe", tool_path);
    if(!(File::IsFile(tool_path) || File::IsFile(tool_exe)))
    {
        printf("Tool '%s' not found", tool_path);
        return false;
    }
    return true;
}

int main(const int argc, const char* const argv[])
{
    printf(
        "agfbuildgen v0.1.0 - A Build Generator for AGF Project Files\n"
        "Copyright (c) 2025 AGS Team and contributors\n"
    );

    //-----------------------------------------------------------------------//
    // Parse input parameters
    //-----------------------------------------------------------------------//

    ParseResult parseResult = Parse(argc,argv,{"-f", "--format"});
    ParsedOptions parsedOptions = parser_to_gen_opts(parseResult);

    if(parsedOptions.Exit) return parsedOptions.ErrorCode;

    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//
    AGF::AGFReader reader;
    HError err = reader.Open(parsedOptions.GameAgf.GetCStr());
    if (!err)
    {
        printf("Error: failed to open source AGF:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    GeneratorOptions opt;

    opt.OutputDir = ExistingPathAsAbsolute(parsedOptions.OutputDir); // we never mkdir this
    opt.GameProjectFile = ExistingPathAsAbsolute(parsedOptions.GameAgf);
    opt.GameProjectDir = Path::GetDirectoryPath(opt.GameProjectFile);
    opt.TempDir = Path::ConcatPaths(opt.GameProjectDir, "temp"); // need to mkdir somewhere...
    // we don't delete the temp after or we will have to rebuild the dependencies everytime.
    // maybe call this a different name?

    // all the entries below should be the default full paths
    // but I don't know any easy way to get them...
    // ideally it would look in the path THIS executable is (assuming there is a Tools dir)
    // but C/C++11 doesn't have an easy api for this... (for Windows/Linux/macOS...), only in C++20
    opt.AgsDefnsFile = "agsdefns.sh"; // need to figure a way to get this?
    opt.ToolAgspak = "agspak"; // I think the exe extension here is optional
    opt.ToolTrac = "trac";
    opt.ToolAgfexport = "agfexport";
    opt.ToolAgf2dlgasc = "agf2dlasc";
    opt.ToolAgscc = "agscc";
    opt.ToolCrm2ash = "crm2ash";

    if(!File::IsFile(opt.GameProjectFile))
    {
        printf("Game project file '%s' doesn't exist", opt.GameProjectFile.GetCStr());
        return -1;
    }
    if(!File::IsDirectory(opt.OutputDir))
    {
        printf("Output directory '%s' doesn't exist", opt.OutputDir.GetCStr());
        return -1;
    }
    if(!File::IsFile(opt.GameProjectFile))
    {
        printf("Game project file '%s' doesn't exist", opt.GameProjectFile.GetCStr());
        return -1;
    }
    // skipping check for opt.AgsDefnsFile for now ...
    if(!tool_exists_in_path(opt.ToolAgspak.GetCStr()))
        return -1;
    if(!tool_exists_in_path(opt.ToolTrac.GetCStr()))
        return -1;
    if(!tool_exists_in_path(opt.ToolAgfexport.GetCStr()))
        return -1;
    if(!tool_exists_in_path(opt.ToolAgf2dlgasc.GetCStr()))
        return -1;
    if(!tool_exists_in_path(opt.ToolAgscc.GetCStr()))
        return -1;
    if(!tool_exists_in_path(opt.ToolCrm2ash.GetCStr()))
        return -1;

    fill_options_from_project(opt, reader);

    // To build an AGS game from the command line you need to use some tools that generate some intermediary files
    // (like headers and script files), these which you would like to put in some temp dir.
    // Now all the tools if you pass a dir that doesn't exist, they won't create the directory and put the file inside,
    // they will error because the directory doesn't exist.
    // So the solution is simple, you create any necessary dir to store intermediate files before you need them.
    // Of course doing this manually everytime may be annoying...
    // But unfortunately make and ninja use timestamp to check if a dir was modified to trigger a rebuild of the rest
    // so we can't have a directory as build dependency - or things would rebuild forever.
    // We also can't put a dummy file and depend on that since there isn't a cross platform solution that wokrs in any
    // shell/terminal environment.
    // A good solution would be to have our own little minimalist ags busybox with minimal mkdir and touch support,
    // and perhaps more as needed. For now, I have to do this here.
    Directory::CreateDirectory(opt.TempDir);

    //-----------------------------------------------------------------------//
    // Generate build file
    //-----------------------------------------------------------------------//

    switch (parsedOptions.Gen) {
        case kNinja:
            opt.OutputFile = Path::ConcatPaths(opt.OutputDir, "build.ninja");
            NinjaGenerator::GenerateNinjaBuild(opt);
            break;
        case kMakefile:
            opt.OutputFile = Path::ConcatPaths(opt.OutputDir, "Makefile");
            MakefileGenerator::GenerateMakefile(opt);
            break;
    }

    return 0;
}

// a few notes here for now
// the idea here is to take a game dir as input and then a second parameter as the build dir
// the generated either Makefile or ninja.build will then be put into the build dir
// I am mostly thinking to default to ninja but be able to take something like `-G make` or `-G ninja` (CMake like)
// other than this I think that less is more here
// so as an example you would run like:
//
// agfbuildgen gamedir/Game.agf gamedir/out && cd gamedir/out && ninja
//
// and then the game would build
//
// NOTE: ninja has a cool thing that I can copy the initial command and put it in ninja itself to make its file self
// regenerable (like, make agfbuildgen run as the first call from ninja sequence)
//
// NOTE2: it may probably default to think it's dir (the dir where the agfbuildgen binary is) is the same as the other
// tools but also take a parameter so you can pass the dir where the ags Tools are.

// big issue is things like copy mv and the like are not standard across systems (in Windows, it may be many different terminals!)
// so they should be avoided at all costs in the builds
// ideally a small "agsbusybox" with little implementation of them as commands
// could make this a lot easier to workout
// it would be super nice to have if needed