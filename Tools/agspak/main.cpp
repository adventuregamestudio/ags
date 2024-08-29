//-----------------------------------------------------------------------//
// TODO:
// * append cmdline option (create new file / append to existing)
// * option for full recursive subdirs?
// * option for explicit file list
//-----------------------------------------------------------------------//
#include <algorithm>
#include <stdio.h>
#include "data/mfl_utils.h"
#include "data/include_utils.h"
#include "util/cmdlineopts.h"
#include "util/file.h"
#include "util/multifilelib.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;
using namespace AGS::DataUtil;

const char *HELP_STRING = "Usage: agspak <input-dir> <output-pak> [OPTIONS]\n"
"Options:\n"
"  -f, --pattern-file <F> set the name F of the file with the include patterns\n"
"  -p <MB>                split game assets between partitions of this size max\n"
"  -r                     recursive mode: include all subdirectories too\n"
"  -v, --verbose          prints packaged files";

int main(int argc, char *argv[])
{
    printf("agspak v0.2.0 - AGS game packaging tool\n"\
        "Copyright (c) 2024 AGS Team and contributors\n");
    ParseResult parseResult = Parse(argc,argv,{"-p", "-f", "--pattern-file"});
    if (parseResult.HelpRequested)
    {
        printf("%s\n", HELP_STRING);
        return 0; // display help and bail out
    }
    if (parseResult.PosArgs.size() < 2)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    // a include pattern file that should be inside the input-dir
    // TO-DO: support nested include pattern files in input-dir
    bool has_include_pattern_file = false;
    String include_pattern_file_name;

    bool verbose = parseResult.Opt.count("-v") || parseResult.Opt.count("--verbose");
    bool do_subdirs = parseResult.Opt.count("-r");
    size_t part_size = 0;
    for (const auto &opt_with_value : parseResult.OptWithValue)
    {
        if (opt_with_value.first == "-p")
        {
            part_size = StrUtil::StringToInt(opt_with_value.second);
        }
        else if (opt_with_value.first == "-f" || opt_with_value.first == "--pattern-file")
        {
            has_include_pattern_file = true;
            include_pattern_file_name = opt_with_value.second;
        }
    }

    const String &src = parseResult.PosArgs[0];
    const String &dst = parseResult.PosArgs[1];
    printf("Input directory: %s\n", src.GetCStr());
    printf("Output pack file: %s\n", dst.GetCStr());
    if (has_include_pattern_file)
        printf("Pattern file name: %s\n", include_pattern_file_name.GetCStr());

    if (!ags_directory_exists(src.GetCStr()))
    {
        printf("Error: not a valid input directory.\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Gather list of files and set up library info
    //-----------------------------------------------------------------------//
    const String &asset_dir = src;
    const String &lib_basefile = dst;

    std::vector<String> files;
    HError err = MakeListOfFiles(files, asset_dir, do_subdirs);
    if (!err)
    {
        printf("Error: failed to gather list of files:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    if (has_include_pattern_file)
    {
        std::vector<String> output_files;
        err = IncludeFiles(files, output_files, asset_dir, include_pattern_file_name, verbose);
        if (!err)
        {
            printf("Error: failed to processes %s file:\n", include_pattern_file_name.GetCStr());
            printf("%s\n", err->FullMessage().GetCStr());
            return -1;
        }

        files = std::move(output_files);
    }

    std::vector<AssetInfo> assets;
    err = MakeAssetListFromFileList(files, assets, asset_dir);
    if (!err)
    {
        printf("Error: failed to prepare list of assets:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    if (assets.size() == 0)
    {
        printf("No valid assets found in the provided directory.\nDone.\n");
        return 0;
    }

    AssetLibInfo lib;
    soff_t part_size_b = part_size * 1024 * 1024; // MB to bytes
    err = MakeAssetLib(lib, lib_basefile, assets, part_size_b);
    if (!err)
    {
        printf("Error: failed to configure asset library:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write pack file
    //-----------------------------------------------------------------------//
    String lib_dir = Path::GetParent(lib_basefile);
    err = WriteLibrary(lib, asset_dir, lib_dir, MFLUtil::kMFLVersion_MultiV30, verbose);
    if (!err)
    {
        printf("Error: failed to write pack file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Pack file(s) written successfully.\nDone.\n");
    return 0;
}
