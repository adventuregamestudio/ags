//-----------------------------------------------------------------------//
// TODO:
// * append cmdline option (create new file / append to existing)
// * option for full recursive subdirs?
// * option for explicit file list
//-----------------------------------------------------------------------//
#include <algorithm>
#include <stdio.h>
#include "data/mfl_utils.h"
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
"  -p <MB>        split game assets between partitions of this size max\n"
"  -r             recursive mode: include all subdirectories too";

int main(int argc, char *argv[])
{
    printf("agspak v0.1.0 - AGS game packaging tool\n"\
        "Copyright (c) 2024 AGS Team and contributors\n");
    ParseResult parseResult = Parse(argc,argv,{"-p"});
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

    bool do_subdirs = parseResult.Opt.count("-r");
    size_t part_size = 0;
    for (const auto& opt_with_value : parseResult.OptWithValue)
    {
        if (opt_with_value.first == "-p")
        {
            part_size = StrUtil::StringToInt(opt_with_value.second);
        }
    }

    const String src = parseResult.PosArgs[0];
    const String dst = parseResult.PosArgs[1];
    printf("Input directory: %s\n", src.GetCStr());
    printf("Output pack file: %s\n", dst.GetCStr());

    if (!ags_directory_exists(src.GetCStr()))
    {
        printf("Error: not a valid input directory.\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Gather list of files and set up library info
    //-----------------------------------------------------------------------//
    const String asset_dir = src;
    const String lib_basefile = dst;

    std::vector<AssetInfo> assets;
    HError err = MakeAssetList(assets, asset_dir, do_subdirs, lib_basefile);
    if (!err)
    {
        printf("Error: failed to gather list of assets:\n");
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
    err = WriteLibrary(lib, asset_dir, lib_dir, MFLUtil::kMFLVersion_MultiV30);
    if (!err)
    {
        printf("Error: failed to write pack file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Pack file(s) written successfully.\nDone.\n");
    return 0;
}
