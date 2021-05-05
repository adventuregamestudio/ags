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


const char *HELP_STRING = "Usage: agf2glvar <in-game.agf> <out-glvar.ash> <out-glvar.asc>\n";

int main(int argc, char *argv[])
{
    printf("agf2glvar v0.1.0 - AGS game's global variables script generator\n"\
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
    if (argc < 4)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    const char *src = argv[1];
    const char *dst_header = argv[2];
    const char *dst_body = argv[3];
    printf("Input game AGF: %s\n", src);
    printf("Output script header: %s\n", dst_header);
    printf("Output script body: %s\n", dst_body);

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

    std::vector<Variable> vars;
    AGF::ReadGlobalVariables(vars, reader.GetGameRoot());

    //-----------------------------------------------------------------------//
    // Create script header & body with global variables
    //-----------------------------------------------------------------------//
    String header = MakeVariablesScriptHeader(vars);
    String body = MakeVariablesScriptBody(vars);

    //-----------------------------------------------------------------------//
    // Write script header & body
    //-----------------------------------------------------------------------//
    Stream *out = File::CreateFile(dst_header);
    if (!out)
    {
        printf("Error: failed to open script header for writing.\n");
        return -1;
    }
    out->Write(header.GetCStr(), header.GetLength());
    delete out;
    printf("Script header written successfully.\n");
    out = File::CreateFile(dst_body);
    if (!out)
    {
        printf("Error: failed to open script body for writing.\n");
        return -1;
    }
    out->Write(body.GetCStr(), body.GetLength());
    delete out;
    printf("Script body written successfully.\nDone.\n");
    return 0;
}
