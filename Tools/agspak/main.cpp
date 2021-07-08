//-----------------------------------------------------------------------//
// TODO:
// * append cmdline option (create new file / append to existing)
//-----------------------------------------------------------------------//
#include <algorithm>
#include <stdio.h>
#include "data/mfl_utils.h"
#include "util/file.h"
#include "util/multifilelib.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

const char *HELP_STRING = "Usage: agspak <input-dir> <output-pak>\n\t[-p <max MB per part>]";

int main(int argc, char *argv[])
{
    printf("agspak v0.1.0 - AGS game packaging tool\n"\
        "Copyright (c) 2021 AGS Team and contributors\n");
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (ags_stricmp(arg, "--help") == 0 || ags_stricmp(arg, "/?") == 0 || ags_stricmp(arg, "-?") == 0)
        {
            printf("%s\n", HELP_STRING);
            return 0; // display help and bail out
        }
    }
    if (argc < 3)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    size_t part_size = 0;
    for (int i = 3; i < argc; ++i)
    {
        if (ags_stricmp(argv[i], "-p") == 0 && (i < argc - 1))
            part_size = StrUtil::StringToInt(argv[++i]);
    }

    const char *src = argv[1];
    const char *dst = argv[2];
    printf("Input directory: %s\n", src);
    printf("Output pack file: %s\n", dst);

    if (!ags_directory_exists(src))
    {
        printf("Error: not a valid input directory.\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Gather list of files and set up library info
    //-----------------------------------------------------------------------//
    String asset_dir = src;
    String lib_basefile = dst;

    std::vector<AssetInfo> assets;
    HError err = MakeAssetList(assets, asset_dir, lib_basefile);
    if (!err)
    {
        printf("Error: failed to gather list of assets:\n");
        printf("%s\n", err->FullMessage().GetCStr());
    }

    AssetLibInfo lib;
    soff_t part_size_b = part_size * 1024 * 1024; // MB to bytes
    err = MakeAssetLib(lib, lib_basefile, assets, part_size_b);
    if (!err)
    {
        printf("Error: failed to configure asset library:\n");
        printf("%s\n", err->FullMessage().GetCStr());
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
    }
    printf("Pack file(s) written successfully.\nDone.\n");
    return 0;
}
