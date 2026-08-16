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
#include <vector>
#include "platform/platform.h"
#include "util/path.h"
#include "util/cmdlineopts.h"
#include "data/agfreader.h"
#include "util/string_compat.h"
#include "generator_common.h"
#include "ninja_generator.h"
#include "makefile_generator.h"
#include "util/file.h"
#include "util/directory.h"
#include "util/textstreamwriter.h"
#if AGS_PLATFORM_OS_WINDOWS
#include "platform/windows/windows.h"
#elif AGS_PLATFORM_OS_MACOS
#include <mach-o/dyld.h>
#elif AGS_PLATFORM_OS_LINUX
#include <unistd.h>
#elif AGS_PLATFORM_OS_FREEBSD
#include <sys/types.h>
#include <sys/sysctl.h>
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
"-t, --tools-dir DIR      Directory containing the AGS command-line tools.\n"
"                         Default: directory containing agfbuildgen.\n"
"-v, --verbose            Enable verbose output for debugging.\n"
"-h, --help               Show this help message and exit.\n"
"";

enum Generator
{
    kNinja,
    kMakefile
};

struct ParsedOptions
{
    Generator Gen = Generator::kNinja;
    String GameAgf;
    String OutputDir {};
    String ToolsDir {};
    bool Verbose = false;
    bool Exit = false;
    int ErrorCode = 0;
    ParsedOptions() = default;
    explicit ParsedOptions(int error_code) { Exit = true; ErrorCode = error_code; }
};

// retrieves THIS executable path
// below code is hugely based on https://github.com/DanielGibson/Snippets/blob/7bad19703feb1cc393fda4438c8415889cccb1c6/DG_misc.h#L293
// but later adapted to AGS and readjusted using Codex GPT-5.6-Sol
String get_this_executable_path()
{
#if AGS_PLATFORM_OS_WINDOWS
    std::vector<wchar_t> path_buf(256u);
    for (;;)
    {
        const DWORD path_len = GetModuleFileNameW(nullptr, path_buf.data(), static_cast<DWORD>(path_buf.size()));
        if (path_len == 0u)
            return {};
        if (path_len < path_buf.size())
            return Path::WidePathToUTF8(path_buf.data());
        path_buf.resize(path_buf.size() * 2u);
    }
#elif AGS_PLATFORM_OS_MACOS
    uint32_t path_size = 0u;
    _NSGetExecutablePath(nullptr, &path_size);
    if (path_size == 0u)
        return {};

    std::vector<char> path_buf(path_size);
    if (_NSGetExecutablePath(path_buf.data(), &path_size) != 0)
        return {};
    return Path::MakeAbsolutePath(path_buf.data());
#elif AGS_PLATFORM_OS_LINUX
    std::vector<char> path_buf(256u);
    for (;;)
    {
        const ssize_t path_len = readlink("/proc/self/exe", path_buf.data(), path_buf.size());
        if (path_len < 0)
            return {};
        if (static_cast<size_t>(path_len) < path_buf.size())
            return String(path_buf.data(), static_cast<size_t>(path_len));
        path_buf.resize(path_buf.size() * 2u);
    }
#elif AGS_PLATFORM_OS_FREEBSD
    int mib[] = {CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1};
    const size_t mib_size = sizeof(mib) / sizeof(mib[0]);
    size_t path_size = 0u;
    if (sysctl(mib, mib_size, nullptr, &path_size, nullptr, 0u) != 0 || path_size == 0u)
        return {};

    std::vector<char> path_buf(path_size);
    if (sysctl(mib, mib_size, path_buf.data(), &path_size, nullptr, 0u) != 0)
        return {};
    return path_buf.data();
#else
    // Should we #error "Unsupported Platform!" ?
    return {};
#endif
}

ParsedOptions parser_to_gen_opts(const ParseResult& parseResult)
{
    if (parseResult.HelpRequested)
    {
        printf("%s", HELP_STRING);
        return ParsedOptions(0); // display help and bail out
    }

    if (parseResult.PosArgs.size() < 2)
    {
        std::cerr << "Error: not enough arguments" << std::endl;
        printf("%s", HELP_STRING);
        return ParsedOptions(-1);
    }

    ParsedOptions parsedOptions;

    for(const auto& opt_with_value : parseResult.OptWithValue)
    {
        if (opt_with_value.first == "-f" || opt_with_value.first == "--format")
        {
            const char * format = opt_with_value.second.GetCStr();
            if(ags_stricmp(format, "makefile") == 0) parsedOptions.Gen = Generator::kMakefile;
            else if(ags_stricmp(format, "ninja") == 0) parsedOptions.Gen = Generator::kNinja;
            else
            {
                printf("Error: unsupported build format '%s'\n", format);
                return ParsedOptions(-1);
            }
        }
        else if (opt_with_value.first == "-t" || opt_with_value.first == "--tools-dir")
        {
            parsedOptions.ToolsDir = opt_with_value.second;
        }
    }

    parsedOptions.GameAgf = parseResult.PosArgs[0];
    parsedOptions.OutputDir = parseResult.PosArgs[1];
    parsedOptions.Verbose = parseResult.Opt.count("-v") || parseResult.Opt.count("--verbose");

    return parsedOptions;
}

// TO-DO: make this return errors as needed
void fill_options_from_project(GeneratorOptions& opt, const AGF::AGFReader &reader)
{
    // we assume these two have matching index for script/header pairs
    // if somehow a game is written by hand and not using the editor
    // it is possible to have a script without a header, which would break things weirdly
    // FIX-ME: possibly a size check, while fragile could help verify if both vectors have the same size?
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

    std::vector<String> trs_names;
    AGF::ReadTranslationList(trs_names, reader.GetGameRoot());
    for (const auto& tn : trs_names)
        opt.TrsFileList.push_back(String::FromFormat("%s.trs", tn.GetCStr()));

    std::vector<int> dialogs;
    AGF::ReadDialogList(dialogs, reader.GetGameRoot());
    opt.HasDialogScripts = !dialogs.empty();

    // AGS 3 stores font IDs rather than filenames.
    // Both variants must be packaged when present.
    // This should be simpler in AGS 4!
    std::vector<int> font_indexes;
    AGF::ReadFontList(font_indexes, reader.GetGameRoot());
    for (const auto &font_index : font_indexes)
    {
        const String ttf_name = String::FromFormat("agsfnt%d.ttf", font_index);
        const String wfn_name = String::FromFormat("agsfnt%d.wfn", font_index);
        if (File::IsFile(Path::ConcatPaths(opt.GameProjectDir, ttf_name)))
            opt.FontFileList.push_back(ttf_name);
        if (File::IsFile(Path::ConcatPaths(opt.GameProjectDir, wfn_name)))
            opt.FontFileList.push_back(wfn_name);
    }

    std::vector<String> custom_dirs;
    AGF::ReadCustomDataDirectories(custom_dirs, reader.GetGameRoot());
    for (String custom_dir : custom_dirs)
    {
        custom_dir.Trim();
        if (custom_dir.IsEmpty() || !Path::IsRelativePath(custom_dir))
            continue;

        const String custom_dir_abs = Path::ConcatPaths(opt.GameProjectDir, custom_dir);
        if (Path::ComparePaths(opt.GameProjectDir, custom_dir_abs) == 0 ||
            !Path::IsSameOrSubDir(opt.GameProjectDir, custom_dir_abs) ||
            !File::IsDirectory(custom_dir_abs))
            continue;
        opt.CustomDataDirList.push_back(custom_dir);
    }

    String game_filename{};
    AGF::ReadGameFileName(game_filename, reader.GetGameRoot());
    if (game_filename.EndsWith(".ags"))
        game_filename.ClipRight(4);
    opt.GameFileName = String::FromFormat("%s.ags", game_filename.GetCStr());
}

// this is meant to check specific file path, not actually the PATH environment var.
// The original idea was in case it was possible to set each tool path or a tools dir
// through command line args...
// Should they instead use environment variables??
// NOTE: should I make an additional function for checking things in PATH env var??
bool tool_exists_in_path(const char* tool_path)
{
    String tool_exe = String::FromFormat("%s.exe", tool_path);
    if (!(File::IsFile(tool_path) || File::IsFile(tool_exe)))
        return false;
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

    ParseResult parseResult = Parse(argc,argv,{"-f", "--format", "-t", "--tools-dir"});
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

    opt.OutputDir = Path::MakeAbsolutePath(parsedOptions.OutputDir); // we never mkdir this
    opt.GameProjectFile = Path::MakeAbsolutePath(parsedOptions.GameAgf);
    opt.GameProjectDir = Path::GetDirectoryPath(opt.GameProjectFile);
    opt.TempDir = Path::ConcatPaths(opt.GameProjectDir, "temp"); // need to mkdir somewhere...
    // we don't delete the temp after or we will have to rebuild the dependencies everytime.
    // maybe call this a different name?

    if (!parsedOptions.ToolsDir.IsNullOrSpace())
    {
        opt.ToolsDir = Path::MakeAbsolutePath(parsedOptions.ToolsDir);
    }
    else
    {
        const String executable_path = get_this_executable_path();
        if (executable_path.IsNullOrSpace())
        {
            printf("Unable to determine agfbuildgen's executable path\n");
            return -1;
        }
        opt.ToolsDir = Path::MakeAbsolutePath(Path::GetParent(executable_path));
    }

    // NOTE: probably this may have to be treated differently later
    // for now let's copy this file wherever the tools are
    opt.AgsDefnsFile = Path::ConcatPaths(opt.ToolsDir, "agsdefns.sh");

    opt.ToolAgspak = Path::ConcatPaths(opt.ToolsDir, "agspak");
    opt.ToolTrac = Path::ConcatPaths(opt.ToolsDir, "trac");
    opt.ToolAgfexport = Path::ConcatPaths(opt.ToolsDir, "agfexport");
    opt.ToolAgf2dlgasc = Path::ConcatPaths(opt.ToolsDir, "agf2dlgasc");
    opt.ToolAgscc = Path::ConcatPaths(opt.ToolsDir, "agscc");
    opt.ToolCrmpak = Path::ConcatPaths(opt.ToolsDir, "crmpak");
    opt.ToolAgf2dta = Path::ConcatPaths(opt.ToolsDir, "agf2dta");

    if (!File::IsDirectory(opt.OutputDir))
    {
        printf("Output directory '%s' doesn't exist", opt.OutputDir.GetCStr());
        return -1;
    }
    if (!File::IsDirectory(opt.ToolsDir))
    {
        printf("Tools directory '%s' doesn't exist\n", opt.ToolsDir.GetCStr());
        return -1;
    }
    if (!File::IsDirectory(opt.GameProjectDir))
    {
        printf("Game project directory '%s' doesn't exist\n", opt.GameProjectDir.GetCStr());
        return -1;
    }
    if (!File::IsFile(opt.GameProjectFile))
    {
        printf("Game project file '%s' doesn't exist\n", opt.GameProjectFile.GetCStr());
        return -1;
    }
    if (!File::IsFile(opt.AgsDefnsFile))
    {
        printf("AGS Script API header file '%s' not found\n", opt.AgsDefnsFile.GetCStr());
        return -1;
    }

    if (!tool_exists_in_path(opt.ToolAgspak.GetCStr()))
    {
        printf("Required tool 'agspak' not found at '%s'\n", opt.ToolAgspak.GetCStr());
        return -1;
    }
    if (!tool_exists_in_path(opt.ToolTrac.GetCStr()))
    {
        printf("Required tool 'trac' not found at '%s'\n", opt.ToolTrac.GetCStr());
        return -1;
    }
    if (!tool_exists_in_path(opt.ToolAgfexport.GetCStr()))
    {
        printf("Required tool 'agfexport' not found at '%s'\n", opt.ToolAgfexport.GetCStr());
        return -1;
    }
    if (!tool_exists_in_path(opt.ToolAgf2dlgasc.GetCStr()))
    {
        printf("Required tool 'agf2dlgasc' not found at '%s'\n", opt.ToolAgf2dlgasc.GetCStr());
        return -1;
    }
    if (!tool_exists_in_path(opt.ToolAgscc.GetCStr()))
    {
        printf("Required tool 'agscc' not found at '%s'\n", opt.ToolAgscc.GetCStr());
        return -1;
    }
    if (!tool_exists_in_path(opt.ToolCrmpak.GetCStr()))
    {
        printf("Required tool 'crmpak' not found at '%s'\n", opt.ToolCrmpak.GetCStr());
        return -1;
    }
    if (!tool_exists_in_path(opt.ToolAgf2dta.GetCStr()))
    {
        printf("Required tool 'agf2dta' not found at '%s'\n", opt.ToolAgf2dta.GetCStr());
        return -1;
    }

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
    if (!Directory::CreateDirectory(opt.TempDir))
    {
        printf("Unable to create temporary build directory '%s'\n", opt.TempDir.GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write ScriptModules.lst
    //-----------------------------------------------------------------------//
    // see Engine/main/game_file.cpp, LoadGameScripts().
    // NOTE: I am unsure if this should really exist here, perhaps something else could generate it?
    {
        std::vector<String> sco_without_global;
        for (const auto& script : opt.ScriptFileList)
        {
            String scriptname = script.Left(script.GetLength() - 4); // Strip extension
            String sco = String::FromFormat("%s.o", scriptname.GetCStr());
            if (sco != "GlobalScript.o")
                sco_without_global.push_back(sco);
        }

        String list_file = Path::ConcatPaths(opt.TempDir, "ScriptModules.lst");
        auto lst_out = File::CreateFile(list_file);
        if (!lst_out)
        {
            printf("Error: unable to create '%s'.\n", list_file.GetCStr());
            return -1;
        }
        TextStreamWriter lst_writer(std::move(lst_out));
        for (const auto& sco : sco_without_global)
            lst_writer.WriteLine(sco);
        lst_writer.Flush();
    }

    //-----------------------------------------------------------------------//
    // Generate build file
    //-----------------------------------------------------------------------//

    String build_file;

    switch (parsedOptions.Gen)
    {
    case kNinja:
        build_file = Path::ConcatPaths(opt.OutputDir, "build.ninja");
        break;
    case kMakefile:
        build_file = Path::ConcatPaths(opt.OutputDir, "Makefile");
        break;
    }

    auto out = File::CreateFile(build_file);
    if (!out)
    {
        printf("Error: unable to create output file '%s'.\n", build_file.GetCStr());
        return -1;
    }

    switch (parsedOptions.Gen)
    {
    case kNinja:
        NinjaGenerator::GenerateNinjaBuild(opt, std::move(out));
        break;
    case kMakefile:
        MakefileGenerator::GenerateMakefile(opt, std::move(out));
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
