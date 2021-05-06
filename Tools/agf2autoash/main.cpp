#include <stdio.h>
#include <vector>
#include "data/agfreader.h"
#include "data/game_utils.h"
#include "data/scriptgen.h"
#include "util/file.h"
#include "util/stream.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;


const char *HELP_STRING = "Usage: agf2autoash <input-game.agf> <output-auto.ash>\n";

int main(int argc, char *argv[])
{
    printf("agf2autoash v0.1.0 - AGS game's auto script header generator\n"\
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
    printf("Input game AGF: %s\n", src);
    printf("Output script header: %s\n", dst);

    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//
    AGF::AGFReader reader;
    HError err = reader.Open(src);
    if (!err)
    {
        printf("Error: failed to open source AGF:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    GameRef game_ref;
    AGF::ReadGameRef(game_ref, reader);

    //-----------------------------------------------------------------------//
    // Create script header
    //-----------------------------------------------------------------------//
    String header = MakeGameAutoScriptHeader(game_ref);

    //-----------------------------------------------------------------------//
    // Write script header
    //-----------------------------------------------------------------------//
    Stream *out = File::CreateFile(dst);
    if (!out)
    {
        printf("Error: failed to open script header for writing.\n");
        return -1;
    }
    out->Write(header.GetCStr(), header.GetLength());
    delete out;
    printf("Script header written successfully.\nDone.\n");
    return 0;
}
