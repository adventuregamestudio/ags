//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
// 
// AGS sprite import utility.
// Parses Game.agf for game sprite specs. Looks up for the source image files,
// loads them up, cuts sprites out, converts to AGS compatible format, and
// optionally either writes individual image files one per sprite into the
// destination directory, or compiles a spritefile.
// 
//=============================================================================
#include "commands.h"
#include "util/cmdlineopts.h"
#include "util/string.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;


const char *BIN_STRING = "spriteimport v0.1.0 - AGS sprite import tool\n"
"Copyright (c) 2026 AGS Team and contributors\n";

const char *HELP_STRING = "Usage:\n"
//------------------------------------------------------------------------------|
"  spriteimport <COMMAND> <GAME.AGF> <OUT-PATH> [<COMMAND-OPTIONS>] [<OPTIONS>]\n"
"      parses the input GAME.AGF file, and writes an output depending on the\n"
"      command and additional options (see below).\n"
"\n"
"Commands:\n"
"  -c, --out-pak    outputs a compiled spritefile. Additional options '-n', '-s'\n"
"                   and '-z' modify this command's behavior.\n"
"  -f, --out-files  outputs sprites as individual image files into the specified\n"
"                   directory. The files's names and format is determined by\n"
"                   the pattern option (see '-p'). If not such pattern provided\n"
"                   then uses \"spr%6N%.png\" by default.\n"
"\n"
"Command options:\n"
"  -n, --index <indexfile>\n"
"                   when output is the spritefile: specifies the accompanying\n"
"                   sprite index file.\n"
"  -p, --pattern <name pattern>\n"
"                   when output is image files: use the given pattern to define\n"
"                   their naming and image format.\n"
"                   The pattern may be given with or without file extension.\n"
"                   The pattern may contain following placeholders:\n"
"                     * %xN% - sprite number, where 'x' may be any single digit\n"
"                              integer specifying number of padding zeroes,\n"
"                              e.g. \"%6N%."
"                   If no pattern is provided, the program will use \"spr%6N%\"\n"
"                   pattern by default. If no extension is specified in the\n"
"                   pattern, then \".png\" will be used as an extension.\n"
// -r, --room-dir   RESERVED: input rooms for 8-bit games (need to read palettes)
"  -s, --storage-flags <flags>\n"
"                   when output is the spritefile: use additional storage\n"
"                   options, defined using a hexadecimal bitset:\n"
"                     * 0x01 - optimize storage size when possible;\n"
"                   e.g. write 16/32-bit images as 8-bit images with palette\n"
"                   (only when this achieves less space). Default is \"0x01\"\n"
"  -z, --compress <type>\n"
"                   when output is the spritefile: use compression:\n"
"                     * none\n"
"                     * rle\n"
"                     * lzw\n"
"                     * deflate\n"
"                   Default is \"deflate\".\n"
"\n"
"Other options:\n"
"  -v, --verbose    print operation details"
;


int DoCommand(const CmdLineOpts::ParseResult &cmdargs)
{
    // Parse the command
    char command = 0;
    for (const auto &opt : cmdargs.Opt)
    {
        if (opt == "-c" || opt == "--out-pak")
        {
            command = 'c'; // compile spritefile
            break;
        }
        if (opt == "-f" || opt == "--out-files")
        {
            command = 'f'; // export as image files
            break;
        }
    }

    // Fixed pos options
    const String src_agf = cmdargs.PosArgs.size() > 0 ? cmdargs.PosArgs[0] : String();
    const String dst_file_or_dir = cmdargs.PosArgs.size() > 1 ? cmdargs.PosArgs[1] : String();

    // TODO: easier way to:
    //  - get either short or long named option;
    //  - get option's value without the search loop in the code
    SpriteImport::CommandOptions opts;
    for (const auto &opt_with_value : cmdargs.OptWithValue)
    {
        if (opt_with_value.first == "-n" || opt_with_value.first == "--index")
        {
            opts.IndexFile = opt_with_value.second;
        }
        else if (opt_with_value.first == "-p" || opt_with_value.first == "--pattern")
        {
            opts.ImageFilePattern = opt_with_value.second;
        }
        else if (opt_with_value.first == "-s" || opt_with_value.first == "--storage-flags")
        {
            opts.StorageFlags = static_cast<SpriteImport::SpriteStorage>(StrUtil::StringToIntHex(opt_with_value.second));
        }
        else if (opt_with_value.first == "-z" || opt_with_value.first == "--compress")
        {
            opts.Compress = DataUtil::CompressionFromName(opt_with_value.second);
        }
    }
    const bool verbose = cmdargs.Opt.count("-v") || cmdargs.Opt.count("--verbose");

    // Init
    SpriteImport::Init();

    // Run supported commands
    switch (command)
    {
    case 'c': // compile spritefile
    {
        if (cmdargs.PosArgs.size() < 2)
            break; // not enough args
        opts.OutputToSpritePak = true;
        return SpriteImport::Command_Import(src_agf, dst_file_or_dir, opts, verbose);
    }
    case 'f': // write as image files
    {
        if (cmdargs.PosArgs.size() < 2)
            break; // not enough args
        opts.OutputToSpritePak = false;
        return SpriteImport::Command_Import(src_agf, dst_file_or_dir, opts, verbose);
    }
    default:
        printf("Error: no valid command is specified\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    printf("Error: not enough arguments\n");
    printf("%s\n", HELP_STRING);
    return -1;
}

int main(int argc, char *argv[])
{
    printf("%s\n", BIN_STRING);

    CmdLineOpts::ParseResult cmdargs = CmdLineOpts::Parse(argc, argv,
        { "-n", "--index", "-p", "--pattern", "-s", "--storage-flags", "-z", "--compress"});
    if (cmdargs.HelpRequested)
    {
        printf("%s\n", HELP_STRING);
        return 0; // display help and bail out
    }

    return DoCommand(cmdargs);
}
