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
// CRM room file pack/unpack utility.
// 
//=============================================================================
#include "commands.h"
#include "util/cmdlineopts.h"
#include "util/string_utils.h"

using namespace AGS::Common;

const char *BIN_STRING = "crmpak v0.2.0 - AGS compiled room's (re)packer\n"
"Copyright (c) 2026 AGS Team and contributors";

const char *HELP_STRING =
   //------------------------------------------------------------------------------|
    "Usage:\n"
    "  crmpak <COMMAND> <ROOM-FILE> [<CONTENT-LIST>] [<OPTIONS>]\n"
    "      executes an operation regarding the chosen room file, and the selected\n"
    "      list of content. Content list is a sequence of key-value pairs separated\n"
    "      by spaces, where the key is a content name and the value is a file path:\n"
    "          CONTENT1 FILE1 CONTENT2 FILE2 ...\n"
    "      Certain commands (such as '-x') may not require specifying file paths,\n"
    "      in which case content list must contain only sequence of content names:\n"
    "          CONTENT1 CONTENT2 ...\n"
    "      Certain other commands (such as '-l') do not require a content list.\n"
    "      Options may adjust the operation further.\n"
    "\n"
    "Commands:\n"
    "  -c, --create           create a room file with specified content;\n"
    "                         if no content list is provided, then will create a\n"
    "                         room with minimal default content.\n"
    // NOTE: we reserve -c input options for the future, in case we want to create from some source
    "  -e, --export           export (extract) content from the existing room file\n"
    "                         into the output file(s).\n"
    "  -i, --import           import (add, overwrite) specified content into the\n"
    "                         existing room file.\n"
    "  -l, --list             print the list of content found inside room file.\n"
    "  -x, --cut              cut (delete) specified content from a room file.\n"
    "\n"
    "Content names:\n"
    "  backN                  background image, where N is a 0-based index;\n"
    "                         supported image formats: BMP, PCX, PNG\n"
    "  mask-hotspot           hotspot mask image\n"
    "  mask-region            region mask image\n"
    "  mask-walkarea          walkable area mask image\n"
    "  mask-walkbehind        walk-begind mask image\n"
    "  script                 compiled script, binary data\n"
    "  script-text            script text (commonly present in v2.x rooms)\n"
    "\n"
    "Command options:\n"
    "  -w <out-room.crm>      for import and cut commands: write the resulting room\n"
    "                         into a new file; otherwise will modify the input file\n"
    "\n"
    "Other options:\n"
    "  -v, --verbose          print operation details"
    ;

CRMPak::Content ParseContent(const String &cont_type, const String &filename)
{
    if (cont_type.CompareLeftNoCase("back") == 0)
    {
        int index = StrUtil::StringToInt(cont_type.Mid(4));
        if (index >= 0)
            return CRMPak::Content(CRMPak::kContent_Background, index, filename);
        return {};
    }

    auto cont = StrUtil::ParseEnum<CRMPak::ContentType, CRMPak::kNumContentTypes>(cont_type,
        CRMPak::GetContentNames(), CRMPak::kContent_Undefined);
    return CRMPak::Content(cont, filename);
}

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
        if (opt == "-i" || opt == "--import")
        {
            command = 'i'; // import
            break;
        }
        if (opt == "-l" || opt == "--list")
        {
            command = 'l'; // list
            break;
        }
        if (opt == "-x" || opt == "--cut")
        {
            command = 'x';
            break;
        }
    }

    const bool is_cmd_with_content = (command == 'c' || command == 'e' || command == 'i' || command == 'x');
    const bool is_cmd_with_cont_files = (command == 'c' || command == 'e' || command == 'i');

    // Fixed pos options
    const String src_room_file = cmdargs.PosArgs.size() > 0 ? cmdargs.PosArgs[0] : String();
    String dst_room_file = src_room_file;

    // Content list
    std::vector<CRMPak::Content> content;
    if (is_cmd_with_content && cmdargs.PosArgs.size() > 1)
    {
        if (is_cmd_with_cont_files)
        {
            if ((cmdargs.PosArgs.size() - 1) % 2 != 0)
                printf("Warning: last content specification is not paired with the filename\n");

            for (size_t i = 1; i < cmdargs.PosArgs.size() - 1; i += 2)
            {
                content.push_back(ParseContent(cmdargs.PosArgs[i], cmdargs.PosArgs[i + 1]));
            }
        }
        else
        {
            for (size_t i = 1; i < cmdargs.PosArgs.size(); ++i)
            {
                content.push_back(ParseContent(cmdargs.PosArgs[i], String()));
            }
        }
    }
    if (is_cmd_with_content && content.size() == 0)
    {
        if (command != 'c')
        {
            printf("Error: no content specified for the command.\n");
            printf("%s\n", HELP_STRING);
            return -1;
        }
    }

    // Options with values
    for (const auto &opt : cmdargs.OptWithValue)
    {
        if (opt.first == "-w")
        {
            dst_room_file = opt.second;
        }
    }

    // Other options
    const bool verbose = cmdargs.Opt.count("-v") || cmdargs.Opt.count("--verbose");

    // Init
    CRMPak::Init();

    // Run supported commands
    switch (command)
    {
    case 'c': // create
    {
        if (cmdargs.PosArgs.size() < 1)
            break; // not enough args
        return CRMPak::Command_Create(dst_room_file, content, verbose);
    }
    case 'e': // export
    {
        if (cmdargs.PosArgs.size() < 3)
            break; // not enough args
        return CRMPak::Command_Export(src_room_file, content, verbose);
    }
    case 'i': // import
    {
        if (cmdargs.PosArgs.size() < 2)
            break; // not enough args
        return CRMPak::Command_Import(src_room_file, dst_room_file, content, verbose);
    }
    case 'l': // list
    {
        if (cmdargs.PosArgs.size() < 1)
            break; // not enough args
        return CRMPak::Command_List(src_room_file);
    }
    case 'x': // cut
    {
        if (cmdargs.PosArgs.size() < 1)
            break; // not enough args
        return CRMPak::Command_Cut(src_room_file, dst_room_file, content, verbose);
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

    CmdLineOpts::ParseResult cmdargs = CmdLineOpts::Parse(argc, argv, {"-w"});
    if (cmdargs.HelpRequested)
    {
        printf("%s\n", HELP_STRING);
        return 0; // display help and bail out
    }

    return DoCommand(cmdargs);
}
