#include <algorithm>
#include <stdio.h>
#include "data/mfl_utils.h"
#include "util/file.h"
#include "util/multifilelib.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

const char *HELP_STRING = "Usage: agsunpak <input-pak> <output-dir>";

int main(int argc, char *argv[])
{
    printf("agsunpak v0.1.0 - AGS game un-packing tool\n"\
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
    printf("Input pack file: %s\n", src);
    printf("Output directory: %s\n", dst);

    if (!ags_directory_exists(dst))
    {
        printf("Error: not a valid output directory.\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Read the library TOC
    //-----------------------------------------------------------------------//
    Stream *in = File::OpenFileRead(src);
    if (!in)
    {
        printf("Error: failed to open pack file for reading.\n");
        return -1;
    }

    // TODO: pick this out into a utility function that inits the lib fully
    AssetLibInfo lib;
    MFLUtil::MFLError mfl_err = MFLUtil::ReadHeader(lib, in);
    delete in;
    if (mfl_err != MFLUtil::kMFLNoError)
    {
        printf("Error: failed to parse pack file:\n");
        printf("%s\n", MFLUtil::GetMFLErrorText(mfl_err).GetCStr());
        return -1;
    }
    if (lib.AssetInfos.size() == 0)
    {
        printf("Pack file has no assets.\nDone.\n");
        return 0;
    }
    
    //-----------------------------------------------------------------------//
    // Extract files
    //-----------------------------------------------------------------------//
    String lib_basefile = Path::GetFilename(src);
    String lib_dir = Path::GetParent(src);
    // Replace the file name of the first library part to an actual source
    // file we just opened, because it may be different from the name
    // saved in lib; e.g. if the lib was attached to *.exe.
    lib.LibFileNames[0] = lib_basefile;
    HError err = UnpackLibrary(lib, lib_dir, dst);
    if (!err)
    {
        printf("Failed unpacking the library\n%s", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Done.\n");
    return 0;
}
