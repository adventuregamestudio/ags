//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
// 
// AGS package file pack/unpack utility.
// 
// TODO:
// * append cmdline option (create new file / append to existing)
// * proper unified error codes for the AGS tools?
// * clarify the use of "verbose" option, and make it consistent
//   throughout the operations.
// 
//=============================================================================
#include <regex>
#include "commands.h"
#include "data/include_utils.h"
#include "util/cmdlineopts.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;

const char *BIN_STRING = "agspak v0.3.0 - AGS game packaging tool\n"
    "Copyright (c) 2025 AGS Team and contributors\n";

const char *HELP_STRING = "Usage:\n"
   //--------------------------------------------------------------------------------|
    "  agspak <COMMAND> <PAK-FILE> [<WORK-DIR>] [<FILES>] [OPTIONS]\n"
    "      executes an operation regarding the chosen package file, a working\n"
    "      directory, and an optional files list. Depending on a command either the\n"
    "      pack or the directory is an input or an output.\n"
    "      File list should be a comma-separated list of file names, which may\n"
    "      contain include/exclude patterns. If no file list is provided, then all\n"
    "      the files found in the respective input location (dir or pack) will be\n"
    "      selected for the operation.\n"
    "      Options may adjust the operation further.\n"
    "\n"
    "Commands:\n"
    "  -c, --create           create a pack file, gathering the files from the\n"
    "                         input directory.\n"
    "  -e, --export           export (extract) files from the existing pack file\n"
    "                         into the output directory.\n"
    "  -l, --list             print pack file's contents.\n"
    "\n"
    "Command options:\n"
    "  -f, --pattern-file <file>\n"
    "                         when creating a pack file, use pattern file with the"
    "                         include/exclude patterns\n"
    "  -p, --partition <MB>   when creating a pack file, split asset files between\n"
    "                         partitions of this size max. Input files are not split,\n"
    "                         so files larger than this amount will occupy a single\n"
    "                         partition\n"
    "  -r, --recursive        when creating a pack file, include all subdirectories\n"
    "                         of a working directory too\n"
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
    }

    // Fixed pos options
    const String pak_file = cmdargs.PosArgs.size() > 0 ? cmdargs.PosArgs[0] : String();
    const String work_dir = cmdargs.PosArgs.size() > 1 ? cmdargs.PosArgs[1] : String();
    const String file_list_str = cmdargs.PosArgs.size() > 2 ? cmdargs.PosArgs[2] : String();
    // Common options
    // a include pattern file that should be inside the input-dir
    // TO-DO: support nested include pattern files in input-dir
    String pattern_file;

    // TODO: easier way to:
    //  - get either short or long named option;
    //  - get option's value without the search loop in the code
    size_t part_size_mb = 0;
    for (const auto &opt_with_value : cmdargs.OptWithValue)
    {
        if (opt_with_value.first == "-f" || opt_with_value.first == "--pattern-file")
        {
            pattern_file = opt_with_value.second;
        }
        else if (opt_with_value.first == "-p" || opt_with_value.first == "--partition")
        {
            part_size_mb = StrUtil::StringToInt(opt_with_value.second);
        }
    }
    const bool do_subdirs = cmdargs.Opt.count("-r") || cmdargs.Opt.count("--recursive");
    const bool verbose = cmdargs.Opt.count("-v") || cmdargs.Opt.count("--verbose");

    std::vector<String> pattern_list;
    if (!file_list_str.IsEmpty())
        pattern_list = file_list_str.Split(',');

    // Run supported commands
    switch (command)
    {
    case 'c': // create
        {
            if (cmdargs.PosArgs.size() < 2)
                break; // not enough args
            return AGSPak::Command_Create(work_dir, pak_file, pattern_list, pattern_file, do_subdirs, part_size_mb, verbose);
        }
    case 'e': // export
        {
            if (cmdargs.PosArgs.size() < 2)
                break; // not enough args
            return AGSPak::Command_Export(pak_file, work_dir, pattern_list);
        }
    case 'l': // list
        {
            if (cmdargs.PosArgs.size() < 1)
                break; // not enough args
            return AGSPak::Command_List(pak_file);
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

    CmdLineOpts::ParseResult cmdargs = CmdLineOpts::Parse(argc, argv, {"-p", "-f", "--pattern-file"});
    if (cmdargs.HelpRequested)
    {
        printf("%s\n", HELP_STRING);
        return 0; // display help and bail out
    }

    return DoCommand(cmdargs);
}
