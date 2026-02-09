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
// AGS sprite file pack/unpack utility.
// 
//=============================================================================
#include "commands.h"
#include "util/cmdlineopts.h"
#include "util/string.h"
#include "util/string_utils.h"

using namespace AGS::Common;


const char *BIN_STRING = "spritepak v0.1.0 - AGS spritefile tool\n"
"Copyright (c) 2026 AGS Team and contributors\n";

const char *HELP_STRING = "Usage:\n"
//------------------------------------------------------------------------------|
"  spritepak <COMMAND> <SPRITEFILE> [<WORK-DIR> | <DEST-SPRITEFILE>] [OPTIONS]\n"
"      executes an operation regarding the chosen sprite file, a working\n"
"      directory OR another destination spritefile. Depending on a command\n"
"      either the spritefile or the directory is an input or an output.\n"
"      Options may adjust the operation further.\n"
"\n"
"Commands:\n"
"  -c, --create           create a spritefile, gathering the files from the\n"
"                         input directory.\n"
"  -e, --export           export (extract) files from the existing spritefile\n"
"                         into the output directory.\n"
"  -l, --list             print spritefile's table of contents.\n"
"  -q, --info             print quick info about spritefile.\n"
"  -w, --rewrite          rewrites spritefile contents into another spritefile,\n"
"                         optionally using different storage options.\n"
"\n"
"Command options:\n"
"  -n, --index <indexfile>\n"
"                         specifies the index file to read or write, depending\n"
"                         respectively on the current command.\n"
"  --out-index <indexfile>\n"
"                         when rewriting a spritefile, specifies new index file.\n"
"  -p, --pattern <file pattern>\n"
"                         when creating the new spritefile, or exporting one,\n"
"                         use the given pattern for individual image files,\n"
"                         either with or without file extension. The pattern\n"
"                         may contain following placeholders:\n"
"                           * %N% - sprite number\n"
"                         If no pattern is provided, the program will use\n"
"                         \"spr%N%\" pattern by default. If neither pattern\n"
"                         nor extension is provided, then \".bmp\" will be used.\n"
// TODO: replace default with PNG after PNG read/write support is added to the engine
"  -s, --storage-flags <flags>\n"
"                         when creating the new spritefile, use additional\n"
"                         storage options, defined using a hexadecimal bitset:\n"
"                           * 0x01 - optimize storage size when possible;\n"
"                             e.g. write 16/32-bit images as 8-bit images with\n"
"                             palette (only when this achieves less space).\n"
"                         default is \"0x01\"\n"
"  -z, --compress <type>  when creating the new spritefile, use compression:\n"
"                           * none\n"
"                           * rle\n"
"                           * lzw\n"
"                           * deflate\n"
"                         default is \"deflate\"\n"
"\n"
"Other options:\n"
"  -v, --verbose          print operation details"
;


int DoCommand(const CmdLineOpts::ParseResult &cmdargs)
{
    // Parse the command
    char command = 0;
    for (const auto &opt : cmdargs.Opt)
    {
        if (opt == "-c" || opt == "--create")
        {
            command = 'c'; // create
            break;
        }
        if (opt == "-e" || opt == "--export")
        {
            command = 'e'; // export
            break;
        }
        if (opt == "-l" || opt == "--list")
        {
            command = 'l'; // list
            break;
        }
        if (opt == "-q" || opt == "--info")
        {
            command = 'q'; // info
            break;
        }
        if (opt == "-w" || opt == "--rewrite")
        {
            command = 'w'; // rewrite
            break;
        }
    }

    // Fixed pos options
    const String sprite_file = cmdargs.PosArgs.size() > 0 ? cmdargs.PosArgs[0] : String();
    const String work_file_or_dir = cmdargs.PosArgs.size() > 1 ? cmdargs.PosArgs[1] : String();

    // TODO: easier way to:
    //  - get either short or long named option;
    //  - get option's value without the search loop in the code
    SpritePak::CommandOptions opts;
    for (const auto &opt_with_value : cmdargs.OptWithValue)
    {
        if (opt_with_value.first == "-n" || opt_with_value.first == "--index")
        {
            opts.IndexFile = opt_with_value.second;
        }
        if (opt_with_value.first == "--out-index")
        {
            opts.OutIndexFile = opt_with_value.second;
        }
        else if (opt_with_value.first == "-p" || opt_with_value.first == "--pattern")
        {
            opts.ImageFilePattern = opt_with_value.second;
        }
        else if (opt_with_value.first == "-s" || opt_with_value.first == "--storage-flags")
        {
            opts.StorageFlags = static_cast<SpritePak::SpriteStorage>(StrUtil::StringToIntHex(opt_with_value.second));
        }
        else if (opt_with_value.first == "-z" || opt_with_value.first == "--compress")
        {
            opts.Compress = SpritePak::CompressionFromName(opt_with_value.second);
        }
    }
    const bool verbose = cmdargs.Opt.count("-v") || cmdargs.Opt.count("--verbose");

    // Init
    SpritePak::Init();

    // Run supported commands
    switch (command)
    {
    case 'c': // create
    {
        if (cmdargs.PosArgs.size() < 2)
            break; // not enough args
        return SpritePak::Command_Create(work_file_or_dir, sprite_file, opts, verbose);
    }
    case 'e': // export
    {
        if (cmdargs.PosArgs.size() < 2)
            break; // not enough args
        return SpritePak::Command_Export(sprite_file, work_file_or_dir, opts, verbose);
    }
    case 'l': // list
    {
        if (cmdargs.PosArgs.size() < 1)
            break; // not enough args
        return SpritePak::Command_List(sprite_file, opts);
    }
    case 'q': // info
    {
        if (cmdargs.PosArgs.size() < 1)
            break; // not enough args
        return SpritePak::Command_Info(sprite_file, opts);
    }
    case 'w': // rewrite
    {
        if (cmdargs.PosArgs.size() < 2)
            break; // not enough args
        return SpritePak::Command_Rewrite(sprite_file, work_file_or_dir, opts, verbose);
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
        { "-n", "-p", "-s", "-z", "--compress", "--index", "--out-index", "--pattern", "--storage-flags",});
    if (cmdargs.HelpRequested)
    {
        printf("%s\n", HELP_STRING);
        return 0; // display help and bail out
    }

    return DoCommand(cmdargs);
}
