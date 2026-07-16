#include <algorithm>
#include <stdio.h>
#include "data/agfreader.h"
#include "data/data_file_writer.h"
#include "util/cmdlineopts.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;
using namespace AGS;

const char *HELP_STRING = "Usage: agf2dta <INPUT-GAME.AGF> <OUTPUT-DIR>\n"
"Creates a \"game28.dta\" file in the OUTPUT-DIR from data in INPUT-GAME.AGF. \n"
"Options:\n"
"  -h, --help             Show this help message\n";

int main(int argc, char *argv[])
{
    printf("agf2dta v0.1.0 - AGS game main data file build tool\n"\
        "Copyright (c) 2026 AGS Team and contributors\n");
    ParseResult parseResult = Parse(argc,argv,{});
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
	
    const String &src_agf = parseResult.PosArgs[0];
    const String &dst_dir = parseResult.PosArgs[1];
	
    printf("Input game AGF: %s\n", src_agf.GetCStr());
    printf("Output dir: %s\n", dst_dir.GetCStr());
	
    //-----------------------------------------------------------------------//
    // validate inputs
    //-----------------------------------------------------------------------//
    if (!File::IsFile(src_agf))
    {
        printf("Error: input game file '%s' not found.\n", src_agf.GetCStr());
        return -1;
    }

    if (!ags_directory_exists(dst_dir.GetCStr()))
    {
        printf("Error: not a valid output directory.\n");
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//
    AGF::AGFReader reader;
    const HError err = reader.Open(src_agf.GetCStr());
    if (!err)
    {
        printf("Error: failed to open source AGF:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    DataUtil::GameData game{};
    AGF::ReadGameData(game, reader);

    //-----------------------------------------------------------------------//
    // Set the main game data struct values from Game.agf
    //-----------------------------------------------------------------------//

    // TODO: Refactor to use this approach instead, for now we are just replicating DataFileWriter.cs approach instead

    //-----------------------------------------------------------------------//
    // Write main game data file to game28.dta
    //-----------------------------------------------------------------------//
    const String out_file = Path::ConcatPaths(dst_dir, "game28.dta");
    std::unique_ptr<Stream> out = File::CreateFile(out_file);
    if (!out)
    {
        printf("Error: unable to create output file '%s'.\n", out_file.GetCStr());
        return -1;
    }

    HError write_err = DataUtil::WriteGameData28(game, std::move(out));
    if (!write_err)
    {
        printf("Error: failed to write game data:\n");
        printf("%s\n", write_err->FullMessage().GetCStr());
        return -1;
    }

    printf("Game main data file(s) written successfully.\nDone.\n");
    return 0;
}
