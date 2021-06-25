//-----------------------------------------------------------------------//
// TODO:
// * append cmdline option (create new file / append to existing)
// * support multiple lib parts
//-----------------------------------------------------------------------//
#include <algorithm>
#include <stdio.h>
#include "data/mfl_utils.h"
#include "util/file.h"
#include "util/multifilelib.h"
#include "util/stdio_compat.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

const char *HELP_STRING = "Usage: agspak <input-dir> <output-pak>";

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
    // TODO: support multiple lib parts
    //-----------------------------------------------------------------------//
    String asset_dir = src;
    String lib_basefile = dst;

    AssetLibInfo lib;
    HError err = MakeAssetLibInfo(lib, asset_dir, lib_basefile);
    if (!err)
    {
        printf("Error: failed to gather list of assets:\n");
        printf("%s\n", err->FullMessage().GetCStr());
    }

    //-----------------------------------------------------------------------//
    // Write pack file
    //-----------------------------------------------------------------------//
    err = WriteLibrary(lib, asset_dir, lib_basefile, MFLUtil::kMFLVersion_MultiV30, 0);
    if (!err)
    {
        printf("Error: failed to write pack file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
    }
    printf("Pack file written successfully.\nDone.\n");
    return 0;
}
